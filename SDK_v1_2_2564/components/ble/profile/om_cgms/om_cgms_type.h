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
#ifndef __OM_CGMS_TYPE_H__
#define __OM_CGMS_TYPE_H__

#include <stdint.h>

/*********************************************************************
 * TYPEDEFS
 */
enum cgm_type {
    CGM_TYPE_CAPILLARY_WHOLE_BLOOD      = 0x1,
    CGM_TYPE_CAPILLARY_PLASMA           = 0x2,
    CGM_TYPE_VENOUS_WHOLE_BLOOD         = 0x3,
    CGM_TYPE_VENOUS_PLASMA              = 0x4,
    CGM_TYPE_ARTERIAL_WHOLE_BLOOD       = 0x5,
    CGM_TYPE_ARTERIAL_PLASMA            = 0x6,
    CGM_TYPE_UNDETERMINED_WHOLE_BLOOD   = 0x7,
    CGM_TYPE_UNDETERMINED_PLASMA        = 0x8,
    CGM_TYPE_INTERSTITIAL_FLUID         = 0x9,
    CGM_TYPE_CONTROL_SOLUTION           = 0xA,
};

enum cgm_location {
    CGM_LOCATION_FINGER                 = 0x1,
    CGM_LOCATION_AST                    = 0x2, // AST: ALTERNATE SITE TEST
    CGM_LOCATION_EARLOBE                = 0x3,
    CGM_LOCATION_CONTROL_SOLUTION       = 0x4,
    CGM_LOCATION_SUBCUTANEOUS_TISSUE    = 0x5,
    CGM_LOCATION_NOT_AVAILABLE          = 0xF,
};

enum cgm_racp_opcode {
    CGM_RACP_OPCODE_REPORT_STORED_RECORDS    = 0x01,
    CGM_RACP_OPCODE_DELETE_STORED_RECORDS    = 0x02,
    CGM_RACP_OPCODE_ABORT_OPERATION          = 0x03,
    CGM_RACP_OPCODE_REPORT_RECORDS_NUMBER    = 0x04,
    CGM_RACP_OPCODE_RECORDS_NUMBER_RESPONSE  = 0x05,
    CGM_RACP_OPCODE_RESPONSE_CODE            = 0x06,
};

enum cgm_racp_op_value {
    CGM_RACP_VALUE_RSV                       = 0x00,
    CGM_RACP_VALUE_ALL_RECORDS               = 0x01,
    CGM_RACP_VALUE_LESS_EQUAL                = 0x02,
    CGM_RACP_VALUE_GREATER_EQUAL             = 0x03,
    CGM_RACP_VALUE_WITHIN_RANGE              = 0x04,
    CGM_RACP_VALUE_FIRST_RECORD              = 0x05,
    CGM_RACP_VALUE_LAST_RECORD               = 0x06,
};

enum cgm_racp_resp {
    CGM_RACP_SUCCESS                         = 0x01,
    CGM_RACP_OPCODE_NOT_SUPPORTED            = 0x02,
    CGM_RACP_INVALID_OPERATOR                = 0x03,
    CGM_RACP_OPERATOR_NOT_SUPPORTED          = 0x04,
    CGM_RACP_INVALID_OPERAND                 = 0x05,
    CGM_RACP_NO_RECORDS_FOUND                = 0x06,
    CGM_RACP_ABORT_UNSUCCESSFUL              = 0x07,
    CGM_RACP_PROCEDURE_NOT_COMPLETED         = 0x08,
    CGM_RACP_OPERAND_NOT_SUPPORTED           = 0x09,
};


enum cgm_ops_ctrl {
    CGM_OPSC_RESERVED_FOR_FUTURE_USE         = 0x00,
    CGM_OPSC_SET_COMM_INTV                   = 0x01,
    CGM_OPSC_GET_COMM_INTV                   = 0x02,
    CGM_OPSC_CGM_COMM_INTV_RESP              = 0x03,
    CGM_OPSC_SET_GLUCOSE_CALIB_VALUE         = 0x04,
    CGM_OPSC_GET_GLUCOSE_CALIB_VALUE         = 0x05,
    CGM_OPSC_GLUCOSE_CALIB_VALUE_RESP        = 0x06,
    CGM_OPSC_SET_PATIENT_HIGH_ALERT_LEVEL    = 0x07,
    CGM_OPSC_GET_PATIENT_HIGH_ALERT_LEVEL    = 0x08,
    CGM_OPSC_PATIENT_HIGH_ALERT_LEVEL_RESP   = 0x09,
    CGM_OPSC_SET_PATIENT_LOW_ALERT_LEVEL     = 0x0A,
    CGM_OPSC_GET_PATIENT_LOW_ALERT_LEVEL     = 0x0B,
    CGM_OPSC_PATIENT_LOW_ALERT_LEVEL_RESP    = 0x0C,
    CGM_OPSC_SET_HYPO_ALERT_LEVEL            = 0x0D,
    CGM_OPSC_GET_HYPO_ALERT_LEVEL            = 0x0E,
    CGM_OPSC_HYPO_ALERT_LEVEL_RESP           = 0x0F,
    CGM_OPSC_SET_HYPER_ALERT_LEVEL           = 0x10,
    CGM_OPSC_GET_HYPER_ALERT_LEVEL           = 0x11,
    CGM_OPSC_HYPER_ALERT_LEVEL_RESP          = 0x12,
    CGM_OPSC_SET_RATE_OF_DEC_ALERT_LEVEL     = 0x13,
    CGM_OPSC_GET_RATE_OF_DEC_ALERT_LEVEL     = 0x14,
    CGM_OPSC_RATE_OF_DEC_ALERT_LEVEL_RESP    = 0x15,
    CGM_OPSC_SET_RATE_OF_INC_ALERT_LEVEL     = 0x16,
    CGM_OPSC_GET_RATE_OF_INC_ALERT_LEVEL     = 0x17,
    CGM_OPSC_RATE_OF_INC_ALERT_LEVEL_RESP    = 0x18,
    CGM_OPSC_RESET_DEVICE_SPECIFIC_ALERT     = 0x19,
    CGM_OPSC_START_THE_SESSION               = 0x1A,
    CGM_OPSC_STOP_THE_SESSION                = 0x1B,
    CGM_OPSC_RESPONSE_CODE                   = 0x1C,
};

enum cgm_ops_resp {
    CGM_OPSR_RFU                             = 0x00,
    CGM_OPSR_SUCCESS                         = 0x01,
    CGM_OPSR_OPCODE_NOT_SUPPORTED            = 0x02,
    CGM_OPSR_INVALID_OPERAND                 = 0x03,
    CGM_OPSR_PROCEDURE_NOT_COMPLETED         = 0x04,
    CGM_OPSR_PARAMETER_OUT_OF_RANGE          = 0x05,
};

enum cgm_dst_offset {
    CGM_DST_OFF_STANDARD         = 0x00,
    CGM_DST_OFF_0_5_HOUR         = 0x02,
    CGM_DST_OFF_1_HOUR           = 0x04,
    CGM_DST_OFF_2_HOUR           = 0x08,
    CGM_DST_OFF_UNKNOWN          = 0xFF,
};

typedef struct om_cgm_feature {
    struct {
        uint8_t calibration                         : 1;
        uint8_t patient_high_low_alerts             : 1;
        uint8_t hypo_alerts                         : 1;
        uint8_t hyper_alerts                        : 1;
        uint8_t rate_of_inc_dec_alerts              : 1;
        uint8_t device_specific_alert               : 1;
        uint8_t sensor_malfunction_detection        : 1;
        uint8_t sensor_temp_high_low_detection      : 1;
        uint8_t sensor_result_high_low_detection    : 1;
        uint8_t low_battery_detection               : 1;
        uint8_t sensor_type_error_detection         : 1;
        uint8_t general_device_fault                : 1;
        uint8_t e2e_crc                             : 1;
        uint8_t multiple_bond                       : 1;
        uint8_t multiple_sessions                   : 1;
        uint8_t trend_information                   : 1;
        uint8_t quality                             : 1;
        uint8_t rsv                                 : 7;
    } featrue;
    uint8_t cgm_type                                : 4; ///< @ref enum cgm_type
    uint8_t cgm_location                            : 4; ///< @ref enum cgm_location
    uint16_t e2e_crc;
} om_cgm_feature_t;

typedef struct om_cgm_time {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
} om_cgm_time_t;

typedef struct om_cgm_state {
    uint8_t session_stopped                     : 1;
    uint8_t device_battery_low                  : 1;
    uint8_t sensor_type_incorrect               : 1;
    uint8_t sensor_malfunction                  : 1;
    uint8_t device_specific_alert               : 1;
    uint8_t general_device_fault_in_sensor      : 1;
    uint8_t rsv_6                               : 1;
    uint8_t rsv_7                               : 1;
    uint8_t time_sync_required                  : 1;
    uint8_t calibration_not_allowed             : 1;
    uint8_t calibration_recommended             : 1;
    uint8_t calibration_required                : 1;
    uint8_t sensor_temp_too_high                : 1;
    uint8_t sensor_temp_too_low                 : 1;
    uint8_t calibration_pending                 : 1;
    uint8_t rsv_15                              : 1;
    uint8_t sensor_result_lower_than_patient    : 1;
    uint8_t sensor_result_higher_than_patient   : 1;
    uint8_t sensor_result_lower_than_hypo       : 1;
    uint8_t sensor_result_higher_than_hyper     : 1;
    uint8_t sensor_rate_of_decrease_exceeded    : 1;
    uint8_t sensor_rate_of_increase_exceeded    : 1;
    uint8_t sensor_result_lower_than_process    : 1;
    uint8_t sensor_result_higher_than_process   : 1;
} om_cgm_state_t;

#endif /* __OM_CGMS_TYPE_H__ */

/** @} */
