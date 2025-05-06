/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    stm32f1xx_it.h
 * @brief   This file contains the headers of the interrupt handlers.
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
#ifndef __STM32F1xx_IT_H
#define __STM32F1xx_IT_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "stdarg.h"
#include "stdbool.h"
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
void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
void EXTI1_IRQHandler(void);
void EXTI2_IRQHandler(void);
void DMA1_Channel1_IRQHandler(void);
void DMA1_Channel5_IRQHandler(void);
void EXTI9_5_IRQHandler(void);
void USART1_IRQHandler(void);
void EXTI15_10_IRQHandler(void);
void TIM6_IRQHandler(void);
void TIM7_IRQHandler(void);
/* USER CODE BEGIN EFP */

  typedef struct
  {
    bool ledSwitchInt;
    bool eReflectorInt;
    bool remoteControlInt1;
    bool remoteControlInt2;
    bool remoteControlInt3;
    bool remoteControlInt4;

    uint16_t TonLast1;
    uint16_t ToffLast1;
    uint16_t Ton1;
    uint16_t Toff1;
    uint32_t Period1;  // ����
    uint32_t Duty1;    // ռ�ձ�
    uint8_t flagUp1;   // ������
    uint8_t flagDown1; // �½���
    float Position1;

    uint16_t TonLast2;
    uint16_t ToffLast2;
    uint16_t Ton2;
    uint16_t Toff2;
    uint32_t Period2;  // ����
    uint32_t Duty2;    // ռ�ձ�
    uint8_t flagUp2;   // ������
    uint8_t flagDown2; // �½���
    float Position2;

    uint32_t timer_overflow_count;
  } stm32f1xx_it;

  uint32_t api_GetSysTickCount1ms(void);
  uint32_t api_GetSysTickCount5ms(void);
  uint32_t api_GetSysTickCount10ms(void);
	uint32_t api_GetSysTickCount20ms(void);
  uint32_t api_GetSysTickCount100ms(void);
  uint32_t api_GetSysTickCount500ms(void);
  uint32_t api_GetSysTickCount5min(void);
  uint32_t api_GetSysTickCount1s(void);
  uint32_t api_GetSysTickCount2s(void);
/* USER CODE END EFP */

#ifdef __cplusplus
}
#endif

#endif /* __STM32F1xx_IT_H */
