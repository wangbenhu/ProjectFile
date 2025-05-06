#ifndef __MYLOWSPEEDMOTOR_H__
#define __MYLOWSPEEDMOTOR_H__

#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "stdarg.h"
#include "stdbool.h"

#define LOW_SPEED_MOTOR1_PWM1_CH TIM_CHANNEL_3
#define LOW_SPEED_MOTOR1_PWM2_CH TIM_CHANNEL_4
#define LOW_SPEED_MOTOR2_PWM1_CH TIM_CHANNEL_1
#define LOW_SPEED_MOTOR2_PWM2_CH TIM_CHANNEL_2
#define LOW_SPEED_MOTOR3_PWM1_CH TIM_CHANNEL_3
#define LOW_SPEED_MOTOR3_PWM2_CH TIM_CHANNEL_4

void myLowSpeedMotor1Init(void);
void myLowSpeedMotor1Clockwise(uint16_t pwmVal);
void myLowSpeedMotor1Counterclockwise(uint16_t pwmVal);
float myLowSpeedMotor1Current(void);

void myLowSpeedMotor2Init(void);
void myLowSpeedMotor2Clockwise(uint16_t pwmVal);
void myLowSpeedMotor2Counterclockwise(uint16_t pwmVal);
void myLowSpeedMotor2Current(void);
float read_Encoder1_Angle(void);
float myLowSpeedMotor2Positional_PID(float target, float current);

void myLowSpeedMotor3Init(void);
void myLowSpeedMotor3Clockwise(uint16_t pwmVal);
void myLowSpeedMotor3Counterclockwise(uint16_t pwmVal);
void myLowSpeedMotor3Current(void);
float read_Encoder2_Angle(void);
float myLowSpeedMotor3Positional_PID(float target, float current);
#endif
