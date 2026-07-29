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
 * @brief    RCC driver source file
 * @details  RCC driver source file
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
#if (RTE_RCC)
#include <stddef.h>
#include "om_device.h"
#include "om_driver.h"


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    rcc_clk_t       rcc_clk;
    __IO uint32_t   *rcc_reg;
} rcc_reg_table_t;

typedef struct {
    uint32_t      periph_freq;
} rcc_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
uint32_t SystemCoreClock = 32 * 1000 * 1000;
static rcc_env_t rcc_env = {
    .periph_freq = 32U * 1000U * 1000U,
};


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static __IO uint32_t *rcc_clk_search(rcc_clk_t rcc_clk)
{
    const rcc_reg_table_t rcc_reg_table[] = {
        {RCC_CLK_CPU,           &OM_CPM->CPU_CFG},
        {RCC_CLK_UART0,         &OM_CPM->UART0_CFG},
        {RCC_CLK_UART1,         &OM_CPM->UART1_CFG},
        {RCC_CLK_UART2,         &OM_CPM->UART2_CFG},
        {RCC_CLK_SF0,           &OM_CPM->SF0_CFG},
        {RCC_CLK_OSPI1,         &OM_CPM->OSPI1_CFG},
        {RCC_CLK_LCD,           &OM_CPM->LCD_CFG},
    };

    for (uint32_t i = 0; i < (sizeof(rcc_reg_table) / sizeof(rcc_reg_table[0])); i++) {
        if (rcc_reg_table[i].rcc_clk == rcc_clk) {
            return rcc_reg_table[i].rcc_reg;
        }
    }

    return NULL;
}

#if (RTE_UART0 || RTE_UART1 || RTE_UART2)
static om_error_t rcc_clk_uart_div_set(volatile uint32_t *rcc_reg, uint32_t freq)
{
    uint32_t div_x256;
    uint32_t int_div, frc_div;

    div_x256 = (uint32_t)((((uint64_t)rcc_env.periph_freq) << 8) / freq);
    int_div = div_x256 >> 8U;
    if (int_div > (CPM_UART_CFG_DIV_COEFF_INT_MASK >> CPM_UART_CFG_DIV_COEFF_INT_POS)) {
        return OM_ERROR_OUT_OF_RANGE;
    }
    frc_div = div_x256 & 0xFFU;

    if (int_div < 2) {
        register_set(rcc_reg, MASK_2REG(CPM_UART_CFG_DIV_SEL,       0,
                                        CPM_UART_CFG_DIV_EN,        1));
    } else {
        register_set(rcc_reg, MASK_4REG(CPM_UART_CFG_DIV_COEFF_INT, int_div,
                                        CPM_UART_CFG_DIV_COEFF_FRC, frc_div,
                                        CPM_UART_CFG_DIV_SEL,       1,
                                        CPM_UART_CFG_DIV_EN,        1));
    }

    return OM_ERROR_OK;
}
#endif


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
void drv_rcc_init(void)
{
    SystemCoreClock = 32U*1000U*1000U;
    rcc_env.periph_freq = 32*1000U*1000U;
}

uint32_t drv_rcc_clock_get(rcc_clk_t rcc_clk)
{
    uint32_t div;
    volatile uint32_t *rcc_reg;
    uint32_t rcc_cfg;

    rcc_reg = rcc_clk_search(rcc_clk);
    rcc_cfg = (rcc_reg == NULL) ? 0U : (*((volatile uint32_t *)rcc_reg));
    switch (rcc_clk) {
        case RCC_CLK_MAIN:
            if (OM_PMU->MISC_CTRL & PMU_MISC_CTRL_MAIN_CLK_SEL_MASK) {
                // select sys clock
                // sel_cpuclk[1:0] = 00: xtal32m, 01: clk64m, 10: syspll96m, 11: syspll48m
                if (OM_DAIF->SYSPLL_CNS0 & DAIF_SYSPLL_SEL_CPUCLK_MASK) {
                    return (OM_PMU->XTAL32M_CNS0 & PMU_XTAL32M_CNS0_SEL_CPUCLK_MASK) ? 48U*1000U*1000U : 96U*1000U*1000U;
                } else {
                    return (OM_PMU->XTAL32M_CNS0 & PMU_XTAL32M_CNS0_SEL_CPUCLK_MASK) ? 64U*1000U*1000U : 32U*1000U*1000U;
                }
            } else {
                // select rc32m
                return 32U*1000U*1000U; // use rc32m
            }
            break;  /*lint !e527 */
        case RCC_CLK_RTC:
            return 32768U;
        case RCC_CLK_LCD:
        case RCC_CLK_SF0:
        case RCC_CLK_OSPI1:
            if (rcc_cfg & CPM_XXX_CFG_GATE_EN_MASK) {               // is gated
                return 0U;
            } else if (!(rcc_cfg & CPM_XXX_CFG_DIV_SEL_MASK)) {     // not select divider
                return rcc_env.periph_freq;
            } else {                                                // select divider and enable
                return rcc_env.periph_freq / (rcc_cfg >> CPM_XXX_CFG_DIV_COEFF_POS);
            }
            break;
        case RCC_CLK_CPU:
        case RCC_CLK_GPDMA:
        case RCC_CLK_GPIO0:
        case RCC_CLK_GPIO1:
        case RCC_CLK_SHA256:
             return SystemCoreClock;
        case RCC_CLK_PERI:
        case RCC_CLK_TIM0:
        case RCC_CLK_TIM1:
        case RCC_CLK_TIM2:
        case RCC_CLK_SPI0:
        case RCC_CLK_SPI1:
        case RCC_CLK_I2C0:
        case RCC_CLK_I2C1:
        case RCC_CLK_QDEC:
        case RCC_CLK_IRTX:
        case RCC_CLK_RGB:
            return rcc_env.periph_freq;
        case RCC_CLK_UART0:
        case RCC_CLK_UART1:
        case RCC_CLK_UART2:
            if (rcc_cfg & CPM_XXX_CFG_GATE_EN_MASK) {               // is gated
                return 0U;
            } else if (!(rcc_cfg & CPM_XXX_CFG_DIV_SEL_MASK)) {     // not select divider
                return rcc_env.periph_freq;
            } else {                                                // select divider and enable
                div = (rcc_cfg & CPM_XXX_CFG_DIV_COEFF_MASK) >> CPM_XXX_CFG_DIV_COEFF_POS;
                return rcc_env.periph_freq / div;
            }
            break;
        default:
            break;
    }

    return 0U;
}

om_error_t drv_rcc_clock_set(rcc_clk_t rcc_clk, uint32_t freq)
{
    om_error_t error = OM_ERROR_OK;
    uint32_t div;
    volatile uint32_t *rcc_reg;

    OM_ASSERT(freq != 0U);
    rcc_reg = rcc_clk_search(rcc_clk);
    switch (rcc_clk) {
        case RCC_CLK_CPU:
            do {
                uint32_t main_clk = drv_rcc_clock_get(RCC_CLK_MAIN);
                OM_ASSERT(freq <= main_clk);

                div = (freq <= 64*1000*1000U) ? freq : main_clk / freq;
                if (div < 2U) {
                    register_set(&OM_CPM->CPU_CFG, MASK_2REG(CPM_CPU_CFG_CPU_DIV_SEL, 0U,
                                                             CPM_CPU_CFG_CPU_DIV_EN,  1U));
                    SystemCoreClock = main_clk;
                } else {
                    register_set(&OM_CPM->CPU_CFG, MASK_3REG(CPM_CPU_CFG_CPU_DIV_COEFF, div,
                                                             CPM_CPU_CFG_CPU_DIV_SEL,   1U,
                                                             CPM_CPU_CFG_CPU_DIV_EN,    1U));
                    SystemCoreClock = main_clk / ((OM_CPM->CPU_CFG & CPM_CPU_CFG_CPU_DIV_COEFF_MASK) >> CPM_CPU_CFG_CPU_DIV_COEFF_POS);
                }
                while((OM_CPM->CPU_CFG & CPM_CPU_CFG_CPU_CLK_SYNC_DONE_MASK) == 0U);
            } while(0);
            break;
        case RCC_CLK_SF0:
        case RCC_CLK_OSPI1:
        case RCC_CLK_LCD:
            div = (uint32_t)(rcc_env.periph_freq / freq);
            if (div < 2U) {
                register_set(rcc_reg, MASK_3REG(CPM_XXX_CFG_DIV_SEL, 0U,
                                                CPM_XXX_CFG_DIV_EN,  0U,
                                                CPM_XXX_CFG_GATE_EN, 0U));
            } else {
                register_set(rcc_reg, MASK_4REG(CPM_XXX_CFG_DIV_COEFF, div,
                                                CPM_XXX_CFG_DIV_SEL,   1U,
                                                CPM_XXX_CFG_DIV_EN,    1U,
                                                CPM_XXX_CFG_GATE_EN,   0U));
            }
            while(((*rcc_reg) & CPM_XXX_CFG_CLK_SYNC_DONE_MASK) == 0U);
            break;

        #if (RTE_UART0 || RTE_UART1 || RTE_UART2)
        case RCC_CLK_UART0:
        case RCC_CLK_UART1:
        case RCC_CLK_UART2:
            error = rcc_clk_uart_div_set(rcc_reg, freq);
            if (error == OM_ERROR_OK) {
                while(((*rcc_reg) & CPM_UART_CFG_CLK_SYNC_DONE_MASK) == 0U);
            }
            break;
        #endif
        default:
            error = OM_ERROR_UNSUPPORTED;
            break;
    }

    return error;
}

void drv_rcc_cpu_clk_source_set(rcc_cpu_clk_source_t source)
{
    uint32_t target_clk = 0U;
    uint8_t rc32m_is_last_enabled;

    OM_CRITICAL_BEGIN();
    DRV_RCC_ANA_CLK_ENABLE_NOIRQ();

    rc32m_is_last_enabled = drv_pmu_rc32m_enable(true);
    if (source == RCC_CPU_CLK_SOURCE_RC32M) {
        rc32m_is_last_enabled = true;
        REGW0(&OM_PMU->MISC_CTRL, PMU_MISC_CTRL_MAIN_CLK_SEL_MASK); // 0:RC32MHz  1:CPU_CLK_IN
        while(!(OM_CPM->CPU_CFG & CPM_CPU_CFG_CPU_MAIN_CLK_SYNC_DONE_MASK));
        target_clk = 32 * 1000 * 1000U;
        drv_pmu_dvdd_voltage_set(PMU_DVDD_VOLTAGE_0P95V);
        goto _exit;
    }

    drv_rcc_cpu_clk_source_set(RCC_CPU_CLK_SOURCE_RC32M);
    // switch cpu_clk_in
    if (source == RCC_CPU_CLK_SOURCE_XTAL32M) {
        drv_pmu_dvdd_voltage_set(PMU_DVDD_VOLTAGE_0P95V);

        REGW0(&OM_PMU->XTAL32M_CNS0, PMU_XTAL32M_CNS0_SEL_CPUCLK_MASK);
        REGW0(&OM_DAIF->SYSPLL_CNS0, DAIF_SYSPLL_SEL_CPUCLK_MASK);
        target_clk = 32 * 1000 * 1000U;
    } else if (source == RCC_CPU_CLK_SOURCE_XTAL64M) {
        drv_pmu_dvdd_voltage_set(PMU_DVDD_VOLTAGE_1P05V);

        REGW0(&OM_PMU->CLK_CTRL_2, PMU_CLK_CTRL_2_SEL_RCOSC_MASK);      // 0:XTAL 1:RC
        REGW1(&OM_PMU->XTAL32M_CNS0, PMU_XTAL32M_CNS0_SEL_CPUCLK_MASK); // 0:xtal32m 1:64m
        REGW0(&OM_DAIF->SYSPLL_CNS0, DAIF_SYSPLL_SEL_CPUCLK_MASK);
        target_clk = 64 * 1000 * 1000U;
    } else if (source == RCC_CPU_CLK_SOURCE_SYSPLL96M) {
        drv_pmu_dvdd_voltage_set(PMU_DVDD_VOLTAGE_1P15V);

        REGW0(&OM_PMU->XTAL32M_CNS0, PMU_XTAL32M_CNS0_SEL_CPUCLK_MASK);
        REGW1(&OM_DAIF->SYSPLL_CNS0, DAIF_SYSPLL_SEL_CPUCLK_MASK);
        target_clk = 96 * 1000 * 1000U;
    } else if (source == RCC_CPU_CLK_SOURCE_SYSPLL48M) {
        drv_pmu_dvdd_voltage_set(PMU_DVDD_VOLTAGE_1P05V);

        REGW1(&OM_PMU->XTAL32M_CNS0, PMU_XTAL32M_CNS0_SEL_CPUCLK_MASK);
        REGW1(&OM_DAIF->SYSPLL_CNS0, DAIF_SYSPLL_SEL_CPUCLK_MASK);
        target_clk = 48 * 1000 * 1000U;
    }
    DRV_DELAY_US(2);
    // switch cpu_clk to cpu_clk_in
    REGW1(&OM_PMU->MISC_CTRL, PMU_MISC_CTRL_MAIN_CLK_SEL_MASK); // 0:RC32MHz  1:CPU_CLK_IN
    while(!(OM_CPM->CPU_CFG & CPM_CPU_CFG_CPU_MAIN_CLK_SYNC_DONE_MASK));

_exit:
    SystemCoreClock = target_clk;

    if (!rc32m_is_last_enabled) {
        drv_pmu_rc32m_enable(false);
    }

    DRV_RCC_ANA_CLK_RESTORE_NOIRQ();
    OM_CRITICAL_END();
}

__RAM_CODE void drv_rcc_periph_clk_source_set(rcc_periph_clk_source_t source)
{
    uint32_t target_clk = 0U;
    uint8_t rc32m_is_last_enabled;

    OM_CRITICAL_BEGIN();
    DRV_RCC_ANA_CLK_ENABLE_NOIRQ();

    rc32m_is_last_enabled = drv_pmu_rc32m_enable(true);
    if (source == RCC_PERIPH_CLK_SOURCE_RC32M) {
        rc32m_is_last_enabled = true;
        REGW0(&OM_CPM->CPU_CFG, CPM_CPU_CFG_PERI_MAIN_CLK_SEL_MASK);
        while(!(OM_CPM->STATUS_READ & CPM_STATUS_READ_PERI_MAIN_CLK_SYNC_DONE_MASK));
        target_clk = 32 * 1000 * 1000U;
        goto _exit;
    }

    drv_rcc_periph_clk_source_set(RCC_PERIPH_CLK_SOURCE_RC32M);
    // switch periph_clk_in
    if (source == RCC_PERIPH_CLK_SOURCE_XTAL32M) {
        REGW(&OM_DAIF->SYSPLL_CNS0, MASK_1REG(DAIF_SYSPLL_SEL_PERICLK, 0U));
        target_clk = 32 * 1000 * 1000U;
    } else if (source == RCC_PERIPH_CLK_SOURCE_SYSPLL48M) {
        REGW(&OM_DAIF->SYSPLL_CNS0, MASK_1REG(DAIF_SYSPLL_SEL_PERICLK, 1U));
        target_clk = 48 * 1000 * 1000U;
    } else if (source == RCC_PERIPH_CLK_SOURCE_XTAL64M) {
        REGW0(&OM_PMU->CLK_CTRL_2, PMU_CLK_CTRL_2_SEL_RCOSC_MASK);      // 0:XTAL 1:RC
        REGW(&OM_DAIF->SYSPLL_CNS0, MASK_1REG(DAIF_SYSPLL_SEL_PERICLK, 2U));
        target_clk = 64 * 1000 * 1000U;
    }
    DRV_DELAY_US(2);
    // switch periph_clk to periph_clk_in
    REGW1(&OM_CPM->CPU_CFG, CPM_CPU_CFG_PERI_MAIN_CLK_SEL_MASK);
    while(!(OM_CPM->STATUS_READ & CPM_STATUS_READ_PERI_MAIN_CLK_SYNC_DONE_MASK));

_exit:
    rcc_env.periph_freq = target_clk;

    if (!rc32m_is_last_enabled) {
        drv_pmu_rc32m_enable(false);
    }
    // recalibrate iflash delay
    drv_iflash_delay_recalib();

    DRV_RCC_ANA_CLK_RESTORE_NOIRQ();
    OM_CRITICAL_END();
}

#endif /* RTE_RCC */


/** @} */
