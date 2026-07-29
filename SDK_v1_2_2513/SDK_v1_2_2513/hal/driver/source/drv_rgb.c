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
 * @brief    RGB driver source file
 * @details  RGB driver source file
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
#if (RTE_RGB)
#include <stddef.h>
#include "om_device.h"
#include "om_driver.h"


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
#if (RTE_RGB_REGISTER_CALLBACK)
    drv_isr_callback_t    isr_cb;          /**< event callback */
#endif
    uint32_t             *tx_buf;          /**< Pointer to out data buffer, system to peripheral direction */
    uint16_t              tx_num;          /**< Total number of data to be send */
    uint16_t              tx_cnt;          /**< Count of data sent */
    uint8_t               gpdma_tx_chan;   /**< peripheral dma tx channel, from system to peripheral direction */
    bool                  is_reset_en;
} rgb_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
static rgb_env_t rgb_env = {
#if (RTE_RGB_REGISTER_CALLBACK)
    .isr_cb        = NULL,
#endif
    .tx_num        = 0,
    .tx_cnt        = 0,
    .tx_buf        = NULL,
    .is_reset_en   = false,
    .gpdma_tx_chan = GPDMA_NUMBER_OF_CHANNELS,
};


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
#if (RTE_GPDMA)
static void rgb_gpdma_tx_isr_cb(void *resource, drv_event_t event, gpdma_chain_trans_t *next_chain)
{
    drv_event_t drv_event;

    switch (event) {
        case DRV_GPDMA_EVENT_TERMINAL_COUNT_REQUEST:
            drv_event = DRV_EVENT_COMMON_WRITE_COMPLETED;
            break;
        case DRV_GPDMA_EVENT_ERROR:
            drv_event = DRV_EVENT_COMMON_ERROR;
            break;
        case DRV_GPDMA_EVENT_ABORT:
            drv_event = DRV_EVENT_COMMON_ABORT;
            break;
        default:
            drv_event = DRV_EVENT_COMMON_NONE;
            break;
    }

    if (rgb_env.is_reset_en) {
        // wait for reset complete
        while(!(OM_RGB->ISR & RGB_ISR_TXRSTIF_MASK));
        OM_RGB->ISR |= RGB_ISR_TXRSTIF_MASK;
    } else {
        // wait for tx complete
        while(!(OM_RGB->ISR & RGB_ISR_TC_MASK));
        OM_RGB->ISR |= RGB_ISR_TC_MASK;
    }
    // disable rgb
    OM_RGB->CESR &= ~RGB_CESR_EN_MASK;
    drv_rgb_isr_callback(drv_event, rgb_env.tx_buf, rgb_env.tx_num);
}
#endif


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Initialize the RGB with parameters in rgb_config_t
 *
 * @param rgb_cfg            The configuration structure pointer, see @ref rgb_config_t
 *
 * @return                   status, see@ref om_error_t
 *******************************************************************************
    */
om_error_t drv_rgb_init(const rgb_config_t *rgb_cfg)
{
    OM_ASSERT(rgb_cfg);

    DRV_RCC_RESET(RCC_CLK_RGB);

    // Data period and reset period
    REGW(&OM_RGB->PR, MASK_2REG(RGB_PR_DATPR, rgb_cfg->data_per, RGB_PR_RSTPR, rgb_cfg->reset_per));
    // Duty of data bit 0 and data bit 1
    REGW(&OM_RGB->DUTYR, MASK_2REG(RGB_DUTYR_DUTY0, rgb_cfg->duty_zero, RGB_DUTYR_DUTY1, rgb_cfg->duty_one));
    // Extra data
    OM_RGB->EXTDR = rgb_cfg->ext_data;
    // Command register
    REGW(&OM_RGB->COMCR, MASK_8REG(RGB_COMCR_RGBWF, (rgb_cfg->rgb_format == RGB_FORMAT_RGB || rgb_cfg->rgb_format == RGB_FORMAT_GRB) ? 0U : 1U,
                                   RGB_COMCR_RGBF, (rgb_cfg->rgb_format == RGB_FORMAT_RGB || rgb_cfg->rgb_format == RGB_FORMAT_RGBW) ? 0U : 1U,
                                   RGB_COMCR_EXTDEN, rgb_cfg->ext_data_en ? 1U : 0U,
                                   RGB_COMCR_EXTDBITS, rgb_cfg->ext_data_bits,
                                   RGB_COMCR_IDLEL, rgb_cfg->idle_level,
                                   RGB_COMCR_FREEF, rgb_cfg->free_format_en ? 1U : 0U,
                                   RGB_COMCR_FREEDBITS, rgb_cfg->free_format_bits,
                                   RGB_COMCR_PSC, rgb_cfg->psc));

    // enable rgb interrupt
    NVIC_ClearPendingIRQ(RGB_IRQn);
    NVIC_SetPriority(RGB_IRQn, RTE_RGB_IRQ_PRIORITY);
    NVIC_EnableIRQ(RGB_IRQn);

    rgb_env.tx_num = 0;
    rgb_env.tx_buf = NULL;
    rgb_env.tx_cnt = 0;
#if (RTE_RGB_REGISTER_CALLBACK)
    rgb_env.isr_cb = NULL;
#endif
    rgb_env.is_reset_en = rgb_cfg->tx_reset_en;

    return OM_ERROR_OK;
}

void drv_rgb_uninit(void)
{
    uint32_t cesr_reg;

    OM_RGB->CESR &= ~RGB_CESR_EN_MASK;  // disable
    // reset RGB FIFO
    cesr_reg = OM_RGB->CESR;
    OM_RGB->CESR = cesr_reg | RGB_CESR_TXFIFORST_MASK;
    OM_RGB->CESR = cesr_reg;
    // disable RGB clock
    DRV_RCC_CLOCK_ENABLE(RCC_CLK_RGB, 0);
}

/**
 *******************************************************************************
 * @brief  rgb write by polling
 *
 * @param data      The data pointer of the data to be write to dma
 * @param data_len  The date length in bytes
 *
 * @return status, see@ref om_error_t
 *******************************************************************************
 */
om_error_t drv_rgb_write(uint32_t *data, uint16_t data_len)
{
    OM_ASSERT(data_len);

    // TX reset enable
    REGW(&OM_RGB->CESR, MASK_1REG(RGB_CESR_TXRSTEN, rgb_env.is_reset_en ? 1U : 0U));

    // enable rgb
    OM_RGB->CESR |= RGB_CESR_EN_MASK;
    for (uint16_t i = 0; i < data_len; i++) {
        while(!(OM_RGB->ISR & RGB_ISR_TXFNF_MASK));
        OM_RGB->DR = data[i];
    }

    if (rgb_env.is_reset_en) {
        // wait for reset complete
        while(!(OM_RGB->ISR & RGB_ISR_TXRSTIF_MASK));
        OM_RGB->ISR |= RGB_ISR_TXRSTIF_MASK;
    } else {
        // wait for tx complete
        while(!(OM_RGB->ISR & RGB_ISR_TC_MASK));
        OM_RGB->ISR |= RGB_ISR_TC_MASK;
    }

    // disable rgb
    OM_RGB->CESR &= ~RGB_CESR_EN_MASK;
    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief  rgb write by interrupt
 *
 * @param data      The data pointer of the data to be write to dma
 * @param data_len  The date length in bytes
 *
 * @return status:
 *    - OM_ERROR_OK:         Begin to transmit
 *    - others:              No
 *******************************************************************************
 */
om_error_t drv_rgb_write_int(uint32_t *data, uint16_t data_len)
{
    OM_ASSERT(data_len);
    // Save transmit buffer info
    rgb_env.tx_num = data_len;
    rgb_env.tx_buf = data;
    rgb_env.tx_cnt = 0;

    // TX reset enable
    REGW(&OM_RGB->CESR, MASK_1REG(RGB_CESR_TXRSTEN, rgb_env.is_reset_en ? 1U : 0U));

    // enable rgb
    OM_RGB->CESR |= RGB_CESR_EN_MASK;
    while ((rgb_env.tx_cnt < rgb_env.tx_num) && (OM_RGB->ISR & RGB_ISR_TXFNF_MASK)) {
        OM_RGB->DR = rgb_env.tx_buf[rgb_env.tx_cnt];
        rgb_env.tx_cnt++;
    }
    if (rgb_env.is_reset_en) {
        OM_RGB->CESR |= RGB_CESR_TXFNFIE_MASK | RGB_CESR_TXRSTIE_MASK;
    } else {
        OM_RGB->CESR |= RGB_CESR_TXFNFIE_MASK | RGB_CESR_TCIE_MASK;
    }

    return OM_ERROR_OK;
}

#if (RTE_GPDMA)
/**
 *******************************************************************************
 * @brief Allocate rgb dma channel
 *
 * @return errno
 *******************************************************************************
 */
om_error_t drv_rgb_gpdma_channel_allocate(void)
{
    gpdma_config_t dma_cfg;
    om_error_t error;

    (void)error;
    if (rgb_env.gpdma_tx_chan >= GPDMA_NUMBER_OF_CHANNELS) {
        rgb_env.gpdma_tx_chan = drv_gpdma_channel_allocate();
        if (rgb_env.gpdma_tx_chan >= GPDMA_NUMBER_OF_CHANNELS) {
            return OM_ERROR_RESOURCES;
        }
        dma_cfg.channel_ctrl    = GPDMA_SET_CTRL(GPDMA_ADDR_CTRL_INC, GPDMA_ADDR_CTRL_FIXED,
                                                 GPDMA_TRANS_WIDTH_4B, GPDMA_TRANS_WIDTH_4B,
                                                 GPDMA_BURST_SIZE_1T, GPDMA_PRIORITY_LOW);
        dma_cfg.src_id          = GPDMA_ID_MEM;
        dma_cfg.dst_id          = GPDMA_ID_RGB;
        dma_cfg.isr_cb          = rgb_gpdma_tx_isr_cb;
        dma_cfg.cb_param        = NULL;
        dma_cfg.chain_trans     = NULL;
        dma_cfg.chain_trans_num = 0U;

        error = drv_gpdma_channel_config(rgb_env.gpdma_tx_chan, &dma_cfg);
        OM_ASSERT(error == OM_ERROR_OK);
        // DMA enable
        OM_RGB->CESR |= RGB_CESR_DMAEN_MASK;
    }

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Release rgb dma channel
 *
 * @return errno
 *******************************************************************************
 */
om_error_t drv_rgb_gpdma_channel_release(void)
{
    if (rgb_env.gpdma_tx_chan < GPDMA_NUMBER_OF_CHANNELS) {
        drv_gpdma_channel_release(rgb_env.gpdma_tx_chan);
        rgb_env.gpdma_tx_chan = GPDMA_NUMBER_OF_CHANNELS;
    }
    OM_RGB->CESR &= ~RGB_CESR_DMAEN_MASK;

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief  rgb write by DMA
 *
 * @param data      The data pointer of the data to be write to dma
 * @param data_len  The date length in bytes
 *
 * @return status:
 *    - OM_ERROR_OK:         Begin to transmit
 *    - others:              No
 *******************************************************************************
 */
om_error_t drv_rgb_write_dma(uint32_t *data, uint16_t data_len)
{
    OM_ASSERT(rgb_env.gpdma_tx_chan < GPDMA_NUMBER_OF_CHANNELS);

    rgb_env.tx_buf = data;
    rgb_env.tx_num = data_len;

    // TX reset enable
    REGW(&OM_RGB->CESR, MASK_1REG(RGB_CESR_TXRSTEN, rgb_env.is_reset_en ? 1U : 0U));

    // enable rgb
    OM_RGB->CESR |= RGB_CESR_EN_MASK;
    return drv_gpdma_channel_enable(rgb_env.gpdma_tx_chan, (uint32_t)(&(OM_RGB->DR)), (uint32_t)data, data_len << GPDMA_TRANS_WIDTH_4B);
}
#endif


#if (RTE_RGB_REGISTER_CALLBACK)
/**
 *******************************************************************************
 * @brief Register interrupt service routine callback for specified RGB device interrupt
 *
 * @param isr_cb           The event callback function, see @ref drv_isr_callback_t
 *
 * @return None
 *******************************************************************************
 */
void drv_rgb_register_isr_callback(drv_isr_callback_t isr_cb)
{
    rgb_env.isr_cb = isr_cb;
}
#endif

/**
 *******************************************************************************
 * @brief The interrupt callback for RGB driver. It is a weak function. User should define
 *        their own callback in user file, other than modify it in the RGB driver.
 *
 * @param event             The driver RGB event
 *******************************************************************************
 */
__WEAK void drv_rgb_isr_callback(drv_event_t event, uint32_t *data, uint16_t num)
{
    #if (RTE_RGB_REGISTER_CALLBACK)
    if (rgb_env.isr_cb != NULL) {
        rgb_env.isr_cb(OM_RGB, event, data, (void *)((uint32_t)num));
    }
    #endif
}

/**
 *******************************************************************************
 * @brief The RGB interrupt service routine function, should be called in RGB IRQHandler
 *
 * @return None
 *******************************************************************************
 */
void drv_rgb_isr(void)
{
    uint32_t int_status = OM_RGB->ISR;
    OM_RGB->ISR = int_status;

    if (rgb_env.is_reset_en) {
        if (int_status & RGB_ISR_TXRSTIF_MASK) {
            OM_RGB->CESR &= ~RGB_CESR_EN_MASK;
            drv_rgb_isr_callback(DRV_EVENT_COMMON_WRITE_COMPLETED, rgb_env.tx_buf, rgb_env.tx_num);
        }
    } else {
        if (int_status & RGB_ISR_TC_MASK) {
            OM_RGB->CESR &= ~RGB_CESR_EN_MASK;
            drv_rgb_isr_callback(DRV_EVENT_COMMON_WRITE_COMPLETED, rgb_env.tx_buf, rgb_env.tx_num);
        }
    }
    if (int_status & RGB_ISR_TXFNF_MASK) {
        while ((rgb_env.tx_cnt < rgb_env.tx_num) && (OM_RGB->ISR & RGB_ISR_TXFNF_MASK)) {
            OM_RGB->DR = rgb_env.tx_buf[rgb_env.tx_cnt];
            rgb_env.tx_cnt++;
        }
        if (rgb_env.tx_cnt == rgb_env.tx_num) {
            OM_RGB->CESR &= ~RGB_CESR_TXFNFIE_MASK;
        }
    }
}

#endif  /* (RTE_RGB) */


/** @} */
