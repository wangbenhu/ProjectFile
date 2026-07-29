/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup EXAMPLE
 * @ingroup
 * @brief    Example driver interface
 * @details  Example driver interface
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __EXAMPLE_H
#define __EXAMPLE_H

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
extern void example_aes(void);
extern void example_efuse(void);
extern void example_audio(void);
extern void example_flash(void);
extern void example_flash_quad(void);
extern void example_flash_secure_register(void);
extern void example_gpadc_read(void);
extern void example_gpadc_read_int(void);
extern void example_gpadc_read_dma(void);
extern void example_gpadc_read_temperature(void);
extern void example_gpadc_read_battery(void);
extern void example_gpadc_read_multi(void);
extern void example_gpadc_read_int_multi(void);
extern void example_gpadc_read_dma_multi(void);
extern void example_gpadc_read_diff(void);
extern void example_gpdma_ram2ram(void);
extern void example_gpdma_ram2ram_chains(void);
extern void example_gpio(void);
extern void example_i2c(void);
extern void example_i2c_dma(void);
extern void example_i2c_int(void);
extern void example_i2s(void);
extern void example_ir(void);
extern void example_lcd(void);
extern void example_lptim_oneshot_count(void);
extern void example_lptim_free_running_count(void);
extern void example_lptim_free_running_pwm(void);
extern void example_lptim_oneshot_pwm(void);
extern void example_lptim_buffered_pwm(void);
extern void example_lptim_double_pwm(void);
extern void example_pmu_timer(void);
extern void example_qdec(void);
extern void example_qdec_with_led(void);
extern void example_rgb_block(void);
extern void example_rgb_int(void);
extern void example_rgb_dma(void);
extern void example_rtc(void);
extern void example_spi_wire4_trans_int(void);
extern void example_spi_wire4_trans_dma(void);
extern void example_spi_wire3_trans_int(void);
extern void example_spi_wire3_trans_dma(void);
extern void example_spi_wire4_trans_int_32m(void);
extern void example_tim_count(void);
extern void example_tim_pwm(void);
extern void example_tim_pwm_complementary(void);
extern void example_tim_pwm_dma(void);
extern void example_tim_capture(void);
extern void example_tim_dma_capture(void);
extern void example_tim_pwm_input(void);
extern void example_uart_block(void);
extern void example_uart_int(void);
extern void example_uart_dma(void);
extern void example_uart_flow_control(void);
extern void example_wdt(void);
extern void example_nvds(void);
extern void example_shelf_mode(void);
#ifdef __cplusplus
}
#endif

#endif  /* __EXAMPLE_H */

/** @} */
