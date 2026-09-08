#ifndef __LITTLEFS_TIME_RTC_H
#define __LITTLEFS_TIME_RTC_H

#include "stdint.h"

#define RTC_DEFAULT_TIME "2025-08-11T18:23:00Z"

typedef struct {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
} time_struct_t;


void timestamp_rtc_init(uint8_t type);

void set_timestamp_date(time_struct_t time);
uint32_t get_timestamp_date(time_struct_t *time);
void rtc_set_time_from_timestamp(uint32_t timestamp);

// 函数：将 RTC 时间日期转为字符串（格式：YYYY-MM-DD HH:MM:SS）
void rtc_to_string(const time_struct_t *time, char *out, uint8_t out_len);
int tim_comper(time_struct_t tim1,time_struct_t tim2);
/**
 * @brief 将字符串解析为 RTC 时间和日期结构
 * @param str 输入时间字符串，例如 "2025-08-07 16:30:45"
 * @param date 输出日期结构
 * @param time 输出时间结构
 * @return 0 成功，-1 失败
 */
int string_to_rtc_time(const char *str,time_struct_t *time) ;
void vStartRtcTask(void);
uint32_t datetime_to_seconds(time_struct_t time);
#endif 
