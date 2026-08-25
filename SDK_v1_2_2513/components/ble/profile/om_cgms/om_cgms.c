/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

/*******************************************************************************
 * INCLUDES
 */
#include <string.h>
#include <math.h>
#include "om_cgms.h"

/*********************************************************************
 * MACROS
 */
#define CGMS_GATT_OP_SUCCESS            0x00
#define CGMS_GATT_WRITE_ERROR_IN_PROC   0xFE
#define CGMS_GATT_WRITE_ERROR           0xFF

void service_om_cgms_write_resp(uint8_t gatt_state);
void service_om_cgms_racp_indicate(const uint8_t *data, int len);
void service_om_cgms_ops_ctrl_indicate(const uint8_t *data, int len);
void service_om_cgms_measurement_notify(const uint8_t *data, int len);
static void om_cgms_report_indicate(uint8_t result);

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint8_t cgms_device_type;
static uint8_t cgms_device_location;
static const om_cgms_handlers_t *cgms_handlers;
static void *om_cgms_user_data;

// 非负数来记录Notify计数，负数表示当前上报状态: 上报完成(CGM_RACP_SUCCESS),上报被中断(CGM_RACP_PROCEDURE_NOT_COMPLETED)
static int om_cgms_record_report_idx = -CGM_RACP_SUCCESS;
static uint8_t om_cgms_record_report_type;
static uint16_t om_cgms_record_report_time_offset;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static uint16_t convertFloattoSFLOAT(float value)
{
    int exp = 0;
    while (fabsf(value) < 200) {
        value *= 10, exp--;
    }
    while (fabsf(value) > 2000) {
        value /= 10, exp++;
    }
    return (((uint32_t)exp & 0x0F) << 12) + ((uint32_t)value & 0xFFF);
}

static void om_cgms_measurement_send_done(const om_cgms_measurement_t *data, const om_cgm_state_t *state)
{
    bool res = true;
    om_cgms_measurement_t tmp_data = {0};
    om_cgm_state_t tmp_state = {0};
    if (om_cgms_record_report_idx < 0) {
        if (om_cgms_record_report_idx != -CGM_RACP_SUCCESS) {
            om_cgms_report_indicate(-om_cgms_record_report_idx);
            om_cgms_record_report_idx = -CGM_RACP_SUCCESS;
        }
        return;
    }
    if (data == NULL) {
        data = &tmp_data;
        state = &tmp_state;
        res = cgms_handlers->get_records(om_cgms_record_report_idx, om_cgms_record_report_type,
                                         om_cgms_record_report_time_offset, &tmp_data, &tmp_state,
                                         om_cgms_user_data);
    }
    if (res) {
        // notify records
        uint8_t len = 0x05, flag = 0;

        uint16_t glu_con = convertFloattoSFLOAT(data->glucose_concentration);
        uint8_t notify_resp [16] = {
            0x00, 0x00, // lenght + flag fill later
            glu_con & 0xFF, glu_con >> 8, data->time_offset & 0xFF, data->time_offset >> 8,
        };
        uint8_t *state_byte = (uint8_t *)state;
        #if 0
        for (int i = 0; i < 3; i++) {
            if (state_byte[i]) {
                flag |= 1 << (7 - i); // @ref CGM Measurement - Flags field
                notify_resp[1 + len++] = state_byte[i];
            }
        }
        #else
        for (int i = 0; i < 3; i++) {
            if (state_byte[2 - i]) {
                flag |= 1 << (5 + i); // @ref CGM Measurement - Flags field
                notify_resp[1 + len++] = state_byte[2 - i];
            }
        }
        #endif
        notify_resp[0] = len, notify_resp[1] = flag;
        service_om_cgms_measurement_notify(notify_resp, len + 1);
        if (om_cgms_record_report_type == CGM_RACP_VALUE_FIRST_RECORD ||
                om_cgms_record_report_type == CGM_RACP_VALUE_LAST_RECORD) {
            om_cgms_record_report_idx = -CGM_RACP_SUCCESS;;
        } else {
            om_cgms_record_report_idx++;
        }
    } else {
        om_cgms_record_report_idx = -CGM_RACP_SUCCESS;
    }
    if (om_cgms_record_report_idx < 0) {
        // response RACP
        om_cgms_report_indicate(CGM_RACP_SUCCESS);
    }
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */

void om_cgms_connected_adapt(void)
{
    om_cgms_record_report_idx = -CGM_RACP_SUCCESS;
}

uint8_t om_cgms_read_feature_adapt(uint8_t resp[6])
{
    om_cgm_feature_t def_feature = {
        .featrue.calibration                        = 0,
        .featrue.patient_high_low_alerts            = 0,
        .featrue.hypo_alerts                        = 0,
        .featrue.hyper_alerts                       = 0,
        .featrue.rate_of_inc_dec_alerts             = 0,
        .featrue.device_specific_alert              = 0,
        .featrue.sensor_malfunction_detection       = 0,
        .featrue.sensor_temp_high_low_detection     = 0,
        .featrue.sensor_result_high_low_detection   = 0,
        .featrue.low_battery_detection              = 0,
        .featrue.sensor_type_error_detection        = 0,
        .featrue.general_device_fault               = 0,
        .featrue.e2e_crc                            = 0,
        .featrue.multiple_bond                      = 0,
        .featrue.multiple_sessions                  = 0,
        .featrue.trend_information                  = 0,
        .featrue.quality                            = 0,
        .featrue.rsv                                = 0,
        .cgm_type                                   = cgms_device_type,
        .cgm_location                               = cgms_device_location,
        .e2e_crc                                    = 0xFFFF
    };
    memcpy(resp, &def_feature, sizeof(def_feature));
    return CGMS_GATT_OP_SUCCESS;
}

uint8_t om_cgms_read_session_start_time_adapt(uint8_t resp[11], uint8_t *crc_valid)
{
    om_cgm_time_t cgm_time;
    int8_t time_zone;
    uint8_t dst_offset;
    uint16_t e2e_crc;
    cgms_handlers->get_session_start_time(&cgm_time, &time_zone, &dst_offset, om_cgms_user_data);
    e2e_crc = 0xFFFF;
    *crc_valid = false;
    memcpy(resp, &cgm_time, sizeof(om_cgm_time_t));
    memcpy(&resp[7], &time_zone, sizeof(uint8_t));
    memcpy(&resp[8], &dst_offset, sizeof(uint8_t));
    memcpy(&resp[9], &e2e_crc, sizeof(uint16_t));
    return CGMS_GATT_OP_SUCCESS;
}

uint8_t om_cgms_write_session_start_time_adapt(const uint8_t *data, int len)
{
    // size(9/11) = year(2) + month(1) + day(1) + hour(1) + min(1) + sec(1) + time_zone(1) + DST-offset(1) + [0 / crc(2)]
    if (len >= 9) {
        om_cgm_time_t cgm_time;
        memcpy(&cgm_time, data, sizeof(om_cgm_time_t));
        int8_t time_zone = data[7];
        uint8_t dst_offset = data[8];
        if (len >= 11) {
            uint16_t e2e_crc = data[9] + (data[10] << 8);
            if (e2e_crc) {
                // e2e_crc not support
            }
            return CGMS_GATT_WRITE_ERROR;
        }
        if (cgm_time.month == 0 || cgm_time.month > 12 || cgm_time.day == 0 || cgm_time.day > 31) {
            return CGMS_GATT_WRITE_ERROR;
        }
        if (cgm_time.hours > 23 || cgm_time.minutes > 59 || cgm_time.seconds > 59) {
            return CGMS_GATT_WRITE_ERROR;
        }
        if (!((-48 <= time_zone && time_zone <= +56) || time_zone == -128)) {
            return CGMS_GATT_WRITE_ERROR;
        }
        if (dst_offset != 0 && dst_offset != 2 && dst_offset != 4 && dst_offset != 8 && dst_offset != 0xFF) {
            return CGMS_GATT_WRITE_ERROR;
        }
        cgms_handlers->set_session_start_time(&cgm_time, time_zone, dst_offset, om_cgms_user_data);
    }
    return CGMS_GATT_OP_SUCCESS;
}

uint8_t om_cgms_read_session_run_time_adapt(uint8_t resp[4], uint8_t *crc_valid)
{
    uint16_t run_time, e2e_crc;
    run_time = cgms_handlers->read_session_run_time(om_cgms_user_data);
    e2e_crc = 0xFFFF;
    *crc_valid = false;
    memcpy(&resp, &run_time, sizeof(uint16_t));
    memcpy(&resp[sizeof(uint16_t)], &e2e_crc, sizeof(uint16_t));
    return CGMS_GATT_OP_SUCCESS;
}

uint8_t om_cgms_read_state_adapt(uint8_t resp[7], uint8_t *crc_valid)
{
    om_cgm_state_t status = {0};
    uint16_t time_offset = 0;
    uint16_t e2e_crc = 0xFFFF;
    cgms_handlers->get_status(&status, &time_offset, om_cgms_user_data);
    *crc_valid = false;
    memcpy(resp, &time_offset, sizeof(uint16_t));
    memcpy(&resp[sizeof(uint16_t)], &status, sizeof(om_cgm_state_t));
    memcpy(&resp[sizeof(uint16_t) + sizeof(om_cgm_state_t)], &e2e_crc, sizeof(uint16_t));
    return CGMS_GATT_OP_SUCCESS;
}

void om_cgms_write_racp_adapt(const uint8_t *data, int len)
{
    if (len < 2) {
        service_om_cgms_write_resp(CGMS_GATT_WRITE_ERROR);
    }
    uint8_t opcode = data[0];
    uint8_t operator = data[1];
    if (opcode == CGM_RACP_OPCODE_REPORT_STORED_RECORDS && om_cgms_record_report_idx >= 0) {
        service_om_cgms_write_resp(CGMS_GATT_WRITE_ERROR_IN_PROC); // procedure already in progress
        return;
    }

    service_om_cgms_write_resp(CGMS_GATT_OP_SUCCESS);
    switch (opcode) {
        case CGM_RACP_OPCODE_REPORT_STORED_RECORDS: {
            switch (operator) {
                case CGM_RACP_VALUE_RSV: {
                    uint8_t resp[] = { CGM_RACP_OPCODE_RESPONSE_CODE, 0x00, opcode, CGM_RACP_INVALID_OPERATOR };
                    service_om_cgms_racp_indicate(resp, sizeof(resp));
                    break;
                }
                case CGM_RACP_VALUE_ALL_RECORDS: {
                    if (len != 2) {
                        uint8_t resp[] = { CGM_RACP_OPCODE_RESPONSE_CODE, 0x00, opcode, CGM_RACP_INVALID_OPERAND };
                        service_om_cgms_racp_indicate(resp, sizeof(resp));
                        break;
                    }
                    // init data
                    om_cgms_record_report_idx = 0;
                    om_cgms_record_report_type = CGM_RACP_VALUE_ALL_RECORDS;
                    om_cgms_record_report_time_offset = 0;
                    // trigger notify send
                    om_cgms_measurement_t meas_data = {0};
                    om_cgm_state_t state = {0};
                    bool res = cgms_handlers->get_records(om_cgms_record_report_idx, om_cgms_record_report_type,
                                                          om_cgms_record_report_time_offset, &meas_data, &state,
                                                          om_cgms_user_data);
                    if (res) {
                        om_cgms_measurement_send_done(&meas_data, &state);
                    } else {
                        om_cgms_report_indicate(CGM_RACP_NO_RECORDS_FOUND);
                        om_cgms_record_report_idx = -CGM_RACP_SUCCESS;
                    }
                    break;
                }
                case CGM_RACP_VALUE_LAST_RECORD: {
                    if (len != 2) {
                        uint8_t resp[] = { CGM_RACP_OPCODE_RESPONSE_CODE, 0x00, opcode, CGM_RACP_INVALID_OPERAND };
                        service_om_cgms_racp_indicate(resp, sizeof(resp));
                        break;
                    }
                    // init data
                    om_cgms_record_report_idx = 0;
                    om_cgms_record_report_type = CGM_RACP_VALUE_LAST_RECORD;
                    om_cgms_record_report_time_offset = 0;
                    // trigger notify send
                    om_cgms_measurement_t meas_data = {0};
                    om_cgm_state_t state = {0};
                    bool res = cgms_handlers->get_records(om_cgms_record_report_idx, om_cgms_record_report_type,
                                                          om_cgms_record_report_time_offset, &meas_data, &state,
                                                          om_cgms_user_data);
                    if (res) {
                        om_cgms_measurement_send_done(&meas_data, &state);
                    } else {
                        om_cgms_report_indicate(CGM_RACP_NO_RECORDS_FOUND);
                        om_cgms_record_report_idx = -CGM_RACP_SUCCESS;
                    }
                    break;
                }
                case CGM_RACP_VALUE_GREATER_EQUAL: {
                    if ((len != 5) || data[2] != 0x01) {
                        uint8_t resp[] = { CGM_RACP_OPCODE_RESPONSE_CODE, 0x00, opcode, CGM_RACP_OPERAND_NOT_SUPPORTED };
                        service_om_cgms_racp_indicate(resp, sizeof(resp));
                        break;
                    }
                    // init data
                    om_cgms_record_report_idx = 0;
                    om_cgms_record_report_type = CGM_RACP_VALUE_GREATER_EQUAL;
                    om_cgms_record_report_time_offset = data[3] + (data[4] << 8);
                    // trigger notify send
                    om_cgms_measurement_t meas_data = {0};
                    om_cgm_state_t state = {0};
                    bool res = cgms_handlers->get_records(om_cgms_record_report_idx, om_cgms_record_report_type,
                                                          om_cgms_record_report_time_offset, &meas_data, &state,
                                                          om_cgms_user_data);
                    if (res) {
                        om_cgms_measurement_send_done(&meas_data, &state);
                    } else {
                        om_cgms_report_indicate(CGM_RACP_NO_RECORDS_FOUND);
                        om_cgms_record_report_idx = -CGM_RACP_SUCCESS;
                    }
                    break;
                }
                default: {
                    // other operator not support
                    uint8_t resp[] = { CGM_RACP_OPCODE_RESPONSE_CODE, 0x00, opcode, CGM_RACP_OPERATOR_NOT_SUPPORTED };
                    service_om_cgms_racp_indicate(resp, sizeof(resp));
                    break;
                }
            }
            break;
        }
        case CGM_RACP_OPCODE_ABORT_OPERATION: {
            uint8_t resp[] = {
                CGM_RACP_OPCODE_RESPONSE_CODE, 0x00, CGM_RACP_OPCODE_ABORT_OPERATION, CGM_RACP_SUCCESS
            };
            service_om_cgms_racp_indicate(resp, sizeof(resp));
            if (om_cgms_record_report_idx >= 0) {
                om_cgms_record_report_idx = -CGM_RACP_SUCCESS;
            }
            break;
        }
        case CGM_RACP_OPCODE_REPORT_RECORDS_NUMBER: {
            // get number of record
            if (!((operator == CGM_RACP_VALUE_ALL_RECORDS) || (operator == CGM_RACP_VALUE_GREATER_EQUAL && len == 5))) {
                uint8_t resp[] = { CGM_RACP_OPCODE_RESPONSE_CODE, 0x00, opcode, CGM_RACP_INVALID_OPERATOR };
                service_om_cgms_racp_indicate(resp, sizeof(resp));
                break;
            }
            uint16_t time_offset;
            if (operator == CGM_RACP_VALUE_ALL_RECORDS) {
                time_offset = 0;
            } else if (operator == CGM_RACP_VALUE_GREATER_EQUAL) {
                time_offset = data[3] + (data[4] << 8);
            } else {
                // not support
                time_offset = 0;
            }
            uint16_t num = cgms_handlers->get_records_number(operator, time_offset, om_cgms_user_data);
            // response RACP
            uint8_t resp[] = { CGM_RACP_OPCODE_RECORDS_NUMBER_RESPONSE, 0x00, num & 0xFF, num >> 8 };
            service_om_cgms_racp_indicate(resp, sizeof(resp));
            break;
        }
        case CGM_RACP_OPCODE_DELETE_STORED_RECORDS: {
            // no break;
        }
        default: {
            uint8_t resp[] = { CGM_RACP_OPCODE_RESPONSE_CODE, 0x00, opcode, CGM_RACP_OPCODE_NOT_SUPPORTED};
            service_om_cgms_racp_indicate(resp, sizeof(resp));
            break;
        }
    }
}

void om_cgms_write_ops_ctrl_adapt(const uint8_t *data, int len)
{
    if (len == 0) {
        service_om_cgms_write_resp(CGMS_GATT_WRITE_ERROR);
    }
    service_om_cgms_write_resp(CGMS_GATT_OP_SUCCESS);
    uint8_t opcode = data[0];
    switch (opcode) {
        case CGM_OPSC_SET_COMM_INTV: {
            if (len < 2) {
                break;
            }
            uint8_t interval = data[1];
            bool res = cgms_handlers->set_comm_interval(interval, om_cgms_user_data);
            uint8_t resp[] = { CGM_OPSC_RESPONSE_CODE, opcode, res ? CGM_OPSR_SUCCESS : CGM_OPSR_PARAMETER_OUT_OF_RANGE };
            service_om_cgms_ops_ctrl_indicate(resp, sizeof(resp));
            break;
        }
        case CGM_OPSC_GET_COMM_INTV: {
            uint8_t interval;
            cgms_handlers->get_comm_interval(&interval, om_cgms_user_data);
            uint8_t resp[] = { CGM_OPSC_CGM_COMM_INTV_RESP, interval };
            service_om_cgms_ops_ctrl_indicate(resp, sizeof(resp));
            break;
        }
        default: {
            uint8_t resp[] = { CGM_OPSC_RESPONSE_CODE, opcode, CGM_OPSR_OPCODE_NOT_SUPPORTED };
            service_om_cgms_ops_ctrl_indicate(resp, sizeof(resp));
            break;
        }
    }
}

static void om_cgms_report_indicate(uint8_t result)
{
    uint8_t resp[] = { CGM_RACP_OPCODE_RESPONSE_CODE, 0x00, CGM_RACP_OPCODE_REPORT_STORED_RECORDS, result};
    service_om_cgms_racp_indicate(resp, sizeof(resp));
}

void om_cgms_measurement_send_done_adapt(void)
{
    om_cgms_measurement_send_done(NULL, NULL);
}

void om_cgms_init(const om_cgms_config_t *config, const om_cgms_handlers_t *cbs, void *user_data)
{
    cgms_device_type = config->device_type;
    cgms_device_location = config->device_location;
    cgms_handlers = cbs;
    om_cgms_user_data = user_data;
    om_cgms_record_report_idx = -CGM_RACP_SUCCESS;
}

/** @} */
