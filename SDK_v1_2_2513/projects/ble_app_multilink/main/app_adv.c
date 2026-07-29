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
#include "app_common.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint8_t sdata[] = {
    0x02, 0x01, 0x06,
    0x09, 0x09,
    'O', 'm', '6', '6', '4', '7', '_', '0',
};
static const int adv_idx_off = 12;
static uint8_t local_addr[] = { 0x00, 0x26, 0x66, 0xBF, 0x01, 0xCC };
static ob_gap_addr_t peer_addr = {1, { 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC} };
static ob_gap_addr_t last_conn;
static ob_adv_param_t adv_param;
static ob_data_t adv_data = { sdata, sizeof(sdata) };

/*********************************************************************
 * EXTERN FUNCTIONS
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief  BLE event process callback
 * @param[in] evt_id  event id
 * @param[in] evt     event parameters
 *******************************************************************************
 */
static void app_adv_event_cb(uint16_t evt_id, const omble_evt_t *evt)
{
    if (evt_id == OB_GAP_EVT_CONNECTED) {
        log_debug("OB_GAP_EVT_CONNECTED(%d): %d\n", evt->gap.conn_idx, evt->gap.connected.adv_idx);
        memcpy(&last_conn, &evt->gap.connected.peer_addr, sizeof(ob_gap_addr_t));
    } else if (evt_id == OB_GAP_EVT_DISCONNECTED) {
        log_debug("OB_GAP_EVT_DISCONNECTED(%d): 0x%02X\n", evt->gap.conn_idx, evt->gap.disconnected.reason);
    } else if (evt_id == OB_GAP_EVT_ADV_STATE_CHANGED) {
        log_debug("OB_GAP_EVT_ADV_STATE_CHANGED(%d), reason:%d\n", evt->gap.adv_state_changed.adv_idx,
                     evt->gap.adv_state_changed.state);
        if (evt->gap.adv_state_changed.state == OB_GAP_ADV_ST_STARTED) {
            if (!app_get_conn_num_get()) {
                log_debug("Warning: Not resource for advertising\n");
            }
        } else if (evt->gap.adv_state_changed.state == OB_GAP_ADV_ST_STOPPED_BY_USER) {
            app_get_conn_num_put();
        }
    } else {
    }
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
void app_adv_init(void)
{
    ob_event_callback_reg(app_adv_event_cb);
}

/**
 *******************************************************************************
 * @brief  Start advertising
 * @param[in] adv_index  index
 *******************************************************************************
 */
uint32_t app_adv_start(int adv_index, int adv_prop)
{
    if (adv_index >= OB_LE_HOST_ADV_SET_NUM) {
        return OB_ERROR_INSUFFICIENT_RESOURCES;
    }
    if (!app_get_conn_num_get()) {
        return OB_ERROR_INSUFFICIENT_RESOURCES;
    } else {
        app_get_conn_num_put();
    }
    adv_param.own_addr_type = OB_ADV_ADDR_TYPE_RANDOM;
    adv_param.prim_phy = OB_ADV_PHY_1M;
    adv_param.secd_phy = OB_ADV_PHY_1M;
    adv_param.tx_pwr = 0;
    adv_param.filter_policy = OB_ADV_FILTER_NONE;
    adv_param.prim_ch_map = OB_ADV_CH_ALL;
    adv_param.prim_intv_min = 0x40;
    adv_param.prim_intv_max = 0x80;
    local_addr[0] =  adv_index;
    adv_param.local_addr = local_addr;
    adv_param.peer_addr = &peer_addr;
    adv_param.adv_properties = adv_prop;
    adv_data.data[adv_idx_off] = '0' + adv_index;
    uint32_t res = ob_gap_adv_start(adv_index, &adv_param, &adv_data, NULL);
    return res;
}

/**
 *******************************************************************************
 * @brief  stop advertising
 * @param[in] adv_index  index
 *******************************************************************************
 */
uint32_t app_adv_stop(int adv_index)
{
    uint32_t res = ob_gap_adv_stop(adv_index);
    return res;
}


/** @} */
