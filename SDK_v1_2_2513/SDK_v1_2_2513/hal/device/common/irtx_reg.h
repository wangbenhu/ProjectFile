/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup IRTX IRTX
 * @ingroup  DEVICE
 * @brief    IRTX register
 * @details  IRTX register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __IRTX_REG_H
#define __IRTX_REG_H


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
/*
 * definitions for the PWM_EN Register
 */
#define IRTX_PWM_EN_POS                             0U
#define IRTX_PWM_EN_MASK                            (1U << 0)
/*
 * definitions for the PWM_MODE Register
 */
#define IRTX_PWM_MODE_POS                           0U
#define IRTX_PWM_MODE_MASK                          (3U << 0)
/*
 * definitions for the PWM_INV Register
 */
#define IRTX_PWM_INV_ENABLE_POS                     0U
#define IRTX_PWM_INV_ENABLE_MASK                    (1U << 0)
/*
 * definitions for the PWM_POLARITY Register
 */
#define IRTX_PWM_POLARITY_POS                       0U
#define IRTX_PWM_POLARITY_MASK                      (1U << 0)
/*
 * definitions for the PWM_TCMP Register
 */
#define IRTX_PWM_TCMP_GEGIN_LEVEL_TIME_POS          0U
#define IRTX_PWM_TCMP_GEGIN_LEVEL_TIME_MASK         (0xFFFF << 0)

/*
 * definitions for the PWM_TMAX Register
 */
#define IRTX_PWM_TMAX_CYCLE_POS                     0U
#define IRTX_PWM_TMAX_CYCLE_MASK                    (0xFFFF << 0)
/*
 * definitions for the PWM_TCMP_SHADOW Register
 */
#define IRTX_PWM_TCMP_SHADOW_GEGIN_LEVEL_TIME_POS   0U
#define IRTX_PWM_TCMP_SHADOW_GEGIN_LEVEL_TIME_MASK  (0xFFFF << 0)
/*
 * definitions for the PWM_TMAX_SHADOW Register
 */
#define IRTX_PWM_TMAX_SHADOW_CYCLE_POS              0U
#define IRTX_PWM_TMAX_SHADOW_CYCLE_MASK             (0xFFFF << 0)
/*
 * definitions for the PWM_PNUM Register
 */
#define IRTX_PWM_PNUM_PULSE_NUMBER_POS              0U
#define IRTX_PWM_PNUM_PULSE_NUMBER_MASK             (0x3FFF << 0)
/*
 * definitions for the PWM_CNT Register
 */
#define IRTX_PWM_CNT_VALUE_POS                      0U
#define IRTX_PWM_CNT_VALUE_MASK                     (0xFFFF << 0)
/*
 * definitions for the PWM_PULSE_CNT Register
 */
#define IRTX_PWM_PULSE_CNT_POS                      0U
#define IRTX_PWM_PULSE_CNT_MASK                     (0U << 0)
/*
 * definitions for the PWM_INT_MASK Register
 */
#define IRTX_PWM_INT_PNUM_INT_POS                   0U
#define IRTX_PWM_INT_PNUM_INT_MASK                  (1U << 0)
#define IRTX_PWM_INT_DMA_MODE_INT_POS               1U
#define IRTX_PWM_INT_DMA_MODE_INT_MASK              (1U << 1)
#define IRTX_PWM_INT_PWM_FRAME_INT_POS              2U
#define IRTX_PWM_INT_PWM_FRAME_INT_MASK             (1U << 2)
/*
 * definitions for the PWM_INT_ST Register
 */
#define IRTX_PWM_INT_ST_PWM_PNUM_INT_POS            0U
#define IRTX_PWM_INT_ST_PWM_PNUM_INT_MASK           (1U << 0)
#define IRTX_PWM_INT_ST_PWM_DMA_MODE_INT_POS        1U
#define IRTX_PWM_INT_ST_PWM_DMA_MODE_INT_MASK       (1U << 1)
#define IRTX_PWM_INT_ST_PWM_CYCLE_DONE_INT_POS      2U
#define IRTX_PWM_INT_ST_PWM_CYCLE_DONE_INT_MASK     (1U << 2)
/*
 * definitions for the FIFO_CNT_INT_MASK Register
 */
#define IRTX_FIFO_CNT_INT_MASK_ID_MASK              (3U << 0)
#define IRTX_FIFO_CNT_INT_MASK_FIFO_CNT_POS         0U
#define IRTX_FIFO_CNT_INT_MASK_FIFO_CNT_MASK        (1U << 0)
#define IRTX_FIFO_CNT_INT_MASK_FIFO_EMPTY_POS       1U
#define IRTX_FIFO_CNT_INT_MASK_FIFO_EMPTY_MASK      (1U << 1)
/*
 * definitions for the FIFO_CNT_INT_ST Register
 */
#define IRTX_FIFO_CNT_INT_ST_FIFO_STATUS_MASK       (3U << 0)
#define IRTX_FIFO_CNT_INT_ST_FIFO_CNT_INT_POS       0
#define IRTX_FIFO_CNT_INT_ST_FIFO_CNT_INT_MASK      (1U << 0)
#define IRTX_FIFO_CNT_INT_ST_FIFO_EMPTY_INT_POS     1
#define IRTX_FIFO_CNT_INT_ST_FIFO_EMPTY_INT_MASK    (1U << 1)
/*
 * definitions for the FIFO_DATA_ENTRY Register
 */
#define IRTX_FIFO_DATA_ENTRY_HIGH_LEVEL_MASK        (1U << 15)
#define IRTX_FIFO_DATA_ENTRY_HIGH_LEVEL_POS         15
#define IRTX_FIFO_DATA_ENTRY_PULSE_GROUP_SEL_MASK   (1U << 14) /* config carrier work at another kind of frequency and duty cycle for another pulse group */
#define IRTX_FIFO_DATA_ENTRY_PULSE_GROUP_SEL_POS    14
/*
 * definitions for the FIFO_NUM_LVL Register
 */
#define IRTX_FIFO_NUM_LVL_FIFO_TRIGER_LVL_POS       0U
#define IRTX_FIFO_NUM_LVL_FIFO_TRIGER_LVL_MASK      (0xFU << 0)
/*
 * definitions for the FIFO_SR Register
 */
#define IRTX_FIFO_SR_FIFO_DATA_NUM_POS              0U
#define IRTX_FIFO_SR_FIFO_DATA_NUM_MASK             (0xF << 0)
#define IRTX_FIFO_SR_FIFO_EMPTY_POS                 4U
#define IRTX_FIFO_SR_FIFO_EMPTY_MASK                (1U << 4)
#define IRTX_FIFO_SR_FIFO_FULL_POS                  5U
#define IRTX_FIFO_SR_FIFO_FULL_MASK                 (1U << 5)
/*
 * definitions for the FIFO_CLR Register
 */
// FIFO_CLR
#define IRTX_FIFO_CLR_POS                           0U
#define IRTX_FIFO_CLR_MASK                          (1U << 0)
/*
 * definitions for the DMA_SADDR Register
 */
#define IRTX_DMA_SADDR_POS                          0U
#define IRTX_DMA_SADDR_MASK                         (0xFFFF << 0)
/*
 * definitions for the DMA_START Register
 */
// DMA START
#define IRTX_DMA_START_POS                          0U
#define IRTX_DMA_START_MASK                         (1U << 0)
/*
 * definitions for the IR_ANA_IF Register
 */
#define IRTX_IR_ANA_IF_PDB_TX_POS                   0U
#define IRTX_IR_ANA_IF_PDB_TX_MASK                  (1U << 0)
#define IRTX_IR_ANA_IF_PWM_DRV_POS                  1U
#define IRTX_IR_ANA_IF_PWM_DRV_MASK                 (1U << 1)
#define IRTX_IR_ANA_IF_PWM_DRV_MIN_MASK             (0U << 1)
#define IRTX_IR_ANA_IF_PWM_DRV_MAX_MASK             (1U << 1)


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    __IO uint32_t PWM_EN;               // offset: 0x00
    __IO uint32_t PWM_MODE;             // offset: 0x04
    __IO uint32_t PWM_INV;              // offset: 0x08
    __IO uint32_t PWM_POLARITY;         // offset: 0x0c
    __IO uint32_t PWM_TCMP;             // offset: 0x10
    __IO uint32_t PWM_TMAX;             // offset: 0x14
    __IO uint32_t PWM_TCMP_SHADOW;      // offset: 0x18
    __IO uint32_t PWM_TMAX_SHADOW;      // offset: 0x1C
    __IO uint32_t PWM_PNUM;             // offset: 0x20
    __IO uint32_t PWM_CNT;              // offset: 0x24
    __I  uint32_t PWM_PULSE_CNT;        // offset: 0x28
    __IO uint32_t PWM_INT_MASK;         // offset: 0x2C
    __IO uint32_t PWM_INT_ST;           // offset: 0x30
    __IO uint32_t FIFO_CNT_INT_MASK;    // offset: 0x34
    __IO uint32_t FIFO_CNT_INT_ST;      // offset: 0x38
    __IO uint32_t FIFO_DATA_ENTRY;      // offset: 0x3C
    __IO uint32_t FIFO_NUM_LVL;         // offset: 0x40
    __I  uint32_t FIFO_SR;              // offset: 0x44
    __IO uint32_t FIFO_CLR;             // offset: 0x48
    __IO uint32_t DMA_SADDR;            // offset: 0x4C
    __IO uint32_t DMA_TRANS_LENGTH;     // offset: 0x50
    __IO uint32_t DMA_START;            // offset: 0x54
    __IO uint32_t IR_ANA_IF;            // offset: 0x58
} OM_IRTX_Type;


#ifdef __cplusplus
}
#endif


#endif  /* __IRTX_REG_H */


/** @} */
