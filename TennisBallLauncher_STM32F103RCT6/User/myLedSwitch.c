#include "myLedSwitch.h"
#include "main.h"
#include "stm32f1xx_it.h"
#include "usart.h"
#include "myApp.h"
#include "myLowSpeedMotor.h"
extern stm32f1xx_it driveInt;
extern myApp myAppStr;
/*
*********************************************************************************************************
* 函 数 名: myLedSwitchOn
* 功能说明: 打开光电开关
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myLedSwitchOn(void)
{
    HAL_GPIO_WritePin(LED_SWITCH_EN_GPIO_Port, LED_SWITCH_EN_Pin, GPIO_PIN_SET);
}

/*
*********************************************************************************************************
* 函 数 名: myLedSwitchOff
* 功能说明: 关闭光电开关
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myLedSwitchOff(void)
{
    HAL_GPIO_WritePin(LED_SWITCH_EN_GPIO_Port, LED_SWITCH_EN_Pin, GPIO_PIN_RESET);
}

/*
*********************************************************************************************************
* 函 数 名: myLedSwitchIntHandle
* 功能说明: 光电开关中断信号处理
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myLedSwitchIntHandle(void)
{
    static uint32_t SysCnt = 0, Debounce_time = 0, intervalCnt = 0;
    uint8_t temp = 0;
    static int16_t intervalTime = -1; // 档位对应的时间
    temp = api_GetSysTickCount10ms();
    if (SysCnt != temp)
    {
        SysCnt = temp;
        if (true == driveInt.ledSwitchInt) // 光电开关有中断输入
        {
            Debounce_time++;        // 软件消抖
            if (Debounce_time == 5) // 50ms消抖
            {
                if (GPIO_PIN_RESET == HAL_GPIO_ReadPin(LED_SWITCH_INPUT_GPIO_Port, LED_SWITCH_INPUT_Pin))
                {
                    myLowSpeedMotor1Clockwise(0); // 停止电机
                    // 光电信号处理
									myAppLedSwitch_Trigger();
                    debug_uart_printf("myLedSwitchIntHandle\r\n");
                    switch (myAppStr.serveFrequency)
                    {
                    case 0x00:                  // 1档：间隔6s后再启动
                        intervalTime = 6 * 100; // ms
                        break;
                    case 0x01:                  // 2档：间隔5s后再启动
                        intervalTime = 5 * 100; // ms
                        break;
                    case 0x02:                  // 3档：间隔4s后再启动
                        intervalTime = 4 * 100; // ms
                        break;
                    case 0x03:                  // 4档：间隔3s后再启动
                        intervalTime = 3 * 100; // ms
                        break;
                    case 0x04:              // 5档：间隔2.5s后再启动
                        intervalTime = 250; // ms
                        break;
                    case 0x05:                  // 6档：间隔2s后再启动
                        intervalTime = 2 * 100; // ms
                        break;
                    case 0x06:              // 7档：间隔1.5s后再启动
                        intervalTime = 150; // ms
                        break;
                    case 0x07:                  // 8档：间隔1s后再启动
                        intervalTime = 1 * 100; // ms
                        break;
                    case 0x08:            // 9档：间隔0s后再启动
                        intervalTime = 0; // ms
                        break;
                    }
                }
                driveInt.ledSwitchInt = false;
                Debounce_time = 0;
            }
        }
        if (intervalTime != -1) // 已经选择档位
        {
            intervalCnt++;
            if (intervalCnt == intervalTime) // 转盘电机启动
            {
                myLowSpeedMotor1Clockwise(80);
                intervalTime = -1;
                intervalCnt = 0;
            }
        }
    }
}
