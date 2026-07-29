/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup DOC DOC
 * @ingroup  DOCUMENT
 * @brief    UART driver source file
 * @details  UART driver source file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_UART0 || RTE_UART1 || RTE_UART2)
#include <stddef.h>
#include "om_device.h"
#include "om_driver.h"


/*******************************************************************************
 * MACROS
 */
#define UART_FIFO_BUF_SIZE         (16)


/*******************************************************************************
 * CONST & VARIABLES
 */
/* uart information */
#if (RTE_UART0)
DRV_DEFINE(UART0, uart0);
#endif
#if (RTE_UART1)
DRV_DEFINE(UART1, uart1);
#endif
#if (RTE_UART2)
DRV_DEFINE(UART2, uart2);
#endif


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static const drv_resource_t *uart_get_resource(OM_UART_Type *om_uart)
{
    static const drv_resource_t *uart_resources[] = {
        #if (RTE_UART0)
        &uart0_resource,
        #endif
        #if (RTE_UART1)
        &uart1_resource,
        #endif
        #if (RTE_UART2)
        &uart2_resource,
        #endif
    };

    for(uint32_t i=0; i<sizeof(uart_resources)/sizeof(uart_resources[0]); i++) {
        if ((uint32_t)om_uart == (uint32_t)uart_resources[i]->reg) {
            return uart_resources[i];
        }
    }

    OM_ASSERT(0);
    return NULL;
}

#if (RTE_GPDMA)
static void uart_gpdma_tx_isr_cb(void *resource, drv_event_t event, gpdma_chain_trans_t *next_chain)
{
    drv_env_t    *env;
    uint32_t      tx_num;
    drv_event_t   drv_event;
    OM_UART_Type *om_uart;

    om_uart  = (OM_UART_Type *)(((const drv_resource_t *)resource)->reg);
    env      = (drv_env_t *)(((const drv_resource_t *)resource)->env);
    tx_num   = env->tx_num;
    env->tx_num = 0U;

    drv_event = DRV_EVENT_COMMON_NONE;
    if (event & DRV_GPDMA_EVENT_TERMINAL_COUNT_REQUEST) {
        drv_event |= DRV_EVENT_COMMON_GPDMA2PERIPH_COMPLETED;
    }
    if (event & DRV_GPDMA_EVENT_ABORT) {
        drv_event |= DRV_EVENT_COMMON_ABORT;
    }
    if (event & DRV_GPDMA_EVENT_ERROR) {
        drv_event |= DRV_EVENT_COMMON_ERROR;
        OM_ASSERT(0);
    }
    OM_CRITICAL_BEGIN();
    env->state &= ~DRV_STATE_TX;
    OM_CRITICAL_END();
    drv_uart_isr_callback(om_uart, drv_event, env->tx_buf, tx_num);
}

static void uart_gpdma_rx_isr_cb(void *resource, drv_event_t event, gpdma_chain_trans_t *next_chain)
{
    drv_env_t   *env;
    uint32_t     rx_num;
    drv_event_t  drv_event;
    OM_UART_Type *om_uart;

    om_uart = (OM_UART_Type *)(((const drv_resource_t *)resource)->reg);
    env     = (drv_env_t *)(((const drv_resource_t *)resource)->env);
    rx_num  = env->rx_num;
    env->rx_num = 0U;

    drv_event = DRV_EVENT_COMMON_NONE;
    if (event & DRV_GPDMA_EVENT_TERMINAL_COUNT_REQUEST) {
        drv_event |= DRV_EVENT_COMMON_READ_COMPLETED;
    }
    if (event & DRV_GPDMA_EVENT_ABORT) {
        drv_event |= DRV_EVENT_COMMON_ABORT;
    }
    if (event & DRV_GPDMA_EVENT_ERROR) {
        drv_event |= DRV_EVENT_COMMON_ERROR;
        OM_ASSERT(0);
    }
    OM_CRITICAL_BEGIN();
    env->state &= ~DRV_STATE_RX;
    OM_CRITICAL_END();
    drv_uart_isr_callback(om_uart, drv_event, env->rx_buf, rx_num);
}
#endif  /* (RTE_GPDMA) */

static drv_event_t uart_rx_line_int_handler(OM_UART_Type *om_uart)
{
    uint32_t    lsr;
    drv_event_t event;

    event = DRV_EVENT_COMMON_NONE;
    lsr   = om_uart->LSR & UART_LSR_LINE_STATUS;

    // OverRun error
    if (lsr & UART_LSR_OE) {
        event |= DRV_EVENT_COMMON_RX_OVERFLOW;
    }
    // Parity error
    if (lsr & UART_LSR_PE) {
        event |= DRV_EVENT_UART_RX_PARITY_ERROR;
    }
    // Break detected
    if (lsr & UART_LSR_BI) {
        event |= DRV_EVENT_UART_RX_BREAK;
    }
    // Framing error
    if (lsr & UART_LSR_FE) {
        event |= DRV_EVENT_UART_RX_FRAME_ERROR;
    }

    return event;
}

static void uart_tx_enable(OM_UART_Type *om_uart, uint8_t tx_en)
{
    uint32_t lcr;

    lcr = om_uart->LCR;
    if (lcr & UART_LCR_HALF_DUPLEX_EN) {
        if (tx_en) {
            lcr |= UART_LCR_HALF_DUPLEX_TX_EN;
        } else {
            lcr &= ~UART_LCR_HALF_DUPLEX_TX_EN;
        }
        om_uart->LCR = lcr;
    }
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief UART initialization
 *
 * @param[in] om_uart       Pointer to UART
 * @param[in] uart_cfg      Configuration for uart
 *
 * @return status:
 *    - OM_ERROR_OK:         Nothing more to do
 *    - others:               No
 *******************************************************************************
 */
om_error_t drv_uart_init(OM_UART_Type *om_uart, const uart_config_t *uart_cfg)
{
    const drv_resource_t   *resource;
    drv_env_t              *env;

    resource = uart_get_resource(om_uart);
    if((resource == NULL) || (uart_cfg == NULL)) {
        return OM_ERROR_PARAMETER;
    }

    // check is supported
    if ((uart_cfg->data_bit == UART_DATA_BIT_9) && (!uart_cfg->lin_enable)) {
        return OM_ERROR_UNSUPPORTED;
    }
    if (!(resource->cap & CAP_UART_LIN_MODE_MASK)) {
        if ((uart_cfg->lin_enable) || (uart_cfg->data_bit == UART_DATA_BIT_9)) {
            return OM_ERROR_UNSUPPORTED;
        }
    }

    switch ((uint32_t)om_uart) {
        #if (RTE_UART0)
        case OM_UART0_BASE:
            DRV_RCC_RESET(RCC_CLK_UART0);
            break;
        #endif
        #if (RTE_UART1)
        case OM_UART1_BASE:
            DRV_RCC_RESET(RCC_CLK_UART1);
            break;
        #endif
        #if (RTE_UART2)
        case OM_UART2_BASE:
            DRV_RCC_RESET(RCC_CLK_UART2);
            break;
        #endif
        default:
            OM_ASSERT(0);
            break;
    }
    env = (drv_env_t *)(resource->env);

    // frame format
    do {
        uint32_t lcr = UART_LCR_RX_EN | (uart_cfg->half_duplex_en ? UART_LCR_HALF_DUPLEX_EN : 0U) | (uart_cfg->lin_enable ? UART_LCR_LIN_EN : 0U);

        // uart Data bits
        switch (uart_cfg->data_bit) {
            case UART_DATA_BIT_5:
                lcr |= UART_LCR_DLS_5;
                break;
            case UART_DATA_BIT_6:
                lcr |= UART_LCR_DLS_6;
                break;
            case UART_DATA_BIT_7:
                lcr |= UART_LCR_DLS_7;
                break;
            case UART_DATA_BIT_8:
                lcr |= UART_LCR_DLS_8;
                break;
            case UART_DATA_BIT_9:
                OM_ASSERT(uart_cfg->lin_enable);
                lcr |= UART_LCR_DATA8EN;
                break;
            default:
                lcr = 0;
                OM_ASSERT(0);
                break;
        }
        // uart Parity
        switch (uart_cfg->parity) {
            case UART_PARITY_EVEN:
                lcr |= (UART_LCR_PEN_MASK | UART_LCR_EPS);
                break;
            case UART_PARITY_ODD:
                lcr |= UART_LCR_PEN_MASK;
                lcr &= ~UART_LCR_EPS;
                break;
            case UART_PARITY_NONE:
                lcr &= ~(UART_LCR_PEN_MASK | UART_LCR_EPS);
                break;
            default:
                OM_ASSERT(0);
                break;
        }
        // uart Stop bits
        switch (uart_cfg->stop_bit) {
            case UART_STOP_BIT_1_5:
            case UART_STOP_BIT_2:
                lcr |= UART_LCR_STOP_2B;
                break;
            case UART_STOP_BIT_1:
                lcr |= UART_LCR_STOP_1B;
                break;
            default:
                OM_ASSERT(0);
                break;
        }

        om_uart->LCR = lcr;
        // uart Baudrate
        drv_uart_set_baudrate(om_uart, uart_cfg->baudrate);
    } while(0);

    // uart Flow control (RTS and CTS lines are only available on uart1)
    switch (uart_cfg->flow_control) {
        case UART_FLOW_CONTROL_NONE:
            om_uart->MCR = 0U;
            break;
        case UART_FLOW_CONTROL_RTS_CTS:
            if (resource->cap & CAP_UART_CTS_RTS_FLOW_CONTROL_MASK) {
                // Auto RTS -- Becomes active when the following occurs:
                //   - Auto Flow Control is selected during configuration
                //   - RTS (MCR[1] bit and MCR[5]bit are both set)
                om_uart->MCR = UART_MCR_RTS | UART_MCR_AFCE;
            } else {
                return OM_ERROR_UNSUPPORTED;
            }
            break;
        default:
            OM_ASSERT(0);
            break;
    }

    // disable all interrupt
    om_uart->IER = 0U;

    // Configure FIFO Control register
    // Set trigger level
    om_uart->FCR = (UART_FCR_RST_RCVR) | (UART_FCR_RST_XMIT)
                 | (UART_FCR_FIFO_EN) | (UART_FCR_TRIGGER_REC_FIFO_1B)
                 | (UART_FCR_TRIGGER_TRANS_FIFO_0B);

    #if (RTE_GPDMA)
    env->gpdma_tx_chan = GPDMA_NUMBER_OF_CHANNELS;
    env->gpdma_rx_chan = GPDMA_NUMBER_OF_CHANNELS;
    #endif

    // Clear and Enable uart IRQ
    NVIC_ClearPendingIRQ(resource->irq_num);
    NVIC_SetPriority(resource->irq_num, resource->irq_prio);
    NVIC_EnableIRQ(resource->irq_num);

    env->isr_cb = NULL;
    env->state  = DRV_STATE_INIT;

    return OM_ERROR_OK;
}

void drv_uart_uninit(OM_UART_Type *om_uart)
{
    switch ((uint32_t)om_uart) {
        #if (RTE_UART0)
        case OM_UART0_BASE:
            DRV_RCC_CLOCK_ENABLE(RCC_CLK_UART0, 0);
            break;
        #endif
        #if (RTE_UART1)
        case OM_UART1_BASE:
            DRV_RCC_CLOCK_ENABLE(RCC_CLK_UART1, 0);
            break;
        #endif
        #if (RTE_UART2)
        case OM_UART2_BASE:
            DRV_RCC_CLOCK_ENABLE(RCC_CLK_UART2, 0);
            break;
        #endif
        default:
            OM_ASSERT(0);
            break;
    }
}

om_error_t drv_uart_set_baudrate(OM_UART_Type *om_uart, uint32_t baudrate)
{
    #define UART_MODE_X_DIV   16U
    const drv_resource_t *resource;
    drv_env_t            *env;
    uint32_t baud_divisor;
    uint32_t freq;

    resource = uart_get_resource(om_uart);
    if(resource == NULL) {
        return OM_ERROR_RESOURCES;
    }

    env = (drv_env_t *)(resource->env);
    if (env->state & DRV_STATE_TX) {
        return OM_ERROR_BUSY;
    }

    /* Compute divisor value. Normally, we should simply return:
     *   NS16550_CLK / MODE_X_DIV / baudrate
     * but we need to round that value by adding 0.5.
     * Rounding is especially important at high baud rates.
     */
    // calculate uart divisor int and baudrate divisor
    baud_divisor = 1;
    freq = baudrate * UART_MODE_X_DIV * baud_divisor;
    // double the divisor, until the int part is under 8 bit range
    while (drv_rcc_clock_set((rcc_clk_t)(size_t)om_uart, freq) != OM_ERROR_OK) {
        freq = freq << 1;
        baud_divisor = baud_divisor << 1;
    }

    OM_CRITICAL_BEGIN();
    /* Baud rate setting.*/
    om_uart->LCR |= UART_LCR_DLAB_MASK;
    om_uart->DLL  = baud_divisor & 0xff;
    om_uart->DLH  = (baud_divisor >> 8) & 0xff;
    om_uart->LCR &= ~UART_LCR_DLAB_MASK;
    OM_CRITICAL_END();

    return OM_ERROR_OK;
}

#if (RTE_UART_REGISTER_CALLBACK)
/**
 *******************************************************************************
 * @brief Register event callback for transmit/receive by interrupt & dma mode
 *
 * @param[in] om_uart       Pointer to UART
 * @param[in] event_cb       Pointer to callback
 *
 *******************************************************************************
 */
void drv_uart_register_isr_callback(OM_UART_Type *om_uart, drv_isr_callback_t cb)
{
    const drv_resource_t *resource;
    drv_env_t            *env;

    resource = uart_get_resource(om_uart);
    if(resource != NULL) {
        env = (drv_env_t *)(resource->env);
        env->isr_cb = cb;
    }
}
#endif

__WEAK void drv_uart_isr_callback(OM_UART_Type *om_uart, drv_event_t event, uint8_t *data, uint32_t num)
{
    #if (RTE_UART_REGISTER_CALLBACK)
    const drv_resource_t *resource;
    drv_env_t            *env;

    resource = uart_get_resource(om_uart);
    if (resource != NULL) {
        env = (drv_env_t *)(resource->env);
        if (env->isr_cb != NULL) {
            env->isr_cb(om_uart, event, data, (void *)num);
        }
    }
    #endif
}

/**
 *******************************************************************************
 * @brief UART send sync break filed in LIN mode
 *
 * @param[in] om_uart       Pointer to UART
 *******************************************************************************
 */
void drv_uart_lin_send_break(OM_UART_Type *om_uart)
{
    uint32_t lcr = om_uart->LCR;
    OM_ASSERT(lcr&UART_LCR_LIN_EN);

    om_uart->LCR = lcr | UART_LCR_SBRK;
    while(om_uart->USR & UART_USR_LIN_BREAK);
    om_uart->LCR = lcr & ~UART_LCR_SBRK;
}

/**
 *******************************************************************************
 * @brief Transmit number of bytes from UART by block mode
 *
 * @param[in] om_uart        Pointer to UART
 * @param[in] data           Pointer where data to transmit from UART
 * @param[in] num            Number of data bytes to transmit
 *
 * @return write bytes
 *******************************************************************************
 */
uint16_t drv_uart_write(OM_UART_Type *om_uart, const uint8_t *data, uint16_t num, uint32_t timeout_ms)
{
    OM_ASSERT((data != NULL) && (num != 0));

    OM_CRITICAL_BEGIN();
    uart_tx_enable(om_uart, 1U);
    OM_CRITICAL_END();
    for (uint16_t i = 0; i < num; i++) {
        om_error_t error;
        DRV_WAIT_MS_UNTIL_TO(!(om_uart->LSR & UART_LSR_THRE), timeout_ms, error);
        if (error != OM_ERROR_OK) {
            return i;
        }
        om_uart->THR = data[i];
    }
    // wait for fifo/hold reg and shift reg empty
    while (!(om_uart->LSR & UART_LSR_TEMT));

    return num;
}

/**
 *******************************************************************************
 * @brief Transmit number of bytes from UART by interrupt mode
 *
 * @param[in] om_uart       Pointer to UART
 * @param[in] data          Pointer where data to transmit from UART
 * @param[in] num           Number of data bytes to transmit
 *
 * @return status:
 *    - OM_ERROR_OK:         Begin to transmit
 *    - others:               No
 *******************************************************************************
 */
om_error_t drv_uart_write_int(OM_UART_Type *om_uart, const uint8_t *data, uint16_t num)
{
    const drv_resource_t   *resource;
    drv_env_t              *env;

    // check input parameter
    OM_ASSERT(num != 0U);
    resource = uart_get_resource(om_uart);
    if(resource == NULL) {
        return OM_ERROR_PARAMETER;
    }
    env = (drv_env_t *)(resource->env);

    // Save transmit buffer info
    env->tx_buf = (uint8_t *)data;
    env->tx_num = num;
    env->tx_cnt = 0U;

    OM_CRITICAL_BEGIN();
    env->state |= DRV_STATE_TX;
    uart_tx_enable(om_uart, 1U);
    // Enable transmit holding register empty interrupt
    om_uart->IER |= UART_IER_THREI | UART_IER_LBDI | UART_IER_NEI;
    while ((om_uart->LSR & UART_LSR_THRE) && (env->tx_cnt < env->tx_num)) {
        om_uart->THR = env->tx_buf[env->tx_cnt++];
    }
    OM_CRITICAL_END();

    return OM_ERROR_OK;
}

#if (RTE_GPDMA)
om_error_t drv_uart_gpdma_channel_allocate(OM_UART_Type *om_uart, drv_gpdma_chan_t channel)
{
    const drv_resource_t   *resource;
    drv_env_t              *env;
    gpdma_config_t          dma_cfg;
    om_error_t              error;

    resource = uart_get_resource(om_uart);
    if(resource == NULL) {
        return OM_ERROR_PARAMETER;
    }
    error = OM_ERROR_OK;
    env = (drv_env_t *)(resource->env);
    if ((channel & DRV_GPDMA_RX_CHAN) && (env->gpdma_rx_chan >= GPDMA_NUMBER_OF_CHANNELS)) {
        OM_ASSERT(resource->cap & CAP_UART_GPDMA_RX_MASK);
        env->gpdma_rx_chan = drv_gpdma_channel_allocate();
        if (env->gpdma_rx_chan >= GPDMA_NUMBER_OF_CHANNELS) {
            return OM_ERROR_RESOURCES;
        }
        dma_cfg.channel_ctrl     = GPDMA_SET_CTRL(GPDMA_ADDR_CTRL_FIXED, GPDMA_ADDR_CTRL_INC,
                                                  GPDMA_TRANS_WIDTH_1B,  GPDMA_TRANS_WIDTH_1B,
                                                  GPDMA_BURST_SIZE_1T, (resource->gpdma_rx.prio) ? GPDMA_PRIORITY_HIGH : GPDMA_PRIORITY_LOW);
        dma_cfg.src_id           = (gpdma_id_t)resource->gpdma_rx.id;
        dma_cfg.dst_id           = GPDMA_ID_MEM;
        dma_cfg.isr_cb           = uart_gpdma_rx_isr_cb;
        dma_cfg.cb_param         = (void *)resource;
        dma_cfg.chain_trans      = NULL;
        dma_cfg.chain_trans_num  = 0U;

        error = drv_gpdma_channel_config(env->gpdma_rx_chan, &dma_cfg);
        OM_ASSERT(error == OM_ERROR_OK);
    }
    if ((channel & DRV_GPDMA_TX_CHAN) && (env->gpdma_tx_chan >= GPDMA_NUMBER_OF_CHANNELS)) {
        OM_ASSERT(resource->cap & CAP_UART_GPDMA_TX_MASK);
        env->gpdma_tx_chan = drv_gpdma_channel_allocate();
        if (env->gpdma_tx_chan >= GPDMA_NUMBER_OF_CHANNELS) {
            if (channel & DRV_GPDMA_RX_CHAN) {
                drv_gpdma_channel_release(env->gpdma_rx_chan);
                env->gpdma_rx_chan = GPDMA_NUMBER_OF_CHANNELS;
            }
            return OM_ERROR_RESOURCES;
        }
        dma_cfg.channel_ctrl     = GPDMA_SET_CTRL(GPDMA_ADDR_CTRL_INC, GPDMA_ADDR_CTRL_FIXED,
                                                  GPDMA_TRANS_WIDTH_1B, GPDMA_TRANS_WIDTH_1B,
                                                  GPDMA_BURST_SIZE_1T, (resource->gpdma_tx.prio) ? GPDMA_PRIORITY_HIGH : GPDMA_PRIORITY_LOW);
        dma_cfg.src_id           = GPDMA_ID_MEM;
        dma_cfg.dst_id           = (gpdma_id_t)resource->gpdma_tx.id;
        dma_cfg.isr_cb           = uart_gpdma_tx_isr_cb;
        dma_cfg.cb_param         = (void *)resource;
        dma_cfg.chain_trans      = NULL;
        dma_cfg.chain_trans_num  = 0U;
        error = drv_gpdma_channel_config(env->gpdma_tx_chan, &dma_cfg);
        OM_ASSERT(error == OM_ERROR_OK);
    }

    return error;
}

om_error_t drv_uart_gpdma_channel_release(OM_UART_Type *om_uart, drv_gpdma_chan_t channel)
{
    const drv_resource_t   *resource;
    drv_env_t              *env;

    resource = uart_get_resource(om_uart);
    if (resource) {
        env = (drv_env_t *)(resource->env);
        if ((channel & DRV_GPDMA_RX_CHAN) && (env->gpdma_rx_chan < GPDMA_NUMBER_OF_CHANNELS)) {
            drv_gpdma_channel_release(env->gpdma_rx_chan);
            env->gpdma_rx_chan = GPDMA_NUMBER_OF_CHANNELS;
        }
        if ((channel & DRV_GPDMA_TX_CHAN) && (env->gpdma_tx_chan < GPDMA_NUMBER_OF_CHANNELS)) {
            drv_gpdma_channel_release(env->gpdma_tx_chan);
            env->gpdma_tx_chan = GPDMA_NUMBER_OF_CHANNELS;
        }
        return OM_ERROR_OK;
    } else {
        return OM_ERROR_PARAMETER;
    }
}

/**
 *******************************************************************************
 * @brief Transmit number of bytes from UART by DMA mode
 *
 * @param[in] om_uart       Pointer to UART
 * @param[in] data           Pointer where data to transmit from UART
 * @param[in] num            Number of data bytes to USAFT transmiter
 *
 * @return status:
 *    - OM_ERROR_OK:         Begin to transmit
 *    - others:              No
 *******************************************************************************
 */
om_error_t drv_uart_write_dma(OM_UART_Type *om_uart, const uint8_t *data, uint16_t num)
{
    const drv_resource_t   *resource;
    drv_env_t              *env;

    // check input parameter
    OM_ASSERT((data != NULL) && (num != 0U));
    resource = uart_get_resource(om_uart);
    if(resource == NULL) {
        return OM_ERROR_PARAMETER;
    }

    env = (drv_env_t *)(resource->env);
    OM_ASSERT(env->gpdma_tx_chan < GPDMA_NUMBER_OF_CHANNELS);
    env->tx_buf = (uint8_t *)data;
    env->tx_num = num;

    OM_CRITICAL_BEGIN();
    uart_tx_enable(om_uart, 1U);
    env->state |= DRV_STATE_TX;
    OM_CRITICAL_END();

    return drv_gpdma_channel_enable(env->gpdma_tx_chan, (uint32_t)(&(om_uart->THR)), (uint32_t)data, num);
}
#endif

void drv_uart_abort_write(OM_UART_Type *om_uart)
{
    const drv_resource_t   *resource;

    resource = uart_get_resource(om_uart);
    if (resource != NULL) {
        drv_env_t *env;
        env = (drv_env_t *)(resource->env);

        #if (RTE_GPDMA)
        if (env->gpdma_tx_chan < GPDMA_NUMBER_OF_CHANNELS) {
            drv_gpdma_channel_disable(env->gpdma_tx_chan);
        }
        #endif

        // Interrupt mode
        OM_CRITICAL_BEGIN();
        om_uart->IER &= ~UART_IER_THREI;   // Disable transmit holding register empty interrupt
        // Transmit FIFO reset
        om_uart->FCR = (UART_FCR_RST_XMIT)
                     | (UART_FCR_FIFO_EN) | (UART_FCR_TRIGGER_REC_FIFO_1B)
                     | (UART_FCR_TRIGGER_TRANS_FIFO_0B);
        env->state &= ~DRV_STATE_TX;
        OM_CRITICAL_END();
    }
}

uint16_t drv_uart_get_write_count(OM_UART_Type *om_uart)
{
    const drv_resource_t   *resource;

    resource = uart_get_resource(om_uart);
    if (resource != NULL) {
        drv_env_t *env;
        env = (drv_env_t *)(resource->env);

        #if (RTE_GPDMA)
        if ((env->gpdma_tx_chan < GPDMA_NUMBER_OF_CHANNELS) && drv_gpdma_channel_is_busy(env->gpdma_tx_chan)) {
            return (env->tx_num - drv_gpdma_channel_get_left_count(env->gpdma_tx_chan));
        }
        #endif
        return env->tx_cnt;
    }
    return 0U;
}

/**
 *******************************************************************************
 * @brief Prepare receive number of bytes by block mode
 *
 * @param[in] om_uart   Pointer to UART
 * @param[in] data      Pointer where data to receive from UART
 * @param[in] num       Number of data bytes from UART receiver
 *
 * @return Number of data bytes from UART
 *******************************************************************************
 */
uint16_t drv_uart_read(OM_UART_Type *om_uart, uint8_t *data, uint16_t num, uint32_t timeout_ms)
{
    // check input parameter
    OM_ASSERT((data != NULL) && (num != 0U));

    OM_CRITICAL_BEGIN();
    uart_tx_enable(om_uart, 0U);
    OM_CRITICAL_END();
    for (uint16_t i = 0; i < num; i++) {
        om_error_t error;
        DRV_WAIT_MS_UNTIL_TO(!(om_uart->LSR & UART_LSR_DR), timeout_ms, error);
        if (error != OM_ERROR_OK) {
            return i;
        }
        data[i] = om_uart->RBR;
    }

    return num;
}

/**
 *******************************************************************************
 * @brief Prepare receive number of bytes by interrupt mode, if num is 0, uart would
 *        receive data until drv_uart_control with UART_CONTROL_ABORT_RECEIVE is called.
 *
 * @param[in] om_uart       Pointer to UART
 * @param[in] data           Pointer where data to receive from UART
 * @param[in] num            Number of data bytes from UART receiver
 *
 * @return status:
 *    - OM_ERROR_OK:         Begin to receive
 *    - others:              No
 *******************************************************************************
 */
om_error_t drv_uart_read_int(OM_UART_Type *om_uart, uint8_t *data, uint16_t num)
{
    const drv_resource_t   *resource;
    drv_env_t              *env;

    // OM_ASSERT((num == 0 && data == NULL) || (num != 0 && data != NULL));
    resource = uart_get_resource(om_uart);
    if(resource == NULL) {
        return OM_ERROR_PARAMETER;
    }
    env = (drv_env_t *)(resource->env);
    env->rx_cnt = 0U;
    env->rx_num = num;
    env->rx_buf = (uint8_t *)data;

    OM_CRITICAL_BEGIN();
    uart_tx_enable(om_uart, 0U);
    env->state |= DRV_STATE_RX;
    // Enable Rx interrupts
    om_uart->IER |= (UART_IER_RLSI | UART_IER_RDI);
    if (om_uart->LCR&UART_LCR_LIN_EN) {
        om_uart->IER |= UART_IER_LBDI;
    }

    OM_CRITICAL_END();

    return OM_ERROR_OK;
}

#if (RTE_GPDMA)
/**
 *******************************************************************************
 * @brief Prepare receive number of bytes by DMA mode
 *
 * @param[in] om_uart       Pointer to UART
 * @param[in] data           Pointer where data to receive from UART
 * @param[in] num            Number of data bytes from UART receiver
 *
 * @return status:
 *    - OM_ERROR_OK:         Begin to receive
 *    - others:              No
 *******************************************************************************
 */
om_error_t drv_uart_read_dma(OM_UART_Type *om_uart, uint8_t *data, uint16_t num)
{
    const drv_resource_t   *resource;
    drv_env_t              *env;

    // check input parameter
    OM_ASSERT((data != NULL) && (num != 0U));

    resource = uart_get_resource(om_uart);
    if(resource == NULL) {
        return OM_ERROR_PARAMETER;
    }

    env = (drv_env_t *)(resource->env);
    OM_ASSERT(env->gpdma_rx_chan < GPDMA_NUMBER_OF_CHANNELS);
    env->rx_buf = (uint8_t *)data;
    env->rx_num = num;

    OM_CRITICAL_BEGIN();
    uart_tx_enable(om_uart, 0U);
    env->state |= DRV_STATE_RX;
    OM_CRITICAL_END();

    return drv_gpdma_channel_enable(env->gpdma_rx_chan, (uint32_t)data, (uint32_t)(&(om_uart->RBR)), num);
}
#endif

void drv_uart_abort_read(OM_UART_Type *om_uart)
{
    const drv_resource_t   *resource;

    resource = uart_get_resource(om_uart);
    if (resource != NULL) {
        drv_env_t *env;
        env = (drv_env_t *)(resource->env);
        // DMA mode - disable DMA channel
        #if (RTE_GPDMA)
        if (env->gpdma_rx_chan < GPDMA_NUMBER_OF_CHANNELS) {
            drv_gpdma_channel_disable(env->gpdma_rx_chan);
        }
        #endif

        // Interrupt mode
        OM_CRITICAL_BEGIN();
        om_uart->IER &= ~(UART_IER_RDI | UART_IER_RLSI);  // Disable receive data available interrupt
        // Receive FIFO reset
        om_uart->FCR = (UART_FCR_RST_RCVR)
                     | (UART_FCR_FIFO_EN) | (UART_FCR_TRIGGER_REC_FIFO_1B)
                     | (UART_FCR_TRIGGER_TRANS_FIFO_0B);
        env->state &= ~DRV_STATE_RX;
        OM_CRITICAL_END();
    }
}

uint16_t drv_uart_get_read_count(OM_UART_Type *om_uart)
{
    const drv_resource_t   *resource;

    resource = uart_get_resource(om_uart);
    if (resource != NULL) {
        drv_env_t *env;
        env = (drv_env_t *)(resource->env);
        #if (RTE_GPDMA)
        if ((env->gpdma_rx_chan < GPDMA_NUMBER_OF_CHANNELS) && drv_gpdma_channel_is_busy(env->gpdma_rx_chan)) {
            return (env->rx_num - drv_gpdma_channel_get_left_count(env->gpdma_rx_chan));
        }
        #endif
        return env->rx_cnt;
    }
    return 0U;
}

uint8_t drv_uart_get_is_busy(OM_UART_Type *om_uart)
{
    const drv_resource_t *resource;

    resource = uart_get_resource(om_uart);
    if (resource != NULL) {
        drv_env_t *env;
        env = (drv_env_t *)(resource->env);
        return env->state != DRV_STATE_INIT;
    }
    return 1;
}

/**
 *******************************************************************************
 * @brief uart interrupt service routine
 *
 * @param[in] om_uart       Pointer to UART
 *
 *******************************************************************************
 */
void drv_uart_isr(OM_UART_Type *om_uart)
{
    drv_event_t event;
    drv_env_t *env;
    const drv_resource_t  *resource;
    uint8_t int_status;
    uint32_t int_status_clr;

    resource = uart_get_resource(om_uart);
    if (resource == NULL) {
        return;
    }

    env = (drv_env_t *)(resource->env);
    int_status = (om_uart->IIR & UART_IIR_INT_ID_MASK);

    switch (int_status) {
        // Receive data avaliable & Character timeout indication
        case UART_IIR_RDI:
        case UART_IIR_CTI:

            event = DRV_EVENT_COMMON_NONE;
            // When rx_num is 0, uart will automatically receive data as long as
            // interrupt triggered, and interrupt will not be closed. If uart need
            // to be stopped, please use drv_uart_control() with UART_CONTROL_ABORT_RECEIVE.
            // When rx_num is not 0, uart will receive data with expected length
            if (env->rx_num == 0) {
                uint8_t buf[UART_FIFO_BUF_SIZE];
                uint8_t cnt = 0;

                // receive all available data
                while ((om_uart->LSR & UART_LSR_DR) && (cnt < sizeof(buf))) {
                    buf[cnt++] = om_uart->RBR;
                }
                // Character time-out indicator
                if (int_status == UART_IIR_CTI) {
                    // Signal RX Time-out event, if not all requested data received
                    event |= DRV_EVENT_UART_RX_TIMEOUT;
                }

                event |= DRV_EVENT_COMMON_READ_COMPLETED;
                drv_uart_isr_callback(om_uart, event, buf, cnt);
            } else {
                while (om_uart->LSR & UART_LSR_DR) {
                    env->rx_buf[env->rx_cnt] = om_uart->RBR;
                    env->rx_cnt++;
                    // Check if requested amount of data is received
                    if (env->rx_cnt == env->rx_num) {
                        // Check RX line interrupt for errors
                        event = (drv_event_t)(DRV_EVENT_COMMON_READ_COMPLETED | uart_rx_line_int_handler(om_uart));
                        om_uart->IER &= ~(UART_IER_RLSI | UART_IER_RDI);
                        env->state &= ~DRV_STATE_RX;
                        break;
                    }
                }

                // Character time-out indicator
                if (int_status == UART_IIR_CTI) {
                    // Signal RX Time-out event, if not all requested data received
                    if (env->rx_cnt != env->rx_num) {
                        event |= DRV_EVENT_UART_RX_TIMEOUT;
                    }
                }
                drv_uart_isr_callback(om_uart, event, env->rx_buf, env->rx_cnt);
            }
            // clear timeout int if timeout come but data is not ready,
            int_status = (om_uart->IIR & UART_IIR_INT_ID_MASK);
            if ((UART_IIR_CTI == int_status) && (!(om_uart->LSR & UART_LSR_DR))) {
                // data come
                om_uart->LCR |= UART_LCR_TIMEOUT_SW_CLEAR;
            }
            break;

        // Transmit holding register empty
        case UART_IIR_THREI:
            // Check if all data is transmitted
            if (env->tx_num == env->tx_cnt) {
                // Disable THRE interrupt
                om_uart->IER &= ~UART_IER_THREI;
                env->state &= ~DRV_STATE_TX;
                // send tx complete event
                drv_uart_isr_callback(om_uart, DRV_EVENT_COMMON_WRITE_COMPLETED, env->tx_buf, env->tx_cnt);
            } else {
                // support programmable THRE
                #if 0
                // Check if FIFO is enabled
                if (om_uart->IIR & UART_IIR_FE) {
                    while (!(om_uart->LSR & UART_LSR_THRE) && (env->tx_num != env->tx_cnt)) {
                        // Write data to Tx FIFO
                        om_uart->THR = env->tx_buf[env->tx_cnt];
                        env->tx_cnt++;
                    }
                } else {
                }
                #endif
                while ((om_uart->LSR & UART_LSR_THRE) && (env->tx_num != env->tx_cnt)) {
                    // Write data to Tx FIFO
                    om_uart->THR = env->tx_buf[env->tx_cnt];
                    env->tx_cnt++;
                }
            }
            break;
        // Receive line status
        case UART_IIR_RLSI:
            event = uart_rx_line_int_handler(om_uart);
            drv_uart_isr_callback(om_uart, event, env->rx_buf, 0U);
            break;
        //Busy detect
        case UART_IIR_BDI:
             int_status_clr = om_uart->USR;
             drv_uart_isr_callback(om_uart, DRV_EVENT_UART_BUSY_DETECT, env->rx_buf, env->rx_num);
             break;
        default:
            if (int_status&UART_IIR_LBDI) {
                om_uart->IER |= UART_IER_LBDCF;
                drv_uart_isr_callback(om_uart, DRV_EVENT_UART_LIN_BREAK_DETECT, env->rx_buf, env->rx_num);
            }
            break;
    }

    (void)int_status_clr;
}


#endif  /* (RTE_UART0 || RTE_UART1 || RTE_UART2) */


/** @} */
