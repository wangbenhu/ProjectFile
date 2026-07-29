/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup AUDIO AUDIO
 * @ingroup  DEVICE
 * @brief    AUDIO register
 * @details  AUDIO register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __AUDIO_REG_H
#define __AUDIO_REG_H


/*******************************************************************************
 * INCLUDES
 */
#include "common_reg.h"


/*******************************************************************************
 * DEFINES
 */
/** \brief AUDIO ADC Contronl Definitions */
#define AU_CTRL_CIC_3ORD_POS                        22U                                                     /*!< ADC_CTRL: 3-order cic filter used position */
#define AU_CTRL_CIC_3ORD_MASK                       (0x01UL << AU_CTRL_CIC_3ORD_POS)                        /*!< ADC_CTRL: 3-order cic filter used mask */

#define AU_CTRL_CIC_2ORD_POS                        21U                                                     /*!< ADC_CTRL: 2-order cic filter used position */
#define AU_CTRL_CIC_2ORD_MASK                       (0x01UL << AU_CTRL_CIC_2ORD_POS)                        /*!< ADC_CTRL: 2-order cic filter used mask */

#define AU_CTRL_CIC_1ORD_POS                        20U                                                     /*!< ADC_CTRL: 1-order cic filter used position */
#define AU_CTRL_CIC_1ORD_MASK                       (0x01UL << AU_CTRL_CIC_1ORD_POS)                        /*!< ADC_CTRL: 1-order cic filter used mask */

#define AU_CTRL_OUT_SEL_POS                         19U                                                     /*!< ADC_CTRL: 4x or 5x sample rate output position */
#define AU_CTRL_OUT_SEL_MASK                        (0x01UL << AU_CTRL_OUT_SEL_POS)                         /*!< ADC_CTRL: 4x or 5x sample rate output mask */

#define AU_CTRL_CIC_4ORD_POS                        18U                                                     /*!< ADC_CTRL: 0:3-order cic,1:4-order cic position */
#define AU_CTRL_CIC_4ORD_MASK                       (0x01UL << AU_CTRL_CIC_4ORD_POS)                        /*!< ADC_CTRL: 0:3-order cic,1:4-order cic mask */

#define AU_CTRL_ANTI_CLIP_POS                       17U                                                     /*!< ADC_CTRL: When output is clipping,gain decreased automatically position */
#define AU_CTRL_ANTI_CLIP_MASK                      (0x01UL << AU_CTRL_ANTI_CLIP_POS)                       /*!< ADC_CTRL: When output is clipping,gain decreased automatically mask */

#define AU_CTRL_INPUT_INV_POS                       16U                                                     /*!< ADC_CTRL: ADC input is inverted position */
#define AU_CTRL_INPUT_INV_MASK                      (0x01UL << AU_CTRL_INPUT_INV_POS)                       /*!< ADC_CTRL: ADC input is inverted mask */

#define AU_CTRL_SR_32K_POS                          14U                                                     /*!< ADC_CTRL: sample rate base = 32k position */
#define AU_CTRL_SR_32K_MASK                         (0x01UL << AU_CTRL_SR_32K_POS)                          /*!< ADC_CTRL: sample rate base = 32k mask */

#define AU_CTRL_SR_44K_POS                          13U                                                     /*!< ADC_CTRL: sample rate base = 44k position */
#define AU_CTRL_SR_44K_MASK                         (0x01UL << AU_CTRL_SR_44K_POS)                          /*!< ADC_CTRL: sample rate base = 44k mask */

#define AU_CTRL_SR_48K_POS                          12U                                                     /*!< ADC_CTRL: sample rate base = 48k position */
#define AU_CTRL_SR_48K_MASK                         (0x01UL << AU_CTRL_SR_48K_POS)                          /*!< ADC_CTRL: sample rate base = 48k mask */

#define AU_CTRL_SR_4X_POS                           10U                                                     /*!< ADC_CTRL: sample rate = sample rate base/4 position */
#define AU_CTRL_SR_4X_MASK                          (0x01UL << AU_CTRL_SR_4X_POS)                           /*!< ADC_CTRL: sample rate = sample rate base/4 mask */

#define AU_CTRL_SR_2X_POS                           9U                                                      /*!< ADC_CTRL: sample rate = sample rate base/2 position */
#define AU_CTRL_SR_2X_MASK                          (0x01UL << AU_CTRL_SR_2X_POS)                           /*!< ADC_CTRL: sample rate = sample rate base/2 mask */

#define AU_CTRL_SR_1X_POS                           8U                                                      /*!< ADC_CTRL: sample rate = sample rate base position */
#define AU_CTRL_SR_1X_MASK                          (0x01UL << AU_CTRL_SR_1X_POS)                           /*!< ADC_CTRL: sample rate = sample rate base mask */

#define AU_CTRL_24B_EN_POS                          7U                                                      /*!< ADC_CTRL: ADC output 24bit position */
#define AU_CTRL_24B_EN_MASK                         (0x01UL << AU_CTRL_24B_EN_POS)                          /*!< ADC_CTRL: ADC output 24bit mask */

#define AU_CTRL_CIC_SCALE_POS                       4U                                                      /*!< ADC_CTRL: Gain before filter position */
#define AU_CTRL_CIC_SCALE_MASK                      (0x07UL << AU_CTRL_CIC_SCALE_POS)                       /*!< ADC_CTRL: Gain before filter mask */

#define AU_CTRL_DC_EN_POS                           3U                                                      /*!< ADC_CTRL: DC offset enable position */
#define AU_CTRL_DC_EN_MASK                          (0x01UL << AU_CTRL_DC_EN_POS)                           /*!< ADC_CTRL: DC offset enable mask */

#define AU_CTRL_DMIC_EN_POS                         2U                                                      /*!< ADC_CTRL: DMIC enable position */
#define AU_CTRL_DMIC_EN_MASK                        (0x01UL << AU_CTRL_DMIC_EN_POS)                         /*!< ADC_CTRL: DMIC enable mask */

#define AU_CTRL_SW_RESET_X_POS                      1U                                                      /*!< ADC_CTRL: ADC soft reset position */
#define AU_CTRL_SW_RESET_X_MASK                     (0x01UL << AU_CTRL_SW_RESET_X_POS)                      /*!< ADC_CTRL: ADC soft reset mask */

#define AU_CTRL_EN_POS                              0U                                                      /*!< ADC_CTRL: ADC enable position */
#define AU_CTRL_EN_MASK                             (0x01UL << AU_CTRL_EN_POS)                              /*!< ADC_CTRL: ADC enable mask */

/** \brief AUDIO ADC Volume Control Definitions */
#define AU_VOL_UNMUTE_RATE_POS                      28U                                                     /*!< ADC_VOL_CTRL: Volume adjust speed in unmute process position */
#define AU_VOL_UNMUTE_RATE_MASK                     (0x0FUL << AU_VOL_UNMUTE_RATE_POS)                      /*!< ADC_VOL_CTRL: Volume adjust speed in unmute process mask */

#define AU_VOL_UPDATE_POS                           24U                                                     /*!< ADC_VOL_CTRL: Volume control is only update while adc_vol_update=1 position */
#define AU_VOL_UPDATE_MASK                          (0x01UL << AU_VOL_UPDATE_POS)                           /*!< ADC_VOL_CTRL: Volume control is only update while adc_vol_update=1 mask */

#define AU_VOL_LADC_POS                             8U                                                      /*!< ADC_VOL_CTRL: Left channel volume control, 0.5dB/step position */
#define AU_VOL_LADC_MASK                            (0xFFUL << AU_VOL_LADC_POS)                             /*!< ADC_VOL_CTRL: Left channel volume control, 0.5dB/step mask */

#define AU_VOL_MUTE_RATE_POS                        4U                                                      /*!< ADC_VOL_CTRL: Volume adjust speed in mute process position */
#define AU_VOL_MUTE_RATE_MASK                       (0x0FUL << AU_VOL_MUTE_RATE_POS)                        /*!< ADC_VOL_CTRL: Volume adjust speed in mute process mask */

#define AU_VOL_MUTE_BYPASS_POS                      2U                                                      /*!< ADC_VOL_CTRL: 1: mute and unmute mechanism is bypass, volume is only controlled by ladcvol/radcvol position */
#define AU_VOL_MUTE_BYPASS_MASK                     (0x01UL << AU_VOL_MUTE_BYPASS_POS)                      /*!< ADC_VOL_CTRL: 1: mute and unmute mechanism is bypass, volume is only controlled by ladcvol/radcvol mask */

#define AU_VOL_ADCUNMU_POS                          1U                                                      /*!< ADC_VOL_CTRL: 1: gain gradually increased from 0 to ladcvol/radcvol position */
#define AU_VOL_ADCUNMU_MASK                         (0x01UL << AU_VOL_ADCUNMU_POS)                          /*!< ADC_VOL_CTRL: 1: gain gradually increased from 0 to ladcvol/radcvol mask */

#define AU_VOL_ADCMU_POS                            0U                                                      /*!< ADC_VOL_CTRL: 1: gain gradually decreased to 0 position */
#define AU_VOL_ADCMU_MASK                           (0x01UL << AU_VOL_ADCMU_POS)                            /*!< ADC_VOL_CTRL: 1: gain gradually decreased to 0 mask */

/** \brief AUDIO ADC ALC Function Control Definitions */
#define AU_ALC_ATTACK_POS                           16U                                                     /*!< ALC_CTRL_1: ALC attack time position */
#define AU_ALC_ATTACK_MASK                          (0x0FUL << AU_ALC_ATTACK_POS)                           /*!< ALC_CTRL_1: ALC attack time mask */

#define AU_ALC_DECAY_POS                            12U                                                     /*!< ALC_CTRL_1: ALC decay time position */
#define AU_ALC_DECAY_MASK                           (0x0FUL << AU_ALC_DECAY_POS)                            /*!< ALC_CTRL_1: ALC decay time mask */

#define AU_ALC_HOLD_POS                             8U                                                      /*!< ALC_CTRL_1: ALC hold time position */
#define AU_ALC_HOLD_MASK                            (0x0FUL << AU_ALC_HOLD_POS)                             /*!< ALC_CTRL_1: ALC hold time mask */

#define AU_ALC_MODEL_SEL_POS                        3U                                                      /*!< ALC_CTRL_1: ALC mode select position */
#define AU_ALC_MODEL_SEL_MASK                       (0x1FUL << AU_ALC_MODEL_SEL_POS)                        /*!< ALC_CTRL_1: ALC mode select mask */

#define AU_ALC_NOISE_TYPE_POS                       2U                                                      /*!< ALC_CTRL_1: ALC noise gate type position */
#define AU_ALC_NOISE_TYPE_MASK                      (0x01UL << AU_ALC_NOISE_TYPE_POS)                       /*!< ALC_CTRL_1: ALC noise gate type mask */

#define AU_ALC_NOISE_EN_POS                         1U                                                      /*!< ALC_CTRL_1: ALC noise gate function enable position */
#define AU_ALC_NOISE_EN_MASK                        (0x01UL << AU_ALC_NOISE_EN_POS)                         /*!< ALC_CTRL_1: ALC noise gate function enable mask */

#define AU_ALC_DIG_EN_POS                           0U                                                      /*!< ALC_CTRL_1: ALC digital gain control enable position */
#define AU_ALC_DIG_EN_MASK                          (0x01UL << AU_ALC_DIG_EN_POS)                           /*!< ALC_CTRL_1: ALC digital gain control enable mask */

/** \brief AUDIO ADC ALC Function Control Definitions */
#define AU_ALC_NOISE_JITTER_POS                     24U                                                     /*!< ALC_CTRL_2: ALC noise threshold margin position */
#define AU_ALC_NOISE_JITTER_MASK                    (0xFFUL << AU_ALC_NOISE_JITTER_POS)                     /*!< ALC_CTRL_2: ALC noise threshold margin mask */

#define AU_ALC_NGTH_POS                             16U                                                     /*!< ALC_CTRL_2: ALC noise gate threshold position */
#define AU_ALC_NGTH_MASK                            (0xFFUL << AU_ALC_NGTH_POS)                             /*!< ALC_CTRL_2: ALC noise gate threshold mask */

#define AU_ALC_TARGET_JITTER_POS                    8U                                                      /*!< ALC_CTRL_2: ALC target margin position */
#define AU_ALC_TARGET_JITTER_MASK                   (0xFFUL << AU_ALC_TARGET_JITTER_POS)                    /*!< ALC_CTRL_2: ALC target margin mask */

#define AU_ALC_TARGET_POS                           0U                                                      /*!< ALC_CTRL_2: ALC target position */
#define AU_ALC_TARGET_MASK                          (0xFFUL << AU_ALC_TARGET_POS)                           /*!< ALC_CTRL_2: ALC target mask */

/** \brief AUDIO ADC Internal Gain Read Definitions */
#define AU_GAIN_READ_POS                            0U                                                      /*!< CODEC_GAIN_READ: ADC gain control read out position(read only) */
#define AU_GAIN_READ_MASK                           (0xFFUL << AU_GAIN_READ_POS)                            /*!< CODEC_GAIN_READ: ADC gain control read out mask(read only) */

/** \brief AUDIO ADC Clock Control Register 1 Definitions */
#define AU_CLK_CTRL1_DMIC_CTRL_POS                  6U                                                      /*!< CODEC_CLK_CTRL_1: DMIC clock ouput control position */
#define AU_CLK_CTRL1_DIMC_CTRL_MASK                 (0x01UL << AU_CLK_CTRL1_DMIC_CTRL_POS)                  /*!< CODEC_CLK_CTRL_1: DMIC clock ouput control mask */

#define AU_CLK_CTRL1_DMIC_SEL_POS                   5U                                                      /*!< CODEC_CLK_CTRL_1: DMIC clock select position */
#define AU_CLK_CTRL1_DMIC_SEL_MASK                  (0x01UL << AU_CLK_CTRL1_DMIC_SEL_POS)                   /*!< CODEC_CLK_CTRL_1: DMIC clock select mask */

#define AU_CLK_CTRL1_32K_EN_POS                     4U                                                      /*!< CODEC_CLK_CTRL_1: 32K enable position */
#define AU_CLK_CTRL1_32K_EN_MASK                    (0x01UL << AU_CLK_CTRL1_32K_EN_POS)                     /*!< CODEC_CLK_CTRL_1: 32K enable mask */

#define AU_CLK_I2S_CONN_CTRL_POS                    2U                                                      /*!< CODEC_CLK_CTRL_1: I2S SDI SDO connect select position */
#define AU_CLK_I2S_CONN_CTRL_MASK                   (0x03UL << AU_CLK_I2S_CONN_CTRL_POS)                    /*!< CODEC_CLK_CTRL_1: I2S SDI SDO connect select mask */

#define AU_CLK_CTRL1_CLK_EN_POS                     1U                                                      /*!< CODEC_CLK_CTRL_1: ADC clock enable, high means active position */
#define AU_CLK_CTRL1_CLK_EN_MASK                    (0x01UL << AU_CLK_CTRL1_CLK_EN_POS)                     /*!< CODEC_CLK_CTRL_1: ADC clock enable, high means active mask */

#define AU_CLK_CTRL1_RSTN_POS                       0U                                                      /*!< CODEC_CLK_CTRL_1: ADC reset, low means active position */
#define AU_CLK_CTRL1_RSTN_MASK                      (0x01UL << AU_CLK_CTRL1_RSTN_POS)                       /*!< CODEC_CLK_CTRL_1: ADC reset, low means active mask */

/** \brief AUDIO ADC Interrupt Status Definitions */
#define AU_INT_STATUS_OFF_POS                       6U                                                      /*!< CODEC_INT_STATUS: Audio power off complete, write 1 to clear interrupt position */
#define AU_INT_STATUS_OFF_MASK                      (0x1UL << AU_INT_STATUS_OFF_POS)                        /*!< CODEC_INT_STATUS: Audio power off complete, write 1 to clear interrupt mask */

#define AU_INT_STATUS_ON_POS                        5U                                                      /*!< CODEC_INT_STATUS: Audio power on complete, write 1 to clear interrupt position */
#define AU_INT_STATUS_ON_MASK                       (0x1UL << AU_INT_STATUS_ON_POS)                         /*!< CODEC_INT_STATUS: Audio power on complete, write 1 to clear interrupt mask */

#define AU_INT_STATUS_INTT_POS                      4U                                                      /*!< CODEC_INT_STATUS: All interrupts are OR position */
#define AU_INT_STATUS_INTT_MASK                     (0x1UL << AU_INT_STATUS_INTT_POS)                       /*!< CODEC_INT_STATUS: All interrupts are OR mask */

#define AU_INT_STATUS_UNMUTE_POS                    3U                                                      /*!< CODEC_INT_STATUS: Unmute interrupt position */
#define AU_INT_STATUS_UNMUTE_MASK                   (0x1UL << AU_INT_STATUS_UNMUTE_POS)                     /*!< CODEC_INT_STATUS: Unmute interrupt mask */

#define AU_INT_STATUS_MUTE_POS                      2U                                                      /*!< CODEC_INT_STATUS: Mute interrupt position */
#define AU_INT_STATUS_MUTE_MASK                     (0x1UL << AU_INT_STATUS_MUTE_POS)                       /*!< CODEC_INT_STATUS: Mute interrupt mask */

#define AU_INT_STATUS_CLIP_POS                      1U                                                      /*!< CODEC_INT_STATUS: ADC output cliping interrupt position */
#define AU_INT_STATUS_CLIP_MASK                     (0x1UL << AU_INT_STATUS_CLIP_POS)                       /*!< CODEC_INT_STATUS: ADC output cliping interrupt mask */

/** \brief AUDIO ADC Interrupt Contronl Definitions */
#define AU_INT_CTRL_OFF_POS                         6U                                                      /*!< CODEC_INT_CTRL: Write 1 mask audio power off interrupt position */
#define AU_INT_CTRL_OFF_MASK                        (0x1UL << AU_INT_CTRL_OFF_POS)                          /*!< CODEC_INT_CTRL: Write 1 mask audio power off interrupt mask */

#define AU_INT_CTRL_ON_POS                          5U                                                      /*!< CODEC_INT_CTRL: Write 1 mask audio power on interrupt position */
#define AU_INT_CTRL_ON_MASK                         (0x1UL << AU_INT_CTRL_ON_POS)                           /*!< CODEC_INT_CTRL: Write 1 mask audio power on interrupt mask */

#define AU_INT_CTRL_UNMUTE_POS                      3U                                                      /*!< CODEC_INT_CTRL: Write 1 mask audio unmute interrupt position */
#define AU_INT_CTRL_UNMUTE_MASK                     (0x1UL << AU_INT_CTRL_UNMUTE_POS)                       /*!< CODEC_INT_CTRL: Write 1 mask audio unmute interrupt mask */

#define AU_INT_CTRL_MUTE_POS                        2U                                                      /*!< CODEC_INT_CTRL: Write 1 mask audio mute interrupt position */
#define AU_INT_CTRL_MUTE_MASK                       (0x1UL << AU_INT_CTRL_MUTE_POS)                         /*!< CODEC_INT_CTRL: Write 1 mask audio mute interrupt mask */

#define AU_INT_CTRL_CLIP_POS                        1U                                                      /*!< CODEC_INT_CTRL: Write 1 mask audio clip interrupt and clear position */
#define AU_INT_CTRL_CLIP_MASK                       (0x1UL << AU_INT_CTRL_CLIP_POS)                         /*!< CODEC_INT_CTRL: Write 1 mask audio clip interrupt and clear mask */

/** \brief AUDIO ADC Threshold Control Definitions */
#define AU_CLIP_THD_CTRL_POS                        0U                                                      /*!< ADC_THD_CTRL: Clip threshold control position */
#define AU_CLIP_THD_CTRL_MASK                       (0x7UL << AU_CLIP_THD_CTRL_POS)                         /*!< ADC_THD_CTRL: Clip threshold control mask */

/** \brief AUDIO ADC Analog Control Signals Definitions */
#define AU_ANA_PD_IREF_POS                          31U                                                     /*!< CODEC_ANA_CTRL_1: Current reference, 1: power down */
#define AU_ANA_PD_IREF_MASK                         (0x1UL << AU_ANA_PD_IREF_POS)

#define AU_ANA_SEL_INNER_IREF_POS                   30U                                                     /*!< CODEC_ANA_CTRL_1: Select Current reference, 1: inner */
#define AU_ANA_SEL_INNER_IREF_MASK                  (0x1UL << AU_ANA_SEL_INNER_IREF_POS)

#define AU_ANA_PD_BIAS_GEN_POS                      29U                                                     /*!< CODEC_ANA_CTRL_1: Power down audio bias gen, 1: power down */
#define AU_ANA_PD_BIAS_GEN_MASK                     (0x1UL << AU_ANA_PD_BIAS_GEN_POS)

#define AU_ANA_EN_REF_BYP_POS                       28U                                                     /*!< CODEC_ANA_CTRL_1: Bypass reference big-R for fast setting */
#define AU_ANA_EN_REF_BYP_MASK                      (0x1UL << AU_ANA_EN_REF_BYP_POS)

#define AU_ANA_PD_AUREF_POS                         27U                                                     /*!< CODEC_ANA_CTRL_1: Power down audio reference gen, 1=power down */
#define AU_ANA_PD_AUREF_MASK                        (0x1UL << AU_ANA_PD_AUREF_POS)

#define AU_ANA_SEL_ADC_INPUT_POS                    26U                                                     /*!< CODEC_ANA_CTRL_1: Select ADC input 0=normal input, 1=test input */
#define AU_ANA_SEL_ADC_INPUT_MASK                   (0x1UL << AU_ANA_SEL_ADC_INPUT_POS)

#define AU_ANA_EN_TST_AUADC_BIAS_POS                25U                                                     /*!< CODEC_ANA_CTRL_1: Enable ADC bias current test, 0=no test, 1=enable */
#define AU_ANA_EN_TST_AUADC_BIAS_MASK               (0x1UL << AU_ANA_EN_TST_AUADC_BIAS_POS)

#define AU_ANA_SHORT_ADC_POS                        24U                                                     /*!< CODEC_ANA_CTRL_1: Short ADC input for testing */
#define AU_ANA_SHORT_ADC_MASK                       (0x1UL << AU_ANA_SHORT_ADC_POS)

#define AU_ANA_RST_ADC_POS                          23U                                                     /*!< CODEC_ANA_CTRL_1: Reset audio ADC 1=reset */
#define AU_ANA_RST_ADC_MASK                         (0x1UL << AU_ANA_RST_ADC_POS)

#define AU_ANA_ADC_TSTSEL_POS                       20U                                                     /*!< CODEC_ANA_CTRL_1: ADC test selection,000=no test */
#define AU_ANA_ADC_TSTSEL_MASK                      (0x7UL << AU_ANA_ADC_TSTSEL_POS)

#define AU_ANA_RCTUNE_POS                           16U                                                     /*!< CODEC_ANA_CTRL_1: Tuning RC constant, default is 8 */
#define AU_ANA_RCTUNE_MASK                          (0xFUL << AU_ANA_RCTUNE_POS)

#define AU_ANA_GAIN_POS                             14U                                                    /*!< CODEC_ANA_CTRL_1: ADC gain,look table */
#define AU_ANA_GAIN_MASK                            (0x3UL << AU_ANA_GAIN_POS)

#define AU_ANA_EN_ADC_DEM_POS                       13U                                                     /*!< CODEC_ANA_CTRL_1: Enable ADC DEM function, 1: enable */
#define AU_ANA_EN_ADC_DEM_MASK                      (0x1UL << AU_ANA_EN_ADC_DEM_POS)

#define AU_ANA_PD_CORE_POS                          12U                                                     /*!< CODEC_ANA_CTRL_1: Power down ADC core, 1: power down */
#define AU_ANA_PD_CORE_MASK                         (0x1UL << AU_ANA_PD_CORE_POS)

#define AU_ANA_PD_BIAS_POS                          11U                                                     /*!< CODEC_ANA_CTRL_1: ADC bias, 1: power down */
#define AU_ANA_PD_BIAS_MASK                         (0x1UL << AU_ANA_PD_BIAS_POS)

#define AU_ANA_PGA_GAIN_POS                         4U                                                      /*!< CODEC_ANA_CTRL_1: PGA gain control,look table */
#define AU_ANA_PGA_GAIN_MASK                        (0xFUL << AU_ANA_PGA_GAIN_POS)

#define AU_ANA_PGA_EN_POS                           3U                                                      /*!< CODEC_ANA_CTRL_1: Enable PGA gain */
#define AU_ANA_PGA_EN_MASK                          (0x1UL << AU_ANA_PGA_EN_POS)

#define AU_ANA_PD_PGA_POS                           1U                                                      /*!< CODEC_ANA_CTRL_1: Power down PGA,1: power down */
#define AU_ANA_PD_PGA_MASK                          (0x1UL << AU_ANA_PD_PGA_POS)

#define AU_ANA_PD_CLK_POS                           0U                                                      /*!< CODEC_ANA_CTRL_1: Power down audio clock,1: power down */
#define AU_ANA_PD_CLK_MASK                          (0x1UL << AU_ANA_PD_CLK_POS)

/** \brief AUDIO ADC Analog Control Signals Definitions */
#define AU_ANA_EN_REF_CAPBYP_POS                    23U                                                     /*!< CODEC_ANA_CTRL_2: Bypass reference capacitor,1: bypass */
#define AU_ANA_EN_REF_CAPBYS_MASK                   (0x1UL << AU_ANA_EN_REF_CAPBYP_POS)

#define AU_ANA_VDRV_EN_POS                          22U                                                     /*!< CODEC_ANA_CTRL_2: Voltage driver enable 1: enable */
#define AU_ANA_VDRV_EN_MASK                         (0x1UL << AU_ANA_VDRV_EN_POS)

#define AU_ANA_RTH_EN_POS                           21U                                                     /*!< CODEC_ANA_CTRL_2: RTH enable 1: enable */
#define AU_ANA_RTH_EN_MASK                          (0x1UL << AU_ANA_RTH_EN_POS)

#define AU_ANA_SEL_BIAS_POS                         16U                                                     /*!< CODEC_ANA_CTRL_2: Select bias value,default:0x12 */
#define AU_ANA_SEL_BIAS_MASK                        (0x1FUL << AU_ANA_SEL_BIAS_POS)

#define AU_ANA_EN_LDO28_OFFCHIPCAP_POS              15U                                                     /*!< CODEC_ANA_CTRL_2: Enable ldo28 offchip capacitor */
#define AU_ANA_EN_LDO28_OFFCHIPCAP_MASK             (0x1UL << AU_ANA_EN_LDO28_OFFCHIPCAP_POS)

#define AU_ANA_LDO28_MODE_POS                       14U                                                     /*!< CODEC_ANA_CTRL_2: Select ldo28 mode,0:1.6v,1:2.8v */
#define AU_ANA_LDO28_MODE_MASK                      (0x1UL << AU_ANA_LDO28_MODE_POS)

#define AU_ANA_CTRL_AU_LDO28_POS                    12U                                                     /*!< CODEC_ANA_CTRL_2: 2.8V driver output adjust, default is 1 */
#define AU_ANA_CTRL_AU_LDO28_MASK                   (0x3UL << AU_ANA_CTRL_AU_LDO28_POS)

#define AU_ANA_EN_AUREF_SRC_DEGEN_POS               10U
#define AU_ANA_EN_AUREF_SRC_DEGEN_MASK              (0x3UL << AU_ANA_EN_AUREF_SRC_DEGEN_POS)

#define AU_ANA_CTRL_AULDO12_POS                     8U                                                      /*!< CODEC_ANA_CTRL_2: 1.2V LDO output adjust, default is 1 */
#define AU_ANA_CTRL_AULDO12_MASK                    (0x3UL << AU_ANA_CTRL_AULDO12_POS)

#define AU_ANA_SEL_INNER_IREF_VREF_POS              7U
#define AU_ANA_SEL_INNER_IREF_VREF_MASK             (0x1UL << AU_ANA_SEL_INNER_IREF_VREF_POS)

#define AU_ANA_PD_LDO12_POS                         6U                                                      /*!< CODEC_ANA_CTRL_2: Power down 1.2V LDO, 1=power down */
#define AU_ANA_PD_LDO12_MASK                        (0x1UL << AU_ANA_PD_LDO12_POS)

#define AU_ANA_PD_LDO28_POS                         5U                                                      /*!< CODEC_ANA_CTRL_2: Power down 2.8V LDO, 1=power down */
#define AU_ANA_PD_LDO28_MASK                        (0x1UL << AU_ANA_PD_LDO28_POS)

#define AU_ANA_EN_AU_TST_POS                        4U                                                      /*!< CODEC_ANA_CTRL_2: Enable audio test function */
#define AU_ANN_EN_AU_TST_MASK                       (0x1UL << AU_ANA_EN_AU_TST_POS)

#define AU_ANA_EN_AUDIO_BIAS_TST_POS                3U                                                      /*!< CODEC_ANA_CTRL_2: Enable audio bias current test 0=no test, 1=enable */
#define AU_ANA_EN_AUDIO_BIAS_TST_MASK               (0x1UL << AU_ANA_EN_AUDIO_BIAS_TST_POS)

#define AU_ANA_TSTSEL_POS                           0U                                                      /*!< CODEC_ANA_CTRL_2: Audio top test selection, 0=no test */
#define AU_ANA_TSTSEL_MASK                          (0x7UL << AU_ANA_TSTSEL_POS)

/** \brief AUDIO ADC Power sequence control */
#define AU_ANA_PWR_ADC_CTRL_POS                     10U                                                     /*!< CODEC_ANA_PWR_1: PGA and ADC control, 1: control by reg, 0: control by fsm */
#define AU_ANA_PWR_ADC_CTRL_MASK                    (0x1UL << AU_ANA_PWR_ADC_CTRL_POS)

#define AU_ANA_PWR_LDO_CTRL_POS                     9U                                                      /*!< CODEC_ANA_PWR_1: LDO and BIAS control, 1: control by reg, 0: control by fsm */
#define AU_ANA_PWR_LDO_CTRL_MASK                    (0x1UL << AU_ANA_PWR_LDO_CTRL_POS)

#define AU_ANA_PWR_ON_EN_POS                        8U                                                      /*!< CODEC_ANA_PWR_1: Audio power sequence ctrl, 1: power on,0: power off */
#define AU_ANA_PWR_ON_EN_MASK                       (0x1UL << AU_ANA_PWR_ON_EN_POS)

#define AU_ANA_PD_AUREF_TIME_POS                    0U                                                      /*!< CODEC_ANA_PWR_1: Slot time */
#define AU_ANA_PD_AUREF_TIME_MASK                   (0xFFUL << AU_ANA_PD_AUREF_TIME_POS)


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct
{
    __IO uint32_t ADC_DECI_FILT_11;
    __IO uint32_t ADC_DECI_FILT_12;
    __IO uint32_t ADC_DECI_FILT_13;
    __IO uint32_t ADC_DECI_FILT_21;
    __IO uint32_t ADC_DECI_FILT_22;
    __IO uint32_t ADC_DECI_FILT_23;
    __IO uint32_t ADC_DECI_FILT_31;
    __IO uint32_t ADC_DECI_FILT_32;
    __IO uint32_t ADC_DECI_FILT_33;
    __IO uint32_t ADC_DECI_FILT_41;
    __IO uint32_t ADC_DECI_FILT_42;
    __IO uint32_t ADC_DECI_FILT_43;
    __IO uint32_t ADC_DECI_FILT_51;
    __IO uint32_t ADC_DECI_FILT_52;
    __IO uint32_t ADC_DECI_FILT_53;
} OM_AUDIO_DECI_Type;

typedef struct
{
    __IO uint32_t ADC_IIR_FILT_11;
    __IO uint32_t ADC_IIR_FILT_12;
    __IO uint32_t ADC_IIR_FILT_13;
    __IO uint32_t ADC_IIR_FILT_21;
    __IO uint32_t ADC_IIR_FILT_22;
    __IO uint32_t ADC_IIR_FILT_23;
    __IO uint32_t ADC_IIR_FILT_31;
    __IO uint32_t ADC_IIR_FILT_32;
    __IO uint32_t ADC_IIR_FILT_33;
} OM_AUDIO_IIR_Type;

typedef struct
{
    __IO uint32_t       ADC_CTRL;                   /*!< offset:0x0000, ADC status control */
    OM_AUDIO_DECI_Type  DECI;                       /*!< offset:0x0004, ADC decimation filter */
    OM_AUDIO_IIR_Type   IIR;                        /*!< offset:0x0040, ADC iir filter */
    __IO uint32_t       REV0[5];                    /*!< offset:0x0064, Reserved */
    __IO uint32_t       ADC_VOL_CTRL;               /*!< offset:0x0078, ADC volume control */
    __IO uint32_t       ALC_CTRL_1;                 /*!< offset:0x007C, ALC function control */
    __IO uint32_t       ALC_CTRL_2;                 /*!< offset:0x0080, ALC function control */
    __IO uint32_t       REV1[15];                   /*!< offset:0x0084, Reserved */
    __I  uint32_t       CODEC_GAIN_READ;            /*!< offset:0x00C0, ADC gain read out */
    __IO uint32_t       CODEC_CLK_CTRL_1;           /*!< offset:0x00C4, ADC clock control */
    __IO uint32_t       REV2[5];                    /*!< offset:0x00C8, Reserved */
    __IO uint32_t       CODEC_INT_STATUS;           /*!< offset:0x00DC, Codec interrupt status */
    __IO uint32_t       CODEC_INT_CTRL;             /*!< offset:0x00E0, Codec interrupt control */
    __IO uint32_t       REV3[4];                    /*!< offset:0x00E4, Reserved */
    __IO uint32_t       ADC_THD_CTRL;               /*!< offset:0x00F4, ADC clip threshold control */
    __IO uint32_t       REV4[2];                    /*!< offset:0x00F8, Reserved */
    __IO uint32_t       CODEC_ANA_CTRL_1;           /*!< offset:0x0100, Analog control singals */
    __IO uint32_t       CODEC_ANA_CTRL_2;           /*!< offset:0x0104, Analog control singals */
    __IO uint32_t       REV5;                       /*!< offset:0x0108, Reserved */
    __IO uint32_t       CODEC_ANA_PWR_1;            /*!< offset:0x010C, Analog power sequence control */
} OM_AUDIO_Type;


#endif /* __AUDIO_REG_H */


/** @} */