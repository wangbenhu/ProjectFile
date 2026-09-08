/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup DOC DOC
 * @ingroup  DOCUMENT
 * @brief    example for using rtc
 * @details  example for using rtc: config second and alarm interrupt
 * @version
 *
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


/*******************************************************************************
 * INCLUDES
 */
#include "om_driver.h"


/*******************************************************************************
 * MACROS
 */


/*******************************************************************************
 * TYPEDEFS
 */


/*******************************************************************************
 * CONST & VARIABLES
 */


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief second interrupt callback
 *
 * @param[in] tm    Pointer to current time
 *
 *******************************************************************************
 */
static void rtc_second_cb(drv_event_t event)
{
    om_printf("second cb\r\n");
}

/**
 *******************************************************************************
 * @brief alarm interrupt callback
 *
 * @param[in] event    Alarm event
 * @param[in] tm       Pointer to current time
 *
 *******************************************************************************
 */
static void rtc_alarm_cb(drv_event_t event)
{
    if (event == DRV_EVENT_RTC_ALARM0) {
        om_printf("alarm  event0\r\n");
    } else if (event == DRV_EVENT_RTC_ALARM1) {
        om_printf("alarm  event1\r\n");
    } else if (event == DRV_EVENT_RTC_ALARM2) {
        om_printf("alarm  event2\r\n");
    }
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief example of using rtc second and alarm function
 *        1. "second cb" will be printed every second
 *        2. "alarm event[n]" will be printed after 2s, 5s, 9s from the beginning
 *
 *******************************************************************************
 */
void example_rtc(void)
{
    rtc_tm_t set_tm;
    rtc_tm_t alarm_tm;

    set_tm.tm_year = 2025 - 1900;   // years from 1900
    set_tm.tm_mon  = 2 - 1;         // months [0, 11]
    set_tm.tm_mday = 12;            // day [1, 31]
    set_tm.tm_hour = 10;            // hours [0, 23]
    set_tm.tm_min  = 51;            // min [0, 59]
    set_tm.tm_sec  = 10;            // sec [0, 59]
    set_tm.tm_wday = 3;             // wday [0, 6]

    memcpy(&alarm_tm, &set_tm, sizeof(rtc_tm_t));

    drv_rtc_init(NULL);

    drv_rtc_register_second_isr_callback(rtc_second_cb);
    drv_rtc_register_alarm_isr_callback(rtc_alarm_cb);

    alarm_tm.tm_sec += 2;
    drv_rtc_alarm_set(RTC_ALARM_EVENT0, &alarm_tm);
    alarm_tm.tm_sec += 3;
    drv_rtc_alarm_set(RTC_ALARM_EVENT1, &alarm_tm);
    alarm_tm.tm_sec += 4;
    drv_rtc_alarm_set(RTC_ALARM_EVENT2, &alarm_tm);

    // start signal
    drv_rtc_timer_set(&set_tm);
}


/** @} */