/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BLE_KEY_Pin GPIO_PIN_13
#define BLE_KEY_GPIO_Port GPIOC
#define BLE_STATUS_Pin GPIO_PIN_14
#define BLE_STATUS_GPIO_Port GPIOC
#define AT_EN_Pin GPIO_PIN_15
#define AT_EN_GPIO_Port GPIOC
#define LED_SWITCH_EN_Pin GPIO_PIN_0
#define LED_SWITCH_EN_GPIO_Port GPIOC
#define LED_SWITCH_INPUT_Pin GPIO_PIN_1
#define LED_SWITCH_INPUT_GPIO_Port GPIOC
#define LED_SWITCH_INPUT_EXTI_IRQn EXTI1_IRQn
#define E_REFLECTOR_Pin GPIO_PIN_2
#define E_REFLECTOR_GPIO_Port GPIOC
#define E_REFLECTOR_EXTI_IRQn EXTI2_IRQn
#define IND_LED2_Pin GPIO_PIN_3
#define IND_LED2_GPIO_Port GPIOC
#define IND_LED1_Pin GPIO_PIN_0
#define IND_LED1_GPIO_Port GPIOA
#define LOW_SPEED_MOTOR3_PWM1_Pin GPIO_PIN_2
#define LOW_SPEED_MOTOR3_PWM1_GPIO_Port GPIOA
#define LOW_SPEED_MOTOR3_PWM2_Pin GPIO_PIN_3
#define LOW_SPEED_MOTOR3_PWM2_GPIO_Port GPIOA
#define LOW_SPEED_MOTOR2_PWM1_Pin GPIO_PIN_6
#define LOW_SPEED_MOTOR2_PWM1_GPIO_Port GPIOA
#define LOW_SPEED_MOTOR2_PWM2_Pin GPIO_PIN_7
#define LOW_SPEED_MOTOR2_PWM2_GPIO_Port GPIOA
#define LOW_SPEED_MOTOR1_ADC_Pin GPIO_PIN_4
#define LOW_SPEED_MOTOR1_ADC_GPIO_Port GPIOC
#define HIGH_SPEED_MOTOR2_ADC_Pin GPIO_PIN_5
#define HIGH_SPEED_MOTOR2_ADC_GPIO_Port GPIOC
#define HIGH_SPEED_MOTOR1_ADC_Pin GPIO_PIN_0
#define HIGH_SPEED_MOTOR1_ADC_GPIO_Port GPIOB
#define BEEP_Pin GPIO_PIN_2
#define BEEP_GPIO_Port GPIOB
#define DEBUG_TX_Pin GPIO_PIN_10
#define DEBUG_TX_GPIO_Port GPIOB
#define DEBUG_RX_Pin GPIO_PIN_11
#define DEBUG_RX_GPIO_Port GPIOB
#define HIGH_SPEED_MOTOR2_CD_Pin GPIO_PIN_12
#define HIGH_SPEED_MOTOR2_CD_GPIO_Port GPIOB
#define REMOTECONTROL_INPUT4_Pin GPIO_PIN_14
#define REMOTECONTROL_INPUT4_GPIO_Port GPIOB
#define REMOTECONTROL_INPUT4_EXTI_IRQn EXTI15_10_IRQn
#define REMOTECONTROL_INPUT3_Pin GPIO_PIN_15
#define REMOTECONTROL_INPUT3_GPIO_Port GPIOB
#define REMOTECONTROL_INPUT3_EXTI_IRQn EXTI15_10_IRQn
#define REMOTECONTROL_INPUT2_Pin GPIO_PIN_6
#define REMOTECONTROL_INPUT2_GPIO_Port GPIOC
#define REMOTECONTROL_INPUT2_EXTI_IRQn EXTI9_5_IRQn
#define REMOTECONTROL_INPUT1_Pin GPIO_PIN_7
#define REMOTECONTROL_INPUT1_GPIO_Port GPIOC
#define REMOTECONTROL_INPUT1_EXTI_IRQn EXTI9_5_IRQn
#define LOW_SPEED_MOTOR1_PWM1_Pin GPIO_PIN_8
#define LOW_SPEED_MOTOR1_PWM1_GPIO_Port GPIOC
#define LOW_SPEED_MOTOR1_PWM2_Pin GPIO_PIN_9
#define LOW_SPEED_MOTOR1_PWM2_GPIO_Port GPIOC
#define HIGH_SPEED_MOTOR2_PWM2_Pin GPIO_PIN_8
#define HIGH_SPEED_MOTOR2_PWM2_GPIO_Port GPIOA
#define HIGH_SPEED_MOTOR2_PWM1_Pin GPIO_PIN_9
#define HIGH_SPEED_MOTOR2_PWM1_GPIO_Port GPIOA
#define HIGH_SPEED_MOTOR1_PWM2_Pin GPIO_PIN_10
#define HIGH_SPEED_MOTOR1_PWM2_GPIO_Port GPIOA
#define HIGH_SPEED_MOTOR1_PWM1_Pin GPIO_PIN_11
#define HIGH_SPEED_MOTOR1_PWM1_GPIO_Port GPIOA
#define HIGH_SPEED_MOTOR1_CD_Pin GPIO_PIN_12
#define HIGH_SPEED_MOTOR1_CD_GPIO_Port GPIOA
#define ANGLE_CODE_2_MISO_Pin GPIO_PIN_15
#define ANGLE_CODE_2_MISO_GPIO_Port GPIOA
#define ANGLE_CODE_2_MOSI_Pin GPIO_PIN_10
#define ANGLE_CODE_2_MOSI_GPIO_Port GPIOC
#define ANGLE_CODE_2_CLK_Pin GPIO_PIN_11
#define ANGLE_CODE_2_CLK_GPIO_Port GPIOC
#define ANGLE_CODE_2_CS_Pin GPIO_PIN_12
#define ANGLE_CODE_2_CS_GPIO_Port GPIOC
#define ANGLE_CODE_1_MISO_Pin GPIO_PIN_2
#define ANGLE_CODE_1_MISO_GPIO_Port GPIOD
#define ANGLE_CODE_1_MOSI_Pin GPIO_PIN_3
#define ANGLE_CODE_1_MOSI_GPIO_Port GPIOB
#define ANGLE_CODE_1_CLK_Pin GPIO_PIN_4
#define ANGLE_CODE_1_CLK_GPIO_Port GPIOB
#define ANGLE_CODE_1_CS_Pin GPIO_PIN_5
#define ANGLE_CODE_1_CS_GPIO_Port GPIOB
#define MCU_TX_Pin GPIO_PIN_6
#define MCU_TX_GPIO_Port GPIOB
#define MCU_RX_Pin GPIO_PIN_7
#define MCU_RX_GPIO_Port GPIOB
#define ANGLE_CODE_2_PWM_Pin GPIO_PIN_8
#define ANGLE_CODE_2_PWM_GPIO_Port GPIOB
#define ANGLE_CODE_2_PWM_EXTI_IRQn EXTI9_5_IRQn
#define ANGLE_CODE_1_PWM_Pin GPIO_PIN_9
#define ANGLE_CODE_1_PWM_GPIO_Port GPIOB
#define ANGLE_CODE_1_PWM_EXTI_IRQn EXTI9_5_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
