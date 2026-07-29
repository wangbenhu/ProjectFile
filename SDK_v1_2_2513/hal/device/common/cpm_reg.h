/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup CPM CPM
 * @ingroup  DEVICE
 * @brief    CPM register
 * @details  CPM register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __CPM_REG_H
#define __CPM_REG_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include "../common/common_reg.h"


/*******************************************************************************
 * TYPEDEFS
 */
/* CPM register file definitions */
typedef struct {
    __IO uint32_t REV;              // offset:0x00
    __IO uint32_t CPU_CFG;          // offset:0x04  (Auto Restore)
    __IO uint32_t APB_CFG;          // offset:0x08  (Auto Restore)
    __IO uint32_t REG_UPD;          // offset:0x0c
    __IO uint32_t SF0_CFG;          // offset:0x10  (Auto Restore)
    __IO uint32_t TIM0_CFG;         // offset:0x14
    __IO uint32_t TIM1_CFG;         // offset:0x18
    __IO uint32_t TIM2_CFG;         // offset:0x1c
    __IO uint32_t UART0_CFG;        // offset:0x20
    __IO uint32_t UART1_CFG;        // offset:0x24
    __IO uint32_t I2C0_CFG;         // offset:0x28
    __IO uint32_t LPTIM_CFG;        // offset:0x2C
    __IO uint32_t BLE_CFG;          // offset:0x30
    __IO uint32_t CPU_TCLK_CFG;     // offset:0x34
    __IO uint32_t AHB_CFG;          // offset:0x38
    __IO uint32_t GPDMA_CFG;        // offset:0x3c
    __IO uint32_t RAM_CFG;          // offset:0x40  (Auto Restore)
    __IO uint32_t AES_CFG;          // offset:0x44
    __IO uint32_t GPIO_CFG;         // offset:0x48  (Auto Restore)
    __IO uint32_t PHY_CFG;          // offset:0x4c
    __IO uint32_t RNG_CFG;          // offset:0x50
    __IO uint32_t I2S_CFG;          // offset:0x54
    __IO uint32_t STATUS_READ;      // offset:0x58
    __IO uint32_t ANA_IF_APB_CFG;   // offset:0x5c
    __IO uint32_t MAC_24G_CFG;      // offset:0x60  (Auto Restore)
    __IO uint32_t ANA_IF_CFG;       // offset:0x64
    __IO uint32_t EFUSE_CFG;        // offset:0x68
    __IO uint32_t SPI0_CFG;         // offset:0x6C
    __IO uint32_t SPI1_CFG;         // offset:0x70
    __IO uint32_t QDEC_CFG;         // offset:0x74
    __IO uint32_t SLEEP_CFG;        // offset:0x78
    __IO uint32_t USB_CFG;          // offset:0x7C
     uint32_t RSVD_x80;
    __IO uint32_t SHA256_CFG;       // offset:0x84
    __IO uint32_t I2C1_CFG;         // offset:0x88
    uint32_t RSVD_x8C;
    __IO uint32_t UART2_CFG;        // offset:0x90
    uint32_t RSVD_x94;
    __IO uint32_t AUDIO_CFG;        // offset:0x98
    __IO uint32_t IRTX_CFG;         // offset:0x9C
    __IO uint32_t RGB_CFG;    // offset:0xA0
    uint32_t RSVD_xA4_xA8[2];
    __IO uint32_t OSPI1_CFG;        // offset:0xAC  (Auto Restore)
    __IO uint32_t LCD_CFG;          // offset:0xB0
    uint32_t RSVD_xB4_xC0[4];
    __IO uint32_t TRNG_CFG;         // offset:0xC4
} OM_CPM_Type;


/*******************************************************************************
 * MACROS
 */
// REV
#define CPM_REV_CPM_REVISION_POS                                    0
#define CPM_REV_CPM_REVISION_MASK                                   0xFFFFFFFFU

// CPU_CFG
#define CPM_CPU_CFG_PERI_MAIN_CLK_SEL_POS                           26
#define CPM_CPU_CFG_PERI_MAIN_CLK_SEL_MASK                          (1U << 26)
#define CPM_CPU_CFG_AHB_CLK_EN_SW_PERIPH_POS                        25
#define CPM_CPU_CFG_AHB_CLK_EN_SW_PERIPH_MASK                       (1U << 25)
#define CPM_CPU_CFG_AHB_CLK_EN_SW_RAM_POS                           24
#define CPM_CPU_CFG_AHB_CLK_EN_SW_RAM_MASK                          (1U << 24)
#define CPM_CPU_CFG_AHB_CLK_EN_PERIPH_POS                           23
#define CPM_CPU_CFG_AHB_CLK_EN_PERIPH_MASK                          (0x1U << 23)
#define CPM_CPU_CFG_AHB_CLK_EN_RAM_POS                              22
#define CPM_CPU_CFG_AHB_CLK_EN_RAM_MASK                             (0x1U << 22)
#define CPM_CPU_CFG_AHB_CLK_MON_POS                                 21
#define CPM_CPU_CFG_AHB_CLK_MON_MASK                                (0x1U << 21)
#define CPM_CPU_CFG_ROM_CLK_EN_POS                                  20
#define CPM_CPU_CFG_ROM_CLK_EN_MASK                                 (1U << 20)
#define CPM_CPU_CFG_CPU_DIV_COEFF_POS                               8
#define CPM_CPU_CFG_CPU_DIV_COEFF_MASK                              (0xffU << 8)
#define CPM_CPU_CFG_CPU_MAIN_CLK_SYNC_DONE_POS                      6
#define CPM_CPU_CFG_CPU_MAIN_CLK_SYNC_DONE_MASK                     (1U << 6)
#define CPM_CPU_CFG_CPU_CLK_SYNC_DONE_POS                           5
#define CPM_CPU_CFG_CPU_CLK_SYNC_DONE_MASK                          (1U << 5)
#define CPM_CPU_CFG_CPU_DIV_SEL_POS                                 2
#define CPM_CPU_CFG_CPU_DIV_SEL_MASK                                (0x1U << 2)
#define CPM_CPU_CFG_CPU_DIV_EN_POS                                  1
#define CPM_CPU_CFG_CPU_DIV_EN_MASK                                 (0x1U << 1)

// APB_CFG
#define CPM_APB_CFG_SYS_APB_SOFT_RESET_POS                          18
#define CPM_APB_CFG_SYS_APB_SOFT_RESET_MASK                         (0x1U << 18)
#define CPM_APB_CFG_PMU_APB_SOFT_RESET_POS                          16
#define CPM_APB_CFG_PMU_APB_SOFT_RESET_MASK                         (0x1U << 16)
#define CPM_APB_CFG_SYS_APB_AUTO_GATE_DIS_POS                       7
#define CPM_APB_CFG_SYS_APB_AUTO_GATE_DIS_MASK                      (0x1U << 7)
#define CPM_APB_CFG_CPM_APB_AUTO_GATE_DIS_POS                       6
#define CPM_APB_CFG_CPM_APB_AUTO_GATE_DIS_MASK                      (0x1U << 6)
#define CPM_APB_CFG_RTC_APB_AUTO_GATE_DIS_POS                       5
#define CPM_APB_CFG_RTC_APB_AUTO_GATE_DIS_MASK                      (0x1U << 5)
#define CPM_APB_CFG_PMU_APB_AUTO_GATE_DIS_POS                       4
#define CPM_APB_CFG_PMU_APB_AUTO_GATE_DIS_MASK                      (0x1U << 4)
#define CPM_APB_CFG_SYS_APB_GATE_EN_POS                             2
#define CPM_APB_CFG_SYS_APB_GATE_EN_MASK                            (0x1U << 2)
#define CPM_APB_CFG_RTC_APB_GATE_EN_POS                             1
#define CPM_APB_CFG_RTC_APB_GATE_EN_MASK                            (0x1U << 1)
#define CPM_APB_CFG_PMU_APB_GATE_EN_POS                             0
#define CPM_APB_CFG_PMU_APB_GATE_EN_MASK                            (0x1U << 0)

// REG_UPD
#define CPM_REG_UPD_32K_STATUS_POS                                   6
#define CPM_REG_UPD_32K_STATUS_MASK                                  (0x1U << 6)
#define CPM_REG_UPD_RC32K_STATUS_POS                                 5
#define CPM_REG_UPD_RC32K_STATUS_MASK                                (0x1U << 5)
#define CPM_REG_UPD_STATUS_CLR_POS                                   3
#define CPM_REG_UPD_STATUS_CLR_MASK                                  (0x1U << 3)
#define CPM_REG_UPD_32K_APB_POS                                      2
#define CPM_REG_UPD_32K_APB_MASK                                     (0x1U << 2)
#define CPM_REG_UPD_RC32K_APB_POS                                    1
#define CPM_REG_UPD_RC32K_APB_MASK                                   (0x1U << 1)

// SF0_CFG
#define CPM_SF_CFG_DIV_COEFF_POS                                    8
#define CPM_SF_CFG_DIV_COEFF_MASK                                   (0xFFU << 8)
#define CPM_SF_CFG_CLK_SYNC_DONE_POS                                5U
#define CPM_SF_CFG_CLK_SYNC_DONE_MASK                               (1U << 5)
#define CPM_SF_CFG_SOFT_RESET_POS                                   4U
#define CPM_SF_CFG_SOFT_RESET_MASK                                  (1U << 4)
#define CPM_SF_CFG_DIV_SEL_POS                                      2U
#define CPM_SF_CFG_DIV_SEL_MASK                                     (1U << 2)
#define CPM_SF_CFG_DIV_EN_POS                                       1U
#define CPM_SF_CFG_DIV_EN_MASK                                      (1U << 1)
#define CPM_SF_CFG_GATE_EN_POS                                      0U
#define CPM_SF_CFG_GATE_EN_MASK                                     (1U << 0)

// OSPI1_CFG
#define CPM_OSPI_CFG_DIV_COEFF_POS                                  8
#define CPM_OSPI_CFG_DIV_COEFF_MASK                                 (0xFFU << 8)
#define CPM_OSPI_CFG_CLK_SYNC_DONE_POS                              5U
#define CPM_OSPI_CFG_CLK_SYNC_DONE_MASK                             (1U << 5)
#define CPM_OSPI_CFG_SOFT_RESET_POS                                 4U
#define CPM_OSPI_CFG_SOFT_RESET_MASK                                (1U << 4)
#define CPM_OSPI_CFG_DIV_SEL_POS                                    2U
#define CPM_OSPI_CFG_DIV_SEL_MASK                                   (1U << 2)
#define CPM_OSPI_CFG_DIV_EN_POS                                     1U
#define CPM_OSPI_CFG_DIV_EN_MASK                                    (1U << 1)
#define CPM_OSPI_CFG_GATE_EN_POS                                    0U
#define CPM_OSPI_CFG_GATE_EN_MASK                                   (1U << 0)

// TIM0_CFG/TIM1_CFG/TIM2_CFG
#define CPM_TIM_CFG_SOFT_RESET_POS                                   4
#define CPM_TIM_CFG_SOFT_RESET_MASK                                  (0x1U << 4)
#define CPM_TIM_CFG_APB_AUTO_GATE_DIS_POS                            3
#define CPM_TIM_CFG_APB_AUTO_GATE_DIS_MASK                           (0x1U << 3)
#define CPM_TIM_CFG_GATE_EN_POS                                      0
#define CPM_TIM_CFG_GATE_EN_MASK                                     (0x1U << 0)

// UART0_CFG/UART1_CFG/UART2_CFG
#define CPM_UART_CFG_DIV_COEFF_FRC_POS                             24
#define CPM_UART_CFG_DIV_COEFF_FRC_MASK                            (0xffU << 24)
#define CPM_UART_CFG_DIV_COEFF_INT_POS                             8
#define CPM_UART_CFG_DIV_COEFF_INT_MASK                            (0x1ffU << 8)
#define CPM_UART_CFG_CLK_SYNC_DONE_POS                             5U
#define CPM_UART_CFG_CLK_SYNC_DONE_MASK                            (1U << 5)
#define CPM_UART_CFG_SOFT_RESET_POS                                4U
#define CPM_UART_CFG_SOFT_RESET_MASK                               (1U << 4)
#define CPM_UART_CFG_DIV_SEL_POS                                   2U
#define CPM_UART_CFG_DIV_SEL_MASK                                  (1U << 2)
#define CPM_UART_CFG_DIV_EN_POS                                    1U
#define CPM_UART_CFG_DIV_EN_MASK                                   (1U << 1)
#define CPM_UART_CFG_GATE_EN_POS                                   0U
#define CPM_UART_CFG_GATE_EN_MASK                                  (1U << 0)

// I2C0_CFG/I2C1_CFG
#define CPM_I2C_CFG_SOFT_RESET_POS                                   4
#define CPM_I2C_CFG_SOFT_RESET_MASK                                  (0x1U << 4)
#define CPM_I2C_CFG_APB_AUTO_GATE_DIS_POS                            3
#define CPM_I2C_CFG_APB_AUTO_GATE_DIS_MASK                           (0x1U << 3)
#define CPM_I2C_CFG_GATE_EN_POS                                      0
#define CPM_I2C_CFG_GATE_EN_MASK                                     (0x1U << 0)

// LPTIM_CFG
#define CPM_LPTIM_CFG_SOFT_RESET_POS                                4
#define CPM_LPTIM_CFG_SOFT_RESET_MASK                               (0x1U << 4)
#define CPM_LPTIM_CFG_APB_AUTO_GATE_DIS_POS                         3
#define CPM_LPTIM_CFG_APB_AUTO_GATE_DIS_MASK                        (0x1U << 3)
#define CPM_LPTIM_CFG_GATE_EN_POS                                   0
#define CPM_LPTIM_CFG_GATE_EN_MASK                                  (0x1U << 0)

// BLE_CFG
#define CPM_BLE_CFG_BLE_MAC_GATE_EN_POS                             9
#define CPM_BLE_CFG_BLE_MAC_GATE_EN_MASK                            (0x1U << 9)
#define CPM_BLE_CFG_BLE_AHB_SOFT_RESET_POS                          7
#define CPM_BLE_CFG_BLE_AHB_SOFT_RESET_MASK                         (0x1U << 7)
#define CPM_BLE_CFG_BLE_CLK_EN_POS                                  2
#define CPM_BLE_CFG_BLE_CLK_EN_MASK                                 (0x1U << 2)
#define CPM_BLE_CFG_BLE_MASTER_CLK_EN_POS                           1
#define CPM_BLE_CFG_BLE_MASTER_CLK_EN_MASK                          (0x1U << 1)
#define CPM_BLE_CFG_BLE_AHB_GATE_EN_POS                             0
#define CPM_BLE_CFG_BLE_AHB_GATE_EN_MASK                            (0x1U << 0)

// CPU_TCLK_CFG
#define CPM_CPU_TCLK_CFG_GATE_EN_POS                                0
#define CPM_CPU_TCLK_CFG_GATE_EN_MASK                               (0x1U << 0)

// AHB_CFG
#define CPM_AHB_CFG_RAM_AUTO_GATE_EN_POS                            16
#define CPM_AHB_CFG_RAM_AUTO_GATE_EN_MASK                           (0x1U << 16)

// GPDMA_CFG
#define CPM_GPDMA_CFG_SOFT_RESET_POS                                4
#define CPM_GPDMA_CFG_SOFT_RESET_MASK                               (0x1U << 4)
#define CPM_GPDMA_CFG_GATE_EN_POS                                   0
#define CPM_GPDMA_CFG_GATE_EN_MASK                                  (0x1U << 0)

// RAM_CFG
#define CPM_RAM_CFG_PLL_RAM_SEL_POS                                 4
#define CPM_RAM_CFG_PLL_RAM_SEL_MASK                                (0x1U << 4)
#define CPM_RAM_CFG_RAM_GATE_EN_POS                                 0
#define CPM_RAM_CFG_RAM_GATE_EN_MASK                                (0x0FU << 0)
#define CPM_RAM_CFG_RAM3_GATE_EN_POS                                3
#define CPM_RAM_CFG_RAM3_GATE_EN_MASK                               (0x1U << 3)
#define CPM_RAM_CFG_RAM2_GATE_EN_POS                                2
#define CPM_RAM_CFG_RAM2_GATE_EN_MASK                               (0x1U << 2)
#define CPM_RAM_CFG_RAM1_GATE_EN_POS                                1
#define CPM_RAM_CFG_RAM1_GATE_EN_MASK                               (0x1U << 1)
#define CPM_RAM_CFG_RAM0_GATE_EN_POS                                0
#define CPM_RAM_CFG_RAM0_GATE_EN_MASK                               (0x1U << 0)

// AES_CFG
#define CPM_AES_CFG_SOFT_RESET_POS                                   4
#define CPM_AES_CFG_SOFT_RESET_MASK                                  (0x1U << 4)
#define COM_AES_CFG_CLK_SEL_POS                                      1
#define COM_AES_CFG_CLK_SEL_MASK                                     (0x1U << 1)
#define CPM_AES_CFG_CLK_EN_POS                                       0U
#define CPM_AES_CFG_CLK_EN_MASK                                      (1U << 0)

// GPIO_CFG
#define CPM_GPIO_CFG_SOFT_RESET_POS                                  4U
#define CPM_GPIO_CFG_SOFT_RESET_MASK                                 (1U << 4)
#define CPM_GPIO_CFG_GATE_EN_POS                                     0U
#define CPM_GPIO_CFG_GATE_EN_MASK                                    (1U << 0)

// PHY_CFG
#define CPM_PHY_CFG_SOFT_RESET_POS                                   4U
#define CPM_PHY_CFG_SOFT_RESET_MASK                                  (1U << 4)
#define CPM_PHY_CFG_APB_AUTO_GATE_DIS_POS                            3U
#define CPM_PHY_CFG_APB_AUTO_GATE_DIS_MASK                           (1U << 3)
#define CPM_PHY_CFG_16M_GATE_EN_POS                                  1U
#define CPM_PHY_CFG_16M_GATE_EN_MASK                                 (1U << 1)
#define CPM_PHY_CFG_APB_GATE_EN_POS                                  0U
#define CPM_PHY_CFG_APB_GATE_EN_MASK                                 (1U << 0)

// RNG_CFG
#define CPM_RNG_CFG_SOFT_RESET_POS                                   4U
#define CPM_RNG_CFG_SOFT_RESET_MASK                                  (1U << 4)
#define CPM_RNG_CFG_APB_AUTO_GATE_DIS_POS                            3U
#define CPM_RNG_CFG_APB_AUTO_GATE_DIS_MASK                           (1U << 3)
#define CPM_RNG_CFG_GATE_EN_POS                                      0U
#define CPM_RNG_CFG_GATE_EN_MASK                                     (1U << 0)

// I2S_CFG
#define CPM_I2S_CFG_MST_DIV_COEFF_POS                               18U
#define CPM_I2S_CFG_MST_DIV_COEFF_MASK                              (0x3FU << 18)
#define CPM_I2S_CFG_MST_ODD_POS                                     17U
#define CPM_I2S_CFG_MST_ODD_MASK                                    (1U << 17)
#define CPM_I2S_CFG_MST_HIGH_NUM_POS                                8U
#define CPM_I2S_CFG_MST_HIGH_NUM_MASK                               (0x1FFU << 8)
#define CPM_I2S_CFG_MS_SRC_SEL_POS                                  6U
#define CPM_I2S_CFG_MS_SRC_SEL_MASK                                 (3U << 6)
#define CPM_I2S_CFG_MS_SEL_POS                                      5U
#define CPM_I2S_CFG_MS_SEL_MASK                                     (1U << 5)
#define CPM_I2S_CFG_SOFT_RESET_POS                                  4U
#define CPM_I2S_CFG_SOFT_RESET_MASK                                 (1U << 4)
#define CPM_I2S_CFG_EXT_INV_POS                                     2U
#define CPM_I2S_CFG_EXT_INV_MASK                                    (1U << 2)
#define CPM_I2S_CFG_MST_EN_POS                                      1U
#define CPM_I2S_CFG_MST_EN_MASK                                     (1U << 1)
#define CPM_I2S_CFG_AHB_GATE_EN_POS                                 0U
#define CPM_I2S_CFG_AHB_GATE_EN_MASK                                (1U << 0)

// STATUS_READ
#define CPM_STATUS_READ_PERI_MAIN_CLK_SYNC_DONE_POS                 0U
#define CPM_STATUS_READ_PERI_MAIN_CLK_SYNC_DONE_MASK                (1U << 0)

// ANA_IF_APB_CFG
#define CPM_ANA_IF_APB_CFG_SOFT_RESET_POS                            4U
#define CPM_ANA_IF_APB_CFG_SOFT_RESET_MASK                           (1U << 4)
#define CPM_ANA_IF_APB_CFG_AUTO_GATE_DIS_POS                         3U
#define CPM_ANA_IF_APB_CFG_AUTO_GATE_DIS_MASK                        (1U << 3)
#define CPM_ANA_IF_APB_CFG_GATE_EN_POS                               0U
#define CPM_ANA_IF_APB_CFG_GATE_EN_MASK                              (1U << 0)

// MAC_24G_CFG
#define CPM_2P4_CFG_MAC_SOFT_RESET_POS                              4U
#define CPM_2P4_CFG_MAC_SOFT_RESET_MASK                             (1U << 4)
#define CPM_2P4_CFG_MAC_AUTO_GATE_DIS_POS                           3U
#define CPM_2P4_CFG_MAC_AUTO_GATE_DIS_MASK                          (1U << 3)
#define CPM_2P4_CFG_TIMER_GATE_EN_POS                               1U
#define CPM_2P4_CFG_TIMER_GATE_EN_MASK                              (1U << 1)
#define CPM_2P4_CFG_MAC_GATE_EN_POS                                 0U
#define CPM_2P4_CFG_MAC_GATE_EN_MASK                                (1U << 0)

// ANA_IF_CFG
#define CPM_ANA_IF_CFG_GATE_EN_POS                                  0U
#define CPM_ANA_IF_CFG_GATE_EN_MASK                                 (1U << 0)

// EFUSE_CFG
#define CPM_EFUSE_CFG_CLK_SYNC_DONE_POS                              5U
#define CPM_EFUSE_CFG_CLK_SYNC_DONE_MASK                             (1U << 5)
#define CPM_EFUSE_CFG_SOFT_RESET_POS                                 4U
#define CPM_EFUSE_CFG_SOFT_RESET_MASK                                (1U << 4)
#define CPM_EFUSE_CFG_APB_AUTO_GATE_DIS_POS                          3U
#define CPM_EFUSE_CFG_APB_AUTO_GATE_DIS_MASK                         (1U << 3)
#define CPM_EFUSE_CFG_CLK_SRC_SEL_POS                                1U
#define CPM_EFUSE_CFG_CLK_SRC_SEL_MASK                               (1U << 1)
#define CPM_EFUSE_CFG_GATE_EN_POS                                    0U
#define CPM_EFUSE_CFG_GATE_EN_MASK                                   (1U << 0)

// SPI0_CFG/SPI1_CFG
#define CPM_SPI_CFG_SOFT_RESET_POS                                   4U
#define CPM_SPI_CFG_SOFT_RESET_MASK                                  (1U << 4)
#define CPM_SPI_CFG_APB_AUTO_GATE_DIS_POS                            3U
#define CPM_SPI_CFG_APB_AUTO_GATE_DIS_MASK                           (1U << 3)
#define CPM_SPI_CFG_GATE_EN_POS                                      0U
#define CPM_SPI_CFG_GATE_EN_MASK                                     (1U << 0)

// QDEC_CFG
#define CPM_QDEC_CFG_DIV_COEFF_POS                                  8U
#define CPM_QDEC_CFG_DIV_COEFF_MASK                                 (0x7FU << 8)
#define CPM_QDEC_CFG_SOFT_RESET_POS                                 4U
#define CPM_QDEC_CFG_SOFT_RESET_MASK                                (1U << 4)
#define CPM_QDEC_CFG_APB_AUTO_GATE_DIS_POS                          3U
#define CPM_QDEC_CFG_APB_AUTO_GATE_DIS_MASK                         (1U << 3)
#define CPM_QDEC_CFG_GATE_EN_POS                                    0U
#define CPM_QDEC_CFG_GATE_EN_MASK                                   (1U << 0)

// SLEEP_CFG
#define CPM_SLEEP_CFG_GATE_EN_POS                                   0U
#define CPM_SLEEP_CFG_GATE_EN_MASK                                  (1U << 0)

// USB_CFG
#define CPM_USB_CFG_GATE_EN_POS                                     3U
#define CPM_USB_CFG_GATE_EN_MASK                                    (1U << 3)
#define CPM_USB_CFG_AHB_GATE_EN_POS                                 1U
#define CPM_USB_CFG_AHB_GATE_EN_MASK                                (1U << 1)

// SHA256_CFG
#define CPM_SHA256_CFG_SOFT_RESET_POS                               4U
#define CPM_SHA256_CFG_SOFT_RESET_MASK                              (1U << 4)
#define CPM_SHA256_CFG_APB_AUTO_GATE_DIS_POS                        3U
#define CPM_SHA256_CFG_APB_AUTO_GATE_DIS_MASK                       (1U << 3)
#define CPM_SHA256_CFG_GATE_EN_POS                                  0U
#define CPM_SHA256_CFG_GATE_EN_MASK                                 (1U << 0)

// AUDIO_CFG
#define CPM_AUDIO_CFG_SOFT_RESET_POS                                4U
#define CPM_AUDIO_CFG_SOFT_RESET_MASK                               (1U << 4)
#define CPM_AUDIO_CFG_APB_AUTO_GATE_DIS_POS                         3U
#define CPM_AUDIO_CFG_APB_AUTO_GATE_DIS_MASK                        (1U << 3)
#define CPM_AUIDO_CFG_GATE_EN_POS                                   0U
#define CPM_AUIDO_CFG_GATE_EN_MASK                                  (1U << 0)

// IRTX_CFG
#define CPM_IRTX_CFG_SOFT_RESET_POS                                 4U
#define CPM_IRTX_CFG_SOFT_RESET_MASK                                (1U << 4)
#define CPM_IRTX_CFG_APB_AUTO_GATE_DIS_POS                          3U
#define CPM_IRTX_CFG_APB_AUTO_GATE_DIS_MASK                         (1U << 3)
#define CPM_IRTX_CFG_GATE_EN_POS                                    0U
#define CPM_IRTX_CFG_GATE_EN_MASK                                   (1U << 0)

// RGB_CFG
#define CPM_RGB_CFG_SOFT_RESET_POS                                  4U
#define CPM_RGB_CFG_SOFT_RESET_MASK                                 (1U << 4)
#define CPM_RGB_CFG_APB_AUTO_GATE_DIS_POS                           3U
#define CPM_RGB_CFG_APB_AUTO_GATE_DIS_MASK                          (1U << 3)
#define CPM_RGB_CFG_GATE_EN_POS                                     0U
#define CPM_RGB_CFG_GATE_EN_MASK                                    (1U << 0)

// LCD_CFG
#define CPM_LCD_CFG_DIV_COEFF_POS                                   8
#define CPM_LCD_CFG_DIV_COEFF_MASK                                  (0xFFU << 8)
#define CPM_LCD_CFG_CLK_SYNC_DONE_POS                               5U
#define CPM_LCD_CFG_CLK_SYNC_DONE_MASK                              (1U << 5)
#define CPM_LCD_CFG_SOFT_RESET_POS                                  4U
#define CPM_LCD_CFG_SOFT_RESET_MASK                                 (1U << 4)
#define CPM_LCD_CFG_DIV_SEL_POS                                     2U
#define CPM_LCD_CFG_DIV_SEL_MASK                                    (1U << 2)
#define CPM_LCD_CFG_DIV_EN_POS                                      1U
#define CPM_LCD_CFG_DIV_EN_MASK                                     (1U << 1)
#define CPM_LCD_CFG_GATE_EN_POS                                     0U
#define CPM_LCD_CFG_GATE_EN_MASK                                    (1U << 0)

// TRNG_CFG
#define CPM_TRNG_CFG_SOFT_RESET_POS                                 4U
#define CPM_TRNG_CFG_SOFT_RESET_MASK                                (1U << 4)
#define CPM_TRNG_CFG_APB_AUTO_GATE_DIS_POS                          3U
#define CPM_TRNG_CFG_APB_AUTO_GATE_DIS_MASK                         (1U << 3)
#define CPM_TRNG_CFG_GATE_EN_POS                                    0U
#define CPM_TRNG_CFG_GATE_EN_MASK                                   (1U << 0)

// Generic_CFG
#define CPM_XXX_CFG_DIV_COEFF_POS                                   8U
#define CPM_XXX_CFG_DIV_COEFF_MASK                                  (0xFFU << 8)
#define CPM_XXX_CFG_CLK_SYNC_DONE_POS                               5U
#define CPM_XXX_CFG_CLK_SYNC_DONE_MASK                              (1U << 5)
#define CPM_XXX_CFG_DIV_SEL_POS                                     2U
#define CPM_XXX_CFG_DIV_SEL_MASK                                    (1U << 2)
#define CPM_XXX_CFG_DIV_EN_POS                                      1U
#define CPM_XXX_CFG_DIV_EN_MASK                                     (1U << 1)
#define CPM_XXX_CFG_GATE_EN_POS                                     0U
#define CPM_XXX_CFG_GATE_EN_MASK                                    (1U << 0)

#endif  /* __CPM_REG_H*/


/** @} */
