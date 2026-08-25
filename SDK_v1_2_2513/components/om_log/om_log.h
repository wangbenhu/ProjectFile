/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup LOG LOG
 * @ingroup  COMPONENTS
 * @brief
 * @details
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */
#ifndef __OM_LOG_H
#define __OM_LOG_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include "om_error.h"
#include "om_log_config.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * TYPEDEFS
 */
typedef enum {
    OM_LOG_ASSERT     = 0U,
    OM_LOG_ERROR      = 1U,
    OM_LOG_WARN       = 2U,
    OM_LOG_INFO       = 3U,
    OM_LOG_DEBUG      = 4U,
    OM_LOG_LEVEL_MAX,
} om_log_lvl_t;


/*******************************************************************************
 * MACROS
 */
#if CONFIG_OM_LOG
#define OM_LOG_INIT()                           om_log_init()
#define OM_LOG(level, ...)                      om_log(level, __VA_ARGS__)
#define OM_LOG_HEXDUMP(level, buf, size, width) om_log_hexdump(level, buf, size, width)
#define OM_LOG_FLUSH()                          om_log_flush()
#else
#define OM_LOG_INIT()
#define OM_LOG(level, ...)
#define OM_LOG_HEXDUMP(level, buf, size, width)
#define OM_LOG_FLUSH()
#endif


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
#if (CONFIG_OM_LOG)
/**
 *******************************************************************************
 * @brief log initialize
 *
 * @return: None
 *******************************************************************************
 */
extern void om_log_init(void);

/**
 *******************************************************************************
 * @brief buffer dump by hex
 *
 * @param level         log level
 * @param buf           buffer pointer
 * @param size          buffer size
 * @param width         dump width
 *
 * @return: None
 *******************************************************************************
 */
extern void om_log_hexdump(om_log_lvl_t level, uint8_t* buf, uint32_t size, uint32_t width);

/**
 *******************************************************************************
 * @brief log flush
 *
 * @return: None
 *******************************************************************************
 */
extern void om_log_flush(void);

/**
 *******************************************************************************
 * @brief restore log from sleep
 *
 *******************************************************************************
 */
extern void om_log_restore(void);

#endif /* (CONFIG_OM_LOG) */

/**
 *******************************************************************************
 * @brief log output
 *
 * @param level         log level
 * @param format        long format string
 *******************************************************************************
 */
extern void om_log(om_log_lvl_t level, const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif /* __OM_LOG_H */

/** @} */
