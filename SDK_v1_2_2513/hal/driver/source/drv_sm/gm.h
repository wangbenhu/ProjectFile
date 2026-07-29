/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup GM GM
 * @ingroup  DRIVER
 * @brief    Basic functions for GM
 * @details  Basic functions for GM
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __GM_H
#define __GM_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "om_device.h"


#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
// 字节转化为字
#define GM_GETU32(p) \
    ((uint32_t)(p)[0] << 24 | \
     (uint32_t)(p)[1] << 16 | \
     (uint32_t)(p)[2] <<  8 | \
     (uint32_t)(p)[3])

#define GM_GETU64(p) \
    ((uint64_t)(p)[0] << 56 | \
     (uint64_t)(p)[1] << 48 | \
     (uint64_t)(p)[2] << 40 | \
     (uint64_t)(p)[3] << 32 | \
     (uint64_t)(p)[4] << 24 | \
     (uint64_t)(p)[5] << 16 | \
     (uint64_t)(p)[6] <<  8 | \
     (uint64_t)(p)[7])

// 字转化为字节
#define GM_PUTU32(p,V) \
    ((p)[0] = (uint8_t)((V) >> 24), \
     (p)[1] = (uint8_t)((V) >> 16), \
     (p)[2] = (uint8_t)((V) >>  8), \
     (p)[3] = (uint8_t)(V))

#define GM_PUT_UINT32_BE(n, b ,i)                       \
{                                                       \
    (b)[(i)    ] = (unsigned char) ( (n) >> 24 );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 3] = (unsigned char) ( (n)       );       \
}

#define GM_PUTU64(p,V) \
    ((p)[0] = (uint8_t)((V) >> 56), \
     (p)[1] = (uint8_t)((V) >> 48), \
     (p)[2] = (uint8_t)((V) >> 40), \
     (p)[3] = (uint8_t)((V) >> 32), \
     (p)[4] = (uint8_t)((V) >> 24), \
     (p)[5] = (uint8_t)((V) >> 16), \
     (p)[6] = (uint8_t)((V) >>  8), \
     (p)[7] = (uint8_t)(V))


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
int gm_hex2bin(const char * in, int in_len, uint8_t * out);

void gm_memxor(void *r, const void *a, const void *b, uint32_t len);

void randombytes(uint8_t *dest, unsigned size);


#ifdef __cplusplus
}
#endif

#endif  /* __GM_H */


/** @} */