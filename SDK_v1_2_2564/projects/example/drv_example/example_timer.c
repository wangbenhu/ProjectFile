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
 * @brief    example for using timer
 * @details  example for using timer: using timer to count, output the pwm, capture
 *           the signal
 * @version
 *
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


/*******************************************************************************
 * INCLUDES
 */
#include <string.h>
#include "om_driver.h"


/*******************************************************************************
 * MACROS
 */
/// Test pad for timer counter
#define PAD_TIM0_COUNT          14
/// Test mux for timer counter
#define MUX_TIM0_COUNT          PINMUX_PAD14_GPIO_MODE_CFG
/// Test pad for timer counter
#define PAD_TIM1_COUNT          15
/// Test mux for timer counter
#define MUX_TIM1_COUNT          PINMUX_PAD15_GPIO_MODE_CFG
/// Test pad for timer counter
#define PAD_TIM2_COUNT          16
/// Test mux for timer counter
#define MUX_TIM2_COUNT          PINMUX_PAD16_GPIO_MODE_CFG

/// Test pad for timer pwm
#define PAD_TIM1_PWM_CH0        12
#define PAD_TIM1_PWM_CH1        13
#define PAD_TIM1_PWM_CH2        14
#define PAD_TIM1_PWM_CH3        15
/// Test pad for timer pwm complementary
#define PAD_TIM1_PWM_CH0N       5
/// Test mux for timer pwm
#define MUX_TIM1_PWM_CH0        PINMUX_PAD12_TIM1_OUT0_CFG
#define MUX_TIM1_PWM_CH1        PINMUX_PAD13_TIM1_OUT1_CFG
#define MUX_TIM1_PWM_CH2        PINMUX_PAD14_TIM1_OUT2_CFG
#define MUX_TIM1_PWM_CH3        PINMUX_PAD15_TIM1_OUT3_CFG
/// Test mux for timer pwm complementary
#define MUX_TIM1_PWM_CH0N       PINMUX_PAD5_TIM1_OUTN0_CFG
/// Test pad for timer capature
#define PAD_TIM0_CAP_CH0        16
/// Test mux for timer capature
#define MUX_TIM0_CAP_CH0        PINMUX_PAD16_TIM0_IO0_CFG

/// Number of output compare val
#define PWM_DMA_OC_VAL_NUM      30
/// Number of capture
#define CAP_SAMPLE_NUM          30


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    uint8_t     cap_index;
    uint32_t    cap_overflow;
    uint32_t    cap_buf[CAP_SAMPLE_NUM];
} cap_env_t;

typedef struct {
    gpdma_chain_trans_t chain[3];
    uint32_t buf0[CAP_SAMPLE_NUM];
    uint32_t buf1[CAP_SAMPLE_NUM];
    uint32_t buf2[CAP_SAMPLE_NUM];
} cap_dma_env_t;

typedef struct {
    gpdma_chain_trans_t chain[3];
    uint32_t buf0[PWM_DMA_OC_VAL_NUM];
    uint32_t buf1[PWM_DMA_OC_VAL_NUM];
    uint32_t buf2[PWM_DMA_OC_VAL_NUM];
} pwm_dma_env_t;

typedef struct {
    uint32_t cap_overflow;
    uint32_t cap_cc1_index;
    uint32_t cap_cc2_index;
    uint32_t cap_cc1_buf[CAP_SAMPLE_NUM];
    uint32_t cap_cc2_buf[CAP_SAMPLE_NUM];
} pwm_input_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
static cap_env_t        cap_env;
static cap_dma_env_t    cap_dma_env;
static pwm_dma_env_t    pwm_dma_env;
static pwm_input_env_t  pwm_input_env;


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief timer counter callback, when the timer generates an update event, toggle
 * the PAD_TIM0_COUNT
 *
 * @param[in] om_tim    Pointer to timer
 * @param[in] event     Timer event
 * @param[in] buff      Pointer to data
 * @param[in] num       Number of data
 *
 *******************************************************************************
 */
static void tim_count_callback(void *om_tim, drv_event_t event, void *buff, void *num)
{
    if (event == DRV_EVENT_TIM_UPDATE) {
        drv_gpio_toggle(OM_GPIO0, BITMASK(PAD_TIM0_COUNT));
    }
}

/**
 *******************************************************************************
 * @brief timer capture callback, The captured data will be put into cap_buf[]
 *
 * @param[in] om_tim    Pointer to timer
 * @param[in] event     Timer event
 * @param[in] buff      Pointer to data
 * @param[in] num       Number of data
 *
 *******************************************************************************
 */
static void tim_capture_callback(void *om_tim, drv_event_t event, void *buff, void *num)
{
    if (event & DRV_EVENT_TIM_UPDATE) {
        cap_env.cap_overflow++;
    }

    if (cap_env.cap_index < CAP_SAMPLE_NUM && event & DRV_EVENT_TIM_CC1) {
        cap_env.cap_buf[cap_env.cap_index++] = (uint32_t)buff + cap_env.cap_overflow * 0xFFFF;
    }
}

/**
 *******************************************************************************
 * @brief timer pwm input callback, The captured data of the falling and rising
 * edges will be put into cap_cc1_buf, cap_cc2_buf respectively.
 *
 * @param[in] om_tim    Pointer to timer
 * @param[in] event     Timer event
 * @param[in] buff      Pointer to data
 * @param[in] num       Number of data
 *
 *******************************************************************************
 */
static void tim_pwm_input_callback(void *om_tim, drv_event_t event, void *buff, void *num)
{
    if (event & DRV_EVENT_TIM_TRIGGER) {
        pwm_input_env.cap_overflow = 0;
    }

    if (event & DRV_EVENT_TIM_UPDATE) {
        pwm_input_env.cap_overflow++;
    }

    if (pwm_input_env.cap_cc1_index < CAP_SAMPLE_NUM && (event & DRV_EVENT_TIM_CC1)) {
        pwm_input_env.cap_cc1_buf[pwm_input_env.cap_cc1_index++] = (uint32_t)buff + pwm_input_env.cap_overflow * 0xFFFF;
    }

    if (pwm_input_env.cap_cc2_index < CAP_SAMPLE_NUM && (event & DRV_EVENT_TIM_CC2)) {
        pwm_input_env.cap_cc2_buf[pwm_input_env.cap_cc2_index++] = (uint32_t)buff + pwm_input_env.cap_overflow * 0xFFFF;
    }
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief example of using timer as counter, toggle the pin level every 500ms
 *
 *******************************************************************************
 */
void example_tim_count(void)
{
    pin_config_t pin_cfg[] = {
        {PAD_TIM0_COUNT, {MUX_TIM0_COUNT}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    };
    gpio_config_t gpio_cfg[] = {
        {OM_GPIO0, PAD_TIM0_COUNT, GPIO_DIR_OUTPUT, GPIO_LEVEL_LOW, GPIO_TRIG_NONE},
    };
    drv_pin_init(pin_cfg, sizeof(pin_cfg) / sizeof(pin_cfg[0]));
    drv_gpio_init(gpio_cfg, sizeof(gpio_cfg) / sizeof(gpio_cfg[0]));

    drv_tim_init(OM_TIM1);
    drv_tim_register_isr_callback(OM_TIM1, tim_count_callback);

    tim_gp_config_t tim_cfg = {
        .period_us = 0xf424,
        .delay_period = 7,
    };

    drv_tim_gp_start(OM_TIM1, &tim_cfg);

    /* if you want to stop the timer, just use drv_tim_gp_stop(OM_TIMx) */
}

/**
 *******************************************************************************
 * @brief example of using timer output pwm, the timer1's channel1 will output pwm
 * waveform:period is 1ms, high level is 300us, low level is 700us
 *
 *******************************************************************************
 */
void example_tim_pwm(void)
{
    pin_config_t pin_cfg[] = {
        {PAD_TIM1_PWM_CH0, {MUX_TIM1_PWM_CH0}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    };
    drv_pin_init(pin_cfg, sizeof(pin_cfg) / sizeof(pin_cfg[0]));

    tim_pwm_output_config_t pwm_config = {
        .cnt_freq = 1000000,            // 1Mhz
        .chan = {
            {0, {TIM_PWM_POL_ACTIVE_HIGH, 0}},
            {0, {TIM_PWM_POL_ACTIVE_HIGH, 0}},
            {0, {TIM_PWM_POL_ACTIVE_HIGH, 0}},
            {0, {TIM_PWM_POL_ACTIVE_HIGH, 0}}
        },
        .gpdma_cfg = {
            0, TIM_CHAN_ALL, NULL
        },
    };

    pwm_config.period_cnt                   = 1000;
    pwm_config.chan[TIM_CHAN_1].en          = 1;
    pwm_config.chan[TIM_CHAN_1].cfg.oc_val  = 300;
    pwm_config.chan[TIM_CHAN_1].cfg.pol     = TIM_PWM_POL_ACTIVE_HIGH;

    drv_tim_init(OM_TIM1);
    drv_tim_pwm_output_start(OM_TIM1, &pwm_config);

    /**
     * if you want to stop the timer, just use drv_tim_pwm_output_stop(...).
     * if you want to change the output compare value, just use drv_tim_pwm_change_oc_val(...)
     */
}

/**
 *******************************************************************************
 * @brief example of using timer output pwm with complementary and dead time, the timer1's channel1 will output pwm
 * waveform:period is 1ms, high level is 300us, low level is 700us,and output pwm complementary waveform:period is 1ms,
 * high level is 700us, low level is 300us,and the dead time is 3*(1/32M) = 93.75 ns(TIM_BDTR DTG=3,dead_time=DTG[7.0] * Tdtg,Tdtg=tDTS,TIM_CR1 CKD=0x0: tDTS =tCK_INT)
 *******************************************************************************
 */
void example_tim_pwm_complementary(void)
{
    pin_config_t pin_cfg[] = {
        {PAD_TIM1_PWM_CH0, {MUX_TIM1_PWM_CH0}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
        {PAD_TIM1_PWM_CH0N, {MUX_TIM1_PWM_CH0N}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    };
    drv_pin_init(pin_cfg, sizeof(pin_cfg) / sizeof(pin_cfg[0]));

    tim_pwm_complementary_output_config_t pwm_config = {
        .cnt_freq = 1000000,            // 1Mhz
        .chan = {
            {0, {TIM_PWM_POL_ACTIVE_HIGH, 0}},
            {0, {TIM_PWM_POL_ACTIVE_HIGH, 0}},
            {0, {TIM_PWM_POL_ACTIVE_HIGH, 0}},
            {0, {TIM_PWM_POL_ACTIVE_HIGH, 0}}
        },
        .gpdma_cfg = {
            0, TIM_CHAN_ALL, NULL
        },
    };

    pwm_config.period_cnt                   = 1000;
    pwm_config.chan[TIM_CHAN_1].en          = 1;
    pwm_config.chan[TIM_CHAN_1].cfg.oc_val  = 300;
    pwm_config.chan[TIM_CHAN_1].cfg.pol     = TIM_PWM_POL_ACTIVE_HIGH;
    pwm_config.chan[TIM_CHAN_1].cfg.complementary_output_enable = true;
    pwm_config.dead_time = 3;
    drv_tim_init(OM_TIM1);
    drv_tim_pwm_complementary_output_start(OM_TIM1, &pwm_config);
    /**
     * if you want to stop the timer, just use drv_tim_pwm_output_stop(...).
     * if you want to change the output compare value, just use drv_tim_pwm_change_oc_val(...)
     */
}

/**
 *******************************************************************************
 * @brief example of using timer+dma ouput pwm with adjustable duty cycle. The
 * duty cycle is stored in pwm_dma_oc_val and gradually increased from 10% to 68%
 * at 2% intervals
 *
 *******************************************************************************
 */
void example_tim_pwm_dma(void)
{
    pin_config_t pin_cfg[] = {
        {PAD_TIM1_PWM_CH0, {MUX_TIM1_PWM_CH0}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    };
    drv_pin_init(pin_cfg, sizeof(pin_cfg) / sizeof(pin_cfg[0]));

    pwm_dma_env.chain[0].src_addr   = (uint32_t)pwm_dma_env.buf0;
    pwm_dma_env.chain[0].size_byte  = PWM_DMA_OC_VAL_NUM * sizeof(uint32_t);
    pwm_dma_env.chain[0].ll_ptr     = &pwm_dma_env.chain[1];

    pwm_dma_env.chain[1].src_addr   = (uint32_t)pwm_dma_env.buf1;
    pwm_dma_env.chain[1].size_byte  = PWM_DMA_OC_VAL_NUM * sizeof(uint32_t);
    pwm_dma_env.chain[1].ll_ptr     = &pwm_dma_env.chain[2];

    pwm_dma_env.chain[2].src_addr   = (uint32_t)pwm_dma_env.buf2;
    pwm_dma_env.chain[2].size_byte  = PWM_DMA_OC_VAL_NUM * sizeof(uint32_t);
    pwm_dma_env.chain[2].ll_ptr     = &pwm_dma_env.chain[0];

    for (uint16_t i = 0; i < PWM_DMA_OC_VAL_NUM; i++) {
        pwm_dma_env.buf0[i] = 200 + i;
        pwm_dma_env.buf1[i] = 400 + i;
        pwm_dma_env.buf2[i] = 600 + i;
    }

    tim_pwm_output_config_t pwm_config = {
        .cnt_freq = 1000000,                // 1Mhz
        .period_cnt = 1000,
        .chan = {
            {0, {TIM_PWM_POL_ACTIVE_HIGH, 0}},
            {0, {TIM_PWM_POL_ACTIVE_HIGH, 0}},
            {0, {TIM_PWM_POL_ACTIVE_HIGH, 0}},
            {0, {TIM_PWM_POL_ACTIVE_HIGH, 0}}
        },
        .gpdma_cfg = {
            .en = 1,
            .tim_chan = TIM_CHAN_1,
            .chain      = &pwm_dma_env.chain[0]
        },
    };

    pwm_config.chan[TIM_CHAN_1].en   = 1;

    drv_tim_init(OM_TIM1);
    drv_tim_gpdma_channel_allocate(OM_TIM1);
    drv_tim_pwm_output_start(OM_TIM1, &pwm_config);

    /* if you want to stop the timer, just use drv_tim_pwm_output_stop(...) and release dma channel. */
}

/**
 *******************************************************************************
 * @brief example of using timer capture rising edge. The capture value is stored
 * in cap_env.cap_buf[]
 *
 *******************************************************************************
 */
void example_tim_capture(void)
{
    pin_config_t pin_config[] = {
        {PAD_TIM0_CAP_CH0, {MUX_TIM0_CAP_CH0}, PMU_PIN_MODE_FLOAT, PMU_PIN_DRIVER_CURRENT_NORMAL},
    };
    drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));

    tim_capture_config_t capture_config = {
        .cnt_freq = 1000000,
        .chan = {
            {0, TIM_CAPTURE_POLARITY_RISING_EDGE},
            {0, TIM_CAPTURE_POLARITY_RISING_EDGE},
            {0, TIM_CAPTURE_POLARITY_RISING_EDGE},
            {0, TIM_CAPTURE_POLARITY_RISING_EDGE}
        },
        .gpdma_cfg = {
            .en       = 0,
            .tim_chan = TIM_CHAN_ALL,
            .chain    = NULL
        },
    };
    capture_config.chan[TIM_CHAN_1].en = 1;

    drv_tim_init(OM_TIM0);
    drv_tim_register_isr_callback(OM_TIM0, tim_capture_callback);
    drv_tim_capture_start(OM_TIM0, &capture_config);

    /* if you want to stop the timer, just use drv_tim_capture_stop(...) */
}

/**
 *******************************************************************************
 * @brief example of using timer+dma capture rising edge. The captured data is
 * placed one by one into cap_dma_env.buf0->buf1->buf2->buf0......
 *
 *******************************************************************************
 */
void example_tim_dma_capture(void)
{
    pin_config_t pin_config[] = {
        {PAD_TIM0_CAP_CH0, {MUX_TIM0_CAP_CH0}, PMU_PIN_MODE_FLOAT, PMU_PIN_DRIVER_CURRENT_NORMAL},
    };
    drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));

    cap_dma_env.chain[0].dst_addr   = (uint32_t)cap_dma_env.buf0;
    cap_dma_env.chain[0].size_byte  = CAP_SAMPLE_NUM * sizeof(uint32_t);
    cap_dma_env.chain[0].ll_ptr     = &cap_dma_env.chain[1];

    cap_dma_env.chain[1].dst_addr   = (uint32_t)cap_dma_env.buf1;
    cap_dma_env.chain[1].size_byte  = CAP_SAMPLE_NUM * sizeof(uint32_t);
    cap_dma_env.chain[1].ll_ptr     = &cap_dma_env.chain[2];

    cap_dma_env.chain[2].dst_addr   = (uint32_t)cap_dma_env.buf2;
    cap_dma_env.chain[2].size_byte  = CAP_SAMPLE_NUM * sizeof(uint32_t);
    cap_dma_env.chain[2].ll_ptr     = &cap_dma_env.chain[0];

    tim_capture_config_t capture_config = {
        .cnt_freq = 1000000,
        .chan = {
            {0, TIM_CAPTURE_POLARITY_RISING_EDGE},
            {0, TIM_CAPTURE_POLARITY_RISING_EDGE},
            {0, TIM_CAPTURE_POLARITY_RISING_EDGE},
            {0, TIM_CAPTURE_POLARITY_RISING_EDGE}
        },
        .gpdma_cfg = {
            .en = 1,
            .tim_chan = TIM_CHAN_1,
            .chain = &cap_dma_env.chain[0]
        }
    };
    capture_config.chan[TIM_CHAN_1].en = 1;

    drv_tim_init(OM_TIM0);
    drv_tim_gpdma_channel_allocate(OM_TIM0);
    drv_tim_capture_start(OM_TIM0, &capture_config);

    /* if you want to stop the timer, just use drv_tim_capture_stop(...) and release dma channel*/
}

/**
 *******************************************************************************
 * @brief example of using timer pwm input mode. TI1FP1 captures the falling edge,
 * TI1FP2 captures the rising edge, when TI1FP1 captures, the tim counter is reset.
 *
 *******************************************************************************
 */
void example_tim_pwm_input(void)
{
    pin_config_t pin_config[] = {
        {PAD_TIM0_CAP_CH0, {MUX_TIM0_CAP_CH0}, PMU_PIN_MODE_FLOAT, PMU_PIN_DRIVER_CURRENT_NORMAL},
    };
    drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));

    tim_pwm_input_config_t cfg = {
        .cnt_freq = 1000000
    };

    drv_tim_init(OM_TIM0);
    drv_tim_register_isr_callback(OM_TIM0, tim_pwm_input_callback);
    drv_tim_pwm_input_start(OM_TIM0, &cfg);

    /* if you want to stop the timer, just use drv_tim_pwm_input_stop(...) */
}

/** @} */