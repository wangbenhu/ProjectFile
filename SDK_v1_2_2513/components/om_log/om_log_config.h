/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup LOG_CONFIG LOG_CONFIG
 * @ingroup  LOG
 * @brief
 * @details
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __OM_LOG_CONFIG_H
#define __OM_LOG_CONFIG_H


/*******************************************************************************
 * INCLUDES
 */
#include "autoconf.h"

#ifdef __cplusplus
extern "C"
{
#endif

#if CONFIG_OM_LOG


/*******************************************************************************
 * MACROS
 */
#if CONFIG_OM_LOG_TO_UART

#ifndef CONFIG_OM_LOG_UART_IDX
#define CONFIG_OM_LOG_UART_IDX          1
#endif

#ifndef CONFIG_OM_LOG_UART_BAUDRATE
#define CONFIG_OM_LOG_UART_BAUDRATE     115200
#endif

#elif CONFIG_OM_LOG_TO_FLASH

#if !(CONFIG_OM_LOG_FLASH_INTERNAL || CONFIG_OM_LOG_FLASH_EXTERNAL)
#define CONFIG_OM_LOG_FLASH_INTERNAL    1
#endif

#ifndef CONFIG_OM_LOG_FLASH_ADDR
#define CONFIG_OM_LOG_FLASH_ADDR        0xFF000
#endif

#ifndef CONFIG_OM_LOG_SAVE_SIZE
#define CONFIG_OM_LOG_SAVE_SIZE         1024
#endif

#endif /* CONFIG_OM_LOG_TO_UART or CONFIG_OM_LOG_TO_FLASH */

#ifndef CONFIG_OM_LOG_TIME
#define CONFIG_OM_LOG_TIME              0
#endif

#ifndef CONFIG_OM_LOG_FILTER_LEVEL
#define CONFIG_OM_LOG_FILTER_LEVEL      OM_LOG_LEVEL_MAX
#endif

#ifndef CONFIG_OM_LOG_LINE_MAX
#define CONFIG_OM_LOG_LINE_MAX          256
#endif

#endif /* CONFIG_OM_LOG */

#ifdef __cplusplus
}
#endif

#endif /* __OM_LOG_CONFIG_H */

/** @} */
