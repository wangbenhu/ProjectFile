/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup GPIO GPIO
 * @ingroup  DEVICE
 * @brief    GPIO register
 * @details  GPIO register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __GPIO_REG_H
#define __GPIO_REG_H


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


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    __I  uint32_t DATA;                         // offset:0x00
    __IO uint32_t DATAOUT;                      // offset:0x04  (Auto Restore)
         uint32_t RESERVED0[2];
    __IO uint32_t OUTENSET;                     // offset:0x10  (Auto Restore)
    __IO uint32_t OUTENCLR;                     // offset:0x14
    __IO uint32_t ALTFUNCSET;                   // offset:0x18
    __IO uint32_t ALTFUNCCLR;                   // offset:0x1C
    __IO uint32_t INTENSET;                     // offset:0x20  (Auto Restore)
    __IO uint32_t INTENCLR;                     // offset:0x24
    __IO uint32_t INTTYPESET;                   // offset:0x28  (Auto Restore)
    __IO uint32_t INTTYPECLR;                   // offset:0x2C
    __IO uint32_t INTPOLSET;                    // offset:0x30  (Auto Restore)
    __IO uint32_t INTPOLCLR;                    // offset:0x34
    __IO uint32_t INTSTATUS;                    // offset:0x38
         uint32_t RESERVED1;
    __IO uint32_t INTBOTHSET;                   // offset:0x40  (Auto Restore)
    __IO uint32_t INTBOTHCLR;                   // offset:0x44
         uint8_t  RESERVED3[0x1000-0x0048];

    __IO uint32_t MASK_0_7[0x100];              // offset:0x1000
    __IO uint32_t MASK_8_15[0x100];             // offset:0x1400
    __IO uint32_t MASK_16_23[0x100];            // offset:0x1800
    __IO uint32_t MASK_24_31[0x100];            // offset:0x1C00
} OM_GPIO_Type;


/*******************************************************************************
 * EXTERN VARIABLES
 */


/*******************************************************************************
 * EXTERN FUNCTIONS
 */


#ifdef __cplusplus
}
#endif


#endif  /* __GPIO_REG_H */


/** @} */

