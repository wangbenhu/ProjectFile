/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    stm32f1xx_it.c
 * @brief   Interrupt Service Routines.
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f1xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usart.h"
#include "myApp.h"
#include "myLowSpeedMotor.h"
extern TIM_HandleTypeDef htim6;
extern myApp myAppStr;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern DMA_HandleTypeDef hdma_adc1;
extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim7;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern UART_HandleTypeDef huart1;
/* USER CODE BEGIN EV */
static uint32_t SysTickCount1ms = 0; // ϵͳʱ��Ƭ������:1msΪ��λ
static uint32_t SysTickCount5ms = 0;
static uint32_t SysTickCount10ms = 0;
static uint32_t SysTickCount20ms = 0;
static uint32_t SysTickCount100ms = 0;
static uint32_t SysTickCount500ms = 0;
static uint32_t SysTickCount1s = 0;
static uint32_t SysTickCount2s = 0;
static uint32_t SysTickCount5min = 0;

stm32f1xx_it driveInt;
/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M3 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F1xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f1xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles EXTI line1 interrupt.
  */
void EXTI1_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI1_IRQn 0 */

  /* USER CODE END EXTI1_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(LED_SWITCH_INPUT_Pin);
  /* USER CODE BEGIN EXTI1_IRQn 1 */

  /* USER CODE END EXTI1_IRQn 1 */
}

/**
  * @brief This function handles EXTI line2 interrupt.
  */
void EXTI2_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI2_IRQn 0 */

  /* USER CODE END EXTI2_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(E_REFLECTOR_Pin);
  /* USER CODE BEGIN EXTI2_IRQn 1 */

  /* USER CODE END EXTI2_IRQn 1 */
}

/**
  * @brief This function handles DMA1 channel1 global interrupt.
  */
void DMA1_Channel1_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel1_IRQn 0 */

  /* USER CODE END DMA1_Channel1_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_adc1);
  /* USER CODE BEGIN DMA1_Channel1_IRQn 1 */

  /* USER CODE END DMA1_Channel1_IRQn 1 */
}

/**
  * @brief This function handles DMA1 channel5 global interrupt.
  */
void DMA1_Channel5_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel5_IRQn 0 */

  /* USER CODE END DMA1_Channel5_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart1_rx);
  /* USER CODE BEGIN DMA1_Channel5_IRQn 1 */

  /* USER CODE END DMA1_Channel5_IRQn 1 */
}

/**
  * @brief This function handles EXTI line[9:5] interrupts.
  */
void EXTI9_5_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI9_5_IRQn 0 */

  /* USER CODE END EXTI9_5_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(REMOTECONTROL_INPUT2_Pin);
  HAL_GPIO_EXTI_IRQHandler(REMOTECONTROL_INPUT1_Pin);
  HAL_GPIO_EXTI_IRQHandler(ANGLE_CODE_2_PWM_Pin);
  HAL_GPIO_EXTI_IRQHandler(ANGLE_CODE_1_PWM_Pin);
  /* USER CODE BEGIN EXTI9_5_IRQn 1 */

  /* USER CODE END EXTI9_5_IRQn 1 */
}

/**
  * @brief This function handles USART1 global interrupt.
  */
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */

  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */
  // ����ӵĺ��������������ڿ����ж�
  USER_UART_IRQHandler(&huart1);
  /* USER CODE END USART1_IRQn 1 */
}

/**
  * @brief This function handles EXTI line[15:10] interrupts.
  */
void EXTI15_10_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI15_10_IRQn 0 */

  /* USER CODE END EXTI15_10_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(REMOTECONTROL_INPUT4_Pin);
  HAL_GPIO_EXTI_IRQHandler(REMOTECONTROL_INPUT3_Pin);
  /* USER CODE BEGIN EXTI15_10_IRQn 1 */

  /* USER CODE END EXTI15_10_IRQn 1 */
}

/**
  * @brief This function handles TIM6 global interrupt.
  */
void TIM6_IRQHandler(void)
{
  /* USER CODE BEGIN TIM6_IRQn 0 */

  /* USER CODE END TIM6_IRQn 0 */
  HAL_TIM_IRQHandler(&htim6);
  /* USER CODE BEGIN TIM6_IRQn 1 */

  /* USER CODE END TIM6_IRQn 1 */
}

/**
  * @brief This function handles TIM7 global interrupt.
  */
void TIM7_IRQHandler(void)
{
  /* USER CODE BEGIN TIM7_IRQn 0 */

  /* USER CODE END TIM7_IRQn 0 */
  HAL_TIM_IRQHandler(&htim7);
  /* USER CODE BEGIN TIM7_IRQn 1 */

  /* USER CODE END TIM7_IRQn 1 */
}

/* USER CODE BEGIN 1 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *tim_baseHandle)
{
  static uint32_t STC_1MS = 0;
  static uint32_t STC_5MS = 0;
  static uint32_t STC_10mS = 0;
	  static uint32_t STC_20mS = 0;
  static uint32_t STC_100mS = 0;
  static uint32_t STC_500mS = 0;
  static uint32_t STC_1S = 0;
  static uint32_t STC_2S = 0;
  //  static uint32_t STC_1_5S = 0;
  //  static uint32_t STC_10S = 0;
  //  static uint32_t STC_30S = 0;
  static uint32_t STC_5MIN = 0;
  static uint32_t MOTOR2_100mS = 0;
  static uint32_t MOTOR3_100mS = 0;

  if (tim_baseHandle->Instance == htim7.Instance) // 1ms��ʱ��
  {

    if (1 == ++STC_1MS)
    {
      SysTickCount1ms++;
      STC_1MS = 0;
    }
    if (5 == ++STC_5MS)
    {
      SysTickCount5ms++;
      STC_5MS = 0;
    }
    if (10 == ++STC_10mS)
    {
      SysTickCount10ms++;
      STC_10mS = 0;
    }
		 if (20 == ++STC_20mS)
    {
      SysTickCount20ms++;
      STC_20mS = 0;
    }
    if (100 == ++STC_100mS)
    {
      SysTickCount100ms++;
      STC_100mS = 0;
    }
    if (500 == ++STC_500mS)
    {
      SysTickCount500ms++;
      STC_500mS = 0;
    }
    if (1000 == ++STC_1S)
    {
      SysTickCount1s++;
      STC_1S = 0;
    }
    if (2000 == ++STC_2S)
    {
      SysTickCount2s++;
      STC_2S = 0;
    }
    //    if (1200 == ++STC_1_5S)
    //    {
    //      SysTickCount1_5s++;
    //      STC_1_5S = 0;
    //    }
    //    if (10000 == ++STC_10S)
    //    {
    //      SysTickCount10s++;
    //      STC_10S = 0;
    //    }
    if (1000 * 60 * 5 == ++STC_5MIN)
    {
      SysTickCount5min++;
      STC_5MIN = 0;
    }
#if 1
    if (100 == ++MOTOR2_100mS)
    {
      MOTOR2_100mS = 0;
      float current_angle1 = 0.0;
      // ��ȡ��ǰ�Ƕ�
      current_angle1 = read_Encoder1_Angle();

      // ����PID���
      float control_output1 = myLowSpeedMotor2Positional_PID(myAppStr.verticalAngle, current_angle1);

      // ����PWM���
      if (control_output1 >= 0)
      {
        myLowSpeedMotor2Clockwise((uint16_t)control_output1); // ��ת
      }
      else
      {
        myLowSpeedMotor2Counterclockwise((uint16_t)(-control_output1)); // ��ת
      }

      // ��ӡ������Ϣ����ѡ��
//      debug_uart_printf("Target1: %.2f, Current1: %.2f, Output1: %.2f\n", myAppStr.verticalAngle, current_angle1, control_output1);
    }
#endif
#if 1
    if (100 == ++MOTOR3_100mS)
    {
      MOTOR3_100mS = 0;
      float current_angle2 = 0.0;
      // ��ȡ��ǰ�Ƕ�
      current_angle2 = read_Encoder2_Angle();

      // ����PID���
      float control_output2 = myLowSpeedMotor3Positional_PID(myAppStr.horizontalAngle, current_angle2);

      // ����PWM���
      if (control_output2 >= 0)
      {
        myLowSpeedMotor3Clockwise((uint16_t)control_output2); // ��ת
      }
      else
      {
        myLowSpeedMotor3Counterclockwise((uint16_t)(-control_output2)); // ��ת
      }
      // ��ӡ������Ϣ����ѡ��
   //   debug_uart_printf("Target2: %.2f, Current2: %.2f, Output2: %.2f\n", myAppStr.horizontalAngle, current_angle2, control_output2);
    }
#endif
  }
  if (__HAL_TIM_GET_FLAG(&htim6, TIM_FLAG_UPDATE) != RESET)
  {
    __HAL_TIM_CLEAR_FLAG(&htim6, TIM_FLAG_UPDATE); // ��������־
    driveInt.timer_overflow_count++;               // ���������1
  }
}
/**
 * @brief  Conversion complete callback in non blocking mode  ����ص�����
 * @param  htim : hadc handle
 * @retval None
 */
// void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
//{

//  uint16_t temp = 0;
//  if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4)
//  {

//    if (driveInt.flag == 0) // ������
//    {
//      driveInt.flag = 1;
//      temp = HAL_TIM_ReadCapturedValue(&htim4, TIM_CHANNEL_4);
//      // debug_uart_printf("TonLast:%0x\r\n", driveInt.TonLast);
//      if (driveInt.TonLast != 0) // ���ǵ�һ��������
//      {
//        driveInt.T = temp - driveInt.TonLast;
//        driveInt.TonLast = temp;
//        // debug_uart_printf("Ton:%0x T:%0x \r\n", driveInt.Ton, driveInt.T, driveInt.Ton X 4098 / (driveInt.T) - 1);
//        debug_uart_printf("%lf\r\n", (float)driveInt.Ton * 4098 / (driveInt.T) - 1);
//      }
//      else
//      {
//        driveInt.TonLast = temp;
//      }
//      // �л����½��ز���
//      __HAL_TIM_SET_CAPTUREPOLARITY(&htim4, TIM_CHANNEL_4, TIM_INPUTCHANNELPOLARITY_FALLING);
//    }
//    else // �½���
//    {
//      driveInt.flag = 0;
//      temp = HAL_TIM_ReadCapturedValue(&htim4, TIM_CHANNEL_4);
//      // debug_uart_printf("2:%0x\r\n", temp);
//      driveInt.Ton = temp - driveInt.TonLast;
//      // �л��������ز���
//      __HAL_TIM_SET_CAPTUREPOLARITY(&htim4, TIM_CHANNEL_4, TIM_INPUTCHANNELPOLARITY_RISING);
//    }
//  }
//}
//=============================================================================
// ��������: ��ȡϵͳʱ��Ƭ
// �����÷�: ϵͳ��������1msΪ��λѭ������
//---------------------------------------------------------
uint32_t api_GetSysTickCount1ms(void)
{
  return (SysTickCount1ms);
}
uint32_t api_GetSysTickCount5ms(void)
{
  return (SysTickCount5ms);
}
uint32_t api_GetSysTickCount20ms(void)
{
  return (SysTickCount20ms);
}
uint32_t api_GetSysTickCount10ms(void)
{
  return (SysTickCount10ms);
}
uint32_t api_GetSysTickCount100ms(void)
{
  return (SysTickCount100ms);
}
uint32_t api_GetSysTickCount500ms(void)
{
  return (SysTickCount500ms);
}
uint32_t api_GetSysTickCount1s(void)
{
  return (SysTickCount1s);
}
uint32_t api_GetSysTickCount2s(void)
{
  return (SysTickCount2s);
}
// uint32_t api_GetSysTickCount1_5s(void)
//{
//   return (SysTickCount1_5s);
// }
// uint32_t api_GetSysTickCount10s(void)
//{
//   return (SysTickCount10s);
// }
uint32_t api_GetSysTickCount5min(void)
{
  return (SysTickCount5min);
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == LED_SWITCH_INPUT_Pin)
  {
    driveInt.ledSwitchInt = true;
  }
  else if (GPIO_Pin == E_REFLECTOR_Pin)
  {
    driveInt.eReflectorInt = true;
  }
  else if (GPIO_Pin == REMOTECONTROL_INPUT1_Pin)
  {
    driveInt.remoteControlInt1 = true;
  }
  else if (GPIO_Pin == REMOTECONTROL_INPUT2_Pin)
  {
    driveInt.remoteControlInt2 = true;
  }
  else if (GPIO_Pin == REMOTECONTROL_INPUT3_Pin)
  {
    driveInt.remoteControlInt3 = true;
  }
  else if (GPIO_Pin == REMOTECONTROL_INPUT4_Pin)
  {
    driveInt.remoteControlInt4 = true;
  }
  else if (GPIO_Pin == ANGLE_CODE_1_PWM_Pin)
  {
    uint32_t current_time1 = __HAL_TIM_GET_COUNTER(&htim6) + (driveInt.timer_overflow_count << 16); // �������
    // debug_uart_printf("%d\r\n", current_time);
    if (GPIO_PIN_SET == HAL_GPIO_ReadPin(ANGLE_CODE_1_PWM_GPIO_Port, ANGLE_CODE_1_PWM_Pin))
    {
      // ������
      driveInt.Ton1 = current_time1;
      if (driveInt.Ton1 > driveInt.TonLast1)
      {
        driveInt.Period1 = driveInt.Ton1 - driveInt.TonLast1; // ����
                                                              // Position = ton X 4098 / (ton + toff) - 1//�Ƕȼ��㹫ʽ
        if (driveInt.Duty1 < driveInt.Period1)
        {
          driveInt.Position1 = (float)driveInt.Duty1 * 4098 / driveInt.Period1 - 1;
          // debug_uart_printf("%lf\r\n", driveInt.Position1);
          // debug_uart_printf("%d,%d,%lf\r\n", driveInt.Duty, driveInt.Period, (float)driveInt.Duty / driveInt.Period);
          // debug_uart_printf("%d,%d,%lf,%lf\n", driveInt.Duty, driveInt.Period, (float)driveInt.Duty / driveInt.Period, driveInt.Position1);
        }
      }
      driveInt.TonLast1 = driveInt.Ton1;
    }
    else
    {
      // �½���
      driveInt.Toff1 = current_time1;
      if (driveInt.Toff1 > driveInt.Ton1)
      {
        driveInt.Duty1 = driveInt.Toff1 - driveInt.Ton1; // ռ�ձ�
        // debug_uart_printf("%d\r\n", driveInt.Duty);
      }
    }
  }

  else if (GPIO_Pin == ANGLE_CODE_2_PWM_Pin)
  {
    uint32_t current_time2 = __HAL_TIM_GET_COUNTER(&htim6) + (driveInt.timer_overflow_count << 16); // �������
    // debug_uart_printf("%d\r\n", current_time);
    if (GPIO_PIN_SET == HAL_GPIO_ReadPin(ANGLE_CODE_2_PWM_GPIO_Port, ANGLE_CODE_2_PWM_Pin))
    {
      // ������
      driveInt.Ton2 = current_time2;
      if (driveInt.Ton2 > driveInt.TonLast2)
      {
        driveInt.Period2 = driveInt.Ton2 - driveInt.TonLast2; // ����
                                                              // Position = ton X 4098 / (ton + toff) - 1//�Ƕȼ��㹫ʽ
        if (driveInt.Duty2 < driveInt.Period2)
        {
          driveInt.Position2 = (float)driveInt.Duty2 * 4098 / driveInt.Period2 - 1;
          // debug_uart_printf("%lf\r\n", driveInt.Position1);
          // debug_uart_printf("%d,%d,%lf\r\n", driveInt.Duty2, driveInt.Period2, (float)driveInt.Duty2 / driveInt.Period2);
          // debug_uart_printf("%d,%d,%lf,%lf\n", driveInt.Duty, driveInt.Period, (float)driveInt.Duty / driveInt.Period, driveInt.Position1);
        }
      }
      driveInt.TonLast2 = driveInt.Ton2;
    }
    else
    {
      // �½���
      driveInt.Toff2 = current_time2;
      if (driveInt.Toff2 > driveInt.Ton2)
      {
        driveInt.Duty2 = driveInt.Toff2 - driveInt.Ton2; // ռ�ձ�
      }
    }
  }
}
/* USER CODE END 1 */
