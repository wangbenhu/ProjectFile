/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup RTC RTC
 * @ingroup  DRIVER
 * @brief    RTC driver
 * @details  RTC driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_RTC_H
#define __DRV_RTC_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_RTC)
#include <stdint.h>
#include "om_driver.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * INCLUDES
 */
#define RTC_YEAR(year)        ((year) - 1900U)


/*******************************************************************************
 * TYPEDEFS
 */
/// RTC Alarm Event Type
typedef enum {
    /// alarm for event0
    RTC_ALARM_EVENT0    = 0U,
    /// alarm for event1
    RTC_ALARM_EVENT1    = 1U,
    /// alarm for event2
    RTC_ALARM_EVENT2    = 2U,

    RTC_ALARM_EVENT_MAX,
} rtc_alarm_event_t;

/// RTC Control
typedef enum {
    RTC_CONTROL_ACCU_SET,               /*!< set accu reg, argu is value */
    RTC_CONTROL_GET_RTC_SECOND_CNT,     /*!< read RTC second count register */
    RTC_CONTROL_SET_RTC_SECOND_CNT,     /*!< write RTC second count register */
} rtc_control_t;

/// RTC time
typedef struct {
    int         tm_sec;         /*!< seconds after the minute (0-59) */
    int         tm_min;         /*!< minutes after the hour (0-59) */
    int         tm_hour;        /*!< hours since midnight (0-23) */
    int         tm_mday;        /*!< day of the month (1-31) */
    int         tm_mon;         /*!< months since January (0-11) */
    int         tm_year;        /*!< years since 1900 */
    int         tm_wday;        /*!< days since Sunday (0-6) */
    int         tm_yday;        /*!< days since January 1 (0-365) */
} rtc_tm_t;

/// RTC second & alarm interrupt callback
typedef void (*rtc_isr_callback_t)(drv_event_t event);


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief RTC initialize
 *
 * @param[in] tm    Pointer to time configuration
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_rtc_init(const rtc_tm_t *tm);

/**
 *******************************************************************************
 * @brief RTC deinitialization
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_rtc_uninit(void);

#if (RTE_RTC_REGISTER_CALLBACK)
/**
 *******************************************************************************
 * @brief Register second isr callback
 *
 * @param[in] cb:     second callback fucntion
 *
 *******************************************************************************
 */
extern void drv_rtc_register_second_isr_callback(rtc_isr_callback_t cb);

/**
 *******************************************************************************
 * @brief Register alarm isr callback
 *
 * @param[in] cb:     alarm callback fucntion
 *
 *******************************************************************************
 */
extern void drv_rtc_register_alarm_isr_callback(rtc_isr_callback_t cb);
#endif

/**
 *******************************************************************************
 * @brief Set current time
 *
 * @param[in] tm:     pointer to time configuration
 *******************************************************************************
 */
extern void drv_rtc_timer_set(const rtc_tm_t *tm);

/**
 *******************************************************************************
 * @brief Get current time
 *
 * @param[out] tm:    pointer to time configuration
 * @return            second counter
 *******************************************************************************
 */
extern uint32_t drv_rtc_timer_get(rtc_tm_t *tm);

/**
 *******************************************************************************
 * @brief Set alarm time
 *
 * @param[in] event:  alarm event
 * @param[in] tm:     pointer to time configuration
 *
 *******************************************************************************
 */
void drv_rtc_alarm_set(rtc_alarm_event_t event, rtc_tm_t *tm);

/**
 *******************************************************************************
 * @brief Get alarm time
 *
 * @param[in] event:  alarm event
 * @param[in] tm:     pointer to time configuration
 *
 *******************************************************************************
 */
extern void drv_rtc_alarm_get(rtc_alarm_event_t event, rtc_tm_t *tm);

/**
 *******************************************************************************
 * @brief RTC control
 *
 * @param[in] control:  control command
 * @param[in] argu:     argument
 *
 * @return control status
 *******************************************************************************
 */
extern void *drv_rtc_control(rtc_control_t control, void *argu);

/**
 *******************************************************************************
 * @brief The interrupt callback for RTC second event. It is a weak function.
 *        User should define own callback in user file, other than modify it in
 *        the RTC driver.
 *
 * @param event  The RTC driver event
 *               - DRV_EVENT_RTC_ALARM0
 *               - DRV_EVENT_RTC_ALARM1
 *               - DRV_EVENT_RTC_ALARM2
 *               - DRV_EVENT_RTC_SECOND
 *******************************************************************************
 */
extern void drv_rtc_second_isr_callback(drv_event_t event);

/**
 *******************************************************************************
  * @brief The interrupt callback for RTC alarm event. It is a weak function.
 *        User should define own callback in user file, other than modify it in
 *        the RTC driver.
 *
 * @param event  rtc driver event
 *               - DRV_EVENT_RTC_ALARM0
 *               - DRV_EVENT_RTC_ALARM1
 *               - DRV_EVENT_RTC_ALARM2
 *               - DRV_EVENT_RTC_SECOND
 *******************************************************************************
 */
extern void drv_rtc_alarm_isr_callback(drv_event_t event);

/**
 *******************************************************************************
 * @brief RTC second interrupt service routine
 *
 *******************************************************************************
 */
extern void drv_rtc_second_isr(void);

/**
 *******************************************************************************
 * @brief RTC alarm interrupt service routine
 *
 *******************************************************************************
 */
extern void drv_rtc_alarm_isr(void);


#ifdef __cplusplus
}
#endif

#endif  /* (RTE_RTC) */

#endif  /* __DRV_RTC_H */


/** @} */
