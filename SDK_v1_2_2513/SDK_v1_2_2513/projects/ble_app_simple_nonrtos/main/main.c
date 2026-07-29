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
 * @brief    ble simple app without rtos main entry
 * @details  ble simple app without rtos main entry
 * @version
 *
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


/*******************************************************************************
 * INCLUDES
 */
#include "bsp.h"
#include "om_driver.h"
#include "evt_timer.h"
#include "pm.h"
#include "nvds.h"
#include "omble.h"
#include "obc.h"
#include "ob_config.h"
#include "om_log.h"


/*******************************************************************************
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


/*******************************************************************************
 * EXAMPLE MODULE CALLBACK FUNCTIONS
 */
static void example_pin_wakeup_isr_handler(void *om_reg, drv_event_t event, void *int_status, void *data)
{
    OM_LOG(OM_LOG_INFO, "pin wakeup: 0x%08X\n", (uint32_t)int_status);
}

__RAM_CODES("APP")
static void example_pm_sleep_notify_handler(pm_sleep_state_t sleep_state, pm_status_t power_status)
{
    switch(sleep_state)
    {
        case PM_SLEEP_ENTRY:
            break;

        case PM_SLEEP_LEAVE_TOP_HALF:
            break;

        case PM_SLEEP_LEAVE_BOTTOM_HALF:
            break;

        default:
            break;
    }
}


/*******************************************************************************
 * STATIC FUNCTIONS
 */
static void ble_init(void)
{
    // BLE initialization
    struct ob_stack_param param = {
        .max_connection         = OB_LE_HOST_CONNECTION_NB,
        .max_ext_adv_set        = OB_LE_HOST_ADV_SET_NUM,
        .max_att_mtu            = OB_LE_HOST_ATT_MTU,
        .max_gatt_serv_num      = OB_LE_HOST_MAX_GATT_SERV_NUM,
        .max_gatt_write_cache   = OB_LE_HOST_ATT_WRITE_CACHE_SIZE,
        .smp_sc_support         = OB_LE_HOST_SC_PAIRING,
    };
    omble_init(&param);

    // Init advertising module
    app_adv_init();
    // Init secure module
    app_sec_init();
    // Init gatt service common
    service_common_init();
    // Init DFU service
    app_om_dfu_init();
    // Init ancs client
    ancs_client_init();
    // Init tspp
    app_tspp_init();
    // Init wechat service
    app_wechat_lite_init();
}

static void oele_init(void)
{
    // example: Event timer
    // evt_timer_set(&example_evt_timer, 2000, EVT_TIMER_REPEAT, example_evt_timer_handler, NULL);

    // example: sleep
    pm_sleep_enable(true);
    pm_sleep_notify_user_callback_register(example_pm_sleep_notify_handler);

    // example: pin wakeup
    drv_pmu_wakeup_pin_set(PAD_BUTTON_0, PMU_PIN_WAKEUP_LOW_LEVEL);
    drv_pmu_wakeup_pin_set(PAD_BUTTON_1, PMU_PIN_WAKEUP_LOW_LEVEL);
    drv_pmu_wakeup_pin_register_callback(example_pin_wakeup_isr_handler);

    // Running...
    OM_LOG(OM_LOG_INFO, "Simple APP Present.\r\n");
}

__RAM_CODES("APP")
static void main_schedule(void)
{
    while(1) {
        // event schedule handle
        evt_schedule();

        OM_CRITICAL_BEGIN();
        // no event, try low power
        if(evt_get_all() == 0) {
            pm_power_manage();
        }
        OM_CRITICAL_END();
    };
}

#if (RTE_PMU_POF_REGISTER_CALLBACK)
static void pmu_pof_isr_callback(void *om_pmu, drv_event_t event, void *voltage, void *mode)
{
    OM_LOG(OM_LOG_WARN, "PMU POF event occured, voltage: [%d], mode: [%d]",
        (uint32_t)voltage, (uint32_t)mode);
}
#endif


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
int main(void)
{
    // Disable WDT
    drv_wdt_init(0);

    // Init internal flash
    flash_config_t config = {
        .clk_div    = 0,
        .delay      = FLASH_DELAY_AUTO,
        .read_cmd   = FLASH_FAST_READ_QIO,
        .write_cmd  = FLASH_PAGE_PROGRAM,
        .spi_mode   = FLASH_SPI_MODE_0,
    };
    drv_flash_init(OM_FLASH0, &config);

    // Init board
    board_init();

    // Init RF
    drv_rf_init();

    // Init NVDS
    nvds_init(0);

    // Init LOG
    OM_LOG_INIT();

    // Init evt and evt timer
    evt_init();
    evt_timer_init();

    // pmu pof enable
    #if (RTE_PMU_POF_REGISTER_CALLBACK)
    drv_pmu_pof_register_callback(pmu_pof_isr_callback);
    #endif
    drv_pmu_pof_enable(true, PMU_POF_VOLTAGE_2P5V, PMU_POF_INT_NEG_EDGE);

    // Init Power management
    pm_init();

    // Init BLE
    ble_init();

    // Init other elements
    oele_init();

    // Main loop
    main_schedule();
}

/** @} */
