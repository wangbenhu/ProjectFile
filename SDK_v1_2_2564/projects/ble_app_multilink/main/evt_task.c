/**
 * @file  main.c
 * @brief  simple server
 * @date Wed, Sep  5, 2018  5:19:05 PM
 * @author liqiang
 *
 * @addtogroup APP_SIMPLE_SERVER_MAIN main.c
 * @ingroup APP_SIMPLE_SERVER
 * @details simple server
 *
 * @{
 */

/*********************************************************************
 * INCLUDES
 */
#include "om_driver.h"
#include "shell.h"
#include "evt.h"
#include "pm.h"
#include "omble.h"
#include "om_log.h"
#include "app_common.h"
#include "evt_timer.h"

// Controller header
#include "obc.h"

/* Kernel includes. */
#include "cmsis_os2.h"

/*********************************************************************
 * MACROS
 */
#define EVENT_SYSTEM_RESERVE_MASK   0x00FF


/*********************************************************************
 * TYPEDEFS
 */


/*********************************************************************
 * CONSTANTS
 */


/*********************************************************************
 * LOCAL VARIABLES
 */
static osSemaphoreId_t xSemBluetooth = NULL;


/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERN FUNCTIONS
 */
void app_adv_init();
void app_sec_init();
void service_common_init(void);
void app_om_dfu_init(void);
void ancs_client_init(void);
void app_media_hid_init(void);
void app_tspp_init(void);
void app_wechat_lite_init();
void app_om_cgms_init(void);
void app_om_bms_init(void);
void app_shell_proc(void);
extern void ob_smp_config_allroles(void);

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static void hardware_init(void)
{
}

/**
 *******************************************************************************
 * @brief  evt timer 0 handler
 *
 * @param[in] timer  timer
 * @param[in] param  param
 *******************************************************************************
 */

/**
 * @brief  bluetooth event handler
 **/
static void vEvtEventHandler(void)
{
    if (xSemBluetooth) {
        osSemaphoreRelease(xSemBluetooth);
    }
}

/**
 * @brief  bluetooth schedule task
 *
 * @param[in] pvParameters  pv parameters
 **/
static void vEvtScheduleTask(void *argument)
{
    hardware_init();
    drv_rf_init();
    evt_init();

    struct ob_stack_param param = {
        .max_connection         = OB_LE_HOST_CONNECTION_NB,
        .max_ext_adv_set        = OB_LE_HOST_ADV_SET_NUM,
        .max_att_mtu            = OB_LE_HOST_ATT_MTU,
        .max_gatt_serv_num      = OB_LE_HOST_MAX_GATT_SERV_NUM,
        .max_gatt_write_cache   = OB_LE_HOST_ATT_WRITE_CACHE_SIZE,
        .smp_sc_support         = OB_LE_HOST_SC_PAIRING,
    };
    //ob_smp_config_allroles();
    omble_init(&param);

    evt_timer_init();
    service_common_init();
    app_adv_init();
    app_sec_init();
    // app_tspp_init();
    app_conn_init();
    app_scan_init();
    app_gatt_client_init();

    // Create semaphore
    xSemBluetooth = osSemaphoreNew(1, 0, NULL);

    // set ke event callback
    evt_schedule_trigger_callback_set(vEvtEventHandler);

    while (1) {
        // schedule
        evt_schedule();
        // process shell command
        app_shell_proc();
        // Wait for semaphore
        osSemaphoreAcquire(xSemBluetooth, osWaitForever);
    }
}

/*********************************************************************
 * CONST VARIABLES
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */
void shell_trigger(void)
{
    vEvtEventHandler();
}
/**
 * @brief  v start bluetooth task
 **/
void vStartEvtTask(void)
{
    const osThreadAttr_t bluetoothThreadAttr = {
        .name = NULL,
        .attr_bits = 0,
        .cb_mem = NULL,
        .cb_size = 0,
        .stack_mem = NULL,
        .stack_size = 4096,
        .priority = osPriorityRealtime,
        .tz_module = 0,
    };

    // Create ble Task
    osThreadNew(vEvtScheduleTask, NULL, &bluetoothThreadAttr);
}

/** @} */

// vim: fdm=marker
