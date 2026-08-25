/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup TRACE TRACE
 * @ingroup  COMPONENTS
 * @brief    trace
 * @details  trace using IO
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


#ifndef __TRC_IO_H
#define __TRC_IO_H

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * INCLUDES
 */
#include "autoconf.h"
#include <stdint.h>


/*******************************************************************************
 * TYPEDEFS
 */
typedef enum {
    // EVT
    TRC_IO_EVT_EXEC_CB,

    // EVT
    TRC_IO_EVT_TIMER_EXEC_CB,

    // Sleep
    TRC_IO_PM_IDLE,
    TRC_IO_PM_SLEEP,
    TRC_IO_PM_DEEP_SLEEP,
    TRC_IO_PM_ULTRA_SLEEP,
    TRC_IO_PM_CHECKER,

    // Number
    TRC_IO_EVENT_NUM,
} trc_io_event_t;


/*******************************************************************************
 * MACROS
 */
/**
 * @brief The event is triged, so the IO bonding the event will output is_high state
 *
 * @param[in]  event:   trace event
 * @param[out] is_high: output state
 */
#if (!CONFIG_TRACE_IO)
#define TRC_IO(event, is_high)
#define trc_io_set(event, pad_idx, idle_level)
#else
#define TRC_IO(event, is_high)                                                 \
    do {                                                                       \
        extern void trc_io_event_handler(trc_io_event_t event, int is_on);     \
        trc_io_event_handler(event, is_high);                                  \
    } while(0)


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief  Bonding IO with the event, pad_idx must be not 0.
 *
 * @param[in] event      event, ref @trc_io_event_t
 * @param[in] pad_idx    pad_idx, must not be 0.
 * @param[in] idle_level idle level, 0U or 1U
 *******************************************************************************
 */
extern void trc_io_set(trc_io_event_t event, uint8_t pad_idx, uint8_t idle_level);

#endif  /* CONFIG_TRACE_IO */


#ifdef __cplusplus
}
#endif

#endif


/** @} */
