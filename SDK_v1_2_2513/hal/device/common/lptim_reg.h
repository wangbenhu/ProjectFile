/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup LPTIM LPTIM
 * @ingroup  DEVICE
 * @brief    LPTIM register
 * @details  LPTIM register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __LPTIM_REG_H
#define __LPTIM_REG_H


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
// LPTIM_EN
#define LPTIM_EN_POS                                           0
#define LPTIM_EN_MASK                                          (0x1U << 0)

// LPTIM_CTRL
#define LPTIM_CTRL_CNTPRESC_POS                                8
#define LPTIM_CTRL_CNTPRESC_MASK                               (0xfU << 8)
#define LPTIM_CTRL_CNTTOPEN_POS                                7
#define LPTIM_CTRL_CNTTOPEN_MASK                               (0x1U << 7)
#define LPTIM_CTRL_BUFTOP_POS                                  6
#define LPTIM_CTRL_BUFTOP_MASK                                 (0x1U << 6)
#define LPTIM_CTRL_STOPMODE_POS                                4
#define LPTIM_CTRL_STOPMODE_MASK                               (0x3U << 4)
#define LPTIM_CTRL_REPMODE23_POS                               2
#define LPTIM_CTRL_REPMODE23_MASK                              (0x3U << 2)
#define LPTIM_CTRL_REPMODE01_POS                               0
#define LPTIM_CTRL_REPMODE01_MASK                              (0x3U << 0)

// LPTIM_CMD
#define LPTIM_CMD_CTO3_POS                                     6
#define LPTIM_CMD_CTO3_MASK                                    (0x1U << 6)
#define LPTIM_CMD_CTO2_POS                                     5
#define LPTIM_CMD_CTO2_MASK                                    (0x1U << 5)
#define LPTIM_CMD_CTO1_POS                                     4
#define LPTIM_CMD_CTO1_MASK                                    (0x1U << 4)
#define LPTIM_CMD_CTO0_POS                                     3
#define LPTIM_CMD_CTO0_MASK                                    (0x1U << 3)
#define LPTIM_CMD_CLEAR_POS                                    2
#define LPTIM_CMD_CLEAR_MASK                                   (0x1U << 2)
#define LPTIM_CMD_STOP_POS                                     1
#define LPTIM_CMD_STOP_MASK                                    (0x1U << 1)
#define LPTIM_CMD_START_POS                                    0
#define LPTIM_CMD_START_MASK                                   (0x1U << 0)

// LPTIM_STAT
#define LPTIM_STAT_RUNNING_POS                                 0
#define LPTIM_STAT_RUNNING_MASK                                (0x1U << 0)

// LPTIM_CNT
#define LPTIM_CNT_POS                                          0
#define LPTIM_CNT_MASK                                         (0xffffU << 0)

// LPTIM_TOP
#define LPTIM_TOP_POS                                          0
#define LPTIM_TOP_MASK                                         (0xffffU << 0)

// LPTIM_TOPBUFF
#define LPTIM_TOPBUFF_POS                                      0
#define LPTIM_TOPBUFF_MASK                                     (0xffffU << 0)

// LPTIM_INTF
#define LPTIM_INTF_REP3_FLG_POS                                8
#define LPTIM_INTF_REP3_FLG_MASK                               (0x1U << 8)
#define LPTIM_INTF_REP2_FLG_POS                                7
#define LPTIM_INTF_REP2_FLG_MASK                               (0x1U << 7)
#define LPTIM_INTF_COMP3_FLG_POS                               6
#define LPTIM_INTF_COMP3_FLG_MASK                              (0x1U << 6)
#define LPTIM_INTF_COMP2_FLG_POS                               5
#define LPTIM_INTF_COMP2_FLG_MASK                              (0x1U << 5)
#define LPTIM_INTF_REP1_FLG_POS                                4
#define LPTIM_INTF_REP1_FLG_MASK                               (0x1U << 4)
#define LPTIM_INTF_REP0_FLG_POS                                3
#define LPTIM_INTF_REP0_FLG_MASK                               (0x1U << 3)
#define LPTIM_INTF_COMP1_FLG_POS                               2
#define LPTIM_INTF_COMP1_FLG_MASK                              (0x1U << 2)
#define LPTIM_INTF_COMP0_FLG_POS                               1
#define LPTIM_INTF_COMP0_FLG_MASK                              (0x1U << 1)
#define LPTIM_INTF_UF_FLG_POS                                  0
#define LPTIM_INTF_UF_FLG_MASK                                 (0x1U << 0)
#define LPTIM_INTF_ALL_POS                                     0
#define LPTIM_INTF_ALL_MASK                                    (0x1FFU << 0)

// LPTIM_INTE
#define LPTIM_INTE_REP3_EN_POS                                 8
#define LPTIM_INTE_REP3_EN_MASK                                (0x1U << 8)
#define LPTIM_INTE_REP2_EN_POS                                 7
#define LPTIM_INTE_REP2_EN_MASK                                (0x1U << 7)
#define LPTIM_INTE_COMP3_EN_POS                                6
#define LPTIM_INTE_COMP3_EN_MASK                               (0x1U << 6)
#define LPTIM_INTE_COMP2_EN_POS                                5
#define LPTIM_INTE_COMP2_EN_MASK                               (0x1U << 5)
#define LPTIM_INTE_REP1_EN_POS                                 4
#define LPTIM_INTE_REP1_EN_MASK                                (0x1U << 4)
#define LPTIM_INTE_REP0_EN_POS                                 3
#define LPTIM_INTE_REP0_EN_MASK                                (0x1U << 3)
#define LPTIM_INTE_COMP1_EN_POS                                2
#define LPTIM_INTE_COMP1_EN_MASK                               (0x1U << 2)
#define LPTIM_INTE_COMP0_EN_POS                                1
#define LPTIM_INTE_COMP0_EN_MASK                               (0x1U << 1)
#define LPTIM_INTE_UF_EN_POS                                   0
#define LPTIM_INTE_UF_EN_MASK                                  (0x1U << 0)

// LPTIM_SYNCBUSY
#define LPTIM_SYNCBUSY_CTO3_POS                                12
#define LPTIM_SYNCBUSY_CTO3_MASK                               (0x1U << 12)
#define LPTIM_SYNCBUSY_CTO2_POS                                11
#define LPTIM_SYNCBUSY_CTO2_MASK                               (0x1U << 11)
#define LPTIM_SYNCBUSY_REP3_BUSY_POS                           10
#define LPTIM_SYNCBUSY_REP3_BUSY_MASK                          (0x1U << 10)
#define LPTIM_SYNCBUSY_REP2_BUSY_POS                           9
#define LPTIM_SYNCBUSY_REP2_BUSY_MASK                          (0x1U << 9)
#define LPTIM_SYNCBUSY_CTO1_POS                                8
#define LPTIM_SYNCBUSY_CTO1_MASK                               (0x1U << 8)
#define LPTIM_SYNCBUSY_CTO0_POS                                7
#define LPTIM_SYNCBUSY_CTO0_MASK                               (0x1U << 7)
#define LPTIM_SYNCBUSY_CLEAR_POS                               6
#define LPTIM_SYNCBUSY_CLEAR_MASK                              (0x1U << 6)
#define LPTIM_SYNCBUSY_STOP_POS                                5
#define LPTIM_SYNCBUSY_STOP_MASK                               (0x1U << 5)
#define LPTIM_SYNCBUSY_START_POS                               4
#define LPTIM_SYNCBUSY_START_MASK                              (0x1U << 4)
#define LPTIM_SYNCBUSY_REP1_BUSY_POS                           3
#define LPTIM_SYNCBUSY_REP1_BUSY_MASK                          (0x1U << 3)
#define LPTIM_SYNCBUSY_REP0_BUSY_POS                           2
#define LPTIM_SYNCBUSY_REP0_BUSY_MASK                          (0x1U << 2)
#define LPTIM_SYNCBUSY_TOP_BUSY_POS                            1
#define LPTIM_SYNCBUSY_TOP_BUSY_MASK                           (0x1U << 1)
#define LPTIM_SYNCBUSY_CNT_BUSY_POS                            0
#define LPTIM_SYNCBUSY_CNT_BUSY_MASK                           (0x1U << 0)

// LPTIM_COMP0
#define LPTIM_COMP0_POS                                        0
#define LPTIM_COMP0_MASK                                       (0xffffU << 0)

// LPTIM_COMP1
#define LPTIM_COMP1_POS                                        0
#define LPTIM_COMP1_MASK                                       (0xffffU << 0)

// LPTIM_COMP2
#define LPTIM_COMP2_POS                                        0
#define LPTIM_COMP2_MASK                                       (0xffffU << 0)

// LPTIM_COMP3
#define LPTIM_COMP3_POS                                        0
#define LPTIM_COMP3_MASK                                       (0xffffU << 0)

// LPTIM_REP0
#define LPTIM_REP0_POS                                         0
#define LPTIM_REP0_MASK                                        (0xffU << 0)

// LPTIM_REP1
#define LPTIM_REP1_POS                                         0
#define LPTIM_REP1_MASK                                        (0xffU << 0)

// LPTIM_REP2
#define LPTIM_REP2POS                                         0
#define LPTIM_REP2_MASK                                        (0xffU << 0)

// LPTIM_REP3
#define LPTIM_REP3_POS                                         0
#define LPTIM_REP3_MASK                                        (0xffU << 0)

// LPTIM_CH0_CFG
#define LPTIM_CH0_CFG_UFOA_POS                                 1
#define LPTIM_CH0_CFG_UFOA_MASK                                (0x3U << 1)
#define LPTIM_CH0_CFG_OPOL_POS                                 0
#define LPTIM_CH0_CFG_OPOL_MASK                                (0x1U << 0)
// LPTIM_CH1_C
#define LPTIM_CH1_CFG_UFOA_POS                                 1
#define LPTIM_CH1_CFG_UFOA_MASK                                (0x3U << 1)
#define LPTIM_CH1_CFG_OPOL_POS                                 0
#define LPTIM_CH1_CFG_OPOL_MASK                                (0x1U << 0)
// LPTIM_CH2_C
#define LPTIM_CH2_CFG_UFOA_POS                                 1
#define LPTIM_CH2_CFG_UFOA_MASK                                (0x3U << 1)
#define LPTIM_CH2_CFG_OPOL_POS                                 0
#define LPTIM_CH2_CFG_OPOL_MASK                                (0x1U << 0)
// LPTIM_CH3_C
#define LPTIM_CH3_CFG_UFOA_POS                                 1
#define LPTIM_CH3_CFG_UFOA_MASK                                (0x3U << 1)
#define LPTIM_CH3_CFG_OPOL_POS                                 0
#define LPTIM_CH3_CFG_OPOL_MASK                                (0x1U << 0)

/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    __IO uint32_t EN;                                      // offset: 0x00
    __IO uint32_t CTRL;                                    // offset: 0x04
    __IO uint32_t CMD;                                     // offset: 0x08
    __I  uint32_t STAT;                                    // offset: 0x0C
    __IO uint32_t CNT;                                     // offset: 0x10
    __IO uint32_t TOP;                                     // offset: 0x14
    __IO uint32_t TOPBUFF;                                 // offset: 0x18
    __IO uint32_t INTF;                                    // offset: 0x1C
    __IO uint32_t INTE;                                    // offset: 0x20
    __I  uint32_t SYNCBUSY;                                // offset: 0x24
         uint8_t  Reserved28_2c[0x2c - 0x28 + 0x4];
    __IO uint32_t COMP0;                                   // offset: 0x30
    __IO uint32_t COMP1;                                   // offset: 0x34
    __IO uint32_t COMP2;                                   // offset: 0x38
    __IO uint32_t COMP3;                                   // offset: 0x3C
    __IO uint32_t REP0;                                    // offset: 0x40
    __IO uint32_t REP1;                                    // offset: 0x44
    __IO uint32_t REP2;                                    // offset: 0x48
    __IO uint32_t REP3;                                    // offset: 0x4C
    __IO uint32_t CH0_CFG;                                 // offset: 0x50
    __IO uint32_t CH1_CFG;                                 // offset: 0x54
    __IO uint32_t CH2_CFG;                                 // offset: 0x58
    __IO uint32_t CH3_CFG;                                 // offset: 0x5C
} OM_LPTIM_Type;


/*******************************************************************************
 * EXTERN VARIABLES
 */


/*******************************************************************************
 * EXTERN FUNCTIONS
 */


#ifdef __cplusplus
}
#endif

#endif  /* __LPTIM_REG_H */


/** @} */
