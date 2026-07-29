/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup AES AES
 * @ingroup  DRIVER
 * @brief    AES driver
 * @details  AES driver apis and typedefs header file
 *          ECB Encrypt: implemented by hardware when keybits=128/256, otherwise implemented by software.
 *          ECB Decrypt: implemented by software
 *          CBC Encrypt: implemented by hardware when keybits=128/256, otherwise implemented by software.
 *          CBC Decrypt: implemented by software
 *          CFB Encrypt: implemented by hardware when keybits=128/256, otherwise implemented by software.
 *          CFB Decrypt: implemented by hardware when keybits=128/256, otherwise implemented by software.
 *          OFB Encrypt: implemented by hardware when keybits=128/256, otherwise implemented by software.
 *          OFB Decrypt: implemented by hardware when keybits=128/256, otherwise implemented by software.
 *          CTR Encrypt: implemented by hardware when keybits=128/256, otherwise implemented by software.
 *          CTR Decrypt: implemented by hardware when keybits=128/256, otherwise implemented by software.
 *          GCM Encrypt: implemented by hardware when keybits=128/256, otherwise implemented by software.
 *          GCM Decrypt: implemented by hardware when keybits=128/256, otherwise implemented by software.
 *          CMAC: implemented by hardware when keybits=128/256, otherwise implemented by software.
 *          LE CCM: implemented by hardware.
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_AES_H
#define __DRV_AES_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_AES)
#include <stdint.h>
#include <string.h>
#include "om_driver.h"

#if (!RTE_AES_KEEP_RAW_ENGINE)

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
#define OM_AES0                         0x1000

#define AES_BLOCK_SIZE                  16


/*******************************************************************************
 * TYPEDEFS
 */
typedef enum {
    DRV_AES_MODE_ECB        = 0,
    DRV_AES_MODE_CBC        = 1,
    DRV_AES_MODE_CFB        = 2,
    DRV_AES_MODE_OFB        = 3,
    DRV_AES_MODE_CTR        = 4,
    DRV_AES_MODE_GCM        = 5,
} drv_aes_mode_t;

typedef enum {
    AES_OP_DECRYPT      = 0,
    AES_OP_ENCRYPT      = 1,
} aes_operation_t;

typedef enum {
    GCM_OP_DECRYPT     = 0,
    GCM_OP_ENCRYPT     = 1,
} gcm_operation_t;

typedef enum {
    AES_KEYBITS_128     = 0,
    AES_KEYBITS_192     = 1,
    AES_KEYBITS_256     = 2,
} aes_keybits_t;

typedef struct {
    aes_operation_t     operation;
    aes_keybits_t       keybits;
    uint8_t             key[32];
} aes_ecb_config_t;

typedef struct {
    aes_operation_t     operation;
    aes_keybits_t       keybits;
    uint8_t             key[32];
    uint8_t             iv[16];
} aes_cbc_config_t;

typedef struct {
    aes_operation_t     operation;
    aes_keybits_t       keybits;
    uint8_t             key[32];
    uint32_t            iv_offset;
    uint8_t             iv[16];
} aes_cfb_config_t;

typedef struct {
    aes_keybits_t       keybits;
    uint8_t             key[32];
    uint32_t            iv_offset;
    uint8_t             iv[16];
} aes_ofb_config_t;

typedef struct {
    aes_keybits_t       keybits;
    uint8_t             key[32];
    uint32_t            nc_offset;
    uint8_t             nonce_counter[16];
} aes_ctr_config_t;

typedef struct {
    gcm_operation_t     gcm_operation;
    aes_keybits_t       keybits;
    uint8_t             key[32];
    uint8_t             iv[16];
    uint8_t             iv_len;
    uint8_t             *addition;
    uint32_t            addition_len;
} aes_gcm_config_t;

typedef struct {
    uint8_t key[16];
    uint8_t nonce[13];
    uint8_t aad;
} aes_ccm_le_config_t;

typedef struct {
    aes_keybits_t       keybits;
    uint8_t             key[32];
} aes_cmac_config_t;
#endif /* RTE_AES_KEEP_RAW_ENGINE */


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Set hardware AES encrypt key
 *
 * @param[in] om_aes    Pointer to HW AES
 * @param[in] key       Pointer to aes key
 * @param[in] keybits   Bits of key
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_hw_setkey_enc(OM_AES_HW_Type *om_aes, const uint8_t *key, uint32_t keybits);

/**
 *******************************************************************************
 * @brief Hardware AES encrypt engine
 *
 * @param[in] om_aes    Pointer to HW AES
 * @param[in] input     Pointer to input data
 * @param[out] output   Pointer to output data
 *******************************************************************************
 */
extern void drv_aes_hw_encrypt(OM_AES_HW_Type *om_aes, const uint8_t input[16], uint8_t output[16]);

#if (!RTE_AES_KEEP_RAW_ENGINE)
/**
 *******************************************************************************
 * @brief Set AES encrypt key
 *
 * @param[in] om_aes    AES id
 * @param[in] key       Pointer to aes key
 * @param[in] keybits   Bits of key
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_setkey_enc(uint32_t om_aes, const uint8_t *key, uint32_t keybits);

/**
 *******************************************************************************
 * @brief Set AES decrypt key
 *
 * @param[in] om_aes    AES id
 * @param[in] key       Pointer to aes key
 * @param[in] keybits   Bits of key
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_setkey_dec(uint32_t om_aes, const uint8_t *key, uint32_t keybits);

/**
 *******************************************************************************
 * @brief AES encrypt engine
 *
 * @param[in] om_aes    aes id
 * @param[in] input     Pointer to input data
 * @param[out] output   Pointer to output data
 *******************************************************************************
 */
extern void drv_aes_encrypt(uint32_t om_aes, const uint8_t input[16], uint8_t output[16]);

/**
 *******************************************************************************
 * @brief AES decrypt engine
 *
 * @param[in] om_aes    aes id
 * @param[in] input     Pointer to input data
 * @param[out] output   Pointer to output data
 *******************************************************************************
 */
extern void drv_aes_decrypt(uint32_t om_aes, const uint8_t input[16], uint8_t output[16]);

/**
 *******************************************************************************
 * @brief Start AES using ECB mode
 *
 * @param[in] om_aes        AES ID
 * @param[in] cfg           AES ECB configuration
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_ecb_start(uint32_t om_aes, const aes_ecb_config_t *cfg);

/**
 *******************************************************************************
 * @brief Continue AES calculation using ECB mode
 *
 * @param[in] om_aes    AES ID
 * @param[in] input     Pointer to input data
 * @param[out] output   Pointer to output data
 * @param[in] length    Length of input data
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_ecb_crypt_continue(uint32_t om_aes, const uint8_t *input, uint8_t *output, uint32_t length);

/**
 *******************************************************************************
 * @brief Stop AES calculation using ECB mode
 *
 * @param[in] om_aes    AES ID
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_ecb_crypt_stop(uint32_t om_aes);

/**
 *******************************************************************************
 * @brief Start AES using CBC mode
 *
 * @param[in] om_aes        AES ID
 * @param[in] cfg           AES CBC configuration
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_cbc_start(uint32_t om_aes, const aes_cbc_config_t *cfg);

/**
 *******************************************************************************
 * @brief Continue AES calculation using CBC mode
 *
 * @param[in] om_aes    AES ID
 * @param[in] input     Pointer to input data
 * @param[out] output   Pointer to output data
 * @param[in] length    Length of input data
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_cbc_crypt_continue(uint32_t om_aes, const uint8_t *input, uint8_t *output, uint32_t length);

/**
 *******************************************************************************
 * @brief Stop AES calculation using CBC mode
 *
 * @param[in] om_aes    AES ID
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_cbc_crypt_stop(uint32_t om_aes);

/**
 *******************************************************************************
 * @brief Start AES using CFB mode
 *
 * @param[in] om_aes        AES ID
 * @param[in] cfg           AES CFB configuration
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_cfb_start(uint32_t om_aes, const aes_cfb_config_t *cfg);

/**
 *******************************************************************************
 * @brief Continue AES calculation using CFB mode
 *
 * @param[in] om_aes    AES ID
 * @param[in] input     Pointer to input data
 * @param[out] output   Pointer to output data
 * @param[in] length    Length of input data
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_cfb_crypt_continue(uint32_t om_aes, const uint8_t *input, uint8_t *output, uint32_t length);

/**
 *******************************************************************************
 * @brief Stop AES calculation using CFB mode
 *
 * @param[in] om_aes    AES ID
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_cfb_crypt_stop(uint32_t om_aes);

/**
 *******************************************************************************
 * @brief Start AES using OFB mode
 *
 * @param[in] om_aes        AES ID
 * @param[in] cfg           AES OFB configuration
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_ofb_start(uint32_t om_aes, const aes_ofb_config_t *cfg);

/**
 *******************************************************************************
 * @brief Continue AES calculation using OFB mode
 *
 * @param[in] om_aes    AES ID
 * @param[in] input     Pointer to input data
 * @param[out] output   Pointer to output data
 * @param[in] length    Length of input data
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_ofb_crypt_continue(uint32_t om_aes, const uint8_t *input, uint8_t *output, uint32_t length);

/**
 *******************************************************************************
 * @brief Stop AES calculation using OFB mode
 *
 * @param[in] om_aes    AES ID
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_ofb_crypt_stop(uint32_t om_aes);

/**
 *******************************************************************************
 * @brief Start AES using CTR mode
 *
 * @param[in] om_aes        AES ID
 * @param[in] cfg           AES CTR configuration
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_ctr_start(uint32_t om_aes, const aes_ctr_config_t *cfg);

/**
 *******************************************************************************
 * @brief Continue AES calculation using CTR mode
 *
 * @param[in] om_aes    AES ID
 * @param[in] input     Pointer to input data
 * @param[out] output   Pointer to output data
 * @param[in] length    Length of input data
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_ctr_crypt_continue(uint32_t om_aes, const uint8_t *input, uint8_t *output, uint32_t length);

/**
 *******************************************************************************
 * @brief Stop AES calculation using CTR mode
 *
 * @param[in] om_aes    AES ID
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_ctr_crypt_stop(uint32_t om_aes);

/**
 *******************************************************************************
 * @brief Start AES calculation using GCM mode, NOTE: this is BIG-ENDIAN
 *
 * @param[in] om_aes    AES ID
 * @param[in] cfg       GCM configuration
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_gcm_start(uint32_t om_aes, const aes_gcm_config_t *cfg);

/**
 *******************************************************************************
 * @brief Continue AES calculation using GCM mode, NOTE: this is BIG-ENDIAN
 *
 * @param[in] om_aes    AES ID
 * @param[in] input     Pointer to input data
 * @param[out] output   Pointer to output data
 * @param[in] length    Length of input data
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_gcm_crypt_continue(uint32_t om_aes, const uint8_t *input, uint8_t *output, uint32_t length);

/**
 *******************************************************************************
 * @brief Stop AES calculation using GCM mode, NOTE: this is BIG-ENDIAN
 *
 * @param[in] om_aes    AES ID
 * @param[out] tag      Pointer to tag(hash)
 * @param[in] tag_len   Length of tag
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_gcm_crypt_stop(uint32_t om_aes, uint8_t *tag, uint8_t tag_len);

/**
 *******************************************************************************
 * @brief Start AES-CCM LE encrypt
 *
 * @param[in]  cfg                      CCM configuration
 * @param[in]  plain_text               Pointer to plain text
 * @param[out] cipher_text_and_tag      Pointer to cipher text and tag
 * @param[in]  text_len                 Length of plain text
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_ccm_le_encrypt(aes_ccm_le_config_t *cfg, uint8_t *plain_text, uint8_t *cipher_text_and_tag, uint32_t text_len);

/**
 *******************************************************************************
 * @brief Start AES-CCM LE decrypt
 * NOTE: Cannot identify MIC error
 *
 * @param[in] cfg                       CCM configuration
 * @param[in] cipher_text_and_tag       Pointer to cipher text and tag
 * @param[out] plain_text               Pointer to plain text
 * @param[in] text_len                  Length of plain text
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_ccm_le_decrypt(aes_ccm_le_config_t *cfg, uint8_t *cipher_text_and_tag, uint8_t *plain_text, uint32_t text_len);

/**
 *******************************************************************************
 * @brief Start cmac calculation
 *
 * @param[in] om_aes    AES ID
 * @param[in] cfg      Pointer to cmac configuration
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_cmac_start(uint32_t om_aes, aes_cmac_config_t *cfg);

/**
 *******************************************************************************
 * @brief Continue cmac calculation
 *
 * @param[in] om_aes    AES ID
 * @param[in] input     Pointer to input data
 * @param[in] len       Length of input data
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_cmac_crypt_continue(uint32_t om_aes, const uint8_t *input, uint32_t len);

/**
 *******************************************************************************
 * @brief Stop cmac calculation
 *
 * @param[in] om_aes    AES ID
 * @param[out] mac       Pointer to mac
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_aes_cmac_crypt_stop(uint32_t om_aes, uint8_t mac[AES_BLOCK_SIZE]);

#ifdef __cplusplus
}
#endif

#endif  /* (RTE_AES_KEEP_RAW_ENGINE)*/

#endif  /* (RTE_AES) */

#endif  /* __DRV_AES_H */


/** @} */
