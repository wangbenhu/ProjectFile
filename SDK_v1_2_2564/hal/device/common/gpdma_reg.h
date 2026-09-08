/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup GPDMA GPDMA
 * @ingroup  DEVICE
 * @brief    GPDMA register
 * @details  GPDMA register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __GPDMA_REG_H
#define __GPDMA_REG_H


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
 * TYPEDEFS
 */
typedef struct {
    __IO uint32_t CTRL;          // offset address 0x44+n*0x14, channel n control register
    __IO uint32_t SRC_ADDR;      // offset address 0x48+n*0x14, channel n source address register
    __IO uint32_t DST_ADDR;      // offset address 0x4C+n*0x14, channel n destination address register
    __IO uint32_t TRANS_SIZE;    // offset address 0x50+n*0x14, channel n transfer size register
    __IO uint32_t LL_PTR;        // offset address 0x54+n*0x14, channel n linker list pointer register
} __IO OM_GPDMA_CHAN_Type;

typedef struct {
         uint32_t           RSVD_0x00_0x1c[8];
    __O  uint32_t           CTRL;                              // offset:0x20
         uint32_t           RSVD_0x24_0x2c[3];
    __IO uint32_t           INT_STATUS;                        // offset:0x30
    __I  uint32_t           CHAN_EN;                           // offset:0x34
         uint32_t           RSVD_0x38_0x3c[2];
    __O  uint32_t           CHAN_ABORT;                        // offset:0x40
         OM_GPDMA_CHAN_Type CHAN[8];                           // offset:0x44
    __O  uint32_t           LLP_SHADOW[8];                     // offset address 0xE4. when transfer is completed, llp_shadow will be NULL
    __I  uint32_t           RSVD_0x104;                        // offset address 0x104
    __IO uint32_t           REQ_ACK_SEL;                       // offset address 0x108, req ack sel
} OM_GPDMA_Type;


/*******************************************************************************
 * MACROS
 */
// CHAN CTRL
#define GPDMA_CHAN_CTRL_ENABLE_POS                              0U
#define GPDMA_CHAN_CTRL_ENABLE_MASK                             (1U << 0)
#define GPDMA_CHAN_CTRL_INTTCMASK_POS                           1U
#define GPDMA_CHAN_CTRL_INTTCMASK_MASK                          (1U << 1)
#define GPDMA_CHAN_CTRL_INTERRMASK_POS                          2U
#define GPDMA_CHAN_CTRL_INTERRMASK_MASK                         (1U << 2)
#define GPDMA_CHAN_CTRL_INTABTMASK_POS                          3U
#define GPDMA_CHAN_CTRL_INTABTMASK_MASK                         (1U << 3)
#define GPDMA_CHAN_CTRL_DSTREQSEL_POS                           4U
#define GPDMA_CHAN_CTRL_DSTREQSEL_MASK                          (0x0FU << 4)
#define GPDMA_CHAN_CTRL_SRCREQSEL_POS                           8U
#define GPDMA_CHAN_CTRL_SRCREQSEL_MASK                          (0x0FU << 8)
#define GPDMA_CHAN_CTRL_DSTADDRCTRL_POS                         12U
#define GPDMA_CHAN_CTRL_DSTADDRCTRL_MASK                        (0x03U << 12)
#define GPDMA_CHAN_CTRL_SRCADDRCTRL_POS                         14U
#define GPDMA_CHAN_CTRL_SRCADDRCTRL_MASK                        (3U << 14)
#define GPDMA_CHAN_CTRL_DSTMODE_POS                             16U
#define GPDMA_CHAN_CTRL_DSTMODE_MASK                            (1U << 16)
#define GPDMA_CHAN_CTRL_SRCMODE_POS                             17U
#define GPDMA_CHAN_CTRL_SRCMODE_MASK                            (1U << 17)
#define GPDMA_CHAN_CTRL_DSTWIDTH_POS                            18
#define GPDMA_CHAN_CTRL_DSTWIDTH_MASK                           (3U << 18)
#define GPDMA_CHAN_CTRL_SRCWIDTH_POS                            20
#define GPDMA_CHAN_CTRL_SRCWIDTH_MASK                           (3U << 20)
#define GPDMA_CHAN_CTRL_SRCBURSTSIZE_POS                        22
#define GPDMA_CHAN_CTRL_SRCBURSTSIZE_MASK                       (7U << 22)
#define GPDMA_CHAN_CTRL_PRIORITY_POS                            29
#define GPDMA_CHAN_CTRL_PRIORITY_MASK                           (1U << 29)

// DMACTRL
#define GPDMA_CTRL_RESET_POS                                    0U
#define GPDMA_CTRL_RESET_MASK                                  (1U << 0)

// GPDMA Interrupt Status register
#define GPDMA_INT_STATUS_ERROR_POS(chan_idx)          (chan_idx)
#define GPDMA_INT_STATUS_ERROR_MASK(chan_idx)         (1U << (chan_idx))
#define GPDMA_INT_STATUS_ABORT_POS(chan_idx)          (8U + (chan_idx))
#define GPDMA_INT_STATUS_ABORT_MASK(chan_idx)         (1U << (8 + (chan_idx)))      /* abort status, one bit per channel */
#define GPDMA_INT_STATUS_TC_POS(chan_idx)             (16U + (chan_idx))
#define GPDMA_INT_STATUS_TC_MASK(chan_idx)            (1U << (16U + (chan_idx)))    /* terminal count status of GPDMA channels, one bit per channel */
#define GPDMA_INT_STATUS_ALL(chan_idx)                (GPDMA_INT_STATUS_ERROR_MASK(chan_idx) |   \
                                                       GPDMA_INT_STATUS_ABORT_MASK(chan_idx) |   \
                                                       GPDMA_INT_STATUS_TC_MASK(chan_idx))


#ifdef __cplusplus
}
#endif

#endif  /* __GPDMA_REG_H */


/** @} */
