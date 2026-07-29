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
#include "autoconf.h"
#include "om_log.h"
#include "om_driver.h"
#include "bsp.h"
#include "nvds.h"
#if (CONFIG_PM)
#include "pm.h"
#endif
#if (CONFIG_SHELL)
#include "shell.h"
#endif
#if (CONFIG_FAULT_HANDLE)
#include "fault_handle.h"
#endif


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
extern void vStartEvtTask(void);
extern void cmd_shell_pta(int argc, char *argv[]);


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
#if (CONFIG_PM)
void pm_sleep_callback(pm_sleep_state_t sleep_state, pm_status_t power_status)
{
    switch (sleep_state) {
        case PM_SLEEP_ENTRY:
            break;
        case PM_SLEEP_RESTORE_HSI:
            break;
        case PM_SLEEP_RESTORE_HSE:
            break;
        case PM_SLEEP_LEAVE_BOTTOM_HALF:
            break;
        default:
            break;
    }
}
#endif

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
void system_init(void)
{
    const flash_config_t config = {
        .clk_div = 0,
        .delay = FLASH_DELAY_AUTO,
        .read_cmd = FLASH_FAST_READ_QIO,
        .write_cmd = FLASH_PAGE_PROGRAM,
        .spi_mode = FLASH_SPI_MODE_0,
    };
    drv_wdt_init(0);
    board_init();
    drv_flash_init(OM_FLASH0, &config);

    #if (CONFIG_SHELL)
    const shell_cmd_t ble_hci_shell_cmd[] = {
        { "pta",     cmd_shell_pta,     "pta <priority_threshold>" },
        { NULL,      NULL,              NULL},     /* donot deleted */
    };
    shell_init(ble_hci_shell_cmd);
    #endif

    OM_LOG_INIT();

    // pmu pof enable
    #if (RTE_PMU_POF_REGISTER_CALLBACK)
    drv_pmu_pof_register_callback(pmu_pof_isr_callback);
    #endif
    drv_pmu_pof_enable(true, PMU_POF_VOLTAGE_2P5V, PMU_POF_INT_NEG_EDGE);

    #if (CONFIG_PM)
    pm_init();
    pm_sleep_enable(false);
    pm_sleep_notify_user_callback_register(pm_sleep_callback);
    #endif

    nvds_init(0);

    // Start Evt Task
    vStartEvtTask();
}

/** @} */
