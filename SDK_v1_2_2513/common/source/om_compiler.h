/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup COMPILER COMPILER
 * @ingroup  COMMON
 * @brief    defines about compilers
 * @details  defines about compilers
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


#ifndef __OM_COMPILER_H
#define __OM_COMPILER_H


#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
#if defined(__ARMCC_VERSION)    /* ARMCLANG compiler */
#define __RAM_CODE              __attribute((section("RAM_CODE"), noinline))
#define __RAM_CODES(s)          __attribute((section("RAM_CODE."s), noinline))
#define __RAM_RODATA            __attribute((section("RAM_RODATA")))
#define __RAM_RODATAS(s)        __attribute((section("RAM_RODATA."s)))
#define __RAM_DATA_NON_INIT     __attribute((section(".bss.RAM_DATA_NON_INIT")))
#ifndef CC_DEPRECATED
#define CC_DEPRECATED           __attribute__((deprecated))
#endif
#elif defined (__GNUC__)          /* GCC compiler */
#define __RAM_CODE              __attribute((section(".ram_code"), noinline))
#define __RAM_CODES(s)          __attribute((section(".ram_code."s), noinline))
#define __RAM_RODATA            __attribute((section(".ram_rodata")))
#define __RAM_RODATAS(s)        __attribute(section(".ram_rodata."s))
#define __RAM_DATA_NON_INIT     __attribute((section(".ram_data_non_init")))

#ifndef CC_DEPRECATED
#define CC_DEPRECATED           __attribute__((deprecated))
#endif
#elif defined (__ICCARM__)      /* IAR compiler */
#define __RAM_CODE              /*_Pragma("location=\".ram_code\"")*/ __ramfunc
#define __RAM_CODES(s)          /*_Pragma("location=\".ram_code\"")*/ __ramfunc
#define __RAM_RODATA            _Pragma("location=\".ram_rodata\"")
#define __RAM_DATA_NON_INIT     __no_init
#ifndef CC_DEPRECATED
#define CC_DEPRECATED
#endif
#else
#error Donot Support Compiler!
#endif

#define __NOINLINE             __attribute__((noinline))

#ifdef __cplusplus
}
#endif

#endif  /*  __OM_COMPILER_H */


/** @} */
