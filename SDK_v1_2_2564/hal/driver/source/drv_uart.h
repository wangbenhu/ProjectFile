/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup UART UART
 * @ingroup  DRIVER
 * @brief    UART driver
 * @details  UART driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_UART_H
#define __DRV_UART_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_UART0 || RTE_UART1 || RTE_UART2)
#include <stdint.h>
#include "om_driver.h"

#ifdef  __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * TYPEDEFS
 */
/// UART Hardware Flow Control
typedef enum {
    /// No hardware control
    UART_FLOW_CONTROL_NONE    = 0,
    /// Request and Clear To Send
    UART_FLOW_CONTROL_RTS_CTS = 1,
} uart_flow_control_t;

/// UART Number of Stop Bits
typedef enum {
    /// UART frame with 1 stop bit
    UART_STOP_BIT_1   = 0,
    /// UART frame with 2 stop bit
    UART_STOP_BIT_2   = 1,
    /// UART frame with 1.5 stop bit
    UART_STOP_BIT_1_5 = 2,
} uart_stop_bit_t;

/// UART Number of Data Bits
typedef enum {
    /// 5-bit long UART frame
    UART_DATA_BIT_5   = 0,
    /// 6-bit long UART frame
    UART_DATA_BIT_6   = 1,
    /// 7-bit long UART frame
    UART_DATA_BIT_7   = 2,
    /// 8-bit long UART frame
    UART_DATA_BIT_8   = 3,
    /// 9-bit long LIN frame, only valid for LIN mode
    UART_DATA_BIT_9   = 4,
} uart_data_bit_t;

/// UART Parity
typedef enum {
    /// No parity
    UART_PARITY_NONE  = 0,
    /// Odd parity
    UART_PARITY_ODD   = 1,
    /// Even parity
    UART_PARITY_EVEN  = 2,
} uart_parity_t;

typedef enum {
    UART_CONTROL_GET_IS_BUSY,              /*!< Uart get busy status */
} uart_control_t;

/// UART config
typedef struct {
    uint32_t               baudrate;       /**< Configures UART communication baud rate. */
    uart_flow_control_t    flow_control;   /**< Specifies whether the hardware flow control is enabled or disabled. */
    uart_data_bit_t        data_bit;       /**< Specifies the number of data bits transmitted or received in a frame. */
    uart_stop_bit_t        stop_bit;       /**< Specifies the number of stop bits transmitted. */
    uart_parity_t          parity;         /**< Specifies the parity mode. */
    uint8_t                half_duplex_en; /**< half-duplex enable */
    uint8_t                lin_enable;     /**< LIN mode enable */
} uart_config_t;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief UART initialization
 *
 * @param[in] om_uart       Pointer to UART
 * @param[in] uart_cfg      Configuration for uart
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_uart_init(OM_UART_Type *om_uart, const uart_config_t *uart_cfg);

/**
 *******************************************************************************
 * @brief Modified baudrate
 *
 * @param om_uart       Pointer to UART
 * @param baudrate      baudrate
 *
 * @return errno        OM_ERROR_OK indicates modified successed, others is failed
 *******************************************************************************
 */
extern om_error_t drv_uart_set_baudrate(OM_UART_Type *om_uart, uint32_t baudrate);

/**
 *******************************************************************************
 * @brief UART uninitialize, gate clock
 *
 * @param[in] om_uart    Pointer to UART
 *******************************************************************************
 */
extern void drv_uart_uninit(OM_UART_Type *om_uart);

#if (RTE_UART_REGISTER_CALLBACK)
/**
 *******************************************************************************
 * @brief Register event callback for transmit/receive by interrupt & dma mode
 *
 * @param[in] om_uart       Pointer to UART
 * @param[in] cb            Pointer to callback
 *******************************************************************************
 */
extern void drv_uart_register_isr_callback(OM_UART_Type *om_uart, drv_isr_callback_t cb);
#endif

/**
 *******************************************************************************
 * @brief The interrupt callback for UART driver. It is a weak function. User should define
 *        their own callback in user file, other than modify it in the UART driver.
 *
 * @param om_uart          The UART device address
 * @param event            The driver uart event
 *                           - DRV_EVENT_COMMON_WRITE_COMPLETED
 *                           - DRV_EVENT_COMMON_READ_COMPLETED
 *                           - DRV_EVENT_UART_RX_TIMEOUT
 *                           - DRV_EVENT_COMMON_RX_OVERFLOW
 *                           - DRV_EVENT_UART_RX_PARITY_ERROR
 *                           - DRV_EVENT_UART_RX_BREAK
 *                           - DRV_EVENT_UART_RX_FRAME_ERROR
 * @param data              The data pointer of data to be read or write
 * @param num               The data buffer valid data count
 *******************************************************************************
 */
extern void drv_uart_isr_callback(OM_UART_Type *om_uart, drv_event_t event, uint8_t *data, uint32_t num);

/**
 *******************************************************************************
 * @brief UART send sync break filed when work in LIN mode
 *
 * @param[in] om_uart       Pointer to UART
 *******************************************************************************
 */
void drv_uart_lin_send_break(OM_UART_Type *om_uart);

/**
 *******************************************************************************
 * @brief Transmit number of bytes from UART by block mode
 *
 * @param[in] om_uart       Pointer to UART
 * @param[in] data          Pointer to data buffer
 * @param[in] num           Number of data bytes to be sent
 * @param[in] timeout_ms    timeout(ms), maybe used for flow control
 *
 * @return write bytes
 *******************************************************************************
 */
extern uint16_t drv_uart_write(OM_UART_Type *om_uart, const uint8_t *data, uint16_t num, uint32_t timeout_ms);

/**
 *******************************************************************************
 * @brief Transmit number of bytes from UART by interrupt mode
 *
 * @param[in] om_uart    Pointer to UART
 * @param[in] data       Pointer to data buffer
 * @param[in] num        Number of data bytes to be sent
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_uart_write_int(OM_UART_Type *om_uart, const uint8_t *data, uint16_t num);

#if (RTE_GPDMA)
/**
 *******************************************************************************
 * @brief Allocate uart dma channel
 *
 * @param[in] om_uart    Pointer to uart
 * @param[in] channel    UART rx/tx channel
 *
 * @return errno
 *******************************************************************************
 */
om_error_t drv_uart_gpdma_channel_allocate(OM_UART_Type *om_uart, drv_gpdma_chan_t channel);

/**
 *******************************************************************************
 * @brief Release uart dma channel
 *
 * @param[in] om_uart    Pointer to uart
 * @param[in] channel    UART rx/tx channel
 *
 * @return errno
 *******************************************************************************
 */
om_error_t drv_uart_gpdma_channel_release(OM_UART_Type *om_uart, drv_gpdma_chan_t channel);

/**
 *******************************************************************************
 * @brief Transmit number of bytes from UART by DMA mode
 *
 * @param[in] om_uart       Pointer to UART
 * @param[in] data          Pointer to data buffer
 * @param[in] num           Number of data bytes to be sent
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_uart_write_dma(OM_UART_Type *om_uart, const uint8_t *data, uint16_t num);
#endif

/**
 *******************************************************************************
 * @brief Abort uart writing
 *
 * @param[in] om_uart   Pointer to UART
 *******************************************************************************
 */
extern void drv_uart_abort_write(OM_UART_Type *om_uart);

/**
 *******************************************************************************
 * @brief get write count for interrupt/gpdma mode
 *
 * @param[in] om_uart   Pointer to UART
 *
 * @return  write count
 *******************************************************************************
 */
extern uint16_t drv_uart_get_write_count(OM_UART_Type *om_uart);

/**
 *******************************************************************************
 * @brief Prepare receive number of bytes by block mode
 *
 * @param[in] om_uart       Pointer to UART
 * @param[in] data          Pointer to data buffer
 * @param[in] num           Number of data bytes to be received
 * @param[in] timeout_ms    time out(ms)
 *
 * @return Number of data bytes from UART
 *******************************************************************************
 */
extern uint16_t drv_uart_read(OM_UART_Type *om_uart, uint8_t *data, uint16_t num, uint32_t timeout_ms);

/**
 *******************************************************************************
 * @brief Prepare receive number of bytes by interrupt mode, if num is 0, uart would
 *        receive data until drv_uart_control with UART_CONTROL_ABORT_RECEIVE is called.
 *
 * @param[in] om_uart       Pointer to UART
 * @param[in] data          Pointer to data buffer
 * @param[in] num           Number of data bytes to be received
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_uart_read_int(OM_UART_Type *om_uart, uint8_t *data, uint16_t num);

#if (RTE_GPDMA)
/**
 *******************************************************************************
 * @brief Prepare receive number of bytes by DMA mode
 *
 * @param[in] om_uart       Pointer to UART
 * @param[in] data          Pointer to data buffer
 * @param[in] num           Number of data bytes to be received
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_uart_read_dma(OM_UART_Type *om_uart, uint8_t *data, uint16_t num);
#endif

/**
 *******************************************************************************
 * @brief Abort uart reading
 *
 * @param[in] om_uart   Pointer to UART
 *******************************************************************************
 */
extern void drv_uart_abort_read(OM_UART_Type *om_uart);

/**
 *******************************************************************************
 * @brief get read count for interrupt/gpdma mode
 *
 * @param[in] om_uart   Pointer to UART
 *
 * @return  read count
 *******************************************************************************
 */
extern uint16_t drv_uart_get_read_count(OM_UART_Type *om_uart);

/**
 *******************************************************************************
 * @brief get busy status for uart
 *
 * @param[in] om_uart   Pointer to UART
 *
 * @return  0: not busy, 1: busy
 *******************************************************************************
 */
extern uint8_t drv_uart_get_is_busy(OM_UART_Type *om_uart);

/**
 *******************************************************************************
 * @brief Get UART base from uart idx
 *
 * @param idx  Index of UART peripheral
 *
 * @return OM_UART Type pointer
 *******************************************************************************
 */
static inline OM_UART_Type* drv_uart_idx2base(uint8_t idx)
{
    OM_UART_Type *const uart[] = {
        #if (RTE_UART0)
        OM_UART0,
        #else
        NULL,
        #endif
        #if (RTE_UART1)
        OM_UART1,
        #else
        NULL,
        #endif
        #if (RTE_UART2)
        OM_UART2,
        #else
        NULL,
        #endif
    };

    return (idx < sizeof(uart)/sizeof(uart[0])) ? uart[idx] : NULL;
}


/**
 *******************************************************************************
 * @brief Uart control
 *
 * @param[in] om_uart       Pointer to UART
 * @param[in] control       Uart control type
 * @param[in] argu          Control argument
 *
 * @return status or error code
 *******************************************************************************
 */
__STATIC_INLINE void *drv_uart_control(OM_UART_Type *om_uart, uart_control_t control, void *argu)
{
    uint32_t ret;

    ret = (uint32_t)OM_ERROR_OK;
    switch (control) {
        case UART_CONTROL_GET_IS_BUSY:
            ret = (uint32_t)drv_uart_get_is_busy(om_uart);
            break;
        default:
            ret = OM_ERROR_PARAMETER;
            break;
    }

    return (void *)ret;
}

/**
 *******************************************************************************
 * @brief uart interrupt service routine
 *
 * @param[in] om_uart       Pointer to UART
 *
 *******************************************************************************
 */
extern void drv_uart_isr(OM_UART_Type *om_uart);


#ifdef  __cplusplus
}
#endif

#endif  /* (RTE_UART0 || RTE_UART1 || RTE_UART2) */

#endif /* __DRV_UART_H */


/** @} */
