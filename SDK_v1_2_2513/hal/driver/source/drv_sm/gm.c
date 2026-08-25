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
 * @brief    GM basic functions
 * @details  GM basic functions
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
#include "gm.h"


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static uint32_t lcg_rand(void)
{
    static uint32_t lcg_seed = 1;

    #define LCG_A       1664525U
    #define LCG_C       1013904223U
    #define LCG_MOD     4294967296U

    lcg_seed = ((uint64_t)lcg_seed * LCG_A + LCG_C) % LCG_MOD;

    return lcg_seed;
}

__WEAK void randombytes(uint8_t *dest, unsigned size)
{
    // NOTE: this should be replaced by a proper random number generator
    unsigned i;

    for (i = 0; i < size; i++) {
        dest[i] = lcg_rand() & 0xFF;
    }
}

// 十六进制转化为int
static int gm_hex2int(char c) {
    if(c >= '0' && c <= '9') {
        return c - '0';
    }else if(c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }else if(c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 * 十六进制转化为二进制
 * @param in 十六进制串
 * @param in_len 十六进制串长度，必须为偶数
 * @param out 二进制缓冲区
 */
int gm_hex2bin(const char * in, int in_len, uint8_t * out) {
    int c = 0;
    if((in_len % 2) != 0) {
        return -1;
    }

    while (in_len) {
        if ((c = gm_hex2int(*in++)) < 0) {
            return -1;
        }
        *out = (uint8_t)c << 4;

        if ((c = gm_hex2int(*in++)) < 0) {
            return -1;
        }
        *out |= (uint8_t)c;

        in_len -= 2;
        out++;
    }
    return 1;
}

void gm_memxor(void *r, const void *a, const void *b, uint32_t len)
{
    uint8_t *pr = r;
    const uint8_t *pa = a;
    const uint8_t *pb = b;
    for (uint32_t i = 0; i < len; i++) {
        pr[i] = pa[i] ^ pb[i];
    }
}


/** @} */
