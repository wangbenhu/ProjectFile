/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup RCC RCC
 * @ingroup  DRIVER
 * @brief    RCC driver
 * @details  RCC driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_RCC_H
#define __DRV_RCC_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_RCC)
#include "om_device.h"
#include "om_driver.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * TYPEDEFS
 */
/// Clock Source Definition
typedef enum {
    RCC_CLK_MAIN        = 0x0100U,
    RCC_CLK_CPU         = 0x0200U,
    RCC_CLK_PERI        = 0x0300U,

    RCC_CLK_SF0         = OM_SF0_BASE,
    RCC_CLK_OSPI1       = OM_OSPI1_BASE,
    RCC_CLK_TIM0        = OM_TIM0_BASE,
    RCC_CLK_TIM1        = OM_TIM1_BASE,
    RCC_CLK_TIM2        = OM_TIM2_BASE,
    RCC_CLK_UART0       = OM_UART0_BASE,
    RCC_CLK_UART1       = OM_UART1_BASE,
    RCC_CLK_UART2       = OM_UART2_BASE,

    RCC_CLK_I2C0        = OM_I2C0_BASE,
    RCC_CLK_I2C1        = OM_I2C1_BASE,
    RCC_CLK_LPTIM       = OM_LPTIM_BASE,
    RCC_CLK_BLE         = OM_BB_BASE,
    RCC_CLK_GPDMA       = OM_GPDMA_BASE,
    RCC_CLK_AES         = OM_AES_BASE,
    RCC_CLK_GPIO0       = OM_GPIO0_BASE,
    RCC_CLK_GPIO1       = OM_GPIO1_BASE,
    RCC_CLK_PHY         = OM_PHY_BASE,
    RCC_CLK_RNG         = OM_RNG_BASE,
    RCC_CLK_TRNG        = OM_TRNG_BASE,
    RCC_CLK_DAIF        = OM_DAIF_BASE,
    RCC_CLK_2P4         = OM_24G_BASE,
    RCC_CLK_EFUSE       = OM_EFUSE_BASE,
    RCC_CLK_SPI0        = OM_SPI0_BASE,
    RCC_CLK_SPI1        = OM_SPI1_BASE,
    RCC_CLK_RTC         = OM_RTC_BASE,
    RCC_CLK_LCD         = OM_LCD_BASE,
    RCC_CLK_SHA256      = OM_SHA256_BASE,
    RCC_CLK_QDEC        = OM_QDEC_BASE,
    RCC_CLK_IRTX        = OM_IRTX_BASE,
    RCC_CLK_RGB         = OM_RGB_BASE,
    RCC_CLK_USB         = OM_USB_BASE,
    RCC_CLK_I2S         = OM_I2S_BASE,
    RCC_CLK_AUDIO       = OM_AUDIO_BASE,
} rcc_clk_t;

typedef enum {
    RCC_CPU_CLK_SOURCE_RC32M,
    RCC_CPU_CLK_SOURCE_XTAL32M,
    RCC_CPU_CLK_SOURCE_XTAL64M,          /* XTAL32M x 2 */
    RCC_CPU_CLK_SOURCE_SYSPLL96M,
    RCC_CPU_CLK_SOURCE_SYSPLL48M,        /* PLL96M / 2 */

    RCC_CPU_CLK_SOURCE_MAX,
} rcc_cpu_clk_source_t;

typedef enum {
    RCC_PERIPH_CLK_SOURCE_RC32M,
    RCC_PERIPH_CLK_SOURCE_XTAL32M,
    RCC_PERIPH_CLK_SOURCE_SYSPLL48M,     /* PLL96M / 2 */
    RCC_PERIPH_CLK_SOURCE_XTAL64M,       /* XTAL32M x 2 */

    RCC_PERIPH_CLK_SOURCE_MAX,
} rcc_periph_clk_source_t;


/*******************************************************************************
 * MACROS
 */
/**
 *******************************************************************************
 * @brief Enable clock for specified peripheral, _rcc_clk_type should used const
 *        or macro not used parameter as in parameter for redused code size.
 *
 * @param[in] _rcc_clk_type         rcc clock type
 * @param[in] enable                0: disable clock   not 0: enable clock
 *******************************************************************************
 */
#define DRV_RCC_CLOCK_ENABLE(_rcc_clk_type, enable)                                                              \
    do {                                                                                                         \
        switch ((uint32_t)_rcc_clk_type) {                                                                       \
            case RCC_CLK_SF0:                                                                                    \
                register_set(&OM_CPM->SF0_CFG, MASK_1REG(CPM_SF_CFG_GATE_EN, (enable) ? 0 : 1));                 \
                break;                                                                                           \
            case RCC_CLK_OSPI1:                                                                                  \
                register_set(&OM_CPM->OSPI1_CFG, MASK_1REG(CPM_OSPI_CFG_GATE_EN, (enable) ? 0 : 1));             \
                break;                                                                                           \
            case RCC_CLK_TIM0:                                                                                   \
                register_set(&OM_CPM->TIM0_CFG, MASK_1REG(CPM_TIM_CFG_GATE_EN, (enable) ? 0 : 1));               \
                break;                                                                                           \
            case RCC_CLK_TIM1:                                                                                   \
                register_set(&OM_CPM->TIM1_CFG, MASK_1REG(CPM_TIM_CFG_GATE_EN, (enable) ? 0 : 1));               \
                break;                                                                                           \
            case RCC_CLK_TIM2:                                                                                   \
                register_set(&OM_CPM->TIM2_CFG, MASK_1REG(CPM_TIM_CFG_GATE_EN, (enable) ? 0 : 1));               \
                break;                                                                                           \
            case RCC_CLK_UART0:                                                                                  \
                register_set(&OM_CPM->UART0_CFG, MASK_1REG(CPM_UART_CFG_GATE_EN, (enable) ? 0 : 1));             \
                break;                                                                                           \
            case RCC_CLK_UART1:                                                                                  \
                register_set(&OM_CPM->UART1_CFG, MASK_1REG(CPM_UART_CFG_GATE_EN, (enable) ? 0 : 1));             \
                break;                                                                                           \
            case RCC_CLK_UART2:                                                                                  \
                register_set(&OM_CPM->UART2_CFG, MASK_1REG(CPM_UART_CFG_GATE_EN, (enable) ? 0 : 1));             \
                break;                                                                                           \
            case RCC_CLK_I2C0:                                                                                   \
                register_set(&OM_CPM->I2C0_CFG, MASK_1REG(CPM_I2C_CFG_GATE_EN, (enable) ? 0 : 1));               \
                break;                                                                                           \
            case RCC_CLK_I2C1:                                                                                   \
                register_set(&OM_CPM->I2C1_CFG, MASK_1REG(CPM_I2C_CFG_GATE_EN, (enable) ? 0 : 1));               \
                break;                                                                                           \
            case RCC_CLK_LPTIM:                                                                                  \
                drv_pmu_basic_reg_set(MASK_1REG(PMU_BASIC_LPTIM_32K_CLK_GATE, (enable) ? 0 : 1));                \
                register_set(&OM_CPM->LPTIM_CFG, MASK_1REG(CPM_LPTIM_CFG_GATE_EN, (enable) ? 0 : 1));            \
                break;                                                                                           \
            case RCC_CLK_BLE:                                                                                    \
                register_set(&OM_CPM->BLE_CFG, MASK_4REG(CPM_BLE_CFG_BLE_AHB_GATE_EN,   (enable) ? 0 : 1,        \
                                                         CPM_BLE_CFG_BLE_MASTER_CLK_EN, (enable) ? 1 : 0,        \
                                                         CPM_BLE_CFG_BLE_CLK_EN,        (enable) ? 1 : 0,        \
                                                         CPM_BLE_CFG_BLE_MAC_GATE_EN,   (enable) ? 0 : 1));      \
                break;                                                                                           \
            case RCC_CLK_GPDMA:                                                                                  \
                register_set(&OM_CPM->GPDMA_CFG, MASK_1REG(CPM_GPDMA_CFG_GATE_EN, (enable) ? 0 : 1));            \
                break;                                                                                           \
            case RCC_CLK_AES:                                                                                    \
                register_set(&OM_CPM->AES_CFG, MASK_1REG(CPM_AES_CFG_CLK_EN, (enable) ? 1 : 0));                 \
                break;                                                                                           \
            case RCC_CLK_GPIO0:                                                                                  \
                register_set(&OM_CPM->GPIO_CFG, MASK_1REG(CPM_GPIO_CFG_GATE_EN, (enable) ? 0 : 1));              \
                break;                                                                                           \
            case RCC_CLK_PHY:                                                                                    \
                register_set(&OM_CPM->PHY_CFG, MASK_2REG(CPM_PHY_CFG_16M_GATE_EN, (enable) ? 0 : 1,              \
                                                         CPM_PHY_CFG_APB_GATE_EN, (enable) ? 0 : 1));            \
                break;                                                                                           \
            case RCC_CLK_RNG:                                                                                    \
                register_set(&OM_CPM->RNG_CFG, MASK_1REG(CPM_RNG_CFG_GATE_EN, (enable) ? 0 : 1));                \
                do {                                                                                             \
                    OM_CPM->REG_UPD = CPM_REG_UPD_STATUS_CLR_MASK;                                               \
                } while(OM_CPM->REG_UPD);                                                                        \
                OM_CPM->REG_UPD = CPM_REG_UPD_RC32K_APB_MASK;                                                    \
                while(!(OM_CPM->REG_UPD & CPM_REG_UPD_RC32K_STATUS_MASK));                                       \
                break;                                                                                           \
            case RCC_CLK_TRNG:                                                                                   \
                register_set(&OM_CPM->TRNG_CFG, MASK_1REG(CPM_TRNG_CFG_GATE_EN, (enable) ? 0 : 1));              \
                break;                                                                                           \
            case RCC_CLK_2P4:                                                                                    \
                register_set(&OM_CPM->MAC_24G_CFG, MASK_1REG(CPM_2P4_CFG_MAC_GATE_EN, (enable) ? 0 : 1));        \
                break;                                                                                           \
            case RCC_CLK_DAIF:                                                                                   \
                register_set(&OM_CPM->ANA_IF_APB_CFG, MASK_1REG(CPM_ANA_IF_APB_CFG_GATE_EN, (enable) ? 0 : 1));  \
                OM_CPM->ANA_IF_CFG = (enable) ? 0 : 1;                                                           \
                break;                                                                                           \
            case RCC_CLK_EFUSE:                                                                                  \
                register_set(&OM_CPM->EFUSE_CFG, MASK_1REG(CPM_EFUSE_CFG_GATE_EN, (enable) ? 0 : 1));            \
                break;                                                                                           \
            case RCC_CLK_SPI0:                                                                                   \
                register_set(&OM_CPM->SPI0_CFG, MASK_1REG(CPM_SPI_CFG_GATE_EN, (enable) ? 0 : 1));               \
                break;                                                                                           \
            case RCC_CLK_SPI1:                                                                                   \
                register_set(&OM_CPM->SPI1_CFG, MASK_1REG(CPM_SPI_CFG_GATE_EN, (enable) ? 0 : 1));               \
                break;                                                                                           \
            case RCC_CLK_RTC:                                                                                    \
                OM_CRITICAL_BEGIN();                                                                             \
                /* open RTC power and clock */                                                                   \
                OM_PMU->PSO_PM |= PMU_PSO_RTC_POWER_ON_MASK;                                                     \
                while (!(OM_PMU->PSO_PM & PMU_PSO_RTC_POWER_STATUS_MASK));  /* wait RTC power on */              \
                drv_pmu_basic_reg_set(MASK_1REG(PMU_BASIC_RTC_CLK_GATE, (enable) ? 0 : 1));                      \
                register_set(&OM_CPM->APB_CFG, MASK_1REG(CPM_APB_CFG_RTC_APB_GATE_EN, (enable) ? 0 : 1));        \
                OM_CRITICAL_END();                                                                               \
                break;                                                                                           \
            case RCC_CLK_LCD:                                                                                    \
                register_set(&OM_CPM->LCD_CFG, MASK_1REG(CPM_LCD_CFG_GATE_EN, (enable) ? 0 : 1));                \
                break;                                                                                           \
            case RCC_CLK_SHA256:                                                                                 \
                register_set(&OM_CPM->SHA256_CFG, MASK_1REG(CPM_SHA256_CFG_GATE_EN, (enable) ? 0 : 1));          \
                break;                                                                                           \
            case RCC_CLK_QDEC:                                                                                   \
                register_set(&OM_CPM->QDEC_CFG, MASK_1REG(CPM_QDEC_CFG_GATE_EN, (enable) ? 0 : 1));              \
                break;                                                                                           \
            case RCC_CLK_IRTX:                                                                                   \
                register_set(&OM_CPM->IRTX_CFG, MASK_1REG(CPM_IRTX_CFG_GATE_EN, (enable) ? 0 : 1));              \
                break;                                                                                           \
            case RCC_CLK_RGB:                                                                                    \
                register_set(&OM_CPM->RGB_CFG, MASK_1REG(CPM_RGB_CFG_GATE_EN, (enable) ? 0 : 1));                \
                break;                                                                                           \
            case RCC_CLK_USB:                                                                                    \
                OM_CPM->USB_CFG = (enable) ? 0U : (CPM_USB_CFG_GATE_EN_MASK | CPM_USB_CFG_AHB_GATE_EN_MASK);     \
                break;                                                                                           \
            case RCC_CLK_I2S:                                                                                    \
                register_set(&OM_CPM->I2S_CFG, MASK_1REG(CPM_I2S_CFG_AHB_GATE_EN, (enable) ? 0 : 1));            \
                break;                                                                                           \
            case RCC_CLK_AUDIO:                                                                                  \
                register_set(&OM_CPM->AUDIO_CFG, MASK_1REG(CPM_AUIDO_CFG_GATE_EN, (enable) ? 0 : 1));            \
                break;                                                                                           \
            default:                                                                                             \
                break;                                                                                           \
        }                                                                                                        \
    } while(0)

/**
 *******************************************************************************
 * @brief Enable peripheral clock and reset peripheral, _rcc_clk_type should used
 *        const or macro not used parameter as in parameter for redused code size.
 *
 * @param[in] _rcc_clk_type         rcc clock type
 *******************************************************************************
 */
#define DRV_RCC_RESET(_rcc_clk_type)                                                            \
    do {                                                                                        \
        DRV_RCC_CLOCK_ENABLE(_rcc_clk_type, 1U);                                                \
        switch((uint32_t)_rcc_clk_type) {                                                       \
            case RCC_CLK_SF0:                                                                   \
                OM_CPM->SF0_CFG |= CPM_SF_CFG_SOFT_RESET_MASK;                                  \
                while(OM_CPM->SF0_CFG & CPM_SF_CFG_SOFT_RESET_MASK);                            \
                break;                                                                          \
            case RCC_CLK_OSPI1:                                                                 \
                OM_CPM->OSPI1_CFG |= CPM_OSPI_CFG_SOFT_RESET_MASK;                              \
                while(OM_CPM->OSPI1_CFG & CPM_OSPI_CFG_SOFT_RESET_MASK);                        \
                break;                                                                          \
            case RCC_CLK_TIM0:                                                                  \
                OM_CPM->TIM0_CFG |= CPM_TIM_CFG_SOFT_RESET_MASK;                                \
                while(OM_CPM->TIM0_CFG & CPM_TIM_CFG_SOFT_RESET_MASK);                          \
                break;                                                                          \
            case RCC_CLK_TIM1:                                                                  \
                OM_CPM->TIM1_CFG |= CPM_TIM_CFG_SOFT_RESET_MASK;                                \
                while(OM_CPM->TIM1_CFG & CPM_TIM_CFG_SOFT_RESET_MASK);                          \
                break;                                                                          \
            case RCC_CLK_TIM2:                                                                  \
                OM_CPM->TIM2_CFG |= CPM_TIM_CFG_SOFT_RESET_MASK;                                \
                while(OM_CPM->TIM2_CFG & CPM_TIM_CFG_SOFT_RESET_MASK);                          \
                break;                                                                          \
            case RCC_CLK_UART0:                                                                 \
                OM_CPM->UART0_CFG |= CPM_UART_CFG_SOFT_RESET_MASK;                              \
                while(OM_CPM->UART0_CFG & CPM_UART_CFG_SOFT_RESET_MASK);                        \
                break;                                                                          \
            case RCC_CLK_UART1:                                                                 \
                OM_CPM->UART1_CFG |= CPM_UART_CFG_SOFT_RESET_MASK;                              \
                while(OM_CPM->UART1_CFG & CPM_UART_CFG_SOFT_RESET_MASK);                        \
                break;                                                                          \
            case RCC_CLK_UART2:                                                                 \
                OM_CPM->UART2_CFG |= CPM_UART_CFG_SOFT_RESET_MASK;                              \
                while(OM_CPM->UART2_CFG & CPM_UART_CFG_SOFT_RESET_MASK);                        \
                break;                                                                          \
            case RCC_CLK_I2C0:                                                                  \
                OM_CPM->I2C0_CFG |= CPM_I2C_CFG_SOFT_RESET_MASK;                                \
                while(OM_CPM->I2C0_CFG & CPM_I2C_CFG_SOFT_RESET_MASK);                          \
                break;                                                                          \
            case RCC_CLK_I2C1:                                                                  \
                OM_CPM->I2C1_CFG |= CPM_I2C_CFG_SOFT_RESET_MASK;                                \
                while(OM_CPM->I2C1_CFG & CPM_I2C_CFG_SOFT_RESET_MASK);                          \
                break;                                                                          \
            case RCC_CLK_LPTIM:                                                                     \
                OM_CRITICAL_BEGIN();                                                                \
                register_set(&OM_PMU->MISC_CTRL, MASK_2REG(PMU_MISC_CTRL_LPTIM_APB_SOFT_RESET, 0,   \
                                                           PMU_MISC_CTRL_LPTIM_CLK_SFOT_RESET, 1)); \
                DRV_DELAY_US(40);                                                                   \
                register_set(&OM_PMU->MISC_CTRL, MASK_2REG(PMU_MISC_CTRL_LPTIM_APB_SOFT_RESET, 1,   \
                                                           PMU_MISC_CTRL_LPTIM_CLK_SFOT_RESET, 0)); \
                OM_CRITICAL_END();                                                                  \
                break;                                                                          \
            case RCC_CLK_BLE:                                                                   \
                OM_CPM->BLE_CFG |= CPM_BLE_CFG_BLE_AHB_SOFT_RESET_MASK;                         \
                OM_CPM->BLE_CFG &= ~CPM_BLE_CFG_BLE_AHB_SOFT_RESET_MASK;                        \
                break;                                                                          \
            case RCC_CLK_GPDMA:                                                                 \
                OM_CPM->GPDMA_CFG |= CPM_GPDMA_CFG_SOFT_RESET_MASK;                             \
                while(OM_CPM->GPDMA_CFG & CPM_GPDMA_CFG_SOFT_RESET_MASK);                       \
                break;                                                                          \
            case RCC_CLK_AES:                                                                   \
                OM_CPM->AES_CFG |= CPM_AES_CFG_SOFT_RESET_MASK;                                 \
                while(OM_CPM->AES_CFG & CPM_AES_CFG_SOFT_RESET_MASK);                           \
                break;                                                                          \
            case RCC_CLK_GPIO0:                                                                 \
            case RCC_CLK_GPIO1:                                                                 \
                OM_CPM->GPIO_CFG  |= CPM_GPIO_CFG_SOFT_RESET_MASK;                              \
                while(OM_CPM->GPIO_CFG & CPM_GPIO_CFG_SOFT_RESET_MASK);                         \
                break;                                                                          \
            case RCC_CLK_PHY:                                                                   \
                OM_CPM->PHY_CFG |= CPM_PHY_CFG_SOFT_RESET_MASK;                                 \
                while(OM_CPM->PHY_CFG & CPM_PHY_CFG_SOFT_RESET_MASK);                           \
                break;                                                                          \
            case RCC_CLK_RNG:                                                                   \
                OM_CPM->RNG_CFG |= CPM_RNG_CFG_SOFT_RESET_MASK;                                 \
                while(OM_CPM->RNG_CFG & CPM_RNG_CFG_SOFT_RESET_MASK);                           \
                break;                                                                          \
            case RCC_CLK_TRNG:                                                                  \
                OM_CPM->TRNG_CFG |= CPM_TRNG_CFG_SOFT_RESET_MASK;                               \
                while(OM_CPM->TRNG_CFG & CPM_TRNG_CFG_SOFT_RESET_MASK);                         \
                break;                                                                          \
            case RCC_CLK_2P4:                                                                   \
                OM_CPM->MAC_24G_CFG |= CPM_2P4_CFG_MAC_SOFT_RESET_MASK;                         \
                while(OM_CPM->MAC_24G_CFG & CPM_2P4_CFG_MAC_SOFT_RESET_MASK);                   \
                break;                                                                          \
            case RCC_CLK_DAIF:                                                                  \
                OM_CPM->ANA_IF_APB_CFG |= CPM_ANA_IF_APB_CFG_SOFT_RESET_MASK;                   \
                while(OM_CPM->ANA_IF_APB_CFG & CPM_ANA_IF_APB_CFG_SOFT_RESET_MASK);             \
                break;                                                                          \
            case RCC_CLK_EFUSE:                                                                 \
                OM_CPM->EFUSE_CFG |= CPM_EFUSE_CFG_SOFT_RESET_MASK;                             \
                while(OM_CPM->EFUSE_CFG & CPM_EFUSE_CFG_SOFT_RESET_MASK);                       \
                break;                                                                          \
            case RCC_CLK_SPI0:                                                                  \
                OM_CPM->SPI0_CFG |= CPM_SPI_CFG_SOFT_RESET_MASK;                                \
                while(OM_CPM->SPI0_CFG & CPM_SPI_CFG_SOFT_RESET_MASK);                          \
                break;                                                                          \
            case RCC_CLK_SPI1:                                                                  \
                OM_CPM->SPI1_CFG |= CPM_SPI_CFG_SOFT_RESET_MASK;                                \
                while(OM_CPM->SPI1_CFG & CPM_SPI_CFG_SOFT_RESET_MASK);                          \
                break;                                                                          \
            case RCC_CLK_LCD:                                                                   \
                OM_CPM->LCD_CFG |= CPM_LCD_CFG_SOFT_RESET_MASK;                                 \
                while(OM_CPM->LCD_CFG & CPM_LCD_CFG_SOFT_RESET_MASK);                           \
                break;                                                                          \
            case RCC_CLK_SHA256:                                                                \
                OM_CPM->SHA256_CFG |= CPM_SHA256_CFG_SOFT_RESET_MASK;                           \
                while(OM_CPM->SHA256_CFG & CPM_SHA256_CFG_SOFT_RESET_MASK);                     \
                break;                                                                          \
            case RCC_CLK_QDEC:                                                                  \
                OM_CPM->QDEC_CFG |= CPM_QDEC_CFG_SOFT_RESET_MASK;                               \
                while(OM_CPM->QDEC_CFG & CPM_QDEC_CFG_SOFT_RESET_MASK);                         \
                break;                                                                          \
            case RCC_CLK_IRTX:                                                                  \
                OM_CPM->IRTX_CFG |= CPM_IRTX_CFG_SOFT_RESET_MASK;                               \
                while(OM_CPM->IRTX_CFG & CPM_IRTX_CFG_SOFT_RESET_MASK);                         \
                break;                                                                          \
            case RCC_CLK_RGB:                                                                   \
                OM_CPM->RGB_CFG |= CPM_RGB_CFG_SOFT_RESET_MASK;                                 \
                while(OM_CPM->RGB_CFG & CPM_RGB_CFG_SOFT_RESET_MASK);                           \
                break;                                                                          \
            case RCC_CLK_USB:                                                                   \
                OM_PMU->USB_CTRL1 |= PMU_USB_CTRL1_AHB_SOFT_RST_MASK|PMU_USB_CTRL1_USB_SOFT_RST_MASK|PMU_USB_CTRL1_WU_SOFT_RST_MASK; \
                while(OM_PMU->USB_CTRL1 & (PMU_USB_CTRL1_AHB_SOFT_RST_MASK|PMU_USB_CTRL1_USB_SOFT_RST_MASK|PMU_USB_CTRL1_WU_SOFT_RST_MASK)); \
                break;                                                                          \
            case RCC_CLK_I2S:                                                                   \
                OM_CPM->I2S_CFG |= CPM_I2S_CFG_SOFT_RESET_MASK;                                 \
                while(OM_CPM->I2S_CFG & CPM_I2S_CFG_SOFT_RESET_MASK);                           \
                break;                                                                          \
            case RCC_CLK_AUDIO:                                                                 \
                OM_CPM->AUDIO_CFG |= CPM_AUDIO_CFG_SOFT_RESET_MASK;                             \
                while(OM_CPM->AUDIO_CFG & CPM_AUDIO_CFG_SOFT_RESET_MASK);                       \
                break;                                                                          \
            case RCC_CLK_RTC:                                                                   \
                OM_CRITICAL_BEGIN();                                                            \
                OM_PMU->MISC_CTRL &= ~PMU_MISC_CTRL_RTC_SOFT_RESET_MASK;                        \
                OM_PMU->MISC_CTRL |= PMU_MISC_CTRL_RTC_APB_SOFT_RESET_MASK;                     \
                while (OM_PMU->MISC_CTRL & PMU_MISC_CTRL_RTC_APB_SOFT_RESET_MASK);              \
                OM_CRITICAL_END();                                                              \
                break;                                                                          \
            default:                                                                            \
                break;                                                                          \
        }                                                                                       \
    } while(0)

/**
 *******************************************************************************
 * @brief ana if clock open
 *
 *******************************************************************************
 */
#define DRV_RCC_ANA_CLK_ENABLE_NOIRQ()                                         \
    do {                                                                       \
        uint32_t __daif_clk_is_dis = OM_CPM->ANA_IF_CFG;                       \
        if (__daif_clk_is_dis) {                                               \
            DRV_RCC_CLOCK_ENABLE(RCC_CLK_DAIF, 1U);                            \
        }                                                                      \

/**
 *******************************************************************************
 * @brief ana if clock restore
 *
 *******************************************************************************
 */
#define DRV_RCC_ANA_CLK_RESTORE_NOIRQ()                                        \
        if (__daif_clk_is_dis) {                                               \
            DRV_RCC_CLOCK_ENABLE(RCC_CLK_DAIF, 0U);                            \
        }                                                                      \
    } while(0)                                                                 \

/**
 *******************************************************************************
 * @brief ana if clock open in the critical section
 *
 *******************************************************************************
 */
#define DRV_RCC_ANA_CLK_ENABLE()        OM_CRITICAL_BEGIN(); DRV_RCC_ANA_CLK_ENABLE_NOIRQ()

/**
 *******************************************************************************
 * @brief ana if clock restore in the critical section
 *
 *******************************************************************************
 */
#define DRV_RCC_ANA_CLK_RESTORE()       DRV_RCC_ANA_CLK_RESTORE_NOIRQ(); OM_CRITICAL_END()


/*******************************************************************************
 * EXTERN VARIABLES
 */
/// CPU clock
extern uint32_t SystemCoreClock;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Initialize system clock
 *
 *******************************************************************************
 */
extern void drv_rcc_init(void);

/**
 *******************************************************************************
 * @brief Get clock frequency for specified clock source
 *
 * @param[in] rcc_clk        Clock source
 *
 * @return clock frequency
 *******************************************************************************
 */
extern uint32_t drv_rcc_clock_get(rcc_clk_t rcc_clk);

/**
 *******************************************************************************
 * @brief Set clock frequency for specified clock source
 *
 * @param[in] rcc_clk        Clock source
 * @param[in] freq           Clock frequency, unit Hz
 *
 * @note if freq <= 64, freq will be considered as frequency division
 *
 * @return error information
 *******************************************************************************
 */
extern om_error_t drv_rcc_clock_set(rcc_clk_t rcc_clk, uint32_t freq);

/**
 *******************************************************************************
 * @brief Set cpu clock source
 *
 * @param[in] source         CPU clock source
 *******************************************************************************
 */
extern void drv_rcc_cpu_clk_source_set(rcc_cpu_clk_source_t source);

/**
 *******************************************************************************
 * @brief Set peripheral clock source
 *
 * @param[in] source         Peripheral clock source
 *******************************************************************************
 */
extern void drv_rcc_periph_clk_source_set(rcc_periph_clk_source_t source);

#ifdef __cplusplus
}
#endif

#endif  /* RTE_RCC */

#endif  /* __DRV_RCC_H */


/** @} */
