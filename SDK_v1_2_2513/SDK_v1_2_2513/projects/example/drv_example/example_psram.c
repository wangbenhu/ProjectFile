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
 * @brief    example for using psram
 * @details  example for using psram: read and write
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
/// Test cs pad for ospi1
#define PAD_OSPI1_CS               8
/// Test ck pad for ospi1
#define PAD_OSPI1_CK               9
/// Test si pad for ospi1
#define PAD_OSPI1_SI               10
/// Test so pad for ospi1
#define PAD_OSPI1_SO               7
/// Test wp pad for ospi1
#define PAD_OSPI1_WP               12
/// Test hd pad for ospi1
#define PAD_OSPI1_HD               11

/*******************************************************************************
 * TYPEDEFS
 */


/*******************************************************************************
 * CONST & VARIABLES
 */
/// Test oflash pin configuration
static const pin_config_t pin_config[] = {
    {PAD_OSPI1_CS, {PINMUX_PAD8_OSPI1_CS_CFG},  PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_OSPI1_CK, {PINMUX_PAD9_OSPI1_CK_CFG},  PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_OSPI1_SI, {PINMUX_PAD10_OSPI1_SI_CFG}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_OSPI1_SO, {PINMUX_PAD7_OSPI1_SO_CFG},  PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_OSPI1_WP, {PINMUX_PAD11_OSPI1_HD_CFG}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_OSPI1_HD, {PINMUX_PAD12_OSPI1_WP_CFG}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
};
/// Buffer that stored the data to be written
static uint8_t write_buf[100];
/// Buffer that stored the data to be read
static uint8_t read_buf[100];

/*******************************************************************************
 * LOCAL FUNCTIONS
 */

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief example of using psram
 *
 *******************************************************************************
 */
void example_psram(void)
{
    drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));

    psram_config_t psram_config = {
        .clk_div = drv_rcc_clock_get(RCC_CLK_OSPI1) / 32000000U,
        .delay = PSRAM_DELAY_AUTO,
        .read_cmd = PSRAM_READ,
        .write_cmd = PSRAM_WRITE,
        .page_cross_en = 1,
        .page_size = 512,
    };
    drv_psram_init(OM_OSPI1, &psram_config);
    drv_psram_write(OM_OSPI1, 0x1000, write_buf, 100);
    drv_psram_read(OM_OSPI1, 0x1000, read_buf, 100);
}

/**
 *******************************************************************************
 * @brief example of using psram in quad mode
 *
 *******************************************************************************
 */
void example_psram_quad(void)
{
    drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));

    psram_config_t psram_config = {
        .clk_div = drv_rcc_clock_get(RCC_CLK_OSPI1) / 32000000U,
        .delay = PSRAM_DELAY_AUTO,
        .read_cmd = PSRAM_FAST_READ_QUAD,
        .write_cmd = PSRAM_WRITE_QUAD,
        .page_cross_en = 1,
        .page_size = 512,
    };
    drv_psram_init(OM_OSPI1, &psram_config);
    drv_psram_quad_mode_enable(OM_OSPI1, 1);
    drv_psram_write(OM_OSPI1, 0x1000, write_buf, 100);
    drv_psram_read(OM_OSPI1, 0x1000, read_buf, 100);
    drv_psram_quad_mode_enable(OM_OSPI1, 0);
}




/** @} */