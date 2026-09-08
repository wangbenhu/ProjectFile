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
 * @brief    example for using rgb
 * @details  example for using rgb: write
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
#define PAD_RGB_OUT                  15
#define MUX_RGB_OUT                  PINMUX_PAD15_RGB_OUT_CFG
#define RGB_DATA_BUF_TOTAL_SIZE      (60 * 3)

/*******************************************************************************
 * TYPEDEFS
 */


/*******************************************************************************
 * CONST & VARIABLES
 */
static volatile uint8_t rgb_stop = 0;

static uint32_t colors[7] = {
    0x000000FF,    //red
    0x000061FF,    //orange
    0x0000FFFF,    //yellow
    0x0000FF00,    //green
    0x00FFFF00,    //cyan
    0x00FF0000,    //blue
    0x00F020A0,    //purple
};

static uint8_t  rgb_data[RGB_DATA_BUF_TOTAL_SIZE];

/// Pinmux Configuration
static pin_config_t pin_config[] = {
    {PAD_RGB_OUT, {MUX_RGB_OUT}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
};

/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static void rgb_callback(void *om_rgb, drv_event_t drv_event, void *param0, void *param1)
{
    if (drv_event & DRV_EVENT_COMMON_WRITE_COMPLETED) {
        rgb_stop = 1;
    }
}

static void test_rgb_init(void)
{
    drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));

    for (int i = 0; i < RGB_DATA_BUF_TOTAL_SIZE/3; i++) {
        rgb_data[3 * i + 0] = (uint8_t)((colors[i%7] & 0x000000FF) >> 0);
        rgb_data[3 * i + 1] = (uint8_t)((colors[i%7] & 0x0000FF00) >> 8);
        rgb_data[3 * i + 2] = (uint8_t)((colors[i%7] & 0x00FF0000) >> 16);
    }

    rgb_stop = 0;
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief example of using rgb in blocking mode
 *
 *******************************************************************************
 */
void example_rgb_block(void)
{
    test_rgb_init();

    const rgb_config_t rgb_cfg = {
        .rgb_format       = RGB_FORMAT_RGB,
        .idle_level       = RGB_IDLE_LEVEL_LOW,
        .data_per         = 40,
        .reset_per        = 3200,  // >2560(80us)
        .duty_zero        = 10,
        .duty_one         = 30,
        .psc              = 0,
        .free_format_en   = false,
        .free_format_bits = 0,
        .ext_data_en      = false,
        .ext_data_bits    = 0,
        .ext_data         = 0,
        .tx_reset_en      = true,
    };

    drv_rgb_init(&rgb_cfg);
    drv_rgb_write((uint32_t *)rgb_data, RGB_DATA_BUF_TOTAL_SIZE/4);
}

/**
 *******************************************************************************
 * @brief example of using rgb in interrupt mode
 *
 *******************************************************************************
 */
void example_rgb_int(void)
{
    test_rgb_init();

    const rgb_config_t rgb_cfg = {
        .rgb_format       = RGB_FORMAT_RGB,
        .idle_level       = RGB_IDLE_LEVEL_LOW,
        .data_per         = 40,
        .reset_per        = 3200,  // >2560(80us)
        .duty_zero        = 10,
        .duty_one         = 30,
        .psc              = 0,
        .free_format_en   = false,
        .free_format_bits = 0,
        .ext_data_en      = false,
        .ext_data_bits    = 0,
        .ext_data         = 0,
        .tx_reset_en      = true,
    };

    drv_rgb_init(&rgb_cfg);
    drv_rgb_register_isr_callback(rgb_callback);
    drv_rgb_write_int((uint32_t *)rgb_data, RGB_DATA_BUF_TOTAL_SIZE/4);
    while(!rgb_stop);
}

/**
 *******************************************************************************
 * @brief example of using rgb in dma mode
 *
 *******************************************************************************
 */
void example_rgb_dma(void)
{
    test_rgb_init();

    const rgb_config_t rgb_cfg = {
        .rgb_format       = RGB_FORMAT_RGB,
        .idle_level       = RGB_IDLE_LEVEL_LOW,
        .data_per         = 40,
        .reset_per        = 3200,  // >2560(80us)
        .duty_zero        = 10,
        .duty_one         = 30,
        .psc              = 0,
        .free_format_en   = false,
        .free_format_bits = 0,
        .ext_data_en      = false,
        .ext_data_bits    = 0,
        .ext_data         = 0,
        .tx_reset_en      = true,
    };

    drv_rgb_init(&rgb_cfg);
    drv_rgb_register_isr_callback(rgb_callback);

    drv_rgb_gpdma_channel_allocate();
    drv_rgb_write_dma((uint32_t *)rgb_data, RGB_DATA_BUF_TOTAL_SIZE/4);
    while(!rgb_stop);
    drv_rgb_gpdma_channel_release();
}
/** @} */