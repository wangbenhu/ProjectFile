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
#include "shell.h"
#include "omble.h"
#include "om_driver.h"
#include "app_common.h"

/*********************************************************************
 * MACROS
 */
#define APP_SHELL_MAX_ARG      4
#define APP_SHELL_MAX_ARG_LEN  8

/*********************************************************************
 * LOCAL VARIABLES
 */
static const char *busy_flag;
static char rec_args[APP_SHELL_MAX_ARG][APP_SHELL_MAX_ARG_LEN + 1];
static char *rec_argv[APP_SHELL_MAX_ARG];
static char rec_argc;
static void (*rec_cmd_shell_handler)(int argc, char *argv[]);

static void cmd_shell_adv(int argc, char *argv[])
{
    if (busy_flag) {
        log_debug("%s\n", busy_flag);
        return;
    }
    if (argc <= 0) {
        log_debug("Usage(adv): adv start/stop [adv_index=0]\n");
        log_debug("\tindex=0: Connectable, others non-connectable\n");
        return;
    }
    uint32_t res;
    int index = argc >= 2 ? atoi(argv[1]) : 0;
    if (!strcmp(argv[0], "start")) {
        res = app_adv_start(index, OB_ADV_PROP_LEGACY_IND);
        log_debug("Starting advertise, index=%d, status=%d\n", index, res);
    } else if (!strcmp(argv[0], "stop")) {
        res = app_adv_stop(index);
        log_debug("Stopping advertise, index=%d, status=%d\n", index, res);
    } else {
        log_debug("Usage(adv): adv start/stop [adv_index=0]\n");
        return;
    }
}

static void cmd_shell_scan(int argc, char *argv[])
{
    if (busy_flag) {
        log_debug("%s\n", busy_flag);
        return;
    }
    uint32_t res;
    uint8_t tout_s = argc >= 1 ? atoi(argv[0]) : 3;
    res = app_scan_start(tout_s);
    if (res == OB_ERROR_NO_ERR) {
        log_debug("Starting scan, timeout %d s\n", tout_s);
    } else {
        log_debug("Starting scan failed, error code = 0x%04X\n", res);
    }
}

static void cmd_shell_conn(int argc, char *argv[])
{
    uint8_t address[7];
    uint32_t res;
    if (argc <= 0) {
        log_debug("Usage(conn): conn [index in scan list] [timeout=5]\n");
        return;
    }
    if (busy_flag) {
        log_debug("%s\n", busy_flag);
        return;
    }
    int index = atoi(argv[0]);
    const char *dev_name;
    bool r = app_scan_device_get(index, address, &dev_name);
    if (!r) {
        log_debug("Device index %d invalid, scan device first\n", index);
        return;
    }
    uint8_t tout_s = argc >= 2 ? atoi(argv[1]) : 5;
    res = app_conn_start(address, tout_s, dev_name);
    if (res == OB_ERROR_NO_ERR) {
        const uint8_t *a = address;
        log_debug("Starting connect to (%d) %02X:%02X:%02X:%02X:%02X:%02X  %s, timeout %d s\n",
                  a[0], a[6], a[5], a[4], a[3], a[2], a[1], dev_name ? dev_name : "", tout_s);
    } else {
        log_debug("Starting connect failed, error code = 0x%04X\n", res);
    }
}

static void cmd_shell_disc(int argc, char *argv[])
{
    uint32_t res;
    struct app_device dev_buffer[OB_LE_HOST_CONNECTION_NB];
    uint8_t max_num = OB_LE_HOST_CONNECTION_NB;
    app_conn_device_get(dev_buffer, &max_num);
    if (argc <= 0) {
        log_debug("Usage(disc): disc [index in conn list]\n");
        if (max_num == 0) {
            log_debug("\tNo connected devices\n");
            return;
        }
        for (int i = 0; i < max_num; i++) {
            uint8_t *a = &dev_buffer[i].addr_type;
            log_debug("\tIdx: %d, (%d) %02X:%02X:%02X:%02X:%02X:%02X  %s\n",
                      dev_buffer[i].conn_idx, a[0], a[6], a[5], a[4], a[3], a[2], a[1], dev_buffer[i].name);
        }
        return;
    }
    if (busy_flag) {
        log_debug("%s\n", busy_flag);
        return;
    }
    int index = atoi(argv[0]);

    for (int i = 0; i < max_num; i++) {
        if (dev_buffer[i].conn_idx == index) {
            res = ob_gap_disconnect(index, 0x13);
            if (res == OB_ERROR_NO_ERR) {
                uint8_t *a = &dev_buffer[i].addr_type;
                log_debug("Disconnecting to idx=%d (%d) %02X:%02X:%02X:%02X:%02X:%02X  %s\n",
                          index, a[0], a[6], a[5], a[4], a[3], a[2], a[1], dev_buffer[i].name);
            } else {
                log_debug("Disconnect failed, error code = 0x%04X\n", res);
            }
        }
        return;
    }
    log_debug("Disconnect failed, no device found(idx=%d)\n", index);
}

static void cmd_shell_gattda(int argc, char *argv[])
{
    if (argc <= 0) {
        log_debug("Usage(gattda): gattda con_idx\n");
        return;
    }
    int index = atoi(argv[0]);
    app_gatt_client_discover_all(index);
}

static void cmd_shell_gattds(int argc, char *argv[])
{
    if (argc <= 0) {
        log_debug("Usage(gattds): gattds con_idx [uuid]\n");
        log_debug("    discover all service:     gattds 0 start_hdl end_hdl\n");
        log_debug("    discover service by uuid: gattds 0 0x180A\n");
        log_debug("    Note: only 16bit uuid support in shell\n");
        return;
    }
    int index = atoi(argv[0]);
    uint16_t uuid = 0;
    if (argc == 2 && !strncmp(argv[1], "0x", 2)) {
        uuid = strtol(&argv[1][2], NULL, 16);
        app_gatt_discover_service_uuid(index, (uint8_t *)&uuid, 2);
    }
    if (argc == 3) {
        uint16_t start_hdl = 0xFFFF, end_hdl = 0;
        if (!strncmp(argv[1], "0x", 2)) {
            start_hdl = strtol(&argv[1][2], NULL, 16);
        } else {
            start_hdl = strtol(argv[1], NULL, 10);
        }
        if (!strncmp(argv[2], "0x", 2)) {
            end_hdl = strtol(&argv[2][2], NULL, 16);
        } else {
            end_hdl = strtol(argv[2], NULL, 10);
        }
        app_gatt_discover_service(index, start_hdl, end_hdl);
    }
}

static void cmd_shell_gattdc(int argc, char *argv[])
{
    if (argc != 3) {
        log_debug("Usage(gattdc): gattdc con_idx start_hdl end_hdl\n");
        log_debug("      example: gattdc 0 0x0001 0xffff\n");
        return;
    }
    int index = atoi(argv[0]);
    if (argc == 3) {
        uint16_t start_hdl = 0xFFFF, end_hdl = 0;
        if (!strncmp(argv[1], "0x", 2)) {
            start_hdl = strtol(&argv[1][2], NULL, 16);
        } else {
            start_hdl = strtol(argv[1], NULL, 10);
        }
        if (!strncmp(argv[2], "0x", 2)) {
            end_hdl = strtol(&argv[2][2], NULL, 16);
        } else {
            end_hdl = strtol(argv[2], NULL, 10);
        }
        app_gatt_discover_char(index, start_hdl, end_hdl);
    }
}

static void cmd_shell_gattdd(int argc, char *argv[])
{
    if (argc != 3) {
        log_debug("Usage(gattdd): gattdd con_idx start_hdl end_hdl\n");
        log_debug("      example: gattdd 0 0x0001 0xffff\n");
        return;
    }
    int index = atoi(argv[0]);
    if (argc == 3) {
        uint16_t start_hdl = 0xFFFF, end_hdl = 0;
        if (!strncmp(argv[1], "0x", 2)) {
            start_hdl = strtol(&argv[1][2], NULL, 16);
        } else {
            start_hdl = strtol(argv[1], NULL, 10);
        }
        if (!strncmp(argv[2], "0x", 2)) {
            end_hdl = strtol(&argv[2][2], NULL, 16);
        } else {
            end_hdl = strtol(argv[2], NULL, 10);
        }
        app_gatt_discover_desc(index, start_hdl, end_hdl);
    }
}

static void cmd_shell_gattrh(int argc, char *argv[])
{
    if (argc != 2) {
        log_debug("Usage(gattrh): gattrh con_idx att_handle\n");
        log_debug("      example: gattrh 0 3\n");
        return;
    }
    int index = atoi(argv[0]);
    if (argc == 2) {
        uint16_t att_handle;
        if (!strncmp(argv[1], "0x", 2)) {
            att_handle = strtol(&argv[1][2], NULL, 16);
        } else {
            att_handle = strtol(argv[1], NULL, 10);
        }
        app_gatt_read(index, att_handle);
    }
}

static void cmd_shell_gattru(int argc, char *argv[])
{
    if (argc != 2) {
        log_debug("Usage(gattru): gattru con_idx uuid\n");
        log_debug("      example: gattru 0 0x2a00\n");
        log_debug("    Note: only 16bit uuid support in shell\n");
        return;
    }
    int index = atoi(argv[0]);
    if (argc == 2) {
        uint16_t uuid;
        if (!strncmp(argv[1], "0x", 2)) {
            uuid = strtol(&argv[1][2], NULL, 16);
        } else {
            uuid = strtol(argv[1], NULL, 10);
        }
        app_gatt_read_uuid(index, (uint8_t *)&uuid, 2);
    }
}

static void cmd_shell_gattw(int type, int argc, char *argv[])
{
    if (argc != 3) {
        const char *cmd = type == OB_GATTC_WRITE_REQ ? "gattwr" : "gattwc";
        log_debug("Usage(%s): %s con_idx att_handle data\n", cmd, cmd);
        log_debug("      example: %s 0 3 0123456789abcdef\n", cmd);
        return;
    }
    int index = atoi(argv[0]);
    uint16_t att_handle;
    if (!strncmp(argv[1], "0x", 2)) {
        att_handle = strtol(&argv[1][2], NULL, 16);
    } else {
        att_handle = strtol(argv[1], NULL, 10);
    }
    uint8_t data[32] = {0}, *p = data;
    for (int i = 0; i < (int)strlen(argv[2]); i++) {
        int n = 0;
        if ('a' <= argv[2][i] && argv[2][i] <= 'f') {
            n = argv[2][i] - 'a' + 10;
        } else if ('A' <= argv[2][i] && argv[2][i] <= 'F') {
            n = argv[2][i] - 'a' + 10;
        } else {
            n = argv[2][i] - '0';
        }
        *p |= n;
        if (i & 1) {
            p++;
        } else {
            *p = *p << 4;
        }
    }
    ob_gattc_write(index, att_handle, type, data, p - data);
}

static void cmd_shell_gattwr(int argc, char *argv[])
{
    cmd_shell_gattw(OB_GATTC_WRITE_REQ, argc, argv);
}

static void cmd_shell_gattwc(int argc, char *argv[])
{
    cmd_shell_gattw(OB_GATTC_WRITE_CMD, argc, argv);
}

#define APP_SHELL_REC_FUNC_DEFINE(_name) \
    static void rec_##_name(int argc, char *argv[]) \
    { \
        if (rec_cmd_shell_handler) { \
            log_debug("Command pending\n"); \
            return; \
        } \
        if (argc > APP_SHELL_MAX_ARG) { \
            log_debug("shell param num excceed limit\n"); \
            return; \
        } \
        for (int i=0;i<argc;i++) { \
            if (strlen(argv[i]) > APP_SHELL_MAX_ARG_LEN) { \
                log_debug("shell param length excceed limit\n"); \
                return; \
            } \
            strcpy(rec_argv[i], argv[i]); \
        } \
        rec_argc = argc; \
        rec_cmd_shell_handler = _name; \
        void shell_trigger(void); \
        shell_trigger(); \
    } \

APP_SHELL_REC_FUNC_DEFINE(cmd_shell_adv)
APP_SHELL_REC_FUNC_DEFINE(cmd_shell_scan)
APP_SHELL_REC_FUNC_DEFINE(cmd_shell_conn)
APP_SHELL_REC_FUNC_DEFINE(cmd_shell_disc)
APP_SHELL_REC_FUNC_DEFINE(cmd_shell_gattda)
APP_SHELL_REC_FUNC_DEFINE(cmd_shell_gattds)
APP_SHELL_REC_FUNC_DEFINE(cmd_shell_gattdc)
APP_SHELL_REC_FUNC_DEFINE(cmd_shell_gattdd)
APP_SHELL_REC_FUNC_DEFINE(cmd_shell_gattrh)
APP_SHELL_REC_FUNC_DEFINE(cmd_shell_gattru)
APP_SHELL_REC_FUNC_DEFINE(cmd_shell_gattwr)
APP_SHELL_REC_FUNC_DEFINE(cmd_shell_gattwc)

const static shell_cmd_t ble_multilink_shell_cmd[] = {
    { "adv",     rec_cmd_shell_adv,     "Start/Stop advertise        Usage: adv start/stop [index=0]"   },
    { "scan",    rec_cmd_shell_scan,    "Scan device                 Usage: scan [timeout=3]" },
    { "conn",    rec_cmd_shell_conn,    "Connect device              Usage: conn [index]"  },
    { "disc",    rec_cmd_shell_disc,    "Disconnect device           Usage: disc con_idx"  },
    { "gattda",  rec_cmd_shell_gattda,  "GATT Client Discover All    Usage: gattc con_idx"  },
    { "gattds",  rec_cmd_shell_gattds,  "GATT Discover Service       Usage: gattds con_idx start_hdl end_hdl"  },
    { "gattdc",  rec_cmd_shell_gattdc,  "GATT Discover Character     Usage: gattdc con_idx start_hdl end_hdl"  },
    { "gattdd",  rec_cmd_shell_gattdd,  "GATT Discover Descriptor    Usage: gattdd con_idx start_hdl end_hdl"  },
    { "gattrh",  rec_cmd_shell_gattrh,  "GATT Read by handle         Usage: gattrh con_idx att_handle"  },
    { "gattru",  rec_cmd_shell_gattru,  "GATT Read by UUID           Usage: gattru con_idx uuid"  },
    { "gattwr",  rec_cmd_shell_gattwr,  "GATT Write Request          Usage: gattwr con_idx att_handle data"  },
    { "gattwc",  rec_cmd_shell_gattwc,  "GATT Write Command          Usage: gattwc con_idx att_handle data"  },
    { NULL,      NULL,              NULL},     /* donot deleted */
};


/*********************************************************************
 * EXTERN FUNCTIONS
 */
void set_shell_busy(const char *busy_desc)
{
    busy_flag = busy_desc;
}

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief  Init shell module
 *******************************************************************************
 */
void app_shell_init(void)
{
    for (int i = 0; i < APP_SHELL_MAX_ARG; i++) {
        rec_argv[i] = rec_args[i];
    }
    shell_init(ble_multilink_shell_cmd);
}

/**
 *******************************************************************************
 * @brief  Process pending command
 *******************************************************************************
 */
void app_shell_proc(void)
{
    if (rec_cmd_shell_handler) {
        rec_cmd_shell_handler(rec_argc, rec_argv);
        rec_cmd_shell_handler = NULL;
    }
}

/** @} */
