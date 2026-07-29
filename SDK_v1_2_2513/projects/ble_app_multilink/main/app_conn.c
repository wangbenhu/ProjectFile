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
#include "evt_timer.h"
#include "app_common.h"

/*********************************************************************
 * MACROS
 */
#define IDX_INVALID 0xFF

/*********************************************************************
 * LOCAL VARIABLES
 */
static evt_timer_t timer_conn;
static const char *conn_dev_name;
static struct app_device conn_devs[OB_LE_HOST_CONNECTION_NB];

/*********************************************************************
 * EXTERN FUNCTIONS
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void app_conn_event_cb(uint16_t evt_id, const omble_evt_t *evt)
{
    int i = evt->gap.conn_idx;
    if (evt_id == OB_GAP_EVT_CONNECTED) {
        set_shell_busy(NULL);
        evt_timer_del(&timer_conn);
        conn_devs[i].addr_type = evt->gap.connected.peer_addr.addr_type;
        memcpy(&conn_devs[i].addr, evt->gap.connected.peer_addr.addr, 6);
        if (!conn_dev_name || evt->gap.connected.role == OB_GAP_ROLE_PHERIPHERAL) {
            conn_dev_name = "";
        }
        strncpy(conn_devs[i].name, conn_dev_name, sizeof(conn_devs[i].name) - 1);
        conn_devs[i].conn_idx = evt->gap.conn_idx;
    } else if (evt_id == OB_GAP_EVT_DISCONNECTED) {
        conn_devs[i].conn_idx = IDX_INVALID;
        app_get_conn_num_put();
    }
}

static void conn_timeout_cb(evt_timer_t *timer, void *param)
{
    ob_gap_connect_cancel();
    app_get_conn_num_put();
    log_debug("Connection timeout\n");
    set_shell_busy(NULL);
}
/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
uint32_t app_conn_start(uint8_t *address, uint8_t timeout, const char *name)
{
    if (timeout == 0) {
        return OB_ERROR_INVALID_PARAM;
    }
    if (!app_get_conn_num_get()) {
        return OB_ERROR_INSUFFICIENT_RESOURCES;
    }

    ob_gap_addr_t peer_addr;
    memcpy(&peer_addr, address, 7);
    ob_conn_phy_param_t conn_phy = {
        .scan_intv = 0xA0,
        .scan_wind = 0x20,
        .conn_intv_min = 0x60,
        .conn_intv_max = 0xA0,
        .latency_max = 5,
        .timeout = 500,
    };
    ob_conn_param_t conn_param = {
        OB_ADV_ADDR_TYPE_RANDOM,
        OB_SCAN_FILTER_BASIC_UNFILTER,
        peer_addr,
        &conn_phy,
        &conn_phy,
        NULL,
    };
    conn_dev_name = name;
    uint32_t res = ob_gap_connect(&conn_param);
    if (res == OB_ERROR_NO_ERR) {
        set_shell_busy("Connectting...Please wait\n");
        evt_timer_set(&timer_conn, timeout * 1000, EVT_TIMER_ONE_SHOT, conn_timeout_cb, NULL);
    } else {
        app_get_conn_num_put();
    }
    return res;
}

void app_conn_device_get(struct app_device *dev_buffer, uint8_t *max_num)
{
    if (!max_num) {
        return;
    }
    if (!dev_buffer) {
        *max_num = 0;
        return;
    }
    int num_limit = *max_num;
    for (int i = 0, j = 0; i < OB_LE_HOST_CONNECTION_NB && j < num_limit; i++) {
        if (conn_devs[i].conn_idx != IDX_INVALID) {
            memcpy(&dev_buffer[j], &conn_devs[i], sizeof(struct app_device));
            j++;
        }
        *max_num = j;
    }
}

void app_conn_init(void)
{
    ob_event_callback_reg(app_conn_event_cb);
    for (int i = 0; i < OB_LE_HOST_CONNECTION_NB; i++) {
        conn_devs[i].conn_idx = IDX_INVALID;
    }
}

/** @} */
