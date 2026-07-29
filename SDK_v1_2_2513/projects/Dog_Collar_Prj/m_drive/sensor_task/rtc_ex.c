#include "rtc_ex.h"
#include "common_def.h"

rtc_tm_t now_tm;

void rtc_init(void)
{
    rtc_tm_t set_tm;
    rtc_tm_t alarm_tm;

    set_tm.tm_year = 2025 - 1900;   // years from 1900
    set_tm.tm_mon  = 0;         		// months [0, 11]
    set_tm.tm_mday = 1;            // day [1, 31]
    set_tm.tm_hour = 0;            // hours [0, 23]
    set_tm.tm_min  = 0;            // min [0, 59]
    set_tm.tm_sec  = 0;            // sec [0, 59]
    set_tm.tm_wday = 1;             // wday [0, 6]

    memcpy(&alarm_tm, &set_tm, sizeof(rtc_tm_t));

    drv_rtc_init(NULL);

    // start signal
    drv_rtc_timer_set(&set_tm);
}

void get_timestamp_date(rtc_tm_t *time)
{
	rtc_tm_t get_tm;
	drv_rtc_timer_get(&get_tm);
	time->tm_year = get_tm.tm_year+1900;
	time->tm_mon = get_tm.tm_mon+1;
	time->tm_mday = get_tm.tm_mday;
	time->tm_hour = get_tm.tm_hour;
	time->tm_min = get_tm.tm_min;
	time->tm_sec = get_tm.tm_sec;
}


