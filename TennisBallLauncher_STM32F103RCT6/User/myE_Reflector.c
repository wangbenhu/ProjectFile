#include "myE_Reflector.h"
#include "main.h"
#include "stm32f1xx_it.h"
#include "usart.h"
#include "myApp.h"
extern stm32f1xx_it driveInt;

/*
*********************************************************************************************************
* 函 数 名: myE_ReflectorIntHandle
* 功能说明: 漫反射开关中断信号处理
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myE_ReflectorIntHandle(void)
{
    static uint32_t SysCnt = 0, Debounce_time = 0;
    uint8_t temp = 0;
    temp = api_GetSysTickCount10ms();
    if (SysCnt != temp)
    {
        SysCnt = temp;
        if (true == driveInt.eReflectorInt) // 漫反射开关有中断输入
        {
            Debounce_time++;        // 软件消抖
            if (Debounce_time == 5) // 50ms消抖
            {
                if (GPIO_PIN_SET == HAL_GPIO_ReadPin(E_REFLECTOR_GPIO_Port, E_REFLECTOR_Pin))
                {
                    // 漫反射信号处理
                    debug_uart_printf("myE_ReflectorIntHandle\r\n");
									myReflector_Trigger();
                }
                driveInt.eReflectorInt = false;
                Debounce_time = 0;
            }
        }
    }
}
