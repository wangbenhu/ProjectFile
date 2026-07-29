/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup SM3 SM3
 * @ingroup  DRIVER
 * @brief    SM3 driver
 * @details  SM3 driver apis and typedefs header file. Implemeted by software.
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_SM3_H
#define __DRV_SM3_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_SM3)
#include <string.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
#define OM_SM3_HMAC_0                   0

#define SM3_DIGEST_SIZE		            32
#define SM3_BLOCK_SIZE		            64
#define SM3_STATE_WORDS		            8


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
	uint32_t    digest[SM3_STATE_WORDS];
	uint64_t    nblocks;
	uint8_t     block[SM3_BLOCK_SIZE];
	uint32_t    num;
} sm3_ctx_t;


/*******************************************************************************
 * EXTERN VARIABLES
 */


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief       SM3 calculate start
 *
 * @param[in] ctx   SM3 context
 *******************************************************************************
 */
extern void drv_sm3_start(sm3_ctx_t *ctx);

/**
 *******************************************************************************
 * @brief      SM3 calculate update
 *
 * @param[in] ctx   SM3 context
 * @param[in] data  Data to be calculated
 * @param[in] data_len  Data length
 *******************************************************************************
 */
extern void drv_sm3_update(sm3_ctx_t *ctx, const uint8_t *data, uint32_t data_len);

/**
 *******************************************************************************
 * @brief    SM3 calculate stop
 *
 * @param[in] ctx   SM3 context
 * @param[in] digest  SM3 digest
 *******************************************************************************
 */
extern void drv_sm3_stop(sm3_ctx_t *ctx, uint8_t digest[SM3_DIGEST_SIZE]);

/**
 *******************************************************************************
 * @brief    SM3 HMAC calculate start
 *
 * @param[in] om_sm3_hmac   SM3 HMAC ID
 * @param[in] key           Pointer to hmac key
 * @param[in] key_len       Key length
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_sm3_hmac_start(uint32_t om_sm3_hmac, const uint8_t *key, uint32_t key_len);

/**
 *******************************************************************************
 * @brief    SM3 HMAC calculate update
 *
 * @param[in] om_sm3_hmac   SM3 HMAC ID
 * @param[in] data          Pointer to data to be calculated
 * @param[in] data_len      Data length
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_sm3_hmac_update(uint32_t om_sm3_hmac, const uint8_t *data, uint32_t data_len);

/**
 *******************************************************************************
 * @brief    SM3 HMAC calculate stop
 *
 * @param[in] om_sm3_hmac   SM3 HMAC ID
 * @param[out] mac          Pointer to result mac
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_sm3_hmac_stop(uint32_t om_sm3_hmac, uint8_t mac[SM3_DIGEST_SIZE]);


#ifdef __cplusplus
}
#endif

#endif  /* (RTE_SM3) */

#endif  /* __DRV_SM3_H */


/** @} */