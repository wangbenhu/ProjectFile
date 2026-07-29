/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup BSP BSP
 * @ingroup  BSP
 * @brief    Board Supported Packages
 * @details  Board Supported Packages top level include file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __BSP_H
#define __BSP_H


/*******************************************************************************
 * INCLUDES
 */
#include "autoconf.h"
#if (CONFIG_BOARD_EVB)
#include "../board_evb/board_evb.h"
#else
#define board_init()
#endif
#if (CONFIG_GLCD_GC9C01 || CONFIG_GLCD_GC9B71)
#include "../source/glcd.h"
#endif


#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Board initialization, initialize pinmux and GPIO configuration
 *******************************************************************************
 */
#ifndef board_init
extern void board_init(void);
#endif


#ifdef __cplusplus
}
#endif

#endif  /* __BSP_H */


/** @} */
