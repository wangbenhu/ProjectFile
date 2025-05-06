#include "myLowSpeedMotor.h"
#include "main.h"
#include "stm32f1xx_it.h"
#include "usart.h"
#include "tim.h"
#include "adc.h"
#include "myBle.h"
#include "myHighSpeedMotor.h"
extern adcRead adcVol;
extern stm32f1xx_it driveInt;
/*------------------转盘减速电机驱动-------------------------------------*/
/*
*********************************************************************************************************
* 函 数 名: myLowSpeedMotor1Init
* 功能说明: 低速电机1初始化
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myLowSpeedMotor1Init(void)
{
    Timer8_LOW_SPEED_MOTOR1_PWM_Start();
}
/*
*********************************************************************************************************
* 函 数 名: myLowSpeedMotor1Clockwise
* 功能说明: 低速电机1顺时针转
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myLowSpeedMotor1Clockwise(uint16_t pwmVal)
{
    setLowSpeedMotor1_PWMCompare(LOW_SPEED_MOTOR1_PWM1_CH, pwmVal);
    setLowSpeedMotor1_PWMCompare(LOW_SPEED_MOTOR1_PWM2_CH, 0);
}

/*
*********************************************************************************************************
* 函 数 名: myLowSpeedMotor1Counterclockwise
* 功能说明: 低速电机1逆时针转
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myLowSpeedMotor1Counterclockwise(uint16_t pwmVal)
{
    setLowSpeedMotor1_PWMCompare(LOW_SPEED_MOTOR1_PWM2_CH, pwmVal);
    setLowSpeedMotor1_PWMCompare(LOW_SPEED_MOTOR1_PWM1_CH, 0);
}
/*
*********************************************************************************************************
* 函 数 名: myLowSpeedMotor1Current
* 功能说明: 低速电机1电流采样
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
float myLowSpeedMotor1Current(void)
{
    uint8_t R76 = 70; // 采样电阻=70mΩ
    static uint32_t SysCnt = 0;
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
        }
        volSamp = sum / adcSamCnt;
        adcSamCnt = 0;
        volRel = (float)(volSamp * 3.3 / 4095 - 1.24) / 8;
        curRel = (float)volRel / R76 * 1000; // 单位A
        debug_uart_printf("volSamp=%0x,volRel=%f,curRel=%f\r\n", volSamp, volRel, curRel);
				return curRel;
    }	return -999;
}
/*------------------转盘减速电机驱动-------------------------------------*/

/*------------------低速电机2驱动-------------------------------------*/
/*
*********************************************************************************************************
* 函 数 名: myLowSpeedMotor2Init
* 功能说明: 低速电机2初始化
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myLowSpeedMotor2Init(void)
{
    Timer3_LOW_SPEED_MOTOR2_PWM_Start();
}
/*
*********************************************************************************************************
* 函 数 名: myLowSpeedMotor2Clockwise
* 功能说明: 低速电机2顺时针转
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myLowSpeedMotor2Clockwise(uint16_t pwmVal)
{
    setLowSpeedMotor2_PWMCompare(LOW_SPEED_MOTOR2_PWM1_CH, pwmVal);
    setLowSpeedMotor2_PWMCompare(LOW_SPEED_MOTOR2_PWM2_CH, 0);
}

/*
*********************************************************************************************************
* 函 数 名: myLowSpeedMotor2Counterclockwise
* 功能说明: 低速电机2逆时针转
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myLowSpeedMotor2Counterclockwise(uint16_t pwmVal)
{
    setLowSpeedMotor2_PWMCompare(LOW_SPEED_MOTOR2_PWM2_CH, pwmVal);
    setLowSpeedMotor2_PWMCompare(LOW_SPEED_MOTOR2_PWM1_CH, 0);
}
/*
*********************************************************************************************************
* 函 数 名: myLowSpeedMotor2Current
* 功能说明: 低速电机2电流采样
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myLowSpeedMotor2Current(void)
{
    // uint8_t R92 = 70; // 采样电阻=70mΩ
    // static uint32_t SysCnt = 0;
    // uint8_t temp = 0;
    // uint16_t volSamp = 0; // ADC采样值
    // float volRel = 0.0;   // 实际电压值
    // float curRel = 0.0;   // 实际电流值
    // temp = api_GetSysTickCount2s();
    // if (SysCnt != temp)
    // {
    //     SysCnt = temp;
    //     ADC_Read();
    // }
    // if (adcVol.volSampleFilish == true)
    // {
    //     for (int i = 0; i < 6; i++)
    //     {
    //         debug_uart_printf("%f ", (float)adcVol.ADC_Vol[i] * 3.3 / 4095);
    //     }
    //     debug_uart_printf("\r\n");
    //     volSamp = adcVol.ADC_Vol[2];
    //     volRel = (float)(volSamp * 3.3 / 4095 - 1.24) / 8;
    //     curRel = (float)volRel / R92 * 1000; // 单位A
    //     debug_uart_printf("myLowSpeedMotor2:volSamp=%0x,volRel=%f,curRel=%f\r\n",
    //                       volSamp, volRel, curRel);
    //     adcVol.volSampleFilish = false;
    //     // myMcuToBleSendData(adcVol.ADC_Vol, 6);
    // }
}
/*
*********************************************************************************************************
* 函 数 名: read_Encoder1_Angle
* 功能说明: 读取编码器角度
* 形  参:   无
* 返 回 值: float angle 角度值
*********************************************************************************************************
*/
float read_Encoder1_Angle(void)
{
    float angle = (float)driveInt.Position1 * 359.91 / 4095;
    return angle;
}
/*
*********************************************************************************************************
* 函 数 名: myLowSpeedMotor2Positional_PID
* 功能说明: 减速电机2位置式PID算法
* 形  参:   float target 目标值
            float current 当前值
* 返 回 值: PWM占空比
*********************************************************************************************************
*/
float myLowSpeedMotor2Positional_PID(float target, float current)
{
    // PID参数
    static float Kp = 1.0;
    static float Ki = 0.01;
    static float Kd = 0.1;
    // PID变量
    static float error = 0.0;
    static float integral = 0.0;
    static float derivative = 0.0;
    static float prev_error = 0.0;
    float output = 0.0;
    error = target - current;        // 计算误差
    integral += error;               // 积分项累加
    derivative = error - prev_error; // 微分项
    prev_error = error;              // 更新上一次误差

    // 计算PID输出
    output = Kp * error + Ki * integral + Kd * derivative;

    // 限制输出范围（例如PWM占空比范围为0-100）
    if (output > 100.0)
        output = 100.0;
    if (output < -100.0)
        output = -100.0;

    return output;
}
/*------------------低速电机2驱动-------------------------------------*/

/*------------------低速电机3驱动-------------------------------------*/
/*
*********************************************************************************************************
* 函 数 名: myLowSpeedMotor3Init
* 功能说明: 低速电机3初始化
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myLowSpeedMotor3Init(void)
{
    Timer2_LOW_SPEED_MOTOR3_PWM_Start();
}
/*
*********************************************************************************************************
* 函 数 名: myLowSpeedMotor3Clockwise
* 功能说明: 低速电机3顺时针转
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myLowSpeedMotor3Clockwise(uint16_t pwmVal)
{
    setLowSpeedMotor3_PWMCompare(LOW_SPEED_MOTOR3_PWM1_CH, pwmVal);
    setLowSpeedMotor3_PWMCompare(LOW_SPEED_MOTOR3_PWM2_CH, 0);
}

/*
*********************************************************************************************************
* 函 数 名: myLowSpeedMotor3Counterclockwise
* 功能说明: 低速电机3逆时针转
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myLowSpeedMotor3Counterclockwise(uint16_t pwmVal)
{
    setLowSpeedMotor3_PWMCompare(LOW_SPEED_MOTOR3_PWM2_CH, pwmVal);
    setLowSpeedMotor3_PWMCompare(LOW_SPEED_MOTOR3_PWM1_CH, 0);
}
/*
*********************************************************************************************************
* 函 数 名: myLowSpeedMotor3Current
* 功能说明: 低速电机3电流采样
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myLowSpeedMotor3Current(void)
{
    // uint8_t R105 = 70; // 采样电阻=70mΩ
    // static uint32_t SysCnt = 0;
    // uint8_t temp = 0;
    // uint16_t volSamp = 0; // ADC采样值
    // float volRel = 0.0;   // 实际电压值
    // float curRel = 0.0;   // 实际电流值
    // temp = api_GetSysTickCount2s();
    // if (SysCnt != temp)
    // {
    //     SysCnt = temp;
    //     ADC_Read();
    // }
    // if (adcVol.volSampleFilish == true)
    // {
    //     for (int i = 0; i < 6; i++)
    //     {
    //         debug_uart_printf("%f ", (float)adcVol.ADC_Vol[i] * 3.3 / 4095);
    //     }
    //     debug_uart_printf("\r\n");
    //     volSamp = adcVol.ADC_Vol[0];
    //     volRel = (float)(volSamp * 3.3 / 4095 - 1.24) / 8;
    //     curRel = (float)volRel / R105 * 1000; // 单位A
    //     debug_uart_printf("myLowSpeedMotor3:volSamp=%0x,volRel=%f,curRel=%f\r\n",
    //                       volSamp, volRel, curRel);
    //     adcVol.volSampleFilish = false;
    //     // myMcuToBleSendData(adcVol.ADC_Vol, 6);
    // }
}
/*
*********************************************************************************************************
* 函 数 名: read_Encoder2_Angle
* 功能说明: 读取编码器2角度
* 形  参:   无
* 返 回 值: float angle 角度值
*********************************************************************************************************
*/
float read_Encoder2_Angle(void)
{
    float angle = (float)driveInt.Position2 * 359.91 / 4095;
    return angle;
}
/*
*********************************************************************************************************
* 函 数 名: myLowSpeedMotor3Positional_PID
* 功能说明: 减速电机3位置式PID算法
* 形  参:   float target 目标值
            float current 当前值
* 返 回 值: PWM占空比
*********************************************************************************************************
*/
float myLowSpeedMotor3Positional_PID(float target, float current)
{
    // PID参数
    static float Kp = 1.0;
    static float Ki = 0.01;
    static float Kd = 0.1;
    // PID变量
    static float error = 0.0;
    static float integral = 0.0;
    static float derivative = 0.0;
    static float prev_error = 0.0;
    float output = 0.0;
    error = target - current;        // 计算误差
    integral += error;               // 积分项累加
    derivative = error - prev_error; // 微分项
    prev_error = error;              // 更新上一次误差

    // 计算PID输出
    output = Kp * error + Ki * integral + Kd * derivative;

    // 限制输出范围（例如PWM占空比范围为0-100）
    if (output > 100.0)
        output = 100.0;
    if (output < -100.0)
        output = -100.0;

    return output;
}
/*------------------低速电机3驱动-------------------------------------*/
