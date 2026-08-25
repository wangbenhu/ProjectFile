/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup SF SF
 * @ingroup  DEVICE
 * @brief    SF register
 * @details  SF register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __SF_REG_H
#define __SF_REG_H


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
/*!< offset:0x00  Interrupt Status register                 */
#define SF_INT_STATUS_CMD_DONE_POS              0U             /*!< Command transfer done interrupt status */
#define SF_INT_STATUS_CMD_DONE_MASK             (0x1U << 0)

/*!< offset:0x04  Interrupt Raw Status register             */
#define SF_RAW_INT_STATUS_CMD_DONE_POS          0U             /*!< Command transfer done raw interrupt status */
#define SF_RAW_INT_STATUS_CMD_DONE_MASK         (0x1U << 0)

/*!< offset:0x08  Interrupt Enable register                 */
#define SF_INT_EN_CMD_DONE_POS                  0U             /*!< Command transfer done interrupt enable */
#define SF_INT_EN_CMD_DONE_MASK                 (0x1U << 0)

/*!< offset:0x0C  Command register                          */
#define SF_COMMAND_RW_SELECT_POS                0U             /*!< Read or Write select */
#define SF_COMMAND_RW_SELECT_MASK               (0x3U << 0)
#define SF_COMMAND_CHIP_SELECT_POS              2U             /*!< Chip select 0/1, only 0 is used */
#define SF_COMMAND_CHIP_SELECT_MASK             (0x1U << 2)
#define SF_COMMAND_KEEP_CS_POS                  4U             /*!< CS remain asserted after command finished */
#define SF_COMMAND_KEEP_CS_MASK                 (0x1U << 4)
#define SF_COMMAND_CMD_BITS_POS                 5U             /*!< Command bits to send, legacy configuration */
#define SF_COMMAND_CMD_BITS_MASK                (0x7FU << 5)
#define SF_COMMAND_DATA_BYTES_POS               12U            /*!< Data bytes to send, legacy configuration */
#define SF_COMMAND_DATA_BYTES_MASK              (0xFFFFFU << 12)

/*!< offset:0x10  Command Data 0 register                   */
#define SF_COMMAND_DATA0_CMD_OPCODE_POS         24U             /*!< Command phase(P0) data 0 to transfer */
#define SF_COMMAND_DATA0_CMD_OPCODE_MASK        (0xFFU << 24)

/*!< offset:0x14  Command Data 1 register                   */
#define SF_COMMAND_DATA1_COMMAND_DATA_POS       0U             /*!< Command phase(P0) data 1 to transfer */
#define SF_COMMAND_DATA1_COMMAND_DATA_MASK      (0xFFFFFFFFU << 0)

/*!< offset:0x18  Read Data 0 register                      */
#define SF_READ_DATA0_READ_DATA_POS             0U             /*!< Command phase(P0) read data 0 to transfer */
#define SF_READ_DATA0_READ_DATA_MASK            (0xFFFFFFFFU << 0)

/*!< offset:0x1C  Read Data 1 register                      */
#define SF_READ_DATA1_READ_DATA_POS             0U             /*!< Address phase(P1) flash address to transfer */
#define SF_READ_DATA1_READ_DATA_MASK            (0xFFFFFFFFU << 0)

/*!< offset:0x20  System data address register              */
#define SF_ADDRESS_ADDRESS_POS                  0U             /*!< System data address to transfer */
#define SF_ADDRESS_ADDRESS_MASK                 (0xFFFFFFFFU << 0)

/*!< offset:0x24  Read Opcode register (Auto Restore)       */
#define SF_READ_OPCODE_CS0_OPCODE_POS           0U             /*!< Not used */
#define SF_READ_OPCODE_CS0_OPCODE_MASK          (0xFFU << 0)
#define SF_READ_OPCODE_CS1_OPCODE_POS           8U             /*!< Not used */
#define SF_READ_OPCODE_CS1_OPCODE_MASK          (0xFFU << 8)

/*!< offset:0x28  Configuration register (Auto Restore)     */
#define SF_CONFIG_CLK_DIV_POS                   0U             /*!< Clock division */
#define SF_CONFIG_CLK_DIV_MASK                  (0xFFU << 0)
#define SF_CONFIG_SPI_MODE_POS                  8U             /*!< Standard SPI mode */
#define SF_CONFIG_SPI_MODE_MASK                 (0x3U << 8)
#define SF_CONFIG_CLK_BYPASS_POS                10U            /*!< Clock division bypass */
#define SF_CONFIG_CLK_BYPASS_MASK               (0x1U << 10)
#define SF_CONFIG_DATA_UNIT_POS                 16U            /*!< Data transfer unit */
#define SF_CONFIG_DATA_UNIT_MASK                (0x3U << 16)

/*!< offset:0x2C  CS Configuration register                 */
#define SF_CS_CONFIG_POL_POS                    0U             /*!< CS active polarity */
#define SF_CS_CONFIG_POL_MASK                   (0x1U << 0)
#define SF_CS_CONFIG_SETUP_POS                  8U             /*!< CS Setup time, ticks between CS assertion to first clock pulse */
#define SF_CS_CONFIG_SETUP_MASK                 (0xFU << 8)
#define SF_CS_CONFIG_HOLD_POS                   16U            /*!< CS Hold time, ticks between last clock pulse to CS de-assertion */
#define SF_CS_CONFIG_HOLD_MASK                  (0xFU << 16)
#define SF_CS_CONFIG_RECOVER_POS                24U            /*!< CS Recover time, ticks between de-assertion and next assertion */
#define SF_CS_CONFIG_RECOVER_MASK               (0xFU << 24)

/*!< offset:0x38  Transparent Remap register                */
#define SF_TRANS_REMAP_REMAP_BASE_POS           0U             /*!< Transparent read address = system bus address | REMAP_BASE */
#define SF_TRANS_REMAP_REMAP_BASE_MASK          (0xFFFFFFU << 0)

/*!< offset:0x3C  Write Protect and Hold register           */
#define SF_WP_HOLD_WP_POS                       0U             /*!< WP output */
#define SF_WP_HOLD_WP_MASK                      (0x1U << 0)
#define SF_WP_HOLD_HOLD_POS                     1U             /*!< HOLD output */
#define SF_WP_HOLD_HOLD_MASK                    (0x1U << 1)
#define SF_WP_HOLD_WP_EN_POS                    2U             /*!< WP output controlled by register enable */
#define SF_WP_HOLD_WP_EN_MASK                   (0x1U << 2)
#define SF_WP_HOLD_HOLD_EN_POS                  3U             /*!< HOLD output controlled by register enable */
#define SF_WP_HOLD_HOLD_EN_MASK                 (0x1U << 3)

/*!< offset:0x40  SPI Format Configuration 0 register       */
#define SF_SW_SPI_CFG0_P0_BITS_POS              0U             /*!< Phase 0 bits */
#define SF_SW_SPI_CFG0_P0_BITS_MASK             (0x7FU << 0)
#define SF_SW_SPI_CFG0_P0_WIDTH_POS             8U             /*!< Phase 0 width */
#define SF_SW_SPI_CFG0_P0_WIDTH_MASK            (0x3U << 8)
#define SF_SW_SPI_CFG0_P1_WIDTH_POS             12U            /*!< Phase 1 width */
#define SF_SW_SPI_CFG0_P1_WIDTH_MASK            (0x3U << 12)
#define SF_SW_SPI_CFG0_P1_BITS_POS              16U            /*!< Phase 1 bits */
#define SF_SW_SPI_CFG0_P1_BITS_MASK             (0x7FU << 16)
#define SF_SW_SPI_CFG0_DUMMY_CNT_POS            24U            /*!< Dummy cycle counts */
#define SF_SW_SPI_CFG0_DUMMY_CNT_MASK           (0x7FU << 24)

/*!< offset:0x44  SPI Format Configuration 1 register       */
#define SF_SW_SPI_CFG1_DATA_BYTES_POS           0U             /*!< Data bytes */
#define SF_SW_SPI_CFG1_DATA_BYTES_MASK          (0xFFFFFU << 0)
#define SF_SW_SPI_CFG1_DATA_WIDTH_POS           20U            /*!< Data width */
#define SF_SW_SPI_CFG1_DATA_WIDTH_MASK          (0x3U << 20)
#define SF_SW_SPI_CFG1_BUF_BYTES_POS            24U            /*!< Buffer bytes */
#define SF_SW_SPI_CFG1_BUF_BYTES_MASK           (0x7U << 24)
#define SF_SW_SPI_CFG1_SW_CFG_EN_POS            31U            /*!< Use SW_CFG for format control */
#define SF_SW_SPI_CFG1_SW_CFG_EN_MASK           (0x1U << 31)

/*!< offset:0x4C  Debug register                            */
#define SF_DBG_IDX_POS                          0U            /*!< Debug index */
#define SF_DBG_IDX_MASK                         (0xFU << 0)
#define SF_DBG_EN_POS                           4U            /*!< Debug enable */
#define SF_DBG_EN_MASK                          (0x1U << 4)
#define SF_DBG_DATA_POS                         16U           /*!< Debug data read */
#define SF_DBG_DATA_MASK                        (0xFFU << 16)

/*!< offset:0x50  Delay control register (Auto Restore)     */
#define SF_DELAY_CTRL_DLY_POS                   0U            /*!< Delay between sample clock to internal SCK pad */
#define SF_DELAY_CTRL_DLY_MASK                  (0xFU << 0)

/*!< offset:0x54  Encrypt control register (Auto Restore)   */
#define SF_ENCRYPT_CTRL_ENCRYPT_EN_POS          0U            /*!< Encryption enable */
#define SF_ENCRYPT_CTRL_ENCRYPT_EN_MASK         (0x1U << 0)
#define SF_ENCRYPT_CTRL_DECRYPT_EN_POS          1U            /*!< Decryption enable */
#define SF_ENCRYPT_CTRL_DECRYPT_EN_MASK         (0x1U << 1)


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    __I  uint32_t INT_STATUS;               /*!< offset:0x00  Interrupt Status register                 */
    __IO uint32_t RAW_INT_STATUS;           /*!< offset:0x04  Interrupt Raw Status register             */
    __IO uint32_t INT_EN;                   /*!< offset:0x08  Interrupt Enable register                 */
    __IO uint32_t CMD;                      /*!< offset:0x0C  Command register                          */
    __IO uint32_t CMD_DATA0;                /*!< offset:0x10  Command Data 0 register                   */
    __IO uint32_t CMD_DATA1;                /*!< offset:0x14  Command Data 1 register                   */
    __IO uint32_t RD_DATA0;                 /*!< offset:0x18  Read Data 0 register                      */
    __IO uint32_t RD_DATA1;                 /*!< offset:0x1C  Read Data 1 register                      */
    __IO uint32_t ADDR;                     /*!< offset:0x20  System data address register              */
    __IO uint32_t OPCODE;                   /*!< offset:0x24  Read Opcode register (Auto Restore)       */
    struct {
        __IO uint32_t SPI_CFG;              /*!< offset:0x28  Configuration register (Auto Restore)     */
        __IO uint32_t CS_CFG;               /*!< offset:0x2C  CS Configuration register                 */
    } CFG[2];
    __IO uint32_t TRANS_REMAP;              /*!< offset:0x38  Transparent Remap register                */
    __IO uint32_t WP_HOLD;                  /*!< offset:0x3C  Write Protect and Hold register           */
    __IO uint32_t SW_CFG0;                  /*!< offset:0x40  SPI Format Configuration 0 register       */
    __IO uint32_t SW_CFG1;                  /*!< offset:0x44  SPI Format Configuration 1 register       */
         uint32_t RES0;                     /*!< offset:0x48                                            */
    __IO uint32_t DBG;                      /*!< offset:0x4C  Debug register                            */
    __IO uint32_t DELAY_CTRL;               /*!< offset:0x50  Delay control register (Auto Restore)     */
    __IO uint32_t ENCRYPT_CTRL;             /*!< offset:0x54  Encrypt control register (Auto Restore)   */
} OM_SF_Type;

#ifdef __cplusplus
}
#endif

#endif  /* __SF_REG_H */


/** @} */
