/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup DEVICE Device
 * @ingroup  HAL
 * @brief    OM_DEVICE device
 * @details  OM_DEVICE device apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __OM_DEVICE_H
#define __OM_DEVICE_H

#ifdef __cplusplus
extern "C"
{
#endif


/// @cond
/*******************************************************************************
 * Interrupt Number Definition
 */
typedef enum IRQn {
    /* Processor Exceptions Numbers */
    NonMaskableInt_IRQn         = -14,     /*  2 Non Maskable Interrupt */
    HardFault_IRQn              = -13,     /*  3 HardFault Interrupt */
    MemoryManagement_IRQn       = -12,     /*  4 Memory Management Interrupt */
    BusFault_IRQn               = -11,     /*  5 Bus Fault Interrupt */
    UsageFault_IRQn             = -10,     /*  6 Usage Fault Interrupt */
    SVCall_IRQn                 =  -5,     /* 11 SV Call Interrupt */
    DebugMonitor_IRQn           =  -4,     /* 12 Debug Monitor Interrupt */
    PendSV_IRQn                 =  -2,     /* 14 Pend SV Interrupt */
    SysTick_IRQn                =  -1,     /* 15 System Tick Interrupt */

    /* Processor Interrupt Numbers */
    BLE_IRQn                    = 0,
    BLE_WAKEUP_IRQn             = 1,
    GPDMA_IRQn                  = 2,
    PMU_PIN_WAKEUP_IRQn         = 3,
    TIM_COMB_IRQn               = 4,
    OM24G_RF_IRQn               = 5,
    PMU_POF_IRQn                = 6,
    PMU_TIMER_IRQn              = 7,
    WDT_IRQn                    = 8,
    UART1_IRQn                  = 9,
    EFUSE_IRQn                  = 10,
    GPIO_IRQn                   = 11,
    GPADC_IRQn                  = 12,
    I2C0_IRQn                   = 13,
    SF0_IRQn                    = 14,
    SOFT0_IRQn                  = 15,
    SOFT1_IRQn                  = 16,
    SOFT2_IRQn                  = 17,
    SOFT3_IRQn                  = 18,
    SOFT4_IRQn                  = 19,
    SOFT5_IRQn                  = 20,
    SOFT6_IRQn                  = 21,
    SOFT7_IRQn                  = 22,
    CRY32M_RDY_IRQn             = 24,
    UART0_IRQn                  = 25,
    SPI0_IRQn                   = 26,
    TIM0_IRQn                   = 28,
    TIM1_IRQn                   = 29,
    TIM2_IRQn                   = 30,
    SPI1_IRQn                   = 31,
    RTC_SECOND_IRQn             = 32,
    RTC_ALARM_IRQn              = 33,
    LPTIM_IRQn                  = 34,
    CRY32M_DECT_IRQn            = 35,
    I2S_IRQn                    = 36,
    UART2_IRQn                  = 37,
    OM24G_TIM_WAKEUP_IRQn       = 38,
    RGB_IRQn                    = 39,
    I2C1_IRQn                   = 40,
    SHA256_IRQn                 = 41,
    USB_IRQn                    = 42,
    USB_BUSACT_IRQn             = 43,
    USB_DMA_IRQn                = 44,
    IRTX_IRQn                   = 45,
    QDEC_IRQn                   = 46,
    OSPI1_IRQn                  = 47,
    LCD_IRQn                    = 48,
    AUDIO_IRQn                  = 49,

    /* Number of IRQ */
    EXTERNAL_IRQn_Num,
} IRQn_Type;

/// Number of DMA channels
#define GPDMA_NUMBER_OF_CHANNELS          (8U)
/// GPDMA Pheripheral Index
typedef enum {
    GPDMA_ID_UART2_TX     = 1,
    GPDMA_ID_UART2_RX     = 2,
    GPDMA_ID_UART0_TX     = 3,
    GPDMA_ID_UART0_RX     = 4,
    GPDMA_ID_UART1_TX     = 5,
    GPDMA_ID_UART1_RX     = 6,
    GPDMA_ID_I2C0_TX      = 7,
    GPDMA_ID_I2C0_RX      = 8,
    GPDMA_ID_I2C1_TX      = 9,
    GPDMA_ID_I2C1_RX      = 10,
    GPDMA_ID_SPI0_TX      = 11,
    GPDMA_ID_SPI0_RX      = 12,
    GPDMA_ID_SPI1_TX      = 13,
    GPDMA_ID_SPI1_RX      = 14,
    GPDMA_ID_TIM0         = 15,
    GPDMA_ID_TIM1         = 16,
    GPDMA_ID_TIM2         = 17,
    GPDMA_ID_I2S_TX       = 18,
    GPDMA_ID_I2S_RX       = 19,
    GPDMA_ID_GPADC        = 20,
    GPDMA_ID_RGB          = 21,

    /// Extended for identifying memory. Note that memory have no req/ack for DMA.
    GPDMA_ID_MEM          = 0xFE,

    GPDMA_ID_INVAILD      = 0xFF,
    GPDMA_ID_MAX          = 0xFF,
} gpdma_id_t;
/// @endcond

/* Start of section using anonymous unions and disabling warnings */
#if   defined (__CC_ARM)
    #pragma push
    #pragma anon_unions
#elif defined (__ICCARM__)
    #pragma language=extended
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wc11-extensions"
    #pragma clang diagnostic ignored "-Wreserved-id-macro"
#elif defined (__GNUC__)
    /* anonymous unions are enabled by default */
#elif defined (__TMS470__)
    /* anonymous unions are enabled by default */
#elif defined (__TASKING__)
    #pragma warning 586
#elif defined (__CSMC__)
    /* anonymous unions are enabled by default */
#else
    #warning Not supported compiler type
#endif


/// @cond
/* Configuration of Core Peripherals */
#define __CHECK_DEVICE_DEFINES    1
#define __CM4_REV                 0x0001    /*!< CM4 r0p1 */
#define __FPU_PRESENT             1U        /* FPU present */
#define __MPU_PRESENT             1U        /* MPU present */
#define __NVIC_PRIO_BITS          4U        /* Number of Bits used for Priority Levels */
#define __Vendor_SysTickConfig    0U        /* Set to 1 if different SysTick Config is used */
#define __VTOR_PRESENT            1U        /* VTOR present */
/// @endcond


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "core_cm4.h"

#include "../common/common_reg.h"
#include "../common/cm4_reg.h"
#include "../common/gpadc_reg.h"
#include "../common/cpm_reg.h"
#include "../common/pmu_reg.h"
#include "../common/daif_reg.h"

#include "../common/gpio_reg.h"
#include "../common/i2c_reg.h"
#include "../common/spi_reg.h"
#include "../common/timer_reg.h"
#include "../common/uart_reg.h"
#include "../common/om24g_reg.h"
#include "../common/efuse_reg.h"
#include "../common/rtc_reg.h"
#include "../common/btphy_reg.h"
#include "../common/btmac_reg.h"
#include "../common/aes_hw_reg.h"
#include "../common/sf_reg.h"
#include "../common/ospi_reg.h"
#include "../common/cache_reg.h"
#include "../common/gpdma_reg.h"
#include "../common/sys_reg.h"
#include "../common/rng_reg.h"
#include "../common/trng_reg.h"
#include "../common/lptim_reg.h"
#include "../common/i2s_reg.h"
#include "../common/audio_reg.h"
#include "../common/lcd_reg.h"
#include "../common/irtx_reg.h"
#include "../common/sha256_reg.h"
#include "../common/usb_reg.h"
#include "../common/qdec_reg.h"
#include "../common/rgb_reg.h"


/// @cond
/*******************************************************************************
 * MACROS
 */
/* Memory base address */
#define OM_MEM_ROM_BASE                0x00100000U
#define OM_MEM_RAM_BASE                0x00200000U
#if (OM6627X)
#define OM_MEM_RAM_SIZE                (128*1024U)
#else
#define OM_MEM_RAM_SIZE                (256*1024U)
#endif
#define OM_MEM_FLASH0_NONCACHED_BASE   0x50000000U
#define OM_MEM_FLASH0_CACHED_BASE      0x00400000U
#define OM_MEM_FLASH1_NONCACHED_BASE   0x60000000U
#define OM_MEM_FLASH1_CACHED_BASE      0x08000000U

/* System Peripheral Base address */
#define OM_SYS_BASE                 0x40000000U
#define OM_CPM_BASE                 0x40001000U
#define OM_PMU_BASE                 0x400e0000U
#define OM_DAIF_BASE                0x400a0000U

/* Application Core Peripheral Base address */
#define OM_EFUSE_BASE               0x40002000U
#define OM_LPTIM_BASE               0x40003000U
#define OM_RNG_BASE                 0x40004000U
#define OM_SPI0_BASE                0x40005000U
#define OM_SPI1_BASE                0x40006000U
#define OM_I2C0_BASE                0x40008000U
#define OM_TRNG_BASE                0x40007000U
#define OM_I2C1_BASE                0x40009000U
#define OM_AUDIO_BASE               0x4000A000U
#define OM_24G_BASE                 0x4000B000U
#define OM_RTC_BASE                 0x4000C000U
#define OM_RGB_BASE                 0x4000D000U
#define OM_UART2_BASE               0x4000E000U
#define OM_QDEC_BASE                0x4000F000U
#define OM_IRTX_BASE                0x40010000U
#define OM_SHA256_BASE              0x40011000U
#define OM_PHY_BASE                 0x40020000U
#define OM_UART0_BASE               0x40040000U
#define OM_UART1_BASE               0x40080000U
#define OM_TIM0_BASE                0x400c0000U
#define OM_TIM1_BASE                0x400c0100U
#define OM_TIM2_BASE                0x400c0200U
#define OM_GPIO0_BASE               0x41200000U
#define OM_GPIO1_BASE               0x41202000U
#define OM_BB_BASE                  0x41300000U
#define OM_PLLRAM_BASE              0x41400000U
#define OM_USB_BASE                 0x41500000U
#define OM_I2S_BASE                 0x41600000U

#define OM_BT_BASE                  0x41300000U
#define OM_AES_BASE                 0x41308000U
#define OM_ICACHE_BASE              0xE0042000U
#define OM_GPDMA_BASE               0x41100000U
#define OM_GPADC_BASE               0x400A2000U
#define OM_SF0_BASE                 0x51000000U
#define OM_OSPI1_BASE               0x53000000U
#define OM_LCD_BASE                 0x55000000U

#define COREDEBUG                   ((COREDEBUG_Type *) DCB_BASE)
/* System Peripheral Type definition */
#define OM_SYS                      ((OM_SYS_Type *)        OM_SYS_BASE)
#define OM_CPM                      ((OM_CPM_Type *)        OM_CPM_BASE)
#define OM_PMU                      ((OM_PMU_Type *)        OM_PMU_BASE)
#define OM_DAIF                     ((OM_DAIF_Type *)       OM_DAIF_BASE)

/* Application Core Peripheral Type definition */
#define OM_EFUSE                    ((OM_EFUSE_Type *)      OM_EFUSE_BASE)
#define OM_LPTIM                    ((OM_LPTIM_Type *)      OM_LPTIM_BASE)
#define OM_RNG                      ((OM_RNG_Type *)        OM_RNG_BASE)
#define OM_TRNG                     ((OM_TRNG_Type *)       OM_TRNG_BASE)
#define OM_SPI0                     ((OM_SPI_Type *)        OM_SPI0_BASE)
#define OM_SPI1                     ((OM_SPI_Type *)        OM_SPI1_BASE)
#define OM_I2C0                     ((OM_I2C_Type *)        OM_I2C0_BASE)
#define OM_I2C1                     ((OM_I2C_Type *)        OM_I2C1_BASE)
#define OM_AUDIO                    ((OM_AUDIO_Type *)      OM_AUDIO_BASE)
#define OM_24G                      ((OM_24G_Type *)        OM_24G_BASE)
#define OM_RTC                      ((OM_RTC_Type *)        OM_RTC_BASE)
#define OM_UART2                    ((OM_UART_Type *)       OM_UART2_BASE)
#define OM_QDEC                     ((OM_QDEC_Type *)       OM_QDEC_BASE)
#define OM_IRTX                     ((OM_IRTX_Type *)       OM_IRTX_BASE)
#define OM_SHA256                   ((OM_SHA256_Type *)     OM_SHA256_BASE)
#define OM_PHY                      ((OM_BTPHY_Type *)      OM_PHY_BASE)
#define OM_UART0                    ((OM_UART_Type *)       OM_UART0_BASE)
#define OM_UART1                    ((OM_UART_Type *)       OM_UART1_BASE)
#define OM_TIM0                     ((OM_TIM_Type *)        OM_TIM0_BASE)
#define OM_TIM1                     ((OM_TIM_Type *)        OM_TIM1_BASE)
#define OM_TIM2                     ((OM_TIM_Type *)        OM_TIM2_BASE)
#define OM_GPIO0                    ((OM_GPIO_Type *)       OM_GPIO0_BASE)
#define OM_GPIO1                    ((OM_GPIO_Type *)       OM_GPIO1_BASE)
#define OM_BT                       ((OM_BTMAC_Type *)      OM_BT_BASE)
#define OM_AES                      ((OM_AES_HW_Type *)     OM_AES_BASE)
#define OM_I2S                      ((OM_I2S_Type *)        OM_I2S_BASE)
#define OM_USB                      ((OM_USB_Type *)        OM_USB_BASE)
#define OM_ICACHE                   ((OM_ICACHE_Type *)     OM_ICACHE_BASE)
#define OM_GPDMA                    ((OM_GPDMA_Type *)      OM_GPDMA_BASE)
#define OM_GPADC                    ((OM_GPADC_Type *)      OM_GPADC_BASE)
#define OM_SF0                      ((OM_SF_Type *)         OM_SF0_BASE)
#define OM_OSPI1                    ((OM_OSPI_Type *)       OM_OSPI1_BASE)
#define OM_LCD                      ((OM_LCD_Type *)        OM_LCD_BASE)
#define OM_RGB                      ((OM_RGB_Type *)        OM_RGB_BASE)

/* Chip capabilities */
#define CAP_RAM_SIZE                (256U * 1024)               /* Unit: KiB */
#define CAP_ROM_SIZE                (32U * 1024)                /* Unit: KiB */
#define CAP_DIG_PAD_NUM              39U

/* peripheral capabilities define */
#define CAP_UART0         ( (1U << CAP_UART_GPDMA_TX_POS)                      \
                          | (1U << CAP_UART_GPDMA_RX_POS)                      \
                          | (0U << CAP_UART_CTS_RTS_FLOW_CONTROL_POS)          \
                          | (0U << CAP_UART_LIN_MODE_POS))

#define CAP_UART1         ( (1U << CAP_UART_GPDMA_TX_POS)                      \
                          | (1U << CAP_UART_GPDMA_RX_POS)                      \
                          | (1U << CAP_UART_CTS_RTS_FLOW_CONTROL_POS)          \
                          | (0U << CAP_UART_LIN_MODE_POS))

#define CAP_UART2         ( (1U << CAP_UART_GPDMA_TX_POS)                      \
                          | (1U << CAP_UART_GPDMA_RX_POS)                      \
                          | (1U << CAP_UART_CTS_RTS_FLOW_CONTROL_POS)          \
                          | (1U << CAP_UART_LIN_MODE_POS))

#define CAP_SPI0          ( (1U << CAP_SPI_MASTER_MODE_POS)                    \
                          | (1U << CAP_SPI_SLAVE_MODE_POS)                     \
                          | (1U << CAP_SPI_GPDMA_POS)                          \
                          | (0x20U << CAP_SPI_FIFO_LEVEL_POS))

#define CAP_SPI1          ( (1U << CAP_SPI_MASTER_MODE_POS)                    \
                          | (1U << CAP_SPI_SLAVE_MODE_POS)                     \
                          | (1U << CAP_SPI_GPDMA_POS)                          \
                          | (0x20U << CAP_SPI_FIFO_LEVEL_POS))

#define CAP_I2C0          ( (1U << CAP_I2C_MASTER_MODE_POS)                    \
                          | (0U << CAP_I2C_SLAVE_MODE_POS)                     \
                          | (1U << CAP_I2C_GPDMA_POS)                          \
                          | (0x10U << CAP_I2C_FIFO_LEVEL_POS))

#define CAP_I2C1          ( (1U << CAP_I2C_MASTER_MODE_POS)                    \
                          | (0U << CAP_I2C_SLAVE_MODE_POS)                     \
                          | (1U << CAP_I2C_GPDMA_POS)                          \
                          | (0x10U << CAP_I2C_FIFO_LEVEL_POS))

#define CAP_GPIO0         0U
#define CAP_GPIO1         0U
#define CAP_TIM0          ( (1U << CAP_TIM_CAPTURE_POS)                        \
                          | (1U << CAP_TIM_PWM_POS)                            \
                          | (1U << CAP_TIM_BDT_POS)                            \
                          | (1U << CAP_TIM_GPDMA_POS))

#define CAP_TIM1          ( (0U << CAP_TIM_CAPTURE_POS)                        \
                          | (1U << CAP_TIM_PWM_POS)                            \
                          | (1U << CAP_TIM_BDT_POS)                            \
                          | (1U << CAP_TIM_GPDMA_POS))

#define CAP_TIM2          ( (0U << CAP_TIM_CAPTURE_POS)                        \
                          | (1U << CAP_TIM_PWM_POS)                            \
                          | (1U << CAP_TIM_BDT_POS)                            \
                          | (1U << CAP_TIM_GPDMA_POS))

#define CAP_EFUSE         ( (64U << CAP_EFUSE_SIZE_POS) )

#define CAP_IRTX          0U

#define CAP_AES0          ( (1U << CAP_AES_HW_SUPPORT_128BITS_POS)             \
                          | (0U << CAP_AES_HW_SUPPORT_192BITS_POS)             \
                          | (1U << CAP_AES_HW_SUPPORT_256BITS_POS))            \

#define CAP_LCD           ( (1U << CAP_LCD_INT_POS))

#define CAP_QDEC          0U
/// @endcond


/* End of section using anonymous unions and disabling warnings */
#if   defined (__CC_ARM)
  #pragma pop
#elif defined (__ICCARM__)
  /* leave anonymous unions enabled */
#elif (defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050))
  #pragma clang diagnostic pop
#elif defined (__GNUC__)
  /* anonymous unions are enabled by default */
#elif defined (__TMS470__)
  /* anonymous unions are enabled by default */
#elif defined (__TASKING__)
  #pragma warning restore
#elif defined (__CSMC__)
  /* anonymous unions are enabled by default */
#else
  #warning Not supported compiler type
#endif


#ifdef __cplusplus
}
#endif

#endif  /* __OM_DEVICE_H */


/** @} */
