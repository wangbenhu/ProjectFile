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
 * @brief    RTC driver source file
 * @details  RTC driver source file
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
#include "RTE_driver.h"
#if (RTE_RTC)
#include <stddef.h>
#include "om_driver.h"


/*******************************************************************************
 * MACROS
 */
#define RTC_BEGIN_YEAR          1900U
#define SECONDS_IN_DAY          (24*60*60U)  /* 24 Hour * 60 minute * 60 second */
#define DAYS_IN_YEAR(year)      (is_leap_year(year) ? 366U : 365U)


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    rtc_isr_callback_t second_isr_cb;
    rtc_isr_callback_t alarm_isr_cb;
} drv_rtc_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
#if (RTE_RTC_REGISTER_CALLBACK)
static drv_rtc_env_t rtc_env = {
    .second_isr_cb = NULL,
    .alarm_isr_cb  = NULL,
};
#endif


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
/**
 * @brief  rtc write GR
 *
 * @param[in] value     value
 **/
static void rtc_write_GR(uint32_t value)
{
    OM_RTC->GR |=  RTC_LOCK_MASK;
    while(!(OM_RTC->GR & RTC_LOCK_SYNC_MASK));
    OM_RTC->GR = value;
    OM_RTC->GR &= ~RTC_LOCK_MASK;
    while(OM_RTC->GR & RTC_LOCK_SYNC_MASK);
}

/**
 * @brief  rtc write SAR0
 *
 * @param[in] event         alarm event
 * @param[in] value         value
 **/
static void rtc_write_SAR(rtc_alarm_event_t event, uint32_t value)
{
    OM_ASSERT(event < RTC_ALARM_EVENT_MAX);

    switch (event) {
        case RTC_ALARM_EVENT0:
            OM_RTC->CR &= ~RTC_AE_0_MASK;
            while (OM_RTC->CR & RTC_AE_0_SYNC_MASK);
            OM_RTC->SAR0 = value;
            OM_RTC->CR |= RTC_AE_0_MASK;
            while (!(OM_RTC->CR & RTC_AE_0_SYNC_MASK));
            break;
        case RTC_ALARM_EVENT1:
            OM_RTC->CR &= ~RTC_AE_1_MASK;
            while (OM_RTC->CR & RTC_AE_1_SYNC_MASK);
            OM_RTC->SAR1 = value;
            OM_RTC->CR |= RTC_AE_1_MASK;
            while (!(OM_RTC->CR & RTC_AE_1_SYNC_MASK));
            break;
        case RTC_ALARM_EVENT2:
            OM_RTC->CR &= ~RTC_AE_2_MASK;
            while (OM_RTC->CR & RTC_AE_2_SYNC_MASK);
            OM_RTC->SAR2 = value;
            OM_RTC->CR |= RTC_AE_2_MASK;
            while (!(OM_RTC->CR & RTC_AE_2_SYNC_MASK));
            break;
        default:
            break;
    }
}

static uint8_t is_leap_year(int32_t year)
{
    year += RTC_BEGIN_YEAR;
    return (!((year) % 400) || (((year) % 100) && !((year) % 4)));
}

static uint8_t days_in_month_get(uint8_t is_leap_year, uint8_t mon)
{
    const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return (is_leap_year && (mon == 1)) ? (days[mon] + 1U) : days[mon];
}

// seconds from 1970->tm
static void rtc_localtime(rtc_tm_t *tm, uint32_t seconds)
{
    uint32_t num_days;
    if (tm) {
        // calculate the time less than a day - hours, minutes, seconds
        uint32_t sec = seconds % SECONDS_IN_DAY;
        tm->tm_sec = sec % 60U;
        sec -= tm->tm_sec;
        tm->tm_min = (sec / 60U) % 60;
        sec -= (tm->tm_min * 60U);
        tm->tm_hour = sec / (60U * 60U);

        // calculate wday, year, yday, month, day
        num_days = (seconds - sec)/SECONDS_IN_DAY;
        tm->tm_wday = (num_days + 4U) % 7U;  // 1970-1-1 is Thursday
        tm->tm_year = 70U;  // from 1970
        while (num_days >= DAYS_IN_YEAR(tm->tm_year)) {
            num_days -= DAYS_IN_YEAR(tm->tm_year);
            tm->tm_year++;
        }
        tm->tm_yday = num_days;
        tm->tm_mon = 0;
        while (num_days >= days_in_month_get(is_leap_year(tm->tm_year), tm->tm_mon)) {
            num_days -= days_in_month_get(is_leap_year(tm->tm_year), tm->tm_mon);
            tm->tm_mon++;
        }
        tm->tm_mday = num_days + 1U;
    }
}

// tm -> seconds from 1970
static uint32_t rtc_mktime(const rtc_tm_t *tm)
{
    uint32_t days;

    days = tm->tm_mday - 1U;
    do {
        int32_t month = tm->tm_mon;
        month -= 1U;
        while(month >= 0) {
            days += days_in_month_get(is_leap_year(tm->tm_year), month);
            month -= 1U;
        }
    } while(0);
    do {
        int32_t year = tm->tm_year;
        year-=1U;
        while (year >= 70U) {
            days += DAYS_IN_YEAR(year);
            year -= 1U;
        }
    } while(0);

    return (days * SECONDS_IN_DAY) + (tm->tm_hour * 60U + tm->tm_min) * 60U + tm->tm_sec;
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief RTC initialize and set time
 *
 * @param[in] tm    Pointer to time configuration
 *
 * @return errno
 *******************************************************************************
 */
om_error_t drv_rtc_init(const rtc_tm_t *tm)
{
    NVIC_ClearPendingIRQ(RTC_SECOND_IRQn);
    NVIC_SetPriority(RTC_SECOND_IRQn, RTE_RTC_SECOND_IRQ_PRIORITY);
    NVIC_EnableIRQ(RTC_SECOND_IRQn);

    NVIC_ClearPendingIRQ(RTC_ALARM_IRQn);
    NVIC_SetPriority(RTC_ALARM_IRQn, RTE_RTC_ALARM_IRQ_PRIORITY);
    NVIC_EnableIRQ(RTC_ALARM_IRQn);

    if (OM_PMU->RSVD_SW_REG[0] & PMU_RSVD_SW_REG_RTC_SECOND_RESTORE_PROCESS_MASK) {
        // RTC second value has been restored after reset, users just need restore other required parameters
        OM_CRITICAL_BEGIN();
        OM_PMU->RSVD_SW_REG[0] &= ~PMU_RSVD_SW_REG_RTC_SECOND_RESTORE_PROCESS_MASK;
        OM_CRITICAL_END();
    } else {
        DRV_RCC_CLOCK_ENABLE(RCC_CLK_RTC, 1U);
        if (OM_RTC->CR & RTC_EN_SYNC_MASK) {
            return OM_ERROR_BUSY;
        }
        // RTC reset (!!! Must power on firstly, then reset it)
        DRV_RCC_RESET(RCC_CLK_RTC);
        // Calibration RTC
        //rtc_write_GR(OM_RTC->GR & (~RTC_LOCK_MASK));
        rtc_write_GR((OM_RTC->GR & ~RTC_NC1HZ_MASK) | (drv_rcc_clock_get(RCC_CLK_RTC) - 1));
        rtc_write_GR(OM_RTC->GR | RTC_LOCK_MASK);

        if (tm) {
            drv_rtc_timer_set(tm);
        }
    }

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief RTC deinitialization
 *
 * @return errno
 *******************************************************************************
 */
om_error_t drv_rtc_uninit(void)
{
    DRV_RCC_CLOCK_ENABLE(RCC_CLK_RTC, 0U);

    NVIC_DisableIRQ(RTC_SECOND_IRQn);

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Set time
 *
 * @param[in] tm    Pointer to time configuration
 *******************************************************************************
 */
void drv_rtc_timer_set(const rtc_tm_t *tm)
{
    uint32_t time_sec;

    time_sec = rtc_mktime(tm);
    drv_rtc_control(RTC_CONTROL_SET_RTC_SECOND_CNT, (void *)((uint32_t)time_sec));
}

/**
 *******************************************************************************
 * @brief Get time
 *
 * @param[out] tm   Pointer to time configuration
 *
 * @return  Second counter
 *******************************************************************************
 */
uint32_t drv_rtc_timer_get(rtc_tm_t *tm)
{
    uint32_t second_cnt;

    // SR register should read 2 times
    do {
        second_cnt = OM_RTC->SR;
    } while (OM_RTC->SR != second_cnt);
    rtc_localtime(tm, second_cnt);

    return second_cnt;
}

/**
 *******************************************************************************
 * @brief Set alarm time
 *
 * @param[in] event     Alarm event
 * @param[in] tm        Pointer to time configuration
 *******************************************************************************
 */
void drv_rtc_alarm_set(rtc_alarm_event_t event, rtc_tm_t *tm)
{
    uint32_t alarm_en_mask;

    OM_ASSERT(event < RTC_ALARM_EVENT_MAX);

    alarm_en_mask = (1 << (RTC_AE_0_POS + (uint32_t)event)) | (1 <<(RTC_AIE_0_POS + (uint32_t)event));

    if (tm) {
        uint32_t time_sec = rtc_mktime(tm);
        rtc_write_SAR(event, (uint32_t)time_sec);
        OM_RTC->CR |= alarm_en_mask;
    } else {
        OM_RTC->CR &= ~alarm_en_mask;
    }
}

/**
 *******************************************************************************
 * @brief Get alarm time
 *
 * @param[in] event     Alarm event
 * @param[out] tm       Pointer to time configuration
 *******************************************************************************
 */
void drv_rtc_alarm_get(rtc_alarm_event_t event, rtc_tm_t *tm)
{
    uint32_t time_sec;

    OM_ASSERT(event < RTC_ALARM_EVENT_MAX);

    switch (event) {
        case RTC_ALARM_EVENT0:
            time_sec = OM_RTC->SAR0;
            break;
        case RTC_ALARM_EVENT1:
            time_sec = OM_RTC->SAR1;
            break;
        case RTC_ALARM_EVENT2:
            time_sec = OM_RTC->SAR2;
            break;
        default:
            time_sec = 0U;
            break;
    }

    rtc_localtime(tm, time_sec);
}

/**
 *******************************************************************************
 * @brief RTC control
 *
 * @param[in] control   control command
 * @param[in] argu      argument
 *
 * @return control status
 *******************************************************************************
 */
void *drv_rtc_control(rtc_control_t control, void *argu)
{
    uint32_t ret;

    ret = (uint32_t)OM_ERROR_OK;
    switch (control) {
        case RTC_CONTROL_ACCU_SET:
            OM_RTC->ACCU = (uint32_t)argu;
            break;
        case RTC_CONTROL_GET_RTC_SECOND_CNT:
            ret = drv_rtc_timer_get(NULL);
            break;
        case RTC_CONTROL_SET_RTC_SECOND_CNT:
            while(OM_RTC->CR & RTC_SR_INI_SYNC_MASK);
            OM_RTC->SR = (uint32_t)argu;  /* set rtc time, NOTE: this will take effect in 3 rtc clk */
            OM_RTC->CR |= RTC_CE_MASK;    /* enable rtc, NOTE: this will take effect in 2 rtc clk */
            DRV_DELAY_US(94);   /* wait 3 rtc clk */
            break;
        default:
            ret = (uint32_t)OM_ERROR_UNSUPPORTED;
            break;
    }

    return (void *)ret;
}

#if (RTE_RTC_REGISTER_CALLBACK)
/**
 *******************************************************************************
 * @brief Register second isr callback
 *
 * @param[in] cb    second callback fucntion
 *******************************************************************************
 */
void drv_rtc_register_second_isr_callback(rtc_isr_callback_t cb)
{
    rtc_env.second_isr_cb = cb;
    if (cb) {
        OM_RTC->CR |= RTC_1HZ_IE_MASK;
    } else {
        OM_RTC->CR &= ~RTC_1HZ_IE_MASK;
    }
}

/**
 *******************************************************************************
 * @brief Register alarm isr callback
 *
 * @param[in] cb    alarm callback fucntion
 *******************************************************************************
 */
void drv_rtc_register_alarm_isr_callback(rtc_isr_callback_t cb)
{
    rtc_env.alarm_isr_cb = cb;
}
#endif

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
__WEAK void drv_rtc_second_isr_callback(drv_event_t event)
{
    #if (RTE_RTC_REGISTER_CALLBACK)
    if (rtc_env.second_isr_cb) {
        rtc_env.second_isr_cb(event);
    }
    #endif
}

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
__WEAK void drv_rtc_alarm_isr_callback(drv_event_t event)
{
    #if (RTE_RTC_REGISTER_CALLBACK)
    if (rtc_env.alarm_isr_cb) {
        rtc_env.alarm_isr_cb(event);
    }
    #endif
}

/**
 *******************************************************************************
 * @brief RTC second interrupt service routine
 *******************************************************************************
 */
void drv_rtc_second_isr(void)
{
    uint32_t  status;

    status = OM_RTC->CR;
    if (status & RTC_1HZ_WAKE_MASK) {
        OM_RTC->CR = status | RTC_1HZ_CLR_MASK;
        drv_rtc_second_isr_callback(DRV_EVENT_RTC_SECOND);
    }
}

/**
 *******************************************************************************
 * @brief RTC alarm interrupt service routine
 *******************************************************************************
 */
void drv_rtc_alarm_isr(void)
{
    uint32_t  status;

    status = OM_RTC->CR;
    for (rtc_alarm_event_t i = RTC_ALARM_EVENT0; i < RTC_ALARM_EVENT_MAX; i++) {
        if ((status >> ((uint32_t)i + RTC_AF_0_WAKE_POS)) & 1U) {
            OM_RTC->CR = status | (1U << (RTC_AF_0_CLR_POS + (uint32_t)i));
            drv_rtc_alarm_isr_callback((drv_event_t)(1U << (16U + i)));
        }
    }
}

#endif  /* RTE_RTC */

/** @} */
