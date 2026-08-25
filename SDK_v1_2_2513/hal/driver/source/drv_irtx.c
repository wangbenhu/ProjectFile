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
 * @brief    driver for irtx source file
 * @details  driver for irtx source file
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
#if (RTE_IRTX)
#include <stddef.h>
#include "om_device.h"
#include "om_driver.h"


/*******************************************************************************
 * TYPEDEFS
 */
/// irtx work mode
typedef enum {
    /// work in normal mode
    IRTX_MODE_NORMAL    = 0x0,
    /// work in fifo mode
    IRTX_MODE_FIFO      = 0x1,
    /// work in dma mode
    IRTX_MODE_DMA       = 0x3,
} irtx_mode_t;

/// irtx environment
typedef struct {
    uint32_t  carrier_freq;
    drv_isr_callback_t isr_cb;
} irtx_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
static irtx_env_t irtx_env = {
    .carrier_freq       = 38000,
    .isr_cb             = NULL,
};


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief init irtx
 *
 * @param[in] irtx_config     Configuration for irtx
 *
 * @return errno
 *******************************************************************************
 */
om_error_t drv_irtx_init(const irtx_config_t *irtx_config)
{
    // soft reset irtx
    DRV_RCC_RESET(RCC_CLK_IRTX);

    // set irtx work in dma mode
    OM_IRTX->PWM_MODE = IRTX_MODE_DMA;

    /* irtx can transmit two kinds of pulse group by different freq and duty cycle */
    // config pulse group0, whose freq and duty cycle configured by PWM_TMAX & PWM_TCMP
    uint32_t irtx_clk = drv_rcc_clock_get((rcc_clk_t)RCC_CLK_IRTX);
    irtx_env.carrier_freq = irtx_config->carrier_freq;
    uint32_t prescaler = irtx_clk / irtx_config->carrier_freq;
    OM_IRTX->PWM_TMAX = prescaler; /* carrier freq */
    OM_IRTX->PWM_TCMP = (uint32_t)prescaler * irtx_config->carrier_duty_cycle / 1000; /* carrier duty cycle */
    /* // config pulse group1, whose freq and duty cycle configured by PWM_TMAX_SHADOW & PWM_TCMP_SHADOW
    uint32_t carrier_freq_group1 = 50000; // 50kHz
    uint32_t duty_cycle_group1 = 500; // 50%
    uint32_t prescaler_group1 = irtx_clk / carrier_freq_group1;
    OM_IRTX->PWM_TMAX_SHADOW = prescaler_group1;
    OM_IRTX->PWM_TCMP_SHADOW  = (uint32_t)prescaler * duty_cycle_group1 / 1000;
    */

    // set irtx polarity
    OM_IRTX->PWM_POLARITY = irtx_config->polarity;
    // set output invert
    OM_IRTX->PWM_INV = irtx_config->invert;

    // config irtx dma interrupt
    NVIC_ClearPendingIRQ(IRTX_IRQn);
    NVIC_SetPriority(IRTX_IRQn, RTE_IRTX_IRQ_PRIORITY);
    NVIC_EnableIRQ(IRTX_IRQn);
    // config dma int mask
    OM_IRTX->PWM_INT_MASK = IRTX_PWM_INT_DMA_MODE_INT_MASK;

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief get wave code of pulse group0
 * @param[in] wave_len_us      wave high level length in us
 * @param[in] wave_level       config wave level is high or low
 *
 * @return return wave code
 *******************************************************************************
 */
irtx_code_t drv_irtx_get_wave_code(const uint32_t wave_len_us, const irtx_wave_level_t wave_level)
{
    /* get input wave code of pulse group0 by its wave length and level status */
    uint32_t carrier_freq_group0_kHz = irtx_env.carrier_freq / 1000;
    uint32_t carrier_num_x1000 = wave_len_us * carrier_freq_group0_kHz;

    //0x3FFF: max carrier num
    OM_ASSERT(carrier_num_x1000 <= (0x3FFF * 1000));

    uint32_t carrier_num_raw = carrier_num_x1000  / 1000;
    irtx_code_t wave_code = (irtx_code_t)(carrier_num_raw & 0xFFFF);
    if (IRTX_WAVE_LEVEL_HIGH == wave_level) {
        wave_code |= IRTX_FIFO_DATA_ENTRY_HIGH_LEVEL_MASK;
    } else {
        wave_code &= (~IRTX_FIFO_DATA_ENTRY_HIGH_LEVEL_MASK);
    }

    // use pulse group0, whose freq and duty cycle configured by PWM_TMAX & PWM_TCMP
    wave_code &= (~IRTX_FIFO_DATA_ENTRY_PULSE_GROUP_SEL_MASK);
    /* // use pulse group1, whose freq and duty cycle configured by PWM_TMAX_SHADOW & PWM_TCMP_SHADOW
    wave_code |= IRTX_FIFO_DATA_ENTRY_PULSE_GROUP_SEL_MASK; */

    return wave_code;
}

/**
 *******************************************************************************
 * @brief write coded data by dma and will enter dma interrupt when write done. it can work only in dma mode
 * @param[in] buff           coded data buff to be written in irtx_code_t data type
 * @param[in] buff_num       buff number of irtx_code_t type to be written
 *
 * @return errno
 *******************************************************************************
 */
om_error_t drv_irtx_write_int(const irtx_code_t *buff, uint32_t data_num)
{
    OM_ASSERT((buff != NULL) && (data_num != 0U));

    OM_IRTX->DMA_SADDR = (uint32_t)buff;
    OM_IRTX->DMA_TRANS_LENGTH = data_num;
    OM_IRTX->PWM_EN = IRTX_PWM_EN_MASK;
    OM_IRTX->DMA_START = IRTX_DMA_START_MASK;

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief write coded data by fifo. it can work only in fifo mode
 * @param[in] buff           coded data buff to be written in irtx_code_t data type
 * @param[in] buff_num       buff number of irtx_code_t type to be written
 *
 * @return errno
 *******************************************************************************
 */
om_error_t drv_irtx_write_fifo(const irtx_code_t *buff, uint32_t buff_num)
{
    OM_ASSERT((buff != NULL) && (buff_num != 0U));

    // enable irtx
    OM_IRTX->PWM_EN = IRTX_PWM_EN_MASK;
    for (uint32_t i = 0; i < buff_num; i++) {
        while(OM_IRTX->FIFO_SR & IRTX_FIFO_SR_FIFO_FULL_MASK);
        OM_IRTX->FIFO_DATA_ENTRY = buff[i];
    }
    while(!(OM_IRTX->FIFO_SR & IRTX_FIFO_SR_FIFO_EMPTY_MASK));

    return OM_ERROR_OK;
}

#if (RTE_IRTX_REGISTER_CALLBACK)
/**
 *******************************************************************************
 * @brief Register event callback for transmit dma mode
 *
 * @param[in] cb            Pointer to callback
 *******************************************************************************
 */
void drv_irtx_register_isr_callback(drv_isr_callback_t cb)
{
    irtx_env.isr_cb = cb;
}
#endif

__WEAK void drv_irtx_isr_callback(drv_event_t event)
{
    #if (RTE_IRTX_REGISTER_CALLBACK)
    if(irtx_env.isr_cb) {
        irtx_env.isr_cb(OM_IRTX, event, NULL, NULL);
    }
    #endif
}

/**
 *******************************************************************************
 * @brief it will be called by irtx interrupt serve
 *
 *******************************************************************************
 */
void drv_irtx_isr(void)
{
    uint32_t pwm_int_status = OM_IRTX->PWM_INT_ST;
    uint8_t fifo_cnt_int_status = OM_IRTX->FIFO_CNT_INT_ST;

    // clear interrupt status
    REGW(&OM_IRTX->PWM_INT_ST, MASK_3REG(IRTX_PWM_INT_PNUM_INT, 1, IRTX_PWM_INT_DMA_MODE_INT, 1, IRTX_PWM_INT_ST_PWM_CYCLE_DONE_INT, 1));
    REGW(&OM_IRTX->FIFO_CNT_INT_ST, MASK_2REG(IRTX_FIFO_CNT_INT_ST_FIFO_CNT_INT, 1, IRTX_FIFO_CNT_INT_ST_FIFO_EMPTY_INT, 1));

    if (NULL != irtx_env.isr_cb) {
        drv_event_t event = DRV_EVENT_IRTX_FIFO_CNT;
        if (pwm_int_status & IRTX_PWM_INT_PNUM_INT_MASK) {
            event = DRV_EVENT_IRTX_PWM_INT_PNUM_INT;
        } else if (pwm_int_status & IRTX_PWM_INT_DMA_MODE_INT_MASK) {
            event = DRV_EVENT_IRTX_PWM_INT_DMA_INT;
        } else if (pwm_int_status & IRTX_PWM_INT_PWM_FRAME_INT_MASK) {
            event = DRV_EVENT_IRTX_PWM_INT_CYCLE_DONE_INT;
        } else if (fifo_cnt_int_status & IRTX_FIFO_CNT_INT_ST_FIFO_CNT_INT_MASK) {
            OM_IRTX->FIFO_CNT_INT_MASK &= (~IRTX_FIFO_CNT_INT_MASK_FIFO_CNT_MASK);
            event = DRV_EVENT_IRTX_FIFO_CNT;
        } else if (fifo_cnt_int_status & IRTX_FIFO_CNT_INT_ST_FIFO_EMPTY_INT_MASK) {
            event = DRV_EVENT_IRTX_FIFO_EMPTY_INT;
        }

        drv_irtx_isr_callback(event);
    }
}


#endif /* (RTE_IRTX) */


/** @} */

