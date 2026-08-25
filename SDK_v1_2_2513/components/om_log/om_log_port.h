/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup LOG_PORT LOG_PORT
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

#ifndef __OM_LOG_PORT_H
#define __OM_LOG_PORT_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include "om_common.h"
#include "om_log_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

#if CONFIG_OM_LOG


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief log backend initialization
 *
 * @return: None
 *******************************************************************************
 */
extern void log_out_init(void);

#if (CONFIG_OM_LOG_TO_SHELL)
/**
 *******************************************************************************
 * @brief shell output, it is used for printing log to shell
 *
 * @param fmt:  log format string
 * @param va:   variable argument list
 *
 * @return: None
 *******************************************************************************
 */
extern void log_shell_out(const char *fmt, va_list va);
#elif (CONFIG_OM_LOG_TO_RTT)
/**
 *******************************************************************************
 * @brief rtt output, it is used for printing log to rtt
 *
 * @param level:log level
 * @param fmt:  log format string
 * @param va:   variable argument list
 *
 * @return: None
 *******************************************************************************
 */
extern void log_rtt_out(om_log_lvl_t level, const char *fmt, va_list va);
#else
/**
 *******************************************************************************
 * @brief log output, it is used for printing log to uart/flash
 *
 * @param buf:  log buffer
 * @param len:  log buffer length
 *
 * @return: None
 *******************************************************************************
 */
extern void log_out(uint8_t *buf, uint32_t len);
#endif

/**
 *******************************************************************************
 * @brief log flush, it is used for flushing log to flash form ram
 *
 *
 * @return: None
 *******************************************************************************
 */
extern void log_out_flush(void);

/**
 *******************************************************************************
 * @brief get time for log output
 *
 *
 * @return: timer tick counter value
 *******************************************************************************
 */
#if CONFIG_OM_LOG_TIME
extern uint32_t log_get_time(void);
#endif

#endif /* CONFIG_OM_LOG */

#ifdef __cplusplus
}
#endif

#endif /* __OM_LOG_PORT_H */

/** @} */
