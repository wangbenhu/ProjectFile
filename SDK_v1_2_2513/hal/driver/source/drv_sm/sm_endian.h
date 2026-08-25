/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup ENDIAN ENDIAN
 * @ingroup  DRIVER
 * @brief    endian processing
 * @details  endian processing
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __SM_ENDIAN_H
#define __SM_ENDIAN_H


/*******************************************************************************
 * INCLUDES
 */
#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
/* Big Endian R/W */
#define GETU16(p)                                   \
	((uint16_t)(p)[0] <<  8 |                       \
	 (uint16_t)(p)[1])

#define GETU32(p)                                   \
	((uint32_t)(p)[0] << 24 |                       \
	 (uint32_t)(p)[1] << 16 |                       \
	 (uint32_t)(p)[2] <<  8 |                       \
	 (uint32_t)(p)[3])

#define GETU64(p)                                   \
	((uint64_t)(p)[0] << 56 |                       \
	 (uint64_t)(p)[1] << 48 |                       \
	 (uint64_t)(p)[2] << 40 |                       \
	 (uint64_t)(p)[3] << 32 |                       \
	 (uint64_t)(p)[4] << 24 |                       \
	 (uint64_t)(p)[5] << 16 |                       \
	 (uint64_t)(p)[6] <<  8 |                       \
	 (uint64_t)(p)[7])


// WARNING: must not write PUTU32(buf, val++)
#define PUTU16(p,V)                                 \
	((p)[0] = (uint8_t)((V) >> 8),                  \
	 (p)[1] = (uint8_t)(V))

#define PUTU32(p,V)                                 \
	((p)[0] = (uint8_t)((V) >> 24),                 \
	 (p)[1] = (uint8_t)((V) >> 16),                 \
	 (p)[2] = (uint8_t)((V) >>  8),                 \
	 (p)[3] = (uint8_t)(V))

#define PUTU64(p,V)                                 \
	((p)[0] = (uint8_t)((V) >> 56),                 \
	 (p)[1] = (uint8_t)((V) >> 48),                 \
	 (p)[2] = (uint8_t)((V) >> 40),                 \
	 (p)[3] = (uint8_t)((V) >> 32),                 \
	 (p)[4] = (uint8_t)((V) >> 24),                 \
	 (p)[5] = (uint8_t)((V) >> 16),                 \
	 (p)[6] = (uint8_t)((V) >>  8),                 \
	 (p)[7] = (uint8_t)(V))

/* Little Endian R/W */
#define GETU16_LE(p)	(*(const uint16_t *)(p))
#define GETU32_LE(p)	(*(const uint32_t *)(p))
#define GETU64_LE(p)	(*(const uint64_t *)(p))

#define PUTU16_LE(p,V)	*(uint16_t *)(p) = (V)
#define PUTU32_LE(p,V)	*(uint32_t *)(p) = (V)
#define PUTU64_LE(p,V)	*(uint64_t *)(p) = (V)

/* Rotate */
#define ROL32(a,n)      (((a)<<(n))|(((a)&0xffffffff)>>(32-(n))))
#define ROL64(a,n)	    (((a)<<(n))|((a)>>(64-(n))))

#define ROR32(a,n)	    ROL32((a),32-(n))
#define ROR64(a,n)	    ROL64(a,64-n)

#ifdef __cplusplus
}
#endif

#endif  /* __SM_ENDIAN_H */


/** @} */