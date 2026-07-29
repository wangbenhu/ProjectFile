/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup EVT_TIMER EVT_TIMER
 * @ingroup  EVT
 * @brief    event timer
 * @details  event timer
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __EVT_TIMER_H
#define __EVT_TIMER_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdbool.h>
#include "om_common.h"
#include "om_driver.h"
#include "evt.h"

#ifdef  __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
/// Max tick: 32bit, 30.52us (1/32768)s
#define EVT_TIMER_MAX_TICK              PMU_TIMER_MAX_TICK
/// Max delay tick
#define EVT_TIMER_MAX_DELAY             (EVT_TIMER_MAX_TICK/2 - 1)
/// unit transmite 1ms to 30.52us
#define EVT_TIMER_MS2TICK(time)         PMU_TIMER_MS2TICK(time)
/// unit transmite 30.52us to 1ms
#define EVT_TIMER_TICK2MS(time)         PMU_TIMER_TICK2MS(time)


/*******************************************************************************
 * TYPEDEFS
 */
/// Timer mode for one shot or repeat
typedef enum {
    /// Only run once
    EVT_TIMER_ONE_SHOT,
    /// Repeat until stop it
    EVT_TIMER_REPEAT,
} evt_timer_mode_t;

/// @cond
/// timer object
typedef struct {
    om_list_node_t hdr;
    uint32_t delay_tick;
    void *cb;
    evt_timer_mode_t mode;
    void *param;

    uint32_t time;
} evt_timer_t;
/// @endcond

/**
 * @brief software timer expire callback
 *
 * @param[in] timer  Timer object
 * @param[in] param  Parameter with evt_timer_set()
 *
 * @return None
 **/
typedef void (*evt_timer_callback_t)(evt_timer_t *timer, void *param);


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/// @cond
/**
 *******************************************************************************
 * @brief timer init
 *
 * @return None
 *******************************************************************************
 **/
extern void evt_timer_init(void);
/// @endcond

/**
 *******************************************************************************
 * @brief Setup a software timer with tick
 *
 * @param[in] timer  the timer object (must be static or global variable)
 * @param[in] delay_tick  its uint is 1/32768s, about 30.52us, max is "EVT_MAX_TIMER_DELAY"
 * @param[in] mode  one shot or repeat
 * @param[in] callback  expire callback
 * @param[in] param  params
 *
 * @return false: Set timer fail;
 *         true: Set timer success
 *******************************************************************************
 **/
extern bool evt_timer_set_tick(evt_timer_t *timer, uint32_t delay_tick, evt_timer_mode_t mode,
        evt_timer_callback_t callback, void *param);

/**
 *******************************************************************************
 * @brief Setup a software timer with millisecond
 *
 * @param[in] timer  the timer object (must be static or global variable)
 * @param[in] delay  its uint is 1ms, max is "CO_TIME_SYS2MS(CO_MAX_TIMER_DELAY)"
 * @param[in] mode  one shot or repeat
 * @param[in] callback  expire callback
 * @param[in] param  params
 *
 * @return false: Set timer fail;
 *         true: Set timer success
 *******************************************************************************
 **/
extern bool evt_timer_set(evt_timer_t *timer, uint32_t delay, evt_timer_mode_t mode,
        evt_timer_callback_t callback, void *param);

/**
 *******************************************************************************
 * @brief delete a timer
 *
 * @param[in] timer  the timer object (must be static or global variable)
 *******************************************************************************
 **/
extern void evt_timer_del(evt_timer_t *timer);

/**
 *******************************************************************************
 * @brief timer dump
 *
 * @note If this function called in co_timer expired callback, the expired timer don't dump
 *
 * @param[in] printf_dump_func  dump function, link printf
 *******************************************************************************
 **/
extern void evt_timer_dump(void *printf_dump_func);

#ifdef  __cplusplus
}
#endif

#endif  /* __EVT_H */

/** @} */
