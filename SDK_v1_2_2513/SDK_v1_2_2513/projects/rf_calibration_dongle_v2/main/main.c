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
 * @brief    dongle manual calibration
 * @details  dongle manual calibration
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
#include "om_log.h"
#include "app_calib_dongle.h"
#include "freq_calib_ble.h"
/*******************************************************************************
 * EXTERN FUNCTIONS
 */


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

static void oele_init(void)
{
    // example: Event timer
    // evt_timer_set(&example_evt_timer, 2000, EVT_TIMER_REPEAT, example_evt_timer_handler, NULL);

    // example: sleep
    pm_sleep_enable(false);
    pm_sleep_notify_user_callback_register(example_pm_sleep_notify_handler);

    // example: pin wakeup
    drv_pmu_wakeup_pin_set(PAD_BUTTON_0, PMU_PIN_WAKEUP_LOW_LEVEL);
    drv_pmu_wakeup_pin_set(PAD_BUTTON_1, PMU_PIN_WAKEUP_LOW_LEVEL);
    drv_pmu_wakeup_pin_register_callback(example_pin_wakeup_isr_handler);
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

    // Init LOG
    OM_LOG_INIT();

    OM_LOG(OM_LOG_INFO, "****************************************\n");
    if (0 != drv_gpio_read(OM_GPIO0, GPIO_MASK(PAD_BUTTON_0))) {
        OM_LOG(OM_LOG_INFO, "* ble \n");
        ble_calib_entry();
    } else {
        OM_LOG(OM_LOG_INFO, "* om24g \n");
        app_calib_init();
    }
    OM_LOG(OM_LOG_INFO, "* version: v2.0.0.0                     \n");
    OM_LOG(OM_LOG_INFO, "* compile time: %s  %s\n", __DATE__, __TIME__);
    OM_LOG(OM_LOG_INFO, "****************************************\n");

    // pmu pof enable
    #if (RTE_PMU_POF_REGISTER_CALLBACK)
    drv_pmu_pof_register_callback(pmu_pof_isr_callback);
    #endif
    drv_pmu_pof_enable(true, PMU_POF_VOLTAGE_2P5V, PMU_POF_INT_NEG_EDGE);

    // Init Power management
    pm_init();


    // // Init other elements
    oele_init();

    // Main loop
    main_schedule();
}

/** @} */
