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
 * @brief    Generate MBR
 * @details  Generate MBR
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct __PACKED {
    uint32_t    magic_num;
    uint32_t    len;
    uint32_t    img_ver;
    uint32_t    img_vma;
    uint32_t    img_lma;
    uint32_t    img_len;
    uint8_t     img_hash[32];
    uint8_t     public_key[64];
    uint8_t     dig_sign[64];
    uint32_t    crc32;
} img_hdr_data_t;

/**
 *******************************************************************************
 * @brief Generate MBR
 *
 * @param[in] img_vma   Application execution address
 * @param[in] img_lma   Application backup address
 * @param[in] img_len   Application length
 * @param[in] img_hash  Application hash
 * @param[out] mbr      Generated MBR(512 bytes)
 *******************************************************************************
 */
void gen_mbr(uint32_t img_vma, uint32_t img_lma, uint32_t img_len, uint8_t img_hash[32], uint8_t mbr[512])
{
    memset(mbr, 0xFF, 512);

    mbr[0] = 0x43; mbr[1] = 0x46; mbr[2] = 0x47; mbr[3] = 0x00; mbr[4] = 0x04;
    mbr[5] = 0x02; mbr[6] = 0x02; mbr[7] = 0x00; mbr[8] = 0x00; mbr[9] = 0x00;
    mbr[10] = 0xAE; mbr[11] = 0x43; mbr[12] = 0x70; mbr[13] = 0xAC;

    img_hdr_data_t *hdr = (img_hdr_data_t *)(mbr + 256);
    hdr->magic_num = 0x0052424d;
    hdr->len       = 0xB4;
    hdr->img_ver   = 0x00;
    hdr->img_vma   = img_vma;
    hdr->img_lma   = img_lma;
    hdr->img_len   = img_len;
    memcpy(hdr->img_hash, img_hash, 32);

    hdr->crc32 = 0xFFFFFFFF;
    // Calculate CRC32
    uint32_t val;
    uint32_t tmp_crc;
    for (uint16_t i = 0; i < sizeof(img_hdr_data_t) - sizeof(uint32_t); i++) {
        val = (uint32_t)(((uint8_t *)hdr)[i] ^ (hdr->crc32 & 0xFF));
        for (uint8_t i = 0; i < 8; i++) {
            if (val & 1) {
                val = 0xEDB88320L ^ (val >> 1);
            } else {
                val = val >> 1;
            }
        }
        hdr->crc32 = val ^ (hdr->crc32 >> 8);
    }
}