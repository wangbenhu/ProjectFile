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
 * @brief    ble and om24g evt task
 * @details  ble and om24g evt task
 * @version
 *
 * Version 1.0
 *  - Initial release
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
#include "bsp.h"
#include "nvds.h"
#include "omble.h"
#include "om_log.h"

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
void app_24g_init(void);

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
    app_24g_init();
    struct ob_stack_param param = {
        .max_connection = 4,
        .max_ext_adv_set = 4,
        .max_att_mtu = 247,
        .max_gatt_serv_num = 8,
        .max_gatt_write_cache = 128,
        .smp_sc_support = true,
    };
    omble_init(&param);

    app_adv_init();
    app_sec_init();
    service_common_init();
    app_om_dfu_init();
    ancs_client_init();
    //app_media_hid_init();
    app_tspp_init();
    app_wechat_lite_init();
    OM_LOG(OM_LOG_DEBUG, "START IAR----- \r\n");

    // Create semaphore
    xSemBluetooth = osSemaphoreNew(1, 0, NULL);

    // set ke event callback
    evt_schedule_trigger_callback_set(vEvtEventHandler);

    while (1) {
        // schedule
        evt_schedule();

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
        .stack_size = 2048,
        .priority = osPriorityRealtime,
        .tz_module = 0,
    };

    // Create ble Task
    osThreadNew(vEvtScheduleTask, NULL, &bluetoothThreadAttr);
}

/** @} */

// vim: fdm=marker
