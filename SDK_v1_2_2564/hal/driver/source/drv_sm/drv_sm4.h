/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup SM4 SM4
 * @ingroup  DRIVER
 * @brief    SM4 driver
 * @details  SM4 driver apis and typedefs header file. Implemeted by software.
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_SM4_H
#define __DRV_SM4_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_SM4)
#include <stdint.h>
#include <string.h>
#include "om_driver.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
#define OM_SM4_CCM_0                0

#define SM4_KEY_SIZE		        16
#define SM4_BLOCK_SIZE		        16
#define SM4_NUM_ROUNDS		        32

#define SM4_CCM_MIN_IV_SIZE         7
#define SM4_CCM_MAX_IV_SIZE         13
#define SM4_CCM_MIN_TAG_SIZE        4
#define SM4_CCM_MAX_TAG_SIZE        16


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
	uint32_t rk[SM4_NUM_ROUNDS];
} sm4_ctx_t;

typedef struct {
    sm4_ctx_t sm4_ctx;
	uint8_t   iv[16];
	uint32_t  ivlen;
} sm4_cbc_mac_ctx_t;


/*******************************************************************************
 * EXTERN VARIABLES
 */


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief SM4 encryption key setting
 *
 * @param[in] ctx SM4 context
 * @param[in] user_key SM4 user key
 *******************************************************************************
 */
void drv_sm4_set_encrypt_key(sm4_ctx_t *ctx, const uint8_t user_key[16]);

/**
 *******************************************************************************
 * @brief SM4 decryption key setting
 *
 * @param[in] ctx SM4 context
 * @param[in] user_key SM4 user key
 *******************************************************************************
 */
void drv_sm4_set_decrypt_key(sm4_ctx_t *ctx, const uint8_t user_key[16]);

/**
 *******************************************************************************
 * @brief SM4 encryption
 *
 * @param[in] ctx SM4 context
 * @param[in] in input data
 * @param[out] out output data
 *******************************************************************************
 */
void drv_sm4_encrypt(const sm4_ctx_t *ctx, const uint8_t in[16], uint8_t out[16]);

/**
 *******************************************************************************
 * @brief SM4 CBC intialization
 *
 * @param[in] ctx SM4 CBC context
 * @param[in] key SM4 key
 *******************************************************************************
 */
void drv_sm4_cbc_mac_init(sm4_cbc_mac_ctx_t *ctx, const uint8_t key[16]);

/**
 *******************************************************************************
 * @brief SM4 CBC update
 *
 * @param[in] ctx       SM4 CBC context
 * @param[in] data      input data
 * @param[in] datalen   input data length
 *******************************************************************************
 */
void drv_sm4_cbc_mac_update(sm4_cbc_mac_ctx_t *ctx, const uint8_t *data, uint32_t datalen);

/**
 *******************************************************************************
 * @brief SM4 CBC finalization
 *
 * @param[in] ctx SM4 CBC context
 * @param[out] mac output mac
 *******************************************************************************
 */
void drv_sm4_cbc_mac_finish(sm4_cbc_mac_ctx_t *ctx, uint8_t mac[16]);

/**
 *******************************************************************************
 * @brief SM4 CCM encryption
 *
 * @param[in] om_sm4_ccm SM4 CCM ID
 * @param[in] key SM4 key
 * @param[in] iv initialization vector
 * @param[in] ivlen initialization vector length
 * @param[in] aad additional authenticated data
 * @param[in] aadlen additional authenticated data length
 * @param[in] in plaintext data
 * @param[in] inlen plaintext data length
 * @param[out] out encrypted data
 * @param[out] tag authentication tag
 * @param[in] taglen authentication tag length
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_sm4_ccm_encrypt(uint32_t om_sm4_ccm,
                               const uint8_t key[16],
                               const uint8_t *iv, uint32_t ivlen,
	                           const uint8_t *aad, size_t aadlen,
                               const uint8_t *in, uint32_t inlen, uint8_t *out,
                               uint8_t *tag, uint32_t taglen);

/**
 *******************************************************************************
 * @brief SM4 CCM decryption
 *
 * @param[in] om_sm4_ccm SM4 CCM ID
 * @param[in] key SM4 key
 * @param[in] iv initialization vector
 * @param[in] ivlen initialization vector length
 * @param[in] aad additional authenticated data
 * @param[in] aadlen additional authenticated data length
 * @param[in] in encrypted data
 * @param[in] inlen encrypted data length
 * @param[out] out plaintext data
 * @param[in] tag authentication tag
 * @param[in] taglen authentication tag length
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_sm4_ccm_decrypt(uint32_t om_sm4_ccm,
                               const uint8_t key[16],
                               const uint8_t *iv, uint32_t ivlen,
	                           const uint8_t *aad, uint32_t aadlen,
                               const uint8_t *in, uint32_t inlen, uint8_t *out,
	                           const uint8_t *tag, uint32_t taglen);

#ifdef __cplusplus
}
#endif

#endif  /* RTE_SM4 */

#endif  /* __DRV_SM4_H */


/** @} */