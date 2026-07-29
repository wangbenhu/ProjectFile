/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup RGB RGB
 * @ingroup  DEVICE
 * @brief    RGB register
 * @details  RGB register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __RGB_REG_H
#define __RGB_REG_H


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
// PR
#define RGB_PR_DATPR_POS                    16
#define RGB_PR_DATPR_MASK                   (0xFFFFU << 16)
#define RGB_PR_RSTPR_POS                    0
#define RGB_PR_RSTPR_MASK                   (0xFFFFU << 0)

// DUTYR
#define RGB_DUTYR_DUTY1_POS                 16
#define RGB_DUTYR_DUTY1_MASK                (0xFFFFU << 16)
#define RGB_DUTYR_DUTY0_POS                 0
#define RGB_DUTYR_DUTY0_MASK                (0xFFFFU << 0)

// COMCR
#define RGB_COMCR_RGBWF_POS                 31
#define RGB_COMCR_RGBWF_MASK                (0x1U << 31)
#define RGB_COMCR_RGBF_POS                  30
#define RGB_COMCR_RGBF_MASK                 (0x1U << 30)
#define RGB_COMCR_EXTDEN_POS                29
#define RGB_COMCR_EXTDEN_MASK               (0x1U << 29)
#define RGB_COMCR_EXTDBITS_POS              24
#define RGB_COMCR_EXTDBITS_MASK             (0x1FU << 24)
#define RGB_COMCR_IDLEL_POS                 23
#define RGB_COMCR_IDLEL_MASK                (0x1U << 23)
#define RGB_COMCR_FREEF_POS                 21
#define RGB_COMCR_FREEF_MASK                (0x1U << 21)
#define RGB_COMCR_FREEDBITS_POS             16
#define RGB_COMCR_FREEDBITS_MASK            (0x1FU << 16)
#define RGB_COMCR_PSC_POS                   0
#define RGB_COMCR_PSC_MASK                  (0xFFFFU << 0)

// CESR
#define RGB_CESR_TXFIFORST_POS              15
#define RGB_CESR_TXFIFORST_MASK             (0x1U << 15)
#define RGB_CESR_TXFTIE_POS                 12
#define RGB_CESR_TXFTIE_MASK                (0x1U << 12)
#define RGB_CESR_TXFEIE_POS                 11
#define RGB_CESR_TXFEIE_MASK                (0x1U << 11)
#define RGB_CESR_TXFNFIE_POS                10
#define RGB_CESR_TXFNFIE_MASK               (0x1U << 10)
#define RGB_CESR_TCIE_POS                   9
#define RGB_CESR_TCIE_MASK                  (0x1U << 9)
#define RGB_CESR_TXRSTIE_POS                8
#define RGB_CESR_TXRSTIE_MASK               (0x1U << 8)
#define RGB_CESR_TXFTCFG_POS                5
#define RGB_CESR_TXFTCFG_MASK               (0x3U << 5)
#define RGB_CESR_DMAEN_POS                  2
#define RGB_CESR_DMAEN_MASK                 (0x1U << 2)
#define RGB_CESR_TXRSTEN_POS                1
#define RGB_CESR_TXRSTEN_MASK               (0x1U << 1)
#define RGB_CESR_EN_POS                     0
#define RGB_CESR_EN_MASK                    (0x1U << 0)

// DR
#define RGB_DR_DAT_POS                      0
#define RGB_DR_DAT_MASK                     (0xFFFFFFFFU << 0)

// EXTDR
#define RGB_EXTDR_EXTDAT_POS                0
#define RGB_EXTDR_EXTDAT_MASK               (0xFFFFFFFFU << 0)

// ISR
#define RGB_ISR_TXFT_POS                    5
#define RGB_ISR_TXFT_MASK                   (0x1U << 5)
#define RGB_ISR_TXFE_POS                    4
#define RGB_ISR_TXFE_MASK                   (0x1U << 4)
#define RGB_ISR_TXFNF_POS                   3
#define RGB_ISR_TXFNF_MASK                  (0x1U << 3)
#define RGB_ISR_TC_POS                      2
#define RGB_ISR_TC_MASK                     (0x1U << 2)
#define RGB_ISR_TXRSTIF_POS                 1
#define RGB_ISR_TXRSTIF_MASK                (0x1U << 1)
#define RGB_ISR_BSY_POS                     0
#define RGB_ISR_BSY_MASK                    (0x1U << 0)


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
     __IO uint32_t PR;
     __IO uint32_t DUTYR;
     __IO uint32_t COMCR;
     __IO uint32_t CESR;
     __IO uint32_t DR;
     __IO uint32_t EXTDR;
     __IO uint32_t ISR;
} OM_RGB_Type;

/*******************************************************************************
 * EXTERN VARIABLES
 */


/*******************************************************************************
 * EXTERN FUNCTIONS
 */


#ifdef __cplusplus
}
#endif


#endif  /* __RGB_REG_H */


/** @} */
