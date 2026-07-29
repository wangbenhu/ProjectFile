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
 * @brief    example for using ir
 * @details  example for using ir:
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
#define PAD_IRTX_OUT  16
#define MUX_IRTX_OUT  PINMUX_PAD16_IRTX_OUT_CFG

/*******************************************************************************
 * TYPEDEFS
 */


/*******************************************************************************
 * CONST & VARIABLES
 */
static const pin_config_t pin_cfg [] = {
    {PAD_IRTX_OUT, {MUX_IRTX_OUT}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
};

/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static void irtx_cb(void *om_reg, drv_event_t event, void *param0, void *param1)
{
    switch (event) {
        case DRV_EVENT_IRTX_PWM_INT_PNUM_INT:
            om_printf("pnum cb\n");
            break;
        case DRV_EVENT_IRTX_PWM_INT_DMA_INT:
            om_printf("dma cb\n");
            break;
        case DRV_EVENT_IRTX_PWM_INT_CYCLE_DONE_INT:
            om_printf("pwm cycle done\n");
            break;
        case DRV_EVENT_IRTX_FIFO_CNT:
            om_printf("fifo cnt cb\n");
            break;
        case DRV_EVENT_IRTX_FIFO_EMPTY_INT:
            om_printf("fifo empty cb\n");
            break;

        default:
            break;
    }
}

static uint16_t drv_irtx_get_group1_wave_code(uint32_t wave_len_us, irtx_wave_level_t wave_level)
{
    irtx_code_t carrier_freq_group1 = 50000; // 50kHz
    uint32_t carrier_freq_kHz = carrier_freq_group1 / 1000;
    uint32_t carrier_num_x1000 = wave_len_us * carrier_freq_kHz;
    uint32_t max_carrier_num = 0x3FFF;
    uint32_t max_carrier_num_x1000 = max_carrier_num * 1000;

    (void)carrier_num_x1000;
    (void)max_carrier_num;
    (void)max_carrier_num_x1000;
    OM_ASSERT(carrier_num_x1000 <= max_carrier_num_x1000);

    uint32_t wave_code_raw = wave_len_us * 50000 / 1000000;
    uint16_t wave_code = wave_code_raw & 0xFFFF;

    if (IRTX_WAVE_LEVEL_HIGH == wave_level) {
        wave_code |= IRTX_FIFO_DATA_ENTRY_HIGH_LEVEL_MASK;
    } else {
        wave_code &= (~IRTX_FIFO_DATA_ENTRY_HIGH_LEVEL_MASK);
    }

    // use pulse group1, whose freq and duty cycle configured by PWM_TMAX_SHADOW & PWM_TCMP_SHADOW
    wave_code |= IRTX_FIFO_DATA_ENTRY_PULSE_GROUP_SEL_MASK;

    return wave_code;
}

static void irtx_wave_code_init(irtx_code_t *wave_buff, bool multi_freq)
{
    irtx_code_t preamble_high;
    irtx_code_t preamble_low;
    irtx_code_t b0_high;
    irtx_code_t b0_low;
    irtx_code_t b1_high;
    irtx_code_t b1_low;
    irtx_code_t repeat0_high;
    irtx_code_t repeat0_low;
    irtx_code_t repeat1_high;
    irtx_code_t repeat1_low;
    irtx_code_t stop_high;
    irtx_code_t stop_low;

    if (multi_freq) {
        preamble_high = drv_irtx_get_group1_wave_code(9000, IRTX_WAVE_LEVEL_HIGH);
        preamble_low = drv_irtx_get_group1_wave_code(4500, IRTX_WAVE_LEVEL_LOW);
        b0_high = drv_irtx_get_group1_wave_code(560, IRTX_WAVE_LEVEL_HIGH);
        b0_low = drv_irtx_get_group1_wave_code(560, IRTX_WAVE_LEVEL_LOW);
        b1_high = drv_irtx_get_group1_wave_code(560, IRTX_WAVE_LEVEL_HIGH);
        b1_low = drv_irtx_get_group1_wave_code(1690, IRTX_WAVE_LEVEL_LOW);
        stop_high = drv_irtx_get_group1_wave_code(560, IRTX_WAVE_LEVEL_HIGH);
        stop_low = drv_irtx_get_group1_wave_code(39840, IRTX_WAVE_LEVEL_LOW);
        repeat0_high = drv_irtx_get_group1_wave_code(9000, IRTX_WAVE_LEVEL_HIGH);
        repeat0_low = drv_irtx_get_group1_wave_code(2250, IRTX_WAVE_LEVEL_LOW);
        repeat1_high = drv_irtx_get_group1_wave_code(560, IRTX_WAVE_LEVEL_HIGH);
        repeat1_low = drv_irtx_get_group1_wave_code(96190, IRTX_WAVE_LEVEL_LOW);
    } else {
        preamble_high = drv_irtx_get_wave_code(9000, IRTX_WAVE_LEVEL_HIGH);
        preamble_low = drv_irtx_get_wave_code(4500, IRTX_WAVE_LEVEL_LOW);
        b0_high = drv_irtx_get_wave_code(560, IRTX_WAVE_LEVEL_HIGH);
        b0_low = drv_irtx_get_wave_code(560, IRTX_WAVE_LEVEL_LOW);
        b1_high = drv_irtx_get_wave_code(560, IRTX_WAVE_LEVEL_HIGH);
        b1_low = drv_irtx_get_wave_code(1690, IRTX_WAVE_LEVEL_LOW);
        stop_high = drv_irtx_get_wave_code(560, IRTX_WAVE_LEVEL_HIGH);
        stop_low = drv_irtx_get_wave_code(39840, IRTX_WAVE_LEVEL_LOW);
        repeat0_high = drv_irtx_get_wave_code(9000, IRTX_WAVE_LEVEL_HIGH);
        repeat0_low = drv_irtx_get_wave_code(2250, IRTX_WAVE_LEVEL_LOW);
        repeat1_high = drv_irtx_get_wave_code(560, IRTX_WAVE_LEVEL_HIGH);
        repeat1_low = drv_irtx_get_wave_code(96190, IRTX_WAVE_LEVEL_LOW);
    }

    /* prepare nec wave encoding of the button "0" on the remote control */
    // wave encoding of preamble
    wave_buff[0] = preamble_high;
    wave_buff[1] = preamble_low;
    // wave encoding of the "0" button data component
    uint32_t key0_data = 0xE916FF00;  // the data component for the "0" button: addr + addr_bar + cmd + cmd_bar = 4byte
    // transport every bit of the data component into irtx wave code
    for (uint32_t i = 0; i < 32; i++) {
        if (1 == ((key0_data >> i) & 0x1)) {
            wave_buff[2 * i + 0 + 2] = b1_high;
            wave_buff[2 * i + 1 + 2] = b1_low;
        } else {
            wave_buff[2 * i + 0 + 2] = b0_high;
            wave_buff[2 * i + 1 + 2] = b0_low;
        }
    }

    // wave encoding of stop
    wave_buff[1*2 + 32*2] = stop_high;
    wave_buff[1*2 + 32*2 + 1] = stop_low;
    // wave encoding of repeat
    wave_buff[1*2 + 32*2 + 2] = repeat0_high;
    wave_buff[1*2 + 32*2 + 3] = repeat0_low;
    wave_buff[1*2 + 32*2 + 4] = repeat1_high;
    wave_buff[1*2 + 32*2 + 5] = repeat1_low;
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief example of using ir
 *
 *******************************************************************************
 */
void example_ir(void)
{
    drv_pin_init(pin_cfg, sizeof(pin_cfg) / sizeof(pin_cfg[0]));

    irtx_config_t irtx_config = {
        .carrier_freq = 38000,
        .carrier_duty_cycle = 350, // 35.0% = 350‰
        .invert = IRTX_OUTPUT_INVERT_DISABLE,
        .polarity = IRTX_CARRIER_POLARITY_HIGH,
    };
    drv_irtx_init(&irtx_config);
    drv_irtx_register_isr_callback(irtx_cb);

    /* nec code form: (1)preamble + (8)addr + (8)addr_bar + (8)cmd + (8)cmd_bar + (1)stop + (1)repeat0 + (1)repeat1  */
    irtx_code_t buff[1*2 + 32*2 + 2 + 2*2]; // 72
    irtx_wave_code_init(buff, false);
    drv_irtx_write_int(buff, sizeof(buff)/sizeof(buff[0]));

    while(!(OM_IRTX->FIFO_SR & IRTX_FIFO_SR_FIFO_EMPTY_MASK));
}



/** @} */