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
 * @brief    fault handle
 * @details  fault handle
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
#if (CONFIG_FAULT_HANDLE)
#include <stdint.h>
#include <stddef.h>
#include "om_driver.h"
#include "fault_handle.h"
#include "om_log.h"


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
__WEAK void fault_context_store(fault_id_t fault_id, uint8_t *context, uint32_t context_len, uint32_t param)
{
    #if (CONFIG_OM_LOG)
    const char *fault_str[] = {
        "LVD",          /* low voltage detect */
        "WDT",          /* WDT timeout */
        "HARD",         /* Hardfault, OM_ASSERT */
        "USER ",        /* User Check */
    };
    uint32_t *reg = (uint32_t *)context;

    OM_LOG(OM_LOG_ERROR, "FAULT by %s, PC at 0x%08x\r\n", fault_str[fault_id], reg[6]);
    OM_LOG_HEXDUMP(OM_LOG_ERROR, (uint8_t*)context, context_len, sizeof(uint32_t));
    OM_LOG_FLUSH();
    #endif

    while(1) {
        #if 0 && (RTE_WDT)
        drv_wdt_keep_alive();
        #endif
        DRV_DELAY_MS(100);
    }
}

void __WEAK fault_callback(fault_id_t fault_id, uint8_t *context, uint32_t context_len, uint32_t param)
{
    __disable_irq();
    fault_context_store(fault_id, context, context_len, param);
    drv_pmu_reset((fault_id == FAULT_ID_WDT) ? PMU_REBOOT_FROM_WDT : PMU_REBOOT_FROM_SOFT_RESET_USER);
    while(1);
}

void fault_handler_c(uint32_t exc_return_code, uint32_t sp)
{
    fault_callback(FAULT_ID_HARD_FAULT, (uint8_t *)sp, 32U, exc_return_code);
}

void fault_wdt_callback(uint32_t exc_return_code, uint32_t sp)
{
    fault_callback(FAULT_ID_WDT, (uint8_t *)sp, 32U, exc_return_code);
}

#endif

/** @} */
