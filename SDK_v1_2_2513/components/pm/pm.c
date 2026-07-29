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
 * @brief    power manager for system
 * @details  power manager for system
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
#include "pm.h"
#if (CONFIG_PM)
#include "om_driver.h"
#include "om_common.h"


/*******************************************************************************
 * MACROS
 */
#ifndef CONFIG_PM_STORE_CALLBACK_NUM
#define CONFIG_PM_STORE_CALLBACK_NUM      6U
#endif
#ifndef CONFIG_PM_CHECKER_NUM
#define CONFIG_PM_CHECKER_NUM             4U
#endif


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    bool                   sleep_enable;
    bool                   ultra_sleep_enable;
    uint16_t               min_sleep_time;
    volatile uint32_t      sleep_state;
    pm_sleep_callback_t    notify_cb;
    pm_sleep_callback_t    store_cb[CONFIG_PM_STORE_CALLBACK_NUM];
    pm_checker_callback_t  checker_callback[CONFIG_PM_CHECKER_NUM];
    pm_checker_priority_t  checker_priority[CONFIG_PM_CHECKER_NUM];
} pm_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
static pm_env_t pm_env = {
    .min_sleep_time = PMU_TIMER_MS2TICK(3),
};
uint32_t pm_cpu_context[3];  /* used for store CPU context, $control, $MSP, $PSP register */


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
/**
 * @brief  pm system enter sleep
 **/
__RAM_CODE static void pm_system_enter_sleep(void)
{
    struct context_regs_tag {
        __IO uint32_t VTOR;
        __IO uint32_t NVIC_IPR[(EXTERNAL_IRQn_Num + 3U) >> 2U];    /* Interrupt Priority Register */
        __IO uint32_t NVIC_ISER[(EXTERNAL_IRQn_Num + 31U) >> 5U];  /* Interrupt Set Enable Register */
        __IO uint8_t SCB_SHPR[12];                                 /* System Handler Priority Register */
        __IO uint32_t SCB_CPACR;                                   /* Coprocessor Access Control Register */
    } context_regs;

    // store
    #if (RTE_FLASH0)
    drv_iflash_store();
    #endif
    context_regs.VTOR = SCB->VTOR;
    for (uint32_t i=0; i<sizeof(context_regs.NVIC_IPR)/sizeof(context_regs.NVIC_IPR[0]); i++) {
        context_regs.NVIC_IPR[i] = ((__IO uint32_t *)(NVIC->IPR))[i];
    }
    for (uint32_t i=0; i<sizeof(context_regs.NVIC_ISER)/sizeof(context_regs.NVIC_ISER[0]); i++) {
        context_regs.NVIC_ISER[i] = NVIC->ISER[i];
    }
    for (uint32_t i=0; i<sizeof(context_regs.SCB_SHPR)/sizeof(context_regs.SCB_SHPR[0]); i++) {
        context_regs.SCB_SHPR[i] = SCB->SHPR[i];
    }
    context_regs.SCB_CPACR = SCB->CPACR;
    do {
        extern void pm_cpu_context_store(void);
        pm_cpu_context_store();
    } while(0);
    __disable_irq();  // disable global interrupt before NVIC restore
    // Enable DWT Cycle counter
    do {
        COREDEBUG->DEMCR |= COREDEBUG_DEMCR_TRCENA_MASK;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    } while(0);
    // restore
    #if (RTE_FLASH0)
    drv_iflash_restore();
    #endif
    SCB->VTOR = context_regs.VTOR;
    for (uint32_t i=0; i<sizeof(context_regs.NVIC_IPR)/sizeof(context_regs.NVIC_IPR[0]); i++) {
        ((__IO uint32_t *)(NVIC->IPR))[i] = context_regs.NVIC_IPR[i];
    }
    for (uint32_t i=0; i<sizeof(context_regs.NVIC_ISER)/sizeof(context_regs.NVIC_ISER[0]); i++) {
        NVIC->ISER[i] = context_regs.NVIC_ISER[i];
    }
    for (uint32_t i=0; i<sizeof(context_regs.SCB_SHPR)/sizeof(context_regs.SCB_SHPR[0]); i++) {
        SCB->SHPR[i] = context_regs.SCB_SHPR[i];
    }
    SCB->CPACR = context_regs.SCB_CPACR;

    // if icache power off in sleep, cache should be re-enable
    drv_icache_enable();
}

/**
 * @brief  system sleep notify
 *
 * @param[in] sleep_state  sleep state
 * @param[in] power_status  power status
 **/
__RAM_CODES("PM")
static void pm_sleep_notify(pm_sleep_state_t sleep_state, pm_status_t power_status)
{
    switch (sleep_state) {
        case PM_SLEEP_ENTRY:
            #if (RTE_OM24G)
            drv_om24g_store();
            #endif
            #if (RTE_OSPI1)
            drv_ospi_store();
            #endif
            break;
        case PM_SLEEP_RESTORE_HSI:
            #if (RTE_OSPI1)
            drv_ospi_restore();
            #endif
            #if (RTE_CALIB)
            drv_calib_sys_restore();
            drv_calib_rf_restore();
            #endif
            break;
        case PM_SLEEP_RESTORE_HSE:
            break;
        case PM_SLEEP_LEAVE_BOTTOM_HALF:
            #if (RTE_OM24G)
            drv_om24g_restore();
            #endif
            break;
        default:
            break;
    }

    // callback user register store/restore
    if (sleep_state != PM_SLEEP_LEAVE_BOTTOM_HALF) {
        for (int i = 0; i < sizeof(pm_env.store_cb)/sizeof(pm_env.store_cb[0]); i++) {
            if (pm_env.store_cb[i]) {
                pm_env.store_cb[i](sleep_state, power_status);
            }
        }
    }

    if (pm_env.notify_cb) {
        pm_env.notify_cb(sleep_state, power_status);
    }
}

/**
 * @brief  system enter sleep
 **/
__RAM_CODES("PM")
static void pm_sleep_enter_common_sleep(pm_status_t power_status)
{
    pm_sleep_notify(PM_SLEEP_STORE, power_status);
    drv_pmu_sleep_enter((power_status == PM_STATUS_DEEP_SLEEP) ? 1U : 0U, /*lint -e747 reboot*/false);

    /* fixed execute NMI ISR from wakeup */
    #if (RTE_WDT)
    uint32_t wdt_status;
    wdt_status = OM_PMU->WDT_STATUS;
    OM_PMU->WDT_STATUS &= ~PMU_WDT_STATUS_WDT_INT_NMI_EN_MASK;
    #endif

    pm_system_enter_sleep();

    // modified default LCD clock disable from sleep
    DRV_RCC_CLOCK_ENABLE(RCC_CLK_LCD, 0);
    OM_CPM->AHB_CFG |= CPM_AHB_CFG_RAM_AUTO_GATE_EN_MASK;

    drv_pmu_sleep_leave(PMU_SLEEP_LEAVE_STEP1_ON_RC32M);
    pm_sleep_notify(PM_SLEEP_RESTORE_HSI, power_status);

    drv_pmu_sleep_leave(PMU_SLEEP_LEAVE_STEP2_WAIT_XTAL32M);
    pm_sleep_notify(PM_SLEEP_RESTORE_HSE, power_status);

    drv_pmu_sleep_leave(PMU_SLEEP_LEAVE_STEP3_FINISH);
    pm_sleep_notify(PM_SLEEP_LEAVE_BOTTOM_HALF, power_status);

    #if (RTE_WDT)
    OM_PMU->WDT_STATUS = wdt_status;
    #endif
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */

/**
 * @brief  system sleep
 *
 * @param[in] status  status
 **/
__RAM_CODES("PM")
void pm_sleep(pm_status_t status)
{
    /* IRQ has been disabled */
    switch(status) {
        case PM_STATUS_SLEEP:
            pm_sleep_enter_common_sleep(PM_STATUS_SLEEP);
            break;

        case PM_STATUS_DEEP_SLEEP:
            if (pm_env.ultra_sleep_enable) {
                pm_sleep_notify(PM_SLEEP_STORE, PM_STATUS_DEEP_SLEEP);
                drv_pmu_sleep_enter(1U, /*lint -e747 reboot*/true);
                while(1);
            } else {
                pm_sleep_enter_common_sleep(PM_STATUS_DEEP_SLEEP);
            }
            break;

        case PM_STATUS_IDLE:
            __WFI();
            break;

        default:
            break;
    }
}

/**
 * @brief  pm sleep checker check
 *
 * @return status
 **/
__RAM_CODES("PM")
pm_status_t pm_sleep_checker_check(void)
{
    pm_status_t status;

    // check pm self sleep state
    if (pm_env.sleep_state) {
        return PM_STATUS_IDLE;
    }

    // check pmu pin-wakeup
    if (drv_pmu_deep_sleep_is_allow()) {
        status = PM_STATUS_DEEP_SLEEP;
    } else {
        return PM_STATUS_IDLE;
    }

    // check PMU_TIMER
    do {
        uint32_t min_left_time = PMU_TIMER_MAX_TICK;
        #if (RTE_PMU_TIMER)
        for (pmu_timer_trig_t i=PMU_TIMER_TRIG_VAL0; i<=PMU_TIMER_TRIG_VAL1; i++) {
            uint32_t left_time = drv_pmu_timer_left_time_get(i);
            if (min_left_time > left_time) {
                min_left_time = left_time;
            }
        }
        #endif

        #if (RTE_WDT)
        uint32_t left_time = drv_wdt_get_left();
        if (min_left_time > left_time) {
            min_left_time = left_time;
        }
        #endif

        if (min_left_time < pm_env.min_sleep_time) {
            return PM_STATUS_IDLE;
        } else if (min_left_time == PMU_TIMER_MAX_TICK) {
            status = PM_STATUS_DEEP_SLEEP;
        } else {
            status = PM_STATUS_SLEEP;
        }
    } while(0);

    for (int i = 0; i < sizeof(pm_env.checker_callback)/sizeof(pm_env.checker_callback[0]); ++i) {
        if (pm_env.checker_callback[i] != NULL) {
            pm_status_t checker_status = pm_env.checker_callback[i]();
            if (status > checker_status) {
                status = checker_status;
            }
            if (status <= PM_STATUS_IDLE) {
                goto _exit;
            }
        }
    }

_exit:
    #if (RTE_CALIB)
    drv_calib_repair_rc_rf_temperature_check();
    drv_calib_repair_rc32k_temperature_check();
    #endif

    return status;
}

/**
 * @brief  system sleep min time set
 *
 * @param[in] time  32k tick
 **/
void pm_sleep_min_time_set(uint16_t tick_32k)
{
    pm_env.min_sleep_time = tick_32k;
}

/**
 *******************************************************************************
 * @brief  pm sleep min time get
 *
 * @return  32k tick
 *******************************************************************************
 */
__RAM_CODES("PM")
uint16_t pm_sleep_min_time_get(void)
{
    return pm_env.min_sleep_time;
}

/**
 *******************************************************************************
 * @brief  pm sleep ultra sleep mode enable
 *
 * @param[in] enable  enable
 *******************************************************************************
 */
void pm_sleep_ultra_sleep_mode_enable(bool enable)
{
    pm_env.ultra_sleep_enable = enable;
}

/**
 *******************************************************************************
 * @brief  pm sleep ultra sleep mode enable
 *
 * @return  is enabled
 *******************************************************************************
 */
bool pm_sleep_ultra_sleep_mode_is_enabled(void)
{
    return pm_env.ultra_sleep_enable;
}

/**
 *******************************************************************************
 * @brief  pm sleep enable
 *
 * @param[in] enable  enable
 *******************************************************************************
 **/
void pm_sleep_enable(bool enable)
{
    pm_env.sleep_enable = enable;
}

/**
 *******************************************************************************
 * @brief  pm sleep prevent
 *
 * @param[in] id  id, reference @ref pm_id_t
 *******************************************************************************
 **/
__RAM_CODES("PM")
void pm_sleep_prevent(pm_id_t id)
{
    uint32_t mask  = 1u << ((int)id);

    OM_CRITICAL_BEGIN();
    pm_env.sleep_state |= mask;
    OM_CRITICAL_END();
}

/**
 *******************************************************************************
 * @brief  pm sleep allow
 *
 * @param[in] id  id, reference @ref pm_id_t
 *
 *******************************************************************************
 **/
__RAM_CODES("PM")
void pm_sleep_allow(pm_id_t id)
{
    uint32_t mask = (1u << (int)id);

    OM_CRITICAL_BEGIN();
    pm_env.sleep_state &= ~mask;
    OM_CRITICAL_END();
}

/**
 *******************************************************************************
 * @brief  pm sleep checker register
 *
 * @param[in] priority  priority
 * @param[in] checker_cb  checker cb
 *******************************************************************************
 **/
om_error_t pm_sleep_checker_callback_register(pm_checker_priority_t priority, pm_checker_callback_t checker_cb)
{
    om_error_t error;

    error = OM_ERROR_OUT_OF_RANGE;
    for (int n = 0; n < sizeof(pm_env.checker_callback)/sizeof(pm_env.checker_callback[0]); ++n) {
        OM_CRITICAL_BEGIN();
        if (pm_env.checker_callback[n] == NULL) {
            // sort and insert
            for (int i = 0; i < n; ++i) {
                // check priority
                if (pm_env.checker_priority[i] < priority) {
                    // move
                    for (int j = n; j > i; --j) {
                        pm_env.checker_callback[j] = pm_env.checker_callback[j-1];
                        pm_env.checker_priority[j] = pm_env.checker_priority[j-1];
                    }
                    // insert
                    pm_env.checker_priority[i] = priority;
                    pm_env.checker_callback[i] = checker_cb;
                    error = OM_ERROR_OK;
                    goto _exit;
                }
            }

            pm_env.checker_priority[n] = priority;
            pm_env.checker_callback[n] = checker_cb;
            error = OM_ERROR_OK;
        } else if (pm_env.checker_callback[n] == checker_cb) {
            error = OM_ERROR_OK;
        }

    _exit:
        OM_CRITICAL_END();
        if (error == OM_ERROR_OK) {
            break;
        }
    }

    return error;
}

/**
 * @brief  system sleep notify user callback register
 *
 * @param[in] notify_cb  sleep notify cb
 **/
void pm_sleep_notify_user_callback_register(pm_sleep_callback_t notify_cb)
{
    pm_env.notify_cb = notify_cb;
}

om_error_t pm_sleep_store_restore_callback_register(pm_sleep_callback_t store_cb)
{
    int i, n;
    om_error_t error;

    n = sizeof(pm_env.store_cb) / sizeof(pm_env.store_cb[0]);
    error = OM_ERROR_OUT_OF_RANGE;
    OM_CRITICAL_BEGIN();
    for (i = 0; i < n; i++) {
        if ((pm_env.store_cb[i] == NULL) || (pm_env.store_cb[i] == store_cb)) {
            pm_env.store_cb[i] = store_cb;
            error = OM_ERROR_OK;
            break;
        }
    }
    OM_CRITICAL_END();

    return error;
}

__RAM_CODES("PM")
void pm_power_manage(void)
{
    pm_status_t status;

    // 1st. check pm sleep
    status = pm_sleep_checker_check();
    // 2th. check sleep enable
    if (!pm_env.sleep_enable) {
        if (status > PM_STATUS_IDLE) {
            status = PM_STATUS_IDLE;
        }
    }

    pm_sleep(status);
}

void pm_init(void)
{
    #if (!RTE_FLASH1_XIP)  // disable OSPI1 clock if not execute OFLASH
    DRV_RCC_CLOCK_ENABLE(RCC_CLK_OSPI1, 0);
    #endif
}
#else
void pm_sleep_prevent(pm_id_t id)
{
}

void pm_sleep_allow(pm_id_t id)
{
}

om_error_t pm_sleep_checker_callback_register(pm_checker_priority_t priority, pm_checker_callback_t checker_cb)
{
    return OM_ERROR_OK;
}

om_error_t pm_sleep_store_restore_callback_register(pm_sleep_callback_t store_cb)
{
    return OM_ERROR_OK;
}
#endif

/** @} */
