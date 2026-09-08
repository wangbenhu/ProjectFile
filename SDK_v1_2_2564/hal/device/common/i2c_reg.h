/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup I2C I2C
 * @ingroup  DEVICE
 * @brief    I2C register
 * @details  I2C register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __I2C_REG_H
#define __I2C_REG_H


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
// CON Reg: control defines
#define I2C_CON_MASTER_MODE_POS                 0
#define I2C_CON_MASTER_MODE_MASK                (0x1U << 0)
#define I2C_CON_SPEED_POS                       1
#define I2C_CON_SPEED_MASK                      (0x3U << 1)
#define I2C_CON_10BITADDR_SLAVE_POS             3
#define I2C_CON_10BITADDR_SLAVE_MASK            (0x1U << 3)
#define I2C_CON_10BITADDR_MASTER_POS            4
#define I2C_CON_10BITADDR_MASTER_MASK           (0x1U << 4)
#define I2C_CON_RESTART_EN_POS                  5
#define I2C_CON_RESTART_EN_MASK                 (0x1U << 5)
#define I2C_CON_SLAVE_DISABLE_POS               6
#define I2C_CON_SLAVE_DISABLE_MASK              (0x1U << 6)
// INTR Reg: I2C Interrupt status & mask defines
#define I2C_INTR_RX_UNDER_POS                   0
#define I2C_INTR_RX_UNDER_MASK                  (0x1U << 0)
#define I2C_INTR_RX_OVER_POS                    1
#define I2C_INTR_RX_OVER_MASK                   (0x1U << 1)
#define I2C_INTR_RX_FULL_POS                    2
#define I2C_INTR_RX_FULL_MASK                   (0x1U << 2)
#define I2C_INTR_TX_OVER_POS                    3
#define I2C_INTR_TX_OVER_MASK                   (0x1U << 3)
#define I2C_INTR_TX_EMPTY_POS                   4
#define I2C_INTR_TX_EMPTY_MASK                  (0x1U << 4)
#define I2C_INTR_RD_REQ_POS                     5
#define I2C_INTR_RD_REQ_MASK                    (0x1U << 5)
#define I2C_INTR_TX_ABRT_POS                    6
#define I2C_INTR_TX_ABRT_MASK                   (0x1U << 6)
#define I2C_INTR_RX_DONE_POS                    7
#define I2C_INTR_RX_DONE_MASK                   (0x1U << 7)
#define I2C_INTR_ACTIVITY_POS                   8
#define I2C_INTR_ACTIVITY_MASK                  (0x1U << 8)
#define I2C_INTR_STOP_DET_POS                   9
#define I2C_INTR_STOP_DET_MASK                  (0x1U << 9)
#define I2C_INTR_START_DET_POS                  10
#define I2C_INTR_START_DET_MASK                 (0x1U << 10)
#define I2C_INTR_GEN_CALL_POS                   11
#define I2C_INTR_GEN_CALL_MASK                  (0x1U << 11)
#define I2C_INTR_TIME_OUT_POS                   12
#define I2C_INTR_TIME_OUT_MASK                  (0x1U << 12)
// STATUS Reg: I2C Status defines
#define I2C_STATUS_ACTIVITY_POS                 0
#define I2C_STATUS_ACTIVITY_MASK                (0x1U << 0)
#define I2C_STATUS_TFNF_POS                     1
#define I2C_STATUS_TFNF_MASK                    (0x1U << 1)
#define I2C_STATUS_TFE_POS                      2
#define I2C_STATUS_TFE_MASK                     (0x1U << 2)
#define I2C_STATUS_RFNE_POS                     3
#define I2C_STATUS_RFNE_MASK                    (0x1U << 3)
#define I2C_STATUS_RFF_POS                      4
#define I2C_STATUS_RFF_MASK                     (0x1U << 4)
#define I2C_STATUS_MST_ACTIVITY_POS             5
#define I2C_STATUS_MST_ACTIVITY_MASK            (0x1U << 5)
#define I2C_STATUS_SLV_ACTIVITY_POS             6
#define I2C_STATUS_SLV_ACTIVITY_MASK            (0x1U << 6)
// ABRT Reg: Abort source defines
#define I2C_TX_ABRT_SRC_7B_ADDR_NOACK_POS       0
#define I2C_TX_ABRT_SRC_7B_ADDR_NOACK_MASK      (0x1U << 0)
#define I2C_TX_ABRT_SRC_10ADDR1_NOACK_POS       1
#define I2C_TX_ABRT_SRC_10ADDR1_NOACK_MASK      (0x1U << 1)
#define I2C_TX_ABRT_SRC_10ADDR2_NOACK_POS       2
#define I2C_TX_ABRT_SRC_10ADDR2_NOACK_MASK      (0x1U << 2)
#define I2C_TX_ABRT_SRC_TXDATA_NOACK_POS        3
#define I2C_TX_ABRT_SRC_TXDATA_NOACK_MASK       (0x1U << 3)
#define I2C_TX_ABRT_SRC_GCALL_NOACK_POS         4
#define I2C_TX_ABRT_SRC_GCALL_NOACK_MASK        (0x1U << 4)
#define I2C_TX_ABRT_SRC_GCALL_READ_POS          5
#define I2C_TX_ABRT_SRC_GCALL_READ_MASK         (0x1U << 5)
#define I2C_TX_ABRT_SRC_SBYTE_ACKDET_POS        7
#define I2C_TX_ABRT_SRC_SBYTE_ACKDET_MASK       (0x1U << 7)
#define I2C_TX_ABRT_SRC_SBYTE_NORSTART_POS      9
#define I2C_TX_ABRT_SRC_SBYTE_NORSTART_MASK     (0x1U << 9)
#define I2C_TX_ABRT_SRC_10B_RD_NORSTART_POS     10
#define I2C_TX_ABRT_SRC_10B_RD_NORSTART_MASK    (0x1U << 10)
#define I2C_TX_ABRT_SRC_MASTER_DIS_POS          11
#define I2C_TX_ABRT_SRC_MASTER_DIS_MASK         (0x1U << 11)
#define I2C_TX_ABRT_SRC_ARB_LOST_POS            12
#define I2C_TX_ABRT_SRC_ARB_LOST_MASK           (0x1U << 12)
#define I2C_TX_ABRT_SRC_SLVFLUSH_TXFIFO_POS     13
#define I2C_TX_ABRT_SRC_SLVFLUSH_TXFIFO_MASK    (0x1U << 13)
#define I2C_TX_ABRT_SRC_SLV_ARBLOST_POS         14
#define I2C_TX_ABRT_SRC_SLV_ARBLOST_MASK        (0x1U << 14)
#define I2C_TX_ABRT_SRC_SLVRD_INTX_POS          15
#define I2C_TX_ABRT_SRC_SLVRD_INTX_MASK         (0x1U << 15)
// DMA CR Reg: DMA send and receive
#define I2C_DMA_CR_RDMAE_POS                    0
#define I2C_DMA_CR_RDMAE_MASK                   (0x1U << 0)
#define I2C_DMA_CR_TDMAE_POS                    1
#define I2C_DMA_CR_TDMAE_MASK                   (0x1U << 1)
// CON1 Reg: i2c write and read
#define I2C_CON1_TX_ENABLE                     (0UL << 12)
#define I2C_CON1_RX_ENABLE                     (1UL << 12)
// CON1 Reg: CON1 control defines
#define I2C_CON1_READBYTES_UPDATE              (1UL << 16)
#define I2C_CON1_CLEAR_I2C_ENABLE              (1UL << 31)
// TIMEOUT Reg: TIMEOUT enable and count defines
#define I2C_TIMEOUT_EN_TIMEOUT_POS             31
#define I2C_TIMEOUT_EN_TIMEOUT_MASK            (0x1U << 31)
#define I2C_TIMEOUT_TIMEOUT_CNT_POS            0
#define I2C_TIMEOUT_TIMEOUT_CNT_MASK           (0x1U << 0)
// CLR_TIME_OUT Reg: TIMEOUT clear interrupt defines
#define I2C_CLR_TIME_OUT_POS                   0
#define I2C_CLR_TIME_OUT_MASK                  (0x1U << 0)

// MACROS
#define I2C_TX_FIFO_DEPTH                       16
#define I2C_RX_FIFO_DEPTH                       16

#define I2CD_NO_ERROR                           0x00   /**< @brief No error.            */
#define I2CD_BUS_ERROR                          0x01   /**< @brief Bus Error.           */
#define I2CD_ARBITRATION_LOST                   0x02   /**< @brief Arbitration Lost.    */
#define I2CD_ACK_FAILURE                        0x04   /**< @brief Acknowledge Failure. */
#define I2CD_OVERRUN                            0x08   /**< @brief Overrun/Underrun.    */
#define I2CD_PEC_ERROR                          0x10   /**< @brief PEC Error in reception. */
#define I2CD_TIMEOUT                            0x20   /**< @brief Hardware timeout.    */
#define I2CD_SMB_ALERT                          0x40   /**< @brief SMBus Alert.         */

#define I2C_INTR_DEFAULT_MASK                   (I2C_INTR_RX_FULL | I2C_INTR_TX_EMPTY | I2C_INTR_TX_ABRT | I2C_INTR_STOP_DET)
#define I2C_INTR_SLAVE_MASK                     (I2C_INTR_RD_REQ | I2C_INTR_TX_ABRT | I2C_INTR_RX_FULL | I2C_INTR_STOP_DET)

#define I2C_DEFAULT_TIMEOUT                     0


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct
{
    __IO uint32_t CON;                                  // offset address 0x00, Register for basic configuration
    __IO uint32_t TAR;                                  // offset address 0x04, Register for target address
    __IO uint32_t SAR;                                  // offset address 0x08, Register for slave address
    __IO uint32_t HS_MADDR;                             // offset address 0x0c, Register for HS Master Mode Code address
    __IO uint32_t DATA_CMD;                             // offset address 0x10, Register for Rx/Tx FIFO data and command
    __IO uint32_t SS_SCL_HCNT;                          // offset address 0x14, Register for SCL high-period count of standard speed
    __IO uint32_t SS_SCL_LCNT;                          // offset address 0x18, Register for SCL low-period count of standard speed
    __IO uint32_t FS_SCL_HCNT;                          // offset address 0x1c, Register for SCL high-period count of fast speed
    __IO uint32_t FS_SCL_LCNT;                          // offset address 0x20, Register for SCL low-period count of fast speed
    __IO uint32_t HS_SCL_HCNT;                          // offset address 0x24, Register for SCL high-period count of high speed
    __IO uint32_t HS_SCL_LCNT;                          // offset address 0x28, Register for SCL low-period count of high speed
    __I  uint32_t INTR_STAT;                            // offset address 0x2c, Register for interrupt status
    __IO uint32_t INTR_MASK;                            // offset address 0x30, Register for interrupt status mask
    __I  uint32_t RAW_INTR_STAT;                        // offset address 0x34, Register for real interrupt status
    __IO uint32_t RX_TL;                                // offset address 0x38, Register controlling entries level that triggers RX_FULL Interrupt
    __IO uint32_t TX_TL;                                // offset address 0x3c, Register controlling entries level that triggers TX_EMPTY Interrupt
    __I  uint32_t CLR_INTR;                             // offset address 0x40, Register for combined and individual interrupts clearing
    __I  uint32_t CLR_RX_UNDER;                         // offset address 0x44, Register for RX_UNDER interrupt clearing
    __I  uint32_t CLR_RX_OVER;                          // offset address 0x48, Register for RX_OVER interrupt clearing
    __I  uint32_t CLR_TX_OVER;                          // offset address 0x4c, Register for TX_OVER interrupt clearing
    __I  uint32_t CLR_RD_REQ;                           // offset address 0x50, Register for RD_REQ interrupt clearing
    __I  uint32_t CLR_TX_ABRT;                          // offset address 0x54, Register for TX_ABRT interrupt clearing
    __I  uint32_t CLR_RX_DONE;                          // offset address 0x58, Register for RX_DONE interrupt clearing
    __I  uint32_t CLR_ACTIVITY;                         // offset address 0x5c, Register for ACTIVITY interrupt clearing
    __I  uint32_t CLR_STOP_DET;                         // offset address 0x60, Register for STOP_DET interrupt clearing
    __I  uint32_t CLR_START_DET;                        // offset address 0x64, Register for START_DET interrupt clearing
    __I  uint32_t CLR_GEN_CALL;                         // offset address 0x68, Register for GEN_CALL interrupt clearing
    __IO uint32_t ENABLE;                               // offset address 0x6c, Register for enable control
    __I  uint32_t STATUS;                               // offset address 0x70, Register for Transfer/ FIFO status indicating
    __I  uint32_t TXFLR;                                // offset address 0x74, Register for entries-number of transmit FIFO
    __I  uint32_t RXFLR;                                // offset address 0x78, Register for entries-number of receive FIFO
    __IO uint32_t SDA_HOLD;                             // offset address 0x7c, Register for SDA hold time amount controlling
    __IO uint32_t TX_ABRT_SOURCE;                       // offset address 0x80, Register for transmit abort source
    __IO uint32_t SLV_DATA_NACK_ONLY;                   // offset address 0x84, Register for generate SLV_DATA_NACK
    __IO uint32_t DMA_CR;                               // offset address 0x88, Register for transmit DMA and receive DMA enabling control
    __IO uint32_t DMA_TDLR;                             // offset address 0x8c, Register for DMA transmit data entries number control
    __IO uint32_t DMA_RDLR;                             // offset address 0x90, Register for DMA receive data entries number control
    __IO uint32_t SDA_SETUP;                            // offset address 0x94, Register for SCL rising edge delay control
    __IO uint32_t ACK_GENERAL_CALL;                     // offset address 0x98, Register for ACK General Call
    __I  uint32_t ENABLE_STATUS;                        // offset address 0x9c, Register for encoded information about component’s parameter
    __IO uint32_t CON1;                                 // offset address 0xa0, Register for I2C control
         uint8_t  Reserveda4_ac[0xac - 0xa4 + 0x4];
    __IO uint32_t TIMEOUT;                              // offset address 0xb0, Register for timeout control
    __IO uint32_t CLR_TIME_OUT;                         // offset address 0xb4, Register for timeout interrupt clear
} OM_I2C_Type;


/*******************************************************************************
 * EXTERN VARIABLES
 */


/*******************************************************************************
 * EXTERN FUNCTIONS
 */


#ifdef __cplusplus
}
#endif


#endif  /* __I2C_REG_H */


/** @} */
