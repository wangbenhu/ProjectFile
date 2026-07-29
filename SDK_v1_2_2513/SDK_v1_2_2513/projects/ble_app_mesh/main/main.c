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
 * @brief    mesh app main entry
 * @details  mesh app main entry
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
#include "omsh_app.h"


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
#if (RTE_PMU_POF_REGISTER_CALLBACK)
static void pmu_pof_isr_callback(void *om_pmu, drv_event_t event, void *voltage, void *mode)
{
    OM_LOG(OM_LOG_WARN, "PMU POF event occured, voltage: [%d], mode: [%d]",
        (uint32_t)voltage, (uint32_t)mode);
}
#endif

static void system_init(void)
{
    // Disable WDT
    drv_wdt_init(0);
    // Init board
    board_init();
    // Init internal flash
    flash_config_t config = {
        .clk_div    = 0,
        .delay      = FLASH_DELAY_AUTO,
        .read_cmd   = FLASH_FAST_READ_QIO,
        .write_cmd  = FLASH_PAGE_PROGRAM,
        .spi_mode   = FLASH_SPI_MODE_0,
    };
    drv_flash_init(OM_FLASH0, &config);

    #if CONFIG_SHELL
    extern const shell_cmd_t mesh_shell_cmd[];
    shell_init(&mesh_shell_cmd[0]);
    #endif

    OM_LOG_INIT();

    // PMU pof enable
    #if (RTE_PMU_POF_REGISTER_CALLBACK)
    drv_pmu_pof_register_callback(pmu_pof_isr_callback);
    #endif
    drv_pmu_pof_enable(true, PMU_POF_VOLTAGE_2P5V, PMU_POF_INT_NEG_EDGE);

    nvds_init(0);

    // Start BLE Mesh Task
    vStartBleMeshTask();
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
int main(void)
{
    // Initialize CMSIS-RTOS
    osKernelInitialize();

    system_init();

    // Start thread execution
    if (osKernelGetState() == osKernelReady) {
        osKernelStart();
    }

    /* Never run here */
    while(1);
}

/** @} */
