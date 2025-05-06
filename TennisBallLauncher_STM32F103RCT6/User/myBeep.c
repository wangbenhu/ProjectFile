#include "myBeep.h"
#include "stm32f1xx_it.h"
#include "main.h"
#include "myApp.h"
#define BEEP_UNIT_TIM 100
#define  Beep_GetSysTickCount() (api_GetSysTickCount##100##ms())
static struct
{
	uint8_t beep_mode;
	uint32_t beep_start_count;
	uint32_t beep_start_time;
	uint8_t beep_status;
}beep_variate={BEEP_CONTINUE_STOP,0,0,0};
/*
*********************************************************************************************************
* 函 数 名: myBeepOn
* 功能说明: 打开蜂鸣器
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myBeepSet(BeepModeList mode,uint32_t time)//100ms倍数
{
	if(mode==BEEP_CONTINUE_STOP||time==0)
	{
		myBeepOff();
		beep_variate.beep_start_time=0;
		beep_variate.beep_status=0;
	}
	else{	
		myBeepOn();
		beep_variate.beep_start_time=time/BEEP_UNIT_TIM;
	}
  beep_variate.beep_mode=mode;
	beep_variate.beep_start_count=0;

}
/*
*********************************************************************************************************
* 函 数 名: myBeepOn
* 功能说明: 打开蜂鸣器
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myBeepOn(void)
{
    HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_SET);
	myBeepStatusSet(1);
}

/*
*********************************************************************************************************
* 函 数 名: myBeepOff
* 功能说明: 关闭蜂鸣器
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myBeepOff(void)
{
   HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
	myBeepStatusSet(0);
}
/*
*********************************************************************************************************
* 函 数 名: myBeepTest
* 功能说明: 蜂鸣器测试
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myBeepTest(void)
{
    static uint32_t SysCnt = 0;
    static bool beepFlag = false;
    uint8_t temp = 0;
    temp = Beep_GetSysTickCount();
    if (SysCnt != temp)
    {
        SysCnt = temp;
		
        switch(beep_variate.beep_mode)
				{
					case BEEP_CONTINUE_STOP:
						break;
					case BEEP_CONTINUE://固定时间
						beep_variate.beep_start_count++;
					if(beep_variate.beep_start_count>=beep_variate.beep_start_time)
					{
						beep_variate.beep_mode=BEEP_CONTINUE_STOP;
						beep_variate.beep_start_count=0;
						beep_variate.beep_start_time=0;
						myBeepOff();
					}
						break;
					case BEEP_INTERVAL:
						beep_variate.beep_start_count++;
					if(beep_variate.beep_start_count>=beep_variate.beep_start_time)
					{
						
						if(beep_variate.beep_status)
						{
							myBeepOff();
						}
						else
						{
							myBeepOn();
						}
						beep_variate.beep_status=beep_variate.beep_status>0?0:1;
						beep_variate.beep_start_count=0;
					}
						break;
					default:
						break;
				}
    }
}
