/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup SHA256 SHA256
 * @ingroup  DEVICE
 * @brief    SHA256 register
 * @details  SHA256 register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __SHA256_REG_H
#define __SHA256_REG_H


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
/// SHA256 control register CTRL(offset 0x00)
#define SHA256_CTRL_INIT_POS                0
#define SHA256_CTRL_INIT_MASK               (1U << 0)            /* start SHA256 initial block*/
#define SHA256_CTRL_NEXT_POS                1
#define SHA256_CTRL_NEXT_MASK               (1U << 1)            /* start SHA256 next block*/
#define SHA256_CTRL_INTM_POS                2
#define SHA256_CTRL_INTM_MASK               (1U << 2)            /* interrupt mask */
#define SHA256_CTRL_INTC_POS                3
#define SHA256_CTRL_INTC_MASK               (1U << 3)            /* interrupt clear */

/// SHA256 status register STATUS(offset 0x04)
#define SHA256_STATUS_READY_POS             0
#define SHA256_STATUS_READY_MASK            (1U << 0)            /* ready for accepting input data */
#define SHA256_STATUS_VALID_POS             1
#define SHA256_STATUS_VALID_MASK            (1U << 1)            /* digest output valid */


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    __IO uint32_t    CTRL;                                  // offset address 0x00, SHA256 control register.
    __I  uint32_t    STATUS;                                // offset address 0x04, SHA256 status register.
    __IO uint32_t    RESERVED[14];
    __IO uint32_t    MSG[16];                               // offset address 0x40, SHA256 Message register.
    __IO uint32_t    DIG[8];                                // offset address 0x80, SHA256 Digest register.
} OM_SHA256_Type;


#ifdef __cplusplus
}
#endif


#endif /* __SHA256_REG_H */

/** @} */
