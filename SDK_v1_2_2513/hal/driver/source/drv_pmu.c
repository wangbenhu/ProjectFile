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
 * @brief    PMU driver source file
 * @details  PMU driver source file
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
#if (RTE_PMU)
#include <stddef.h>
#include "om_common.h"
#include "om_driver.h"


/*******************************************************************************
 * MACROS
 */
#define PMU_IO_OUTPUT_LATCH_CTRL_BY_SOFTWARE     1


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    volatile uint32_t ana_mask;
    // wakeup pin edge
    volatile uint32_t pin_wakeup_none_edge_mask[2];
    volatile uint32_t pin_wakeup_double_edge_mask[2];
    #if (RTE_PMU_PIN_WAKEUP_REGISTER_CALLBACK)
    drv_isr_callback_t pin_wakeup_isr_callback;
    #endif
    #if (RTE_PMU_POF_REGISTER_CALLBACK)
    drv_isr_callback_t pof_isr_callback;
    #endif
    pmu_32k_sel_t clock_32k;
    volatile bool pin_wakeup_sleep_recently;
    uint8_t enable_32k_with_deep_sleep                  : 1;
    uint8_t enable_pof                                  : 1;
    uint8_t reserved                                    : 6;
} pmu_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
static pmu_env_t drv_pmu_env;
pmu_dcdc_info_t pmu_dcdc_info;


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief  drv pmu topclk xtal32m switch to fast startup mode
 *******************************************************************************
 */
static void drv_pmu_xtal32m_switch_to_fast_startup_mode(void)
{
    // open xtal32m ctrl clock
    REGW1(&OM_DAIF->CLK_ENS, DAIF_XTAL32M_CTRL_CLK_EN_MASK);

    // Next XTAL32M startup use fast-startup mode (this will lead XTAL32M non-ready)
    REGW0(&OM_PMU->MISC_CTRL, PMU_MISC_CTRL_FIRST_RUN_REG_MASK);
    // No real fast startup flow
    REGW(&OM_PMU->XTAL32M_CNS0, MASK_4REG(PMU_XTAL32M_CNS0_EN_OSC32M_CHIRPRAMP_ME,1, PMU_XTAL32M_CNS0_EN_OSC32M_CHIRPRAMP_MO,0,
                                          PMU_XTAL32M_CNS0_EN_XTAL32M_NRB_ME,1,      PMU_XTAL32M_CNS0_EN_XTAL32M_NRB_MO,0));
    // Restart
    REGW1(&OM_PMU->XTAL32M_CNS1, PMU_XTAL32M_CNS1_XTAL32M_RESTART_MASK);
    DRV_DELAY_US(200);    // When fast-startup process has been done once, need 200us to wait for ready status
    // Wait ready
    while(!(OM_DAIF->XTAL32M_INTRS & DAIF_XTAL32M_CLK_RDY_MASK));
    // FSM ctrl
    REGW(&OM_PMU->XTAL32M_CNS0, MASK_2REG(PMU_XTAL32M_CNS0_EN_OSC32M_CHIRPRAMP_ME,0, PMU_XTAL32M_CNS0_EN_XTAL32M_NRB_ME,0));

    // close xtal32m ctrl clock
    REGW0(&OM_DAIF->CLK_ENS, DAIF_XTAL32M_CTRL_CLK_EN_MASK);
}

/**
 * @brief  pmu xtal32m startup param setup
 **/
static void drv_pmu_xtal32m_startup_param_setup(void)
{
    REGW(&OM_PMU->CLK_CTRL_1, MASK_1REG(PMU_CLK_CTRL_1_SEL_XTAL32M_GM, 4)); // from 2 to 4
}

/**
 * @brief  pmu xtal32m fast startup param setup
 **/
static void drv_pmu_xtal32m_fast_startup_param_setup(void)
{
    REGW(&OM_PMU->XTAL32M_CNS1, MASK_1REG(PMU_XTAL32M_CNS1_XTAL32M_NRB_POR,3));
}

/**
 *******************************************************************************
 * @brief pmu reset cpu
 *
 * @note: This function must be INLINE function, avoid stack used
 *******************************************************************************
 **/
static void pmu_cpu_reset(void)
{
    __disable_irq();
    __DMB();
    OM_PMU->DIG_RESET = PMU_DIG_RESET_KEY;
    while(1);
}

/**
 * @brief Force into reboot sleep mode
 *
 * Power consumption is lower than the deep sleep mode. All SRAM will be powered down.
 * But chip will be reboot from ROM when wakeup.
 *
 * @return None
 **/
static void drv_pmu_force_into_reboot_sleep_mode(void)
{
    __disable_irq();
    drv_pmu_iflash_power_mode_set(PMU_IFLASH_AUTO_SLEEP_POWER_DOWN_ENABLE);
    register_set(&OM_PMU->RSVD_SW_REG[0], PMU_RSVD_SW_REG_REBOOT_REASON_MASK, PMU_RSVD_SW_REG_REBOOT_FROM_ULTRA_DEEP_SLEEP);
    SCB->SCR |= 0x04;    // DEEPSLEEP flag
    OM_PMU->SLEEP_WAKE_CTRL &= ~(PMU_SLEEP_WAKE_CTRL_RETENTION_RESTORE_EN_MASK + PMU_SLEEP_WAKE_CTRL_RETENTION_SAVE_EN_MASK);  // disable HW store/restore register
    // All RAM into power off mode in sleep
    OM_PMU->PSO_PM &= ~(PMU_PSO_RAM1_2_POWER_ON_MASK | PMU_PSO_RAM3_POWER_ON_MASK | PMU_PSO_RAM4_POWER_ON_MASK | PMU_PSO_RAM5_POWER_ON_MASK | PMU_PSO_ICACHE_POWER_ON_MASK);
    OM_PMU->RAM_CTRL_2 = (OM_PMU->RAM_CTRL_2 & (~(0xFFFU << 18)))
                        | PMU_RAM_CTRL_2_RAM0_SD_HW_CTRL_EN_MASK | PMU_RAM_CTRL_2_RAM1_SD_HW_CTRL_EN_MASK
                        | PMU_RAM_CTRL_2_RAM2_SD_HW_CTRL_EN_MASK | PMU_RAM_CTRL_2_RAM_PSO_SD_HW_CTRL_EN_MASK
                        | PMU_RAM_CTRL_2_RAM_BLE_SD_HW_CTRL_EN_MASK | PMU_RAM_CTRL_2_RAM_IC_SD_HW_CTRL_EN_MASK;
    OM_PMU->RAM_CTRL_1 = (OM_PMU->RAM_CTRL_1 & (~((3U << 30) | (3U << 26) | (3U << 22))))
                        | PMU_RAM_CTRL_1_RAM3_SD_HW_CTRL_EN_MASK
                        | PMU_RAM_CTRL_1_RAM4_SD_HW_CTRL_EN_MASK
                        | PMU_RAM_CTRL_1_RAM5_SD_HW_CTRL_EN_MASK;
    __WFI();
    pmu_cpu_reset();  // Must be some IRQ pending, Force reboot
}

/**
 * @brief  pmu xtal32m fast startup
 *
 * @param[in] force  force
 **/
static void drv_pmu_xtal32m_fast_startup(bool force)
{
    DRV_RCC_ANA_CLK_ENABLE();

    // check
    if(force || !drv_pmu_xtal32m_is_enabled()) {
        // Next XTAL32M startup use fast-startup mode.
        REGW0(&OM_PMU->MISC_CTRL, PMU_MISC_CTRL_FIRST_RUN_REG_MASK);

        // Make sure CPU running on RC32M->startup RC32m->switch xtal32m->power down rc32m
        drv_rcc_cpu_clk_source_set(RCC_CPU_CLK_SOURCE_RC32M);
        drv_pmu_xtal32m_fast_startup_param_setup();
        drv_pmu_xtal32m_enable(true);
        drv_rcc_cpu_clk_source_set(RCC_CPU_CLK_SOURCE_XTAL32M);
        drv_rcc_periph_clk_source_set(RCC_PERIPH_CLK_SOURCE_XTAL32M);
        drv_pmu_rc32m_enable(false);
    }

    SystemCoreClock = 32 * 1000 * 1000U;

    DRV_RCC_ANA_CLK_RESTORE();
}


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Set dvdd voltage
 *
 * @param[in] voltage  dvdd voltage
 *******************************************************************************
 */
__RAM_CODE void drv_pmu_dvdd_voltage_set(pmu_dvdd_voltage_t voltage)
{
    OM_CRITICAL_BEGIN();
    drv_pmu_register_step_set(&OM_PMU->ANA_REG, MASK_STEP_SSAT(PMU_ANA_REG_PMU_DIG_LDO_TRIM, drv_calib_repair_env.dig_ldo + (int8_t)voltage), true/*should_update*/, 10/*delay_us*/);
    #if (RTE_FLASH0)
    drv_iflash_delay_recalib();
    #endif
    OM_CRITICAL_END();
}

__RAM_CODES("PM")
void drv_pmu_basic_reg_set(uint32_t mask, uint32_t mask_value)
{
    register uint32_t reg_prev;

    mask_value = mask_value & mask;
    OM_CRITICAL_BEGIN();
    reg_prev = OM_PMU->BASIC;
    if ((reg_prev & mask) != mask_value) {
        reg_prev &= ~mask;
        reg_prev |= mask_value;
        OM_PMU->BASIC = reg_prev;
        while ((OM_PMU->BASIC & mask) != mask_value);
    }
    OM_CRITICAL_END();
}

/**
 * @brief  pmu 32k switch to rc
 *
 * @param[in] calib  calib
 * @param[in] pd_others  pd others
 **/
void drv_pmu_32k_switch_to_rc(bool calib, bool pd_others)
{
    // Power on rc32k
    // REGW0(&OM_PMU->MISC_CTRL, PMU_MISC_REG_PD_RC32K_MASK);
    // while(!(OM_PMU->STATUS_READ & PMU_STATUS_READ_RC32K_READY_MASK));
    // calib it
    if(calib) {
        drv_calib_rc32k();
    }

    // Switch
    REGW(&OM_PMU->MISC_CTRL, MASK_1REG(PMU_MISC_CTRL_CLK_32K_SEL, PMU_32K_SEL_RC));
    while((OM_PMU->STATUS_READ & (PMU_STATUS_READ_CLK_32K_RC_CRY_DONE_MASK|PMU_STATUS_READ_CLK_32K_DIV_DONE_MASK)) !=
                                 (PMU_STATUS_READ_CLK_32K_RC_CRY_DONE_MASK|PMU_STATUS_READ_CLK_32K_DIV_DONE_MASK));

    if (pd_others) {
        REGW1(&OM_PMU->MISC_CTRL, PMU_MISC_CTRL_PD_CRY32K_MASK); // xtal32k
    }
}

/**
 * @brief  pmu topclk double preset
 **/
void drv_pmu_clk64m_enable(bool enable)
{
    OM_CRITICAL_BEGIN();
    if (enable) {
        REGW1(&OM_PMU->ANA_PD, PMU_ANA_PD_EN_64M_MASK);
    } else {
        REGW0(&OM_PMU->ANA_PD, PMU_ANA_PD_EN_64M_MASK);
    }
    OM_CRITICAL_END();
}

/**
 * @brief  pmu topclk xtal64m is enabled
 **/
bool drv_pmu_clk64m_is_enabled(void)
{
    return (OM_PMU->ANA_PD & PMU_ANA_PD_EN_64M_MASK) ? true : false;
}

/**
 * @brief  pmu rc32m power enable
 *
 * @return is_last_enabled ?
 **/
__RAM_CODE bool drv_pmu_rc32m_enable(bool enable)
{
    bool is_last_enabled = (OM_PMU->MISC_CTRL_1 & PMU_MISC_CTRL_1_REG_PD_RC32M_MASK) ? false : true;

    if(enable) {
        if (!is_last_enabled) {
            REGW0(&OM_PMU->MISC_CTRL_1, PMU_MISC_CTRL_1_REG_PD_RC32M_MASK);
            // must delay more than 15us
            DRV_DELAY_US(10 *3);
        }
    } else {
        if (is_last_enabled) {
            REGW1(&OM_PMU->MISC_CTRL_1, PMU_MISC_CTRL_1_REG_PD_RC32M_MASK);
        }
    }

    return is_last_enabled;
}

/**
 * @brief  pmu topclk xtal32m power enable
 *
 * @param[in] enable
 *
 * XTAL32M
 *
 * if CRY32M_EN==1, xtal32m will be fast-startuped automatically after wakeup (ignore PD_CRY32M)
 *
 * CRY32M_EN does not control xtal32m directly, fucntion:
 *   - xtal32m fast-startup FLAG after wakeup (ignore PD_CRY32M)
 *   - 0 to reset xtal32m-startup-ready signal
 *
 * PD_CRY32M edge can control xtal32m directly, function:
 *   - rising edge: power down
 *   - falling edge: power on
 *
 **/
void drv_pmu_xtal32m_enable(bool enable)
{
    DRV_RCC_ANA_CLK_ENABLE();
    if(enable) {
        // open xtal32m ctrl clock
        REGW1(&OM_DAIF->CLK_ENS, DAIF_XTAL32M_CTRL_CLK_EN_MASK);
        // Power on
        REGW1(&OM_PMU->MISC_CTRL_1, PMU_MISC_CTRL_1_CRY32M_EN_MASK);
        // Wait xtal32m ready
        while(!(OM_DAIF->XTAL32M_INTRS & DAIF_XTAL32M_CLK_RDY_MASK));
        // close xtal32m ctrl clock
        REGW0(&OM_DAIF->CLK_ENS, DAIF_XTAL32M_CTRL_CLK_EN_MASK);
    } else {
        REGW0(&OM_PMU->MISC_CTRL_1, PMU_MISC_CTRL_1_CRY32M_EN_MASK);
    }
    DRV_RCC_ANA_CLK_RESTORE();
}

/**
 * @brief  pmu topclk xtal32m is enabled
 **/
bool drv_pmu_xtal32m_is_enabled(void)
{
    // if cpu clk source is cpu_clk_in(xtal32m, xtal64m, syspll96m), then we say xtal32m is enabled
    return (OM_PMU->MISC_CTRL & PMU_MISC_CTRL_MAIN_CLK_SEL_MASK) ? true : false;
}

/**
 * @brief When this Function is called:
 * 1. cpu clk and periph clk will switch to xtal32m.
 * 2. rc32m will be power down.
 * 3. RC, RC32M will be calibed.
 * 4. fast startup mode will be set.
 **/
void drv_pmu_xtal32m_startup(void)
{
    DRV_RCC_ANA_CLK_ENABLE();

    if (!drv_pmu_xtal32m_is_enabled() || (drv_pmu_reboot_reason() == PMU_REBOOT_FROM_ULTRA_DEEP_SLEEP)) {
        // Next XTAL32M startup use normal-startup mode.
        REGW1(&OM_PMU->MISC_CTRL, PMU_MISC_CTRL_FIRST_RUN_REG_MASK);

        // Make sure CPU running on RC32M
        drv_rcc_cpu_clk_source_set(RCC_CPU_CLK_SOURCE_RC32M);
        drv_rcc_periph_clk_source_set(RCC_PERIPH_CLK_SOURCE_RC32M);

        // Try open xtal32m
        drv_pmu_xtal32m_startup_param_setup();
        drv_pmu_xtal32m_enable(true);

        // calib RC
        drv_calib_sys_rc();
        // switch to fast startup mode (must before switch_to_xtal32m)
        drv_pmu_xtal32m_switch_to_fast_startup_mode();
        // to xtal32m
        drv_rcc_cpu_clk_source_set(RCC_CPU_CLK_SOURCE_XTAL32M);
        drv_rcc_periph_clk_source_set(RCC_PERIPH_CLK_SOURCE_XTAL32M);
        // calib RC32M
        drv_calib_sys_rc32m();
        // power off rc32m
        drv_pmu_rc32m_enable(false);
    } else {
        // Next XTAL32M startup use fast-startup mode.
        REGW0(&OM_PMU->MISC_CTRL, PMU_MISC_CTRL_FIRST_RUN_REG_MASK);
    }

    // disable all daif clock
    OM_DAIF->CLK_ENS = 0;
    SystemCoreClock = 32 * 1000 * 1000U;
    DRV_RCC_ANA_CLK_RESTORE();
}

/**
 *******************************************************************************
 * @brief  syspll power enable (pll96M and 48M)
 *
 * @param[in] enable  enable
 *******************************************************************************
 */
void drv_pmu_syspll_power_enable(uint8_t enable)
{
    DRV_RCC_ANA_CLK_ENABLE();
    OM_CRITICAL_BEGIN();

    if (enable) {
        if (OM_DAIF->SYSPLL_CNS0 & DAIF_SYSPLL_PD_VBAT_MASK) {
            OM_DAIF->CLK_CFG |= DAIF_XTAL32M_EN_CKO16M_SYSPLL_MASK;
            OM_DAIF->SYSPLL_CNS0 &= ~DAIF_SYSPLL_PD_VBAT_MASK;
            DRV_DELAY_US(10);
            OM_DAIF->SYSPLL_CNS0 &= ~DAIF_SYSPLL_RSTN_DIG_MASK;
            OM_DAIF->SYSPLL_CNS0 |= DAIF_SYSPLL_RSTN_DIG_MASK;
            OM_DAIF->SYSPLL_CNS0 |= DAIF_SYSPLL_EN_AFC_MASK;
            DRV_DELAY_US(2);
            OM_DAIF->SYSPLL_CNS0 &= ~(DAIF_SYSPLL_EN_AFC_MASK | DAIF_SYSPLL_GT_USBCLK48M_MASK);
            while (!(OM_DAIF->SYSPLL_CNS1 & DAIF_SYSPLL_LOCK_MASK));
        }
    } else {
        uint32_t syspll_cns;

        syspll_cns = OM_DAIF->SYSPLL_CNS0 | DAIF_SYSPLL_GT_USBCLK48M_MASK;
        if (!(syspll_cns & DAIF_SYSPLL_PD_VBAT_MASK)) {
            syspll_cns |= DAIF_SYSPLL_PD_VBAT_MASK;
        }
        OM_DAIF->SYSPLL_CNS0 = syspll_cns;
    }

    OM_CRITICAL_END();
    DRV_RCC_ANA_CLK_RESTORE();
}

/**
 * @brief  pmu select 32k
 *
 * @param[in] clk32k  clk32k
 *
 * @note
 *   If selecting PMU_32K_SEL_32768HZ_XTAL clock and using BLE,
 *   the `obcc.lpclk_drift` in obc_cc.h should be changed to correct ppm (may be 50 ppm)
 *
 **/
void drv_pmu_select_32k(pmu_32k_sel_t clk32k)
{
    // Default: rc32k powered on, xtal32k powered down
    switch(clk32k) {
        case PMU_32K_SEL_RC:
            drv_pmu_select_32k(PMU_32K_SEL_DIV);
            drv_pmu_32k_switch_to_rc(true /*calib*/, true/*pd_others*/);
            break;

        case PMU_32K_SEL_32768HZ_XTAL:
            // Power on rc32k
            //REGW0(&OM_PMU->MISC_CTRL, PMU_MISC_REG_PD_RC32K_MASK);
            //while(!(OM_PMU->STATUS_READ & PMU_RC32K_READY_MASK));

            DRV_RCC_ANA_CLK_ENABLE_NOIRQ();
            // GPIO reuse (IO24/IO25 reuse as xtal32k input)
            REGW1(&OM_PMU->ANA_REG, PMU_ANA_REG_GPIO_REUSE_MASK);
            // Open xtal32m calibration clock for xtal32k ready check
            REGW1(&OM_DAIF->CLK_ENS, DAIF_XTAL32M_CTRL_CLK_EN_MASK|DAIF_C32K_CLK_EN_MASK);
            // Power on xtal32k
            REGW0(&OM_PMU->MISC_CTRL, PMU_MISC_CTRL_PD_CRY32K_MASK);
            while(!(OM_PMU->BASIC & PMU_BASIC_CRY32K_READY_MASK));
            // Close xtal32m calibration clock
            REGW0(&OM_DAIF->CLK_ENS, DAIF_XTAL32M_CTRL_CLK_EN_MASK|DAIF_C32K_CLK_EN_MASK);
            DRV_RCC_ANA_CLK_RESTORE_NOIRQ();

            // Switch
            REGW(&OM_PMU->MISC_CTRL, MASK_1REG(PMU_MISC_CTRL_CLK_32K_SEL, PMU_32K_SEL_32768HZ_XTAL));
            while((OM_PMU->STATUS_READ & (PMU_STATUS_READ_CLK_32K_RC_CRY_DONE_MASK|PMU_STATUS_READ_CLK_32K_DIV_DONE_MASK)) !=
                                         (PMU_STATUS_READ_CLK_32K_RC_CRY_DONE_MASK|PMU_STATUS_READ_CLK_32K_DIV_DONE_MASK));

            // power down others
            //REGW1(&OM_PMU->MISC_CTRL, PMU_MISC_REG_PD_RC32K_MASK);

            // Keep on
            REGW0(&OM_PMU->MISC_CTRL_1, PMU_MISC_CTRL_1_CRY32M_KEEP_ON_MASK);
            break;

        case PMU_32K_SEL_DIV:
            // Power on rc32k
            //REGW0(&OM_PMU->MISC_CTRL, PMU_MISC_REG_PD_RC32K_MASK);
            //while(!(OM_PMU->STATUS_READ & PMU_RC32K_READY_MASK));

            // Open clock
            REGW1(&OM_SYS->RST_32K_OSC_CTRL, SYS_CRY32M_DIV_EN_MASK);

            // Switch
            REGW(&OM_PMU->MISC_CTRL, MASK_1REG(PMU_MISC_CTRL_CLK_32K_SEL, PMU_32K_SEL_DIV));
            while((OM_PMU->STATUS_READ & (PMU_STATUS_READ_CLK_32K_RC_CRY_DONE_MASK|PMU_STATUS_READ_CLK_32K_DIV_DONE_MASK)) !=
                                         (PMU_STATUS_READ_CLK_32K_RC_CRY_DONE_MASK|PMU_STATUS_READ_CLK_32K_DIV_DONE_MASK));

            // power down others
            //REGW1(&OM_PMU->MISC_CTRL, PMU_MISC_REG_PD_CRY32K_MASK | PMU_MISC_REG_PD_RC32K_MASK);
            break;
    }

    drv_pmu_env.clock_32k = clk32k;
}

/**
 * @brief pmu get 32k select
 *
 * @return 32k select
 **/
__RAM_CODES("PM")
pmu_32k_sel_t drv_pmu_select_32k_get(void)
{
    return drv_pmu_env.clock_32k;
}

/**
 * @brief what power status should be entried
 *
 * @return power status
 **/
__RAM_CODES("PM")
bool drv_pmu_deep_sleep_is_allow(void)
{
    uint32_t pin_wakeup;
    volatile uint32_t *const gpio_pol[] = {
        &(OM_PMU->GPIO_POL),
        &(OM_PMU->GPIO_POL_1),
    };

    for (uint32_t i=0; i<sizeof(gpio_pol)/sizeof(gpio_pol[0]); i++) {
        pin_wakeup = OM_PMU->GPIO_WAKEUP[i];
        if (pin_wakeup) {
            OM_GPIO_Type *gpio_base;
            gpio_base = drv_gpio_idx2base(i);
            if (gpio_base != NULL) {
                uint32_t pin_wakeup_cur_level = drv_gpio_read(gpio_base, pin_wakeup);
                if (drv_pmu_env.pin_wakeup_double_edge_mask[i]) {
                    uint32_t pin_wakeup_cur_level_double_edge = pin_wakeup_cur_level & drv_pmu_env.pin_wakeup_double_edge_mask[i];
                    REGW(gpio_pol[i], drv_pmu_env.pin_wakeup_double_edge_mask[i], pin_wakeup_cur_level_double_edge);
                }

                if (drv_pmu_env.pin_wakeup_none_edge_mask[i]) {
                    uint32_t pin_wakeup_cur_level_none_edge = pin_wakeup_cur_level & drv_pmu_env.pin_wakeup_none_edge_mask[i];
                    uint32_t pin_wakeup_sleep_level_none_edge = *(gpio_pol[i]);
                    pin_wakeup_sleep_level_none_edge &= drv_pmu_env.pin_wakeup_none_edge_mask[i];
                    if (pin_wakeup_cur_level_none_edge != pin_wakeup_sleep_level_none_edge) {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

/**
 *******************************************************************************
 * @brief  pmu sleep enter
 *
 * @param[in] is_deep_sleep  sleep status
 * @param[in] reboot_req  reboot req
 *******************************************************************************
 */
__RAM_CODES("PM")
void drv_pmu_sleep_enter(uint8_t is_deep_sleep, bool reboot_req)
{
    // pof disable
    if (drv_pmu_env.enable_pof) {
        REGW(&OM_PMU->POF_INT_CTRL, MASK_1REG(PMU_POF_INT_CTRL_PMU_POF_INT_EN, 0));
    }

    do {
        if (is_deep_sleep && (!drv_pmu_env.enable_32k_with_deep_sleep)) {
            if (drv_pmu_env.clock_32k == PMU_32K_SEL_32768HZ_XTAL) {
                // Power on 32k rc and switch to it
                drv_pmu_32k_switch_to_rc(false /*calib*/, true /*pd_others*/);
            }
            // Set a flag to power down 32K (PMU_BASIC_WAKEUPB_DIS=0: when close 32k, PIN can wakeup)
            drv_pmu_basic_reg_set(MASK_2REG(PMU_BASIC_SLEEP_WO_32K, 1, PMU_BASIC_WAKEUPB_DIS, 0));
            break;
        }
        drv_pmu_basic_reg_set(MASK_2REG(PMU_BASIC_SLEEP_WO_32K, 0, PMU_BASIC_WAKEUPB_DIS, 1));
    } while(0);

    // Switch to xtal32m when using syspll
    if ((SystemCoreClock == 96 * 1000 * 1000U) || (SystemCoreClock == 48 * 1000 * 1000U)) {
        drv_rcc_cpu_clk_source_set(RCC_CPU_CLK_SOURCE_XTAL32M);
    }
    if (drv_rcc_clock_get(RCC_CLK_PERI) != 32 * 1000 * 1000U) {
        drv_rcc_periph_clk_source_set(RCC_PERIPH_CLK_SOURCE_XTAL32M);
    }

    // clear pmu gpio interrupt
    #if (PMU_IO_OUTPUT_LATCH_CTRL_BY_SOFTWARE)
    // latch io: disable HW auto latch, enable IO latch by SW
    OM_PMU->MISC_CTRL |= (PMU_MISC_CTRL_CLR_PMU_INT_MASK | PMU_MISC_CTRL_GPIO_AUTO_LATCH_FSM_DIS_MASK | PMU_MISC_CTRL_GPIO_AUTO_LATCH_CTRL_MASK);
    #else
    OM_PMU->MISC_CTRL |= PMU_MISC_CTRL_CLR_PMU_INT_MASK;
    #endif
    while(OM_PMU->MISC_CTRL & PMU_MISC_CTRL_CLR_PMU_INT_MASK);
    while(OM_PMU->STATUS_READ & PMU_STATUS_READ_CLR_PMU_INT_SYNC_APB_MASK);

    // 1M clock for digital
    REGW(&OM_PMU->ANA_PD, MASK_1REG(PMU_ANA_PD_REG_PD_DCDC_OSC, 1));

    // Into reboot sleep mode
    if (reboot_req) {
        drv_pmu_force_into_reboot_sleep_mode();
    }

    register_set1(&OM_PMU->GPIO_PU_EN_1, PMU_GPIO_PU_CTRL_1_GPIO_PL_CTRL_SF_SI_MASK
                                       | PMU_GPIO_PU_CTRL_1_GPIO_PL_CTRL_SF_SO_MASK
                                       | PMU_GPIO_PU_CTRL_1_GPIO_PL_CTRL_SF_HOLD_MASK
                                       | PMU_GPIO_PU_CTRL_1_GPIO_PL_CTRL_SF_WP_MASK);
}

/**
 * @brief pmu leave lowpower status, call by system
 *
 * @param[in] step  step
 * @param[in] state  lowpower state
 *
 * @return None
 **/
__RAM_CODES("PM")
void drv_pmu_sleep_leave(pmu_sleep_leave_step_t step)
{
    if (step & PMU_SLEEP_LEAVE_STEP1_ON_RC32M) {
        // Open daif clock
        DRV_RCC_CLOCK_ENABLE(RCC_CLK_DAIF, 1U);

        drv_pmu_env.ana_mask = 0;
        drv_pmu_env.pin_wakeup_sleep_recently = true;

        register_set0(&OM_PMU->GPIO_PU_EN_1, PMU_GPIO_PU_CTRL_1_GPIO_PL_CTRL_SF_SI_MASK
                                           | PMU_GPIO_PU_CTRL_1_GPIO_PL_CTRL_SF_SO_MASK
                                           | PMU_GPIO_PU_CTRL_1_GPIO_PL_CTRL_SF_HOLD_MASK
                                           | PMU_GPIO_PU_CTRL_1_GPIO_PL_CTRL_SF_WP_MASK);
    }

    if (step & PMU_SLEEP_LEAVE_STEP2_WAIT_XTAL32M) {
        // Wait xtal32m ready
        while(!(OM_DAIF->XTAL32M_INTRS & DAIF_XTAL32M_CLK_RDY_MASK));
        // Wait switch to xtal32m OK
        __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();

        // default rc32m is opened, close it
        drv_pmu_rc32m_enable(false);
    }

    if (step & PMU_SLEEP_LEAVE_STEP3_FINISH) {
        #if (PMU_IO_OUTPUT_LATCH_CTRL_BY_SOFTWARE)
        // release IO latch by SW
        OM_PMU->MISC_CTRL &= ~PMU_MISC_CTRL_GPIO_AUTO_LATCH_CTRL_MASK;
        #endif
        // re-enable xtal32k
        if(!drv_pmu_env.enable_32k_with_deep_sleep) {
            if (drv_pmu_env.clock_32k == PMU_32K_SEL_32768HZ_XTAL) {
                drv_pmu_select_32k(drv_pmu_env.clock_32k);
            }
        }

        // disable all daif clock
        OM_DAIF->CLK_ENS = 0;

        // close daif clock
        DRV_RCC_CLOCK_ENABLE(RCC_CLK_DAIF, 0U);

        // pof enable
        if (drv_pmu_env.enable_pof) {
            REGW(&OM_PMU->POF_INT_CTRL, MASK_1REG(PMU_POF_INT_CTRL_PMU_POF_INT_EN, 1));
        }
    }
}

void drv_pmu_32k_enable_in_deep_sleep(bool enable)
{
    drv_pmu_env.enable_32k_with_deep_sleep = enable ? 1 : 0;
}

void drv_pmu_pin_input_enable(uint8_t pin_idx, uint8_t ie)
{
    uint32_t pin_mask;
    pin_mask = 1U << (pin_idx % 32);

    if (pin_idx < 32) {
        register_set(&OM_PMU->GPIO_IE_CTRL, pin_mask, ie ? pin_mask : 0U);
    } else {
        register_set(&OM_PMU->GPIO_IE_CTRL_1, pin_mask, ie ? pin_mask : 0U);
    }
}

/**
 * @brief Set pin mode, and setting IE enable
 *
 * @param[in] pin_idx  pin index
 * @param[in] mode  pin mode
 *
 * @return None
 **/
void drv_pmu_pin_mode_set(uint8_t pin_idx, pmu_pin_mode_t mode)
{
    uint32_t pin_mask;
    volatile uint32_t *ode_ctrl, *pu_en, *pd_ctrl;

    pin_mask = 1U << (pin_idx % 32);
    if (pin_idx < 32) {
        ode_ctrl = &(OM_PMU->GPIO_ODE_CTRL);
        pu_en    = &(OM_PMU->GPIO_PU_EN);
        pd_ctrl  = &(OM_PMU->GPIO_PD_CTRL);
    } else {
        ode_ctrl = &(OM_PMU->GPIO_ODE_CTRL_1);
        pu_en    = &(OM_PMU->GPIO_PU_EN_1);
        pd_ctrl  = &(OM_PMU->GPIO_PD_CTRL_1);
    }

    switch(mode) {
        case PMU_PIN_MODE_FLOAT:
        case PMU_PIN_MODE_PP:
            register_set0(ode_ctrl, pin_mask);
            register_set0(pu_en,    pin_mask);
            register_set0(pd_ctrl,  pin_mask);
            break;
        case PMU_PIN_MODE_PD:
            register_set0(ode_ctrl, pin_mask);
            register_set0(pu_en,    pin_mask);
            register_set1(pd_ctrl,  pin_mask);
            break;
        case PMU_PIN_MODE_PU:
            register_set0(ode_ctrl, pin_mask);
            register_set1(pu_en,    pin_mask);
            register_set0(pd_ctrl,  pin_mask);
            break;
        case PMU_PIN_MODE_OD:
            register_set1(ode_ctrl, pin_mask);
            register_set0(pu_en,    pin_mask);
            register_set0(pd_ctrl,  pin_mask);
            break;
        case PMU_PIN_MODE_OD_PU:
            register_set1(ode_ctrl, pin_mask);
            register_set1(pu_en,    pin_mask);
            register_set0(pd_ctrl,  pin_mask);
            break;
        default:
            break;
    }
}

/**
 * @brief Set gpio driven current
 *
 * @param[in] pin_idx  pin idx
 * @param[in] driven  current driven (Large driven current should be push-pull output)
 *
 * @return None
 **/
void drv_pmu_pin_driven_current_set(uint8_t pin_idx, pmu_pin_driver_current_t driven)
{
    uint32_t pin_group, pin_mask;

    pin_group = (pin_idx < 32U) ? 0U : 1U;
    pin_mask = 1U << (pin_idx % 32);

    if (driven != PMU_PIN_DRIVER_CURRENT_MAX) {
        OM_PMU->GPIO_DRV_CTRL[pin_group] &= ~pin_mask;
    } else {
        OM_PMU->GPIO_DRV_CTRL[pin_group] |= pin_mask;
    }
}

void drv_pmu_pin_pullup_set(uint8_t pin_idx, pmu_pin_pullup_t pullup)
{
    uint32_t pin_mask;
    volatile uint32_t *pl_ctrl_bit0, *pl_ctrl_bit1;

    // the OM_PMU->GPIO_PL_CTRL_11 read error.
    static uint32_t gpio_pl_ctrl_11_value  = 0;

    pin_mask = 1U << (pin_idx % 32);
    if (pin_idx < 32) {
        pl_ctrl_bit0 = &(OM_PMU->GPIO_PL_CTRL_0);
        pl_ctrl_bit1 = &(OM_PMU->GPIO_PL_CTRL_1);
    } else {
        pl_ctrl_bit0 = &(OM_PMU->GPIO_DRV_CTRL_3);
        pl_ctrl_bit1 = &(OM_PMU->GPIO_PL_CTRL_11);

        if (pullup & (1U << 1)) {
            gpio_pl_ctrl_11_value |= pin_mask;
        }
        else {
            gpio_pl_ctrl_11_value &= ~(pin_mask);
        }
    }

    register_set(pl_ctrl_bit0, pin_mask, (pullup & (1U << 0)) ? pin_mask : 0U);
    if (pin_idx < 32) {
        register_set(pl_ctrl_bit1, pin_mask, (pullup & (1U << 1)) ? pin_mask : 0U);
    }
    else {
        register_set(pl_ctrl_bit1, 0x7F, gpio_pl_ctrl_11_value);
    }
}

/**
 * @brief pmu analog power enable, call by system
 *
 * @param[in] enable  enable/disable
 * @param[in] ana  analog type
 *
 * @return None
 **/
__RAM_CODES("PM")
void drv_pmu_ana_enable(bool enable, pmu_ana_type_t ana)
{
    OM_CRITICAL_BEGIN();

    if(enable) {
        if ((drv_pmu_env.ana_mask & ana) == 0) {
            if(drv_pmu_env.ana_mask == 0) {
                DRV_RCC_CLOCK_ENABLE(RCC_CLK_DAIF, 1U);
            }

            switch(ana) {
                case PMU_ANA_RF:
                case PMU_ANA_RF_24G:
                case PMU_ANA_RF_BLE:
                    if ((drv_pmu_env.ana_mask & (PMU_ANA_RF|PMU_ANA_RF_24G|PMU_ANA_RF_BLE)) == 0) {
                        /*
                         * PHY
                         */
                        DRV_RCC_CLOCK_ENABLE(RCC_CLK_PHY, 1U);
                        #if 0
                        // 优化阻塞信道灵敏度策略
                        REGWA(&OM_PHY->TONE_SUPPRESSION_CTRL, MASK_7REG(PHY_TONE_SUPPRESSION_CTRL_EN_TS,2,
                                                                        PHY_TONE_SUPPRESSION_CTRL_TS_EN_GAIN_COND,0,
                                                                        PHY_TONE_SUPPRESSION_CTRL_TS_MAG_LIM,3,
                                                                        PHY_TONE_SUPPRESSION_CTRL_TS_MODE,1,
                                                                        PHY_TONE_SUPPRESSION_CTRL_TS_MAG_EN,1,
                                                                        PHY_TONE_SUPPRESSION_CTRL_TS_K,1,
                                                                        PHY_TONE_SUPPRESSION_CTRL_TS_EST_DUR,1));
                        #endif

                        /*
                         * ANA power/clock
                         */
                        REGW1(&OM_DAIF->CLK_ENS, DAIF_PLL_VTRACK_CLK_EN_MASK | DAIF_PLL_LUT_CLK_EN_MASK |
                                DAIF_MAIN_FSM_CLK_EN_MASK | DAIF_RX_AGC_CLK_EN_MASK | DAIF_DCOC_LUT_CLK_EN_MASK |
                                DAIF_SDM_CLK_EN_MASK | DAIF_PLL_CLK_REF_EN_MASK | DAIF_PLL_AFC_CLK_EN_MASK);
                        REGW1(&OM_DAIF->CLK_CFG, DAIF_XTAL32M_EN_CKO16M_DIG_MASK|DAIF_XTAL32M_EN_CKO16M_ANA_MASK|DAIF_XTAL32M_EN_CKO16M_PLL_MASK);
                    }
                    break;

                case PMU_ANA_GPADC:
                    REGW1(&OM_DAIF->CLK_ENS, DAIF_GPADC_CLK_EN_MASK);
                    REGW1(&OM_DAIF->CLK_CFG, DAIF_XTAL32M_EN_CKO16M_GPADC_MASK);
                    break;

                case PMU_ANA_CALIB_RC32K:
                    REGW1(&OM_DAIF->CLK_ENS, DAIF_RC_32K_TUNE_CLK_EN_MASK);
                    break;
                case PMU_ANA_AUDIO:
                case PMU_ANA_I2S:
                    // Close Audio CLOCK 16M
                    REGW1(&OM_DAIF->CLK_CFG, DAIF_XTAL32M_EN_CKO16M_AUDIO_MASK);
                    break;

                default:
                    break;
            }

            drv_pmu_env.ana_mask |= ana;
        }
    } else {
        if (drv_pmu_env.ana_mask & ana) {
            drv_pmu_env.ana_mask &= ~ana;

            switch(ana) {
                case PMU_ANA_RF:
                case PMU_ANA_RF_24G:
                case PMU_ANA_RF_BLE:
                    if ((drv_pmu_env.ana_mask & (PMU_ANA_RF | PMU_ANA_RF_24G | PMU_ANA_RF_BLE)) == 0) {
                        /*
                         * ANA power/clock
                         */
                        // Wait for the digital state machine to finish running before turning off the clock.
                        DRV_DAIF_WAIT_IDLE();

                        // close clock
                        DRV_RCC_CLOCK_ENABLE(RCC_CLK_PHY, 0U);
                        REGW0(&OM_DAIF->CLK_ENS, DAIF_PLL_VTRACK_CLK_EN_MASK | DAIF_PLL_LUT_CLK_EN_MASK |
                                DAIF_MAIN_FSM_CLK_EN_MASK | DAIF_RX_AGC_CLK_EN_MASK | DAIF_DCOC_LUT_CLK_EN_MASK |
                                DAIF_SDM_CLK_EN_MASK | DAIF_PLL_CLK_REF_EN_MASK | DAIF_PLL_AFC_CLK_EN_MASK);
                        REGW0(&OM_DAIF->CLK_CFG, /*DAIF_XTAL32M_EN_CKO16M_DIG_MASK|*/DAIF_XTAL32M_EN_CKO16M_ANA_MASK|DAIF_XTAL32M_EN_CKO16M_PLL_MASK);
                    }
                    break;

                case PMU_ANA_GPADC:
                    REGW0(&OM_DAIF->CLK_ENS, DAIF_GPADC_CLK_EN_MASK);
                    REGW0(&OM_DAIF->CLK_CFG, DAIF_XTAL32M_EN_CKO16M_GPADC_MASK);
                    break;

                case PMU_ANA_CALIB_RC32K:
                    REGW0(&OM_DAIF->CLK_ENS, DAIF_RC_32K_TUNE_CLK_EN_MASK);
                    break;
                case PMU_ANA_AUDIO:
                case PMU_ANA_I2S:
                    // Close Audio CLOCK 16M
                    REGW0(&OM_DAIF->CLK_CFG, DAIF_XTAL32M_EN_CKO16M_AUDIO_MASK);
                    break;

                default:
                    break;
            }

            if(drv_pmu_env.ana_mask == 0) {
                DRV_RCC_CLOCK_ENABLE(RCC_CLK_DAIF, 0U);
            }
        }
    }

    OM_CRITICAL_END();
}

/**
 * @brief analog is enabled
 *
 * @param[in] ana  analog module
 *
 * @return enabled?
 **/
bool drv_pmu_ana_is_enabled(pmu_ana_type_t ana)
{
    return (drv_pmu_env.ana_mask & ana) ? true : false;
}

/**
 *******************************************************************************
 * @brief  drv pmu pin wakeup out of date
 *
 * @note:
 * PIN_WAKEUP_IRQ will be generated not only from pin-wakeup but also each GPIO irq(not sleep)
 * So do this to let PIN_WAKEUP_IRQ only generate from pin-wakeup
 *
 * @note: PMU_PIN_WAKEUP_IRQn must less then GPIO0_IRQn/GPIO1_IRQn
 *******************************************************************************
 */
void drv_pmu_pin_wakeup_out_of_date(void)
{
    drv_pmu_env.pin_wakeup_sleep_recently = false;
}

/**
 * @brief pmu enable INSIDE dcdc
 *
 * @param[in] enable  enable/disable
 *
 * @return None
 **/
void drv_pmu_dcdc_enable(bool enable)
{
    if(enable) {
        if (pmu_dcdc_info.dcdc_vout_is_set) {
            REGW(&OM_PMU->ANA_PD_1, MASK_1REG_SSAT(PMU_ANA_PD_1_DCDC_VOUT, pmu_dcdc_info.dcdc_vout));
        } else {
            REGW(&OM_PMU->ANA_PD_1, MASK_1REG_SSAT(PMU_ANA_PD_1_DCDC_VOUT, drv_calib_repair_env.dcdc_vout));
        }
        DRV_DELAY_US(100);
        REGW(&OM_PMU->ANA_PD, MASK_1REG(PMU_ANA_PD_DCDC_DIS, 0));
        DRV_DELAY_US(10 *20);
        REGW(&OM_PMU->ANA_PD, MASK_1REG(PMU_ANA_PD_ANA_1P2LDO_DIS, 1));
        REGW1(&OM_PMU->SW_STATUS, PMU_SW_STATUS_DCDC_ENABLED_MASK);
    } else {
        REGW(&OM_PMU->ANA_PD, MASK_1REG(PMU_ANA_PD_ANA_1P2LDO_DIS, 0));
        DRV_DELAY_US(10 *20);
        REGW(&OM_PMU->ANA_PD, MASK_1REG(PMU_ANA_PD_DCDC_DIS, 0));
        REGW(&OM_PMU->ANA_PD_1, MASK_1REG(PMU_ANA_PD_1_DCDC_VOUT, 0));
        REGW0(&OM_PMU->SW_STATUS, PMU_SW_STATUS_DCDC_ENABLED_MASK);
    }
}

void drv_pmu_ram_power_down(const pmu_ram_block_t blocks)
{
    uint32_t power_status;

    REGW(&OM_PMU->PSO_PM, MASK_5REG(PMU_PSO_RAM1_2_POWER_ON, ((blocks.pmu_ram_block_sram1 && blocks.pmu_ram_block_sram2) ? 0 : 1),
                                    PMU_PSO_RAM3_POWER_ON,   (blocks.pmu_ram_block_sram3 ? 0 : 1),
                                    #if (OM_MEM_RAM_SIZE == 256*1024U)
                                    PMU_PSO_RAM4_POWER_ON,   (blocks.pmu_ram_block_sram4 ? 0 : 1),
                                    PMU_PSO_RAM5_POWER_ON,   (blocks.pmu_ram_block_sram5 ? 0 : 1),
                                    #else
                                    PMU_PSO_RAM4_POWER_ON,   0,
                                    PMU_PSO_RAM5_POWER_ON,   0,
                                    #endif
                                    PMU_PSO_ICACHE_POWER_ON, (blocks.pmu_ram_block_icache ? 0 : 1)));
    power_status =  ((blocks.pmu_ram_block_sram1 && blocks.pmu_ram_block_sram2) ? 0U : PMU_PSO_RAM1_2_POWER_STATUS_MASK)
                  | (blocks.pmu_ram_block_sram3 ? 0U : PMU_PSO_RAM3_POWER_STATUS_MASK)
                  #if (OM_MEM_RAM_SIZE == 256*1024U)
                  | (blocks.pmu_ram_block_sram4 ? 0U : PMU_PSO_RAM4_POWER_STATUS_MASK)
                  | (blocks.pmu_ram_block_sram5 ? 0U : PMU_PSO_RAM5_POWER_STATUS_MASK)
                  #endif
                  | (blocks.pmu_ram_block_icache ? 0U: PMU_PSO_ICACHE_POWER_STATUS_MASK);
    while ((OM_PMU->PSO_PM & power_status) != power_status);
}

void drv_pmu_ram_power_down_in_sleep(pmu_ram_block_t blocks)
{
    uint32_t ram_ctrl;

    ram_ctrl = OM_PMU->RAM_CTRL_2 & (~(0xFFFU << 18));
    OM_PMU->RAM_CTRL_2 = ram_ctrl
                        | (blocks.pmu_ram_block_sram0 ? PMU_RAM_CTRL_2_RAM0_SD_HW_CTRL_EN_MASK : PMU_RAM_CTRL_2_RAM0_DS_HW_CTRL_EN_MASK)
                        | (blocks.pmu_ram_block_sram1 ? PMU_RAM_CTRL_2_RAM1_SD_HW_CTRL_EN_MASK : PMU_RAM_CTRL_2_RAM1_DS_HW_CTRL_EN_MASK)
                        | (blocks.pmu_ram_block_sram2 ? PMU_RAM_CTRL_2_RAM2_SD_HW_CTRL_EN_MASK : PMU_RAM_CTRL_2_RAM2_DS_HW_CTRL_EN_MASK)
                        | (blocks.pmu_ram_block_pso ? PMU_RAM_CTRL_2_RAM_PSO_SD_HW_CTRL_EN_MASK : PMU_RAM_CTRL_2_RAM_PSO_DS_HW_CTRL_EN_MASK)
                        | (blocks.pmu_ram_block_ble ? PMU_RAM_CTRL_2_RAM_BLE_SD_HW_CTRL_EN_MASK : PMU_RAM_CTRL_2_RAM_BLE_DS_HW_CTRL_EN_MASK)
                        | (blocks.pmu_ram_block_icache ? PMU_RAM_CTRL_2_RAM_IC_SD_HW_CTRL_EN_MASK : PMU_RAM_CTRL_2_RAM_IC_DS_HW_CTRL_EN_MASK);

    ram_ctrl = OM_PMU->RAM_CTRL_1 & (~((3U << 30) | (3U << 26) | (3U << 22)));
    OM_PMU->RAM_CTRL_1 = ram_ctrl
                         #if (OM_MEM_RAM_SIZE == 256*1024U)
                         | (blocks.pmu_ram_block_sram4 ? PMU_RAM_CTRL_1_RAM4_SD_HW_CTRL_EN_MASK : PMU_RAM_CTRL_1_RAM4_DS_HW_CTRL_EN_MASK)
                         | (blocks.pmu_ram_block_sram5 ? PMU_RAM_CTRL_1_RAM5_SD_HW_CTRL_EN_MASK : PMU_RAM_CTRL_1_RAM5_DS_HW_CTRL_EN_MASK)
                         #endif
                         | (blocks.pmu_ram_block_sram3 ? PMU_RAM_CTRL_1_RAM3_SD_HW_CTRL_EN_MASK : PMU_RAM_CTRL_1_RAM3_DS_HW_CTRL_EN_MASK);
}

/**
 * @brief pmu gpio wakeup pin setup
 *
 * @param[in] pin_idx  pin index
 * @param[in] trigger_type  wakeup trigger type
 *
 * @return None
 **/
void drv_pmu_wakeup_pin_set(uint8_t pin_idx, pmu_pin_wakeup_type_t trigger_type)
{
    uint32_t pin_mask;
    uint32_t pin_group;
    volatile uint32_t *gpio_pol;

    pin_mask = 1U << (pin_idx % 32);
    pin_group = (pin_idx < 32U) ? 0U : 1U;
    if (pin_idx < 32) {
        gpio_pol = &(OM_PMU->GPIO_POL);
    } else {
        gpio_pol = &(OM_PMU->GPIO_POL_1);
    }

    OM_CRITICAL_BEGIN();
    switch(trigger_type) {
        case PMU_PIN_WAKEUP_DISABLE:
            OM_PMU->GPIO_WAKEUP[pin_group] &= ~pin_mask;
            REGW0(gpio_pol, pin_mask);
            break;
        case PMU_PIN_WAKEUP_FALLING_EDGE:
        case PMU_PIN_WAKEUP_LOW_LEVEL: // FALLING_EDGE
            OM_PMU->GPIO_WAKEUP[pin_group] |= pin_mask;
            REGW1(gpio_pol, pin_mask);
            break;
        case PMU_PIN_WAKEUP_RISING_FAILING_EDGE:
        case PMU_PIN_WAKEUP_RISING_EDGE:
        case PMU_PIN_WAKEUP_HIGH_LEVEL: // RISING_EDGE
            OM_PMU->GPIO_WAKEUP[pin_group] |= pin_mask;
            REGW0(gpio_pol, pin_mask);
            break;
    }

    switch (trigger_type) {
        case PMU_PIN_WAKEUP_LOW_LEVEL:
        case PMU_PIN_WAKEUP_HIGH_LEVEL:
            drv_pmu_env.pin_wakeup_none_edge_mask[pin_group] |= pin_mask;
            drv_pmu_env.pin_wakeup_double_edge_mask[pin_group] &= ~pin_mask;
            break;
        case PMU_PIN_WAKEUP_RISING_FAILING_EDGE:
            drv_pmu_env.pin_wakeup_none_edge_mask[pin_group] &= ~pin_mask;
            drv_pmu_env.pin_wakeup_double_edge_mask[pin_group] |= pin_mask;
            break;
        default:
            break;
    }

    REGW(&OM_PMU->WAKE_DEB, PMU_WAKE_DEB_PIN_WAKE_LEVEL_EDGE_SEL_MASK | PMU_WAKE_DEB_PIN_DEBOUNCE_CYCLE_WAKE_MASK |
                            PMU_WAKE_DEB_PIN_DEBOUNCE_COEFF_WAKE_MASK | PMU_WAKE_DEB_PIN_DEB_RST_MASK,
                            (1<<PMU_WAKE_DEB_PIN_WAKE_LEVEL_EDGE_SEL_POS) | PMU_WAKE_DEB_PIN_DEB_RST_MASK);

    OM_CRITICAL_END();

    if (trigger_type == PMU_PIN_WAKEUP_DISABLE) {
        if (!(OM_PMU->GPIO_WAKEUP[0] || OM_PMU->GPIO_WAKEUP[1])) {
            NVIC_DisableIRQ(PMU_PIN_WAKEUP_IRQn);
            NVIC_ClearPendingIRQ(PMU_PIN_WAKEUP_IRQn);
        }
    } else {
        if (!NVIC_GetEnableIRQ(PMU_PIN_WAKEUP_IRQn)) {
            NVIC_ClearPendingIRQ(PMU_PIN_WAKEUP_IRQn);
            NVIC_SetPriority(PMU_PIN_WAKEUP_IRQn, RTE_PMU_PIN_WAKEUP_IRQ_PRIORITY);
            NVIC_EnableIRQ(PMU_PIN_WAKEUP_IRQn);
        }
    }
}

#if (RTE_PMU_PIN_WAKEUP_REGISTER_CALLBACK)
void drv_pmu_wakeup_pin_register_callback(drv_isr_callback_t isr_callback)
{
    drv_pmu_env.pin_wakeup_isr_callback = isr_callback;
}
#endif

__WEAK void drv_pmu_pin_wakeup_isr_callback(OM_PMU_Type *om_pmu, drv_event_t event, uint32_t int_status)
{
    #if (RTE_PMU_PIN_WAKEUP_REGISTER_CALLBACK)
    if(drv_pmu_env.pin_wakeup_isr_callback) {
        drv_pmu_env.pin_wakeup_isr_callback(om_pmu, event, (void *)int_status, NULL);
    }
    #endif
}

/**
 * @brief Force system to reboot
 *
 * @return None
 **/
void drv_pmu_reset(pmu_reboot_reason_t reason)
{
    OM_ASSERT(reason >= PMU_REBOOT_FROM_WDT);
    OM_ASSERT(reason <= (PMU_RSVD_SW_REG_REBOOT_REASON_MASK >> PMU_RSVD_SW_REG_REBOOT_REASON_POS));
    // Disable ALL IRQ, MUST use __set_PRIMASK(1)
    __disable_irq();
    // store RTC second count register
    #if (RTE_RTC)
    if (OM_RTC->CR & RTC_EN_SYNC_MASK) {    // RTC is enabled
        OM_PMU->RSVD_SW_REG[1] = drv_rtc_timer_get(NULL);
        OM_PMU->RSVD_SW_REG[0] |= PMU_RSVD_SW_REG_RTC_SECOND_RESTORE_PROCESS_MASK;
    }
    #endif
    // set reboot reason
    register_set(&OM_PMU->RSVD_SW_REG[0], PMU_RSVD_SW_REG_REBOOT_REASON_MASK, reason);
    // Remap and Reset
    pmu_cpu_reset();
}

/**
 * @brief Get charge status
 *
 * @return status
 **/
pmu_charge_status_t drv_pmu_charge_status(void)
{
    volatile uint32_t charge_status = OM_SYS->CHRGR_STAT;

    if(charge_status & SYS_CHRGR_INSERT_DETECT_MASK) {
        return (charge_status & SYS_CHRGR_FINISH_MASK) ? PMU_CHARGE_COMPLETE : PMU_CHARGE_CHARGING;
    } else {
        return PMU_CHARGE_EXTRACT;
    }
}

/**
 * @brief  pmu recalib sysclk
 **/
void drv_pmu_topclk_recalib(void)
{
    DRV_RCC_ANA_CLK_ENABLE();
    drv_pmu_xtal32m_fast_startup(false);
    drv_rcc_cpu_clk_source_set(RCC_CPU_CLK_SOURCE_RC32M);
    // calib RC
    drv_calib_sys_rc();
    // to xtal32m
    drv_rcc_cpu_clk_source_set(RCC_CPU_CLK_SOURCE_XTAL32M);
    // calib RC32M
    drv_calib_sys_rc32m();

    if (drv_pmu_clk64m_is_enabled()) {
        drv_rcc_cpu_clk_source_set(RCC_CPU_CLK_SOURCE_XTAL64M);
    }
    // power off rc32m
    drv_pmu_rc32m_enable(false);
    DRV_RCC_ANA_CLK_RESTORE();
}

/**
 *******************************************************************************
 * @brief  register step set, @ref drv_pmu_register_step_set()
 *
 * @param[in] reg  reg
 * @param[in] mask  mask
 * @param[in] pos  pos
 * @param[in] value  value
 * @param[in] should_update  should update
 * @param[in] delay_us  delay us
 *******************************************************************************
 */
__RAM_CODE void drv_pmu_register_step_set(volatile uint32_t *reg, uint32_t mask, uint32_t pos, uint32_t value, bool should_update, uint32_t delay_us)
{
    uint32_t cur = REGR(reg, mask, pos);
    int8_t variable;

    variable = (cur > value) ? -1 : 1;
    while(cur != value) {
        cur += variable;
        REGW(reg, mask, cur<<pos);
        if(should_update) {
            OM_PMU->ANA_REG |= PMU_ANA_REG_DIG_LDO_UPDATE_MASK;
            while(OM_PMU->ANA_REG & PMU_ANA_REG_DIG_LDO_UPDATE_MASK);
        }
        DRV_DELAY_US(delay_us);
    }
}

/**
 *******************************************************************************
 * @brief  drv pmu pin wakeup isr
 *******************************************************************************
 */
void drv_pmu_pin_wakeup_isr(void)
{
    uint32_t int_status;
    const drv_event_t pin_wakeup_event[] = {
        DRV_EVENT_PMU_PIN_WAKEUP_GPIO0,
        DRV_EVENT_PMU_PIN_WAKEUP_GPIO1,
    };

    OM_PMU->MISC_CTRL |= PMU_MISC_CTRL_CLR_PMU_INT_MASK;  // clear interrupt
    // Sleep wakeup: may 2ms delay
    if(drv_pmu_env.pin_wakeup_sleep_recently) {
        for (uint32_t i=0; i<sizeof(pin_wakeup_event)/sizeof(pin_wakeup_event[0]); i++) {
            uint32_t gpio_latch = OM_PMU->GPIO_LATCH[i];
            int_status = gpio_latch & OM_PMU->GPIO_WAKEUP[i];
            if (int_status) {
                #if (RTE_GPIO0 || RTE_GPIO1)
                do {
                    OM_GPIO_Type *om_gpio;
                    om_gpio = drv_gpio_idx2base(i);
                    if (om_gpio) {
                        drv_gpio_control(om_gpio, GPIO_CONTROL_CLEAR_INT, (void *)int_status);
                    }
                } while (0);
                #endif
                drv_pmu_pin_wakeup_isr_callback(OM_PMU, pin_wakeup_event[i], int_status);
            }
        }
        // wakeup out of date
        drv_pmu_pin_wakeup_out_of_date();
    }
}

void drv_pmu_pof_enable(bool enable, pmu_pof_voltage_t voltage, pmu_pof_int_edge_t mode)
{
    OM_CRITICAL_BEGIN();
    if(enable) {
        REGW(&OM_PMU->POF_INT_CTRL, MASK_3REG(PMU_POF_INT_CTRL_PMU_POF_INT_EN, 0,
                                              PMU_POF_INT_CTRL_PMU_POF_TH_REG, voltage,
                                              PMU_POF_INT_CTRL_PMU_POF_INT_MODE, mode));
        // wait for 3 * 32K clock at least for PMU_POF_FLAG_DET
        DRV_DELAY_US(150);
        if (((OM_PMU->POF_INT_CTRL & PMU_POF_INT_CTRL_PMU_POF_FLAG_DET_MASK) == 0) && (drv_pmu_env.enable_pof == 0)) {
            drv_pmu_env.enable_pof = 1;
            drv_pmu_pof_isr_callback(OM_PMU, DRV_EVENT_PMU_POF);
        }
        REGW(&OM_PMU->POF_INT_CTRL, MASK_1REG(PMU_POF_INT_CTRL_PMU_POF_INT_EN, 1));
        if (!NVIC_GetEnableIRQ(PMU_POF_IRQn)) {
            NVIC_ClearPendingIRQ(PMU_POF_IRQn);
            NVIC_SetPriority(PMU_POF_IRQn, RTE_PMU_POF_IRQ_PRIORITY);
            NVIC_EnableIRQ(PMU_POF_IRQn);
        }
        drv_pmu_env.enable_pof = 1;
    } else {
        REGW(&OM_PMU->POF_INT_CTRL, MASK_1REG(PMU_POF_INT_CTRL_PMU_POF_INT_EN, 0));
        NVIC_DisableIRQ(PMU_POF_IRQn);
        NVIC_ClearPendingIRQ(PMU_POF_IRQn);
        drv_pmu_env.enable_pof = 0;
    }
    OM_CRITICAL_END();
}

#if (RTE_PMU_POF_REGISTER_CALLBACK)
void drv_pmu_pof_register_callback(drv_isr_callback_t isr_callback)
{
    drv_pmu_env.pof_isr_callback = isr_callback;
}
#endif

__WEAK void drv_pmu_pof_isr_callback(OM_PMU_Type *om_pmu, drv_event_t event)
{
    #if (RTE_PMU_POF_REGISTER_CALLBACK)
    if(drv_pmu_env.pof_isr_callback) {
        uint32_t vol = REGR(&OM_PMU->POF_INT_CTRL, MASK_POS(PMU_POF_INT_CTRL_PMU_POF_TH_REG));
        uint32_t mode = REGR(&OM_PMU->POF_INT_CTRL, MASK_POS(PMU_POF_INT_CTRL_PMU_POF_INT_MODE));
        drv_pmu_env.pof_isr_callback(om_pmu, event, (void *)vol, (void *)mode);
    }
    #endif
}

void drv_pmu_pof_isr(void)
{
    OM_PMU->POF_INT_CTRL |= PMU_POF_INT_CTRL_PMU_POF_INT_CLR_MASK;
    drv_pmu_pof_isr_callback(OM_PMU, DRV_EVENT_PMU_POF);
}

void drv_pmu_shelf_mode_enable(uint8_t enable)
{
    #define SHIP_MODE_DELAY       (2*8*1000*1000/32000U)  /* at lease 8 32K clk */
    uint32_t ship_en;

    ship_en = enable ? PMU_SHIP_CTRL_PMU_SM_EN_MASK : 0U;
    for (uint8_t i = 0U; i < 2U; i++) {
        OM_PMU->SHIP_CTRL = ship_en | PMU_SHIP_CTRL_PMU_SM_SW_TRIG_MASK;
        DRV_DELAY_US(SHIP_MODE_DELAY);
        OM_PMU->SHIP_CTRL = ship_en;
        if (i == 0U) {
            DRV_DELAY_US(SHIP_MODE_DELAY);
        }
    }
}

/**
 * @brief pmu dump
 *
 *
 * @note
 *
 * The dump infomation looks like this:
 *   [PMU] prevent_status=00000000
 *   [PMU] wakeup_pin=0001000004(cur_level=0001000004 sleep_level=0001000004)
 *   [PMU] pull_up=FFFD7F9CDF(cur_level=FFFD7F9CDC) pull_down=0000000000(cur_level=0000000000) all_cur_level=FFFFFFFFFC
 *   [PMU] clocking: CPU(32MHz) SF0 SF1 UART0 GPIO ANA
 *
 * Explain:
 * 1st line:
 *   Something (peripheral, user...) prevent system sleep.
 *   bitmask reference @ref pm_id_t
 * 2nd line:
 *   Bitmask of wakeup pin.
 *   If cur_level != sleep_level, system can't sleep.
 * 3rd line:
 *   Inside pull-up and pull-down status.
 *   if pull_up is not equal to it's cur_level, symtem has current leakage in sleep.
 *   if pull_down's cur_level is not equal to 0, system has current leakage in sleep.
 * 4th line:
 *   Working modules.
 *   peripheral: powered block, the more block are powered on, the greater the current consumption during sleep.
 **/
void drv_pmu_dump(void *printf_dump_func)
{
    uint32_t gpio0_wake_en_mask         = OM_PMU->GPIO_WAKEUP[0];
    uint32_t gpio0_current_level_mask   = OM_GPIO0->DATA;
    uint32_t gpio0_wakeup_level_mask    = OM_PMU->GPIO_POL;
    uint32_t gpio0_pull_up_mask         = OM_PMU->GPIO_PU_EN;
    uint32_t gpio0_pull_down_mask       = OM_PMU->GPIO_PD_CTRL;

    uint32_t gpio1_wake_en_mask         = OM_PMU->GPIO_WAKEUP[1];
    uint32_t gpio1_current_level_mask   = OM_GPIO1->DATA;
    uint32_t gpio1_wakeup_level_mask    = OM_PMU->GPIO_POL_1;
    uint32_t gpio1_pull_up_mask         = OM_PMU->GPIO_PU_EN_1;
    uint32_t gpio1_pull_down_mask       = OM_PMU->GPIO_PD_CTRL_1;

    void (*__printf)(const char *fmt, ...) = (void (*)(const char *format, ...))printf_dump_func;
    if (__printf == NULL) {
        return;
    }

    // GPIO0 GROUP wakeup pin
    __printf("[PMU] GPIO0 GROUP wakeup pin: 0x%08X(cur_level: 0x%08X sleep_level: 0x%08X)\n",
            gpio0_wake_en_mask, gpio0_wake_en_mask & gpio0_current_level_mask, gpio0_wakeup_level_mask);
    // GPIO0 GROUP pull status
    __printf("[PMU] GPIO0 GROUP pull up: 0x%08X(cur_level:0x%08X) pull down: 0x%08X(cur_level:0x%08X) all cur_level: 0x%08X\n",
            gpio0_pull_up_mask, gpio0_pull_up_mask & gpio0_current_level_mask, gpio0_pull_down_mask, gpio0_pull_down_mask & gpio0_current_level_mask, gpio0_current_level_mask);


    // GPIO1 GROUP wakeup pin
    __printf("[PMU] GPIO1 GROUP wakeup pin: 0x%08X(cur_level: 0x%08X sleep_level: 0x%08X)\n",
            gpio1_wake_en_mask, gpio1_wake_en_mask & gpio1_current_level_mask, gpio1_wakeup_level_mask);
    // GPIO1 GROUP pull status
    __printf("[PMU] GPIO1 GROUP pull up: 0x%08X(cur_level:0x%08X) pull down: 0x%08X(cur_level:0x%08X) all cur_level: 0x%08X\n",
            gpio1_pull_up_mask, gpio1_pull_up_mask & gpio1_current_level_mask, gpio1_pull_down_mask, gpio1_pull_down_mask & gpio1_current_level_mask, gpio1_current_level_mask);

    // peripherals clock
    if (!(OM_CPM->SF0_CFG & CPM_SF_CFG_GATE_EN_MASK)) {
        __printf(" SF");
    }
    if (!(OM_CPM->TIM0_CFG & CPM_TIM_CFG_GATE_EN_MASK)) {
        __printf(" TIM0");
    }
    if (!(OM_CPM->TIM1_CFG & CPM_TIM_CFG_GATE_EN_MASK)) {
        __printf(" TIM1");
    }
    if (!(OM_CPM-> TIM2_CFG & CPM_TIM_CFG_GATE_EN_MASK)) {
        __printf(" TIM2");
    }
    if (!(OM_CPM->UART0_CFG & CPM_UART_CFG_GATE_EN_MASK)) {
        __printf(" UART0");
    }
    if (!(OM_CPM->UART1_CFG & CPM_UART_CFG_GATE_EN_MASK)) {
        __printf(" UART1");
    }
    if (!(OM_CPM->UART2_CFG & CPM_UART_CFG_GATE_EN_MASK)) {
        __printf(" UART2");
    }
    if (!(OM_CPM->I2C0_CFG & CPM_I2C_CFG_GATE_EN_MASK)) {
        __printf(" I2C0");
    }
    if (!(OM_CPM->I2C1_CFG & CPM_I2C_CFG_GATE_EN_MASK)) {
        __printf(" I2C1");
    }
    if (!(OM_PMU->BASIC & PMU_BASIC_LPTIM_32K_CLK_GATE_MASK)) {
        __printf(" LPTIM");
    }
    if (!(OM_CPM->BLE_CFG & CPM_BLE_CFG_BLE_AHB_GATE_EN_MASK)) {
        __printf(" BLE");
    }
    if (!(OM_CPM->GPDMA_CFG & CPM_GPDMA_CFG_GATE_EN_MASK)) {
        __printf(" DMA");
    }
    if (OM_CPM->AES_CFG & CPM_AES_CFG_CLK_EN_MASK) {
        __printf(" AES");
    }
    if (!(OM_CPM->GPIO_CFG & CPM_GPIO_CFG_GATE_EN_MASK)) {
        __printf(" GPIO0");
    }
    if (!(OM_CPM->PHY_CFG & CPM_PHY_CFG_APB_GATE_EN_MASK)) {
        __printf(" PHY");
    }
    if (!(OM_CPM->RNG_CFG & CPM_RNG_CFG_GATE_EN_MASK)) {
        __printf(" RNG");
    }
    if (!(OM_CPM->MAC_24G_CFG & CPM_2P4_CFG_MAC_GATE_EN_MASK)) {
        __printf(" 2.4G");
    }
    if (!(OM_CPM->ANA_IF_CFG & CPM_ANA_IF_CFG_GATE_EN_MASK)) {
        __printf(" DAIF");
    }
    if (!(OM_CPM->EFUSE_CFG & CPM_EFUSE_CFG_GATE_EN_MASK)) {
        __printf(" EFUSE");
    }
    if (!(OM_CPM->SPI0_CFG & CPM_SPI_CFG_GATE_EN_MASK)) {
        __printf(" SPI0");
    }
    if (!(OM_CPM->SPI1_CFG & CPM_SPI_CFG_GATE_EN_MASK)) {
        __printf(" SPI1");
    }
    if (!(OM_CPM->APB_CFG & CPM_APB_CFG_RTC_APB_GATE_EN_MASK)) {
        __printf(" RTC");
    }
    __printf("\n");
}

#endif  /* RTE_PMU */

/** @} */
