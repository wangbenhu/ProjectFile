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
#include "service_om_bms.h"
#include "omble.h"
#include "om_bms.h"

#define BMS_SERV_UUID                   0x181E
#define BMS_CTRL_POINT                  0x2AA4
#define BMS_FEATURE                     0x2AA5

enum {
    IDX_BMS_SVC,
    IDX_CTRL_POINT_CHAR,
    IDX_CTRL_POINT_VAL,
    IDX_FEATURE_CHAR,
    IDX_FEATURE_VAL,
};
/*********************************************************************
 * LOCAL VARIABLES
 */
static uint16_t m_start_handle;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void service_om_bms_evt_cb(uint16_t evt_id, const omble_evt_t *evt)
{
    if (evt_id == OB_GATTS_EVT_WRITE_REQ) {
        const ob_gatts_evt_write_req_t *req = &evt->gatt.write_req;
        if (req->att_hdl == m_start_handle + IDX_CTRL_POINT_VAL) {
            uint8_t res = om_bms_write_ops_ctrl_adapt(req->data, req->len);
            ob_gatts_write_response(evt->gatt.conn_idx, res);
        }
    } else if (evt_id == OB_GATTS_EVT_READ_REQ) {
        const ob_gatts_evt_read_req_t *req = &evt->gatt.read_req;
        if (req->att_hdl == m_start_handle + IDX_FEATURE_VAL) {
            uint8_t resp_data[3];
            om_bms_read_feature_adapt(resp_data);
            ob_gatts_read_response(evt->gatt.conn_idx, OB_GATT_ERR_NO_ERROR, (uint8_t *)resp_data, sizeof(resp_data));
        }
    }
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
void om_bms_service_init(void)
{
    static const uint8_t serv_bms[2]  = {0x1e, 0x18};
    static const uint8_t char_ctrl_point[2] = {0xa4, 0x2a};
    static const uint8_t char_feature[2] = {0xa5, 0x2a};
    static const ob_gatt_item_t atts_dev[] = {
        { ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
        { char_ctrl_point, OB_UUID_16BIT, OB_ATT_PROP_WRITE },
        { ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
        { char_feature,     OB_UUID_16BIT, OB_ATT_PROP_READ },
    };
    static const ob_gatt_serv_t att_serv = {
        serv_bms, OB_UUID_16BIT,
        sizeof(atts_dev) / sizeof(atts_dev[0]), atts_dev
    };
    ob_gatts_add_service(&att_serv, &m_start_handle);
}

void service_om_bms_init(void)
{
    om_bms_service_init();
    ob_event_callback_reg(service_om_bms_evt_cb);
}

/** @} */
