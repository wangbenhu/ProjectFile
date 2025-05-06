/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    tim.h
 * @brief   This file contains all the function prototypes for
 *          the tim.c file
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __TIM_H__
#define __TIM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern TIM_HandleTypeDef htim1;

extern TIM_HandleTypeDef htim2;

extern TIM_HandleTypeDef htim3;

extern TIM_HandleTypeDef htim6;

extern TIM_HandleTypeDef htim7;

extern TIM_HandleTypeDef htim8;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_TIM1_Init(void);
void MX_TIM2_Init(void);
void MX_TIM3_Init(void);
void MX_TIM6_Init(void);
void MX_TIM7_Init(void);
void MX_TIM8_Init(void);

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* USER CODE BEGIN Prototypes */
  void Timer7_Start(void);
  void Timer6_Start(void);
  void Timer4_Input_Capture_Start(void);
  void Timer8_LOW_SPEED_MOTOR1_PWM_Start(void);
  void Timer8_LOW_SPEED_MOTOR1_PWM_Stop(void);
  void Timer3_LOW_SPEED_MOTOR2_PWM_Start(void);
  void Timer3_LOW_SPEED_MOTOR2_PWM_Stop(void);
  void Timer2_LOW_SPEED_MOTOR3_PWM_Start(void);
  void Timer2_LOW_SPEED_MOTOR3_PWM_Stop(void);
  void Timer1_HIGH_SPEED_MOTOR1_PWM_Start(void);
  void Timer1_HIGH_SPEED_MOTOR1_PWM_Stop(void);
  void Timer1_HIGH_SPEED_MOTOR2_PWM_Start(void);
  void Timer1_HIGH_SPEED_MOTOR2_PWM_Stop(void);
  void setLowSpeedMotor1_PWMCompare(uint32_t Channel, uint16_t pwmVal);
  void setLowSpeedMotor2_PWMCompare(uint32_t Channel, uint16_t pwmVal);
  void setLowSpeedMotor3_PWMCompare(uint32_t Channel, uint16_t pwmVal);
  void setHighSpeedMotor1_PWMCompare(uint32_t Channel, uint16_t pwmVal);
  void setHighSpeedMotor2_PWMCompare(uint32_t Channel, uint16_t pwmVal);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __TIM_H__ */

