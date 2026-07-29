/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup SPI SPI
 * @ingroup  DEVICE
 * @brief    SPI register
 * @details  SPI register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __SPI_REG_H
#define __SPI_REG_H


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
// SPI transfer FIFO bytes number
#define SPI_TX_FIFO_NUM                        32U
#define SPI_RX_FIFO_NUM                        32U

// CTRL Reg: clock divider: for master mode only; set high and low time of SPI_CLK to (clk_divider + 1)
#define SPI_CTRL_CLK_DIVIDER_POS                0U
#define SPI_CTRL_CLK_DIVIDER_MASK              (0xFFFF << 0)
// CTRL Reg: master enable: 1-master device; 0-slave device
#define SPI_CTRL_MASTER_EN_POS                  16U
#define SPI_CTRL_MASTER_EN_MASK                (1U << 16)
// CTRL Reg: SPI mode: 1-spi mode 1(use 2nd clock edge); 0-spi mode 0(use 1st clock edge)
#define SPI_CTRL_MODE_POS                       17U
#define SPI_CTRL_MODE_MASK                     (1U << 17)
#define SPI_CTRL_MODE_1                         1U
#define SPI_CTRL_MODE_0                         0U
// CTRL Reg: level of SPI_RDY(CE) pin after a tranfer in master mode
#define SPI_CTRL_MASTER_CE_AT_END_POS           18U
#define SPI_CTRL_MASTER_CE_AT_END_MASK         (1U << 18)
// CTRL Reg: soft reset SPI hardware, except setup register
#define SPI_CTRL_SOFT_RST_POS                   19U
#define SPI_CTRL_SOFT_RST_MASK                 (1U << 19)
// CTRL Reg: MSB first transfer: 1-MSB is sent/received first; 0-LSB is sent/received first
#define SPI_CTRL_MSB_FIRST_POS                  20U
#define SPI_CTRL_MSB_FIRST_MASK                (1U << 20)
// CTRL Reg: clock is inverted: 1-SPI_CLK is high when idle; 0-SPI_CLK is low when idle
#define SPI_CTRL_INVERT_CLK_POS                 21U
#define SPI_CTRL_INVERT_CLK_MASK               (1U << 21)
#define SPI_CTRL_CLK_HIGH_WHEN_IDLE             1U
#define SPI_CTRL_CLK_LOW_WHEN_IDLE              0U
// CTRL Reg: use SPI_RDY out pin: 1-master/slave use SPI_RDY as bidirect read line; 0-master/slave use SPI_RDY as CE
#define SPI_CTRL_USE_RDY_OUT_POS                22U
#define SPI_CTRL_USE_RDY_OUT_MASK              (1U << 22U)
// CTRL Reg: 1-data is writen and read SPI_DO pin; 0-data is writen on SPI_DO pin
#define SPI_CTRL_BIDIRECT_DATA_POS              23U
#define SPI_CTRL_BIDIRECT_DATA_MASK            (1U << 23)
// CTRL Reg: 1-SPI_DO pin is high-Z while byte is being transferred; 0-SPI_DO pin is driver while byte is being transfered
#define SPI_CTRL_ACTIVE_DO_ENL_POS              24U
#define SPI_CTRL_ACTIVE_DO_ENL_MASK            (1U << 24)
// CTRL Reg: 1-SPI_DO pin is high-Z while byte is not being transferred; 0-SPI_DO pin is driver while byte is not being transfered
#define SPI_CTRL_INACTIVE_DO_ENL_POS            25U
#define SPI_CTRL_INACTIVE_DO_ENL_MASK          (1U << 25)
// CTRL Reg: set the Rx FIFO trigger level for intterupt
#define SPI_CTRL_RX_TRIG_LEVEL_POS              26U
#define SPI_CTRL_RX_TRIG_LEVEL_MASK            (3U << 26)
// TODO
// CTRL Reg: reset receiver FIFO pointers and byte counters and overrun status
#define SPI_CTRL_RX_CLR_FIFO_POS                28U
#define SPI_CTRL_RX_CLR_FIFO_MASK              (1U << 28)
// CTRL Reg: reset transmitter FIFO pointers and byte counters and overrun status
#define SPI_CTRL_TX_CLR_FIFO_POS                29U
#define SPI_CTRL_TX_CLR_FIFO_MASK              (1U << 29)
// CTRL Reg: enable rx fifo
#define SPI_CTRL_RX_FIFO_EN_POS                 30U
#define SPI_CTRL_RX_FIFO_EN_MASK               (1U << 30)
// CTRL Reg: enable tx fifo
#define SPI_CTRL_TX_FIFO_EN_POS                 31U
#define SPI_CTRL_TX_FIFO_EN_MASK               (1U << 31)

// STAT Reg: rx not empty: this bit set to 1 whenever a complete incoming byte has been received
#define SPI_STAT_RX_NOT_EMPTY_POS               0U
#define SPI_STAT_RX_NOT_EMPTY_MASK             (1U << 0)
// STAT Reg: rxfifo overrun error
#define SPI_STAT_RX_FIFO_OVERRUN_POS            1U
#define SPI_STAT_RX_FIFO_OVERRUN_MASK          (1U << 1)
// STAT Reg: txfifo overrun error
#define SPI_STAT_TX_FIFO_OVERRUN_POS            2U
#define SPI_STAT_TX_FIFO_OVERRUN_MASK          (1U << 2)
// STAT Reg: this bit enable transmiter FIFO empty interrupt
#define SPI_STAT_TX_EMPTY_INT_EN_POS            4U
#define SPI_STAT_TX_EMPTY_INT_EN_MASK          (1U << 4)
// STAT Reg: transmitter FIFO empty
#define SPI_STAT_TX_EMPTY_POS                   5U
#define SPI_STAT_TX_EMPTY_MASK                 (1U << 5)
// STAT Reg: this bit enable receiver FIFO trigger level intterupt
#define SPI_STAT_RX_TRIG_INT_EN_POS             6U
#define SPI_STAT_RX_TRIG_INT_EN_MASK           (1U << 6)
// STAT Reg: rx fifo trigger level reached
#define SPI_STAT_RX_FIFO_TRIG_POS               7U
#define SPI_STAT_RX_FIFO_TRIG_MASK             (1U << 7)
// STAT Reg: receiver FIFO byte count
#define SPI_STAT_RX_BYTE_CNT_POS                8U
#define SPI_STAT_RX_BYTE_CNT_MASK              (0xFFU << 8)
// STAT Reg: transmitter FIFO byte count
#define SPI_STAT_TX_BYTE_CNT_POS                16U
#define SPI_STAT_TX_BYTE_CNT_MASK              (0xFFU << 16)
// STAT Reg: SPI transfer is process
#define SPI_STAT_SPI_ACTIVE_POS                 24U
#define SPI_STAT_SPI_ACTIVE_MASK               (1U << 24)
// STAT Reg: level read on SPI_RDY pin
#define SPI_STAT_SPI_RDY_IN_POS                 25U
#define SPI_STAT_SPI_RDY_IN_MASK               (1U << 25)
// STAT Reg: level being drivern on SPI_RDY pin
#define SPI_STAT_SPI_RDY_OUT_POS                26U
#define SPI_STAT_SPI_RDY_OUT_MASK              (1U << 26)
// STAT Reg: SPI is waiting for SPI_RDY line go high
#define SPI_STAT_W4_RDY_HIGH_POS                27U
#define SPI_STAT_W4_RDY_HIGH_MASK              (1U << 27)
// STAT Reg: bit count of current byte being written/read
#define SPI_STAT_BIT_COUNT_POS                  28U
#define SPI_STAT_BIT_COUNT_MASK                (0x07U << 28)
// STAT Reg: SPI intterrupt status & clear
#define SPI_STAT_SPI_INT_POS                    31U
#define SPI_STAT_SPI_INT_MASK                  (1U << 31)

// DMACR Reg
#define SPI_DMACR_TDMAE_POS                     1U
#define SPI_DMACR_TDMAE_MASK                   (1U << 1)
#define SPI_DMACR_RDMAE_POS                     0U
#define SPI_DMACR_RDMAE_MASK                   (1U << 0)

// CSNCTRL Reg: CSN manual
#define SPI_CSNCTRL_CS_MODE_POS                 0U
#define SPI_CSNCTRL_CS_MODE_MASK               (1U << 0)
// CNSCTRL Reg: CSN level
#define SPI_CSNCTRL_CS_GPO_POS                  1U
#define SPI_CSNCTRL_CS_GPO_MASK                (1U << 1)

// DELAY_CFG Reg: RX INT MASK
#define SPI_DELAY_CFG_RX_INT_POS                5U
#define SPI_DELAY_CFG_RX_INT_MASK              (1U << 5)
// DELAY_CFG Reg: TX INT MASK
#define SPI_DELAY_CFG_TX_INT_POS                4U
#define SPI_DELAY_CFG_TX_INT_MASK              (1U << 4)
// DELAY_CFG Reg: DELAY SAMPLE
#define SPI_DELAY_CFG_SAMPLE_FE_EN_POS          2U
#define SPI_DELAY_CFG_SAMPLE_FE_EN_MASK        (1U << 2)
#define SPI_DELAY_CFG_SAMPLE_CYCLE_NUM_POS      0U
#define SPI_DELAY_CFG_SAMPLE_CYCLE_NUM_MASK    (3U << 0)


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    __IO uint32_t CTRL;                 // offset: 0x00
    __IO uint32_t WDATA;                // offset: 0x04
    __I  uint32_t RDATA;                // offset: 0x08
    __IO uint32_t STAT;                 // offset: 0x0C
    __IO uint32_t DMACR;                // offset: 0x10
    __IO uint32_t DMATDLR;              // offset: 0x14
    __IO uint32_t DMARDLR;              // offset: 0x18
    __IO uint32_t CSNCTRL;              // offset: 0x1C
    __IO uint32_t DELAY_CFG;            // offset: 0x20
} OM_SPI_Type;


/*******************************************************************************
 * EXTERN VARIABLES
 */


/*******************************************************************************
 * EXTERN FUNCTIONS
 */


#ifdef __cplusplus
}
#endif


#endif  /* __SPI_REG_H */


/** @} */
