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
 * @brief    example for using gpadc
 * @details  example for using gpadc: read blocking, interrupt, and DMA
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
#include "om_driver.h"


/*******************************************************************************
 * MACROS
 */
#define TEST_SIZE                 20
#define PAD_GPADC_CH_GPIO2        2
#define MUX_GPADC_CH_GPIO2        PINMUX_PAD2_INPUT_MODE_CFG
#define PAD_GPADC_CH_GPIO3        3
#define MUX_GPADC_CH_GPIO3        PINMUX_PAD3_INPUT_MODE_CFG
#define PAD_GPADC_CH_GPIO8        8
#define MUX_GPADC_CH_GPIO8        PINMUX_PAD8_INPUT_MODE_CFG

/*******************************************************************************
 * TYPEDEFS
 */


/*******************************************************************************
 * CONST & VARIABLES
 */
/// Pin configuration
static const pin_config_t pin_config[] = {
    {PAD_GPADC_CH_GPIO2, {MUX_GPADC_CH_GPIO2}, PMU_PIN_MODE_FLOAT, PMU_PIN_DRIVER_CURRENT_NORMAL},
};

static const pin_config_t multi_pin_config[] = {
    {PAD_GPADC_CH_GPIO2, {MUX_GPADC_CH_GPIO2}, PMU_PIN_MODE_FLOAT, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_GPADC_CH_GPIO3, {MUX_GPADC_CH_GPIO3}, PMU_PIN_MODE_FLOAT, PMU_PIN_DRIVER_CURRENT_NORMAL},
};

static const pin_config_t diff_pin_config[] = {
    {PAD_GPADC_CH_GPIO2, {MUX_GPADC_CH_GPIO2}, PMU_PIN_MODE_FLOAT, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_GPADC_CH_GPIO8, {MUX_GPADC_CH_GPIO8}, PMU_PIN_MODE_FLOAT, PMU_PIN_DRIVER_CURRENT_NORMAL},
};

/// Buffer that stores the data to be received
static int16_t out[TEST_SIZE];
static int16_t out_multi[3*TEST_SIZE];
/// Read finish flag
static volatile uint8_t gpadc_done;

/*******************************************************************************
 * LOCAL FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief callback for GPADC interrupt
 *
 * @param[in] om_gpadc    Pointer to GPADC
 * @param[in] event       GPADC event
 *                        - DRV_EVENT_COMMON_READ_COMPLETED
 *                        - DRV_EVENT_COMMON_ABORT
 *                        - DRV_EVENT_COMMON_ERROR
 * @param[in] read_buf    Pointer to receive buffer
 * @param[in] read_cnt    The number of received data
 *
 *******************************************************************************
 */
static void gpadc_read_cb(void *om_gpadc, drv_event_t event, void *read_buf, void *read_cnt)
{
    if (event == DRV_EVENT_COMMON_READ_COMPLETED) {
        gpadc_done = 1;
    }
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief example of read in blocking mode
 *
 *******************************************************************************
 */
void example_gpadc_read(void)
{
    drv_gpadc_config_t config;
    config.channel_p = GPADC_CH_P_GPIO2;
    config.channel_n = GPADC_CH_N_AVSS;
    config.mode = GPADC_MODE_SINGLE;
    config.gain = GPADC_GAIN_1_3_INTERNAL_REF;
    config.sum_num = GPADC_SUM_NUM_256;
    config.sampling_cycles = GPADC_SAMPLING_CYCLES_128;

    drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));
    drv_gpadc_init(&config);

    drv_gpadc_read(config.channel_p, &out[0], TEST_SIZE);

    drv_gpadc_control(GPADC_CONTROL_DISABLE_CLOCK, NULL);
}

/**
 *******************************************************************************
 * @brief example of read in interrupt mode
 *
 *******************************************************************************
 */
void example_gpadc_read_int(void)
{
    drv_gpadc_config_t config;
    config.channel_p = GPADC_CH_P_GPIO2;
    config.channel_n = GPADC_CH_N_AVSS;
    config.mode = GPADC_MODE_SINGLE;
    config.gain = GPADC_GAIN_1_3_INTERNAL_REF;
    config.sum_num = GPADC_SUM_NUM_256;
    config.sampling_cycles = GPADC_SAMPLING_CYCLES_128;

    drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));
    drv_gpadc_init(&config);
    drv_gpadc_register_isr_callback(gpadc_read_cb);

    drv_gpadc_read_int(config.channel_p, &out[0], TEST_SIZE);

    while(!gpadc_done);
    gpadc_done = 0;

    drv_gpadc_control(GPADC_CONTROL_DISABLE_CLOCK, NULL);
}

/**
 *******************************************************************************
 * @brief example of read in dma mode
 *
 *******************************************************************************
 */
void example_gpadc_read_dma(void)
{
#if (RTE_GPDMA)
    drv_gpadc_config_t config;
    config.channel_p = GPADC_CH_P_GPIO2;
    config.channel_n = GPADC_CH_N_AVSS;
    config.mode = GPADC_MODE_SINGLE;
    config.gain = GPADC_GAIN_1_3_INTERNAL_REF;
    config.sum_num = GPADC_SUM_NUM_256;
    config.sampling_cycles = GPADC_SAMPLING_CYCLES_128;

    drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));
    drv_gpadc_init(&config);
    drv_gpadc_register_isr_callback(gpadc_read_cb);

    drv_gpadc_gpdma_channel_allocate();
    drv_gpadc_read_dma(config.channel_p, &out[0], TEST_SIZE);

    while(!gpadc_done);
    gpadc_done = 0;

    drv_gpadc_gpdma_channel_release();
    drv_gpadc_control(GPADC_CONTROL_DISABLE_CLOCK, NULL);
#endif
}

/**
 *******************************************************************************
 * @brief example of read temperature in blocking mode
 *
 *******************************************************************************
 */
void example_gpadc_read_temperature(void)
{
    drv_gpadc_config_t config;
    config.channel_p = GPADC_CH_P_TEMPERATURE;
    config.channel_n = GPADC_CH_N_AVSS;
    config.mode = GPADC_MODE_SINGLE;
    config.gain = GPADC_GAIN_1_3_INTERNAL_REF;
    config.sum_num = GPADC_SUM_NUM_256;
    config.sampling_cycles = GPADC_SAMPLING_CYCLES_128;

    drv_gpadc_init(&config);

    drv_gpadc_read(config.channel_p, &out[0], TEST_SIZE);

    drv_gpadc_control(GPADC_CONTROL_DISABLE_CLOCK, NULL);
}

/**
 *******************************************************************************
 * @brief example of read vbat in blocking mode
 *
 *******************************************************************************
 */
void example_gpadc_read_battery(void)
{
    drv_gpadc_config_t config;
    config.channel_p = GPADC_CH_P_VBAT;
    config.channel_n = GPADC_CH_N_AVSS;
    config.mode = GPADC_MODE_SINGLE;
    config.gain = GPADC_GAIN_1_3_INTERNAL_REF;
    config.sum_num = GPADC_SUM_NUM_256;
    config.sampling_cycles = GPADC_SAMPLING_CYCLES_128;

    drv_gpadc_init(&config);

    drv_gpadc_read(config.channel_p, &out[0], TEST_SIZE);

    drv_gpadc_control(GPADC_CONTROL_DISABLE_CLOCK, NULL);
}

/**
 *******************************************************************************
* @brief example of read multi channels in blocking mode
         NOTE: do not support (GPADC_CH_TEMPERATURE | GPADC_CH_VBAT) or (GPADC_CH_TEMPERATURE | GPADC_CH_GPIOx)
 *
 *******************************************************************************
 */
void example_gpadc_read_multi(void)
{
    drv_gpadc_config_t config;
    config.channel_p = GPADC_CH_P_VBAT | GPADC_CH_P_GPIO2 | GPADC_CH_P_GPIO3;
    config.channel_n = GPADC_CH_N_AVSS;
    config.mode = GPADC_MODE_SINGLE;
    config.gain = GPADC_GAIN_1_3_INTERNAL_REF;
    config.sum_num = GPADC_SUM_NUM_256;
    config.sampling_cycles = GPADC_SAMPLING_CYCLES_128;

    drv_pin_init(multi_pin_config, sizeof(multi_pin_config) / sizeof(multi_pin_config[0]));
    drv_gpadc_init(&config);

    drv_gpadc_read(config.channel_p, &out_multi[0], 3*TEST_SIZE);

    drv_gpadc_control(GPADC_CONTROL_DISABLE_CLOCK, NULL);
}

/**
 *******************************************************************************
* @brief example of read multi channels in interrupt mode
         NOTE: do not support (GPADC_CH_TEMPERATURE | GPADC_CH_VBAT) or (GPADC_CH_TEMPERATURE | GPADC_CH_GPIOx)
 *
 *******************************************************************************
 */
void example_gpadc_read_int_multi(void)
{
    drv_gpadc_config_t config;
    config.channel_p = GPADC_CH_P_VBAT | GPADC_CH_P_GPIO2 | GPADC_CH_P_GPIO3;
    config.channel_n = GPADC_CH_N_AVSS;
    config.mode = GPADC_MODE_SINGLE;
    config.gain = GPADC_GAIN_1_3_INTERNAL_REF;
    config.sum_num = GPADC_SUM_NUM_256;
    config.sampling_cycles = GPADC_SAMPLING_CYCLES_128;

    drv_pin_init(multi_pin_config, sizeof(multi_pin_config) / sizeof(multi_pin_config[0]));
    drv_gpadc_init(&config);
    drv_gpadc_register_isr_callback(gpadc_read_cb);

    drv_gpadc_read_int(config.channel_p, &out_multi[0], 3*TEST_SIZE);

    while(!gpadc_done);
    gpadc_done = 0;

    drv_gpadc_control(GPADC_CONTROL_DISABLE_CLOCK, NULL);
}

 /**
 *******************************************************************************
* @brief example of read multi channels in dma mode
         NOTE: do not support (GPADC_CH_TEMPERATURE | GPADC_CH_VBAT) or (GPADC_CH_TEMPERATURE | GPADC_CH_GPIOx)
 *
 *******************************************************************************
 */
void example_gpadc_read_dma_multi(void)
{
#if (RTE_GPDMA)
    drv_gpadc_config_t config;
    config.channel_p = GPADC_CH_P_VBAT | GPADC_CH_P_GPIO2 | GPADC_CH_P_GPIO3;
    config.channel_n = GPADC_CH_N_AVSS;
    config.mode = GPADC_MODE_SINGLE;
    config.gain = GPADC_GAIN_1_3_INTERNAL_REF;
    config.sum_num = GPADC_SUM_NUM_256;
    config.sampling_cycles = GPADC_SAMPLING_CYCLES_128;

    drv_pin_init(multi_pin_config, sizeof(multi_pin_config) / sizeof(multi_pin_config[0]));
    drv_gpadc_init(&config);
    drv_gpadc_register_isr_callback(gpadc_read_cb);

    drv_gpadc_gpdma_channel_allocate();
    drv_gpadc_read_dma(config.channel_p, &out_multi[0], 3*TEST_SIZE);

    while(!gpadc_done);
    gpadc_done = 0;

    drv_gpadc_gpdma_channel_release();
    drv_gpadc_control(GPADC_CONTROL_DISABLE_CLOCK, NULL);
#endif
}
/**
 *******************************************************************************
 * @brief example of read in blocking mode by difference mode
 *
 *******************************************************************************
 */
void example_gpadc_read_diff(void)
{
    drv_gpadc_config_t config;
    config.channel_p = GPADC_CH_P_GPIO2;
    config.channel_n = GPADC_CH_N_GPIO8;
    config.mode = GPADC_MODE_DIFF;
    config.gain = GPADC_GAIN_1_3_INTERNAL_REF;
    config.sum_num = GPADC_SUM_NUM_256;
    config.sampling_cycles = GPADC_SAMPLING_CYCLES_128;

    drv_pin_init(diff_pin_config, sizeof(diff_pin_config) / sizeof(diff_pin_config[0]));
    drv_gpadc_init(&config);

    drv_gpadc_read(config.channel_p, &out[0], TEST_SIZE);

    drv_gpadc_control(GPADC_CONTROL_DISABLE_CLOCK, NULL);
}

/** @} */