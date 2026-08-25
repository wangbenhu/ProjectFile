/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup COMMON COMMON
 * @ingroup  DRIVER
 * @brief    COMMON driver
 * @details  COMMON driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_COMMON_H
#define __DRV_COMMON_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#include <stdint.h>
#include "om_device.h"

#ifdef  __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
/// Peripheral driver timeout max delay
#define DRV_MAX_DELAY              (0xFFFFFFFFU)
/// BIT Mask defined
#define BITMASK(n)                 (1U << (n))


/*******************************************************************************
 * TYPEDEFS
 */
/// General peripheral driver event, [0-15]： driver common event: [0-3] common error event, [4-8] common event
typedef enum {
    DRV_EVENT_COMMON_NONE                       = 0,
    DRV_EVENT_COMMON_ERROR                      = (1U << 0),               /**< Common error */
    DRV_EVENT_COMMON_ABORT                      = (1U << 1),               /**< Abort transmit/receive/read/write event */

    DRV_EVENT_COMMON_GENERAL                    = (1U << 4),               /**< used for General event */
    DRV_EVENT_COMMON_WRITE_COMPLETED            = (1U << 5),               /**< write completed, from system to peripheral direction */
    DRV_EVENT_COMMON_TRANSMIT_COMPLETED         = (1U << 5),               /**< transmit completed, from system to peripheral direction */
    DRV_EVENT_COMMON_READ_COMPLETED             = (1U << 6),               /**< read completed, from peripheral to system direction */
    DRV_EVENT_COMMON_RECEIVE_COMPLETED          = (1U << 6),               /**< read completed, from peripheral to system direction */
    DRV_EVENT_COMMON_TRANSFER_COMPLETED         = (1U << 5) + (1U << 6),   /**< read and write completed, such as SPI peripheral */
    DRV_EVENT_COMMON_RX_OVERFLOW                = (1U << 7),               /**< Received FIFO overflow event */
    DRV_EVENT_COMMON_GPDMA2PERIPH_COMPLETED     = (1U << 8),               /**< system GPDMA to peripheral completed event, from system to peripheral direction */
    /*!< PMU */
    DRV_EVENT_PMU_PIN_WAKEUP_GPIO0              = (1U << 16),
    DRV_EVENT_PMU_PIN_WAKEUP_GPIO1              = (1U << 17),
    DRV_EVENT_PMU_POF                           = (1U << 31),
    /*!< I2C */
    DRV_EVENT_I2C_TIMEOUT                       = (1U << 9),               /**< I2C timeout event */
    DRV_EVENT_I2C_TXADDR_NACK                   = (1U << 10),              /**< I2C tx addr nack event  */
    DRV_EVENT_I2C_RXADDR_NACK                   = (1U << 11),              /**< I2C rx addr nack event  */
    DRV_EVENT_I2C_TXDATA_NACK                   = (1U << 12),              /**< I2C tx data nack event  */
    DRV_EVENT_I2C_RXDATA_UNDER                  = (1U << 13),              /**< I2C rx data not exist event  */
    DRV_EVENT_I2C_STOP_DET                      = (1U << 14),              /**< I2C stop det event  */
    /*!< GPDMA */
    DRV_GPDMA_EVENT_TERMINAL_COUNT_REQUEST      = (1U << 16),
    DRV_GPDMA_EVENT_ERROR                       = (1U << 17),
    DRV_GPDMA_EVENT_ABORT                       = (1U << 18),
    /*!< UART */
    DRV_EVENT_UART_RX_TIMEOUT                   = (1U << 16),
    DRV_EVENT_UART_RX_BREAK                     = (1U << 17),
    DRV_EVENT_UART_RX_FRAME_ERROR               = (1U << 18),
    DRV_EVENT_UART_RX_PARITY_ERROR              = (1U << 19),
    DRV_EVENT_UART_CTS                          = (1U << 20),
    DRV_EVENT_UART_DSR                          = (1U << 21),
    DRV_EVENT_UART_DCD                          = (1U << 22),
    DRV_EVENT_UART_RI                           = (1U << 23),
    DRV_EVENT_UART_LIN_BREAK_DETECT             = (1U << 24),
    DRV_EVENT_UART_BUSY_DETECT                  = (1U << 25),
    /*!< TIMER */
    DRV_EVENT_TIM_UPDATE                        = (1U << 16),
    DRV_EVENT_TIM_CC1                           = (1U << 17),
    DRV_EVENT_TIM_CC2                           = (1U << 18),
    DRV_EVENT_TIM_CC3                           = (1U << 19),
    DRV_EVENT_TIM_CC4                           = (1U << 20),
    DRV_EVENT_TIM_TRIGGER                       = (1U << 21),
    DRV_EVENT_TIM_CC1_OVERFLOW                  = (1U << 24),
    DRV_EVENT_TIM_CC2_OVERFLOW                  = (1U << 25),
    DRV_EVENT_TIM_CC3_OVERFLOW                  = (1U << 26),
    DRV_EVENT_TIM_CC4_OVERFLOW                  = (1U << 27),
    DRV_EVENT_TIM_GPDMA_COMPLETE                = (1U << 28),
    /*!< LP TIMER */
    DRV_EVENT_LPTIM_UNDER_FLOW                  = (1U << 16),
    DRV_EVENT_LPTIM_COMP0                       = (1U << 17),
    DRV_EVENT_LPTIM_COMP1                       = (1U << 18),
    DRV_EVENT_LPTIM_REP0                        = (1U << 19),
    DRV_EVENT_LPTIM_REP1                        = (1U << 20),
    DRV_EVENT_LPTIM_COMP2                       = (1U << 21),
    DRV_EVENT_LPTIM_COMP3                       = (1U << 22),
    DRV_EVENT_LPTIM_REP2                        = (1U << 23),
    DRV_EVENT_LPTIM_REP3                        = (1U << 24),
    /*!< OM24G */
    DRV_EVENT_OM24G_RX_TM                       = (1U << 16),
    DRV_EVENT_OM24G_RX_OVERLAY                  = (1U << 17),
    DRV_EVENT_OM24G_MAX_RT                      = (1U << 18),
    DRV_EVENT_OM24G_INT_TIMER0                  = (1U << 19),
    DRV_EVENT_OM24G_INT_TIMER1                  = (1U << 20),
    DRV_EVENT_OM24G_SYNC                        = (1U << 21),
    /*! IRTX */
    DRV_EVENT_IRTX_PWM_INT_PNUM_INT             = (1U << 0),
    DRV_EVENT_IRTX_PWM_INT_DMA_INT              = (1U << 1),
    DRV_EVENT_IRTX_PWM_INT_CYCLE_DONE_INT       = (1U << 2),
    DRV_EVENT_IRTX_FIFO_CNT                     = (1U << 3),
    DRV_EVENT_IRTX_FIFO_EMPTY_INT               = (1U << 4),
    /*! RTC */
    DRV_EVENT_RTC_ALARM0                        = (1U << 16),
    DRV_EVENT_RTC_ALARM1                        = (1U << 17),
    DRV_EVENT_RTC_ALARM2                        = (1U << 18),
    DRV_EVENT_RTC_SECOND                        = (1U << 19),
    /*! QDEC */
    DRV_EVENT_QDEC_SAMPLERDY                    = (1U << 16),
    DRV_EVENT_QDEC_REPORTRDY                    = (1U << 17),
    DRV_EVENT_QDEC_ACCOF                        = (1U << 18),
    DRV_EVENT_QDEC_DBLRDY                       = (1U << 19),
    DRV_EVENT_QDEC_STOPPED                      = (1U << 20),
    /*! OSPI */
    DRV_EVENT_OSPI_LIST_NODE_COMPLETED          = (1U << 16),
    DRV_EVENT_OSPI_LIST_COMPLETED               = (1U << 17),
    /*! FLASH */
    DRV_EVENT_FLASH_WRITE_TRANSFER_COMPLETED    = (1U << 16),               /**< Flash write command transfer completed event, but flash is not programed, so polling wip is required  */
    DRV_EVENT_FLASH_LIST_NODE_COMPLETED         = (1U << 17),
    DRV_EVENT_FLASH_LIST_COMPLETED              = (1U << 18),
    /*! PSRAM */
    DRV_EVENT_PSRAM_LIST_NODE_COMPLETED         = (1U << 16),
    DRV_EVENT_PSRAM_LIST_COMPLETED              = (1U << 17),
} drv_event_t;

/**
 * @brief general driver isr event callback prototype
 *
 * @param[in] om_reg: pointer to peripheral registers base(GPDMA channel base for dma peripheral)
 * @param[in] event:  driver event
 * @param[in] param0: refer to sepcific scenarios
 * @param[in] param1: refer to sepcific scenarios
 */
typedef void (*drv_isr_callback_t)(void *om_reg, drv_event_t event, void *param0, void *param1);

/**
 * @brief Peripheral state
 */
typedef enum {
    /* common state for all peripheral */
    DRV_STATE_UNINIT   = 0U,             /**< uninit state */
    DRV_STATE_INIT     = (1U << 0),      /**< init state */

    /* DRV_STATE_TX/RX used for general peripheral, for example: UART, I2C, SPI etc. */
    DRV_STATE_TX       = (1U << 2),      /**< Tx state */
    DRV_STATE_RX       = (1U << 3),      /**< rx state */

    /* DRV_STATE_START/CONTINUE/STOP used for stream peripheral, for example: AES, HASH etc. */
    DRV_STATE_START    = (1U << 4),      /**< Start state */
    DRV_STATE_CONTINUE = (1U << 5),      /**< Continue state */
    DRV_STATE_STOP     = (1U << 6),      /**< Stop state */

    DRV_STATE_MASK     = 0xFFU,          /**< used for reset and clear all state flag */
} drv_state_t;

/* common GPDMA channel for all peripheral */
typedef enum {
    DRV_GPDMA_TX_CHAN  = (1U << 0),                                /**< Tx GPDMA channel */
    DRV_GPDMA_RX_CHAN  = (1U << 1),                                /**< Rx GPDMA channel */
    DRV_GPDMA_CHAN_ALL = (DRV_GPDMA_TX_CHAN | DRV_GPDMA_RX_CHAN),  /**< Tx/Rx GPDMA channel */
} drv_gpdma_chan_t;

/// General peripheral driver environment
typedef struct {
    drv_isr_callback_t           isr_cb;          /**< event callback                      */
    uint16_t                     tx_num;          /**< Total number of data to be send     */
    uint16_t                     tx_cnt;          /**< Count of data sent*/
    uint8_t                     *tx_buf;          /**< Pointer to out data buffer, system to peripheral direction */
    uint16_t                     rx_num;          /**< Total number of data to be received, peripheral to system direction */
    uint16_t                     rx_cnt;          /**< Count of data received, peripheral to system direction */
    uint8_t                     *rx_buf;          /**< Pointer to in data buffer            */
    union {
        volatile uint8_t         state;           /**< Indicate device busy or not          */
        volatile uint8_t         busy;            /* deprecated */
    };
    uint8_t                      gpdma_tx_chan;   /**< peripheral dma tx channel, from system to peripheral direction */
    uint8_t                      gpdma_rx_chan;   /**< peripheral dma rx channel, from peripheral to system direction */
} drv_env_t;

/// GPDMA info for driver
typedef struct {
    uint8_t                      id;                /**< dma_id         */
    uint8_t                      prio;              /**< dma_priority_t */
} drv_gpdma_t;

/// Peripheral resource description
typedef struct {
    uint32_t                     cap;               /**< capabilities               */
    void                        *reg;               /**< peripheral registers base  */
    void                        *env;               /**< peripheral environment     */
    IRQn_Type                    irq_num;           /**< peripheral IRQn_Type       */
    uint8_t                      irq_prio;          /**< peripheral irq priority    */
    drv_gpdma_t                  gpdma_tx;          /**< peripheral dma tx info, from system to peripheral direction */
    drv_gpdma_t                  gpdma_rx;          /**< peripheral dma rx info, form peripheral to system direction */
} drv_resource_t;

/*
 * Used to define drv_env_t and drv_resource_t structures
 */
#define DRV_DEFINE(NAMEn, namen)                                               \
static drv_env_t namen##_env = {                                               \
    .isr_cb = NULL,                                                            \
    .tx_num = 0,                                                               \
    .tx_cnt = 0,                                                               \
    .tx_buf = NULL,                                                            \
    .rx_num = 0,                                                               \
    .rx_cnt = 0,                                                               \
    .rx_buf = NULL,                                                            \
    .state  = DRV_STATE_UNINIT,                                                \
    .gpdma_tx_chan = GPDMA_NUMBER_OF_CHANNELS,                                 \
    .gpdma_rx_chan = GPDMA_NUMBER_OF_CHANNELS,                                 \
};                                                                             \
static const drv_resource_t namen##_resource = {                               \
    .cap      = CAP_##NAMEn,                                                   \
    .reg      = OM_##NAMEn,                                                    \
    .env      = (void *)&namen##_env,                                          \
    .irq_num  = NAMEn##_IRQn,                                                  \
    .irq_prio = RTE_##NAMEn##_IRQ_PRIORITY,                                    \
    .gpdma_tx = {                                                              \
        .id   = GPDMA_ID_##NAMEn##_TX,                                         \
        .prio = 0,                                                             \
    },                                                                         \
    .gpdma_rx = {                                                              \
        .id   = GPDMA_ID_##NAMEn##_RX,                                         \
        .prio = 0,                                                             \
    },                                                                         \
}

#ifdef __cplusplus
}
#endif

#endif  /* __DRV_COMMON_H */


/** @} */
