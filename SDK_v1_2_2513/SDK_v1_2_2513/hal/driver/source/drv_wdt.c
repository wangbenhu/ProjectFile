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
 * @brief    WDT driver source file
 * @details  WDT driver source file
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
#include "RTE_driver.h"
#if (RTE_WDT)
#include <stddef.h>
#include "om_device.h"
#include "om_driver.h"


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    drv_isr_callback_t      isr_cb;
} wdt_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
#if (RTE_WDT_REGISTER_CALLBACK)
static wdt_env_t wdt_env = {
    .isr_cb = NULL,
};
#endif


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
#if (RTE_WDT_REGISTER_CALLBACK)
/**
 * @brief wdt register callback
 *
 * @param[in] isr_cb  callback
 *
 * @return None
 **/
void drv_wdt_register_isr_callback(drv_isr_callback_t isr_cb)
{
    wdt_env.isr_cb = isr_cb;
}
#endif

__WEAK void drv_wdt_isr_callback(void)
{
    #if (RTE_WDT_REGISTER_CALLBACK)
    if (wdt_env.isr_cb) {
        wdt_env.isr_cb(NULL, DRV_EVENT_COMMON_GENERAL, NULL, NULL);
    }
    #endif
}

/**
 * @brief watch dog keepalive
 *
 * @return None
 **/
void drv_wdt_keep_alive(void)
{
    // keep wdt alive can open wdt even if it has been closed
    OM_PMU->WDT_KR_CFG = 0xAAAA;
}

/**
 * @brief enable watch dog
 *
 * @param[in] timeout_ms  timeout with second
 **/
void drv_wdt_enable(uint32_t timeout_ms)
{
    OM_PMU->WDT_STATUS |= PMU_WDT_STATUS_WDT_FLAG_CLR_MASK;
    if (timeout_ms) {
        // Enable WDT cpu en int and NMI int
        register_set(&OM_PMU->WDT_STATUS, MASK_2REG(PMU_WDT_STATUS_WDT_INT_CPU_EN, 1, PMU_WDT_STATUS_WDT_INT_NMI_EN, 1));
        // Enable WDT NVIC int
        NVIC_ClearPendingIRQ(WDT_IRQn);
        NVIC_SetPriority(WDT_IRQn, RTE_WDT_IRQ_PRIORITY);
        NVIC_EnableIRQ(WDT_IRQn);
        //Initialize the watchdog counter
        while(OM_PMU->WDT_STATUS & PMU_WDT_STATUS_LD_WDT_KR_STATUS_MASK);
        OM_PMU->WDT_RLR_CFG = timeout_ms * 128 / 1000;
        OM_PMU->WDT_KR_CFG = 0x5555;
        while(OM_PMU->WDT_STATUS & PMU_WDT_STATUS_LD_WDT_KR_STATUS_MASK);
        OM_PMU->WDT_KR_CFG = 0xAAAA;
    } else {
        // Disable WDT cpu en int and NMI int
        register_set(&OM_PMU->WDT_STATUS, MASK_2REG(PMU_WDT_STATUS_WDT_INT_CPU_EN, 0, PMU_WDT_STATUS_WDT_INT_NMI_EN, 0));
        // Disable WDT NVIC int
        NVIC_DisableIRQ(WDT_IRQn);
    }
}

__RAM_CODES("PM") uint32_t drv_wdt_get_left(void)
{
    uint32_t left_count;
    while(OM_PMU->WDT_STATUS & PMU_WDT_STATUS_LD_WDT_KR_STATUS_MASK);
    if (register_get(&OM_PMU->WDT_KR_CFG, MASK_POS(PMU_WDT_KR_CFG_WDT_KR)) != 0x6666) {
        left_count = register_get(&OM_PMU->WDT_STATUS, MASK_POS(PMU_WDT_STATUS_WDT_TIMER));
        return (left_count * 1000 / 128);
    } else {
        return 0xFFFFFFFF;
    }
}

void drv_wdt_isr(void)
{
    OM_PMU->WDT_STATUS |= PMU_WDT_STATUS_WDT_FLAG_CLR_MASK;
    drv_wdt_isr_callback();
}


#endif  /* RTE_WDT */

/** @} */
