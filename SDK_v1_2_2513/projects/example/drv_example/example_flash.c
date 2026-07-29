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
 * @brief    example for using flash driver
 * @details  example for using flash driver: erase, read, write
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
    {PAD_OSPI1_WP, {PINMUX_PAD12_OSPI1_WP_CFG}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_OSPI1_HD, {PINMUX_PAD11_OSPI1_HD_CFG}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
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
 * @brief example of working with flash: erase, write, read
 *
 *******************************************************************************
 */
void example_flash(void)
{
    for (uint8_t i = 0; i < 100; i++) {
        write_buf[i] = i;
    }

    /* inside flash */
    // Init Flash
    flash_config_t iflash_config = {
        .clk_div = drv_rcc_clock_get(RCC_CLK_SF0) / 32000000U,
        .delay = FLASH_DELAY_AUTO,
        .read_cmd = FLASH_READ,
        .write_cmd = FLASH_PAGE_PROGRAM,
        .spi_mode = FLASH_SPI_MODE_0,
    };
    drv_flash_init(OM_FLASH0, &iflash_config);
    // Disable write protect for flash0, it is enabled by default during the flash initialization process.
    drv_flash_write_protect_set_volatile(OM_FLASH0, FLASH_PROTECT_NONE);

    // there are 2 ways to erase flash, one is to use drv_flash_erase, the other is to use drv_flash_erase_start and poll done
    #if 1
    // Erase 4k in 128k
    drv_flash_erase(OM_FLASH0, 128 * 1024, FLASH_ERASE_4K);
    #else
    // erase start
    drv_flash_erase_start(OM_FLASH0, 128 * 1024, FLASH_ERASE_4K);
    // poll done, timeout 1000ms
    for (int i = 0; i < 1000; i += 10) {
        if (drv_flash_erase_is_done(OM_FLASH0)) {
            break;
        }
        osDelay(10);
    }
    #endif

    // Read 100 bytes in 128k, it should be all 0xFF
    drv_flash_read(OM_FLASH0, 128 * 1024, read_buf, 100);
    // Write 100 bytes to 128k
    drv_flash_write(OM_FLASH0, 128 * 1024, write_buf, 100);
    // Read 100 bytes in 128k, it should be same as write_buf
    drv_flash_read(OM_FLASH0, 128 * 1024, read_buf, 100);
    // Enable write protect, it should not be erased and write.
    drv_flash_write_protect_set_volatile(OM_FLASH0, FLASH_PROTECT_ALL);

    /* external flash */
    drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));
    // Init Flash
    flash_config_t oflash_config = {
        .clk_div = drv_rcc_clock_get(RCC_CLK_OSPI1) / 32000000U,
        .delay = FLASH_DELAY_AUTO,
        .read_cmd = FLASH_READ,
        .write_cmd = FLASH_PAGE_PROGRAM,
        .spi_mode = FLASH_SPI_MODE_0,
    };
    drv_flash_init(OM_FLASH1, &oflash_config);
    // Erase 4k in 128k
    drv_flash_erase(OM_FLASH1, 128 * 1024, FLASH_ERASE_4K);
    // Read 100 bytes in 128k, it should be all 0xFF
    drv_flash_read(OM_FLASH1, 128 * 1024, read_buf, 100);
    // Write 100 bytes to 128k
    drv_flash_write(OM_FLASH1, 128 * 1024, write_buf, 100);
    // Read 100 bytes in 128k, it should be same as write_buf
    drv_flash_read(OM_FLASH1, 128 * 1024, read_buf, 100);
}

/**
 *******************************************************************************
 * @brief example of working with flash in quad mode
 *
 *******************************************************************************
 */
void example_flash_quad(void)
{
    /* external flash */
    drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));
    // Init Flash
    flash_config_t oflash_config = {
        .clk_div = drv_rcc_clock_get(RCC_CLK_OSPI1) / 32000000U,
        .delay = FLASH_DELAY_AUTO,
        .read_cmd = FLASH_FAST_READ_QIO,
        .write_cmd = FLASH_PAGE_PROGRAM_QI,
        .spi_mode = FLASH_SPI_MODE_0,
    };
    drv_flash_init(OM_FLASH1, &oflash_config);
    // Erase 4k in 128k
    drv_flash_erase(OM_FLASH1, 128 * 1024, FLASH_ERASE_4K);
    // Read 100 bytes in 128k, it should be all 0xFF
    drv_flash_read(OM_FLASH1, 128 * 1024, read_buf, 100);
    // Write 100 bytes to 128k
    drv_flash_write(OM_FLASH1, 128 * 1024, write_buf, 100);
    // Read 100 bytes in 128k, it should be same as write_buf
    drv_flash_read(OM_FLASH1, 128 * 1024, read_buf, 100);
}

void example_flash_secure_register(void)
{
    for (uint8_t i = 0; i < 100; i++) {
        write_buf[i] = i;
    }

    /* inside flash */
    // Init Flash
    flash_config_t iflash_config = {
        .clk_div = drv_rcc_clock_get(RCC_CLK_SF0) / 32000000U,
        .delay = FLASH_DELAY_AUTO,
        .read_cmd = FLASH_READ,
        .write_cmd = FLASH_PAGE_PROGRAM,
        .spi_mode = FLASH_SPI_MODE_0,
    };
    drv_flash_init(OM_FLASH0, &iflash_config);
    // Erase secure register 1
    drv_flash_secure_register_erase(OM_FLASH0, 1);
    // Write 100 bytes to secure register 1
    drv_flash_secure_register_write(OM_FLASH0, 1, 0, write_buf, 100);
    // Read 100 bytes from secure register 1
    drv_flash_secure_register_read(OM_FLASH0, 1, 0, read_buf, 100);
}
/** @} */
