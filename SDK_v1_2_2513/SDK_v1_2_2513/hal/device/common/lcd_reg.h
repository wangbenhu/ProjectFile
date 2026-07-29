/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup LCD LCD
 * @ingroup  DEVICE
 * @brief    LCD register
 * @details  LCD register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __LCD_REG_H
#define __LCD_REG_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include "common_reg.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */

// LCD interrupt status INTR_STATUS (offset 0x00)
#define LCD_INTR_STATUS_SPI_CMD_DONE_MASK              (1U << 0)
#define LCD_INTR_STATUS_SPI_CMD_DONE_POS               (0U)

// LCD raw interrupt status RAW_INTR_STATUS (offset 0x04)
#define LCD_RAW_INTR_STATUS_SPI_CMD_DONE_MASK          (1U << 0)
#define LCD_RAW_INTR_STATUS_SPI_CMD_DONE_POS           (0U)
#define LCD_LIST_INT_STATUS_LOAD_DONE_MASK             (1U << 4)
#define LCD_LIST_INT_STATUS_LOAD_DONE_POS              (4U)

// LCD interrupt mask INTR_MASK (offset 0x08)
#define LCD_INTR_MASK_SPI_CMD_DONE_MASK                (1U << 0)
#define LCD_INTR_MASK_SPI_CMD_DONE_POS                  0U
#define LCD_INTR_MASK_LIST_LOAD_DONE_MASK              (1U << 4)
#define LCD_INTR_MASK_LIST_LOAD_DONE_POS                4U

// LCD command register COMMAND (offset 0x0C)
#define LCD_COMMAND_CMD_POS                             0U
#define LCD_COMMAND_CMD_MASK                           (3U << 0)
#define LCD_COMMAND_CMD_NOP                             0U
#define LCD_COMMAND_CMD_READ                            1U
#define LCD_COMMAND_CMD_WRITE                           2U
#define LCD_COMMAND_CS_POS                              2U
#define LCD_COMMAND_CS_MASK                            (3U << 2)
#define LCD_COMMAND_DATA_2_LANE_EN_POS                  3U
#define LCD_COMMAND_DATA_2_LANE_EN_MASK                (1U << 3)
#define LCD_COMMAND_KEEP_CS_POS                         4U
#define LCD_COMMAND_KEEP_CS_MASK                       (1U << 4)
#define LCD_COMMAND_NUM_OF_CMD_BITS_POS                 5U
#define LCD_COMMAND_NUM_OF_CMD_BITS_MASK               (0x7F << 5)
#define LCD_COMMAND_NUM_OF_DATA_BYTES_POS               12U
#define LCD_COMMAND_NUM_OF_DATA_BYTES_MASK             (0xFFFF << 12)

// LCD command data0 register COMMAND_DATA0_REG (offset 0x10)
#define LCD_COMMAND_DATA0_CMD_POS                       24U
#define LCD_COMAMND_DATA0_ADDR_MASK                     0xFFFFFF

// LCD command data1 register COMMAND_DATA1_REG (offset 0x14)
// LCD read data0 register READ0_REG (offset 0x18)
// LCD read data0 register READ1_REG (offset 0x1C)
// LCD address register ADDRESS_REG (offset 0x20)

// LCD read opcode register READ_OPCODE_REG (offset 0x24)
#define LCD_READ_OPCODE_CS0_OPCODE_POS                  0
#define LCD_READ_OPCODE_CS0_OPCODE_MASK                (0xFF << 0)
#define LCD_READ_OPCODE_CS1_OPCODE_POS                  8
#define LCD_READ_OPCODE_CS1_OPCODE_MASK                (0xFF << 8)

// LCD configuration 0 and 1 register CONFIG[x].CTRL (offset 0x28, 0x30)
#define LCD_CONFIG_CLK_DIV_POS                          0U
#define LCD_CONFIG_CLK_DIV_MASK                        (0xFF << 0)
#define LCD_CONFIG_MODE_POS                             8U
#define LCD_CONFIG_MODE_MASK                           (3U << 8)
#define LCD_CONFIG_BP_CLK_DIV_POS                       10U
#define LCD_CONFIG_BP_CLK_DIV_MASK                     (1U << 10)
#define LCD_CONFIG_DLY_SMP_POS                          12U
#define LCD_CONFIG_DLY_SMP_MASK                        (7U << 12)
#define LCD_CONFIG_DATA_WIDTH_POS                       16U
#define LCD_CONFIG_DATA_WIDTH_MASK                     (3U << 16)
#define LCD_CONFIG_SPI_IF_MODE_POS                      18U
#define LCD_CONFIG_SPI_IF_MODE_MASK                    (7U << 18)
#define LCD_CONFIG_TX_DATA_MODE_POS                     21U
#define LCD_CONFIG_TX_DATA_MODE_MASK                   (3U << 21)

// LCD chip select 0 and 1 register CONFIG[x].CS (offset 0x2C, 0x34)
#define LCD_CONFIG_CS_POL_POS                           0
#define LCD_CONFIG_CS_POL_MASK                         (1U << 0)
#define LCD_CONFIG_CS_POL_ACT_LOW                       0U
#define LCD_CONFIG_CS_POL_ACT_HIGH                      1U
#define LCD_CONFIG_CS_IGNORE_CNT_POS                    1U
#define LCD_CONFIG_CS_IGNORE_CNT_MASK                  (0x7FU << 1)
#define LCD_CONFIG_CS_SETUP_TIME_POS                    8U
#define LCD_CONFIG_CS_SETUP_TIME_MASK                  (0xFFU << 8)
#define LCD_CONFIG_CS_HOLD_TIME_POS                     16U
#define LCD_CONFIG_CS_HOLD_TIME_MASK                   (0xFFU << 16)
#define LCD_CONFIG_CS_RECOVER_POS                       24U
#define LCD_CONFIG_CS_RECOVER_MASK                     (0xFFU << 24)

// LCD software configure register 0 SW_SPI_CFG0(offset 0x40)
#define LCD_SW_SPI_CFG0_P0_BIT_CNT_POS                  0
#define LCD_SW_SPI_CFG0_P0_BIT_CNT_MASK                (0x7FU << 0)
#define LCD_SW_SPI_CFG0_P0_BUS_WIDTH_POS                8
#define LCD_SW_SPI_CFG0_P0_BUS_WIDTH_MASK              (0x7U << 8)
#define LCD_SW_SPI_CFG0_P1_BUS_WIDTH_POS                12
#define LCD_SW_SPI_CFG0_P1_BUS_WIDTH_MASK              (0x7U << 12)
#define LCD_SW_SPI_CFG0_P1_BIT_CNT_POS                  16
#define LCD_SW_SPI_CFG0_P1_BIT_CNT_MASK                (0x7FU << 16)
#define LCD_SW_SPI_CFG0_DUMMY_CYCLE_POS                 24
#define LCD_SW_SPI_CFG0_DUMMY_CYCLE_MASK               (0x7FU << 24)

// LCD software configure register 1 SW_SPI_CFG1(offset 0x44)
#define LCD_SW_SPI_CFG1_SDATA_BYTE_CNT_POS              0
#define LCD_SW_SPI_CFG1_SDATA_BYTE_CNT_MASK            (0xFFFFFU << 0)
#define LCD_SW_SPI_CFG1_SDATA_BUS_WIDTH_POS             20
#define LCD_SW_SPI_CFG1_SDATA_BUS_WIDTH_MASK           (0x3U << 20)
#define LCD_SW_SPI_CFG1_BUF_WIDTH_BYTES_POS             24
#define LCD_SW_SPI_CFG1_BUF_WIDTH_BYTES_MASK           (0x7U << 24)
#define LCD_SW_SPI_CFG1_LIST_LOAD_EN_POS                30
#define LCD_SW_SPI_CFG1_LIST_LOAD_EN_MASK              (0x1U << 30)
#define LCD_SW_SPI_CFG1_CFG_EN_POS                      31
#define LCD_SW_SPI_CFG1_CFG_EN_MASK                    (0x1U << 31)
/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    __I  uint32_t INTR_STATUS;
    __IO uint32_t RAW_INTR_STATUS;
    __IO uint32_t INTR_MASK;
    __IO uint32_t COMMAND;
    __IO uint32_t COMMAND_DATA0_REG;
    __IO uint32_t COMMAND_DATA1_REG;
    __IO uint32_t READ0_REG;
    __IO uint32_t READ1_REG;
    __IO uint32_t ADDRESS_REG;
    __IO uint32_t READ_OPCODE_REG;
    struct {
        __IO uint32_t SPI_CFG;
        __IO uint32_t CS_CFG;
    } CONFIG[2];
    __IO uint32_t TRANS_REMAP_REG;
    __IO uint32_t WP_HOLD_REG;
    __IO uint32_t SW_SPI_CFG_0;
    __IO uint32_t SW_SPI_CFG_1;
    __IO uint32_t RSVD;
    __IO uint32_t DEBUG_REG;
    __IO uint32_t LIST_LOAD_ADDR_REG;
} OM_LCD_Type;


/*******************************************************************************
 * EXTERN VARIABLES
 */


/*******************************************************************************
 * EXTERN FUNCTIONS
 */


#ifdef __cplusplus
}
#endif


#endif  /* __LCD_REG_H */


/** @} */
