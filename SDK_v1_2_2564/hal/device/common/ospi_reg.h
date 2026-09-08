/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup OSPI OSPI
 * @ingroup  DEVICE
 * @brief    OSPI Register for OnMicro OM66xx
 * @details  common OSPI Register definitions
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __OSPI_REG_H
#define __OSPI_REG_H


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
// OSPI interrupt status register OSPI_IS(offset 0x00)
#define OSPI_INT_STATUS_SCD_POS                 0
#define OSPI_INT_STATUS_SCD_MASK                (0x1U << 0)          /* command done interrupt */

// OSPI raw interrupt status register RIS(offset 0x04)
#define OSPI_RAW_INT_STATUS_SRIS_POS            0
#define OSPI_RAW_INT_STATUS_SRIS_MASK           (0x1U << 0)          /* command done raw interrupt */
#define OSPI_RAW_INT_STATUS_LLI_POS             4
#define OSPI_RAW_INT_STATUS_LLI_MASK            (0x1U << 4)          /* linked list interrupt mask */

// OSPI interrupt mask register IM(offset 0x08)
#define OSPI_INT_MASK_CDM_POS                   0
#define OSPI_INT_MASK_CDM_MASK                  (0x1U << 0)          /* command done interrupt mask */
#define OSPI_INT_MASK_LLIM_POS                  4
#define OSPI_INT_MASK_LLIM_MASK                 (0x1U << 4)          /* linked list interrupt mask */

// OSPI command register CMD(offset 0x0C)
#define OSPI_CMD_COMMAND_POS                    0
#define OSPI_CMD_COMMAND_MASK                   (0x3U << 0)          /* command, 1 for read and 2 for write */
#define OSPI_CMD_CS_POS                         2
#define OSPI_CMD_CS_MASK                        (0x1U << 2)          /* chip select */
#define OSPI_CMD_KCS_POS                        4
#define OSPI_CMD_KCS_MASK                       (0x1U << 4)          /* keep chip select */
#define OSPI_CMD_CMD_BITS_POS                   5
#define OSPI_CMD_CMD_BITS_MASK                  (0x7FU << 5)         /* command bit number */
#define OSPI_CMD_DATA_BYTES_POS                 12
#define OSPI_CMD_DATA_BYTES_MASK                (0xFFFFFU << 12)     /* data byte number */

// OSPI opcode register OPCODE(offset 0x24)
#define OSPI_OPCODE_RD0_POS                     0
#define OSPI_OPCODE_RD0_MASK                    (0xFFU << 0)         /* read opcode0 */
#define OSPI_OPCODE_RD1_POS                     8
#define OSPI_OPCODE_RD1_MASK                    (0xFFU << 8)         /* read opcode1 */
#define OSPI_OPCODE_WR0_POS                     16
#define OSPI_OPCODE_WR0_MASK                    (0xFFU << 16)        /* write opcode0 */
#define OSPI_OPCODE_WR1_POS                     24
#define OSPI_OPCODE_WR1_MASK                    (0xFFU << 24)        /* write opcode1 */

// OSPI configuration 0/1 register CFG0(offset 0x28/0x30)
#define OSPI_CFG_CLK_DIV_POS                    0
#define OSPI_CFG_CLK_DIV_MASK                   (0xFFU << 0)         /* clock divider */
#define OSPI_CFG_CPHA_POS                       8
#define OSPI_CFG_CPHA_MASK                      (0x1U << 8)          /* clock phase */
#define OSPI_CFG_CPOL_POS                       9
#define OSPI_CFG_CPOL_MASK                      (0x1U << 9)          /* clock polarity */
#define OSPI_CFG_BPC_POS                        10
#define OSPI_CFG_BPC_MASK                       (0x1U << 10)         /* bypass clock divider */
#define OSPI_CFG_DLY_COMP_POS                   12
#define OSPI_CFG_DLY_COMP_MASK                  (0x7U << 12)         /* delayed sampling */
#define OSPI_CFG_DLY_POS                        12
#define OSPI_CFG_DLY_MASK                       (0x3U << 12)         /* delayed sampling */
#define OSPI_CFG_FDS_POS                        14
#define OSPI_CFG_FDS_MASK                       (0x1U << 14)         /* falling edge delayed sampling */
#define OSPI_CFG_DDM_POS                        15
#define OSPI_CFG_DDM_MASK                       (0x1U << 15)         /* dual device mode */
#define OSPI_CFG_WIDTH_POS                      16
#define OSPI_CFG_WIDTH_MASK                     (0x3U << 16)         /* data width */
#define OSPI_CFG_SDR_ASYNC_EN_POS               18
#define OSPI_CFG_SDR_ASYNC_EN_MASK              (0x1U << 18)         /* sdr async enable */
#define OSPI_CFG_SDR_ASYNC_DLY_POS              20
#define OSPI_CFG_SDR_ASYNC_DLY_MASK             (0xFU << 20)         /* sdr async delay */

// OSPI CS configuration 0/1 register CSCFG0(offset 0x2C/0x34)
#define OSPI_CS_CFG_CSP_POS                     0
#define OSPI_CS_CFG_CSP_MASK                    (0x1U << 0)          /* chip select polarity */
#define OSPI_CS_CFG_TRSP_IGNORE_CNT_POS         1
#define OSPI_CS_CFG_TRSP_IGNORE_CNT_MASK        (0x7FU << 1)         /* transparent head ignore count, for psarm mode */
#define OSPI_CS_CFG_CS_SETUP_POS                8
#define OSPI_CS_CFG_CS_SETUP_MASK               (0xFFU << 8)         /* chip select setup time */
#define OSPI_CS_CFG_CS_HOLD_POS                 16
#define OSPI_CS_CFG_CS_HOLD_MASK                (0xFFU << 16)        /* chip select hold time */
#define OSPI_CS_CFG_CS_RECOVER_POS              24
#define OSPI_CS_CFG_CS_RECOVER_MASK             (0xFFU << 24)        /* chip select recover time */

// OSPI write protect and hold register WPH(offset 0x3C)
#define OSPI_WP_HOLD_WP_POS                     0
#define OSPI_WP_HOLD_WP_MASK                    (0xFFU << 0)         /* write protect */
#define OSPI_WP_HOLD_HD_POS                     1
#define OSPI_WP_HOLD_HD_MASK                    (0xFFU << 1)         /* hold */

// OSPI software configure register 0 SW_CFG0/SW_WRITE_CFG0(offset 0x40/0x64)
#define OSPI_SW_CFG0_P0_BIT_CNT_POS             0
#define OSPI_SW_CFG0_P0_BIT_CNT_MASK            (0x7FU << 0)         /* command phase0 bit count */
#define OSPI_SW_CFG0_P0_BUS_WIDTH_POS           8
#define OSPI_SW_CFG0_P0_BUS_WIDTH_MASK          (0x7U << 8)          /* command phase0 bus width */
#define OSPI_SW_CFG0_P1_BUS_WIDTH_POS           12
#define OSPI_SW_CFG0_P1_BUS_WIDTH_MASK          (0x7U << 12)         /* command phase1 bus width */
#define OSPI_SW_CFG0_P1_BIT_CNT_POS             16
#define OSPI_SW_CFG0_P1_BIT_CNT_MASK            (0x7FU << 16)        /* command phase1 bit count */
#define OSPI_SW_CFG0_DUMMY_CYCLE_POS            24
#define OSPI_SW_CFG0_DUMMY_CYCLE_MASK           (0x7FU << 24)        /* dummy cycle before data phase */

// OSPI software configure register 1 SW_CFG1/SW_WRITE_CFG1(offset 0x44/0x64)
#define OSPI_SW_CFG1_SDATA_BYTE_CNT_POS         0
#define OSPI_SW_CFG1_SDATA_BYTE_CNT_MASK        (0xFFFFFU << 0)      /* in/out data byte count */
#define OSPI_SW_CFG1_SDATA_BUS_WIDTH_POS        20
#define OSPI_SW_CFG1_SDATA_BUS_WIDTH_MASK       (0x7U << 20)         /* sdata bus width */
#define OSPI_SW_CFG1_BUF_WIDTH_BYTES_POS        24
#define OSPI_SW_CFG1_BUF_WIDTH_BYTES_MASK       (0x7U << 24)         /* buffer width count in bytes */
#define OSPI_SW_CFG1_FIXED_LIST_EN_POS          28
#define OSPI_SW_CFG1_FIXED_LIST_EN_MASK         (0x1U << 28)         /* fixed list enable */
#define OSPI_SW_CFG1_LLEN_POS                   29
#define OSPI_SW_CFG1_LLEN_MASK                  (0x1U << 29)         /* list load enable */
#define OSPI_SW_CFG1_HB_POS                     30
#define OSPI_SW_CFG1_HB_MASK                    (0x1U << 30)         /* hyberbus */
#define OSPI_SW_CFG1_EN_POS                     31
#define OSPI_SW_CFG1_EN_MASK                    (0x1U << 31)         /* software configuration enable */
#define OSPI_SW_RW_CFG1_EN_POS                  31
#define OSPI_SW_RW_CFG1_EN_MASK                 (0x1U << 31)         /* read and write separate enable */

// OSPI debug register DEBUG(offset 0x4C)
#define OSPI_DEBUG_IDX_POS                      0
#define OSPI_DEBUG_IDX_MASK                     (0xFU << 0)          /* debug source select */
#define OSPI_DEBUG_EN_POS                       5
#define OSPI_DEBUG_EN_MASK                      (0x1U << 5)          /* debug enable */
#define OSPI_DEBUG_DATA_POS                     16
#define OSPI_DEBUG_DATA_MASK                    (0xFFFFU << 16)      /* debug data */

// OSPI status register STATUS(offset 0x50)
#define OSPI_STATUS_BUSY_POS                    0
#define OSPI_STATUS_BUSY_MASK                   (0x1U << 0)          /* busy infomation */

// OSPI external memory infomation register MTYPE0/1(offset 0x54/0x58)
#define OSPI_MEM_TYPE_DT_POS                    0
#define OSPI_MEM_TYPE_DT_MASK                   (0x1U << 0)          /* device type */
#define OSPI_MEM_TYPE_BPO_POS                   1
#define OSPI_MEM_TYPE_BPO_MASK                  (0x1U << 1)          /* bypass opcode register */
#define OSPI_MEM_TYPE_RBX_POS                   2
#define OSPI_MEM_TYPE_RBX_MASK                  (0x1U << 2)          /* row boundary crossing */
#define OSPI_MEM_TYPE_ADDR_WIDTH_POS            3
#define OSPI_MEM_TYPE_ADDR_WIDTH_MASK           (0x1U << 3)          /* address width, 24/32bits */
#define OSPI_MEM_TYPE_MEM_SIZE_POS              4
#define OSPI_MEM_TYPE_MEM_SIZE_MASK             (0x7U << 4)          /* external memory size */
#define OSPI_MEM_TYPE_PROTOCAL_POS              12
#define OSPI_MEM_TYPE_PROTOCAL_MASK             (0x3U << 12)         /* protocal */
#define OSPI_MEM_TYPE_PAGE_CROSS_EN_POS         15
#define OSPI_MEM_TYPE_PAGE_CROSS_EN_MASK        (0x1U << 15)         /* enable auto page cross */
#define OSPI_MEM_TYPE_PAGE_SIZE_POS             16
#define OSPI_MEM_TYPE_PAGE_SIZE_MASK            (0xFFFFU << 16)      /* page size in bytes */

// OSPI security register SEC_CTRL(offset 0x60)
#define OSPI_SEC_CTRL_ENC_POS                   0
#define OSPI_SEC_CTRL_ENC_MASK                  (0x1U << 0)          /* encrypt enable */
#define OSPI_SEC_CTRL_DEC_POS                   1
#define OSPI_SEC_CTRL_DEC_MASK                  (0x1U << 1)          /* decrypt enable */

// OSPI shadow configurations for list function SHADOW_CFG(offset 0x7C)
#define OSPI_SHADOW_CFG_DATA_BYTE_CNT_POS       0
#define OSPI_SHADOW_CFG_DATA_BYTE_CNT_MASK      (0xFFFFFU << 0)      /* data cnt to transfer in byte */
#define OSPI_SHADOW_CFG_TRANS_INT_EN_POS        26
#define OSPI_SHADOW_CFG_TRANS_INT_EN_MASK       (0x1U << 26)         /* data transfer done interrupt enable */
#define OSPI_SHADOW_CFG_LIST_INT_EN_POS         27
#define OSPI_SHADOW_CFG_LIST_INT_EN_MASK        (0x1U << 27)         /* list process done interrupt enable */
#define OSPI_SHADOW_CFG_RW_CMD_POS              28
#define OSPI_SHADOW_CFG_RW_CMD_MASK             (0x1U << 28)         /* indicate read or write */
#define OSPI_SHADOW_CFG_LLEN_POS                29
#define OSPI_SHADOW_CFG_LLEN_MASK               (0x1U << 29)         /* list process enable */
#define OSPI_SHADOW_CFG_TRANS_INT_POS           30
#define OSPI_SHADOW_CFG_TRANS_INT_MASK          (0x1U << 30)         /* data transfer done interrupt flag */
#define OSPI_SHADOW_CFG_LIST_INT_POS            31
#define OSPI_SHADOW_CFG_LIST_INT_MASK           (0x1U << 32)         /* list process done interrupt enable */


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    __I  uint32_t    INT_STATUS;        // offset address 0x00, ospi interrupt status register
    __IO uint32_t    RAW_INT_STATUS;    // offset address 0x04, ospi raw interrupt status register
    __IO uint32_t    INT_EN;            // offset address 0x08, ospi interrupt enable register
    __IO uint32_t    CMD;               // offset address 0x0C, ospi command register
    __IO uint32_t    CMD_DATA0;         // offset address 0x10, ospi command data0 register
    __IO uint32_t    CMD_DATA1;         // offset address 0x14, ospi command data1 register
    __IO uint32_t    RD_DATA0;          // offset address 0x18, ospi read0 register
    __IO uint32_t    RD_DATA1;          // offset address 0x1C, ospi read1 register
    __IO uint32_t    ADDR;              // offset address 0x20, ospi address register
    __IO uint32_t    OPCODE;            // offset address 0x24, ospi opcode register(Auto Restore)
    struct {
        __IO uint32_t    SPI_CFG;       // offset address 0x28/0x30, ospi configuration register 0(Auto Restore)
        __IO uint32_t    CS_CFG;        // offset address 0x2C/0x34, ospi CS configuration register 0
    } CFG[2];
    __IO uint32_t    TRANSP_REMAP;      // offset address 0x38, ospi transparent remap register
    __IO uint32_t    WP_HOLD;           // offset address 0x3C, ospi write protect and hold register
    __IO uint32_t    SW_CFG0;           // offset address 0x40, ospi cfg0 register
    __IO uint32_t    SW_CFG1;           // offset address 0x44, ospi cfg1 register
         uint32_t    RES0;              // offset address 0x48, reserved register
    __IO uint32_t    DBG;               // offset address 0x4C, ospi debug register
    __I  uint32_t    STATUS;            // offset address 0x50, ospi spi module register
    __IO uint32_t    MEM_TYPE0;         // offset address 0x54, ospi external memory 0 information register
    __IO uint32_t    MEM_TYPE1;         // offset address 0x58, ospi external memory 1 information register
    __IO uint32_t    LIST_LOAD_ADDR;    // offset address 0x5C, start address of list operation
    __IO uint32_t    SEC_CTRL;          // offset address 0x60, security control(Auto Restore)
    __IO uint32_t    SW_WR_CFG0;        // offset address 0x64, ospi write cfg0 register
    __IO uint32_t    SW_WR_CFG1;        // offset address 0x68, ospi write cfg1 register
         uint32_t    RES1;              // offset address 0x6C, reserved register
         uint32_t    RES2;              // offset address 0x70, reserved register
         uint32_t    RES3;              // offset address 0x74, reserved register
         uint32_t    RES4;              // offset address 0x78, reserved register
    __IO uint32_t    SHADOW_CFG;        // offset address 0x7C, shadow register for list function
    __IO uint32_t    CMD_WR_DATA0;      // offset address 0x80, ospi command data0 for write
} OM_OSPI_Type;


#ifdef __cplusplus
}
#endif


#endif  /*__OSPI_REG_H */


/** @} */
