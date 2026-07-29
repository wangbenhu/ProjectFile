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

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static struct dev_rec {
    uint8_t addr[6];
    uint8_t addr_type;
    int8_t  rssi;
    uint8_t data_len;
    uint8_t data[31];
} scan_dev_record[50];
int scan_dev_count;

static ob_scanning_param_t param_scan_1m = {
    .scan_type = false,
    .interval = 0x80,
    .window = 0x40,
};
/*********************************************************************
 * EXTERN FUNCTIONS
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static char *get_name(const char *report, int len)
{
    static char name[32];
    const char *data = report;
    while (len > 0) {
        uint8_t l = *data++; len--;
        if (len < l || l==0) {
            return NULL;
        }
        uint8_t type = *data++; len--;
        if (type == 0x08 || type == 0x09) {
            int name_len = l - 1 < sizeof(name) ? l - 1 : sizeof(name);
            memcpy(name, data, name_len);
            name[name_len] = '\0';
            return name;
        }
        data += l - 1; len -= l - 1;
    }
    return NULL;
}

static void dev_record(const ob_gap_evt_adv_report_t *report)
{
    int min_rssi_idx = 0, min_rssi = 127, rec_idx = -1;
    for (int i = 0; i < scan_dev_count; i++) {
        if (min_rssi > scan_dev_record[i].rssi) {
            min_rssi = scan_dev_record[i].rssi;
            min_rssi_idx = i;
        }
        if (scan_dev_record[i].addr_type == report->addr.addr_type && !memcmp(scan_dev_record[i].addr, &report->addr.addr, 6) &&
                scan_dev_record[i].data_len == report->data_len &&
                !memcmp(scan_dev_record[i].data, report->data, report->data_len)) {
            rec_idx = i;
            break;
        }
    }
    if (rec_idx == -1) {
        if (scan_dev_count < (int)(sizeof(scan_dev_record) / sizeof(scan_dev_record[0]))) {
            rec_idx = scan_dev_count++;
        } else {
            rec_idx = min_rssi_idx;
        }
    }
    memcpy(&scan_dev_record[rec_idx].addr, &report->addr.addr, 6);
    scan_dev_record[rec_idx].addr_type = report->addr.addr_type;
    scan_dev_record[rec_idx].rssi = report->rssi;
    scan_dev_record[rec_idx].data_len = report->data_len < sizeof(scan_dev_record[rec_idx].data) ? \
        report->data_len : sizeof(scan_dev_record[rec_idx].data);
    memcpy(scan_dev_record[rec_idx].data, report->data, scan_dev_record[rec_idx].data_len);
}

static void app_scan_event_cb(uint16_t evt_id, const omble_evt_t *evt)
{
    if (evt_id == OB_GAP_EVT_ADV_REPORT) {
        dev_record(&evt->gap.adv_report);
    } else if (evt_id == OB_GAP_EVT_TIMEOUT) {
        set_shell_busy(NULL);
        log_debug("Scan Finished:\n");
        for (int i = 0; i < scan_dev_count; i++) {
            struct dev_rec *d = &scan_dev_record[i];
            uint8_t *a = &d->addr[0];
            log_debug("\t%2d.", i + 1);
            log_debug("  %d  %02X:%02X:%02X:%02X:%02X:%02X", d->addr_type, a[5], a[4], a[3], a[2], a[1], a[0]);
            log_debug(" %4d", d->rssi);
            const char *name = get_name((char *)d->data, d->data_len);
            if (name) {
                log_debug("  %s", name);
            }
            log_debug("\n");
            // Dump advertise data
            // log_debug_ARRAY_EX("\t\t", d->data, d->data_len);
        }
    }
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
uint32_t app_scan_start(uint8_t timeout)
{
    if (timeout == 0) {
        return OB_ERROR_INVALID_PARAM;
    }
    ob_scan_param_t scan_param = {
        OB_ADV_ADDR_TYPE_RANDOM,
        OB_SCAN_FILTER_BASIC_UNFILTER,
        timeout * 100,
        &param_scan_1m,
        NULL,
    };
    scan_dev_count = 0;
    uint32_t res = ob_gap_scan_start(&scan_param);
    if (res == OB_ERROR_NO_ERR) {
        set_shell_busy("Scanning...Please wait\n");
    }
    return res;
}

bool app_scan_device_get(uint8_t index, uint8_t *address, const char **name)
{
    index--; // number to index
    if (name) {
        *name = NULL;
    }
    if (index >= scan_dev_count) {
        return false;
    } else {
        address[0] = scan_dev_record[index].addr_type;
        memcpy(&address[1], &scan_dev_record[index].addr[0], 6);
        *name = get_name((char *)scan_dev_record[index].data, scan_dev_record[index].data_len);
        return true;
    }
}

void app_scan_init(void)
{
    ob_event_callback_reg(app_scan_event_cb);
}

/** @} */
