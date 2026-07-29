/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup CM4
 * @ingroup  DEVICE
 * @brief    CM4 register
 * @details  CM4 register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __CM4_REG_H
#define __CM4_REG_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include "common_reg.h"


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
  __IO uint32_t DHCSR;
  __O  uint32_t DCRSR;
  __IO uint32_t DCRDR;
  __IO uint32_t DEMCR;
} COREDEBUG_Type;


/*******************************************************************************
 * MACROS
 */
/* Debug Exception and Monitor Control Register Definitions */
#define COREDEBUG_DEMCR_TRCENA_POS         24U
#define COREDEBUG_DEMCR_TRCENA_MASK        (1U << 24)


#endif  /* __CM4_REG_H */


/** @} */
