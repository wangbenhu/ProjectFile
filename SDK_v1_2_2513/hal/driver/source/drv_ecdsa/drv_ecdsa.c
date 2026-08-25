/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup DOC DOC
 * @ingroup  DOCUMENT
 * @brief    ECDSA driver source file
 * @details  ECDSA driver source file, implemeted by software.
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_ECDSA)
#include <stddef.h>
#include "om_device.h"
#include "om_driver.h"
#include "uECC/uECC.h"


/*******************************************************************************
 * CONST & VARIABLES
 */
static uint32_t lcg_seed = 1;


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static void lcg_srand(uint32_t seed)
{
    lcg_seed = seed;
}

static uint32_t lcg_rand(void)
{
    #define LCG_A       1664525U
    #define LCG_C       1013904223U
    #define LCG_MOD     4294967296U

    lcg_seed = ((uint64_t)lcg_seed * LCG_A + LCG_C) % LCG_MOD;

    return lcg_seed;
}

static int ecdsa_rng(uint8_t *dest, unsigned size)
{
    unsigned i;

    for (i = 0; i < size; i++) {
        dest[i] = lcg_rand() & 0xFF;
    }

    return 1;
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief ECDSA driver initialization
 *
 * @param[in] cfg  Pointer to ecdsa configuration
 *******************************************************************************
 */
void drv_ecdsa_init(ecdsa_config_t *cfg)
{
    OM_ASSERT(cfg != NULL);

    lcg_srand(cfg->random_seed);
    om_uecc_set_rng(ecdsa_rng);
}

/**
 *******************************************************************************
 * @brief Generate ECDSA public key from private key
 *
 * @param[in]  private_key   Pointer to private key, 4-byte alignment is required
 * @param[out] public_key    Pointer to generated public key, 4-byte alignment is required
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_ecdsa_generate_public_key(const void *private_key, void *public_key)
{
    OM_ASSERT(private_key != NULL && OM_IS_ALIGN(private_key, 4) &&
              public_key != NULL  && OM_IS_ALIGN(public_key, 4));

    if (om_uecc_compute_public_key((const uint8_t *)private_key, (uint8_t *)public_key, om_uecc_secp256r1())) {
        return OM_ERROR_OK;
    }

    return OM_ERROR_FAIL;
}

/**
 *******************************************************************************
 * @brief Generate ECDSA private key and public key
 *
 * @param[out] private_key   Pointer to generated private key, 4-byte alignment is required
 * @param[out] public_key    Pointer to generated public key, 4-byte alignment is required
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_ecdsa_generate_key_pair(void *private_key, void *public_key)
{
    OM_ASSERT(private_key != NULL && OM_IS_ALIGN(private_key, 4) &&
              public_key != NULL  && OM_IS_ALIGN(public_key, 4));

    if (om_uecc_make_key((uint8_t *)public_key, (uint8_t *)private_key, om_uecc_secp256r1())) {
        return OM_ERROR_OK;
    }

    return OM_ERROR_FAIL;
}

/**
 *******************************************************************************
 * @brief ECDSA sign
 *
 * @param[in] private_key   Pointer to private key, 4-byte alignment is required
 * @param[in] msg_hash      Pointer to message hash
 * @param[in] hash_size     Message hash size
 * @param[out] signature    Pointer to generated signature
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_ecdsa_sign(const void *private_key, const void *msg, uint32_t msg_len, void *signature)
{
    OM_ASSERT(private_key != NULL && OM_IS_ALIGN(private_key, 4) &&
              msg != NULL && signature != NULL);

    if (om_uecc_sign((const uint8_t *)private_key, (const uint8_t *)msg, msg_len, (uint8_t *)signature, om_uecc_secp256r1())) {
        return OM_ERROR_OK;
    }

    return OM_ERROR_FAIL;
}

/**
 *******************************************************************************
 * @brief ECDSA verify
 *
 * @param[in] public_key    Pointer to public key, 4-byte alignment is required
 * @param[in] msg_hash      Pointer to message hash
 * @param[in] hash_size     Message hash size
 * @param[in] signature     Pointer to signature
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_ecdsa_verify(const void *public_key, const void *msg, uint32_t msg_len, const void *signature)
{
    OM_ASSERT(public_key != NULL && OM_IS_ALIGN(public_key, 4) &&
              msg != NULL && signature != NULL);

    if (om_uecc_verify((const uint8_t *)public_key, (const uint8_t *)msg, msg_len, (const uint8_t *)signature, om_uecc_secp256r1())) {
        return OM_ERROR_OK;
    }

    return OM_ERROR_VERIFY;
}

/**
 *******************************************************************************
 * @brief ECDSA shared secret
 *
 * @param[in] peer_public_key   Pointer to peer public key, 4-byte alignment is required
 * @param[in] private_key       Pointer to private key, 4-byte alignment is required
 * @param[out] shared_secret    Pointer to shared secret
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_uecc_shared_secret(const void *peer_public_key, const void *private_key, void *shared_secret)
{
    if (om_uecc_shared_secret((const uint8_t *)peer_public_key, (const uint8_t *)private_key, (uint8_t *)shared_secret, om_uecc_secp256r1())) {
        return OM_ERROR_OK;
    }

    return OM_ERROR_FAIL;
}

/**
 *******************************************************************************
 * @brief ECDSA compress public key
 *
 * @param[in] public_key    Pointer to public key, 4-byte alignment is required
 * @param[out] compressed    Pointer to compressed public key
 *******************************************************************************
 */
void drv_uecc_compress(const void *public_key, void *compressed)
{
    om_uecc_compress(public_key, compressed, om_uecc_secp256r1());
}

/**
 *******************************************************************************
 * @brief ECDSA decompress public key
 *
 * @param[in] compressed     Pointer to compressed public key
 * @param[out] public_key    Pointer to decompressed public key
 *******************************************************************************
 */
void drv_uecc_decompress(const void *compressed, void *public_key)
{
    om_uecc_decompress(compressed, public_key, om_uecc_secp256r1());
}

#endif  /* (RTE_ECDSA) */

/** @} */
