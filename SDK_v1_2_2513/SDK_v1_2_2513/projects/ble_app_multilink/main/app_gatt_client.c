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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "omble.h"
#include "om_driver.h"
#include "app_common.h"


/*******************************************************************************
 * MACROS
 */
#define printf(...) om_log(OM_LOG_INFO, ##__VA_ARGS__)


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief  BLE event process callback
 * @param[in] evt_id  event id
 * @param[in] evt     event parameters
 *******************************************************************************
 */
static void app_gatt_event_cb(uint16_t evt_id, const omble_evt_t *evt)
{
    if (evt_id == OB_GAP_EVT_CONNECTED) {
        // conn_idx = evt->gap.conn_idx;
    } else if (evt_id == OB_GAP_EVT_DISCONNECTED) {
    } else if (evt_id == OB_GAP_EVT_ADV_STATE_CHANGED) {
    } else if (evt_id == OB_GATT_EVT_MTU_EXCHANGED) {
    } else if (evt_id == OB_GATTC_EVT_FIND_SERV_RSP) {
        const ob_gattc_evt_find_serv_rsp_t *rsp = &evt->gatt.find_serv_rsp;
        if (rsp->status == OB_GATT_ERR_NO_ERROR) {
            for (int i = 0; i < rsp->service_num; i++) {
                log_debug("Service: 0x%04X - 0x%04X  ", rsp->service[i].start_hdl, rsp->service[i].end_hdl);
                if (rsp->service[i].uuid_len == 2) {
                    log_debug("0x%04X\n", rsp->service[i].uuid[0] + (rsp->service[i].uuid[1] << 8));
                } else {
                    log_debug_array(rsp->service[i].uuid, rsp->service[i].uuid_len);
                    log_debug("\n");
                }
            }
        } else if (rsp->status == OB_GATT_ERR_ATTRIBUTE_NOT_FOUND) {
            log_debug("No GATT service found.\n");
        }
    } else if (evt_id == OB_GATTC_EVT_FIND_SERV_BY_UUID_RSP) {
        const ob_gattc_evt_find_serv_by_uuid_rsp_t *rsp = &evt->gatt.find_serv_by_uuid_rsp;
        if (rsp->status == OB_GATT_ERR_NO_ERROR) {
            for (int i = 0; i < rsp->service_num; i++) {
                log_debug("Service: 0x%04X - 0x%04X  ", rsp->service[i].start_hdl, rsp->service[i].end_hdl);
                if (rsp->service[i].uuid_len == 2) {
                    log_debug("0x%04X\n", rsp->service[i].uuid[0] + (rsp->service[i].uuid[1] << 8));
                } else {
                    log_debug_array(rsp->service[i].uuid, rsp->service[i].uuid_len);
                    log_debug("\n");
                }
            }
        } else if (rsp->status == OB_GATT_ERR_ATTRIBUTE_NOT_FOUND) {
            log_debug("No GATT service found.\n");
        }
    } else if (evt_id == OB_GATTC_EVT_FIND_CHAR_RSP) {
        const ob_gattc_evt_find_char_rsp_t *rsp = &evt->gatt.find_char_rsp;
        if (rsp->status == OB_GATT_ERR_NO_ERROR) {
            for (int i = 0; i < rsp->char_num; i++) {
                log_debug("Char Value Handle: 0x%02X\n", rsp->characteristic[i].value_hdl);
                log_debug("Char UUID:  ");
                if (rsp->characteristic[i].uuid_len == 2) {
                    log_debug("0x%04X\n", rsp->characteristic[i].uuid[0] + (rsp->characteristic[i].uuid[1] << 8));
                } else {
                    log_debug_array(rsp->characteristic[i].uuid, rsp->characteristic[i].uuid_len);
                    log_debug("\n");
                }
                log_debug("Char Prop: %02X\n", rsp->characteristic[i].properties);
            }
        } else if (rsp->status == OB_GATT_ERR_ATTRIBUTE_NOT_FOUND) {
            log_debug("No GATT characteristic found.\n");
        }
    } else if (evt_id == OB_GATTC_EVT_FIND_DESC_RSP) {
        const ob_gattc_evt_find_desc_rsp_t *rsp = &evt->gatt.find_desc_rsp;
        if (rsp->status == OB_GATT_ERR_NO_ERROR) {
            for (int i = 0; i < rsp->desc_num; i++) {
                log_debug("Desc Handle: 0x%02X\n", rsp->descriptor[i].att_hdl);
                log_debug("Desc UUID:  ");
                if (rsp->descriptor[i].uuid_len == 2) {
                    log_debug("0x%04X\n", rsp->descriptor[i].uuid[0] + (rsp->descriptor[i].uuid[1] << 8));
                } else {
                    log_debug_array(rsp->descriptor[i].uuid, rsp->descriptor[i].uuid_len);
                    log_debug("\n");
                }
            }
        } else if (rsp->status == OB_GATT_ERR_ATTRIBUTE_NOT_FOUND) {
            log_debug("No GATT descriptor found.\n");
        }
    } else if (evt_id == OB_GATTC_EVT_READ_RSP) {
        const ob_gattc_evt_read_rsp_t *rsp = &evt->gatt.read_rsp;
        if (rsp->status == OB_GATT_ERR_NO_ERROR) {
            log_debug("GATT read by handle success: ");
            log_debug_array(rsp->data, rsp->len);
            log_debug("\n");
        } else {
            log_debug("GATT read by handle failed: 0x%02X.\n", rsp->status);
        }
    } else if (evt_id == OB_GATTC_EVT_READ_BY_UUID_RSP) {
        const ob_gattc_evt_read_by_uuid_rsp_t *rsp = &evt->gatt.read_by_uuid_rsp;
        if (rsp->status == OB_GATT_ERR_NO_ERROR) {
            log_debug("GATT read by uuid success(hdl:0x%02X): ", rsp->att_hdl);
            log_debug_array(rsp->data, rsp->len);
            log_debug("\n");
        } else {
            log_debug("GATT read by uuid failed: 0x%02X.\n", rsp->status);
        }
    } else {
    }
}


#define DUMP(d, l) do { for(int i=0;i<l;i++) printf("%02X ", d[i]); printf("\n"); } while(0)
static uint16_t disc_all_service_flag, group_st_hdl, group_en_hdl, char_en_hdl, desc_st_hdl, desc_en_hdl;
static uint32_t app_gatt_client_disc_all_service(uint8_t conn_idx, uint16_t start_hdl, uint16_t end_hdl, uint8_t state)
{
    // ob_gattc_mtu_req(conn_idx);
    disc_all_service_flag = true;
    return ob_gattc_find_service_by_handle(conn_idx, 0x0001, 0xFFFF);
}
static char *get_uuid_str(uint16_t uuid)
{
    const static struct uuid_str_t {
        int num;
        char *str;
    } uuid_str[] = {
        { 0x1800, "Generic Access" },
        { 0x1801, "Generic Attribute" },
        { 0x1804, "Tx Power" },
        { 0x1805, "Current Time" },
        { 0x1808, "Glucose" },
        { 0x1809, "Health Thermometer" },
        { 0x180A, "Device Information" },
        { 0x180D, "Heart Rate" },
        { 0x180F, "Battery" },
        { 0x1810, "Blood Pressure" },
        { 0x1811, "Alert Notification" },
        { 0x1812, "Human Interface Device" },
        { 0x1813, "Scan Parameters" },
        { 0x1827, "Mesh Provisioning" },
        { 0x1828, "Mesh Proxy" },
        { 0x2A00, "Device Name" },
        { 0x2A01, "Appearance" },
        { 0x2A02, "Peripheral Privacy Flag" },
        { 0x2A03, "Reconnection Address" },
        { 0x2A04, "Peripheral Preferred Connection Parameters" },
        { 0x2A05, "Service Changed" },
        { 0x2A07, "Tx Power Level" },
        { 0x2A08, "Date Time" },
        { 0x2A09, "Day of Week" },
        { 0x2A0A, "Day Date Time" },
        { 0x2A0C, "Exact Time 256" },
        { 0x2A0E, "Time Zone" },
        { 0x2A0F, "Local Time Information" },
        { 0x2A18, "Glucose Measurement" },
        { 0x2A19, "Battery Level" },
        { 0x2A21, "Measurement Interval" },
        { 0x2A22, "Boot Keyboard Input Report" },
        { 0x2A23, "System ID" },
        { 0x2A24, "Model Number String" },
        { 0x2A25, "Serial Number String" },
        { 0x2A26, "Firmware Revision String" },
        { 0x2A27, "Hardware Revision String" },
        { 0x2A28, "Software Revision String" },
        { 0x2A29, "Manufacturer Name String" },
        { 0x2A2A, "IEEE 11073-20601 Regulatory Certification Data List" },
        { 0x2A2B, "Current Time" },
        { 0x2A32, "Boot Keyboard Output Report" },
        { 0x2A33, "Boot Mouse Input Report" },
        { 0x2A34, "Glucose Measurement Context" },
        { 0x2A35, "Blood Pressure Measurement" },
        { 0x2A37, "Heart Rate Measurement" },
        { 0x2A38, "Body Sensor Location" },
        { 0x2A39, "Heart Rate Control Point" },
        { 0x2A3F, "Alert Status" },
        { 0x2A4A, "HID Information" },
        { 0x2A4B, "Report Map" },
        { 0x2A4C, "HID Control Point" },
        { 0x2A4D, "Report" },
        { 0x2A4E, "Protocol Mode" },
        { 0x2A4F, "Scan Interval Window" },
        { 0x2A50, "PnP ID" },
        { 0x2A51, "Glucose Feature" },
        { 0x2A6D, "Pressure" },
        { 0x2A6E, "Temperature" },
        { 0x2A6F, "Humidity" },
        { 0x2AA6, "Central Address Resolution" },
        { 0x2AA7, "CGM Measurement" },
        { 0x2AA8, "CGM Feature" },
        { 0x2AA9, "CGM Status" },
        { 0x2AAA, "CGM Session Start Time" },
        { 0x2AAB, "CGM Session Run Time" },
        { 0x2AAC, "CGM Specific Ops Control Point" },
        { 0x2ADB, "Mesh Provisioning Data In" },
        { 0X2ADC, "Mesh Provisioning Data Out" },
        { 0X2ADD, "Mesh Proxy Data In" },
        { 0X2ADE, "Mesh Proxy Data Out" },
        { 0x2800, "Primary Service" },
        { 0x2801, "Secondary Service" },
        { 0x2802, "Include" },
        { 0x2803, "Characteristic" },
        { 0x2900, "Characteristic Extended Properties" },
        { 0x2901, "Characteristic User Description" },
        { 0x2902, "Client Characteristic Configuration" },
        { 0x2903, "Server Characteristic Configuration" },
        { 0x2904, "Characteristic Presentation Format" },
        { 0x2905, "Characteristic Aggregate Format" },
        { 0x2906, "Valid Range" },
        { 0x2907, "External Report Reference" },
        { 0x2908, "Report Reference" },
        { 0xFEBA, "Tencent Holdings Limited" },
        { 0xFEB2, "Microsoft Corporation" },
        { 0xFE95, "Xiaomi Inc." },
        { 0xFE59, "Nordic Semiconductor ASA" },
        { 0xFDAB, "Xiaomi Inc." },
        { 0xFDAA, "Xiaomi Inc." },
    };
    for (size_t i = 0; i < sizeof(uuid_str) / sizeof(uuid_str[0]); i++) {
        if (uuid == uuid_str[i].num) {
            return uuid_str[i].str;
        }
    }
    return "-";
}

static void dump_service(const ob_gatt_service_t *serv)
{
    printf("[0x%04X - 0x%04X] UUID: ", serv->start_hdl, serv->end_hdl);
    if (serv->uuid_len == 2) {
        uint16_t uuid = serv->uuid[0] + (serv->uuid[1] << 8);
        printf("0x%04X | %s\n", uuid, get_uuid_str(uuid));

    } else {
        DUMP(serv->uuid, serv->uuid_len);
    }
}

static void dump_character(int conn_idx, const ob_gatt_characteristic_t *character)
{
    printf("[0x%04X]     CHAR UUID: ", character->value_hdl - 1);
    if (character->uuid_len == 2) {
        uint16_t uuid = character->uuid[0] + (character->uuid[1] << 8);
        printf("0x%04X | %s\n", uuid, get_uuid_str(uuid));
    } else {
        DUMP(character->uuid, character->uuid_len);
    }
    printf("[0x%04X]          PROP:   0x%02X | ", character->value_hdl, character->properties);
    char prop[64] = { '\0' };
    strcat(prop, (character->properties & OB_ATT_PROP_IND) == OB_ATT_PROP_IND ? "IND " : "");
    strcat(prop, (character->properties & OB_ATT_PROP_NTF) == OB_ATT_PROP_NTF ? "NTF " : "");
    strcat(prop, (character->properties & OB_ATT_PROP_WRITE) == OB_ATT_PROP_WRITE ? "WRITE_REQ " : "");
    strcat(prop, (character->properties & OB_ATT_PROP_WRITE_CMD) == OB_ATT_PROP_WRITE_CMD ? "WRITE_CMD " : "");
    strcat(prop, (character->properties & OB_ATT_PROP_READ) == OB_ATT_PROP_READ ? "READ " : "");
    printf("%s", prop);
    if ((character->properties & OB_ATT_PROP_READ) == OB_ATT_PROP_READ) {
        ob_gattc_read(conn_idx, character->value_hdl, 0);
    } else {
        printf("\n");
    }
}

static void dump_descriptor(const ob_gatt_descriptor_t *desc)
{
    printf("[0x%04X]          DESC: ", desc->att_hdl);
    if (desc->uuid_len == 2) {
        uint16_t uuid = desc->uuid[0] + (desc->uuid[1] << 8);
        printf("0x%04X | %s\n", uuid, get_uuid_str(uuid));
    } else {
        DUMP(desc->uuid, desc->uuid_len);
    }
}
void dump_read_data(const ob_gattc_evt_read_rsp_t *rsp)
{
    if (rsp->status == 0) {
        int dump_len = rsp->len <= 20 ? rsp->len : 20, string_flag = 1;
        for (int i = 0; i < dump_len; i++) {
            if ((rsp->data[i] < ' ' || rsp->data[i] > '~') && !(i == dump_len - 1 && rsp->data[i] == '\0')) {
                string_flag = 0;
                break;
            }
        }
        char *suffix = dump_len == 20 ? "..." : "";
        if (dump_len >= 3 && string_flag) {
            char output[21] = { '\0' };
            strncpy(output, (char *)rsp->data, dump_len);
            printf(" | \"%s\"%s\n", output, suffix);
        } else {
            printf(" | { ");
            for (int i = 0; i < dump_len; i++) {
                printf("%02X ", rsp->data[i]);
            }
            printf( "%s}\n", suffix);
        }
    } else if ((rsp->status & 0xFF) == 0x05) {
        printf("| { Read Insufficient Authentication }\n");
    } else {
        printf("| { Read Failed 0x%02X }\n", rsp->status & 0xFF);
    }
}

static void app_gatt_client_disc_all_event_cb(uint16_t evt_id, const omble_evt_t *evt)
{
    if (!disc_all_service_flag) {
        return;
    }
    if (evt_id == OB_GATTC_EVT_FIND_SERV_RSP) {
        const ob_gattc_evt_find_serv_rsp_t *rsp = &evt->gatt.find_serv_rsp;
        if (rsp->status == OB_GATT_ERR_NO_ERROR) {
            group_st_hdl = rsp->service[0].start_hdl, group_en_hdl = rsp->service[0].end_hdl;
            dump_service(&rsp->service[0]);
            char_en_hdl = group_st_hdl;
            desc_st_hdl = group_st_hdl, desc_en_hdl = group_en_hdl;
            ob_gattc_find_characteristic(evt->gatt.conn_idx, group_st_hdl, group_en_hdl);
        } else if (rsp->status == OB_GATT_ERR_ATTRIBUTE_NOT_FOUND) {
            printf("Done...\n");
            disc_all_service_flag = false;
        }
    } else if (evt_id == OB_GATTC_EVT_FIND_CHAR_RSP) {
        const ob_gattc_evt_find_char_rsp_t *rsp = &evt->gatt.find_char_rsp;
        if (rsp->status == OB_GATT_ERR_NO_ERROR) {
            if (rsp->characteristic[0].value_hdl - 1 > char_en_hdl + 1) {
                desc_st_hdl = char_en_hdl + 1;
                desc_en_hdl = rsp->characteristic[0].value_hdl - 2;
                char_en_hdl = desc_en_hdl;
                ob_gattc_find_descriptor(evt->gatt.conn_idx, desc_st_hdl, desc_en_hdl);
            } else if (rsp->characteristic[0].value_hdl == group_en_hdl) {
                dump_character(evt->gatt.conn_idx, &rsp->characteristic[0]);
                ob_gattc_find_service_by_handle(evt->gatt.conn_idx, group_en_hdl + 1, 0xFFFF);
            } else if (rsp->characteristic[0].value_hdl == group_en_hdl - 1) {
                dump_character(evt->gatt.conn_idx, &rsp->characteristic[0]);
                desc_st_hdl = desc_en_hdl = group_en_hdl;
                char_en_hdl = desc_en_hdl;
                ob_gattc_find_descriptor(evt->gatt.conn_idx, desc_st_hdl, desc_en_hdl);
            } else {
                dump_character(evt->gatt.conn_idx, &rsp->characteristic[0]);
                char_en_hdl = rsp->characteristic[0].value_hdl;
                desc_st_hdl = char_en_hdl + 1, desc_en_hdl = group_en_hdl;
                ob_gattc_find_characteristic(evt->gatt.conn_idx, char_en_hdl + 1, group_en_hdl);
            }
        } else if (rsp->status == OB_GATT_ERR_ATTRIBUTE_NOT_FOUND) {
            char_en_hdl = desc_en_hdl;
            ob_gattc_find_descriptor(evt->gatt.conn_idx, desc_st_hdl, desc_en_hdl);
        }
    } else if (evt_id == OB_GATTC_EVT_FIND_DESC_RSP) {
        const ob_gattc_evt_find_desc_rsp_t *rsp = &evt->gatt.find_desc_rsp;
        if (rsp->status == OB_GATT_ERR_NO_ERROR) {
            for (int i = 0; i < rsp->desc_num; i++) {
                dump_descriptor(&rsp->descriptor[i]);
            }
            if (rsp->descriptor[rsp->desc_num - 1].att_hdl < desc_en_hdl) {
                desc_st_hdl = rsp->descriptor[rsp->desc_num - 1].att_hdl;
                ob_gattc_find_descriptor(evt->gatt.conn_idx, desc_st_hdl, desc_en_hdl);
            } else {
                if (char_en_hdl < group_en_hdl) {
                    ob_gattc_find_characteristic(evt->gatt.conn_idx, char_en_hdl + 1, group_en_hdl);
                } else {
                    ob_gattc_find_service_by_handle(evt->gatt.conn_idx, group_en_hdl + 1, 0xFFFF);
                }
            }
        } else if (rsp->status == OB_GATT_ERR_ATTRIBUTE_NOT_FOUND) {
            if (char_en_hdl < group_en_hdl) {
                ob_gattc_find_characteristic(evt->gatt.conn_idx, char_en_hdl + 1, group_en_hdl);
            } else {
                ob_gattc_find_service_by_handle(evt->gatt.conn_idx, group_en_hdl + 1, 0xFFFF);
            }
        }
    } else if (evt_id == OB_GATTC_EVT_READ_RSP) {
        const ob_gattc_evt_read_rsp_t *rsp = &evt->gatt.read_rsp;
        dump_read_data(rsp);
    } else if (evt_id == OB_GATT_EVT_MTU_EXCHANGED) {
        // att_mtu = evt->gatt.mtu_exchanged.mtu;
    }
    ob_event_abort();
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
uint32_t app_gatt_discover_service(uint8_t conn_idx, uint16_t start_handle, uint16_t end_handle)
{
    return ob_gattc_find_service_by_handle(conn_idx, start_handle, end_handle);
}

uint32_t app_gatt_discover_service_uuid(uint8_t conn_idx, const uint8_t *uuid, uint16_t uuid_len)
{
    return ob_gattc_find_service_by_uuid(conn_idx, 0x0001, 0xFFFF, uuid, uuid_len);
}

uint32_t app_gatt_discover_char(uint8_t conn_idx, uint16_t start_handle, uint16_t end_handle)
{
    return ob_gattc_find_characteristic(conn_idx, start_handle, end_handle);
}

uint32_t app_gatt_discover_desc(uint8_t conn_idx, uint16_t start_handle, uint16_t end_handle)
{
    return ob_gattc_find_descriptor(conn_idx, start_handle, end_handle);
}

uint32_t app_gatt_read(uint8_t conn_idx, uint16_t handle)
{
    return ob_gattc_read(conn_idx, handle, 0);
}

uint32_t app_gatt_read_uuid(uint8_t conn_idx, const uint8_t *uuid, uint16_t uuid_len)
{
    return ob_gattc_read_by_uuid(conn_idx, 1, 0xFFFF, uuid, uuid_len);
}

uint32_t app_gatt_write(uint8_t conn_idx, uint16_t handle,  uint8_t type, const uint8_t *data, uint16_t len)
{
    return ob_gattc_write(conn_idx, handle, type, data, len);
}

uint32_t app_gatt_client_discover_all(uint8_t conn_idx)
{
    return app_gatt_client_disc_all_service(conn_idx, 0x0001, 0xFFFF, 0);
}

void app_gatt_client_init(void)
{
    ob_event_callback_reg(app_gatt_client_disc_all_event_cb);
    ob_event_callback_reg(app_gatt_event_cb);
}


/** @} */
