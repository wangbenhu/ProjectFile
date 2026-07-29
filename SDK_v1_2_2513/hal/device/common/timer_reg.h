/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup TIMER TIMER
 * @ingroup  DEVICE
 * @brief    TIMER register
 * @details  TIMER register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __TIMER_REG_H
#define __TIMER_REG_H


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
// TIM_CR1
#define TIM_CTR1_CKD_POS                                               8
#define TIM_CTR1_CKD_MASK                                              (0x3U << 8)      // Clock division
#define TIM_CTR1_ARPE_POS                                              7
#define TIM_CTR1_ARPE_MASK                                             (0x1U << 7)      // Auto-reload preload enable
#define TIM_CTR1_CMS_POS                                               5
#define TIM_CTR1_CMS_MASK                                              (0x3U << 5)      // Center-aligned mdoe selection
#define TIM_CTR1_DIR_POS                                               4
#define TIM_CTR1_DIR_MASK                                              (0x1U << 4)      // direction
#define TIM_CTR1_OPM_POS                                               3
#define TIM_CTR1_OPM_MASK                                              (0x1U << 3)      // one pulse mode
#define TIM_CTR1_URS_POS                                               2
#define TIM_CTR1_URS_MASK                                              (0x1U << 2)      // Update request source
#define TIM_CTR1_UDIS_POS                                              1
#define TIM_CTR1_UDIS_MASK                                             (0x1U << 1)      // Update disable
#define TIM_CTR1_CEN_POS                                               0
#define TIM_CTR1_CEN_MASK                                              (0x1U << 0)      // Counter Enable
// TIM_CR
#define TIM_CTR2_OIS1_MASK                                             (0x1U << 8)
#define TIM_CTR2_TI1S_POS                                              7
#define TIM_CTR2_TI1S_MASK                                             (0x1U << 7)      // TI1 selection
#define TIM_CTR2_MMS_POS                                               4
#define TIM_CTR2_MMS_MASK                                              (0x7U << 4)      // Master mode selection
#define TIM_CTR2_CCDS_POS                                              3
#define TIM_CTR2_CCDS_MASK                                             (0x1U << 3)      // Capture/Compare DMA selection
#define TIM_CTR2_CCUS_POS                                              2
#define TIM_CTR2_CCUS_MASK                                             (0x1U << 2)      // Capture/Compare control update selection
#define TIM_CTR2_CCPC_POS                                              0
#define TIM_CTR2_CCPC_MASK                                             (0x1U << 0)      // Capture/Compare preload control

// TIM_SMCR
#define TIM_SMCR_ETP_POS                                               15
#define TIM_SMCR_ETP_MASK                                              (0x1U << 15)     // External trigger polarity
#define TIM_SMCR_ECE_POS                                               14
#define TIM_SMCR_ECE_MASK                                              (0x1U << 14)     // External clock enable
#define TIM_SMCR_ETPS_POS                                              12
#define TIM_SMCR_ETPS_MASK                                             (0x3U << 12)     // External trigger prescaler
#define TIM_SMCR_ETF_POS                                               8
#define TIM_SMCR_ETF_MASK                                              (0xfU << 8)      // External trigger filter
#define TIM_SMCR_MSM_POS                                               7
#define TIM_SMCR_MSM_MASK                                              (0x1U << 7)      // Master/slave mode
#define TIM_SMCR_TS_POS                                                4
#define TIM_SMCR_TS_MASK                                               (0x7U << 4)      // Trigger selection
#define TIM_SMCR_SMS_POS                                               0
#define TIM_SMCR_SMS_MASK                                              (0x7U << 0)      // Slave mode selection

/*
------------|----------|----------|----------|----------|
- slave tim |   ITR0   |   ITR1   |   ITR2   |   ITR3   |
-   TIM1    |          |   TIM2   |   TIM3   |          |
-   TIM2    |   TIM1   |          |   TIM3   |          |
-   TIM3    |   TIM1   |   TIM2   |          |          |
------------|----------|----------|----------|----------|
*/

// TIM_DIER
#define TIM_DIER_TDE_POS                                               14
#define TIM_DIER_TDE_MASK                                              (0x1U << 14)     // Trigger DMA request enable
#define TIM_DIER_COMDE_POS                                             13
#define TIM_DIER_COMDE_MASK                                            (0x1U << 13)     // COM DMA request enable
#define TIM_DIER_CC4DE_POS                                             12
#define TIM_DIER_CC4DE_MASK                                            (0x1U << 12)
#define TIM_DIER_CC3DE_POS                                             11
#define TIM_DIER_CC3DE_MASK                                            (0x1U << 11)     // Capture/Compare 3 DMA request enable
#define TIM_DIER_CC2DE_POS                                             10
#define TIM_DIER_CC2DE_MASK                                            (0x1U << 10)
#define TIM_DIER_CC1DE_POS                                             9
#define TIM_DIER_CC1DE_MASK                                            (0x1U << 9)
#define TIM_DIER_UDE_POS                                               8
#define TIM_DIER_UDE_MASK                                              (0x1U << 8)      // Update DMA request enable
#define TIM_DIER_BIE_POS                                               7
#define TIM_DIER_BIE_MASK                                              (0x1U << 7)      // Break interrupt enable
#define TIM_DIER_TIE_POS                                               6
#define TIM_DIER_TIE_MASK                                              (0x1U << 6)      // Trigger interrupt enable
#define TIM_DIER_COMIE_POS                                             5
#define TIM_DIER_COMIE_MASK                                            (0x1U << 5)      // COM interrupt enable
#define TIM_DIER_CC4IE_POS                                             4
#define TIM_DIER_CC4IE_MASK                                            (0x1U << 4)
#define TIM_DIER_CC3IE_POS                                             3
#define TIM_DIER_CC3IE_MASK                                            (0x1U << 3)      // Capture/Compare 3 interrupt enable
#define TIM_DIER_CC2IE_POS                                             2
#define TIM_DIER_CC2IE_MASK                                            (0x1U << 2)
#define TIM_DIER_CC1IE_POS                                             1
#define TIM_DIER_CC1IE_MASK                                            (0x1U << 1)
#define TIM_DIER_UIE_POS                                               0
#define TIM_DIER_UIE_MASK                                              (0x1U << 0)      // Update interrupt enable

// TIM_SR
#define TIM_SR_CC4OF_POS                                               12
#define TIM_SR_CC4OF_MASK                                              (0x1U << 12)
#define TIM_SR_CC3OF_POS                                               11
#define TIM_SR_CC3OF_MASK                                              (0x1U << 11)     // Capture/Compare 3 overcapture flag
#define TIM_SR_CC2OF_POS                                               10
#define TIM_SR_CC2OF_MASK                                              (0x1U << 10)
#define TIM_SR_CC1OF_POS                                               9
#define TIM_SR_CC1OF_MASK                                              (0x1U << 9)
#define TIM_SR_BIF_POS                                                 7
#define TIM_SR_BIF_MASK                                                (0x1U << 7)      // Break interrupt flag
#define TIM_SR_TIF_POS                                                 6
#define TIM_SR_TIF_MASK                                                (0x1U << 6)      // Trigger interrupt flag
#define TIM_SR_COMIF_POS                                               5
#define TIM_SR_COMIF_MASK                                              (0x1U << 5)      // COM interrupt flag
#define TIM_SR_CC4IF_POS                                               4
#define TIM_SR_CC4IF_MASK                                              (0x1U << 4)
#define TIM_SR_CC3IF_POS                                               3
#define TIM_SR_CC3IF_MASK                                              (0x1U << 3)      // Capture/Compare 3 interrupt flag
#define TIM_SR_CC2IF_POS                                               2
#define TIM_SR_CC2IF_MASK                                              (0x1U << 2)
#define TIM_SR_CC1IF_POS                                               1
#define TIM_SR_CC1IF_MASK                                              (0x1U << 1)
#define TIM_SR_UIF_POS                                                 0
#define TIM_SR_UIF_MASK                                                (0x1U << 0)      // Update interrupt flag

// TIM_EGR
#define TIM_EGR_32UG_POS                                               8
#define TIM_EGR_32UG_MASK                                              (0x1U << 8)      // NOTE: 6626 32bit timer update generation
#define TIM_EGR_BG_POS                                                 7
#define TIM_EGR_BG_MASK                                                (0x1U << 7)      // Break generation
#define TIM_EGR_TG_POS                                                 6
#define TIM_EGR_TG_MASK                                                (0x1U << 6)      // Trigger generation
#define TIM_EGR_COMG_POS                                               5
#define TIM_EGR_COMG_MASK                                              (0x1U << 5)      // Capture/Compare control update generation
#define TIM_EGR_CC4G_POS                                               4
#define TIM_EGR_CC4G_MASK                                              (0x1U << 4)
#define TIM_EGR_CC3G_POS                                               3
#define TIM_EGR_CC3G_MASK                                              (0x1U << 3)      // Capture/Compare 3 event generation
#define TIM_EGR_CC2G_POS                                               2
#define TIM_EGR_CC2G_MASK                                              (0x1U << 2)
#define TIM_EGR_CC1G_POS                                               1
#define TIM_EGR_CC1G_MASK                                              (0x1U << 1)
#define TIM_EGR_UG_POS                                                 0
#define TIM_EGR_UG_MASK                                                (0x1U << 0)      // Update event generation

// TIM_CCMR1
// (Input capture mode)
#define TIM_CCMR1_IC2F_POS                                             12
#define TIM_CCMR1_IC2F_MASK                                            (0xfU << 12)     // Input capture 2 filter
#define TIM_CCMR1_IC2PSC_POS                                           10
#define TIM_CCMR1_IC2PSC_MASK                                          (0x3U << 10)     // Capture/Compare 2 Selection
#define TIM_CCMR1_CC2S_POS                                             8
#define TIM_CCMR1_CC2S_MASK                                            (0x3U << 8)      // Capture/Compare 2 Selection
#define TIM_CCMR1_IC1F_POS                                             4
#define TIM_CCMR1_IC1F_MASK                                            (0xfU << 4)      // Input capture 1 prescaler
#define TIM_CCMR1_IC1PSC_POS                                           2
#define TIM_CCMR1_IC1PSC_MASK                                          (0x3U << 2)      // Input capture 1 prescaler
#define TIM_CCMR1_CC1S_POS                                             0
#define TIM_CCMR1_CC1S_MASK                                            (0x3U << 0)      // Capture/Compare 1 Selection
// (Output compare mode)
#define TIM_CCMR1_OC2CE_POS                                            15
#define TIM_CCMR1_OC2CE_MASK                                           (0x1U << 15)     // Output Compare 2 clear enable
#define TIM_CCMR1_OC2M_POS                                             12
#define TIM_CCMR1_OC2M_MASK                                            (0x7U << 12)     // Output Compare 2 mode
#define TIM_CCMR1_OC2PE_POS                                            11
#define TIM_CCMR1_OC2PE_MASK                                           (0x1U << 11)     // Output Compare 2 preload enable
#define TIM_CCMR1_OC2FE_POS                                            10
#define TIM_CCMR1_OC2FE_MASK                                           (0x1U << 10)     // Output Compare 2 fast enable
#define TIM_CCMR1_CC2S_POS                                             8
#define TIM_CCMR1_CC2S_MASK                                            (0x3U << 8)      // Capture/Compare 2 Selection
#define TIM_CCMR1_OC1CE_POS                                            7
#define TIM_CCMR1_OC1CE_MASK                                           (0x1U << 7)      // Output Compare 1 clear enable
#define TIM_CCMR1_OC1M_POS                                             4
#define TIM_CCMR1_OC1M_MASK                                            (0x7U << 4)      // Output Compare 1 mode
#define TIM_CCMR1_OC1PE_POS                                            3
#define TIM_CCMR1_OC1PE_MASK                                           (0x1U << 3)      // Output Compare 1 preload enable
#define TIM_CCMR1_OC1FE_POS                                            2
#define TIM_CCMR1_OC1FE_MASK                                           (0x1U << 2)      // Output Compare 1 fast enable
#define TIM_CCMR1_CC1S_POS                                             0
#define TIM_CCMR1_CC1S_MASK                                            (0x3U << 0)      // Capture/Compare 1 Selection

// TIM_CCMR2
// (Input capture mode)
#define TIM_CCMR2_IC4F_POS                                             12
#define TIM_CCMR2_IC4F_MASK                                            (0xfU << 12)
#define TIM_CCMR2_IC4PSC_POS                                           10
#define TIM_CCMR2_IC4PSC_MASK                                          (0x3U << 10)
#define TIM_CCMR2_CC4S_POS                                             8
#define TIM_CCMR2_CC4S_MASK                                            (0x3U << 8)

#define TIM_CCMR2_IC3F_POS                                             4
#define TIM_CCMR2_IC3F_MASK                                            (0xfU << 4)      // Input capture 3 filter
#define TIM_CCMR2_IC3PSC_POS                                           2
#define TIM_CCMR2_IC3PSC_MASK                                          (0x3U << 2)      // Input capture 3 prescaler
#define TIM_CCMR2_CC3S_POS                                             0
#define TIM_CCMR2_CC3S_MASK                                            (0x3U << 0)      // Capture/Compare 3 Selection
// (Output compare mode)
#define TIM_CCMR2_OC4CE_POS                                            15
#define TIM_CCMR2_OC4CE_MASK                                           (0x1U << 15)
#define TIM_CCMR2_OC4M_POS                                             12
#define TIM_CCMR2_OC4M_MASK                                            (0x7U << 12)
#define TIM_CCMR2_OC4PE_POS                                            11
#define TIM_CCMR2_OC4PE_MASK                                           (0x1U << 11)
#define TIM_CCMR2_OC4FE_POS                                            10
#define TIM_CCMR2_OC4FE_MASK                                           (0x1U << 10)
#define TIM_CCMR2_CC4S_POS                                             8
#define TIM_CCMR2_CC4S_MASK                                            (0x3U << 8)
#define TIM_CCMR2_OC3CE_POS                                            7
#define TIM_CCMR2_OC3CE_MASK                                           (0x1U << 7)      // Output Compare 3 clear enable
#define TIM_CCMR2_OC3M_POS                                             4
#define TIM_CCMR2_OC3M_MASK                                            (0x7U << 4)      // Output Compare 3 mode
#define TIM_CCMR2_OC3PE_POS                                            3
#define TIM_CCMR2_OC3PE_MASK                                           (0x1U << 3)      // Output Compare 3 preload enable
#define TIM_CCMR2_OC3FE_POS                                            2
#define TIM_CCMR2_OC3FE_MASK                                           (0x1U << 2)      // Capture/Compare 3 fast enable
#define TIM_CCMR2_CC3S_POS                                             0
#define TIM_CCMR2_CC3S_MASK                                            (0x3U << 0)      // Capture/Compare 3 Selection

// TIM_CCER
#define TIM_CCER_CC4P_POS                                              13
#define TIM_CCER_CC4P_MASK                                             (0x1U << 13)
#define TIM_CCER_CC4E_POS                                              12
#define TIM_CCER_CC4E_MASK                                             (0x1U << 12)
#define TIM_CCER_CC3NP_POS                                             11
#define TIM_CCER_CC3NP_MASK                                            (0x1U << 11)     // Capture/Compare 3 complementary output polarity
#define TIM_CCER_CC3NE_POS                                             10
#define TIM_CCER_CC3NE_MASK                                            (0x1U << 10)     // Capture/Compare 3 complementary output enable
#define TIM_CCER_CC3P_POS                                              9
#define TIM_CCER_CC3P_MASK                                             (0x1U << 9)      // Capture/Compare 3 output polarity
#define TIM_CCER_CC3E_POS                                              8
#define TIM_CCER_CC3E_MASK                                             (0x1U << 8)      // Capture/Compare 3 output enable
#define TIM_CCER_CC2NP_POS                                             7
#define TIM_CCER_CC2NP_MASK                                            (0x1U << 7)
#define TIM_CCER_CC2NE_POS                                             6
#define TIM_CCER_CC2NE_MASK                                            (0x1U << 6)
#define TIM_CCER_CC2P_POS                                              5
#define TIM_CCER_CC2P_MASK                                             (0x1U << 5)
#define TIM_CCER_CC2E_POS                                              4
#define TIM_CCER_CC2E_MASK                                             (0x1U << 4)
#define TIM_CCER_CC1NP_POS                                             3
#define TIM_CCER_CC1NP_MASK                                            (0x1U << 3)
#define TIM_CCER_CC1NE_POS                                             2
#define TIM_CCER_CC1NE_MASK                                            (0x1U << 2)
#define TIM_CCER_CC1P_POS                                              1
#define TIM_CCER_CC1P_MASK                                             (0x1U << 1)
#define TIM_CCER_CC1E_POS                                              0
#define TIM_CCER_CC1E_MASK                                             (0x1U << 0)

// TIM_CNT
#define TIM_CNT_CNT_POS                                                0
#define TIM_CNT_CNT_MASK                                               (0xffffU << 0)

// TIM_PSC
#define TIM_PSC_PSC_POS                                                0
#define TIM_PSC_PSC_MASK                                               (0xffffU << 0)

// TIM_ARR
#define TIM_ARR_ARR_POS                                                0
#define TIM_ARR_ARR_MASK                                               (0xffffU << 0)

// TIM_RCR
#define TIM_RCR_REP_POS                                                0
#define TIM_RCR_REP_MASK                                               (0xffU << 0)

// TIM_CCR1
#define TIM_CCR1_POS                                                   0
#define TIM_CCR1_MASK                                                  (0xffffU << 0)

// TIM_CCR2
#define TIM_CCR2_POS                                                   0
#define TIM_CCR2_MASK                                                  (0xffffU << 0)

// TIM_CCR3
#define TIM_CCR3_POS                                                   0
#define TIM_CCR3_MASK                                                  (0xffffU << 0)

// TIM_CCR4
#define TIM_CCR4_POS                                                   0
#define TIM_CCR4_MASK                                                  (0xffffU << 0)

// TIM_BDTR
#define TIM_BDTR_MOE_POS                                               15
#define TIM_BDTR_MOE_MASK                                              (0x1U << 15)     // Main output enable
#define TIM_BDTR_AOE_POS                                               14
#define TIM_BDTR_AOE_MASK                                              (0x1U << 14)     // Automatic output enable
#define TIM_BDTR_BKP_POS                                               13
#define TIM_BDTR_BKP_MASK                                              (0x1U << 13)     // Break polarity
#define TIM_BDTR_BKE_POS                                               12
#define TIM_BDTR_BKE_MASK                                              (0x1U << 12)     // Break enable
#define TIM_BDTR_OSSR_POS                                              11
#define TIM_BDTR_OSSR_MASK                                             (0x1U << 11)     // Off-state selection for Run mode
#define TIM_BDTR_OSSI_POS                                              10
#define TIM_BDTR_OSSI_MASK                                             (0x1U << 10)     // Off-state selection for Idle mode
#define TIM_BDTR_LOCK_POS                                              8
#define TIM_BDTR_LOCK_MASK                                             (0x3U << 8)      // Lock configuratio
#define TIM_BDTR_DTG_POS                                               0
#define TIM_BDTR_DTG_MASK                                              (0xffU << 0)     // Dead-time generator setup

// TIM_DCR
#define TIM_DCR_DBL_POS                                                8
#define TIM_DCR_DBL_MASK                                               (0x1fU << 8)     // DMA burst length
#define TIM_DCR_DBA_POS                                                0
#define TIM_DCR_DBA_MASK                                               (0x1fU << 0)     // DMA base address

// TIM_DMAR
#define TIM_DMAR_DMAB_POS                                              0
#define TIM_DMAR_DMAB_MASK                                             (0xffffU << 0)   // DMA register for burst accesses



/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    __IO uint32_t     CTR1;                     /*!< TIM control register 1,              Address offset: 0x00 */
    __IO uint32_t     CTR2;                     /*!< TIM control register 2,              Address offset: 0x04 */
    __IO uint32_t     SMCR;                     /*!< TIM slave mode control register,     Address offset: 0x08 */
    __IO uint32_t     DIER;                     /*!< TIM DMA/interrupt enable register,   Address offset: 0x0C */
    __IO uint32_t     SR;                       /*!< TIM status register,                 Address offset: 0x10 */
    __IO uint32_t     EGR;                      /*!< TIM event generation register,       Address offset: 0x14 */
    __IO uint32_t     CCMR1;                    /*!< TIM capture/compare mode register 1, Address offset: 0x18 */
    __IO uint32_t     CCMR2;                    /*!< TIM capture/compare mode register 2, Address offset: 0x1C */
    __IO uint32_t     CCER;                     /*!< TIM capture/compare enable register, Address offset: 0x20 */
    __IO uint32_t     CNT;                      /*!< TIM counter register,                Address offset: 0x24 */
    __IO uint32_t     PSC;                      /*!< TIM prescaler,                       Address offset: 0x28 */
    __IO uint32_t     ARR;                      /*!< TIM auto-reload register,            Address offset: 0x2C */
    __IO uint32_t     RCR;                      /*!< TIM repetition counter register,     Address offset: 0x30 */
    __IO uint32_t     CCR[4];                   /*!< TIM capture/compare register 0,1,2   Address offset: 0x34,0x38,0x3C 0x40 */
    __IO uint32_t     BDTR;                     /*!< TIM break and dead-time register,    Address offset: 0x44 */
    __IO uint32_t     DCR;                      /*!< TIM DMA control register,            Address offset: 0x48 */
    __IO uint32_t     DMAR;                     /*!< TIM DMA address for full transfer,   Address offset: 0x4C */
} OM_TIM_Type;


/*******************************************************************************
 * EXTERN VARIABLES
 */


/*******************************************************************************
 * EXTERN FUNCTIONS
 */


#ifdef __cplusplus
}
#endif


#endif  /* __TIMER_REG_H */


/** @} */

