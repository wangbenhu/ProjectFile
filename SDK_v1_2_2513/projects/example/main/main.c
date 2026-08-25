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
 * @brief    main entry
 * @details  main entry
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
#include "autoconf.h"
#include "om_driver.h"
#include "example.h"


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
int main(void)
{
    static const pin_config_t pin_config[] = {
        {5,  {PINMUX_PAD5_UART1_TRX_CFG}, PMU_PIN_MODE_PU, PMU_PIN_DRIVER_CURRENT_NORMAL},
        {6,  {PINMUX_PAD6_UART1_RX_CFG},  PMU_PIN_MODE_PU, PMU_PIN_DRIVER_CURRENT_NORMAL},
    };

    drv_wdt_init(0);
    drv_pin_init(pin_config, sizeof(pin_config)/sizeof(pin_config[0]));
    static const uart_config_t uart_cfg = {
        .baudrate     = 115200,
        .flow_control = UART_FLOW_CONTROL_NONE,
        .data_bit     = UART_DATA_BIT_8,
        .stop_bit     = UART_STOP_BIT_1,
        .parity       = UART_PARITY_NONE,
    };
    drv_uart_init(OM_UART1, &uart_cfg);

    om_printf("Example project\r\n");

    // example_aes();

    // example_efuse();

    // example_audio();

    // example_flash();
    // example_flash_quad();

    // example_gpadc_read();
    // example_gpadc_read_int();
    // example_gpadc_read_dma();
    // example_gpadc_read_temperature();
    // example_gpadc_read_battery();
    // example_gpadc_read_multi();
    // example_gpadc_read_int_multi();
    // example_gpadc_read_dma_multi();
    // example_gpadc_read_diff();

    // example_gpdma_ram2ram();
    // example_gpdma_ram2ram_chains();

    // example_gpio();

    // example_i2c();

    // example_i2s();

    // example_ir();

    // example_lcd();

    // example_lptim_oneshot_count();
    // example_lptim_free_running_count();
    // example_lptim_free_running_pwm();
    // example_lptim_oneshot_pwm();
    // example_lptim_buffered_pwm();
    // example_lptim_double_pwm();

    // example_pmu_timer();

    // example_qdec();
    // example_qdec_with_led();

    // example_rgb_block();
    // example_rgb_int();
    // example_rgb_dma();

    // example_rtc();

    // example_spi_wire4_trans_int();
    // example_spi_wire4_trans_dma();
    // example_spi_wire3_trans_int();
    // example_spi_wire3_trans_dma();
    // example_spi_wire4_trans_int_32m();

    // example_tim_count();
    // example_tim_pwm();
    // example_tim_pwm_complementary();
    // example_tim_pwm_dma();
    // example_tim_capture();
    // example_tim_dma_capture();
    // example_tim_pwm_input();

    // example_uart_block();
    // example_uart_int();
    // example_uart_dma();
    // example_uart_flow_control();

    // example_wdt();

    // #if (CONFIG_NVDS)
    // example_nvds();
    // #endif

    while(1);
}

void om_putchar(char character)
{
    drv_uart_write(OM_UART1, (uint8_t *)&character, 1, DRV_MAX_DELAY);
}

/** @} */
