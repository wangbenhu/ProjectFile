/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup LOG LOG
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


/*******************************************************************************
 * INCLUDES
 */
#include <stdarg.h>
#include <string.h>
#include "om_log.h"
#include "om_common.h"
#include "om_log_port.h"
#if (CONFIG_PM)
#include "pm.h"
#endif

#if (CONFIG_OM_LOG)


/*******************************************************************************
 * MACRO
 */
#define NEWLINE_SIGN    "\r\n"
#define _is_ascii(ch)   ((ch) <= 0x7E && (ch) >= 0x20)


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static uint32_t log_strcpy(uint32_t cur_len, char* dst, const char* src)
{
    const char* src_old = src;
    uint32_t src_len = 0;

    if (dst == NULL || src == NULL) {
        return 0;
    }
    src_len = strlen(src);
    while (src_len && ((((uint32_t)src) & (sizeof(uint32_t) - 1)) ||
                       (((uint32_t)dst) & (sizeof(uint32_t) - 1)))) {
        if (cur_len++ < CONFIG_OM_LOG_LINE_MAX) {
            *dst++ = *src++;
            src_len--;
        } else {
            break;
        }
    }
    while (src_len >= sizeof(uint32_t)) {
        if (cur_len < CONFIG_OM_LOG_LINE_MAX) {
            *(uint32_t*)dst = *(uint32_t*)src;
            dst += sizeof(uint32_t);
            src += sizeof(uint32_t);
            cur_len += sizeof(uint32_t);
            src_len -= sizeof(uint32_t);
        } else {
            break;
        }
    }
    while (src_len) {
        if (cur_len++ < CONFIG_OM_LOG_LINE_MAX) {
            *dst++ = *src++;
            src_len--;
        } else {
            break;
        }
    }
    return src - src_old;
}

static void log_format_vout(om_log_lvl_t level, uint32_t format_len, const char* format, va_list va)
{
    #if (CONFIG_OM_LOG_TO_SHELL)
    (void)level;
    (void)format_len;
    log_shell_out(format, va);
    #elif (CONFIG_OM_LOG_TO_RTT)
    (void)format_len;
    log_rtt_out(level, format, va);
    #else
    (void)level;
    (void)va;
    log_out((uint8_t *)format, format_len);
    #endif
}

static void log_format_out(om_log_lvl_t level, uint32_t format_len, const char* format, ...)
{
    va_list va;

    va_start(va, format);
    // len is 0, indicate that len is not used
    log_format_vout(level, format_len, format, va);
    va_end(va);
}

#if (CONFIG_PM)
static void om_log_restore_callback(pm_sleep_state_t sleep_state, pm_status_t power_status)
{
    if (sleep_state == PM_SLEEP_RESTORE_HSI) {
        om_log_restore();
    }
}
#endif

__WEAK void log_restore()
{
    return;
}


/*******************************************************************************
 * PUBLIC FUNCTION
 */

void om_log_init(void)
{
    log_out_init();
    #if (CONFIG_PM)
    pm_sleep_store_restore_callback_register(om_log_restore_callback);
    #endif
}

void om_log(om_log_lvl_t level, const char* format, ...)
{
    uint8_t line_buf[CONFIG_OM_LOG_LINE_MAX];
    uint32_t log_len = 0;
    va_list va;

    if (format == NULL || level > CONFIG_OM_LOG_FILTER_LEVEL) {
        return;
    }
    // set string line end
    line_buf[CONFIG_OM_LOG_LINE_MAX - 1] = '\0';

    va_start(va, format);

    #if CONFIG_OM_LOG_TIME
    // add format time
    om_snprintf((char *)(line_buf + log_len), CONFIG_OM_LOG_LINE_MAX - log_len, "[%06d] ", log_get_time());
    log_len += strlen((char *)(line_buf + log_len));
    #endif

    #if (CONFIG_OM_LOG_TO_SHELL || CONFIG_OM_LOG_TO_RTT)
    // add raw string to buffer for shell or rtt
    log_len += log_strcpy(log_len, (char *)(line_buf + log_len), format);
    if (log_len < CONFIG_OM_LOG_LINE_MAX) {
        line_buf[log_len++] = '\0';
    }
    #else
    // add format string to buffer for others(e.g. uart/flash)
    int res = om_vsnprintf((char *)(line_buf + log_len), CONFIG_OM_LOG_LINE_MAX - log_len, format, va);
    if ((log_len + res <= CONFIG_OM_LOG_LINE_MAX) && (res > -1)) {
        log_len += res;
    } else {
        log_len = CONFIG_OM_LOG_LINE_MAX;
    }
    #endif
    // do log output
    log_format_vout(level, log_len, (const char *)line_buf, va);
    va_end(va);
}

void om_log_hexdump(om_log_lvl_t level, uint8_t* buf, uint32_t size, uint32_t width)
{
    uint32_t i, j;
    uint8_t line_buf[CONFIG_OM_LOG_LINE_MAX + 1];  // line string + '\0
    uint32_t log_len;
    char dump_string[8];
    int32_t fmt_result;
    uint8_t sign_len;

    if (level > CONFIG_OM_LOG_FILTER_LEVEL) {
        return;
    }

    for (i = 0, log_len = 0; i < size; i += width) {
        // package header
        if (i == 0) {
            log_len += log_strcpy(log_len, (char *)(line_buf + log_len), "D/HEX ");
            log_len += log_strcpy(log_len, (char *)(line_buf + log_len), ": ");
        } else {
            log_len = 6 + 2;
            memset(line_buf, ' ', log_len);
        }
        fmt_result = om_snprintf((char *)(line_buf + log_len),
                            CONFIG_OM_LOG_LINE_MAX, "%04X-%04X: ", i, i + width - 1);
        // calculate log length
        if ((fmt_result > -1) && (fmt_result <= CONFIG_OM_LOG_LINE_MAX)) {
            log_len += fmt_result;
        } else {
            log_len = CONFIG_OM_LOG_LINE_MAX;
        }
        // dump hex
        for (j = 0; j < width; j++) {
            if (i + j < size) {
                om_snprintf(dump_string, sizeof(dump_string), "%02X ", buf[i + j]);
            } else {
                strncpy(dump_string, "   ", sizeof(dump_string));
            }
            log_len += log_strcpy(log_len, (char *)(line_buf + log_len), dump_string);
            if ((j + 1) % 8 == 0) {
                log_len += log_strcpy(log_len, (char *)(line_buf + log_len), " ");
            }
        }
        log_len += log_strcpy(log_len, (char *)(line_buf + log_len), "  ");
        // dump char for hex
        for (j = 0; j < width; j++) {
            if (i + j < size) {
                om_snprintf(dump_string, sizeof(dump_string), "%c", _is_ascii(buf[i + j]) ? buf[i + j] : '.');
                log_len += log_strcpy(log_len, (char *)(line_buf + log_len), dump_string);
            }
        }
        // newline
        sign_len = strlen(NEWLINE_SIGN);
        if (sign_len) {
            // overflow check and reserve some space for newline sign
            if (log_len + sign_len > CONFIG_OM_LOG_LINE_MAX) {
                log_len = CONFIG_OM_LOG_LINE_MAX - sign_len;
            }
            // package newline sign
            log_len += log_strcpy(log_len, (char *)(line_buf + log_len), NEWLINE_SIGN);
        }
        // if use format, added line end character, otherwise, use raw string and '\0' is ignored
        line_buf[log_len] = '\0';
        // do log output
        log_format_out(level, log_len, (const char *)line_buf);
    }
}

void om_log_flush(void)
{
    log_out_flush();
}

void om_log_restore(void)
{
    log_restore();
}

#else /* CONFIG_OM_LOG */

void om_log(om_log_lvl_t level, const char* format, ...)
{
    (void)level;
    (void)format;
}

#endif /* CONFIG_OM_LOG */

/** @} */
