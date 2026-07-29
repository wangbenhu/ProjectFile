/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup DAIF DAIF
 * @ingroup  DEVICE
 * @brief    DAIF register
 * @details  DAIF register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DAIF_REG_H
#define __DAIF_REG_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include "../common/common_reg.h"


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    __IO uint32_t PD_CFG0;                      // offset: 4*0x000 = 0x00  (Auto Restore)
    __IO uint32_t PD_CFG1;                      // offset: 4*0x001 = 0x04
    __IO uint32_t PD_CFG2;                      // offset: 4*0x002 = 0x08
    __IO uint32_t PA_CNS;                       // offset: 4*0x003 = 0x0C  (Auto Restore)
    __IO uint32_t RC32K_TUN;                    // offset: 4*0x004 = 0x10
    __IO uint32_t CLK_CFG;                      // offset: 4*0x005 = 0x14
    __IO uint32_t ECO_REG0;                     // offset: 4*0x006 = 0x18
    __IO uint32_t ECO_REG1;                     // offset: 4*0x007 = 0x1C
    __IO uint32_t XTAL32M_INTRS;                // offset: 4*0x008 = 0x20
    __IO uint32_t FREQ_CFG0;                    // offset: 4*0x009 = 0x24  (Auto Restore)
    __IO uint32_t FREQ_CFG1;                    // offset: 4*0x00A = 0x28
    __IO uint32_t FREQ_CFG2;                    // offset: 4*0x00B = 0x2C
    __IO uint32_t VCO_CTRL0;                    // offset: 4*0x00C = 0x30  (Auto Restore)
    __IO uint32_t VCO_CTRL1;                    // offset: 4*0x00D = 0x34
    __IO uint32_t PLL_CTRL0;                    // offset: 4*0x00E = 0x38  (Auto Restore)
    __IO uint32_t PLL_CTRL1;                    // offset: 4*0x00F = 0x3C  (Auto Restore)
    __IO uint32_t PLL_CTRL2;                    // offset: 4*0x010 = 0x40  (Auto Restore)
    __IO uint32_t LNA_MIX_CFG;                  // offset: 4*0x011 = 0x44  (Auto Restore)
    __IO uint32_t TIA_LPF_CFG;                  // offset: 4*0x012 = 0x48  (Auto Restore)
    __IO uint32_t RC32M_TUN;                    // offset: 4*0x013 = 0x4C
    __IO uint32_t FREQ_CFG3;                    // offset: 4*0x014 = 0x50  (Auto Restore)
    __IO uint32_t RX_RCCAL_CTRL;                // offset: 4*0x015 = 0x54
    __IO uint32_t MIX_CFG0;                     // offset: 4*0x016 = 0x58
    __IO uint32_t MAIN_ST_CFG0;                 // offset: 4*0x017 = 0x5C  (Auto Restore)
    __IO uint32_t MAIN_ST_CFG1;                 // offset: 4*0x018 = 0x60
    __IO uint32_t MAIN_ST_CFG2;                 // offset: 4*0x019 = 0x64
    __IO uint32_t AGC_CFG0;                     // offset: 4*0x01A = 0x68  (Auto Restore)
    __IO uint32_t AGC_CFG1;                     // offset: 4*0x01B = 0x6C  (Auto Restore)
    __IO uint32_t MIX_CFG1;                     // offset: 4*0x01C = 0x70
    __IO uint32_t PLL_PHY_CTRL;                 // offset: 4*0x01D = 0x74
    __IO uint32_t IR_RX_CFG;                    // offset: 4*0x01E = 0x78
    __IO uint32_t TIA_DCOC_CFG;                 // offset: 4*0x01F = 0x7C  (Auto Restore)
    __IO uint32_t SYSPLL_CNS0;                  // offset: 4*0x020 = 0x80
    __IO uint32_t SYSPLL_CNS1;                  // offset: 4*0x021 = 0x84
    __IO uint32_t RF_CFG;                       // offset: 4*0x022 = 0x88
    __IO uint32_t RF_TX_DATA;                   // offset: 4*0x023 = 0x8C
    __IO uint32_t FILT_AGC_LUT_REG0;            // offset: 4*0x024 = 0x90  (Auto Restore)
    __IO uint32_t FILT_AGC_LUT_REG1;            // offset: 4*0x025 = 0x94  (Auto Restore)
    __IO uint32_t FILT_AGC_LUT_REG2;            // offset: 4*0x026 = 0x98  (Auto Restore)
    __IO uint32_t RX_GAINC_LUT_REG0;            // offset: 4*0x027 = 0x9C
    __IO uint32_t RX_GAINC_LUT_REG1;            // offset: 4*0x028 = 0xA0
    __IO uint32_t RESERVED_xA4_xBC[7];          // offset: 4*(0x02F - 0x29)
    __IO uint32_t PA_CTRL;                      // offset: 4*0x030 = 0xC0
    __IO uint32_t RSVD_xC4;                     // offset: 4*0x031 = 0xC4
    __IO uint32_t TRX_SW_CFG;                   // offset: 4*0x032 = 0xC8  (Auto Restore)
    __IO uint32_t RSVD_xCC;                     // offset: 4*0x033 = 0xCC
    __IO uint32_t CLK_ENS;                      // offset: 4*0x034 = 0xD0
    __IO uint32_t PLL_LUT_DBG;                  // offset: 4*0x035 = 0xD4
    __IO uint32_t RESERVED_XD8;                 // offset: 4*0x036 = 0xD8
    __IO uint32_t ANA_TST_CTRL;                 // offset: 4*0x037 = 0xDC
         uint32_t RSVD_xE0_xF0[5];
    __IO uint32_t MON_DMA_CFG0;                 // offset: 4*0x03D = 0xF4
    __IO uint32_t MON_DMA_CFG1;                 // offset: 4*0x03E = 0xF8
    __IO uint32_t MON_DMA_CFG2;                 // offset: 4*0x03F = 0xFC
    __IO uint32_t DBG_REG;                      // offset: 4*0x040 = 0x100
    __IO uint32_t HS6621CB_CFG;                 // offset: 4*0x041 = 0x104
    __IO uint32_t KDCO_LUT_1M_REG0;             // offset: 4*0x042 = 0x108  (Auto Restore)
    __IO uint32_t KDCO_LUT_1M_REG1;             // offset: 4*0x043 = 0x10C  (Auto Restore)
    __IO uint32_t KDCO_LUT_2M_REG0;             // offset: 4*0x044 = 0x110  (Auto Restore)
    __IO uint32_t KDCO_LUT_2M_REG1;             // offset: 4*0x045 = 0x114  (Auto Restore)
    __IO uint32_t KDCO_LUT_1M_REG2;             // offset: 4*0x046 = 0x118  (Auto Restore)
         uint32_t RSVD_x11C_x18C[29];           // offset: 4*(0x063 - 0x047)
    __IO uint32_t PLL_AFC_CTRL;                 // offset: 4*0x064 = 0x190
    __IO uint32_t RC32K_TUN_OUT;                // offset: 4*0x065 = 0x194
    __IO uint32_t TRX_EXT_PD_CFG;               // offset: 4*0x066 = 0x198
    __IO uint32_t CLK_CHK_CNS;                  // offset: 4*0x067 = 0x19C
    __IO uint32_t CLK_CHK_STATUS;               // offset: 4*0x068 = 0x1A0
         uint32_t RESERVE6[0x400-0x068-1];
    __IO uint32_t TIA_CR1;                      // offset: 4*0x400 = 0x1000
    __IO uint32_t TIA_CR2;                      // offset: 4*0x401 = 0x1004
    __IO uint32_t RXADC_STATUS;                 // offset: 4*0x402 = 0x1008
    __IO uint32_t TIA_DCOC_R1;                  // offset: 4*0x403 = 0x100c
    __IO uint32_t TIA_DCOC_R2;                  // offset: 4*0x404 = 0x1010
    __IO uint32_t TIA_DCOC_R3;                  // offset: 4*0x405 = 0x1014
         uint32_t RESERVE7[0xC00-0x405-1];
    __IO uint32_t PA_GAIN_REG_9;                // offset: 4*0xC00 = 0x3000
    __IO uint32_t PA_GAIN_REG_8;                // offset: 4*0xC01 = 0x3004
    __IO uint32_t PA_GAIN_REG_7;                // offset: 4*0xC02 = 0x3008
    __IO uint32_t PA_GAIN_REG_6;                // offset: 4*0xC03 = 0x300C
    __IO uint32_t PA_GAIN_REG_5;                // offset: 4*0xC04 = 0x3010
    __IO uint32_t PA_GAIN_REG_4;                // offset: 4*0xC05 = 0x3014
    __IO uint32_t PA_GAIN_REG_3;                // offset: 4*0xC06 = 0x3018
    __IO uint32_t PA_GAIN_REG_2;                // offset: 4*0xC07 = 0x301C
    __IO uint32_t PA_GAIN_REG_1;                // offset: 4*0xC08 = 0x3020
    __IO uint32_t PA_GAIN_CFG;                  // offset: 4*0xC09 = 0x3024
} OM_DAIF_Type;


/*******************************************************************************
 * MACROS
 */
// PD_CFG0
#define DAIF_PD_RAMP_CTRL_MO_POS                         0
#define DAIF_RF_CONSTGM_PD_MO_POS                        1
#define DAIF_TRS_LDO_PD_MO_POS                           2
#define DAIF_RF_LNA_PD_MO_POS                            3
#define DAIF_RF_LDO_PD_MO_POS                            4
#define DAIF_PA_CALI_PD_MO_POS                           6
#define DAIF_PKDT_PD_MO_POS                              8
#define DAIF_RF_MIX_PD_MO_POS                            9
#define DAIF_TIA_PD_MO_POS                               10
#define DAIF_FILTER_PD_MO_POS                            11
#define DAIF_RXADC_PD_MO_POS                             12
#define DAIF_RXADC_LDO_PD_MO_POS                         13
#define DAIF_PA_ALL_PD_MO_POS                            14
#define DAIF_PA_LDO_PD_MO_POS                            15
#define DAIF_PD_RAMP_CTRL_ME_POS                         16
#define DAIF_RF_CONSTGM_PD_ME_POS                        17
#define DAIF_TRS_LDO_PD_ME_POS                           18
#define DAIF_RF_LNA_PD_ME_POS                            19
#define DAIF_RF_LDO_PD_ME_POS                            20
#define DAIF_GPADC_CALI_EN_VBAT_POS                      21
#define DAIF_PA_CALI_PD_ME_POS                           22
#define DAIF_PKDT_PD_ME_POS                              24
#define DAIF_RF_MIX_PD_ME_POS                            25
#define DAIF_TIA_PD_ME_POS                               26
#define DAIF_FILTER_PD_ME_POS                            27
#define DAIF_RXADC_PD_ME_POS                             28
#define DAIF_RXADC_LDO_PD_ME_POS                         29
#define DAIF_PA_ALL_PD_ME_POS                            30
#define DAIF_PA_LDO_PD_ME_POS                            31
#define DAIF_PD_RAMP_CTRL_MO_MASK                        0x00000001
#define DAIF_RF_CONSTGM_PD_MO_MASK                       0x00000002
#define DAIF_TRS_LDO_PD_MO_MASK                          0x00000004
#define DAIF_RF_LNA_PD_MO_MASK                           0x00000008
#define DAIF_RF_LDO_PD_MO_MASK                           0x00000010
#define DAIF_PA_CALI_PD_MO_MASK                          0x00000040
#define DAIF_PKDT_PD_MO_MASK                             0x00000100
#define DAIF_RF_MIX_PD_MO_MASK                           0x00000200
#define DAIF_TIA_PD_MO_MASK                              0x00000400
#define DAIF_FILTER_PD_MO_MASK                           0x00000800
#define DAIF_RXADC_PD_MO_MASK                            0x00001000
#define DAIF_RXADC_LDO_PD_MO_MASK                        0x00002000
#define DAIF_PA_ALL_PD_MO_MASK                           0x00004000
#define DAIF_PA_LDO_PD_MO_MASK                           0x00008000
#define DAIF_PD_RAMP_CTRL_ME_MASK                        0x00010000
#define DAIF_RF_CONSTGM_PD_ME_MASK                       0x00020000
#define DAIF_TRS_LDO_PD_ME_MASK                          0x00040000
#define DAIF_RF_LNA_PD_ME_MASK                           0x00080000
#define DAIF_RF_LDO_PD_ME_MASK                           0x00100000
#define DAIF_GPADC_CALI_EN_VBAT_MASK                     0x00200000
#define DAIF_PA_CALI_PD_ME_MASK                          0x00400000
#define DAIF_PKDT_PD_ME_MASK                             0x01000000
#define DAIF_RF_MIX_PD_ME_MASK                           0x02000000
#define DAIF_TIA_PD_ME_MASK                              0x04000000
#define DAIF_FILTER_PD_ME_MASK                           0x08000000
#define DAIF_RXADC_PD_ME_MASK                            0x10000000
#define DAIF_RXADC_LDO_PD_ME_MASK                        0x20000000
#define DAIF_PA_ALL_PD_ME_MASK                           0x40000000
#define DAIF_PA_LDO_PD_ME_MASK                           0x80000000
// PD_CFG1
#define DAIF_RFPLL_PD_ALL_MO_POS                         0
#define DAIF_IF_LDO_PD_MO_POS                            1
#define DAIF_RFPLL_PD_LOTX_MO_POS                        3
#define DAIF_RFPLL_PD_VCDET_MO_POS                       4
#define DAIF_PKDT_RST_MO_POS                             6
#define DAIF_RFPLL_PD_BUF_POS                            9
#define DAIF_PA_BIAS_PD_POS                              10
#define DAIF_PA_DRV0_PD_POS                              11
#define DAIF_PMU_PD_SL_POS                               12
#define DAIF_PMU_PD_TS_POS                               13
#define DAIF_RFPLL_PD_ALL_ME_POS                         16
#define DAIF_IF_LDO_PD_ME_POS                            17
#define DAIF_RFPLL_PD_LOTX_ME_POS                        19
#define DAIF_RFPLL_PD_VCDET_ME_POS                       20
#define DAIF_PKDT_RST_ME_POS                             22
#define DAIF_RFPLL_PD_ALL_MO_MASK                        0x00000001
#define DAIF_IF_LDO_PD_MO_MASK                           0x00000002
#define DAIF_RFPLL_PD_LOTX_MO_MASK                       0x00000008
#define DAIF_RFPLL_PD_VCDET_MO_MASK                      0x00000010
#define DAIF_PKDT_RST_MO_MASK                            0x00000040
#define DAIF_RFPLL_PD_BUF_MASK                           0x00000200
#define DAIF_PA_BIAS_PD_MASK                             0x00000400
#define DAIF_PA_DRV0_PD_MASK                             0x00000800
#define DAIF_PMU_PD_SL_MASK                              0x00001000
#define DAIF_PMU_PD_TS_MASK                              (1U << 13)
#define DAIF_RFPLL_PD_ALL_ME_MASK                        0x00010000
#define DAIF_IF_LDO_PD_ME_MASK                           0x00020000
#define DAIF_RFPLL_PD_LOTX_ME_MASK                       0x00080000
#define DAIF_RFPLL_PD_VCDET_ME_MASK                      0x00100000
#define DAIF_PKDT_RST_ME_MASK                            0x00400000
// PD_CFG2
#define DAIF_RFPLL_PD_TXDAC_ME_POS                       0
#define DAIF_RFPLL_PD_PFD_POS                            1
#define DAIF_RFPLL_PD_TXDAC_MO_POS                       2
#define DAIF_RFPLL_PD_VCO_POS                            5
#define DAIF_RFPLL_PD_TXDAC_ME_MASK                      0x00000001
#define DAIF_RFPLL_PD_PFD_MASK                           0x00000002
#define DAIF_RFPLL_PD_TXDAC_MO_MASK                      0x00000004
#define DAIF_RFPLL_PD_VCO_MASK                           0x00000020
// PA_CNS
#define DAIF_PA_DRV0_NCT_MO_POS                          0
#define DAIF_PA_DRV0_PCT_MO_POS                          5
#define DAIF_PA_CORE_NUM_MO_POS                          10
#define DAIF_PA_DRIV_NUM_MO_POS                          16
#define DAIF_EN_BYPASS_PALDO_POS                         22
#define DAIF_PA_DBG_POS                                  23
#define DAIF_PA_GAIN_IDX_REG_POS                         24
#define DAIF_PA_CALI_OUT_POS                             31
#define DAIF_PA_DRV0_NCT_MO_MASK                         0x0000001F
#define DAIF_PA_DRV0_PCT_MO_MASK                         0x000003E0
#define DAIF_PA_CORE_NUM_MO_MASK                         0x0000FC00
#define DAIF_PA_DRIV_NUM_MO_MASK                         0x003F0000
#define DAIF_EN_BYPASS_PALDO_MASK                        0x00400000
#define DAIF_PA_DBG_MASK                                 0x00800000
#define DAIF_PA_GAIN_IDX_REG_MASK                        0x3F000000
#define DAIF_PA_CALI_OUT_MASK                            0x80000000
// RC32K_TUNE
#define DAIF_RC_32K_TUNE_START_POS                       0
#define DAIF_RST_RCOSC_32K_POS                           2
#define DAIF_RC_32K_TUNE_SWRSTN_POS                      3
#define DAIF_RC_32K_TUNE_WIN_CNT_THRESHOLD_POS           4
#define DAIF_RC_32K_TUNE_TARGET_REG_POS                  12
#define DAIF_RC_32K_TUNE_START_MASK                      0x00000001
#define DAIF_RST_RCOSC_32K_MASK                          0x00000004
#define DAIF_RC_32K_TUNE_SWRSTN_MASK                     0x00000008
#define DAIF_RC_32K_TUNE_WIN_CNT_THRESHOLD_MASK          0x00000FF0
#define DAIF_RC_32K_TUNE_TARGET_REG_MASK                 0x3FFFF000
// CLK_CFG
#define DAIF_XTAL32M_EN_CKO16M_DIG_POS                   0
#define DAIF_XTAL32M_EN_CKO16M_DIG_MASK                  (1U << 0)
#define DAIF_XTAL32M_EN_CKO16M_ANA_POS                   1
#define DAIF_XTAL32M_EN_CKO16M_ANA_MASK                  (1U << 1)
#define DAIF_XTAL32M_EN_CKO16M_SYSPLL_POS                2
#define DAIF_XTAL32M_EN_CKO16M_SYSPLL_MASK               (1U << 2)
#define DAIF_XTAL32M_EN_CKO16M_PLL_POS                   3
#define DAIF_XTAL32M_EN_CKO16M_PLL_MASK                  (1U << 3)
#define DAIF_XTAL32M_SEL_GPADC_CLK_POS                   4
#define DAIF_XTAL32M_SEL_GPADC_CLK_MASK                  (1U << 4)
#define DAIF_XTAL32M_EN_CKO16M_GPADC_POS                 5
#define DAIF_XTAL32M_EN_CKO16M_GPADC_MASK                (1U << 5)
#define DAIF_XTAL32M_EN_CKO16M_AUDIO_POS                 6
#define DAIF_XTAL32M_EN_CKO16M_AUDIO_MASK                (1U << 6)
#define DAIF_XTAL32M_SEL_GPADC_CLK_32M_POS               7
#define DAIF_XTAL32M_SEL_GPADC_CLK_32M_MASK              (1U << 7)
// ECO_REG0
#define DAIF_ECO_REG0_POS                                0
#define DAIF_ECO_REG0_MASK                               0xFFFFFFFF
// ECO_REG1
#define DAIF_ECO_REG1_POS                                0
#define DAIF_ECO_REG1_MASK                               0xFFFFFFFF
// XTAL32M_INTR
#define DAIF_XTAL32M_INTRS_POS                           0
#define DAIF_XTAL32M_INTRS_MASK_POS                      1
#define DAIF_XTAL32M_CLK_RDY_POS                         15
#define DAIF_XTAL32M_INTRS_MASK                          0x00000001
#define DAIF_XTAL32M_INTRS_MASK_MASK                     0x00000002
#define DAIF_XTAL32M_CLK_RDY_MASK                        0x00008000
// FREQ_CFG0
#define DAIF_FREQ_REG_MO_POS                             0
#define DAIF_FREQ_REG_ME_POS                             12
#define DAIF_INTER_FREQ_2M_0_15_POS                      16
#define DAIF_FREQ_REG_MO_MASK                            0x00000FFF
#define DAIF_FREQ_REG_ME_MASK                            0x00001000
#define DAIF_INTER_FREQ_2M_0_15_MASK                     0xFFFF0000
// FREQ_CFG1
#define DAIF_INTER_FREQ_1M_POS                           0
#define DAIF_INTER_FREQ_1M_MASK                          0x01FFFFFF
// FREQ_CFG2
#define DAIF_FREQ_COR_POS                                0
#define DAIF_INTER_FREQ_2M_16_24_POS                     23
#define DAIF_FREQ_COR_MASK                               0x007FFFFF
#define DAIF_INTER_FREQ_2M_16_24_MASK                    0xFF800000
// VCO_CTRL0
#define DAIF_AFC_START_POS                               0
#define DAIF_KDCO_START_POS                              1
#define DAIF_VTRACK_MODE_POS                             3
#define DAIF_VTRACK_MO_POS                               4
#define DAIF_TRX_DBG_POS                                 7
#define DAIF_TX_TRACK_OUF_SYNC_POS                       8
#define DAIF_RX_TRACK_OUF_SYNC_POS                       9
#define DAIF_AFC_EN_MO_POS                               10
#define DAIF_AFC_EN_ME_POS                               11
#define DAIF_AFC_TUNE_MO_POS                             12
#define DAIF_RX_EN_MO_POS                                21
#define DAIF_TX_EN_MO_POS                                22
#define DAIF_AFC_TUNE_ME_POS                             23
#define DAIF_KDCO_MO_POS                                 24
#define DAIF_KDCO_ME_POS                                 31
#define DAIF_AFC_START_MASK                              0x00000001
#define DAIF_KDCO_START_MASK                             0x00000002
#define DAIF_VTRACK_MODE_MASK                            0x00000008
#define DAIF_VTRACK_MO_MASK                              0x00000070
#define DAIF_TRX_DBG_MASK                                0x00000080
#define DAIF_TX_TRACK_OUF_SYNC_MASK                      0x00000100
#define DAIF_RX_TRACK_OUF_SYNC_MASK                      0x00000200
#define DAIF_AFC_EN_MO_MASK                              0x00000400
#define DAIF_AFC_EN_ME_MASK                              0x00000800
#define DAIF_AFC_TUNE_MO_MASK                            0x000FF000
#define DAIF_RX_EN_MO_MASK                               0x00200000
#define DAIF_TX_EN_MO_MASK                               0x00400000
#define DAIF_AFC_TUNE_ME_MASK                            0x00800000
// VCO_CTRL1
#define DAIF_AFC_RAM_AUTO_GATE_POS                       1
#define DAIF_BLE_1M_2M_SEL_MO_POS                        4
#define DAIF_BLE_1M_2M_SEL_ME_POS                        5
#define DAIF_AFC_TARGET_BASE_POS                         16
#define DAIF_AFC_RAM_AUTO_GATE_MASK                      0x00000002
#define DAIF_BLE_1M_2M_SEL_MO_MASK                       0x00000010
#define DAIF_BLE_1M_2M_SEL_ME_MASK                       0x00000020
#define DAIF_AFC_TARGET_BASE_MASK                        0xFFFF0000
// PLL_CTRL0
#define DAIF_CTRL_LDO_PFD_POS                            0
#define DAIF_CTRL_LDO_BUF_POS                            2
#define DAIF_CTRL_LDO_VCO_POS                            5
#define DAIF_SEL_KVCO_POS                                8
#define DAIF_BP_DIOX_POS                                 10
#define DAIF_ICP_CTRL_POS                                11
#define DAIF_VREF_VCDET_POS                              14
#define DAIF_REG_FTUN_POS                                18
#define DAIF_EN_CP_TX_POS                                21
#define DAIF_SEL_AFC_VC_POS                              22
#define DAIF_SEL_DCTEST_POS                              24
#define DAIF_EN_DCTEST_POS                               27
#define DAIF_CTRL_LDO_PFD_MASK                           0x00000003
#define DAIF_CTRL_LDO_BUF_MASK                           0x0000001C
#define DAIF_CTRL_LDO_VCO_MASK                           0x000000E0
#define DAIF_SEL_KVCO_MASK                               0x00000300
#define DAIF_BP_DIOX_MASK                                0x00000400
#define DAIF_ICP_CTRL_MASK                               0x00003800
#define DAIF_VREF_VCDET_MASK                             0x0000C000
#define DAIF_REG_FTUN_MASK                               0x000C0000
#define DAIF_EN_CP_TX_MASK                               0x00200000
#define DAIF_SEL_AFC_VC_MASK                             0x00C00000
#define DAIF_SEL_DCTEST_MASK                             0x07000000
#define DAIF_EN_DCTEST_MASK                              0x08000000
// PLL_CTRL1
#define DAIF_RDPLL_SEL_VCO_IBIAS_POS                     0
#define DAIF_DIGI_DIN_REG_POS                            4
#define DAIF_DIGI_DIN_BYPASS_POS                         14
#define DAIF_CON_BIAS_IDAC_PLL_POS                       15
#define DAIF_FREQ_DEVIA_BYPASS_3RD_POS                   21
#define DAIF_FREQ_DEVIA_COEFF_3RD_POS                    22
#define DAIF_RDPLL_SEL_VCO_IBIAS_MASK                    0x0000000F
#define DAIF_DIGI_DIN_REG_MASK                           0x00003FF0
#define DAIF_DIGI_DIN_BYPASS_MASK                        0x00004000
#define DAIF_CON_BIAS_IDAC_PLL_MASK                      0x001F8000
#define DAIF_FREQ_DEVIA_BYPASS_3RD_MASK                  0x00200000
#define DAIF_FREQ_DEVIA_COEFF_3RD_MASK                   (0xFFU << 22)
// PLL_CTRL2
#define DAIF_DITHER_EN_POS                               0
#define DAIF_DITHER_IN_POS                               1
#define DAIF_ENB_INTEGER_SDM_POS                         2
#define DAIF_FREQ_DEVIA_DIV_POS                          3
#define DAIF_DATA_SYNC_BYPASS_POS                        6
#define DAIF_RFPLL_MOD_DIV_MO_POS                        7
#define DAIF_RFPLL_MOD_DIV_ME_POS                        14
#define DAIF_EN_KVCO2_POS                                20
#define DAIF_RFPLL_ENB_RX_DIV_MO_POS                     23
#define DAIF_RFPLL_ENB_RX_DIV_ME_POS                     24
#define DAIF_MOD_DIV_EDGE_SEL_POS                        25
#define DAIF_TX_SDM_EDGE_SEL_POS                         26
#define DAIF_PLL_CLK_FB_INV_POS                          27
#define DAIF_RFPLL_SEL_DACCLK_PLL_POS                    28
#define DAIF_DITHER_EN_MASK                              0x00000001
#define DAIF_DITHER_IN_MASK                              0x00000002
#define DAIF_ENB_INTEGER_SDM_MASK                        0x00000004
#define DAIF_FREQ_DEVIA_DIV_MASK                         0x00000018
#define DAIF_DATA_SYNC_BYPASS_MASK                       0x00000040
#define DAIF_RFPLL_MOD_DIV_MO_MASK                       0x00003F80
#define DAIF_RFPLL_MOD_DIV_ME_MASK                       0x00004000
#define DAIF_EN_KVCO2_MASK                               0x00700000
#define DAIF_RFPLL_ENB_RX_DIV_MO_MASK                    0x00800000
#define DAIF_RFPLL_ENB_RX_DIV_ME_MASK                    0x01000000
#define DAIF_MOD_DIV_EDGE_SEL_MASK                       0x02000000
#define DAIF_TX_SDM_EDGE_SEL_MASK                        0x04000000
#define DAIF_PLL_CLK_FB_INV_MASK                         0x08000000
#define DAIF_RFPLL_SEL_DACCLK_PLL_MASK                   0x10000000
// LNA_MIX_CFG
#define DAIF_RF_ATTEN_EN_MO_POS                          0
#define DAIF_RF_ATTEN_EN_ME_POS                          1
#define DAIF_RF_ATTEN_GAIN_MO_POS                        2
#define DAIF_RF_ATTEN_GAIN_ME_POS                        4
#define DAIF_RF_ATTEN_CLOAD_POS                          5
#define DAIF_RF_LNA_GAIN_MO_POS                          6
#define DAIF_RF_LNA_GAIN_ME_POS                          9
#define DAIF_RF_LNA_VBCT_POS                             10
#define DAIF_RF_MIX_VBCT_POS                             13
#define DAIF_RF_CONSTGM_CT_POS                           16
#define DAIF_RF_LNA_GNDING_POS                           18
#define DAIF_RF_ATTEN_EN_MO_MASK                         0x00000001
#define DAIF_RF_ATTEN_EN_ME_MASK                         0x00000002
#define DAIF_RF_ATTEN_GAIN_MO_MASK                       0x0000000C
#define DAIF_RF_ATTEN_GAIN_ME_MASK                       0x00000010
#define DAIF_RF_ATTEN_CLOAD_MASK                         0x00000020
#define DAIF_RF_LNA_GAIN_MO_MASK                         0x000001C0
#define DAIF_RF_LNA_GAIN_ME_MASK                         0x00000200
#define DAIF_RF_LNA_VBCT_MASK                            0x00000C00
#define DAIF_RF_MIX_VBCT_MASK                            0x0000E000
#define DAIF_RF_CONSTGM_CT_MASK                          0x00030000
#define DAIF_RF_LNA_GNDING_MASK                          0x00040000
// TIA_LPF_CFG
#define DAIF_FILTER_MOD_SEL_MO_POS                       1
#define DAIF_FILTER_MOD_SEL_ME_POS                       2
#define DAIF_FILTER_SWAP_POS                             4
#define DAIF_FILTER_IBCT_1M_POS                          5
#define DAIF_FILTER_IBCT_2M_POS                          7
#define DAIF_FILTER_GTUNE_MO_POS                         11
#define DAIF_FILTER_GTUNE_ME_POS                         14
#define DAIF_FILTER_MOD_SEL_MO_MASK                      0x00000002
#define DAIF_FILTER_MOD_SEL_ME_MASK                      0x00000004
#define DAIF_FILTER_SWAP_MASK                            0x00000010
#define DAIF_FILTER_IBCT_1M_MASK                         0x00000060
#define DAIF_FILTER_IBCT_2M_MASK                         0x00000180
#define DAIF_FILTER_GTUNE_MO_MASK                        0x00003800
#define DAIF_FILTER_GTUNE_ME_MASK                        0x00004000
// RC32M_TUN
#define DAIF_RC_32M_TUNE_START_POS                       0
#define DAIF_OSC32M_CT_POS                               8
#define DAIF_RC_32M_TUNE_START_MASK                      0x00000001
#define DAIF_OSC32M_CT_MASK                              0x0000FF00
// FREQ_CFG3
#define DAIF_FREQ_FRAC_REG_POS                           0
#define DAIF_FREQ_0P5_EN_POS                             31
#define DAIF_FREQ_FRAC_REG_MASK                          0x003FFFFF
#define DAIF_FREQ_0P5_EN_MASK                            0x80000000
// RX_RCCAL_CTRL
#define DAIF_RX_RCCAL_START_POS                          0
#define DAIF_RX_RCCAL_DLY1_POS                           1
#define DAIF_RX_RCCAL_DLY2_POS                           5
#define DAIF_RX_RCCAL_AVG_POS                            9
#define DAIF_RCCAL_PRELOAD_POS                           13
#define DAIF_RX_RCCAL_IQSEL_POS                          17
#define DAIF_RX_RCCAL_AVG_F2_POS                         18
#define DAIF_RX_RCCAL_REF_POS                            20
#define DAIF_RX_RCCAL_START_MASK                         0x00000001
#define DAIF_RX_RCCAL_DLY1_MASK                          0x0000001E
#define DAIF_RX_RCCAL_DLY2_MASK                          0x000001E0
#define DAIF_RX_RCCAL_AVG_MASK                           0x00000E00
#define DAIF_RCCAL_PRELOAD_MASK                          0x0001E000
#define DAIF_RX_RCCAL_IQSEL_MASK                         0x00020000
#define DAIF_RX_RCCAL_AVG_F2_MASK                        0x000C0000
#define DAIF_RX_RCCAL_REF_MASK                           0x3FF00000
// MIX_CFG0
#define DAIF_FREQ_DEVIA_COEFF_TX_POS                     0
#define DAIF_FREQ_DEVIA_COEFF_TX_MASK                    (0xFFU << 0)
#define DAIF_FREQ_DEVIA_BYPASS_TX_POS                    8
#define DAIF_FREQ_DEVIA_BYPASS_TX_MASK                   (1U << 8)
#define DAIF_SDM_DIV_POS                                 9
#define DAIF_SDM_DIV_MASK                                (1U << 9)
#define DAIF_VTRACK_EN_MO_POS                            10
#define DAIF_VTRACK_EN_MO_MASK                           (1U << 10)
#define DAIF_VTRACK_EN_ME_POS                            11
#define DAIF_VTRACK_EN_ME_MASK                           (1U << 11)
#define DAIF_XTAL32M_CT_SEL_MO_POS                       12
#define DAIF_XTAL32M_CT_SEL_MO_MASK                      (1U << 12)
#define DAIF_XTAL32M_CT_SEL_ME_POS                       13
#define DAIF_XTAL32M_CT_SEL_ME_MASK                      (1U << 13)
#define DAIF_PD_PHY_ADC_MO_POS                           14
#define DAIF_PD_PHY_ADC_MO_MASK                          (1U << 14)
#define DAIF_PD_PHY_ADC_ME_POS                           15
#define DAIF_PD_PHY_ADC_ME_MASK                          (1U << 15)
#define DAIF_PHY_ADC_WAIT_POS                            16
#define DAIF_PHY_ADC_WAIT_MASK                           (0x1FFU << 16)
#define DAIF_TIA_IBCT_POS                                25
#define DAIF_TIA_IBCT_MASK                               (3U << 25)
// MAIN_ST_CFG0
#define DAIF_TXLDO_WAIT_POS                              0
#define DAIF_TX_PLL_WAIT_POS                             16
#define DAIF_TXLDO_WAIT_MASK                             0x0000FFFF
#define DAIF_TX_PLL_WAIT_MASK                            0xFFFF0000
// MAIN_ST_CFG1
#define DAIF_PA_WAIT_POS                                 0
#define DAIF_RAMP_1US_POS                                8
#define DAIF_TX_WAIT_POS                                 9
#define DAIF_AGC_SYNC_TIME_POS                           12
#define DAIF_RXLDO_WAIT_POS                              16
#define DAIF_PA_WAIT_MASK                                0x000000FF
#define DAIF_RAMP_1US_MASK                               0x00000100
#define DAIF_TX_WAIT_MASK                                0x00000E00
#define DAIF_AGC_SYNC_TIME_MASK                          0x0000F000
#define DAIF_RXLDO_WAIT_MASK                             0xFFFF0000
// MAIN_ST_CFG2
#define DAIF_RX_PLL_WAIT_POS                             0
#define DAIF_TX_END_WAIT_POS                             16
#define DAIF_RX_END_WAIT_POS                             24
#define DAIF_RX_PLL_WAIT_MASK                            0x0000FFFF
#define DAIF_TX_END_WAIT_MASK                            0x00FF0000
#define DAIF_RX_END_WAIT_MASK                            0xFF000000
// AGC_CFG0
#define DAIF_PKDT_IQSEL_POS                              0
#define DAIF_PKDT_IBCT_POS                               1
#define DAIF_PKDT_V0CT_POS                               3
#define DAIF_PKDT_V1CT_POS                               5
#define DAIF_PKDT_V3CT_POS                               6
#define DAIF_PKDT_V4CT_POS                               7
#define DAIF_FILTER_MAX6DB_POS                           18
#define DAIF_PKDT_IQSEL_MASK                             0x00000001
#define DAIF_PKDT_IBCT_MASK                              0x00000006
#define DAIF_PKDT_V0CT_MASK                              0x00000018
#define DAIF_PKDT_V1CT_MASK                              0x00000020
#define DAIF_PKDT_V3CT_MASK                              0x00000040
#define DAIF_PKDT_V4CT_MASK                              0x00000080
#define DAIF_FILTER_MAX6DB_MASK                          0x00040000
// AGC_CFG1
#define DAIF_FILT_GAIN_INI_POS                           8
#define DAIF_INI_WAIT_TIME_POS                           11
#define DAIF_GET_PIF_TIME_POS                            12
#define DAIF_AGC_SETTLE_TIME1_POS                        18
#define DAIF_AGC_SETTLE_TIME2_POS                        20
#define DAIF_WAIT_SYNC_TIME_POS                          22
#define DAIF_LOAD_CYCLE_POS                              25
#define DAIF_GAIN_INI_POS                                28
#define DAIF_FILT_GAIN_INI_MASK                          0x00000700
#define DAIF_INI_WAIT_TIME_MASK                          0x00000800
#define DAIF_GET_PIF_TIME_MASK                           0x00003000
#define DAIF_AGC_SETTLE_TIME1_MASK                       0x000C0000
#define DAIF_AGC_SETTLE_TIME2_MASK                       0x00300000
#define DAIF_WAIT_SYNC_TIME_MASK                         0x01C00000
#define DAIF_LOAD_CYCLE_MASK                             0x0E000000
#define DAIF_GAIN_INI_MASK                               0xF0000000
// MIX_CFG1
#define DAIF_PEAKDET_START_POS                           0U
#define DAIF_PEAKDET_START_MASK                          (1U << 0)
#define DAIF_ICON_VCO_MO_POS                             1U
#define DAIF_ICON_VCO_MO_MASK                            (7U << 1)
#define DAIF_ICON_VCO_ME_POS                             4U
#define DAIF_ICON_VCO_ME_MASK                            (1U << 4)
#define DAIF_PD_PKDET_VCO_MO_POS                         5U
#define DAIF_PD_PKDET_VCO_MO_MASK                        (1U << 5)
#define DAIF_PD_PKDET_VCO_ME_POS                         6U
#define DAIF_PD_PKDET_VCO_ME_MASK                        (1U << 6)
#define DAIF_VREF_PKDET_VCO_POS                          7U
#define DAIF_VREF_PKDET_VCO_MASK                         (7U << 7)
#define DAIF_RX_GAIN_IDX_MO_POS                          15U
#define DAIF_RX_GAIN_IDX_MO_MASK                         (0x0FU << 15)
#define DAIF_RX_GAIN_IDX_ME_POS                          19U
#define DAIF_RX_GAIN_IDX_ME_MASK                         (1U << 19)
#define DAIF_FILTER_GAIN_IDX_MO_POS                      20U
#define DAIF_FILTER_GAIN_IDX_MO_MASK                     (7U << 20)
#define DAIF_FILTER_GAIN_IDX_ME_POS                      23U
#define DAIF_FILTER_GAIN_IDX_ME_MASK                     (1U << 23)
#define DAIF_AFC_BASE_OFFSET_POS                         24U
#define DAIF_AFC_BASE_OFFSET_MASK                        (0x3FU << 24)
// PLL_PHY_CTRL
#define DAIF_PHY_16M_EN_MO_POS                           0U
#define DAIF_PHY_16M_EN_MO_MASK                          (1U << 0)
#define DAIF_PHY_16M_EN_ME_POS                           1U
#define DAIF_PHY_16M_EN_ME_MASK                          (1U << 1)
#define DAIF_PHY_16M_DIVNUM_MO_POS                       2U
#define DAIF_PHY_16M_DIVNUM_MO_MASK                      (0x7FU << 2)
#define DAIF_PHY_16M_DIVNUM_ME_POS                       9U
#define DAIF_PHY_16M_DIVNUM_ME_MASK                      (1U << 9)
// IR_RX_CFG
#define DAIF_WIN_CNT_THRESHOLD_POS                       15
#define DAIF_WIN_CNT_THRESHOLD_MASK                      0xFFFF8000
// TIA_DCOC_CFG
#define DAIF_TIA_ST_POS                                  0
#define DAIF_TIA_EN_DCOC_POS                             2
#define DAIF_TIA_CTUNE_ME_POS                            7
#define DAIF_TIA_GAIN_MO_POS                             18
#define DAIF_TIA_GAIN_ME_POS                             20
#define DAIF_TIA_CTUNE_MO_POS                            21
#define DAIF_TIA_SWAP_DCOC_MO_POS                        25
#define DAIF_TIA_SWAP_DCOC_ME_POS                        26
#define DAIF_TIA_SWAP_POS                                27
#define DAIF_TIA_MODE_SEL_MO_POS                         28
#define DAIF_TIA_MODE_SEL_ME_POS                         29
#define DAIF_TIA_ST_MASK                                 0x00000001
#define DAIF_TIA_EN_DCOC_MASK                            0x00000004
#define DAIF_TIA_CTUNE_ME_MASK                           0x00000080
#define DAIF_TIA_GAIN_MO_MASK                            0x000C0000
#define DAIF_TIA_GAIN_ME_MASK                            0x00100000
#define DAIF_TIA_CTUNE_MO_MASK                           0x01E00000
#define DAIF_TIA_SWAP_DCOC_MO_MASK                       0x02000000
#define DAIF_TIA_SWAP_DCOC_ME_MASK                       0x04000000
#define DAIF_TIA_SWAP_MASK                               0x08000000
#define DAIF_TIA_MODE_SEL_MO_MASK                        0x10000000
#define DAIF_TIA_MODE_SEL_ME_MASK                        0x20000000
// SYSPLL_CNS0
#define DAIF_SYSPLL_PD_VBAT_POS                          0U
#define DAIF_SYSPLL_PD_VBAT_MASK                         (1U << 0)
#define DAIF_SYSPLL_SEL_ICP_POS                          1U
#define DAIF_SYSPLL_SEL_ICP_MASK                         (7U << 1)
#define DAIF_SYSPLL_CTRL_ANALDO_POS                      4U
#define DAIF_SYSPLL_CTRL_ANALDO_MASK                     (3U << 4)
#define DAIF_SYSPLL_EN_TSTCLK_POS                        6U
#define DAIF_SYSPLL_EN_TSTCLK_MASK                       (1U << 6)
#define DAIF_SYSPLL_EN_DLY_POS                           7U
#define DAIF_SYSPLL_EN_DLY_MASK                          (1U << 7)
#define DAIF_SYSPLL_SEL_DEGLITCH_POS                     8U
#define DAIF_SYSPLL_SEL_DEGLITCH_MASK                    (1U << 8)
#define DAIF_SYSPLL_FREF_POS                             9U
#define DAIF_SYSPLL_FREF_MASK                            (1U << 9)
#define DAIF_SYSPLL_RSTN_DIG_POS                         10U
#define DAIF_SYSPLL_RSTN_DIG_MASK                        (1U << 10)
#define DAIF_SYSPLL_EN_AFC_POS                           11U
#define DAIF_SYSPLL_EN_AFC_MASK                          (1U << 11)
#define DAIF_SYSPLL_CTMN_POS                             12U
#define DAIF_SYSPLL_CTMN_MASK                            (1U << 12)
#define DAIF_SYSPLL_CT_POS                               13U
#define DAIF_SYSPLL_CT_MASK                              (0x1FU << 13)
#define DAIF_SYSPLL_SEL_LOCKTIME_POS                     18U
#define DAIF_SYSPLL_SEL_LOCKTIME_MASK                    (7U << 18)
#define DAIF_SYSPLL_SEL_CPUCLK_POS                       21U
#define DAIF_SYSPLL_SEL_CPUCLK_MASK                      (1U << 21)
#define DAIF_SYSPLL_GT_CPUCLK_POS                        23U
#define DAIF_SYSPLL_GT_CPUCLK_MASK                       (1U << 23)
#define DAIF_SYSPLL_SEL_PERICLK_POS                      24U
#define DAIF_SYSPLL_SEL_PERICLK_MASK                     (3U << 24)
#define DAIF_SYSPLL_GT_PERICLK_POS                       26U
#define DAIF_SYSPLL_GT_PERICLK_MASK                      (1U << 26)
#define DAIF_SYSPLL_GT_USBCLK48M_POS                     27U
#define DAIF_SYSPLL_GT_USBCLK48M_MASK                    (1U << 27)
#define DAIF_SYSPLL_SEL_TRNG_POS                         28U
#define DAIF_SYSPLL_SEL_TRNG_MASK                        (3U << 28)
#define DAIF_SYSPLL_GT_TRNG_POS                          30U
#define DAIF_SYSPLL_GT_TRNG_MASK                         (1U << 30)
// SYSPLL_CNS1
#define DAIF_SYSPLL_LOCK_POS                             0U
#define DAIF_SYSPLL_LOCK_MASK                            (1U << 0)
#define DAIF_SYSPLL_CT_AFC_POS                           1U
#define DAIF_SYSPLL_CT_AFC_MASK                          (0x1FU << 1)
// RF_CFG
#define DAIF_RF_PAGAIN_POS                               1U
#define DAIF_RF_PAGAIN_MASK                              (3U << 1)
#define DAIF_RF_TXEN_ME_POS                              3U
#define DAIF_RF_TXEN_ME_MASK                             (1U << 3)
#define DAIF_RF_TXEN_MO_POS                              4U
#define DAIF_RF_TXEN_MO_MASK                             (1U << 4)
#define DAIF_RF_RXBYPASS_POS                             5U
#define DAIF_RF_RXBYPASS_MASK                            (1U << 5)
#define DAIF_RF_RXGAIN_ME_POS                            6U
#define DAIF_RF_RXGAIN_ME_MASK                           (1U << 6)
#define DAIF_RF_RXGAIN_MO_POS                            7U
#define DAIF_RF_RXGAIN_MO_MASK                           (1U << 7)
#define DAIF_RF_RXEN_ME_POS                              8U
#define DAIF_RF_RXEN_ME_MASK                             (1U << 8)
#define DAIF_RF_RXEN_MO_POS                              9U
#define DAIF_RF_RXEN_MO_MASK                             (1U << 9)
#define DAIF_RF_LOWPOWER_POS                             10U
#define DAIF_RF_LOWPOWER_MASK                            (1U << 10)
#define DAIF_RF_GAIN_IDX_SMALL_POS                       11U
#define DAIF_RF_GAIN_IDX_SMALL_MASK                      (0x0FU << 11)
#define DAIF_RF_GAIN_IDX_LARGE_POS                       15U
#define DAIF_RF_GAIN_IDX_LARGE_MASK                      (0x0FU << 15)
#define DAIF_RF_TX_WAIT_TIME_FALL_POS                    19U
#define DAIF_RF_TX_WAIT_TIME_FALL_MASK                   (0x0FU << 19)
#define DAIF_RF_TX_WAIT_TIME_RISE_POS                    23U
#define DAIF_RF_TX_WAIT_TIME_RISE_MASK                   (0x0F << 23)
// RF_TX_DATA
#define DAIF_RF_COMMAND_POS                              0U
#define DAIF_RF_COMMAND_MASK                             (0x7FFFU << 0)
#define DAIF_RF_START_POS                                16U
#define DAIF_RF_START_MASK                               (1U << 16)
#define DAIF_RF_RX_WAIT_TIME_FALL_POS                    17U
#define DAIF_RF_RX_WAIT_TIME_FALL_MASK                   (0x0FU << 17)
#define DAIF_RF_RX_WAIT_TIME_RISE_POS                    21U
#define DAIF_RF_RX_WAIT_TIME_RISE_MASK                   (0x0FU << 21)
// FILT_AGC_LUT_REG0
#define DAIF_FILT_AGC_LUT_0_POS                          0
#define DAIF_FILT_AGC_LUT_1_POS                          9
#define DAIF_FILT_AGC_LUT_2_POS                          18
#define DAIF_FILT_AGC_LUT_0_MASK                         0x000001FF
#define DAIF_FILT_AGC_LUT_1_MASK                         0x0003FE00
#define DAIF_FILT_AGC_LUT_2_MASK                         0x07FC0000
// FILT_AGC_LUT_REG1
#define DAIF_FILT_AGC_LUT_3_POS                          0
#define DAIF_FILT_AGC_LUT_4_POS                          9
#define DAIF_FILT_AGC_LUT_5_POS                          18
#define DAIF_FILT_AGC_LUT_3_MASK                         0x000001FF
#define DAIF_FILT_AGC_LUT_4_MASK                         0x0003FE00
#define DAIF_FILT_AGC_LUT_5_MASK                         0x07FC0000
// FILT_AGC_LUT_REG2
#define DAIF_FILT_AGC_LUT_6_POS                          0
#define DAIF_FILT_AGC_LUT_7_POS                          9
#define DAIF_FILT_AGC_LUT_8_POS                          18
#define DAIF_FILT_AGC_LUT_6_MASK                         0x000001FF
#define DAIF_FILT_AGC_LUT_7_MASK                         0x0003FE00
#define DAIF_FILT_AGC_LUT_8_MASK                         0x07FC0000
// RX_GAINC_LUT_REG0
#define DAIF_RX_GAINC_LUT0_POS                           0
#define DAIF_RX_GAINC_LUT1_POS                           6
#define DAIF_RX_GAINC_LUT2_POS                           12
#define DAIF_RX_GAINC_LUT3_POS                           18
#define DAIF_RX_GAINC_LUT4_POS                           24
#define DAIF_RX_GAINC_LUT0_MASK                          0x0000003F
#define DAIF_RX_GAINC_LUT1_MASK                          0x00000FC0
#define DAIF_RX_GAINC_LUT2_MASK                          0x0003F000
#define DAIF_RX_GAINC_LUT3_MASK                          0x00FC0000
#define DAIF_RX_GAINC_LUT4_MASK                          0x3F000000
// RX_GAINC_LUT_REG1
#define DAIF_RX_GAINC_LUT5_POS                           0
#define DAIF_RX_GAINC_LUT6_POS                           6
#define DAIF_RX_GAINC_LUT7_POS                           12
#define DAIF_RX_GAINC_LUT8_POS                           18
#define DAIF_RX_GAINC_LUT5_MASK                          0x0000003F
#define DAIF_RX_GAINC_LUT6_MASK                          0x00000FC0
#define DAIF_RX_GAINC_LUT7_MASK                          0x0003F000
#define DAIF_RX_GAINC_LUT8_MASK                          0x00FC0000
// PA_CTRL
#define DAIF_PA_DRV0_PCOMP_POS                           0U
#define DAIF_PA_DRV0_PCOMP_MASK                          (1U << 0)
#define DAIF_PA_DRV0_NCOMP_POS                           1U
#define DAIF_PA_DRV0_NCOMP_MASK                          (1U << 1)
#define DAIF_TRS_TXCAP_CT_POS                            3U
#define DAIF_TRS_TXCAP_CT_MASK                           (3U << 3)
#define DAIF_TRS_RXCAP_CT_POS                            5U
#define DAIF_TRS_RXCAP_CT_MASK                           (0x0FU << 5)
#define DAIF_TRS_ANTCAP_CT_POS                           9U
#define DAIF_TRS_ANTCAP_CT_MASK                          (0x0FU << 9)
// TRX_SW_CFG
#define DAIF_TRX_EN_MO_POS                               0
#define DAIF_TRX_EN_ME_POS                               1
#define DAIF_PA_RAMP_TSEL_POS                            4
#define DAIF_PA_IBCT_POS                                 5
#define DAIF_PA_PATTERN_SEL_POS                          7
#define DAIF_TRX_EN_MO_MASK                              0x00000001
#define DAIF_TRX_EN_ME_MASK                              0x00000002
#define DAIF_PA_RAMP_TSEL_MASK                           0x00000010
#define DAIF_PA_IBCT_MASK                                0x00000060
#define DAIF_PA_PATTERN_SEL_MASK                         0x00000080
// CLK_ENS
#define DAIF_GPADC_CLK_EN_POS                            0
#define DAIF_PLL_AFC_CLK_EN_POS                          1
#define DAIF_PLL_AMP_CLK_EN_POS                          2
#define DAIF_PLL_AMP_CLK_EN_MASK                         (1U << 2)
#define DAIF_PLL_VTRACK_CLK_EN_POS                       3
#define DAIF_PLL_LUT_CLK_EN_POS                          4
#define DAIF_PLL_CLK_AFC_EN_POS                          5
#define DAIF_RC_32K_TUNE_CLK_EN_POS                      6
#define DAIF_RC_32M_TUNE_CLK_EN_POS                      7
#define DAIF_MAIN_FSM_CLK_EN_POS                         9
#define DAIF_RX_AGC_CLK_EN_POS                           10
#define DAIF_DCOC_LUT_CLK_EN_POS                         11
#define DAIF_SDM_CLK_EN_POS                              12
#define DAIF_RF_CTRL_CLK_EN_POS                          14
#define DAIF_RF_CTRL_CLK_EN_MASK                         (1U << 14)
#define DAIF_MON_DMA_RF_CLK_EN_POS                       15
#define DAIF_XTAL32M_CTRL_CLK_EN_POS                     16
#define DAIF_PLL_CLK_REF_EN_POS                          17
#define DAIF_CC_CLK_EN_POS                               18
#define DAIF_C32K_CLK_EN_POS                             20
#define DAIF_GPADC_CLK_EN_MASK                           0x00000001
#define DAIF_PLL_AFC_CLK_EN_MASK                         0x00000002
#define DAIF_PLL_VTRACK_CLK_EN_MASK                      0x00000008
#define DAIF_PLL_LUT_CLK_EN_MASK                         0x00000010
#define DAIF_PLL_CLK_AFC_EN_MASK                         0x00000020
#define DAIF_RC_32K_TUNE_CLK_EN_MASK                     0x00000040
#define DAIF_RC_32M_TUNE_CLK_EN_MASK                     0x00000080
#define DAIF_MAIN_FSM_CLK_EN_MASK                        0x00000200
#define DAIF_RX_AGC_CLK_EN_MASK                          0x00000400
#define DAIF_DCOC_LUT_CLK_EN_MASK                        0x00000800
#define DAIF_SDM_CLK_EN_MASK                             0x00001000
#define DAIF_MON_DMA_RF_CLK_EN_MASK                      0x00008000
#define DAIF_XTAL32M_CTRL_CLK_EN_MASK                    0x00010000
#define DAIF_PLL_CLK_REF_EN_MASK                         0x00020000
#define DAIF_CC_CLK_EN_MASK                              0x00040000
#define DAIF_C32K_CLK_EN_MASK                            0x00100000
// PLL_LUT_DBG
#define DAIF_PLL_LUT_DATA_POS                            0
#define DAIF_PLL_LUT_IDX_POS                             16
#define DAIF_PLL_LUT_WR_POS                              24
#define DAIF_PLL_LUT_LU_POS                              28
#define DAIF_PLL_LUT_DATA_MASK                           0x00003FFF
#define DAIF_PLL_LUT_IDX_MASK                            0x007F0000
#define DAIF_PLL_LUT_WR_MASK                             0x01000000
#define DAIF_PLL_LUT_LU_MASK                             0x10000000
// MON_DMA_CFG0
#define DAIF_RF_MON_ENABLE_POS                           0
#define DAIF_RF_MON_DONE_INTR_POS                        1
#define DAIF_RF_MON_CFG_POS                              4
#define DAIF_RF_MON_THRSHLD_POS                          8
#define DAIF_PC_MON_ENABLE_POS                           12
#define DAIF_PC_MON_DONE_INTR_POS                        13
#define DAIF_PC_MON_THRSHLD_POS                          16
#define DAIF_RF_MON_ENABLE_MASK                          0x00000001
#define DAIF_RF_MON_DONE_INTR_MASK                       0x00000002
#define DAIF_RF_MON_CFG_MASK                             0x00000030
#define DAIF_RF_MON_THRSHLD_MASK                         0x00000F00
#define DAIF_PC_MON_ENABLE_MASK                          0x00001000
#define DAIF_PC_MON_DONE_INTR_MASK                       0x00002000
#define DAIF_PC_MON_THRSHLD_MASK                         0x000F0000
// MON_DMA_CFG1
#define DAIF_RF_MON_MAX_POS                              0
#define DAIF_PC_MON_MAX_POS                              16
#define DAIF_RF_MON_MAX_MASK                             0x0000FFFF
#define DAIF_PC_MON_MAX_MASK                             0xFFFF0000
// MON_DMA_CFG2
#define DAIF_MON_DMA_BASE_ADDR_POS                       2
#define DAIF_MON_DMA_BASE_ADDR_MASK                      0xFFFFFFFC
// DBG_REG
#define DAIF_DBG_IDX_POS                                 0
#define DAIF_DBG_EN_POS                                  4
#define DAIF_DBG_DATA_POS                                16
#define DAIF_DBG_IDX_MASK                                0x0000000F
#define DAIF_DBG_EN_MASK                                 0x00000010
#define DAIF_DBG_DATA_MASK                               0xFFFF0000
// HS6621CB_CFG
#define DAIF_FSM_6621CB_FIX_EN_POS                       0
#define DAIF_FSM_6621CB_FIX_EN_MASK                      0x00000001
// KDCO_LUT_1M_REG0
#define DAIF_KDCO_LUT_1M_0_POS                           0
#define DAIF_KDCO_LUT_1M_1_POS                           6
#define DAIF_KDCO_LUT_1M_2_POS                           12
#define DAIF_KDCO_LUT_1M_3_POS                           18
#define DAIF_KDCO_LUT_1M_4_POS                           24
#define DAIF_KDCO_LUT_1M_0_MASK                          0x0000003F
#define DAIF_KDCO_LUT_1M_1_MASK                          0x00000FC0
#define DAIF_KDCO_LUT_1M_2_MASK                          0x0003F000
#define DAIF_KDCO_LUT_1M_3_MASK                          0x00FC0000
#define DAIF_KDCO_LUT_1M_4_MASK                          0x3F000000
// KDCO_LUT_1M_REG1
#define DAIF_KDCO_LUT_1M_5_POS                           0
#define DAIF_KDCO_LUT_1M_6_POS                           6
#define DAIF_KDCO_LUT_1M_7_POS                           12
#define DAIF_KDCO_LUT_1M_8_POS                           18
#define DAIF_KDCO_LUT_1M_9_POS                           24
#define DAIF_KDCO_LUT_1M_5_MASK                          0x0000003F
#define DAIF_KDCO_LUT_1M_6_MASK                          0x00000FC0
#define DAIF_KDCO_LUT_1M_7_MASK                          0x0003F000
#define DAIF_KDCO_LUT_1M_8_MASK                          0x00FC0000
#define DAIF_KDCO_LUT_1M_9_MASK                          0x3F000000
// KDCO_LUT_2M_REG0
#define DAIF_KDCO_LUT_2M_0_POS                           0
#define DAIF_KDCO_LUT_2M_1_POS                           8
#define DAIF_KDCO_LUT_2M_2_POS                           16
#define DAIF_KDCO_LUT_2M_3_POS                           24
#define DAIF_KDCO_LUT_2M_0_MASK                          0x0000003F
#define DAIF_KDCO_LUT_2M_1_MASK                          0x00003F00
#define DAIF_KDCO_LUT_2M_2_MASK                          0x003F0000
#define DAIF_KDCO_LUT_2M_3_MASK                          0x3F000000
// KDCO_LUT_2M_REG1
#define DAIF_KDCO_LUT_2M_4_POS                           0
#define DAIF_KDCO_LUT_2M_5_POS                           8
#define DAIF_KDCO_LUT_1M_10_POS                          16
#define DAIF_KDCO_LUT_2M_4_MASK                          0x0000003F
#define DAIF_KDCO_LUT_2M_5_MASK                          0x00003F00
#define DAIF_KDCO_LUT_1M_10_MASK                         0x003F0000
// KDCO_LUT_1M_REG2
#define DAIF_KDCO_LUT_1M_11_POS                          0U
#define DAIF_KDCO_LUT_1M_11_MASK                         (0x3FU << 0)
#define DAIF_KDCO_LUT_1M_12_POS                          6U
#define DAIF_KDCO_LUT_1M_12_MASK                         (0x3FU << 6)
#define DAIF_KDCO_LUT_1M_13_POS                          12U
#define DAIF_KDCO_LUT_1M_13_MASK                         (0x3FU << 12)
#define DAIF_KDCO_LUT_1M_14_POS                          18U
#define DAIF_KDCO_LUT_1M_14_MASK                         (0x3FU << 18)
// PLL_AFC_CTRL
#define DAIF_PLL_AFC_START_FREQ_POS                      0U
#define DAIF_PLL_AFC_START_FREQ_MASK                     (0xFFFU << 0)
#define DAIF_PLL_AFC_STOP_FREQ_POS                       16U
#define DAIF_PLL_AFC_STOP_FREQ_MASK                      (0xFFFU << 16)
// RC32K_TUN_OUT
#define DAIF_RC32K_RTUNE_POS                             0
#define DAIF_RC32K_CTUNE_POS                             8
#define DAIF_RC32K_RTUNE_MASK                            0x000000FF
#define DAIF_RC32K_CTUNE_MASK                            0x00007F00
// TRX_EXT_PD_CFG
#define DAIF_TX_EXT_PD_EN_POS                            0
#define DAIF_TX_EXT_PD_POL_POS                           1
#define DAIF_TX_EXT_PD_TCFG_POS                          2
#define DAIF_RX_EXT_PD_EN_POS                            16
#define DAIF_RX_EXT_PD_POL_POS                           17
#define DAIF_RX_EXT_PD_TCFG_POS                          18
#define DAIF_TX_EXT_PD_EN_MASK                           0x00000001
#define DAIF_TX_EXT_PD_POL_MASK                          0x00000002
#define DAIF_TX_EXT_PD_TCFG_MASK                         0x0000FFFC
#define DAIF_RX_EXT_PD_EN_MASK                           0x00010000
#define DAIF_RX_EXT_PD_POL_MASK                          0x00020000
#define DAIF_RX_EXT_PD_TCFG_MASK                         0xFFFC0000
// CLK_CHK_CNS
#define DAIF_CLK_CHK_START_POS                           0
#define DAIF_CLK_CHK_INTR_EN_POS                         1
#define DAIF_CLK_CHK_INTR_ST_POS                         2
#define DAIF_CLK_CHK_REF_SEL_POS                         4
#define DAIF_CLK_CHK_WIN_CNT_THRSH_POS                   8
#define DAIF_CLK_CHK_START_MASK                          0x00000001
#define DAIF_CLK_CHK_INTR_EN_MASK                        0x00000002
#define DAIF_CLK_CHK_INTR_ST_MASK                        0x00000004
#define DAIF_CLK_CHK_REF_SEL_MASK                        0x00000010
#define DAIF_CLK_CHK_WIN_CNT_THRSH_MASK                  0xFFFFFF00
// CLK_CJK_STATUS
#define DAIF_CLK_CNT_POS                                 0
#define DAIF_CLK_CNT_MASK                                0xFFFFFFFF
// TIA_CR1
#define DAIF_TIA_START_POS                      0
#define DAIF_RXADC_RST_MO_POS                   1
#define DAIF_TIA_DIN_DCOC_I_MO_POS              2
#define DAIF_TIA_DIN_DCOC_Q_MO_POS              7
#define DAIF_TIA_SEL_DCOC_FS_MO_POS             12
#define DAIF_RXADC_RST_ME_POS                   20
#define DAIF_TIA_DIN_DCOC_I_ME_POS              21
#define DAIF_TIA_DIN_DCOC_Q_ME_POS              22
#define DAIF_TIA_SEL_DCOC_FS_ME_POS             23
#define DAIF_TIA_START_MASK                     0x00000001
#define DAIF_RXADC_RST_MO_MASK                  0x00000002
#define DAIF_TIA_DIN_DCOC_I_MO_MASK             0x0000007C
#define DAIF_TIA_DIN_DCOC_Q_MO_MASK             0x00000F80
#define DAIF_TIA_SEL_DCOC_FS_MO_MASK            0x00003000
#define DAIF_RXADC_RST_ME_MASK                  0x00100000
#define DAIF_TIA_DIN_DCOC_I_ME_MASK             0x00200000
#define DAIF_TIA_DIN_DCOC_Q_ME_MASK             0x00400000
#define DAIF_TIA_SEL_DCOC_FS_ME_MASK            0x00800000
// TIA_CR2
#define DAIF_PLL_STABLE_DLY_POS                 0
#define DAIF_DC_STABLE_DLY_POS                  4
#define DAIF_TIA_MO_DLY_POS                     8
#define DAIF_AVG_NUM_POS                        12
#define DAIF_TIA_LP_CP_SEL_MO_POS               15
#define DAIF_TIA_LP_CP_SEL_ME_POS               16
#define DAIF_DCOC_GIDX_POS                      17
#define DAIF_FILTER_LP_CP_SEL_MO_POS            24
#define DAIF_FILTER_LP_CP_SEL_ME_POS            25
#define DAIF_PLL_STABLE_DLY_MASK                0x0000000F
#define DAIF_DC_STABLE_DLY_MASK                 0x000000F0
#define DAIF_TIA_MO_DLY_MASK                    0x00000F00
#define DAIF_AVG_NUM_MASK                       0x00007000
#define DAIF_TIA_LP_CP_SEL_MO_MASK              0x00008000
#define DAIF_TIA_LP_CP_SEL_ME_MASK              0x00010000
#define DAIF_DCOC_GIDX_MASK                     0x00FE0000
#define DAIF_FILTER_LP_CP_SEL_MO_MASK           0x01000000
#define DAIF_FILTER_LP_CP_SEL_ME_MASK           0x02000000
// RXADC_STATUS
#define DAIF_RXADC_IOUT_POS                     0
#define DAIF_RXADC_QOUT_POS                     16
#define DAIF_RXADC_DELAY_POS                    29
#define DAIF_RXADC_IOUT_MASK                    0x000001FF
#define DAIF_RXADC_QOUT_MASK                    0x01FF0000
#define DAIF_RXADC_DELAY_MASK                   0xE0000000
// TIA_DCOC_R1
#define DAIF_TIA_DCOC_DIN_0_POS                 0
#define DAIF_TIA_DCOC_DIN_1_POS                 10
#define DAIF_TIA_DCOC_DIN_2_POS                 20
#define DAIF_TIA_DCOC_DIN_0_MASK                0x000003FF
#define DAIF_TIA_DCOC_DIN_1_MASK                0x000FFC00
#define DAIF_TIA_DCOC_DIN_2_MASK                0x3FF00000
// TIA_DCOC_R2
#define DAIF_TIA_DCOC_DIN_3_POS                 0
#define DAIF_TIA_DCOC_DIN_4_POS                 10
#define DAIF_TIA_DCOC_DIN_5_POS                 20
#define DAIF_TIA_DCOC_DIN_3_MASK                0x000003FF
#define DAIF_TIA_DCOC_DIN_4_MASK                0x000FFC00
#define DAIF_TIA_DCOC_DIN_5_MASK                0x3FF00000
// TIA_DCOC_R3
#define DAIF_TIA_DCOC_DIN_6_POS                 0
#define DAIF_TIA_CTUNE_FSM_POS                  10
#define DAIF_TIA_DCOC_DIN_6_MASK                0x000003FF
#define DAIF_TIA_CTUNE_FSM_MASK                 0x00003C00
// PA_GAIN_CFG
#define DAIF_PA_GAIN_START_POS                  0
#define DAIF_PA_GAIN_TD_DLY_POS                 1
#define DAIF_PA_GAIN_CALI_DONE_POS              5
#define DAIF_PA_GAIN_START_MASK                 0x00000001
#define DAIF_PA_GAIN_TD_DLY_MASK                0x0000001E
#define DAIF_PA_GAIN_CALI_DONE_MASK             0x00000020
//PA_GAIN_REG_9
#define DAIF_PA_GAIN_17_DRV0_NCT_POS              0
#define DAIF_PA_GAIN_17_DRV0_PCT_POS              5
#define DAIF_PA_GAIN_17_DRIV_NUM_POS              10
#define DAIF_PA_GAIN_18_DRV0_NCT_POS              16
#define DAIF_PA_GAIN_18_DRV0_PCT_POS              21
#define DAIF_PA_GAIN_18_DRIV_NUM_POS              26
#define DAIF_PA_GAIN_17_DRV0_NCT_MASK             0x0000001F
#define DAIF_PA_GAIN_17_DRV0_PCT_MASK             0x000003E0
#define DAIF_PA_GAIN_17_DRIV_NUM_MASK             0x0000FC00
#define DAIF_PA_GAIN_18_DRV0_NCT_MASK             0x001F0000
#define DAIF_PA_GAIN_18_DRV0_PCT_MASK             0x03E00000
#define DAIF_PA_GAIN_18_DRIV_NUM_MASK             0xFC000000
//PA_GAIN_REG_8
#define DAIF_PA_GAIN_15_DRV0_NCT_POS              0
#define DAIF_PA_GAIN_15_DRV0_PCT_POS              5
#define DAIF_PA_GAIN_15_DRIV_NUM_POS              10
#define DAIF_PA_GAIN_16_DRV0_NCT_POS              16
#define DAIF_PA_GAIN_16_DRV0_PCT_POS              21
#define DAIF_PA_GAIN_16_DRIV_NUM_POS              26
#define DAIF_PA_GAIN_15_DRV0_NCT_MASK             0x0000001F
#define DAIF_PA_GAIN_15_DRV0_PCT_MASK             0x000003E0
#define DAIF_PA_GAIN_15_DRIV_NUM_MASK             0x0000FC00
#define DAIF_PA_GAIN_16_DRV0_NCT_MASK             0x001F0000
#define DAIF_PA_GAIN_16_DRV0_PCT_MASK             0x03E00000
#define DAIF_PA_GAIN_16_DRIV_NUM_MASK             0xFC000000
//PA_GAIN_REG_7
#define DAIF_PA_GAIN_13_DRV0_NCT_POS              0
#define DAIF_PA_GAIN_13_DRV0_PCT_POS              5
#define DAIF_PA_GAIN_13_DRIV_NUM_POS              10
#define DAIF_PA_GAIN_14_DRV0_NCT_POS              16
#define DAIF_PA_GAIN_14_DRV0_PCT_POS              21
#define DAIF_PA_GAIN_14_DRIV_NUM_POS              26
#define DAIF_PA_GAIN_13_DRV0_NCT_MASK             0x0000001F
#define DAIF_PA_GAIN_13_DRV0_PCT_MASK             0x000003E0
#define DAIF_PA_GAIN_13_DRIV_NUM_MASK             0x0000FC00
#define DAIF_PA_GAIN_14_DRV0_NCT_MASK             0x001F0000
#define DAIF_PA_GAIN_14_DRV0_PCT_MASK             0x03E00000
#define DAIF_PA_GAIN_14_DRIV_NUM_MASK             0xFC000000
//PA_GAIN_REG_6
#define DAIF_PA_GAIN_11_DRV0_NCT_POS              0
#define DAIF_PA_GAIN_11_DRV0_PCT_POS              5
#define DAIF_PA_GAIN_11_DRIV_NUM_POS              10
#define DAIF_PA_GAIN_12_DRV0_NCT_POS              16
#define DAIF_PA_GAIN_12_DRV0_PCT_POS              21
#define DAIF_PA_GAIN_12_DRIV_NUM_POS              26
#define DAIF_PA_GAIN_11_DRV0_NCT_MASK             0x0000001F
#define DAIF_PA_GAIN_11_DRV0_PCT_MASK             0x000003E0
#define DAIF_PA_GAIN_11_DRIV_NUM_MASK             0x0000FC00
#define DAIF_PA_GAIN_12_DRV0_NCT_MASK             0x001F0000
#define DAIF_PA_GAIN_12_DRV0_PCT_MASK             0x03E00000
#define DAIF_PA_GAIN_12_DRIV_NUM_MASK             0xFC000000
//PA_GAIN_REG_5
#define DAIF_PA_GAIN_9_DRV0_NCT_POS               0
#define DAIF_PA_GAIN_9_DRV0_PCT_POS               5
#define DAIF_PA_GAIN_9_DRIV_NUM_POS               10
#define DAIF_PA_GAIN_10_DRV0_NCT_POS              16
#define DAIF_PA_GAIN_10_DRV0_PCT_POS              21
#define DAIF_PA_GAIN_10_DRIV_NUM_POS              26
#define DAIF_PA_GAIN_9_DRV0_NCT_MASK              0x0000001F
#define DAIF_PA_GAIN_9_DRV0_PCT_MASK              0x000003E0
#define DAIF_PA_GAIN_9_DRIV_NUM_MASK              0x0000FC00
#define DAIF_PA_GAIN_10_DRV0_NCT_MASK             0x001F0000
#define DAIF_PA_GAIN_10_DRV0_PCT_MASK             0x03E00000
#define DAIF_PA_GAIN_10_DRIV_NUM_MASK             0xFC000000
//PA_GAIN_REG_4
#define DAIF_PA_GAIN_7_DRV0_NCT_POS               0
#define DAIF_PA_GAIN_7_DRV0_PCT_POS               5
#define DAIF_PA_GAIN_7_DRIV_NUM_POS               10
#define DAIF_PA_GAIN_8_DRV0_NCT_POS               16
#define DAIF_PA_GAIN_8_DRV0_PCT_POS               21
#define DAIF_PA_GAIN_8_DRIV_NUM_POS               26
#define DAIF_PA_GAIN_7_DRV0_NCT_MASK              0x0000001F
#define DAIF_PA_GAIN_7_DRV0_PCT_MASK              0x000003E0
#define DAIF_PA_GAIN_7_DRIV_NUM_MASK              0x0000FC00
#define DAIF_PA_GAIN_8_DRV0_NCT_MASK              0x001F0000
#define DAIF_PA_GAIN_8_DRV0_PCT_MASK              0x03E00000
#define DAIF_PA_GAIN_8_DRIV_NUM_MASK              0xFC000000
//PA_GAIN_REG_3
#define DAIF_PA_GAIN_5_DRV0_NCT_POS               0
#define DAIF_PA_GAIN_5_DRV0_PCT_POS               5
#define DAIF_PA_GAIN_5_DRIV_NUM_POS               10
#define DAIF_PA_GAIN_6_DRV0_NCT_POS               16
#define DAIF_PA_GAIN_6_DRV0_PCT_POS               21
#define DAIF_PA_GAIN_6_DRIV_NUM_POS               26
#define DAIF_PA_GAIN_5_DRV0_NCT_MASK              0x0000001F
#define DAIF_PA_GAIN_5_DRV0_PCT_MASK              0x000003E0
#define DAIF_PA_GAIN_5_DRIV_NUM_MASK              0x0000FC00
#define DAIF_PA_GAIN_6_DRV0_NCT_MASK              0x001F0000
#define DAIF_PA_GAIN_6_DRV0_PCT_MASK              0x03E00000
#define DAIF_PA_GAIN_6_DRIV_NUM_MASK              0xFC000000
//PA_GAIN_REG_2
#define DAIF_PA_GAIN_3_DRV0_NCT_POS               0
#define DAIF_PA_GAIN_3_DRV0_PCT_POS               5
#define DAIF_PA_GAIN_3_DRIV_NUM_POS               10
#define DAIF_PA_GAIN_4_DRV0_NCT_POS               16
#define DAIF_PA_GAIN_4_DRV0_PCT_POS               21
#define DAIF_PA_GAIN_4_DRIV_NUM_POS               26
#define DAIF_PA_GAIN_3_DRV0_NCT_MASK              0x0000001F
#define DAIF_PA_GAIN_3_DRV0_PCT_MASK              0x000003E0
#define DAIF_PA_GAIN_3_DRIV_NUM_MASK              0x0000FC00
#define DAIF_PA_GAIN_4_DRV0_NCT_MASK              0x001F0000
#define DAIF_PA_GAIN_4_DRV0_PCT_MASK              0x03E00000
#define DAIF_PA_GAIN_4_DRIV_NUM_MASK              0xFC000000
//PA_GAIN_REG_1
#define DAIF_PA_GAIN_1_DRV0_NCT_POS               0
#define DAIF_PA_GAIN_1_DRV0_PCT_POS               5
#define DAIF_PA_GAIN_1_DRIV_NUM_POS               10
#define DAIF_PA_GAIN_2_DRV0_NCT_POS               16
#define DAIF_PA_GAIN_2_DRV0_PCT_POS               21
#define DAIF_PA_GAIN_2_DRIV_NUM_POS               26
#define DAIF_PA_GAIN_1_DRV0_NCT_MASK              0x0000001F
#define DAIF_PA_GAIN_1_DRV0_PCT_MASK              0x000003E0
#define DAIF_PA_GAIN_1_DRIV_NUM_MASK              0x0000FC00
#define DAIF_PA_GAIN_2_DRV0_NCT_MASK              0x001F0000
#define DAIF_PA_GAIN_2_DRV0_PCT_MASK              0x03E00000
#define DAIF_PA_GAIN_2_DRIV_NUM_MASK              0xFC000000

// Wait for the digital state machine to finish running before turning off the clock.
#define DRV_DAIF_WAIT_IDLE() \
    do { \
        OM_DAIF->DBG_REG = 0x13; \
        om_error_t ret; /* Max: 25us */ \
        DRV_WAIT_US_UNTIL_TO((REGR(&OM_DAIF->DBG_REG, MASK_POS(DAIF_DBG_DATA)) & 0xF), 30, ret); (void) ret; \
        OM_DAIF->DBG_REG = 0; \
    } while (0)

#endif

/** @} */
