/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup DOC DOC
 * @ingroup  DOCUMENT
 * @brief    system
 * @details  system
 *
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
#include "om_log.h"


/*******************************************************************************
 * MACROS
 */
#define log_debug(...) om_log(OM_LOG_INFO, ##__VA_ARGS__)
#define hexdump(d, l) do{for(int i=0;i<l;i++)log_debug("%02X ", ((uint8_t*)d)[i]);log_debug("\n");}while(0);


/*******************************************************************************
 * LOCAL VARIABLES
 */
/*const static int  app_gap_appearance = 0x03C2;*/
/// Advertise data
static uint8_t sdata[] = {
    /* Flags: BLE limited discoverable mode and BR/EDR not supported */
    0x02, 0x01, 0x06,
    /* incomplete list of service class UUIDs: (0x1812) */
    /*0x03, 0x02, 0x12, 0x18,*/
    /* incomplete list of service class UUIDs: (0xFEE7) */
    /*0x03, 0x02, 0xE7, 0xFE,*/
    /* Apperance */
    /*0x03, 0x19, app_gap_appearance & 0xff, (app_gap_appearance>>8) & 0xff,*/
    /* Complete Local Name */
    7, 0x09,
    'O','M','6','6','2','7',
};

static uint8_t local_addr[] = { 0x01, 0xA1, 0x27, 0x66, 0xBF, 0x01 };
static ob_gap_addr_t peer_addr = {1, { 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC} };
static ob_gap_addr_t last_conn;
static ob_adv_param_t adv_param;
static ob_data_t adv_data = { sdata, sizeof(sdata) };
static ob_data_t scan_rsp_data = { sdata, sizeof(sdata) };


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
static void app_adv_event_cb(uint16_t evt_id, const omble_evt_t *evt)
{
    if (evt_id == OB_GAP_EVT_CONNECTED) {
        log_debug("OB_GAP_EVT_CONNECTED(%d): %d\n", evt->gap.conn_idx, evt->gap.connected.adv_idx);
        memcpy(&last_conn, &evt->gap.connected.peer_addr, sizeof(ob_gap_addr_t));
    } else if (evt_id == OB_GAP_EVT_DISCONNECTED) {
        log_debug("OB_GAP_EVT_DISCONNECTED(%d): 0x%02X\n", evt->gap.conn_idx, evt->gap.disconnected.reason);
        ob_gap_adv_start(0, &adv_param, &adv_data, &scan_rsp_data);
    } else if (evt_id == OB_GAP_EVT_ADV_STATE_CHANGED) {
        log_debug("OB_GAP_EVT_ADV_STATE_CHANGED(%d), reason:%d\n", evt->gap.adv_state_changed.adv_idx,
               evt->gap.adv_state_changed.state);
    } else if (evt_id == OB_GAP_EVT_SCAN_REQ_RECV) {
        log_debug("OB_GAP_EVT_SCAN_REQ_RECV %d:  %d %02X:%02X:%02X:%02X:%02X:%02X\n",
               evt->gap.scan_req_recv.adv_idx,
               evt->gap.scan_req_recv.addr.addr_type,
               evt->gap.scan_req_recv.addr.addr[0],
               evt->gap.scan_req_recv.addr.addr[1],
               evt->gap.scan_req_recv.addr.addr[2],
               evt->gap.scan_req_recv.addr.addr[3],
               evt->gap.scan_req_recv.addr.addr[4],
               evt->gap.scan_req_recv.addr.addr[5]);
    } else {
    }
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief  Init advertising module
 *******************************************************************************
 */
void app_adv_init(void)
{
    ob_event_callback_reg(app_adv_event_cb);

    adv_param.own_addr_type = OB_ADV_ADDR_TYPE_RANDOM;
    adv_param.prim_phy = OB_ADV_PHY_1M;
    adv_param.secd_phy = OB_ADV_PHY_1M;
    adv_param.tx_pwr = 0;
    adv_param.filter_policy = OB_ADV_FILTER_NONE;
    adv_param.prim_ch_map = OB_ADV_CH_ALL;
    adv_param.prim_intv_min = 0x40;
    adv_param.prim_intv_max = 0x80;
    adv_param.local_addr = local_addr;
    adv_param.peer_addr = &peer_addr;
    // adv_param.adv_properties = OB_ADV_PROP_EXT_CONN_NONSCAN;
    // adv_param.adv_properties = OB_ADV_PROP_LEGACY_DIRECT_IND_HIGH;
    // adv_param.adv_properties = OB_ADV_PROP_EXT_NONCONN_SCAN;
    adv_param.adv_properties = OB_ADV_PROP_LEGACY_IND;
    ob_gap_adv_start(0, &adv_param, &adv_data, &scan_rsp_data);
}


/** @} */
