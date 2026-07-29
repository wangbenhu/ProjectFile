/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup SM2 SM2
 * @ingroup  DRIVER
 * @brief    SM2 Driver
 * @details  SM2 driver apis and typedefs header files. Implemeted by software.
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_SM2_H
#define __DRV_SM2_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_SM2)
#include <string.h>
#include <stdint.h>
#include "gm.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
#define OM_SM2_EXCH_0               0
#define OM_SM2_EXCH_1               1


/*******************************************************************************
 * TYPEDEFS
 */


/*******************************************************************************
 * EXTERN VARIABLES
 */


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Generate sm2 public key from private key
 *
 * @param[in] private_key private key
 * @param[out] public_key public key
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_sm2_generate_public_key(const uint8_t private_key[32], uint8_t public_key[64]);

/**
 *******************************************************************************
 * @brief Generate sm2 key pair
 *
 * @param[out] private_key private key
 * @param[out] public_key public key
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_sm2_key_generate(uint8_t private_key[32], uint8_t public_key[64]);

/**
 *******************************************************************************
 * @brief key exchange initialization
 *
 * @param[in] om_sm2_exch sm2 exch ID
 * @param[in] private_key private key
 * @param[in] public_key public key
 * @param[in] rand_private_key random private key
 * @param[in] rand_public_key random public key
 * @param[in] isInitiator 1:requester, 0:responder
 * @param[in] id_bytes  user id
 * @param[in] idLen user id length
 *******************************************************************************
 */
extern void drv_sm2_exch_init(uint32_t om_sm2_exch,
                       uint8_t private_key[32], uint8_t public_key[64],
                       uint8_t rand_private_key[32], uint8_t rand_public_key[64],
                       unsigned char isInitiator,
                       const unsigned char *id_bytes, unsigned int idLen);

/**
 *******************************************************************************
 * @brief Calculate exchange key and hash
 *
 * @param[in] om_sm2_exch sm2 exch ID
 * @param[in] peer_p peer public key
 * @param[in] peer_rp peer random public key
 * @param[in] peer_id_bytes peer user id
 * @param[in] idLen peer user id length
 * @param[in] kLen key length
 * @param[out] output key || S1/SB || S2/SA，Len is kLen + 64
 *******************************************************************************
 */
extern void drv_sm2_exch_calculate(uint32_t om_sm2_exch,
                            const uint8_t *peer_p, const uint8_t *peer_rp,
                            const uint8_t *peer_id_bytes, uint32_t idLen,
                            uint32_t kLen,
                            uint8_t *output);

/**
 *******************************************************************************
 * @brief Calculate SM2 shared secret
 *
 * @param[in] peer_public_key peer public key
 * @param[in] private_key private key
 * @param[out] shared_secret shared secret
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_sm2_shared_secret(const uint8_t peer_public_key[64],
                                 const uint8_t private_key[32],
                                 uint8_t shared_secret[64]);

#ifdef __cplusplus
}
#endif

#endif  /* RTE_SM2 */

#endif  /* __DRV_SM2_H */


/** @} */