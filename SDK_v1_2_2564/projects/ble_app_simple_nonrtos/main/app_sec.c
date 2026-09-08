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
#include "om_log.h"

/*********************************************************************
 * MACROS
 */
#define log_debug(...) om_log(OM_LOG_INFO, ##__VA_ARGS__)
#define hexdump(d, l) do{for(int i=0;i<l;i++)log_debug("%02X ", ((uint8_t*)d)[i]);log_debug("\n");}while(0);
#define PERIPHERAL_IOCAP  OB_SMP_IOCAP_NONE

/*********************************************************************
 * LOCAL VARIABLES
 */

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
static void app_sec_event_cb(uint16_t evt_id, const omble_evt_t *evt)
{
    if (evt_id == OB_GAP_EVT_CONNECTED) {
        log_debug("OB_GAP_EVT_CONNECTED(%d): %d\n", evt->gap.conn_idx, evt->gap.connected.adv_idx);
    } else if (evt_id == OB_GAP_EVT_DISCONNECTED) {
        log_debug("OB_GAP_EVT_DISCONNECTED 0x%02X\n", evt->gap.disconnected.reason);
    } else if (evt_id == OB_GAP_EVT_PIN_REQUEST) {
        log_debug("OB_GAP_EVT_PIN_REQUEST:%d\n", evt->gap.pin_request.type);
        ob_smp_pin_t pin_info;
        switch (evt->gap.pin_request.type) {
            case OB_SMP_PIN_TYPE_DIS:
                log_debug("show pin:%06d\n", evt->gap.pin_request.pin_code);
            case OB_SMP_PIN_TYPE_NONE:
                ob_gap_pin_response(evt->gap.conn_idx, true, &pin_info);
                break;
            case OB_SMP_PIN_TYPE_PK_REQ:
            case OB_SMP_PIN_TYPE_OOB_REQ: {
                break;
            }
        }
    } else if (evt_id == OB_GAP_EVT_BONDED) {
        log_debug("OB_GAP_EVT_BONDED: 0x%04X\n", evt->gap.bonded.status);
    } else if (evt_id == OB_GAP_EVT_BOND_INFO_REQUEST) {
        log_debug("OB_GAP_EVT_BOND_INFO_REQUEST:%s\n", evt->gap.bond_info_request.type == OB_BOND_INFO_LTK ? "LTK" : "IRK");
        ob_bond_info_t bond_info = { evt->gap.bond_info_request.type };
        if (evt->gap.bond_info_request.type == OB_BOND_INFO_LTK) {
            memcpy(bond_info.enc_info.ltk, (uint8_t *)"LTKLTKLTKLTKLTK", 16);
            memcpy(bond_info.enc_info.random, (uint8_t *)"_RANDOM_", 8);
            bond_info.enc_info.ediv = 0x3344;
        } else {
            uint8_t addr[7] = {OB_ADV_ADDR_TYPE_RANDOM};
            ob_gap_addr_get(OB_ADV_ADDR_TYPE_RANDOM, &addr[1]);
            memcpy(bond_info.id_info.irk, (uint8_t *)"IRKIRKIRKIRKIRK", 16);
            memcpy(&bond_info.id_info.id_addr, addr, sizeof(addr));
        }
        ob_gap_bond_info_response(evt->gap.conn_idx, &bond_info);
    } else if (evt_id == OB_GAP_EVT_LTK_REQUEST) {
        log_debug("OB_GAP_EVT_LTK_REQUEST\n");
        ob_gap_ltk_response(evt->gap.conn_idx, (uint8_t *)"LTKLTKLTKLTKLTK");
    } else if (evt_id == OB_GAP_EVT_PAIRING_REQUEST) {
        log_debug("OB_GAP_EVT_PAIRING_REQUEST\n");
        ob_pairing_param_t response;
        response.authreq.bond_flags = 1;
        response.authreq.mitm = 1;
        response.authreq.sc = 0;
        response.oob_data_flag = 0;
        response.initiator_key_distribution = OB_SMP_DIST_BIT_ENC_KEY | OB_SMP_DIST_BIT_ID_KEY;
        response.responder_key_distribution = OB_SMP_DIST_BIT_ENC_KEY;
        response.io_capability = PERIPHERAL_IOCAP;
        ob_gap_pairing_response(evt->gap.conn_idx, &response);
    } else if (evt_id == OB_GAP_EVT_BOND_INFO) {
        log_debug("OB_GAP_EVT_BOND_INFO\n");
        if (evt->gap.bond_info.type == OB_BOND_INFO_LTK) {
            log_debug("Recv LTK: ");
            hexdump(evt->gap.bond_info.enc_info.ltk, 16);
            log_debug("Recv EDIV: 0x%04X\n", evt->gap.bond_info.enc_info.ediv);
            log_debug("Recv RANDOM: ");
            hexdump(evt->gap.bond_info.enc_info.random, 8);
        } else {
            log_debug("Recv IRK: ");
            hexdump(evt->gap.bond_info.id_info.irk, 16);
            log_debug("Recv ID: %01X ", evt->gap.bond_info.id_info.id_addr.addr_type);
            hexdump(&evt->gap.bond_info.id_info.id_addr.addr, 6);
        }
    } else if (evt_id == OB_GAP_EVT_ENCRYPT) {
        log_debug("OB_GAP_EVT_ENCRYPT: %d\n", evt->gap.encrypt.encrypted);
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
void app_sec_init(void)
{
    ob_event_callback_reg(app_sec_event_cb);
}

/** @} */
