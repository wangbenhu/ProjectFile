#ifndef __RTC_EX_H__
#define __RTC_EX_H__
#include "om_driver.h"

extern rtc_tm_t now_tm;
void rtc_init(void);
void get_timestamp_date(rtc_tm_t *time);

#endif

