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
 * @brief    CMSIS Device System Source File
 * @details  CMSIS Device System Source File
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
#include <stddef.h>
#include "om_device.h"
#include "om_common.h"


/*******************************************************************************
 * TYPEDEFS
 */
/**
 * @brief Exception / Interrupt Handler Function Prototype
 */
typedef void(*VECTOR_TABLE_Type)(void);

/**
 *******************************************************************************
 * @brief Exception / Interrupt Vector table
 *******************************************************************************
 */
extern const VECTOR_TABLE_Type __VECTOR_TABLE[];    /*lint !e526*/


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static inline void register_set(volatile uint32_t *reg, uint32_t mask, uint32_t value)
{
    register uint32_t reg_prev;

    reg_prev = *reg;
    reg_prev &= ~mask;
    reg_prev |= mask & value;
    *reg = reg_prev;
}

static inline void register_pmu_basic_set(uint32_t mask, uint32_t value)
{
    register uint32_t reg_prev;

    reg_prev = OM_PMU->BASIC;
    reg_prev &= ~mask;
    reg_prev |= mask & value;
    OM_PMU->BASIC = reg_prev;
    while ((OM_PMU->BASIC & mask) != (value & mask));
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief System initialization function
 *******************************************************************************
 */
void SystemInit(void)
{
    /* check chip version */
    while((SYS_CHIP_ID_GET() & 0xFFFFU) != 26151U);
    /* while (((OM_SYS->REV_ID & SYS_REV_CHIP_VER_MASK) >> SYS_REV_CHIP_VER_POS) != (CONFIG_HARDWARE_VERSION - 1U)); */
    /* check RAM size */
    #if (OM_MEM_RAM_SIZE)
    while (__get_MSP() > (OM_MEM_RAM_SIZE + 0x20000000U));
    #endif
    /* power down unused ram */
    #if (OM_MEM_RAM_SIZE == 128*1024U)
    OM_PMU->PSO_PM &= ~(PMU_PSO_RAM4_POWER_ON_MASK | PMU_PSO_RAM5_POWER_ON_MASK);
    #endif
    // Retention LDO: 0=0.6v 2=0.7v 3=0.75v 4=0.8v 5=0.85v 6=0.9v 7=0.95v(default) 8=1.0v
    register_set(&OM_PMU->ANA_PD_1, PMU_ANA_PD_1_DIG_RETLDO_TRIM_MASK, (5U << PMU_ANA_PD_1_DIG_RETLDO_TRIM_POS));
    // DCDC on delay is 3*32k
    register_pmu_basic_set(PMU_BASIC_DCDC_ON_DELAY_MASK, (3U << PMU_BASIC_DCDC_ON_DELAY_POS));
    // DCDC频率振荡调节寄存器,CTUNE值越大振荡频率越慢
    register_set(&OM_PMU->ANA_PD, PMU_ANA_PD_DCDC_CTUNE_MASK, (3U << PMU_ANA_PD_DCDC_CTUNE_POS));
    // DCDC PSM/PWM 电流切换调节寄存器,电流大的时候DCDC会从PSM模式切到PWM模式
    register_set(&OM_PMU->ANA_PD_1, PMU_ANA_PD_1_DCDC_PSM_VREF_MASK, (0U << PMU_ANA_PD_1_DCDC_PSM_VREF_POS));

    #if 0
    // power on all sram and icache ram(power on in ROM)
    OM_PMU->PSO_PM |= PMU_PSO_RAM1_2_POWER_ON_MASK | PMU_PSO_RAM3_POWER_ON_MASK
                   | PMU_PSO_RAM4_POWER_ON_MASK | PMU_PSO_RAM5_POWER_ON_MASK
                   | PMU_PSO_ICACHE_POWER_ON_MASK;
    uint32_t power_status = PMU_PSO_RAM1_2_POWER_STATUS_MASK | PMU_PSO_RAM3_POWER_STATUS_MASK
                          | PMU_PSO_RAM4_POWER_STATUS_MASK | PMU_PSO_RAM5_POWER_STATUS_MASK
                          | PMU_PSO_ICACHE_POWER_STATUS_MASK;
    while ((OM_PMU->PSO_PM & power_status) != power_status);
    #endif

    /* icache enable */
    OM_ICACHE->CONFIG = 0;
    OM_ICACHE->CTRL = ICACHE_CTRL_CEN_MASK;
    while((OM_ICACHE->STATUS & ICACHE_STATUS_CSTS_MASK) == 0);

    /**
     * Disable clock & Reset: UART1/EFUSE/GPDMA using in ROM;
     * LCD and LPTIM is enabled as default
     * FLASH1 disabled in pm_init
     * RTC clock disabled
     * RAM auto gate enable
     **/
    OM_CPM->UART1_CFG |= CPM_UART_CFG_GATE_EN_MASK | CPM_UART_CFG_SOFT_RESET_MASK;
    OM_CPM->EFUSE_CFG |= CPM_EFUSE_CFG_GATE_EN_MASK | CPM_EFUSE_CFG_SOFT_RESET_MASK;
    OM_CPM->GPDMA_CFG |= CPM_GPDMA_CFG_GATE_EN_MASK | CPM_GPDMA_CFG_SOFT_RESET_MASK;
    OM_CPM->LCD_CFG |= CPM_LCD_CFG_GATE_EN_MASK;
    register_pmu_basic_set(PMU_BASIC_LPTIM_32K_CLK_GATE_MASK, PMU_BASIC_LPTIM_32K_CLK_GATE_MASK);
    OM_CPM->LPTIM_CFG |= CPM_LPTIM_CFG_GATE_EN_MASK;
    if (!(OM_PMU->RSVD_SW_REG[0] & PMU_RSVD_SW_REG_RTC_SECOND_RESTORE_PROCESS_MASK)) {
        register_pmu_basic_set(PMU_BASIC_RTC_CLK_GATE_MASK, PMU_BASIC_RTC_CLK_GATE_MASK);
        OM_CPM->APB_CFG |= CPM_APB_CFG_RTC_APB_GATE_EN_MASK;
    }
    OM_CPM->AHB_CFG |= CPM_AHB_CFG_RAM_AUTO_GATE_EN_MASK;

    // retention ALL SRAM, icache ram, ble ram, pso ram when deep sleep
    uint32_t ram_ctrl = OM_PMU->RAM_CTRL_2 & (~(0xFFFU << 18));
    OM_PMU->RAM_CTRL_2 =  ram_ctrl | PMU_RAM_CTRL_2_RAM0_DS_HW_CTRL_EN_MASK | PMU_RAM_CTRL_2_RAM1_DS_HW_CTRL_EN_MASK
                         | PMU_RAM_CTRL_2_RAM2_DS_HW_CTRL_EN_MASK | PMU_RAM_CTRL_2_RAM_PSO_DS_HW_CTRL_EN_MASK
                         | PMU_RAM_CTRL_2_RAM_BLE_DS_HW_CTRL_EN_MASK | PMU_RAM_CTRL_2_RAM_IC_DS_HW_CTRL_EN_MASK;
    ram_ctrl = OM_PMU->RAM_CTRL_1 & (~((3U << 30) | (3U << 26) | (3U << 22)));
    OM_PMU->RAM_CTRL_1 = ram_ctrl | PMU_RAM_CTRL_1_RAM3_DS_HW_CTRL_EN_MASK | PMU_RAM_CTRL_1_RAM4_DS_HW_CTRL_EN_MASK | PMU_RAM_CTRL_1_RAM5_DS_HW_CTRL_EN_MASK;

    #if defined (__VTOR_PRESENT) && (__VTOR_PRESENT == 1U)
    SCB->VTOR = (uint32_t) & (__VECTOR_TABLE[0]);
    #endif

    #if (__FPU_PRESENT == 1U) && (__FPU_USED == 1U)
    SCB->CPACR |= ((3U << 20U) | (3U << 22U));              /* Enable CP10 and CP11 Full Access */
    #endif

    #ifdef UNALIGNED_SUPPORT_DISABLE
    SCB->CCR |= SCB_CCR_UNALIGN_TRP_Msk;
    #endif

    // Enable DWT Cycle counter
    do {
        COREDEBUG->DEMCR |= COREDEBUG_DEMCR_TRCENA_MASK;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    } while(0);

    // Set xtal32m load capacitance
    register_set(&OM_PMU->CLK_CTRL_2, PMU_CLK_CTRL_2_CT_XTAL32M_MASK, (37U << PMU_CLK_CTRL_2_CT_XTAL32M_POS));      // TX
    register_set(&OM_PMU->OM26B_CFG0, PMU_OM26B_CFG0_XTAL32M_CT_RX_MASK,(37U << PMU_OM26B_CFG0_XTAL32M_CT_RX_POS)); // RX

    // Increase PMU LDO output current limit to improve ldo voltage stability
    register_set(&OM_PMU->OM26B_CFG0, PMU_OM26B_CFG0_PMU_LDO_ANANLDO_ILIMIT_MASK, (1U << PMU_OM26B_CFG0_PMU_LDO_ANANLDO_ILIMIT_POS));

    // Reset ROM Uart1 Pins(5,6) to GPIO
    register_set(&OM_SYS->PINMUX[1], SYS_PINMUX_MASK(28, 8));
    register_set(&OM_SYS->PINMUX[1], SYS_PINMUX_MASK(28, 16));
    // Fixed io are latched when wdt reset in sleep or wakeup in ultra_deep_sleep
    register_set(&OM_PMU->MISC_CTRL, (PMU_MISC_CTRL_GPIO_AUTO_LATCH_FSM_DIS_MASK | PMU_MISC_CTRL_GPIO_AUTO_LATCH_CTRL_MASK), 1U << PMU_MISC_CTRL_GPIO_AUTO_LATCH_FSM_DIS_POS);
}

/**
 *******************************************************************************
 * @brief  system init post, called after init ".data", ".bss" and "other section"
 *
 * @note  This may be implemented in the system library to import data such as CP/FT calibration.
 *******************************************************************************
 */
__WEAK void SystemInitPost(void)
{
    extern void lib_cpft_init(void);
    extern void lib_cpft_uninit(void);

    //TODO: CPFT & Flash settings
    // NULL: Set iflash default config: freq is 8MHz, delay is 2, bus width is 2
    extern uint8_t drv_iflash_init(OM_SF_Type *om_flash, void *cfg);
    drv_iflash_init(OM_SF0, NULL);
    lib_cpft_init();
    lib_cpft_uninit();
}

/** @} */
