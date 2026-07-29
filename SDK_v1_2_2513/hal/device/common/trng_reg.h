/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup TRNG TRNG
 * @ingroup  DEVICE
 * @brief    TRNG register
 * @details  TRNG register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __TRNG_REG_H
#define __TRNG_REG_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include "common_reg.h"


/*******************************************************************************
 * MACROS
 */
#define TRNG_CSR0_PD_3P3_POS                                        0
#define TRNG_CSR0_PD_3P3_MASK                                       (0x1U << 0)
#define TRNG_CSR0_LOSC_ISEL_POS                                     4
#define TRNG_CSR0_LOSC_ISEL_MASK                                    (0x3U << 4)
#define TRNG_CSR0_LOSC_RSEL_POS                                     8
#define TRNG_CSR0_LOSC_RSEL_MASK                                    (0x3U << 8)
#define TRNG_CSR0_LDO_CTRL_POS                                      16
#define TRNG_CSR0_LDO_CTRL_MASK                                     (0x7U << 16)

#define TRNG_CSR1_GEN_EN_POS                                        0
#define TRNG_CSR1_GEN_EN_MASK                                       (0x1U << 0)
#define TRNG_CSR1_RND_CLR_POS                                       1
#define TRNG_CSR1_RND_CLR_MASK                                      (0x1U << 1)
#define TRNG_CSR1_SAMCLK_SEL_POS                                    2
#define TRNG_CSR1_SAMCLK_SEL_MASK                                   (0x1U << 2)
#define TRNG_CSR1_AFC_EN_POS                                        8
#define TRNG_CSR1_AFC_EN_MASK                                       (0x1U << 8)
#define TRNG_CSR1_AFC_STOP_POS                                      9
#define TRNG_CSR1_AFC_STOP_MASK                                     (0x1U << 9)

#define TRNG_ISR_RND_VLD_POS                                        0
#define TRNG_ISR_RND_VLD_MASK                                       (0x1U << 0)
#define TRNG_ISR_AFC_DONE_POS                                       1
#define TRNG_ISR_AFC_DONE_MASK                                      (0x1U << 1)

#define TRNG_AFCCFGR_HOSC_FSEL_INIT_POS                             0
#define TRNG_AFCCFGR_HOSC_FSEL_INIT_MASK                            (0xFU << 0)
#define TRNG_AFCCFGR_K_TARGET_POS                                   8
#define TRNG_AFCCFGR_K_TARGET_MASK                                  (0x1FFU << 8)
#define TRNG_AFCCFGR_HOSC_FSEL_POS                                  20
#define TRNG_AFCCFGR_HOSC_FSEL_MASK                                 (0xFU << 20)


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    __IO uint32_t CSR0;                            // 0x00
    __IO uint32_t CSR1;                            // 0x04
    __IO uint32_t ISR;                             // 0x08
    __IO uint32_t IER;                             // 0x0c
    __IO uint32_t AFCCFGR;                         // 0x10
    __IO uint32_t RNDDR;                           // 0x14
} OM_TRNG_Type;


/*******************************************************************************
 * MACROS
 */


#endif  /* __TRNG_REG_H */


/** @} */
