/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup RNG RNG
 * @ingroup  DRIVER
 * @brief    RNG driver
 * @details  RNG driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_RNG_H
#define __DRV_RNG_H
/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_RNG)
#include <stdint.h>
#include "om_driver.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief read random numbers
 *
 * @param[out] random   Pointer to buffer that stores random numbers
 * @param[in]  num      Number of random numbers
 *******************************************************************************
 */
extern void drv_rng_read(uint8_t *random, uint32_t num);


#ifdef __cplusplus
}
#endif

#endif  /* RTE_RNG */

#endif  /* __DRV_RNG_H */


/** @} */