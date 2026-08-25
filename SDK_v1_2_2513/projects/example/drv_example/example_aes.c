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
 * @brief    example for using aes
 * @details  example for using aes:
 * @version
 *
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


/*******************************************************************************
 * INCLUDES
 */
#include "om_driver.h"


/*******************************************************************************
 * MACROS
 */


/*******************************************************************************
 * TYPEDEFS
 */


/*******************************************************************************
 * CONST & VARIABLES
 */
static const unsigned char aes_test_ecb_dec[][16] = {
    { 0x44, 0x41, 0x6A, 0xC2, 0xD1, 0xF5, 0x3C, 0x58,
      0x33, 0x03, 0x91, 0x7E, 0x6B, 0xE9, 0xEB, 0xE0 },
    { 0x48, 0xE3, 0x1E, 0x9E, 0x25, 0x67, 0x18, 0xF2,
      0x92, 0x29, 0x31, 0x9C, 0x19, 0xF1, 0x5B, 0xA4 },
    { 0x05, 0x8C, 0xCF, 0xFD, 0xBB, 0xCB, 0x38, 0x2D,
      0x1F, 0x6F, 0x56, 0x58, 0x5D, 0x8A, 0x4A, 0xDE }
};

static const unsigned char aes_test_ecb_enc[][16] = {
    { 0xC3, 0x4C, 0x05, 0x2C, 0xC0, 0xDA, 0x8D, 0x73,
      0x45, 0x1A, 0xFE, 0x5F, 0x03, 0xBE, 0x29, 0x7F },
    { 0xF3, 0xF6, 0x75, 0x2A, 0xE8, 0xD7, 0x83, 0x11,
      0x38, 0xF0, 0x41, 0x56, 0x06, 0x31, 0xB1, 0x14 },
    { 0x8B, 0x79, 0xEE, 0xCC, 0x93, 0xA0, 0xEE, 0x5D,
      0xFF, 0x30, 0xB4, 0xEA, 0x21, 0x63, 0x6D, 0xA4 }
};

/*******************************************************************************
 * LOCAL FUNCTIONS
 */

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief example of using aes
 *
 *******************************************************************************
 */
void example_aes(void)
{
    int i, j, u, mode;
    unsigned char key[32];
    unsigned char buf[64];
    const unsigned char *aes_result;
    uint8_t num_tests;
    aes_ecb_config_t ecb_cfg;

    num_tests = sizeof(aes_test_ecb_enc) / sizeof(*aes_test_ecb_enc);
    memset(key, 0, 32);

    for (i = 0; i < num_tests << 1; i++) {
        u = i >> 1;
        mode = i & 1;

        memset(buf, 0, 16);

        aes_result = (mode == AES_OP_DECRYPT) ? aes_test_ecb_dec[u] : aes_test_ecb_enc[u];

        memcpy(ecb_cfg.key, key, sizeof(key));
        ecb_cfg.keybits = (aes_keybits_t)u;
        ecb_cfg.operation = (aes_operation_t)mode;

        drv_aes_ecb_start(OM_AES0, &ecb_cfg);
        for (j = 0; j < 10000; j++) {
            drv_aes_ecb_crypt_continue(OM_AES0, buf, buf, 16);
        }
        drv_aes_ecb_crypt_stop(OM_AES0);

        for (j = 0; j < 16; j++) {
            if (aes_result[j] != buf[j]) {
                om_printf("AES ECB mode test failed");
                break;
            }
        }
    }
}



/** @} */