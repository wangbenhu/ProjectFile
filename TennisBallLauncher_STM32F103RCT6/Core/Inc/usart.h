/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    usart.h
 * @brief   This file contains all the function prototypes for
 *          the usart.c file
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
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
#include "stdarg.h"
#include "stdbool.h"
/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;

extern UART_HandleTypeDef huart3;

/* USER CODE BEGIN Private defines */
#define DEBUG_UART_TX_BUF_SIZE 240
#define BLE_UART_TX_BUF_SIZE 240
#define BLE_UART_RX_BUF_SIZE 240
#define DMA_RX_BUF_SIZE 240
/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);
void MX_USART3_UART_Init(void);

/* USER CODE BEGIN Prototypes */

  struct ble_uart_type
  {
    uint8_t debug_uart_tx_buf[DEBUG_UART_TX_BUF_SIZE]; // DEBUG UART���ͻ���
    uint8_t ble_uart_tx_buf[BLE_UART_TX_BUF_SIZE];     // BLE UART���ͻ���
    uint8_t ble_uart_rx_buf[BLE_UART_RX_BUF_SIZE];     // BLE UART���ջ���
    uint8_t g_ble_uart_rx_len;                         // BLE UART�������ݳ���
    uint8_t dma_rx_buf[DMA_RX_BUF_SIZE];               // DMA���ջ���
    bool g_ble_rec_filish_flag;
  };

  void debug_uart_printf(char *fmt, ...);
  void USER_UART_IRQHandler(UART_HandleTypeDef *huart);
  void uartSendToBle(uint8_t *tx_buf, uint8_t len);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

