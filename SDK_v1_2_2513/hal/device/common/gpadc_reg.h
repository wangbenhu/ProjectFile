/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup GPADC GPADC
 * @ingroup  DEVICE
 * @brief    GPADC register
 * @details  GPADC register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __GPADC_REG_H
#define __GPADC_REG_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include "../common/common_reg.h"


/*******************************************************************************
 * TYPEDEFS
 */
// INTR / INTR_MSK
#define GPADC_EOC_0_POS                 0
#define GPADC_EOC_1_POS                 1
#define GPADC_EOC_2_POS                 2
#define GPADC_EOC_3_POS                 3
#define GPADC_EOC_4_POS                 4
#define GPADC_EOC_5_POS                 5
#define GPADC_EOC_6_POS                 6
#define GPADC_EOC_7_POS                 7
#define GPADC_EOC_8_POS                 8
#define GPADC_EOC_9_POS                 9
#define GPADC_EOC_A_POS                 10
#define GPADC_EOC_B_POS                 11
#define GPADC_EOS_POS                   12
#define GPADC_EOA_POS                   13
#define GPADC_OVR_POS                   14
#define GPADC_EOC_0_MASK                0x00000001
#define GPADC_EOC_1_MASK                0x00000002
#define GPADC_EOC_2_MASK                0x00000004
#define GPADC_EOC_3_MASK                0x00000008
#define GPADC_EOC_4_MASK                0x00000010
#define GPADC_EOC_5_MASK                0x00000020
#define GPADC_EOC_6_MASK                0x00000040
#define GPADC_EOC_7_MASK                0x00000080
#define GPADC_EOC_8_MASK                0x00000100
#define GPADC_EOC_9_MASK                0x00000200
#define GPADC_EOC_A_MASK                0x00000400
#define GPADC_EOC_B_MASK                0x00000800
#define GPADC_EOS_MASK                  0x00001000
#define GPADC_EOA_MASK                  0x00002000
#define GPADC_OVR_MASK                  0x00004000
#define GPADC_INTR_ALL_MASK             0x00007FFF

// DLY_CFG
#define GPADC_CFG_CHG_DLY_POS           0
#define GPADC_PD_DLY_MAX_POS            8
#define GPADC_ORB_DLY_POS               16
#define GPADC_CFG_CHG_DLY_MASK          0x000000FF
#define GPADC_PD_DLY_MAX_MASK           0x0000FF00
#define GPADC_ORB_DLY_MASK              0x00FF0000

// ADC_CFG0
#define GPADC_PD_POS                    0
#define GPADC_PD_CORE_POS               1
#define GPADC_PD_REFBUF_POS             2
#define GPADC_RST_POS                   3
#define GPADC_EN_SCALE_POS              4
#define GPADC_PD_VBAT_DET_POS           5
#define GPADC_SEL_VREF_POS              6
#define GPADC_MODE_SEL_POS              7
#define GPADC_VCTRL_LDO_POS             8
#define GPADC_CTRL_VREF_POS             10
#define GPADC_EN_DEM_POS                12
#define GPADC_EN_TST_POS                13
#define GPADC_TST_SEL_POS               14
#define GPADC_STARTB_MO_POS             17
#define GPADC_STARTB_ME_POS             18
#define GPADC_PMU_TRIM_CTRL_POS         19
#define GPADC_PMU_TS_ICTRL_POS          21
#define GPADC_CMREG_EN_POS              23
#define GPADC_INPUT_SELN_ME_POS         24
#define GPADC_INPUT_SELN_MO_POS         25
#define GPADC_PD_MASK                   0x00000001
#define GPADC_PD_CORE_MASK              0x00000002
#define GPADC_PD_REFBUF_MASK            0x00000004
#define GPADC_RST_MASK                  0x00000008
#define GPADC_EN_SCALE_MASK             0x00000010
#define GPADC_PD_VBAT_DET_MASK          0x00000020
#define GPADC_SEL_VREF_MASK             0x00000040
#define GPADC_MODE_SEL_MASK             0x00000080
#define GPADC_VCTRL_LDO_MASK            0x00000300
#define GPADC_CTRL_VREF_MASK            0x00000C00
#define GPADC_EN_DEM_MASK               0x00001000
#define GPADC_EN_TST_MASK               0x00002000
#define GPADC_TST_SEL_MASK              0x0001C000
#define GPADC_STARTB_MO_MASK            0x00020000
#define GPADC_STARTB_ME_MASK            0x00040000
#define GPADC_PMU_TRIM_CTRL_MASK        0x00180000
#define GPADC_PMU_TS_ICTRL_MASK         0x00600000
#define GPADC_CMREG_EN_MASK             0x00800000
#define GPADC_INPUT_SELN_ME_MASK        0x01000000
#define GPADC_INPUT_SELN_MO_MASK        0x1E000000

// ADC_CFG1
#define GPADC_AUTO_PD1_POS              0
#define GPADC_AUTO_DELAY_POS            1
#define GPADC_AUTO_PD2_POS              2
#define GPADC_TRIG_MODE_POS             3
#define GPADC_TRIG_RES_POS              5
#define GPADC_TRIG_HW_SEL_POS           6
#define GPADC_DMA_EN_POS                11
#define GPADC_FILT_BYPASS_MO_POS        12
#define GPADC_FILT_BYPASS_ME_POS        13
#define GPADC_VIN_OUTPUT_MODE_POS       14
#define GPADC_AVG_BYPASS_POS            15
#define GPADC_SUM_NUM_POS               16
#define GPADC_SMP_TIME_POS              19
#define GPADC_DIGI_CLK_FREQ_POS         24
#define GPADC_AUTO_PD1_MASK             0x00000001
#define GPADC_AUTO_DELAY_MASK           0x00000002
#define GPADC_AUTO_PD2_MASK             0x00000004
#define GPADC_TRIG_MODE_MASK            0x00000018
#define GPADC_TRIG_RES_MASK             0x00000020
#define GPADC_TRIG_HW_SEL_MASK          0x000007C0
#define GPADC_DMA_EN_MASK               0x00000800
#define GPADC_FILT_BYPASS_MO_MASK       0x00001000
#define GPADC_FILT_BYPASS_ME_MASK       0x00002000
#define GPADC_VIN_OUTPUT_MODE_MASK      0x00004000
#define GPADC_AVG_BYPASS_MASK           0x00008000
#define GPADC_SUM_NUM_MASK              0x00070000
#define GPADC_SMP_TIME_MASK             0x00780000
#define GPADC_DIGI_CLK_FREQ_MASK        0xFF000000

// ADC_CFG2
#define GPADC_SCANDIR_POS               0
#define GPADC_SEQ_VECT_POS              1
#define GPADC_SW_TRIGGER_TRUE_POS       13
#define GPADC_SEQ_LIFE_POS              16
#define GPADC_SCANDIR_MASK              0x00000001
#define GPADC_SEQ_VECT_MASK             0x00001FFE
#define GPADC_SW_TRIGGER_TRUE_MASK      0x00002000
#define GPADC_SEQ_LIFE_MASK             0xFFFF0000

// CALI_CFG
#define GPADC_GAIN_ERR_POS              0
#define GPADC_VOS_POS                   14
#define GPADC_AUTO_COMPEN_POS           31
#define GPADC_GAIN_ERR_MASK             0x00003FFF
#define GPADC_VOS_MASK                  0x3FFFC000
#define GPADC_AUTO_COMPEN_MASK          0x80000000

// TEMP_CFG
#define GPADC_VOS_TEMP_POS              0
#define GPADC_VOS_TEMP_MASK             0x00003FFF

// CH_X_DATA
#define GPADC_DATA_LOW_POS              0
#define GPADC_DATA_LOW_MASK             0x0000FFFF

// CH_DMA_DATA
#define GPADC_CH_DMA_DATA_POS           0
#define GPADC_CH_DMA_DATA_MASK          0xFFFFFFFF

// DMA_CNS
#define GPADC_DFSM_POS                  0
#define GPADC_CH_ID_DMA_POS             4
#define GPADC_DFSM_MASK                 0x00000003
#define GPADC_CH_ID_DMA_MASK            0x000000F0

// ADC_DBG0
#define GPADC_CH_ID_SMP_POS             0
#define GPADC_DATA_RDY_VECT_POS         4
#define GPADC_SEQ_LIFE_CNT_POS          16
#define GPADC_CH_ID_SMP_MASK            0x0000000F
#define GPADC_DATA_RDY_VECT_MASK        0x0000FFF0
#define GPADC_SEQ_LIFE_CNT_MASK         0xFFFF0000

// ADC_DBG1
#define GPADC_SFSM_POS                  0
#define GPADC_ADC_START_POS             3
#define GPADC_OVR_BLOCK_POS             4
#define GPADC_SEQ_N_FINISH_POS          5
#define GPADC_INPUT_SEL_N_POS           6
#define GPADC_INPUT_SEL_P_POS           10
#define GPADC_SMP_VIN_POS               16
#define GPADC_SFSM_MASK                 0x00000007
#define GPADC_ADC_START_MASK            0x00000008
#define GPADC_OVR_BLOCK_MASK            0x00000010
#define GPADC_SEQ_N_FINISH_MASK         0x00000020
#define GPADC_INPUT_SEL_N_MASK          0x000003C0
#define GPADC_INPUT_SEL_P_MASK          0x00003C00
#define GPADC_SMP_VIN_MASK              0xFFFF0000

// ADC_SELN
#define GPADC_SELN_SDIR_POS             0
#define GPADC_SEQ_VECT_N_POS            1
#define GPADC_SEQ_LIFE_N_POS            16
#define GPADC_SELN_SDIR_MASK            0x00000001
#define GPADC_SEQ_VECT_N_MASK           0x00001FFE
#define GPADC_SEQ_LIFE_N_MASK           0xFFFF0000

// DCALI_CFG
#define GPADC_GAIN_ERR_DIFF_POS         0
#define GPADC_VOS_DIFF_POS              14
#define GPADC_GAIN_ERR_DIFF_MASK        0x00003FFF
#define GPADC_VOS_DIFF_MASK             0x3FFFC000

typedef struct {
    __IO uint32_t INTR;                 // offset: 0x00  Raw interrupt write 1 to clear
    __IO uint32_t INTR_MSK;             // offset: 0x04  Interrupt mask
    __IO uint32_t DLY_CFG;              // offset: 0x08  dly counter register
    __IO uint32_t ADC_CFG0;             // offset: 0x0C  Adc status register
    __IO uint32_t ADC_CFG1;             // offset: 0x10  Adc work mode register
    __IO uint32_t ADC_CFG2;             // offset: 0x14  Channel register
    __IO uint32_t CALI_CFG;             // offset: 0x18  Calib register
    __IO uint32_t TEMP_CFG;             // offset: 0x1C  Vos_temp register
    __IO uint32_t CH_0_DATA;            // offset: 0x20  Channel 0 output data
    __IO uint32_t CH_1_DATA;            // offset: 0x25  Channel 1 output data
    __IO uint32_t CH_2_DATA;            // offset: 0x28  Channel 2 output data
    __IO uint32_t CH_3_DATA;            // offset: 0x2C  Channel 3 output data
    __IO uint32_t CH_4_DATA;            // offset: 0x30  Channel 4 output data
    __IO uint32_t CH_5_DATA;            // offset: 0x34  Channel 5 output data
    __IO uint32_t CH_6_DATA;            // offset: 0x38  Channel 6 output data
    __IO uint32_t CH_7_DATA;            // offset: 0x3C  Channel 7 output data
    __IO uint32_t CH_8_DATA;            // offset: 0x40  Channel 8 output data
    __IO uint32_t CH_9_DATA;            // offset: 0x44  Channel 9 output data
    __IO uint32_t CH_A_DATA;            // offset: 0x48  Channel A output data
    __IO uint32_t CH_B_DATA;            // offset: 0x4C  Channel B output data
    __IO uint32_t CH_DMA_DATA;          // offset: 0x50  Output data in DMA mode
    __IO uint32_t DMA_CNS;              // offset: 0x54  Dma control and status
    __IO uint32_t ADC_DBG0;             // offset: 0x58  GPADC debug for channel information
    __IO uint32_t ADC_DBG1;             // offset: 0x5C  GPADC debug for power information
    __IO uint32_t ADC_SELN;             // offset: 0x60  Channel n register
    __IO uint32_t DCALI_CFG;            // offset: 0x64  Calib in diff mode register
} OM_GPADC_Type;


/*******************************************************************************
 * MACROS
 */


#endif  /* __GPADC_REG_H */


/** @} */
