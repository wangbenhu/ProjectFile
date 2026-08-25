#ifndef __M_BATTERY_H__
#define __M_BATTERY_H__

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include "om_log.h"
/* Kernel includes. */
#include "cmsis_os2.h"
#include "common_def.h"


//#define log_debug(...) om_log(OM_LOG_INFO, ##__VA_ARGS__)

//电池从4.25以上冲上去的 需要delay后充满，如果是上电后读取直接是4.3以上直接算满电
//#define BATTERY_VOLTAGE_VOLTAGE_OVER	(4300)
#define BATTERY_VOLTAGE_FULL 			(4100)//100%
//#define BATTERY_VOLTAGE_NORMAL_M5 		(3550)//4.3v-3.5v 	//100%-20% 
//#define BATTERY_VOLTAGE_LOW_M3 			(3300)//3.5v-3.30v 	//20%-0%
//#define BATTERY_VOLTAGE_EMPTY_M2 		(3300)//3.30v		//%0	

uint8_t VoltageToPercent(uint32_t voltage);
CHARGE_STATUS_T Get_ChargeIO_Status(void);
uint32_t r_PowerOn_BatteryLevel(void);
// 获取电池容量百分比
uint8_t PM_GetBatteryCapacity(void);
// 获取电池充电状态
CHARGE_STATUS_T PM_GetChargeStatus(void);
void battery_updata_flag_clear(void);
uint8_t battery_updata_flag_get(void);
//获取ADC采集原始值
uint32_t get_gpadc_read_data(void);
//中断方式读取电池电压
void pm_vbattery_get_int(void);
uint32_t PercentToVoltage(uint8_t soc);
#endif
