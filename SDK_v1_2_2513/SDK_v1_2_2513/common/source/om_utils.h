/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup UTILS UTILS
 * @ingroup  COMMON
 * @brief    COMMON Typedefs
 * @details  COMMON Typedefs
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


#ifndef __OM_UTILS_H
#define __OM_UTILS_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include <string.h>
#include "cmsis_compiler.h"
#include "autoconf.h"


#ifdef  __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
/**
 *******************************************************************************
 * @brief  The OM_ASSERT macro is used for function's parameters check.
 * @param  expr: If expr is false, it calls om_assert_failed function
 *         which reports the name of the source file and the source
 *         line number of the call that failed.
 *         If expr is true, it returns no value.
 * @return None
 *******************************************************************************
 */
#if (CONFIG_OM_ASSERT)
#define OM_ASSERT(expr)                                                        \
    do {                                                                       \
        if (!(expr)) {                                                         \
            __BKPT(0); __builtin_unreachable();                                \
        }                                                                      \
    } while (0)

/// assert with while
#define OM_ASSERT_WHILE(cond, expr)                                            \
    do {                                                                       \
        if ((unsigned)(cond)) {                                                \
            if (!(expr)) {                                                     \
                __BKPT(0); __builtin_unreachable();                            \
            }                                                                  \
        }                                                                      \
    } while (0)
#else
#define OM_ASSERT(expr)                 ((void)0U)
#define OM_ASSERT_WHILE(cond, expr)     ((void)0U)
#endif  /* (CONFIG_OM_ASSERT) */

#if defined(__GNUC__)
/* A compile-time constant with the value 0. If `const_expr` is not a
 * compile-time constant with a nonzero value, cause a compile-time error. */
#define OM_STATIC_ASSERT( const_expr )                                         \
    ( 0 && sizeof( struct { unsigned int STATIC_ASSERT : 1 - 2 * ! ( const_expr ); } ) )
#endif

/**
 *******************************************************************************
 *  @brief Align val to the nearest higher multiple of align, where align is a
 *         power of 2(eg., 2, 4, 8, ... 2**n)
 *
 *  @param val   Value to align.
 *  @param align Align, is a power of 2.
 *
 *  @return Aligned Value
 *******************************************************************************
 */
#define OM_ALIGN_CEIL(val, align)    (((unsigned)((val) + ((align) - 1))) & (~((align) - 1)))

/**
 *******************************************************************************
 *  @brief Align val to the nearest lower multiple of align, where align is a
 *         power of 2(eg., 2, 4, 8, ... 2**n)
 *
 *  @param val    Value to align.
 *  @param align  Align
 *
 *  @return Value aligned.
 *******************************************************************************
 */
#define OM_ALIGN_FLOOR(val, align)    ((unsigned)(val) & (~((align) - 1)))

/**
 *******************************************************************************
 *  @brief Check val is aligned, where align is a power of 2(eg., 2, 4, 8, ... 2**n)
 *
 *  @param val   Value to align.
 *  @param align Align value, align range in [2, 4, 8, ...2**31, 0]
 *
 *  @return is aligned.
 *******************************************************************************
 */
#define OM_IS_ALIGN(val, align)   (!((unsigned)(val) & ((unsigned)(align) - 1)))

#ifndef OM_BSWAP16
#define OM_BSWAP16(x)       __REVSH(x)
#endif /* OM_BSWAP16 */

#ifndef OM_BSWAP32
#define OM_BSWAP32(x)       __REV(x)
#endif /* OM_BSWAP32 */

/* used for specify function/macro parameter need ALIGN to 2 Bytes */
#ifdef __ALIGN2
#error "redefine __ALIGN2"
#else
#define __ALIGN2
#endif

/* used for specify function/macro parameter need ALIGN to 4 Bytes */
#ifdef __ALIGN4
#error "redefine __ALIGN4"
#else
#define __ALIGN4
#endif


#ifdef __cplusplus
}
#endif

#endif  /* __OM_UTILS_H */


/** @} */
