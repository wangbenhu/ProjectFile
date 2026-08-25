/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup RNG RNG
 * @ingroup  DEVICE
 * @brief    RNG register
 * @details  RNG register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __RNG_REG_H
#define __RNG_REG_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include "common_reg.h"


/*******************************************************************************
 * MACROS
 */
#define RNG_READY_POS           0
#define RNG_READY_MASK          (1U << 0)

#define RNG_READY_CLR_POS       4
#define RNG_READY_CLR_MASK      (1U << 4)


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    __IO uint32_t RANDOM;
    __IO uint32_t RDY;
} OM_RNG_Type;


/*******************************************************************************
 * MACROS
 */


#endif  /* __RNG_REG_H */


/** @} */
