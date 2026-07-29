#include "littlefs_time_rtc.h"
#include "../source/drv_rtc.h"
#include <stdio.h>
#include "cmsis_os2.h"
#include "common_def.h"

/**
* @brief log redefine
*/
#define RTC_DEBUG_CONFIG 		1
#if (RTC_DEBUG_CONFIG == 1)
#define RTC_LOG_DEBUG(format, ...)               	log_debug( format,  ## __VA_ARGS__)
#else
#define RTC_LOG_DEBUG(format, ...)
#endif

#define TEST_TASK_RTC 		0

osMutexId_t rtc_mutex_id;  // 互斥锁ID
int get_weekday(int year, int month, int day);


// 每个月的天数
static const uint8_t days_in_month[12] = {
    31,28,31,30,31,30,31,31,30,31,30,31
};

//时间转换字符串
// 判断是否为闰年
static bool is_leap_year(int year) {
    return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

// 计算指定年份之前的闰年数量
static int leap_years_since_epoch(int year) {
    year--; // 不包含当前年
    return (year / 4) - (year / 100) + (year / 400);
}
void timestamp_rtc_init(uint8_t type)
{
	    // 创建互斥锁
    const osMutexAttr_t rtc_mutex_attr = { .name = "rtc_mutex" };
    rtc_mutex_id = osMutexNew(&rtc_mutex_attr);
	
	osMutexAcquire(rtc_mutex_id, osWaitForever);
	time_struct_t default_tm;
    rtc_tm_t set_tm;

	string_to_rtc_time(RTC_DEFAULT_TIME,&default_tm);
//	RTC_LOG_DEBUG("timestamp_rtc_init = %s %d %d %d %d %d %d \r\n",RTC_DEFAULT_TIME,default_tm.year,\
//	default_tm.month,default_tm.day,default_tm.hour,default_tm.minute,default_tm.second);
    set_tm.tm_year = default_tm.year - 1900;   // years from 1900
    set_tm.tm_mon  = default_tm.month - 1;         // months [0, 11]
    set_tm.tm_mday = default_tm.day;            // day [1, 31]
    set_tm.tm_hour = default_tm.hour;            // hours [0, 23]
    set_tm.tm_min  = default_tm.minute;            // min [0, 59]
    set_tm.tm_sec  = default_tm.second;            // sec [0, 59]
	
	
    set_tm.tm_wday = get_weekday(default_tm.year,default_tm.month,default_tm.day);             // wday [0, 6]

    drv_rtc_init(NULL);
    // start signal
    if(type)
	{
		drv_rtc_timer_set(&set_tm);
	}
	osMutexRelease(rtc_mutex_id);
}

// 将日期时间转换为秒数（Unix时间戳）
uint32_t datetime_to_seconds(time_struct_t time) {
    // 校验输入范围（可根据需求调整）
    if (time.year < 1970 || time.month < 1 || time.month > 12 || time.day < 1 || time.day > 31 ||
        time.hour < 0 || time.hour > 23 || time.minute < 0 || time.minute > 59 || time.second < 0 || time.second > 59) {
        return 0; // 或返回错误码
    }

    // 每月天数（平年）
    const uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    // 计算从1970年到目标年的总天数
    uint32_t total_days = (time.year - 1970) * 365;
    total_days += leap_years_since_epoch(time.year) - leap_years_since_epoch(1970);

    // 处理当前年的月份天数
    for (int m = 1; m < time.month; m++) {
        total_days += days_in_month[m - 1];
        if (m == 2 && is_leap_year(time.year)) {
            total_days++; // 闰年2月多1天
        }
    }

    // 加上当前月的天数
    total_days += time.day - 1; // 天数从0开始计数

    // 转换为秒数
    uint32_t total_seconds = total_days * 86400ULL; // 每天86400秒
    total_seconds += time.hour * 3600ULL;
    total_seconds += time.minute * 60ULL;
    total_seconds += time.second;

    return total_seconds;
}

//时间戳字符串转换时间
// 秒数转年月日时分秒
void seconds_to_datetime(uint32_t seconds, time_struct_t *dt) {
    uint32_t days = seconds / 86400;
    uint32_t rem  = seconds % 86400;

    dt->hour = rem / 3600;
    rem %= 3600;
    dt->minute = rem / 60;
    dt->second = rem % 60;

    // 从1970年开始计算
    dt->year = 1970;
    while (1) {
        uint16_t days_this_year = is_leap_year(dt->year) ? 366 : 365;
        if (days >= days_this_year) {
            days -= days_this_year;
            dt->year++;
        } else {
            break;
        }
    }

    // 计算月份
    dt->month = 1;
    for (int i = 0; i < 12; i++) {
        uint8_t dim = days_in_month[i];
        if (i == 1 && is_leap_year(dt->year)) dim = 29; // 2月闰年
        if (days >= dim) {
            days -= dim;
            dt->month++;
        } else {
            break;
        }
    }

    dt->day = days + 1; // day 从 1 开始
}
uint32_t get_timestamp_date(time_struct_t *time)
{
	osMutexAcquire(rtc_mutex_id, osWaitForever);
	uint32_t return_sec=0;
	rtc_tm_t get_tm;
	if(time==NULL)
	{
		time_struct_t test_time;
		drv_rtc_timer_get(&get_tm);
		test_time.year = get_tm.tm_year+1900;
		test_time.month =get_tm.tm_mon+1;
		test_time.day =get_tm.tm_mday;
		test_time.hour =get_tm.tm_hour;
		test_time.minute = get_tm.tm_min;
		test_time.second =get_tm.tm_sec;
		return_sec = datetime_to_seconds(test_time);
	}
	else
	{
		drv_rtc_timer_get(&get_tm);
		time->year = get_tm.tm_year+1900;
		time->month =get_tm.tm_mon+1;
		time->day =get_tm.tm_mday;
		time->hour =get_tm.tm_hour;
		time->minute = get_tm.tm_min;
		time->second =get_tm.tm_sec;
		return_sec = datetime_to_seconds(*time);
	}
	osMutexRelease(rtc_mutex_id);
	return return_sec;
}
// 函数：将 RTC 时间日期转为字符串（格式：YYYY-MM-DD HH:MM:SS）
void rtc_to_string(const time_struct_t *time, char *out, uint8_t out_len) {
	
    if (!time  || !out || out_len < sizeof(time_struct_t)) {
        if (out) strncpy(out, "Invalid", out_len);
        return; 
    }

    snprintf(out, out_len, "%4d-%02d-%02dT%02d:%02d:%02dZ",
             time->year, time->month, time->day,
             time->hour, time->minute, time->second);
}

int tim_comper(time_struct_t tim1,time_struct_t tim2)
{
	int return_data=0;
	uint32_t tim1_tmp=0,tim2_tmp=0;
	tim1_tmp = datetime_to_seconds(tim1);
	tim2_tmp = datetime_to_seconds(tim2);
	return_data = tim1_tmp > tim2_tmp ? 1 : (tim1_tmp < tim2_tmp ? -1 : 0);
	return return_data;
}

/**
 * @brief 将字符串解析为 RTC 时间和日期结构
 * @param str 输入时间字符串，例如 "2025-08-07 16:30:45"
 * @param date 输出日期结构
 * @param time 输出时间结构
 * @return 0 成功，-1 失败
 */
int string_to_rtc_time(const char *str,time_struct_t *time) {
	
    if (!str || !time) return -1;

    int year, month, day, hour, minute, second;

    // 解析格式：YYYY-MM-DD HH:MM:SS
    int matched = sscanf(str, "%d-%d-%dT%d:%d:%dZ",
                         &year, &month, &day,
                         &hour, &minute, &second);

    if (matched != 6) return -1;

    // 裁剪到 RTC 支持范围
   // if (year < 2000 || year > 2099) return -1;

    time->year  = (uint32_t)year;
    time->month = (uint8_t)month;
    time->day  = (uint8_t)day;

    time->hour   = (uint8_t)hour;
    time->minute = (uint8_t)minute;
    time->second = (uint8_t)second;
    return 0;
}
/**
 * @brief 获取星期几
 * @param year  年份（例如 2025）
 * @param month 月份（1~12）
 * @param day   日期（1~31）
 * @return      0~6（0=星期日，1=星期一，...，6=星期六）
 */
int get_weekday(int year, int month, int day) {
    // 基于 Zeller 算法
    if (month < 3) {
        month += 12;
        year--;
    }

    int k = year % 100;        // 年的后两位
    int j = year / 100;        // 世纪数

    int h = (day + 13*(month + 1)/5 + k + k/4 + j/4 + 5*j) % 7;

    // Zeller 输出：0=Saturday, 1=Sunday, ..., 6=Friday
    // 转换为 0=Sunday, 1=Monday, ..., 6=Saturday：	
    return (h + 6) % 7;
}

void set_timestamp_date(time_struct_t time)
{
	osMutexAcquire(rtc_mutex_id, osWaitForever);
	rtc_tm_t set_tm;
	set_tm.tm_year = time.year - 1900;   // years from 1900
    set_tm.tm_mon  = time.month - 1;         // months [0, 11]
    set_tm.tm_mday = time.day;            // day [1, 31]
    set_tm.tm_hour = time.hour;            // hours [0, 23]
    set_tm.tm_min  = time.minute;            // min [0, 59]
    set_tm.tm_sec  = time.second;            // sec [0, 59]

    set_tm.tm_wday = get_weekday(set_tm.tm_year,set_tm.tm_mon,set_tm.tm_mday);             // wday [0, 6]
	
	drv_rtc_timer_set(&set_tm);
	osMutexRelease(rtc_mutex_id);
}

//时间戳转UTC格式
void rtc_set_time_from_timestamp(uint32_t timestamp)
{
    time_struct_t time;
    
    uint32_t seconds = timestamp;
    
    // 计算分钟
    time.minute = (seconds / 60) % 60;
    seconds -= time.minute * 60;
    
    // 计算小时
    time.hour = (seconds / 3600) % 24;
    seconds -= time.hour * 3600;
    
    // 计算天数（从1970-01-01开始）
    uint32_t days = timestamp / 86400;
    
    // 年份计算（从1970年开始）
    uint32_t year = 1970;
    uint32_t days_in_year;
    
    while (days > 0) {
        // 判断闰年
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            days_in_year = 366;
        } else {
            days_in_year = 365;
        }
        
        if (days >= days_in_year) {
            days -= days_in_year;
            year++;
        } else {
            break;
        }
    }
    
    time.year = year;
    
    // 月份计算
    uint8_t month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    // 闰年调整
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        month_days[1] = 29;
    }
    
    uint8_t month = 0;
    while (days >= month_days[month]) {
        days -= month_days[month];
        month++;
    }
    
    time.month = month + 1;  // 转换为1-12
    time.day = days + 1;     // 转换为1-31
    
    // 设置秒数
    time.second = seconds % 60;
    
    // 调用设置函数
    set_timestamp_date(time);
}

#if (TEST_TASK_RTC)
void rtcTimerCallback(void *argument) {
  // 在这里执行定时器相关的操作
	uint32_t tmp_tim=0;
		time_struct_t get_time;
	tmp_tim = get_timestamp_date(&get_time);
 RTC_LOG_DEBUG("date = %d\r\n",tmp_tim);
//	get_time.day+=1;
	seconds_to_datetime(tmp_tim,&get_time);
	 RTC_LOG_DEBUG("sec  = %d %d %d %d %d %d\r\n\r\n",get_time.year,get_time.month,get_time.day,get_time.hour,get_time.minute,get_time.second);
}



static void vRtcScheduleTask(void *argument)
{
	timestamp_rtc_init(1);
	char get_date_buffer[30]={0};
	uint8_t get_date_len=0;
	time_struct_t get_time;
	
/*	
	// 创建一个周期性定时器
  const osTimerAttr_t timer_attr = {
    .name = "MyTimer",
    .attr_bits = 0,
    .cb_mem = NULL,
    .cb_size = 0,
  };
  osTimerId_t timer_id = osTimerNew(rtcTimerCallback, osTimerPeriodic, NULL, &timer_attr);

  if (timer_id != NULL) {
	  om_log(OM_LOG_INFO,"osTimerStart ok\r\n");
    // 启动定时器，设置周期为1000毫秒 (1秒)
    osTimerStart(timer_id, 2000);
  }
  else
  {
	   om_log(OM_LOG_INFO,"osTimerStart no\r\n");
  }
	*/
	
	while (1) 
	{		
		osDelay(2000);
		//get_timestamp_date(&get_time);
		//rtc_to_string(&get_time,get_date_buffer,sizeof(get_date_buffer));
		
	//	om_log(OM_LOG_INFO,"recver = %s\r\n",get_date_buffer);
	//	om_log(OM_LOG_INFO,"date = %d\r\n",get_timestamp_date(&get_time));
//		
//		get_time.second = get_time.second <(59-2)?get_time.second+2:0;
//		set_timestamp_date(get_time);
//		om_log(OM_LOG_INFO,"1111\r\n");
//		get_timestamp_date(&get_time);
//		om_log(OM_LOG_INFO,"222\r\n");
//		rtc_to_string(&get_time,get_date_buffer,sizeof(get_date_buffer));
		//om_log(OM_LOG_INFO,"date2 = %s\r\n",get_date_buffer);

	}
}
void vStartRtcTask(void)
{
    const osThreadAttr_t rtcThreadAttr = {
        .name = NULL,
        .attr_bits = 0,
        .cb_mem = NULL,
        .cb_size = 0,
        .stack_mem = NULL,
        .stack_size = 4096,
        .priority = osPriorityNone,
        .tz_module = 0,
    };

    // Create ble Task
    osThreadNew(vRtcScheduleTask, NULL, &rtcThreadAttr);
}
#else

#endif
