/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @version
 * Version V20210119.1.0
 *  - Initial release
 *
 * @{
 */
/*******************************************************************************
 * INCLUDES
 */
#include <string.h>
#include "omble.h"
#include "om_dfu.h"
#include "service_om_dfu.h"

/*********************************************************************
 * MACROS
 */
#define PROP_DFU_W_N  (ATT_PROP_WRITE | ATT_PROP_NTF)
#define PROP_DFU_WRN  (ATT_PROP_WRITE | ATT_PROP_READ  | ATT_PROP_NTF)
#define PROP_DFU_W_R  (ATT_PROP_WRITE | ATT_PROP_READ)
#define ATT_LEN_CCCD  sizeof(uint16_t)
#define DFU_VER_SIZE  (sizeof(DFU_APP_DESCRIBE)-1)
/*********************************************************************
 * LOCAL VARIABLES
 */
// Start handle
static uint16_t m_start_handle;

#define NOT_END 0xFF
#define END_RESET 0xFE
static uint8_t m_end_state = NOT_END;

enum {
    IDX_DFU_SVC,
    IDX_DFU_CTRL_CHAR,
    IDX_DFU_CTRL_VAL,
    IDX_DFU_CTRL_DESC,
    IDX_DFU_PKG_CHAR,
    IDX_DFU_PKG_VAL,
    IDX_DFU_VER_CHAR,
    IDX_DFU_VER_VAL,
    IDX_DFU_VER_DESC,
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void service_om_dfu_init(void)
{
    // static const uint8_t serv_dfu[16] = {0x41, 0x54, 0x4F, 0x2D, 0x4D, 0x4F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5F, 0xBF, 0x01, 0x00, 0x00};
    // static const uint8_t char1[16] = {0x41, 0x54, 0x4F, 0x2D, 0x4D, 0x4F, 0x00, 0x00, 0x00, 0x00, 0x01, 0x5F, 0xBF, 0x01, 0x00, 0x00};
    // static const uint8_t char2[16] = {0x41, 0x54, 0x4F, 0x2D, 0x4D, 0x4F, 0x00, 0x00, 0x00, 0x00, 0x02, 0x5F, 0xBF, 0x01, 0x00, 0x00};
    // static const uint8_t char3[16] = {0x41, 0x54, 0x4F, 0x2D, 0x4D, 0x4F, 0x00, 0x00, 0x00, 0x00, 0x03, 0x5F, 0xBF, 0x01, 0x00, 0x00};

    static const uint8_t serv_dfu[2] = {0x59, 0xFE};
    static const uint8_t char1[16] = {0x50, 0xEA, 0xDA, 0x30, 0x88, 0x83, 0xB8, 0x9F, 0x60, 0x4F, 0x15, 0xF3, 0x01, 0x00, 0xC9, 0x8E};
    static const uint8_t char2[16] = {0x50, 0xEA, 0xDA, 0x30, 0x88, 0x83, 0xB8, 0x9F, 0x60, 0x4F, 0x15, 0xF3, 0x02, 0x00, 0xC9, 0x8E};
    static const uint8_t char3[2] = {0x03, 0x00};

    static const ob_gatt_item_t atts_dfu[] = {
        { ob_att_char_def, OB_UUID_16BIT,  OB_ATT_PROP_READ },
        { char1,           OB_UUID_128BIT, OB_ATT_PROP_NTF | OB_ATT_PROP_WRITE },
        { ob_att_cccd_def, OB_UUID_16BIT,  OB_ATT_PROP_READ | OB_ATT_PROP_WRITE },
        { ob_att_char_def, OB_UUID_16BIT,  OB_ATT_PROP_READ },
        { char2,           OB_UUID_128BIT, OB_ATT_PROP_WRITE_CMD },
        { ob_att_char_def, OB_UUID_16BIT,  OB_ATT_PROP_READ },
        { char3,           OB_UUID_16BIT, OB_ATT_PROP_READ | OB_ATT_PROP_WRITE | OB_ATT_PROP_NTF },
        { ob_att_cccd_def, OB_UUID_16BIT,  OB_ATT_PROP_READ | OB_ATT_PROP_WRITE },
    };
    static const ob_gatt_serv_t att_serv_dfu = {
        serv_dfu, OB_UUID_16BIT,
        sizeof(atts_dfu) / sizeof(atts_dfu[0]), atts_dfu
    };
    ob_gatts_add_service(&att_serv_dfu, &m_start_handle);
}

static void dfu_response(uint8_t conn_idx,dfu_response_t *dfu_rsp_data)
{
    uint16_t len = dfu_rsp_data->length;
    ob_gatts_hvx_t hvx = {
        OB_HANDLE_VALUE_NTF,
        m_start_handle + IDX_DFU_CTRL_VAL, // handle
        &dfu_rsp_data->rsp_code,
        len,
    };
    ob_gatts_send_hvx(conn_idx, &hvx);
}

static void dfu_response_version(uint8_t conn_idx, dfu_version_t *dfu_version)
{
    uint16_t len = sizeof(struct dfu_version_data);
    ob_gatts_hvx_t hvx = {
        OB_HANDLE_VALUE_NTF,
        m_start_handle + IDX_DFU_VER_VAL, // handle
        (uint8_t *) &dfu_version->version_data,
        len,
    };
    ob_gatts_send_hvx(conn_idx, &hvx);
}

void service_om_dfu_evt_cb(uint16_t evt_id, const omble_evt_t *evt)
{
    if (evt_id == OB_GATTS_EVT_WRITE_REQ) {
        const uint8_t *data = evt->gatt.write_req.data;
        uint16_t len = evt->gatt.write_req.len;
        dfu_response_t dfu_rsp_data = {0};
        switch (evt->gatt.write_req.att_hdl - m_start_handle) {
            case IDX_DFU_CTRL_VAL:
                dfu_write_cmd((uint8_t *)data, len, &dfu_rsp_data);
                dfu_response(evt->gatt.conn_idx, &dfu_rsp_data);
                break;
            case IDX_DFU_PKG_VAL:
                dfu_write_data(data, len, &dfu_rsp_data);
                if (dfu_rsp_data.length != DFU_RESP_SIZE_NO_DATA) {
                    dfu_response(evt->gatt.conn_idx, &dfu_rsp_data);
                }
                break;
            case IDX_DFU_VER_VAL: {
                dfu_version_t version;
                uint32_t cmd = 0xFFFFFFFF;
                if (len == 4) {
                    cmd = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + (data[3]);
                } else if (len == 1) {
                    cmd = data[0];
                }
                dfu_write_version_char(cmd, &version);
                dfu_response_version(evt->gatt.conn_idx, &version);
            }   break;
        }
    } else if (evt_id == OB_GAP_EVT_CONNECTED) {
        m_end_state = NOT_END;
    } else if (evt_id == OB_GAP_EVT_DISCONNECTED) {
        dfu_reset(DFU_UPDATE_ST_DISCONNECT);
        if (m_end_state == DFU_UPDATE_ST_SUCCESS) {
            drv_pmu_reset(PMU_REBOOT_FROM_SOFT_RESET);
        }
    } else if (evt_id == OB_GATT_EVT_TX_COMPLETE) {
        if (m_end_state != NOT_END) {
            app_om_dfu_update_end_ind_handler(m_end_state, NULL);
        }
    }
}

static void dfu_begin_ind_handler(uint8_t status, void *p)
{
    app_om_dfu_update_start_ind_handler(status, p);
}

static void dfu_prog_ind_handler(uint8_t status, void *p)
{
    app_om_dfu_update_prog_ind_handler(status, p);
}

static void dfu_end_ind_handler(uint8_t status, void *p)
{
    m_end_state = status;
}

uint8_t dfu_user_check_handler(uint8_t img_type, uint32_t img_size, uint32_t img_version)
{
    return app_om_dfu_user_check_ind_handler(img_type, img_size, img_version);
}

const dfu_cb_itf_t dfu_cb_itf = {
    dfu_begin_ind_handler,
    dfu_prog_ind_handler,
    dfu_end_ind_handler,
    dfu_user_check_handler,
};


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
__WEAK void app_om_dfu_update_start_ind_handler(uint8_t status, void *p)
{
    ob_gap_conn_params_t param = {0x0c, 0, 500};
    ob_gap_conn_param_update(0, &param);
}
__WEAK void app_om_dfu_update_prog_ind_handler(uint8_t status, void *p) {}
__WEAK void app_om_dfu_update_end_ind_handler(uint8_t status, void *p)
{
    ob_gap_disconnect(0, 0x13);
}

__WEAK uint8_t app_om_dfu_user_check_ind_handler(uint8_t img_type, uint32_t img_size, uint32_t img_version)
{
    if (img_type == IMAGE_TYPE_MBR_USR1) {
        om_error_t upgrade_check(uint32_t upgrade_img_ver);
        if (OM_ERROR_OK != upgrade_check(img_version)) {
            return DFU_VERSION_NOT_MATCH;
        }
    }
    return DFU_SUCCESS;
}

void app_om_dfu_init(void)
{
    service_om_dfu_init();
    ob_event_callback_reg(service_om_dfu_evt_cb);
}

/** @} */
