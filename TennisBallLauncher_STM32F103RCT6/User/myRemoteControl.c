#include "myRemoteControl.h"
#include "main.h"
#include "stm32f1xx_it.h"
#include "usart.h"
#include "myHighSpeedMotor.h"
extern stm32f1xx_it driveInt;
myRemoteControl_str myRemoteControl;
/*
*********************************************************************************************************
* 函 数 名: myRemoteControlIntHandle
* 功能说明: 遥控器中断信号处理
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myRemoteControlIntHandle(void)
{
    static uint32_t SysCnt = 0, Debounce_time_1 = 0, Debounce_time_2 = 0, Debounce_time_3 = 0, Debounce_time_4 = 0;
    uint8_t temp = 0, keyTemp1 = 0, keyTemp2 = 0, keyTemp3 = 0, keyTemp4 = 0;
    static uint8_t keyRecord = 0, keyScanTim = 0;
    temp = api_GetSysTickCount10ms();
    if (SysCnt != temp)
    {
        SysCnt = temp;
        if (true == driveInt.remoteControlInt1) // 有中断输入
        {
            Debounce_time_1++;        // 软件消抖
            if (Debounce_time_1 == 5) // 50ms消抖
            {
                if (GPIO_PIN_SET == HAL_GPIO_ReadPin(REMOTECONTROL_INPUT1_GPIO_Port, REMOTECONTROL_INPUT1_Pin))
                {
                    // 信号处理
                    keyTemp1 = 0x01;
                    // debug_uart_printf("myRemoteControlIntHandle 1\r\n");
                }
                driveInt.remoteControlInt1 = false;
                Debounce_time_1 = 0;
            }
        }
        if (true == driveInt.remoteControlInt2) // 有中断输入
        {
            Debounce_time_2++;        // 软件消抖
            if (Debounce_time_2 == 5) // 50ms消抖
            {
                if (GPIO_PIN_SET == HAL_GPIO_ReadPin(REMOTECONTROL_INPUT2_GPIO_Port, REMOTECONTROL_INPUT2_Pin))
                {
                    // 信号处理
                    keyTemp2 = 0x02;
                    // debug_uart_printf("myRemoteControlIntHandle 2\r\n");
                }
                driveInt.remoteControlInt2 = false;
                Debounce_time_2 = 0;
            }
        }
        if (true == driveInt.remoteControlInt3) // 有中断输入
        {
            Debounce_time_3++;        // 软件消抖
            if (Debounce_time_3 == 5) // 50ms消抖
            {
                if (GPIO_PIN_SET == HAL_GPIO_ReadPin(REMOTECONTROL_INPUT3_GPIO_Port, REMOTECONTROL_INPUT3_Pin))
                {
                    // 信号处理
                    keyTemp3 = 0x04;
                    // debug_uart_printf("myRemoteControlIntHandle 3\r\n");
                }
                driveInt.remoteControlInt3 = false;
                Debounce_time_3 = 0;
            }
        }
        if (true == driveInt.remoteControlInt4) // 有中断输入
        {
            Debounce_time_4++;        // 软件消抖
            if (Debounce_time_4 == 5) // 50ms消抖
            {
                if (GPIO_PIN_SET == HAL_GPIO_ReadPin(REMOTECONTROL_INPUT4_GPIO_Port, REMOTECONTROL_INPUT4_Pin))
                {
                    // 信号处理
                    keyTemp4 = 0x08;
                    // debug_uart_printf("myRemoteControlIntHandle 4\r\n");
                }
                driveInt.remoteControlInt4 = false;
                Debounce_time_4 = 0;
            }
        }
        if (keyTemp1 != 0 || keyTemp2 != 0 || keyTemp3 != 0 || keyTemp4 != 0)
        {
            keyRecord = keyRecord | keyTemp1 | keyTemp2 | keyTemp3 | keyTemp4;
            // debug_uart_printf("keyRecord=%0x\r\n", keyRecord);
            keyScanTim = 0;
            return;
        }
        if (keyRecord != 0)
        {
            keyScanTim++;
        }
        if (keyScanTim >= 100) // 1s后检测键值
        {
            switch (keyRecord)
            {
            case 0x08:
                myRemoteControl.keyVal = 0x01; // 1
                keyRecord = 0x00;
                keyScanTim = 0;
                // myHighSpeedMotor1Clockwise(5);
                // myHighSpeedMotor2Counterclockwise(5);
                // myLowSpeedMotor2Clockwise(0);
                debug_uart_printf("keyVal 1\r\n");
                break;
            case 0x0A:
                myRemoteControl.keyVal = 0x02; // 2
                keyRecord = 0x00;
                keyScanTim = 0;
                // myHighSpeedMotor1Clockwise(15);
                // myLowSpeedMotor2Clockwise(80);
                myHighSpeedMotor2Counterclockwise(15);
                debug_uart_printf("keyVal 2\r\n");
                break;
            case 0x04:
                myRemoteControl.keyVal = 0x03; // 3
                keyRecord = 0x00;
                keyScanTim = 0;
                // myHighSpeedMotor1Clockwise(25);
                myHighSpeedMotor2Counterclockwise(25);
                debug_uart_printf("keyVal 3\r\n");
                break;
            case 0x02:
                myRemoteControl.keyVal = 0x04; // 4
                keyRecord = 0x00;
                keyScanTim = 0;
                // myHighSpeedMotor1Clockwise(35);
                myHighSpeedMotor2Counterclockwise(35);
                debug_uart_printf("keyVal 4\r\n");
                break;
            case 0x0C:
                myRemoteControl.keyVal = 0x05; // 5
                keyRecord = 0x00;
                keyScanTim = 0;
                // myHighSpeedMotor1Clockwise(45);
                myHighSpeedMotor2Counterclockwise(45);
                debug_uart_printf("keyVal 5\r\n");
                break;
            case 0x06:
                myRemoteControl.keyVal = 0x06; // 6
                keyRecord = 0x00;
                keyScanTim = 0;
                // myHighSpeedMotor1Clockwise(55);
                myHighSpeedMotor2Counterclockwise(55);
                debug_uart_printf("keyVal 6\r\n");
                break;
            case 0x0E:
                myRemoteControl.keyVal = 0x07; // 7
                keyRecord = 0x00;
                keyScanTim = 0;
                // myHighSpeedMotor1Clockwise(65);
                myHighSpeedMotor2Counterclockwise(68);
                debug_uart_printf("keyVal 7\r\n");
                break;
            case 0x01:
                myRemoteControl.keyVal = 0x08; // 8
                keyRecord = 0x00;
                keyScanTim = 0;
                // myHighSpeedMotor1Clockwise(75);
                // myHighSpeedMotor2Counterclockwise(78);
                debug_uart_printf("keyVal 8\r\n");
                break;
            case 0x09:
                myRemoteControl.keyVal = 0x09; // 9
                keyRecord = 0x00;
                keyScanTim = 0;
                // myHighSpeedMotor1Clockwise(85);
                // myHighSpeedMotor2Counterclockwise(85);
                debug_uart_printf("keyVal 9\r\n");
                break;
            case 0x05:
                myRemoteControl.keyVal = 0x0A; // 10
                keyRecord = 0x00;
                keyScanTim = 0;
                // myHighSpeedMotor1Clockwise(95);
                // myHighSpeedMotor2Counterclockwise(95);
                debug_uart_printf("keyVal 10\r\n");
                break;
            case 0x0D:
                myRemoteControl.keyVal = 0x0B; // 11
                keyRecord = 0x00;
                keyScanTim = 0;
                // myHighSpeedMotor1Clockwise(0);
                myHighSpeedMotor2Counterclockwise(0);
                debug_uart_printf("keyVal 11\r\n");
                break;
            }
        }
    }
}
