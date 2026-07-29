/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup AES_HW AES_HW
 * @ingroup  DEVICE
 * @brief    AES_HW register
 * @details  AES_HW register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __AES_HW_REG_H
#define __AES_HW_REG_H


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
#define AES_HW_CTRL_START_POS                                          0
#define AES_HW_CTRL_START_MASK                                         (1U << 0)
#define AES_HW_CTRL_MODE_POS                                           1
#define AES_HW_CTRL_MODE_MASK                                          (1U << 1)
#define AES_HW_CTRL_DONE_POS                                           4
#define AES_HW_CTRL_DONE_MASK                                          (1U << 4)
#define AES_HW_CTRL_INTR_EN_POS                                        8
#define AES_HW_CTRL_INTR_EN_MASK                                       (1U << 8)

// AES CCM CTRL
#define AES_HW_CCM_CTRL_START_POS                                      0
#define AES_HW_CCM_CTRL_START_MASK                                     (1U << 0)
#define AES_HW_CCM_CTRL_ENABLE_POS                                     1
#define AES_HW_CCM_CTRL_ENABLE_MASK                                    (1U << 1)
#define AES_HW_CCM_CTRL_END_INT_STATUS_POS                             2
#define AES_HW_CCM_CTRL_END_INT_STATUS_MASK                            (1U << 2)
#define AES_HW_CCM_CTRL_MODE_SEL_POS                                   3
#define AES_HW_CCM_CTRL_MODE_SEL_MASK                                  (1U << 3)
#define AES_HW_CCM_CTRL_END_INT_POS                                    4
#define AES_HW_CCM_CTRL_END_INT_MASK                                   (1U << 4)
#define AES_HW_CCM_CTRL_MICERR_INT_POS                                 5
#define AES_HW_CCM_CTRL_MICERR_INT_MASK                                (1U << 5)
#define AES_HW_CCM_CTRL_MICERR_ACK_POS                                 6
#define AES_HW_CCM_CTRL_MICERR_ACK_MASK                                (1U << 6)
#define AES_HW_CCM_CTRL_MICERR_INT_STATUS_POS                          7
#define AES_HW_CCM_CTRL_MICERR_INT_STATUS_MASK                         (1U << 7)
#define AES_HW_CCM_CTRL_ABORT_INT_STATUS_POS                           8
#define AES_HW_CCM_CTRL_ABORT_INT_STATUS_MASK                          (1U << 8)
#define AES_HW_CCM_CTRL_ABORT_INT_POS                                  9
#define AES_HW_CCM_CTRL_ABORT_INT_MASK                                 (1U << 9)

#define AES_HW_CLOCK_EN_WHEN_USING_BLE_REG                             (*(volatile uint32_t *)0x41300220)


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    __IO uint32_t CTRL;                 // offset: 0x000
         uint32_t RESERVED0[63];
    __IO uint32_t KEY[8];               // offset: 0x100
    __IO uint32_t DATA[4];              // offset: 0x120
         uint32_t RESERVED1[993];
    // CCM
    __IO uint32_t CCM_KEY[4];           // offset: 0x10B4
         uint32_t RESERVED2[91];
    __IO uint32_t CCM_SRCADDR;          // offset: 0x1230
    __IO uint32_t CCM_NONCE[3];         // offset: 0x1234
    __IO uint32_t CCM_NONCE3_LEN;       // offset: 0x1240
    __IO uint32_t CCM_BLOCK1;           // offset: 0x1244
    __IO uint32_t CCM_CTRL;             // offset: 0x1248
    __IO uint32_t CCM_DSTADDR;          // offset: 0x124C
    __IO uint32_t CCM_TX_MIC;           // offset: 0x1250
    __IO uint32_t CCM_RX_MIC;           // offset: 0x1254
} OM_AES_HW_Type;


/*******************************************************************************
 * EXTERN VARIABLES
 */


/*******************************************************************************
 * EXTERN FUNCTIONS
 */


#ifdef __cplusplus
}
#endif


#endif  /* __AES_HW_REG_H */


/** @} */

