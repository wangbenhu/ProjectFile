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
#include "service_om_cgms.h"
#include "om_cgms.h"
#include "om_cgms_type.h"
#include "omble.h"

/*********************************************************************
 * MACROS
 */
#define CGMS_SERV_UUID                  0x181F
#define CGMS_MEASUREMENT_UUID           0x2AA7
#define CGMS_FEATURE_UUID               0x2AA8
#define CGMS_STATUS_UUID                0x2AA9
#define CGMS_SESSION_RUN_TIME_UUID      0x2AAB
#define CGMS_SESSION_START_TIME_UUID    0x2AAA
#define RECORD_ACCESS_POINT_UUID        0x2A52
#define CGMS_OPS_CONTROL_UUID           0x2AAC

#define CGMS_GAP_CONN_INVALID           0xFF
#define CGMS_NOTIFY_ID                  0xA7

/*********************************************************************
 * TYPEDEFS
 */
enum {
    IDX_CGMS_SVC,
    IDX_CGMS_MEASUREMENT_CHAR,
    IDX_CGMS_MEASUREMENT_VAL,
    IDX_CGMS_MEASUREMENT_DESC,
    IDX_CGMS_FEATURE_CHAR,
    IDX_CGMS_FEATURE_VAL,
    IDX_CGMS_STATUS_CHAR,
    IDX_CGMS_STATUS_VAL,
    IDX_CGMS_SESSION_START_TIME_CHAR,
    IDX_CGMS_SESSION_START_TIME_VAL,
    IDX_CGMS_SESSION_RUN_TIME_CHAR,
    IDX_CGMS_SESSION_RUN_TIME_VAL,
    IDX_RECORD_ACCESS_POINT_CHAR,
    IDX_RECORD_ACCESS_POINT_VAL,
    IDX_RECORD_ACCESS_POINT_DESC,
    IDX_CGMS_OPS_CONTROL_CHAR,
    IDX_CGMS_OPS_CONTROL_VAL,
    IDX_CGMS_OPS_CONTROL_DESC,
};

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint8_t  m_conn_idx;
static uint16_t m_start_handle;
static uint16_t m_measurement_desc;
static uint16_t m_rec_acc_desc;
static uint16_t m_ops_control_desc;
static uint8_t m_pending_racp_data[64]; // len + data
static uint8_t m_pending_ops_data[64]; // len + data

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
void service_om_cgms_evt_cb(uint16_t evt_id, const omble_evt_t *evt)
{
    if (evt_id == OB_GAP_EVT_CONNECTED) {
        m_conn_idx = evt->gap.conn_idx;
        om_cgms_connected_adapt();
    } else if (evt_id == OB_GAP_EVT_DISCONNECTED) {
        m_conn_idx = CGMS_GAP_CONN_INVALID;
        m_pending_racp_data[0] = 0;
    } else if (evt_id == OB_GATTS_EVT_READ_REQ) {
        const ob_gatts_evt_read_req_t *req = &evt->gatt.read_req;
        if (req->att_hdl == m_start_handle + IDX_CGMS_MEASUREMENT_DESC) {
            ob_gatts_read_response(evt->gatt.conn_idx, OB_GATT_ERR_NO_ERROR,
                                   (uint8_t *)&m_measurement_desc, sizeof(m_measurement_desc));
        } else if (req->att_hdl == m_start_handle + IDX_RECORD_ACCESS_POINT_DESC) {
            ob_gatts_read_response(evt->gatt.conn_idx, OB_GATT_ERR_NO_ERROR,
                                   (uint8_t *)&m_rec_acc_desc, sizeof(m_rec_acc_desc));
        } else if (req->att_hdl == m_start_handle + IDX_CGMS_OPS_CONTROL_DESC) {
            ob_gatts_read_response(evt->gatt.conn_idx, OB_GATT_ERR_NO_ERROR,
                                   (uint8_t *)&m_ops_control_desc, sizeof(m_ops_control_desc));
        } else if (req->att_hdl == m_start_handle + IDX_CGMS_FEATURE_VAL) {
            uint8_t resp[6];
            uint8_t gatt_resp = om_cgms_read_feature_adapt(resp);
            ob_gatts_read_response(evt->gatt.conn_idx, gatt_resp, resp, sizeof(om_cgm_feature_t));
        } else if (req->att_hdl == m_start_handle + IDX_CGMS_STATUS_VAL) {
            uint8_t crc_valid, resp[7];
            uint8_t gatt_resp = om_cgms_read_state_adapt(resp, &crc_valid);
            ob_gatts_read_response(evt->gatt.conn_idx, gatt_resp, (uint8_t *)resp,
                                   sizeof(uint16_t) + sizeof(om_cgm_state_t) + !!crc_valid * sizeof(uint16_t));
        } else if (req->att_hdl == m_start_handle + IDX_CGMS_SESSION_START_TIME_VAL) {
            uint8_t crc_valid, resp[11];
            uint8_t gatt_resp = om_cgms_read_session_start_time_adapt(resp, &crc_valid);
            ob_gatts_read_response(evt->gatt.conn_idx, gatt_resp, (uint8_t *)resp, 9 + !!crc_valid * sizeof(uint16_t));
        } else if (req->att_hdl == m_start_handle + IDX_CGMS_SESSION_RUN_TIME_VAL) {
            uint8_t crc_valid, resp[4];
            uint8_t gatt_resp = om_cgms_read_session_run_time_adapt(resp, &crc_valid);
            ob_gatts_read_response(evt->gatt.conn_idx, gatt_resp, (uint8_t *)resp,
                                   sizeof(uint16_t) + !!crc_valid * sizeof(uint16_t));
        }
    } else if (evt_id == OB_GATTS_EVT_WRITE_REQ) {
        const ob_gatts_evt_write_req_t *req = &evt->gatt.write_req;
        if (req->att_hdl == m_start_handle + IDX_CGMS_MEASUREMENT_DESC) {
            ob_gatts_write_response(evt->gatt.conn_idx, OB_GATT_ERR_NO_ERROR);
            m_measurement_desc = evt->gatt.write_req.data[0];
        } else if (req->att_hdl == m_start_handle + IDX_RECORD_ACCESS_POINT_DESC) {
            ob_gatts_write_response(evt->gatt.conn_idx, OB_GATT_ERR_NO_ERROR);
            m_rec_acc_desc = evt->gatt.write_req.data[0];
        } else if (req->att_hdl == m_start_handle + IDX_CGMS_OPS_CONTROL_DESC) {
            ob_gatts_write_response(evt->gatt.conn_idx, OB_GATT_ERR_NO_ERROR);
            m_ops_control_desc = evt->gatt.write_req.data[0];
        } else if (req->att_hdl == m_start_handle + IDX_CGMS_SESSION_START_TIME_VAL) {
            uint8_t gatt_resp = om_cgms_write_session_start_time_adapt(req->data, req->len);
            ob_gatts_write_response(evt->gatt.conn_idx, gatt_resp);
        } else if (req->att_hdl == m_start_handle + IDX_RECORD_ACCESS_POINT_VAL) {
            if (m_measurement_desc == 0) {
                ob_gatts_write_response(evt->gatt.conn_idx, 0xFD);
                return;
            }
            om_cgms_write_racp_adapt(req->data, req->len);
        } else if (req->att_hdl == m_start_handle + IDX_CGMS_OPS_CONTROL_VAL) {
            if (m_measurement_desc == 0) {
                ob_gatts_write_response(evt->gatt.conn_idx, 0xFD);
                return;
            }
            om_cgms_write_ops_ctrl_adapt(req->data, req->len);
        }
    } else if (evt_id == OB_GATTS_EVT_INDICATE_CFM) {
        if (evt->gatt.indicate_cfm.att_hdl == m_start_handle + IDX_RECORD_ACCESS_POINT_VAL && m_pending_racp_data[0] > 0) {
            ob_gatts_hvx_t hvx = {
                OB_HANDLE_VALUE_IND,
                m_start_handle + IDX_RECORD_ACCESS_POINT_VAL, // handle
                &m_pending_racp_data[1],
                m_pending_racp_data[0],
            };
            ob_gatts_send_hvx(m_conn_idx, &hvx);
            m_pending_racp_data[0] = 0;
        }
        if (evt->gatt.indicate_cfm.att_hdl == m_start_handle + IDX_CGMS_OPS_CONTROL_VAL && m_pending_ops_data[0] > 0) {
            ob_gatts_hvx_t hvx = {
                OB_HANDLE_VALUE_IND,
                m_start_handle + IDX_CGMS_OPS_CONTROL_VAL, // handle
                &m_pending_ops_data[1],
                m_pending_ops_data[0],
            };
            ob_gatts_send_hvx(m_conn_idx, &hvx);
            m_pending_ops_data[0] = 0;
        }
    } else if (evt_id == OB_GATT_EVT_TX_COMPLETE) {
        if (evt->gatt.tx_complete.id == CGMS_NOTIFY_ID) {
            om_cgms_measurement_send_done_adapt();
        }
    }
}

void service_om_cgms_racp_indicate(const uint8_t *data, int len)
{
    if (m_conn_idx != CGMS_GAP_CONN_INVALID) {
        ob_gatts_hvx_t hvx = {
            OB_HANDLE_VALUE_IND,
            m_start_handle + IDX_RECORD_ACCESS_POINT_VAL, // handle
            data,
            len,
        };
        uint8_t res = ob_gatts_send_hvx(m_conn_idx, &hvx);
        if (res != OB_GATT_ERR_NO_ERROR) {
            len = len < (int)sizeof(m_pending_racp_data) - 1 ? len : (int)sizeof(m_pending_racp_data) - 1;
            m_pending_racp_data[0] = len;
            memcpy(&m_pending_racp_data[1], data, len);
        }
    }
}

void service_om_cgms_ops_ctrl_indicate(const uint8_t *data, int len)
{
    if (m_conn_idx != CGMS_GAP_CONN_INVALID) {
        ob_gatts_hvx_t hvx = {
            OB_HANDLE_VALUE_IND,
            m_start_handle + IDX_CGMS_OPS_CONTROL_VAL, // handle
            data,
            len,
        };
        uint8_t res = ob_gatts_send_hvx(m_conn_idx, &hvx);
        if (res != OB_GATT_ERR_NO_ERROR) {
            len = len < (int)sizeof(m_pending_ops_data) - 1 ? len : (int)sizeof(m_pending_ops_data) - 1;
            m_pending_ops_data[0] = len;
            memcpy(&m_pending_ops_data[1], data, len);
        }
    }
}

void service_om_cgms_measurement_notify(const uint8_t *data, int len)
{
    if (m_conn_idx != CGMS_GAP_CONN_INVALID) {
        ob_gatts_hvx_t hvx = {
            OB_HANDLE_VALUE_NTF,
            m_start_handle + IDX_CGMS_MEASUREMENT_VAL, // handle
            data,
            len,
            CGMS_NOTIFY_ID,
        };
        ob_gatts_send_hvx(m_conn_idx, &hvx);
    }
}

void service_om_cgms_write_resp(uint8_t gatt_state)
{
    ob_gatts_write_response(m_conn_idx, gatt_state);
}

void om_cgms_service_init(void)
{
    static const uint8_t serv_esls[2]  = {CGMS_SERV_UUID & 0xFF,            CGMS_SERV_UUID >> 8};
    static const uint8_t char1[2] = {CGMS_MEASUREMENT_UUID & 0xFF,          CGMS_MEASUREMENT_UUID >> 8};
    static const uint8_t char2[2] = {CGMS_FEATURE_UUID & 0xFF,              CGMS_FEATURE_UUID >> 8};
    static const uint8_t char3[2] = {CGMS_STATUS_UUID & 0xFF,               CGMS_STATUS_UUID >> 8};
    static const uint8_t char4[2] = {CGMS_SESSION_START_TIME_UUID & 0xFF,   CGMS_SESSION_START_TIME_UUID >> 8};
    static const uint8_t char5[2] = {CGMS_SESSION_RUN_TIME_UUID & 0xFF,     CGMS_SESSION_RUN_TIME_UUID >> 8};
    static const uint8_t char6[2] = {RECORD_ACCESS_POINT_UUID & 0xFF,       RECORD_ACCESS_POINT_UUID >> 8};
    static const uint8_t char7[2] = {CGMS_OPS_CONTROL_UUID & 0xFF,          CGMS_OPS_CONTROL_UUID >> 8};
    static const ob_gatt_item_t atts_dfu[] = {
        { ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
        { char1,           OB_UUID_16BIT, OB_ATT_PROP_NTF },
        { ob_att_cccd_def, OB_UUID_16BIT, OB_ATT_PROP_READ | OB_ATT_PROP_WRITE },
        { ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
        { char2,           OB_UUID_16BIT, OB_ATT_PROP_READ },
        { ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
        { char3,           OB_UUID_16BIT, OB_ATT_PROP_READ },
        { ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
        { char4,           OB_UUID_16BIT, OB_ATT_PROP_READ | OB_ATT_PROP_WRITE },
        { ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
        { char5,           OB_UUID_16BIT, OB_ATT_PROP_READ },
        { ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
        { char6,           OB_UUID_16BIT, OB_ATT_PROP_IND | OB_ATT_PROP_WRITE },
        { ob_att_cccd_def, OB_UUID_16BIT, OB_ATT_PROP_READ | OB_ATT_PROP_WRITE },
        { ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
        { char7,           OB_UUID_16BIT, OB_ATT_PROP_IND | OB_ATT_PROP_WRITE },
        { ob_att_cccd_def, OB_UUID_16BIT, OB_ATT_PROP_READ | OB_ATT_PROP_WRITE },
    };
    static const ob_gatt_serv_t att_serv_dfu = {
        serv_esls, OB_UUID_16BIT,
        sizeof(atts_dfu) / sizeof(atts_dfu[0]), atts_dfu
    };
    ob_gatts_add_service(&att_serv_dfu, &m_start_handle);
}

void service_om_cgms_init(void)
{
    om_cgms_service_init();
    ob_event_callback_reg(service_om_cgms_evt_cb);
}

/** @} */
