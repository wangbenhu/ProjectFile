/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup COMMON_REG COMMON_REG
 * @ingroup  DEVICE
 * @brief    COMMON register
 * @details  COMMON register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __COMMON_REG_H
#define __COMMON_REG_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
#ifndef _I
#ifdef __cplusplus
    #define   __I     volatile             /*!< Defines 'read only' permissions */
#else
    #define   __I     volatile const       /*!< Defines 'read only' permissions */
#endif
#endif

#define     __O     volatile             /*!< Defines 'write only' permissions */
#define     __IO    volatile             /*!< Defines 'read / write' permissions */


/* peripheral capabilities define */
// UART
#define CAP_UART_GPDMA_TX_POS                 (0U)
#define CAP_UART_GPDMA_TX_MASK                (1U << 0)
#define CAP_UART_GPDMA_RX_POS                 (1U)
#define CAP_UART_GPDMA_RX_MASK                (1U << 1)
#define CAP_UART_CTS_RTS_FLOW_CONTROL_POS     (2U)
#define CAP_UART_CTS_RTS_FLOW_CONTROL_MASK    (1U << 2)
#define CAP_UART_LIN_MODE_POS                 (3U)
#define CAP_UART_LIN_MODE_MASK                (1U << 3)
#define CAP_UART_FIFO_LEVEL_POS               (24U)
#define CAP_UART_FIFO_LEVEL_MASK              (0xFFU << 24)

// SPI
#define CAP_SPI_MASTER_MODE_POS               (0U)
#define CAP_SPI_MASTER_MODE_MASK              (1U << 0)
#define CAP_SPI_SLAVE_MODE_POS                (1U)
#define CAP_SPI_SLAVE_MODE_MASK               (1U << 1)
#define CAP_SPI_GPDMA_POS                     (2U)
#define CAP_SPI_GPDMA_MASK                    (1U << 2)
#define CAP_SPI_FIFO_LEVEL_POS                (24U)
#define CAP_SPI_FIFO_LEVEL_MASK               (0xFFU << 24)

// I2C
#define CAP_I2C_MASTER_MODE_POS               (0U)
#define CAP_I2C_MASTER_MODE_MASK              (1U << 0)
#define CAP_I2C_SLAVE_MODE_POS                (1U)
#define CAP_I2C_SLAVE_MODE_MASK               (1U << 1)
#define CAP_I2C_GPDMA_POS                     (2U)
#define CAP_I2C_GPDMA_MASK                    (1U << 2)
#define CAP_I2C_FIFO_LEVEL_POS                (24U)
#define CAP_I2C_FIFO_LEVEL_MASK               (0xFFU << 24)

// GPIO
#define CAP_GPIO_PIN_NUM_MAX_POS              (8U)
#define CAP_GPIO_PIN_NUM_MAX_MASK             (0xFF << 8)
#define CAP_GPIO_PIN_NUM_MIN_POS              (0U)
#define CAP_GPIO_PIN_NUM_MIN_MASK             (0xFF << 0)

// TIM
#define CAP_TIM_CAPTURE_POS                   (0)
#define CAP_TIM_CAPTURE_MASK                  (1U << 0)
#define CAP_TIM_PWM_POS                       (1)
#define CAP_TIM_PWM_MASK                      (1U << 1)
#define CAP_TIM_BDT_POS                       (2)
#define CAP_TIM_BDT_MASK                      (1U << 2)
#define CAP_TIM_GPDMA_POS                     (3)
#define CAP_TIM_GPDMA_MASK                    (1U << 3)

// EFUSE
#define CAP_EFUSE_SIZE_POS                    (0)
#define CAP_EFUSE_SIZE_MASK                   (0xFFFFU << 0)

// AES
#define CAP_AES_HW_SUPPORT_128BITS_POS        (0)
#define CAP_AES_HW_SUPPORT_128BITS_MASK       (1U << 0)
#define CAP_AES_HW_SUPPORT_192BITS_POS        (1)
#define CAP_AES_HW_SUPPORT_192BITS_MASK       (1U << 1)
#define CAP_AES_HW_SUPPORT_256BITS_POS        (2)
#define CAP_AES_HW_SUPPORT_256BITS_MASK       (1U << 2)

// FLASH
#define CAP_FLASH_XIP_POS                     (0U)
#define CAP_FLASH_XIP_MASK                    (1U << 0)

// LCD
#define CAP_LCD_INT_POS                       (0U)
#define CAP_LCD_INT_MASK                      (1U << 0)

// base address
#define OM_BUS_ROM_BASE                       0x00100000
#define OM_BUS_SRAM_BASE                      0x20000000
#define OM_BUS_SRAM_CODE_BASE                 0x00200000
#define OM_BUS_SF_CACHABLE_BASE               0x00400000
#define OM_BUS_SF_CACHABLE_BASE_1             0x00800000
#define OM_BUS_OTP_CACHABLE_BASE              0x00C00000
#define OM_BUS_SF_NONCACHABLE_BASE            0x50000000
#define OM_BUS_SF_NONCACHABLE_BASE_1          0x52000000
#define OM_BUS_OTP_NONCACHABLE_BASE           0x60000000
#define OM_BUS_EFUSE_NONCACHABLE_BASE         0x40002000 // @ref HS_EFUSE->READ_DATA
#define OM_BUS_EFUSE_FAKE_BASE                0x70000000 // only used for ISP
#define OM_BUS_SF_BASE                        OM_BUS_SF_CACHABLE_BASE
#define OM_BUS_SF_BASE_1                      OM_BUS_SF_CACHABLE_BASE_1
#define OM_BUS_OTP_BASE                       OM_BUS_OTP_CACHABLE_BASE
#define OM_BUS_BASE_MASK                      0xFFC00000
#define OM_BUS_OFFSET_MASK                    0x003FFFFF
#define OM_BUS_BASE(addr)                     ((addr) & OM_BUS_BASE_MASK)
#define OM_BUS_OFFSET(addr)                   ((addr) & OM_BUS_OFFSET_MASK)
#define OM_BUS_ROM_SIZE                       (16*1024)
#define OM_BUS_SRAM_SIZE                      (80*1024)
#define OM_BUS_OTP_SIZE                       (0*1024)


/*******************************************************************************
 * TYPEDEFS
 */


/*******************************************************************************
 * EXTERN VARIABLES
 */


/*******************************************************************************
 * EXTERN FUNCTIONS
 */


#ifdef __cplusplus
}
#endif

#endif  /* __COMMON_REG_H */


/** @} */
