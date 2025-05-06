#include "myPower.h"
#include "adc.h"
#include "usart.h"
#include "stm32f1xx_it.h"
#include "myBle.h"


extern adcRead adcVol;
/*
*********************************************************************************************************
* 函 数 名: batVolHandle
* 功能说明: 电池电量处理
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void batVolHandle(void)
{
    static uint32_t SysCnt = 0;
    static bool onReadVBAT = false; // 开机检测直流电压标志或者ADC检测出错就一直检测
    uint8_t temp = 0;
    uint32_t R3 = 90000, R4 = 10000; // 单位：Ω
    uint32_t adc1 = 0;
    float vol = 0.0, vbat = 0.0;

    if (false == onReadVBAT)
    {
        temp = api_GetSysTickCount100ms(); // 100ms检测一次
        onReadVBAT = true;
    }
    else
    {
        temp = api_GetSysTickCount2s(); // api_GetSysTickCount5min(); // 5分钟检测一次
    }
    if (SysCnt != temp)
    {
        SysCnt = temp;
				if(ble_status_get())
				{
					ADC_Read();
					adc1 = adcVol.ADC_Vol[0];
					vol = (float)(adc1 * 3.3 / 4095);
					vbat = (float)(((R3 + R4) * vol) / R4 + 0.65); // 最大30V
					Power_Report_Set(vbat);
					APP_Data_Res_call_fun(DATA_TYPE_POWER_REPORT);
				}
      //  debug_uart_printf("vbat:%f\r\n", vbat);
    }
}
