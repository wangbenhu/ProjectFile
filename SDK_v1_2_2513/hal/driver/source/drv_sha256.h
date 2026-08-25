/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup SHA256 SHA256
 * @ingroup  DRIVER
 * @brief    SHA256 driver
 * @details  SHA256 driver apis and typedefs header file, implemeted by hardware.
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_SHA256_H
#define __DRV_SHA256_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_SHA256)
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
 * @brief Start SHA256 calculation.
 *
 *******************************************************************************
 */
extern void drv_sha256_start(void);

/**
 *******************************************************************************
 * @brief Update SHA256 calculation.
 * @note: This function can be called multiple times to update the hash
 * calculation with new data. The data can be any length.
 *
 * @param[in] data          Data to be hashed
 * @param[in] len           Length of data to be hashed
 *******************************************************************************
 */
extern void drv_sha256_update(const uint8_t *data, uint32_t len);

/**
 *******************************************************************************
 * @brief Finish SHA256 calculation.
 *
 * @param[out] hash          Hash result
 *******************************************************************************
 */
extern void drv_sha256_finish(uint8_t hash[32]);

#ifdef __cplusplus
}
#endif

#endif  /* (RTE_SHA256) */

#endif  /* __DRV_SHA256_H */


/** @} */
