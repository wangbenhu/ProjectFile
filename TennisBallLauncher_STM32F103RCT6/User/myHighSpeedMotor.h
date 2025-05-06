#ifndef __MYHIGHSPEEDMOTOR_H__
#define __MYHIGHSPEEDMOTOR_H__

#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "stdarg.h"
#include "stdbool.h"

#define HIGH_SPEED_MOTOR1_PWM1_CH TIM_CHANNEL_4
#define HIGH_SPEED_MOTOR1_PWM2_CH TIM_CHANNEL_3
#define HIGH_SPEED_MOTOR1_CD_H 1 // “1”允许LO、HO随IN输入控制
#define HIGH_SPEED_MOTOR1_CD_L 0 // “0”强行使LO、HO输出低电平

#define HIGH_SPEED_MOTOR2_PWM1_CH TIM_CHANNEL_2
#define HIGH_SPEED_MOTOR2_PWM2_CH TIM_CHANNEL_1
#define HIGH_SPEED_MOTOR2_CD_H 1 // “1”允许LO、HO随IN输入控制
#define HIGH_SPEED_MOTOR2_CD_L 0 // “0”强行使LO、HO输出低电平

#define MOTOR_PWMVAL 1

void myHighSpeedMotor1Init(void);
void myHighSpeedMotor1Clockwise(uint16_t pwmVal);
void myHighSpeedMotor1Counterclockwise(uint16_t pwmVal);
float myHighSpeedMotor1Current(void);
void myHighSpeedMotor2Init(void);
void myHighSpeedMotor2Clockwise(uint16_t pwmVal);
void myHighSpeedMotor2Counterclockwise(uint16_t pwmVal);
float myHighSpeedMotor2Current(void);
void movingAverageFilter(uint16_t input[], uint16_t output[], uint8_t dataSize, uint8_t N);
#endif
