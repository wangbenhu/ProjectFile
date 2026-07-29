/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup BTPHY BTPHY
 * @ingroup  DEVICE
 * @brief    BTPHY register
 * @details  BTPHY register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __BTPHY_REG_H
#define __BTPHY_REG_H


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
#define PHY_RX_CTRL_DPLL_CFO_RANGE_POS                      20
#define PHY_RX_CTRL_DPLL_CFO_RANGE_MASK                     (0x3U << 20U)

#define PHY_PWR_CNT_TH_32BIT_POS                            16
#define PHY_PWR_CNT_TH_32BIT_MASK                           (0x3U << 16U)

#define PHY_PWR_AMP_TH_2P4G_POS                             12
#define PHY_PWR_AMP_TH_2P4G_MASK                            (0xFU << 12U)

#define PHY_PWR_AMP_TH_0P50_POS                             8
#define PHY_PWR_AMP_TH_0P50_MASK                            (0xFU << 8U)

#define PHY_FE_BUF_RD_OFFSET_POS                            4
#define PHY_FE_BUF_RD_OFFSET_MASK                           (0x7U << 4U)

#define PHY_EN_PER_COND_POS                                 2
#define PHY_EN_PER_COND_MASK                                (0x1U << 2U)

#define PHY_SINGLE_ADC_SEL_POS                              1
#define PHY_SINGLE_ADC_SEL_MASK                             (0x1U << 1U)

#define PHY_RVS_IF_ROT_POS                                  0
#define PHY_RVS_IF_ROT_MASK                                 (0x1U << 0U)

#define PHY_DET_MODE_DET_MODE_POS                           0
#define PHY_DET_MODE_DET_MODE_MASK                          (0x3U << 0U)

#define PHY_TX_CTRL0_BDR_PPM_TX_POS                        0
#define PHY_TX_CTRL0_BDR_PPM_TX_MASK                       (0x3FFFFU << 0U)

#define PHY_TX_CTRL0_EN_INTERP_POS                         20
#define PHY_TX_CTRL0_EN_INTERP_MASK                        (0x1U << 20U)

#define PHY_TX_CTRL0_BP_GAU_POS                            22
#define PHY_TX_CTRL0_BP_GAU_MASK                           (0x1U << 22U)

#define PHY_TX_CTRL0_COEF_GFSK_POS                         24
#define PHY_TX_CTRL0_COEF_GFSK_MASK                        (0x1FU << 24U)

#define PHY_TX_CTRL0_TX_GFSK_MODE_POS                      29
#define PHY_TX_CTRL0_TX_GFSK_MODE_MASK                     (0x7U << 29U)

#define PHY_TX_CTRL1_NDUTY_POS                             0
#define PHY_TX_CTRL1_NDUTY_MASK                            (0x3U << 0U)
#define PHY_TX_CTRL1_GF_GCMP1_POS                          4
#define PHY_TX_CTRL1_GF_GCMP1_MASK                         (0xFU << 4U)
#define PHY_TX_CTRL1_GF_GCMP2_POS                          8
#define PHY_TX_CTRL1_GF_GCMP2_MASK                         (0xFU << 8U)

#define PHY_DPLL_DET_SEL_POS                               0
#define PHY_DPLL_DET_SEL_MASK                              (0x1U << 0U)

// reg: fysnc_det_info: 0x56*4
#define PHY_FYSNC_DET_INFO_FYSNC_DET_ID_POS                0
#define PHY_FYSNC_DET_INFO_FYSNC_DET_ID_MASK               (0x1U << 0U)
#define PHY_FYSNC_DET_INFO_FYSNC_CFO_EST_POS               16
#define PHY_FYSNC_DET_INFO_FYSNC_CFO_EST_MASK              (0xFFFFU << 16U)

#define PHY_STR_CTRL_EN_STR_POS                            0
#define PHY_STR_CTRL_EN_STR_MASK                           (0x1U << 0U)

#define PHY_STR_CTRL_BDR_PPM_RX_POS                        16
#define PHY_STR_CTRL_BDR_PPM_RX_MASK                       (0xFFFFU << 16U)

#define PHY_RX_STR_K_GFSK_POS                              0
#define PHY_RX_STR_K_GFSK_MASK                             (0x7U << 0U)
#define PHY_RX_STR_K_2P4G_POS                              4
#define PHY_RX_STR_K_2P4G_MASK                             (0x7U << 4U)
#define PHY_RX_STR_K_2P4G_ARB_POS                          8
#define PHY_RX_STR_K_2P4G_ARB_MASK                         (0x7U << 8U)

#define PHY_FD_CFO_CMP_FD_CFO_CMP_POS                      0
#define PHY_FD_CFO_CMP_FD_CFO_CMP_MASK                     (0x1U << 0U)

// reg: iq_comp addr: 0x52*4
#define PHY_IQ_COMP_TXIQ_COMP_PH_POS                        24
#define PHY_IQ_COMP_TXIQ_COMP_PH_MASK                      (0x7fU << 24U)
#define PHY_IQ_COMP_TXIQ_COMP_GAIN_POS                      16
#define PHY_IQ_COMP_TXIQ_COMP_GAIN_MASK                    (0xffU << 16U)
#define PHY_IQ_COMP_RXIQ_COMP_PH_POS                        8
#define PHY_IQ_COMP_RXIQ_COMP_PH_MASK                      (0x7fU << 8U)
#define PHY_IQ_COMP_RXIQ_COMP_GAIN_POS                      0
#define PHY_IQ_COMP_RXIQ_COMP_GAIN_MASK                    (0x7fU << 0U)

// reg: rssi_est_real_time 0x0F*4
#define PHY_RSSI_EST_REAL_TIME_POS                      0
#define PHY_RSSI_EST_REAL_TIME_MASK                     (0x1U << 0U)

// reg: rssi_cap_mode: 0x11*4
#define PHY_RSSI_CAP_MODE_POS                           0
#define PHY_RSSI_CAP_MODE_MASK                          (0x1fU << 0U)

// reg: rssi_est_real_time 0x33*4
#define PHY_RSSI_SIG_DBM_EST_O_POS                      0
#define PHY_RSSI_SIG_DBM_EST_O_MASK                     (0xFFU << 0U)

// reg: rssi_cap_mode: 0x53*4
#define PHY_RSSI_EST_EN_POS                             0
#define PHY_RSSI_EST_EN_MASK                            (0x1U << 0U)

// reg: reg_phy_rst_n addr: 0x38*4
#define PHY_REG_PHY_RST_N_REG_PHY_RST_N_POS                 0
#define PHY_REG_PHY_RST_N_REG_PHY_RST_N_MASK               (0x1U << 0U)

#define PHY_H_RX_CTRL_FXP_POS                                0
#define PHY_H_RX_CTRL_RVS_FXP_POS                            8
#define PHY_H_RX_CTRL_FXP_MASK                               (0xFFU << 0U)
#define PHY_H_RX_CTRL_RVS_FXP_MASK                           (0xFFU << 8U)


// reg: rx_gfsk_sync_ctrl addr: 0x39*4
#define PHY_RX_GFSK_SYNC_CTRL_GFSK_DEC_BKOFF_LEN_POS        28
#define PHY_RX_GFSK_SYNC_CTRL_EN_SBE_32_POS                 24
#define PHY_RX_GFSK_SYNC_CTRL_SBE_MAX_TH_32_POS             16
#define PHY_RX_GFSK_SYNC_CTRL_EN_2ND_ADDR_POS               8
#define PHY_RX_GFSK_SYNC_CTRL_XCORR_TH_32_POS               0

#define PHY_RX_GFSK_SYNC_CTRL_GFSK_DEC_BKOFF_LEN_MASK      (0xfU << 28U)
#define PHY_RX_GFSK_SYNC_CTRL_EN_SBE_32_MASK               (0x1U << 24U)
#define PHY_RX_GFSK_SYNC_CTRL_SBE_MAX_TH_32_MASK           (0xfU << 16U)
#define PHY_RX_GFSK_SYNC_CTRL_EN_2ND_ADDR_MASK             (0x1U << 8U)
#define PHY_RX_GFSK_SYNC_CTRL_XCORR_TH_32_MASK             (0x7fU << 0U)

///TONE_SUPPRESSION_CTRL
#define PHY_TONE_SUPPRESSION_CTRL_EN_TS_POS                0
#define PHY_TONE_SUPPRESSION_CTRL_TS_EN_GAIN_COND_POS      4
#define PHY_TONE_SUPPRESSION_CTRL_TS_MAG_LIM_POS           8
#define PHY_TONE_SUPPRESSION_CTRL_TS_MODE_POS              12
#define PHY_TONE_SUPPRESSION_CTRL_TS_MAG_EN_POS            16
#define PHY_TONE_SUPPRESSION_CTRL_TS_K_POS                 20
#define PHY_TONE_SUPPRESSION_CTRL_TS_EST_DUR_POS           24
#define PHY_TONE_SUPPRESSION_CTRL_EN_TS_MASK               (0x3U << 0U)
#define PHY_TONE_SUPPRESSION_CTRL_TS_EN_GAIN_COND_MASK     (0x1U << 4U)
#define PHY_TONE_SUPPRESSION_CTRL_TS_MAG_LIM_MASK          (0x7U << 8U)
#define PHY_TONE_SUPPRESSION_CTRL_TS_MODE_MASK             (0x1U << 12U)
#define PHY_TONE_SUPPRESSION_CTRL_TS_MAG_EN_MASK           (0x1U << 16U)
#define PHY_TONE_SUPPRESSION_CTRL_TS_K_MASK                (0x7U << 20U)
#define PHY_TONE_SUPPRESSION_CTRL_TS_EST_DUR_MASK          (0x7U << 26U)

/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    __IO uint32_t DET_MODE;                   // offset: 0x00*4 = 0x00  (Auto Restore)
         uint32_t RESERVE1[1];                // offset: 0x01*4 = 0x04
    __IO uint32_t DET_OFFSET;                 // offset: 0x02*4 = 0x08
    __IO uint32_t QML_CTRL;                   // offset: 0x03*4 = 0x0C
    __IO uint32_t EN_FAGC;                    // offset: 0x04*4 = 0x10
    __IO uint32_t EN_ROT2;                    // offset: 0x05*4 = 0x14
    __IO uint32_t EN_DC_REMOVAL;              // offset: 0x06*4 = 0x18
         uint32_t RESERVE2[1];                // offset: 0x07*4 = 0x1C
    __IO uint32_t RX_RFGAIN_MAX;              // offset: 0x08*4 = 0x20
    __IO uint32_t RESERVE3[3];                // offset: 0x09*4 = 0x24
    __IO uint32_t DCNOTCH_K;                  // offset: 0x0C*4 = 0x30
    __IO uint32_t DCNOTCH_K2;                 // offset: 0x0D*4 = 0x34
    __IO uint32_t RSSI_K;                     // offset: 0x0E*4 = 0x38
    __IO uint32_t RSSI_EST_REAL_TIME;         // offset: 0x0F*4 = 0x3C
    __IO uint32_t RESERVE4[1];                // offset: 0x10*4 = 0x40
    __IO uint32_t RSSI_CAP_MODE;              // offset: 0x11*4 = 0x44
    __IO uint32_t RSSI_TIMEOUT_CNST;          // offset: 0x12*4 = 0x48
    __IO uint32_t RESERVE5[1];                // offset: 0x13*4 = 0x4C
    __IO uint32_t FAGC_REF_DB;                // offset: 0x14*4 = 0x50
    __IO uint32_t FAGC_LWIN;                  // offset: 0x15*4 = 0x54
    __IO uint32_t FAGC_UPD_MODE;              // offset: 0x16*4 = 0x58
         uint32_t RESERVE6[9];                // offset: 0x17*4 = 0x5C
    __IO uint32_t BW_K;                       // offset: 0x20*4 = 0x80
    __IO uint32_t RX_STR_K;                   // offset: 0x21*4 = 0x84
    __IO uint32_t FD_CFO_CMP;                 // offset: 0x22*4 = 0x88  (Auto Restore)
    __IO uint32_t STR_CTRL;                   // offset: 0x23*4 = 0x8C  (Auto Restore)
         uint32_t RESERVE7[3];                // offset: 0x24*4 = 0x90
    __IO uint32_t SDET_A;                     // offset: 0x27*4 = 0x9C
    __IO uint32_t SDET_B;                     // offset: 0x28*4 = 0xA0
    __IO uint32_t EN_CFO_TRAC_SDET;           // offset: 0x29*4 = 0xA4
         uint32_t RESERVE8[9];                // offset: 0x2A*4 = 0xA8
    __IO uint32_t SIG_DBM_EST_O;              // offset: 0x33*4 = 0xCC
    __IO uint32_t RX_EN_REG;                  // offset: 0x34*4 = 0xD0
    __IO uint32_t IQ_IN_SWAP;                 // offset: 0x35*4 = 0xD4
    __IO uint32_t RX_INV_CLK;                 // offset: 0x36*4 = 0xD8
         uint32_t RESERVE9[1];                // offset: 0x37*4 = 0xDC
    __IO uint32_t REG_PHY_RST_N;              // offset: 0x38*4 = 0xE0
    __IO uint32_t RX_GFSK_SYNC_CTRL;          // offset: 0x39*4 = 0xE4  (Auto Restore)
         uint32_t RESERVE10[0x4b-0x39-1];     // offset: 0x3A*4 = 0xE8
    __IO uint32_t CTRL_2829;                  // offset: 0x4b*4 = 0x12C
         uint32_t RESERVE11[1];               // offset: 0x4c*4 = 0x130
    __IO uint32_t RX_CTRL;                    // offset: 0x4d*4 = 0x134
    __IO uint32_t TX_CTRL0;                   // offset: 0x4e*4 = 0x138  (Auto Restore)
    __IO uint32_t TX_CTRL1;                   // offset: 0x4f*4 = 0x13C
         uint32_t RESERVE12[2];               // offset: 0x50*4 = 0x140
    __IO uint32_t IQ_COMP;                    // offset: 0x52*4 = 0x148
    __IO uint32_t RSSI_EST_EN;                // offset: 0x53*4 = 0x14C
    __IO uint32_t TONE_SUPPRESSION_CTRL;      // offset: 0x54*4 = 0x150
    __IO uint32_t DPLL_DET_SEL;               // offset: 0x55*4 = 0x154
    __IO uint32_t FYSNC_DET_INFO;             // offset: 0x56*4 = 0x158
    __IO uint32_t FYSNC_DET_ADDR;             // offset: 0x57*4 = 0x15C
    __IO uint32_t H_RX_CTRL;                  // offset: 0x58*4 = 0x160  (Auto Restore)
} OM_BTPHY_Type;


/*******************************************************************************
 * EXTERN VARIABLES
 */


/*******************************************************************************
 * EXTERN FUNCTIONS
 */


#ifdef __cplusplus
}
#endif


#endif  /* __BTPHY_REG_H */


/** @} */
