/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup BTMAC BTMAC
 * @ingroup  DEVICE
 * @brief    BTMAC register
 * @details  BTMAC register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __BTMAC_REG_H
#define __BTMAC_REG_H


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
         uint32_t RESERVE1[136];
    __IO uint32_t EM_CLOCK_EN;                // offset: 0x220
} OM_BTMAC_Type;


/*******************************************************************************
 * EXTERN VARIABLES
 */


/*******************************************************************************
 * EXTERN FUNCTIONS
 */


#ifdef __cplusplus
}
#endif


#endif  /* __BTMAC_REG_H */

/** @} */

