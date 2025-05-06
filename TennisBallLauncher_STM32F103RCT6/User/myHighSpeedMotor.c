#include "myHighSpeedMotor.h"
#include "main.h"
#include "stm32f1xx_it.h"
#include "usart.h"
#include "tim.h"
#include "adc.h"
#include "myBle.h"

extern adcRead adcVol;
/*
*********************************************************************************************************
* 函 数 名: movingAverageFilter
* 功能说明: 滑动滤波算法
* 形  参:   input[]：待滤波数据
            output[]:滤波之后数据
            dataSize：数据个数
            N：窗口大小
* 返 回 值: 无
*********************************************************************************************************
*/
void movingAverageFilter(uint16_t input[], uint16_t output[], uint8_t dataSize, uint8_t N)
{
    uint32_t sum = 0;

    // 初始化前N个输出数据
    for (uint8_t i = 0; i < N; ++i)
    {
        sum += input[i];
        output[i] = sum / (i + 1);
    }

    // 移动平均滤波
    for (uint8_t i = N; i < dataSize; ++i)
    {
        sum = sum - input[i - N] + input[i];
        output[i] = sum / N;
    }
}
/*------------------高速电机1驱动-------------------------------------*/
/*
*********************************************************************************************************
* 函 数 名: myHighSpeedMotor1Init
* 功能说明: 高速电机1初始化
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myHighSpeedMotor1Init(void)
{
    Timer1_HIGH_SPEED_MOTOR1_PWM_Start();
    HAL_GPIO_WritePin(HIGH_SPEED_MOTOR1_CD_GPIO_Port,
                      HIGH_SPEED_MOTOR1_CD_Pin, GPIO_PIN_SET); // “1”允许LO、HO随IN输入控制
}
/*
*********************************************************************************************************
* 函 数 名: myHighSpeedMotor1Clockwise
* 功能说明: 高速电机1顺时针转
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myHighSpeedMotor1Clockwise(uint16_t pwmVal)
{
    static uint16_t pwmValLast = 45; // 电机启动最小占空比
    uint16_t temp = 0;
    setHighSpeedMotor1_PWMCompare(HIGH_SPEED_MOTOR1_PWM2_CH, 0);
    if (pwmValLast < pwmVal) // 电机启动
    {
        temp = pwmValLast;
        while (temp <= pwmVal) // 缓慢启动
        {
            setHighSpeedMotor1_PWMCompare(HIGH_SPEED_MOTOR1_PWM1_CH, temp);
            temp = temp + MOTOR_PWMVAL;
            HAL_Delay(2000);
        }
        pwmValLast = pwmVal;
    }
    else // 电机减速
    {
        temp = pwmValLast;
        while (temp >= pwmVal)
        {
            temp = temp - 5;
            setHighSpeedMotor1_PWMCompare(HIGH_SPEED_MOTOR1_PWM1_CH, temp);
            HAL_Delay(2000);
        }
        pwmValLast = pwmVal;
    }
}

/*
*********************************************************************************************************
* 函 数 名: myHighSpeedMotor1Counterclockwise
* 功能说明: 高速电机1逆时针转
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myHighSpeedMotor1Counterclockwise(uint16_t pwmVal)
{

    static uint16_t pwmValLast = 45; // 电机启动最小占空比
    uint16_t temp = 0;
    setHighSpeedMotor1_PWMCompare(HIGH_SPEED_MOTOR1_PWM1_CH, 0);
    if (pwmValLast < pwmVal) // 电机启动
    {
        temp = pwmValLast;
        while (temp <= pwmVal) // 缓慢启动
        {
            setHighSpeedMotor1_PWMCompare(HIGH_SPEED_MOTOR1_PWM2_CH, temp);
            temp = temp + MOTOR_PWMVAL;
            HAL_Delay(2000);
        }
        pwmValLast = pwmVal;
    }
    else // 电机减速
    {
        temp = pwmValLast;
        while (temp >= pwmVal)
        {
            temp = temp - 5;
            setHighSpeedMotor1_PWMCompare(HIGH_SPEED_MOTOR1_PWM2_CH, temp);
            HAL_Delay(2000);
        }
        pwmValLast = pwmVal;
    }
}

/*
*********************************************************************************************************
* 函 数 名: myHighSpeedMotor1Current
* 功能说明: 高速电机1电流采样
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
float myHighSpeedMotor1Current(void)
{
    uint8_t R41 = 5; // 采样电阻=5mΩ
    float compensationVal = 0.013468;
    static uint32_t SysCnt = 0;
    static bool adcflag = false;
    uint8_t temp = 0;
    static uint16_t input[10], adcSamCnt = 0; // ADC采样值
    uint16_t volSamp = 0;
    float volRel = 0.0; // 实际电压值
    float curRel = 0.0; // 实际电流值
    uint16_t output[10], N = 3, sum = 0;
    temp = api_GetSysTickCount10ms();
    if (SysCnt != temp)
    {
        SysCnt = temp;

        if (adcSamCnt < 10)
        {
            ADC_Read();
            input[adcSamCnt++] = adcVol.ADC_Vol[0];
            return -999;
        }
        movingAverageFilter(input, output, adcSamCnt, N);
        for (uint8_t i = 0; i < adcSamCnt; ++i)
        {
            sum = sum + output[i];
            // OM_LOG_DEBUG("%d ", output[i]);
        }
        volSamp = sum / adcSamCnt;
        adcSamCnt = 0;
        volRel = (float)(volSamp * 3.3 / 4095 - 1.24 + compensationVal) / 8;
        curRel = (float)volRel / R41 * 1000; // 单位A
        // debug_uart_printf("myHighSpeedMotor1:volSamp=%f,volRel=%f,curRel=%f\r\n",
        //                   (float)volSamp * 3.3 / 4095, volRel, curRel);
        debug_uart_printf("%lf\n", curRel);
				return curRel;
    }	return -999;
}
/*------------------高速电机1驱动-------------------------------------*/
/*------------------高速电机2驱动-------------------------------------*/
/*
*********************************************************************************************************
* 函 数 名: myHighSpeedMotor2Init
* 功能说明: 高速电机2初始化
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myHighSpeedMotor2Init(void)
{
    Timer1_HIGH_SPEED_MOTOR2_PWM_Start();
    HAL_GPIO_WritePin(HIGH_SPEED_MOTOR2_CD_GPIO_Port,
                      HIGH_SPEED_MOTOR2_CD_Pin, GPIO_PIN_SET); // “1”允许LO、HO随IN输入控制
}
/*
*********************************************************************************************************
* 函 数 名: myHighSpeedMotor2Clockwise
* 功能说明: 高速电机2顺时针转
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myHighSpeedMotor2Clockwise(uint16_t pwmVal)
{
    static uint16_t pwmValLast = 45; // 电机启动最小占空比
    uint16_t temp = 0;
    setHighSpeedMotor2_PWMCompare(HIGH_SPEED_MOTOR2_PWM2_CH, 0);
    if (pwmValLast < pwmVal) // 电机启动
    {
        temp = pwmValLast;
        while (temp <= pwmVal) // 缓慢启动
        {
            setHighSpeedMotor2_PWMCompare(HIGH_SPEED_MOTOR2_PWM1_CH, temp);
            temp = temp + MOTOR_PWMVAL;
            HAL_Delay(2000);
        }
        pwmValLast = pwmVal;
    }
    else // 电机减速
    {
        temp = pwmValLast;
        while (temp >= pwmVal)
        {
            temp = temp - 5;
            setHighSpeedMotor2_PWMCompare(HIGH_SPEED_MOTOR2_PWM1_CH, temp);
            HAL_Delay(2000);
        }
        pwmValLast = pwmVal;
    }
}

/*
*********************************************************************************************************
* 函 数 名: myHighSpeedMotor2Counterclockwise
* 功能说明: 高速电机2逆时针转
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myHighSpeedMotor2Counterclockwise(uint16_t pwmVal)
{
    static uint16_t pwmValLast = 45; // 电机启动最小占空比
    uint16_t temp = 0;
    setHighSpeedMotor2_PWMCompare(HIGH_SPEED_MOTOR2_PWM1_CH, 0);
    if (pwmValLast < pwmVal) // 电机启动
    {
        temp = pwmValLast;
        while (temp <= pwmVal) // 缓慢启动
        {
            setHighSpeedMotor2_PWMCompare(HIGH_SPEED_MOTOR2_PWM2_CH, temp);
            temp = temp + MOTOR_PWMVAL;
            HAL_Delay(500);
        }
        pwmValLast = pwmVal;
    }
    else // 电机减速
    {
        temp = pwmValLast;
        while (temp >= pwmVal)
        {
            temp = temp - 5;
            setHighSpeedMotor2_PWMCompare(HIGH_SPEED_MOTOR2_PWM2_CH, temp);
            HAL_Delay(2000);
        }
        pwmValLast = pwmVal;
    }
}
/*
*********************************************************************************************************
* 函 数 名: myHighSpeedMotor2Current
* 功能说明: 高速电机2电流采样
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
float myHighSpeedMotor2Current(void)
{
    uint8_t R58 = 5; // 采样电阻=5mΩ
    float compensationVal = 0.013468;
    static uint32_t SysCnt = 0;
    static bool adcflag = false;
    uint8_t temp = 0;
    static uint16_t input[10], adcSamCnt = 0; // ADC采样值
    uint16_t volSamp = 0;
    float volRel = 0.0; // 实际电压值
    float curRel = 0.0; // 实际电流值
    uint16_t output[10], N = 3, sum = 0;
    temp = api_GetSysTickCount10ms();
    if (SysCnt != temp)
    {
        SysCnt = temp;

        if (adcSamCnt < 10)
        {
            ADC_Read();
            input[adcSamCnt++] = adcVol.ADC_Vol[1];
              return -999;
        }
        movingAverageFilter(input, output, adcSamCnt, N);
        for (uint8_t i = 0; i < adcSamCnt; ++i)
        {
            sum = sum + output[i];
            // OM_LOG_DEBUG("%d ", output[i]);
        }
        volSamp = sum / adcSamCnt;
        adcSamCnt = 0;
        volRel = (float)(volSamp * 3.3 / 4095 - 1.24 + compensationVal) / 8;
        curRel = (float)volRel / R58 * 1000; // 单位A
        // debug_uart_printf("myHighSpeedMotor1:volSamp=%f,volRel=%f,curRel=%f\r\n",
        //                   (float)volSamp * 3.3 / 4095, volRel, curRel);
				return curRel;
       // debug_uart_printf("%lf\n", curRel);
    }
		return -999;
}
/*------------------高速电机1驱动-------------------------------------*/
