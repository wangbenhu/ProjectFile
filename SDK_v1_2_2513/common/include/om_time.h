/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup TIME TIME
 * @ingroup  COMMON
 * @brief    Time
 * @details  Time
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __OM_TIME_H
#define __OM_TIME_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include <stdbool.h>
#include "om_driver.h"


/*********************************************************************
 * MACROS
 */

/// Max timer tick
#define OM_TIMER_MAX_TICK       0xFFFFFFFFU
/// Max timer delay
#define OM_TIMER_MAX_DELAY      (OM_TIMER_MAX_TICK/2 - 1)
/// Invalid time
#define OM_TIME_INVALID         0xFFFFFFFFU


/*********************************************************************
 * TYPEDEFS
 */


/*********************************************************************
 * EXTERN VARIABLES
 */


/*********************************************************************
 * EXTERN FUNCTIONS
 */

/**
 * @brief get current stack time(tick)
 *
 * @return current time, uint is 30.5us
 **/
__STATIC_FORCEINLINE uint32_t om_time(void)
{
    return drv_pmu_timer_cnt_get();
}

/**
 * @brief convert time to safe
 *
 * @param[in] time  time
 *
 * @return safe time
 **/
__STATIC_FORCEINLINE uint32_t om_time_to_safe(uint32_t time)
{
#if (OM_TIMER_MAX_TICK == 0xFFFFFFFF)
    return time;
#else
    return time & OM_TIMER_MAX_TICK;
#endif
}

/**
 * @brief time increase +
 *
 * @param[in] time  time
 * @param[in] increase  +value
 *
 * @return increase result
 **/
__STATIC_FORCEINLINE uint32_t om_time_increase(uint32_t time, uint32_t increase)
{
#if (OM_TIMER_MAX_TICK == 0xFFFFFFFF)
    return time + increase;
#else
    OM_ASSERT(time <= OM_TIMER_MAX_TICK);
    return om_time_to_safe(time + increase);
#endif
}

/**
 * @brief time decrease -
 *
 * @param[in] time  time value
 * @param[in] decrease  -value
 *
 * @return decrease result
 **/
__STATIC_FORCEINLINE uint32_t om_time_decrease(uint32_t time, uint32_t decrease)
{
#if (OM_TIMER_MAX_TICK == 0xFFFFFFFF)
    return time - decrease;
#else
    OM_ASSERT(time <= OM_TIMER_MAX_TICK);
    return (time < decrease) ? (OM_TIMER_MAX_TICK + 1 - (decrease - time)) : (time - decrease);
#endif
}

/**
 * @brief different between with two time
 *
 * @param[in] me  one time
 * @param[in] he  another time
 *
 * @return different
 **/
__STATIC_FORCEINLINE uint32_t om_time_diff(uint32_t me, uint32_t he)
{
#if (OM_TIMER_MAX_TICK == 0xFFFFFFFF)
    uint32_t diff1 = me - he;
    uint32_t diff2 = he - me;
#else
    uint32_t diff1 = (me > he) ? (me - he) : (he - me);
    uint32_t diff2 = OM_TIMER_MAX_TICK - diff1 + 1;
#endif

    return (diff1 < diff2) ? diff1 : diff2;
}

/**
 * @brief compare between with two time
 *
 * @param[in] me  one time
 * @param[in] he  another time
 *
 * @return if me is smaller or equeal, return true, otherwise return false.
 **/
__STATIC_FORCEINLINE bool om_time_compare_equal_lesser(uint32_t me, uint32_t he)
{
#if (OM_TIMER_MAX_TICK == 0xFFFFFFFF)
    return (he - me < OM_TIMER_MAX_DELAY);
#else
    return (me > he) ? (me - he > OM_TIMER_MAX_DELAY) : (he - me < OM_TIMER_MAX_DELAY);
#endif
}

/**
 * @brief compare between with two time
 *
 * @param[in] me  one time
 * @param[in] he  another time
 *
 * @return if me is smaller, return true, otherwise return false.
 **/
__STATIC_FORCEINLINE bool om_time_compare_lesser(uint32_t me, uint32_t he)
{
    return (me != he) && om_time_compare_equal_lesser(me, he);
}

/**
 * @brief compare between with two time
 *
 * @param[in] me  one time
 * @param[in] he  another time
 *
 * @return if me is greater or equeal, return true, otherwise return false.
 **/
__STATIC_FORCEINLINE bool om_time_compare_equal_greater(uint32_t me, uint32_t he)
{
    return (me == he) || !om_time_compare_equal_lesser(me, he);
}

/**
 * @brief compare between with two time
 *
 * @param[in] me  one time
 * @param[in] he  another time
 *
 * @return if me is greater, return true, otherwise return false.
 **/
__STATIC_FORCEINLINE bool om_time_compare_greater(uint32_t me, uint32_t he)
{
    return !om_time_compare_equal_lesser(me, he);
}

/**
 * @brief whether time is pasted
 *
 * @param[in] time  time
 * @param[in] cur_time  current time
 *
 * @return whether pasted
 **/
__STATIC_FORCEINLINE bool om_time_past(uint32_t time, uint32_t cur_time)
{
    return om_time_compare_equal_lesser(time, cur_time);
}

/**
 * @brief time pasting time
 *
 * @param[in] time  time
 * @param[in] cur_time  current time
 *
 * @return 0:pasted, >0:will past time
 **/
__STATIC_FORCEINLINE uint32_t om_time_pasting_time(uint32_t time, uint32_t cur_time)
{
    uint32_t delay;

#if (OM_TIMER_MAX_TICK == 0xFFFFFFFF)
    delay = time - cur_time;
#else
    if (time > cur_time)
        delay = time - cur_time;
    else
        delay = OM_TIMER_MAX_TICK - cur_time + time;
#endif

    return (delay > OM_TIMER_MAX_DELAY) ? 0 : delay;
}

/**
 * @brief time delay tick is pasted
 *
 * @param[in] delay  delay with tick
 * @param[in] prev_past_time  prev delay time, if delay pasted, the value will be modify to current time
 *
 * @return pasted
 **/
__STATIC_FORCEINLINE bool om_time_delay_past(uint32_t delay, uint32_t *prev_past_time)
{
    bool pasted;
    uint32_t cur_time = om_time();

    if(*prev_past_time == OM_TIMER_MAX_TICK)
        *prev_past_time = cur_time;

    pasted = om_time_diff(cur_time, *prev_past_time) > delay;
    if(pasted)
        *prev_past_time = cur_time;

    return pasted;
}

/**
 * @brief time intersection
 *
 * @param[in] time1  time1
 * @param[in] time1_duration  time1_duration
 * @param[in] time2  time2
 * @param[in] time2_duration  time2_duration
 *
 * @return whether is intersection
 **/
__STATIC_FORCEINLINE bool om_time_intersection(uint32_t time1, uint32_t time1_duration,
                                        uint32_t time2, uint32_t time2_duration)
{
    uint32_t time1_tail = om_time_increase(time1, time1_duration);
    uint32_t time2_tail = om_time_increase(time2, time2_duration);

    return !(om_time_compare_equal_lesser(time2_tail, time1) || om_time_compare_equal_lesser(time1_tail, time2));
}

#ifdef __cplusplus
}
#endif

#endif

/** @} */
