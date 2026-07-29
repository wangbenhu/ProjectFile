/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup EFUSE EFUSE
 * @ingroup  DEVICE
 * @brief    EFUSE register
 * @details  EFUSE register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __EFUSE_REG_H
#define __EFUSE_REG_H


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
// EFUSE PROGRAM_ENABLE (offset 0xF00)
#define EFUSE_PROGRAM_ENABLE_POS                         0
#define EFUSE_PROGRAM_ENABLE_MASK                       (1U << 0)

// EFUSE PROGRAM_START (offset 0xF04)
#define EFUSE_PROGRAM_START_POS                          0U
#define EFUSE_PROGRAM_START_MASK                        (1U << 0)

// EFUSE AVDD_TIM_CFG (offset 0xF08)
#define EFUSE_AVDD_TIMING_CFG_POS                        0U
#define EFUSE_AVDD_TIMING_CFG_MASK                       (0xFFU << 0)

// EFUSE PROGRAM_ADDRESS (offset 0xF0C)
#define EFUSE_PROGRAM_ADDRESS_POS                        0U
#define EFUSE_PROGRAM_ADDRESS_MASK                       (0xFFU << 0)

// EFUSE PROGRAM_DATA (offset 0xF10)
#define EFUSE_PROGRAM_DATA_POS                           0U
#define EFUSE_PROGRAM_DATA_MASK                          (0xFFU << 0)

// EFUSE CTRL (offset 0xF14)
#define EFUSE_CTRL_PROGRAM_FORBID_POS                    0U
#define EFUSE_CTRL_PROGRAM_FORBID_MASK                   (1U << 0)
#define EFUSE_CTRL_UID_READ_POS                          1U
#define EFUSE_CTRL_UID_READ_MASK                         (1U << 1)
#define EFUSE_CTRL_PMU_PROG_FORBID_POS                   2U
#define EFUSE_CTRL_PMU_PROG_FORBID_MASK                  (0xFU << 2)

// EFUSE PROGRAM_INTR (offset 0xF28)
#define EFUSE_PROGRAM_INTR_INT_EN_POS                    0U
#define EFUSE_PROGRAM_INTR_INT_EN_MASK                   (1U << 0)
#define EFUSE_PROGRAM_INTR_INT_CLR_POS                   1U
#define EFUSE_PROGRAM_INTR_INT_CLR_MASK                  (1U << 1)
#define EFUSE_PROGRAM_INTR_INT_GEN_POS                   2U
#define EFUSE_PROGRAM_INTR_INT_GEN_MASK                  (1U << 2)

// EFUSE STATUS (offset 0xF2C)
#define EFUSE_STATUS_CTRL_STATE_POS                      0U
#define EFUSE_STATUS_CTRL_STATE_MASK                     (1U << 0)
#define EFUSE_STATUS_ERROR_SET_PROG_POS                  2U
#define EFUSE_STATUS_ERROR_SET_PROG_MASK                 (1U << 2)
#define EFUSE_STATUS_ERROR_SET_READ_POS                  3U
#define EFUSE_STATUS_ERROR_SET_READ_MASK                 (1U << 3)
#define EFUSE_STATUS_CTRL_STATE_DEBUG_POS                8U
#define EFUSE_STATUS_CTRL_STATE_DEBUG_MASK               (0xFU << 8)

// EFUSE AVDD TIMING(ns)
#define EFUSE_AVDD_TIMING_NS                             (10 * 1000)

// EFUSE PROGRAM TIMING(ns)
#define EFUSE_PROGRAM_CFG0_L16_NS                        1000U
#define EFUSE_PROGRAM_CFG0_H16_NS                        100U
#define EFUSE_PROGRAM_CFG1_L16_NS                        11900U
#define EFUSE_PROGRAM_CFG1_H16_NS                        10000U
#define EFUSE_PROGRAM_CFG2_L16_NS                        50U

// EFUSE READ TIMING(ns)
#define EFUSE_READ_CFG_L8_NS                             40U
#define EFUSE_READ_CFG_L16_NS                            (EFUSE_READ_CFG_L8_NS + 35)
#define EFUSE_READ_CFG_H24_NS                            10U
#define EFUSE_READ_CFG_H32_NS                            100U


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    __I  uint8_t   READ_DATA[64];   // offset:0x00
         uint8_t   RSVD[0xF00-64];
    __IO uint32_t  PROGRAM_ENABLE;  // offset:0xF00
    __IO uint32_t  PROGRAM_START;   // offset:0xF04
    __IO uint32_t  AVDD_TIMING_CFG; // offset:0xF08
    __IO uint32_t  PROGRAM_ADDRESS; // offset:0xF0C
    __IO uint32_t  PROGRAM_DATA;    // offset:0xF10
    __IO uint32_t  CTRL;            // offset:0xF14
    __IO uint32_t  PROGRAM_CFG0;    // offset:0xF18
    __IO uint32_t  PROGRAM_CFG1;    // offset:0xF1C
    __IO uint32_t  PROGRAM_CFG2;    // offset:0xF20
    __IO uint32_t  READ_CFG;        // offset:0xF24
    __IO uint32_t  PROGRAM_INTR;    // offset:0xF28
    __I  uint32_t  STATUS;          // offset:0xF2C
} OM_EFUSE_Type;


/*******************************************************************************
 * EXTERN VARIABLES
 */


/*******************************************************************************
 * EXTERN FUNCTIONS
 */


#ifdef __cplusplus
}
#endif


#endif  /* __EFUSE_REG_H */


/** @} */

