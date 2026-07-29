/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup SYS SYS
 * @ingroup  DEVICE
 * @brief    SYS register
 * @details  SYS register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __SYS_REG_H
#define __SYS_REG_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include "common_reg.h"


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    __I  uint32_t REV_ID;               // offset:0x00
    __I  uint32_t CHIP_ID;              // offset:0x04
         uint32_t RESERVE1[3];
    __IO uint32_t SYS_TICK;             // offset:0x14
    __IO uint32_t GPIO_POWER_UP_STATUS; // offset:0x18
    __IO uint32_t GPIO_POWER_UP_STATUS_1;//offset:0x1C
    __IO uint32_t RST_32K_OSC_CTRL;     // offset:0x20
         uint32_t RESERVE3[5];
    __IO uint32_t MON;                  // offset:0x38
    __IO uint32_t USB_CTRL;             // offset:0x3C
    __I  uint32_t CHRGR_STAT;           // offset:0x40
    __IO uint32_t SOFT_INT_SET;         // offset:0x44
    __IO uint32_t SOFT_INT_CLR;         // offset:0x48
         uint32_t RESERVE5[13];
    __IO uint32_t PINMUX[10];           // offset:0x80  (Auto Restore)
} OM_SYS_Type;


/*******************************************************************************
 * MACROS
 */
// REV_ID
#define SYS_REV_ANA_POS                             0U
#define SYS_REV_ANA_MASK                            (0x0FU << 0)
#define SYS_REV_CHIP_VER_POS                        8U
#define SYS_REV_CHIP_VER_MASK                       (0x0FU << 8)
#define SYS_REV_FPGA_POS                            15U
#define SYS_REV_FPGA_MASK                           (1U << 15)
#define SYS_IS_FPGA()                               (OM_SYS->REV_ID & SYS_REV_FPGA_MASK)

// CHIP_ID
#define SYS_CHIP_ID_GET()                           (OM_SYS->CHIP_ID)

// GPIO_POWER_UP_STATUS
#define SYS_GPIO_STAUS_POWERDOWN_MASK               0x80000000
// SOFT_INT
#define SYS_SOFT_INI_0_MASK                         0x00000001
#define SYS_SOFT_INI_1_MASK                         0x00000002
#define SYS_SOFT_INI_2_MASK                         0x00000004
#define SYS_SOFT_INI_3_MASK                         0x00000008
#define SYS_SOFT_INI_4_MASK                         0x00000010
#define SYS_SOFT_INI_5_MASK                         0x00000020
#define SYS_SOFT_INI_6_MASK                         0x00000040
#define SYS_SOFT_INI_7_MASK                         0x00000080
// SYS_TICK
#define SYS_CPU_SYS_TICK_CON_POS                    0
#define SYS_CPU_SYS_TICK_UPD_POS                    26
#define SYS_CPU_SYS_TICK_CON_MASK                   0x03FFFFFF
#define SYS_CPU_SYS_TICK_UPD_MASK                   0x04000000
// RST_32K_OSC_CTRL
#define SYS_CRY32M_DIV_EN_POS                       12
#define SYS_CLK_32K_DIV_RST_TRIG_POS                11
#define SYS_RST_32K_RC_TRIG_POS                     10
#define SYS_RST_32K_RC_TIME_POS                     0
#define SYS_CRY32M_DIV_EN_MASK                      0x00001000
#define SYS_CLK_32K_DIV_RST_TRIG_MASK               0x00000800
#define SYS_RST_32K_RC_TRIG_MASK                    0x00000400
#define SYS_RST_32K_RC_TIME_MASK                    0x000003FF
// MON
#define SYS_MON_CPM(n)                              (0x0000 + n)
#define SYS_MON_TIMER0(n)                           (0x0100 + n)
#define SYS_MON_TIMER1(n)                           (0x0200 + n)
#define SYS_MON_TIMER2(n)                           (0x0300 + n)
#define SYS_MON_SFLASH(n)                           (0x0400 + n)
#define SYS_MON_QDEC(n)                             (0x0500 + n)
#define SYS_MON_EFUSE(n)                            (0x0600 + n)
#define SYS_MON_HS2P4(n)                            (0x0700 + n)
#define SYS_MON_WATCHDOG(n)                         (0x0800 + n)
#define SYS_MON_APB(n)                              (0x0900 + n)
#define SYS_MON_AHB(n)                              (0x0A00 + n)
#define SYS_MON_I2C1(n)                             (0x0B00 + n)
#define SYS_MON_UART1(n)                            (0x0C00 + n)
#define SYS_MON_BASEBAND(n)                         (0x0D00 + n)
#define SYS_MON_PHY(n)                              (0x0E00 + n)
#define SYS_MON_RGB(n)                              (0x0F00 + n)
#define SYS_MON_DAIF(n)                             (0x1000 + n)
#define SYS_MON_PMU(n)                              (0x1100 + n)
#define SYS_MON_SFLASH1(n)                          (0x1200 + n)
#define SYS_MON_RNG(n)                              (0x1300 + n)
#define SYS_MON_LCD(n)                              (0x1400 + n)
#define SYS_MON_AUDIO(n)                            (0x1500 + n)
#define SYS_MON_USB(n)                              (0x1600 + n)
#define SYS_MON_TRNG(n)                             (0x1700 + n)
#define SYS_MON_UART0(n)                            (0x1800 + n)
#define SYS_MON_UART2(n)                            (0x1900 + n)
#define SYS_MON_I2S(n)                              (0x1A00 + n)
#define SYS_MON_SPI0(n)                             (0x1B00 + n)
#define SYS_MON_SPI1(n)                             (0x1C00 + n)


// Simulation
#define SYS_SIMULATION_FLAG                         (*(__IO uint32_t *)0x40000c0c)
// USB_CTRL
#define SYS_USB_OTG_CID_EN_POS                      28
#define SYS_USB_OTG_CID_POS                         24
#define SYS_USB_VBUS_STATE_POS                      20
#define SYS_USB_PD_VBUS_DET_POS                     16
#define SYS_USB_BUS_ACTIVE_SYNC_POS                 12
#define SYS_USB_BUS_MON_EN_POS                      8
#define SYS_USB_SESS_CTRL_EN_POS                    7
#define SYS_USB_SESS_CTRL_POS                       4
#define SYS_USB_TEST_MODE_POS                       0
#define SYS_USB_OTG_CID_EN_MASK                     0x10000000
#define SYS_USB_OTG_CID_MASK                        0x01000000
#define SYS_USB_VBUS_STATE_MASK                     0x00700000
#define SYS_USB_PD_VBUS_DET_MASK                    0x00010000
#define SYS_USB_BUS_ACTIVE_SYNC_MASK                0x00001000
#define SYS_USB_BUS_MON_EN_MASK                     0x00000100
#define SYS_USB_SESS_CTRL_EN_MASK                   0x00000080
#define SYS_USB_SESS_CTRL_MASK                      0x00000070
#define SYS_USB_TEST_MODE_MASK                      0x00000001
// CHRGR_STAT
#define SYS_CHRGR_FINISH_MASK                       0x00000001
#define SYS_CHRGR_CHARGING_LARGE0_TRICKLE1_MASK     0x00000002
#define SYS_CHRGR_INSERT_DETECT_MASK                0x00000004
// PINMUX
#define SYS_PINMUX_MASK(f,p)                        (0xFF<<(p)),((f)<<(p))
#define SYS_PINMUX_MASK_POS(p)                      (0xFF<<(p)),(p)
// PINMUX0
#define SYS_PINMUX_GPIO_DIN_HOLD_SEL_MASK           0x00000080
#define SYS_PINMUX_SYSPLL_GT_CPUCLK_HW_CTRL_MASK    0x00800000


#endif  /* __SYS_REG_H */


/** @} */
