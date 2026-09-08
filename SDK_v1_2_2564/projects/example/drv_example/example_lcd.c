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
 * @brief    example for using lcd
 * @details  example for using lcd:
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
#include "../../../bsp/source/glcd.h"

/*******************************************************************************
 * MACROS
 */
//GC9C01
#define PAD_LCD_CS0_GC9C01     21
#define PAD_LCD_SCK_GC9C01     14
#define PAD_LCD_IO0_GC9C01     18
#define PAD_LCD_IO1_GC9C01     15
#define PAD_LCD_IO2_GC9C01     20
#define PAD_LCD_IO3_GC9C01     19
#define PAD_LCD_RST_GC9C01     8

#define MUX_LCD_CS0_GC9C01     PINMUX_PAD21_LCD_CS0_CFG
#define MUX_LCD_SCK_GC9C01     PINMUX_PAD14_LCD_SCK_CFG
#define MUX_LCD_IO0_GC9C01     PINMUX_PAD18_LCD_IO0_CFG
#define MUX_LCD_IO1_GC9C01     PINMUX_PAD15_LCD_IO1_CFG
#define MUX_LCD_IO2_GC9C01     PINMUX_PAD20_LCD_IO2_CFG
#define MUX_LCD_IO3_GC9C01     PINMUX_PAD19_LCD_IO3_CFG
#define MUX_LCD_RST_GC9C01     PINMUX_PAD8_GPIO_MODE_CFG

#define PIC_BUFF_LEN           (100*100*3)

//ST7796
#define PAD_LCD_RST_ST7796     15
#define PAD_LCD_CS_ST7796      21
#define PAD_LCD_SCK_ST7796     14
#define PAD_LCD_SDI_ST7796     18
#define PAD_LCD_DCX_RS_ST7796  20

//ST7789
#define PAD_LCD_RST_ST7789     15
#define PAD_LCD_CS_ST7789      21
#define PAD_LCD_SCK_ST7789     14
#define PAD_LCD_SDI_ST7789     18
#define PAD_LCD_DCX_RS_ST7789  20
/*******************************************************************************
 * TYPEDEFS
 */


/*******************************************************************************
 * CONST & VARIABLES
 */
static uint8_t pic_buf_rgb565[PIC_BUFF_LEN];

/// Pinmux Configuration GC9C01
static pin_config_t pin_config_gc9c01[] = {
    {PAD_LCD_CS0_GC9C01, {MUX_LCD_CS0_GC9C01}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_LCD_SCK_GC9C01, {MUX_LCD_SCK_GC9C01}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_LCD_IO0_GC9C01, {MUX_LCD_IO0_GC9C01}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_LCD_IO1_GC9C01, {MUX_LCD_IO1_GC9C01}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_LCD_IO2_GC9C01, {MUX_LCD_IO2_GC9C01}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_LCD_IO3_GC9C01, {MUX_LCD_IO3_GC9C01}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_LCD_RST_GC9C01, {MUX_LCD_RST_GC9C01}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
};

/// GPIO Configuration GC9C01
static gpio_config_t gpio_config_gc9c01[] = {
    {OM_GPIO0, PAD_LCD_RST_GC9C01,  GPIO_DIR_OUTPUT, GPIO_LEVEL_HIGH, GPIO_TRIG_NONE},
};

/// Pinmux Configuration ST7796
static const pin_config_t pin_config_st7796[] = {
    {PAD_LCD_RST_ST7796,    {PINMUX_GPIO_MODE_CFG},        PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_MAX},
    {PAD_LCD_CS_ST7796,     {PINMUX_PAD21_LCD_CS0_CFG},    PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_MAX},
    {PAD_LCD_SCK_ST7796,    {PINMUX_PAD14_LCD_SCK_CFG},    PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_MAX},
    {PAD_LCD_SDI_ST7796,    {PINMUX_PAD18_LCD_IO0_CFG},    PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_MAX},
    {PAD_LCD_DCX_RS_ST7796, {PINMUX_PAD20_LCD_DCX_RS_CFG}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_MAX},
};

/// GPIO Configuration ST7796
static const gpio_config_t gpio_config_st7796[] = {
    {OM_GPIO0, PAD_LCD_RST_ST7796,  GPIO_DIR_OUTPUT, GPIO_LEVEL_HIGH, GPIO_TRIG_NONE},
};

/// Pinmux Configuration ST7789
static const pin_config_t pin_config_st7789[] = {
    {PAD_LCD_RST_ST7789,    {PINMUX_GPIO_MODE_CFG},        PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_MAX},
    {PAD_LCD_CS_ST7789,     {PINMUX_PAD21_LCD_CS0_CFG},    PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_MAX},
    {PAD_LCD_SCK_ST7789,    {PINMUX_PAD14_LCD_SCK_CFG},    PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_MAX},
    {PAD_LCD_SDI_ST7789,    {PINMUX_PAD18_LCD_IO0_CFG},    PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_MAX},
    {PAD_LCD_DCX_RS_ST7789, {PINMUX_PAD20_LCD_DCX_RS_CFG}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_MAX},
};

/// GPIO Configuration ST7789
static const gpio_config_t gpio_config_st7789[] = {
    {OM_GPIO0, PAD_LCD_RST_ST7789,  GPIO_DIR_OUTPUT, GPIO_LEVEL_HIGH, GPIO_TRIG_NONE},
};
/*******************************************************************************
 * LOCAL FUNCTIONS
 */

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief example of using lcd GC9C01 with FLASH_LIKE mode
 *
 *******************************************************************************
 */
void example_lcd_gc9c01(void)
{
    // white pic
    for (uint32_t i = 0; i < PIC_BUFF_LEN; i++) {
        pic_buf_rgb565[i] = 0xFF;
    }

    uint8_t  clk_div = 0;
    uint16_t x1 = 0;
    uint16_t y1 = 0;
    uint16_t x2 = 200;
    uint16_t y2 = 200;
    uint8_t* display_data = (uint8_t*)pic_buf_rgb565;

    drv_pin_init(pin_config_gc9c01, sizeof(pin_config_gc9c01) / sizeof(pin_config_gc9c01[0]));
    drv_gpio_init(gpio_config_gc9c01, sizeof(gpio_config_gc9c01) / sizeof(gpio_config_gc9c01[0]));

    glcd_init(clk_div, (uint32_t)OM_GPIO0, PAD_LCD_RST_GC9C01);
    glcd_set_disp_window(x1, y1, x2, y2);
    glcd_disp_flush(display_data, (x2 - x1 + 1) * (y2 - y1 + 1) * LCD_BYTES_PER_PIX);
    glcd_disp_flush_finish();
}

/**
 *******************************************************************************
 * @brief example of using lcd ST7796 with 4WM1 mode
 *
 *******************************************************************************
 */
void example_lcd_st7796(void)
{
    uint8_t  clk_div = 2;
    uint32_t color_565 = 0xF800; //red

    drv_pin_init(pin_config_st7796, sizeof(pin_config_st7796) / sizeof(pin_config_st7796[0]));
    drv_gpio_init(gpio_config_st7796, sizeof(gpio_config_st7796) / sizeof(gpio_config_st7796[0]));

    // Init LCD
    glcd_init(clk_div, (uint32_t) OM_GPIO0, PAD_LCD_RST_ST7796);
    glcd_clear(color_565, 0, 0, LCD_X_RESOLUTION - 1, LCD_Y_RESOLUTION - 1);
    glcd_disp_flush_finish();
}

/**
 *******************************************************************************
 * @brief example of using lcd ST7789 with3WM1 2 lane mode
 *
 *******************************************************************************
 */
void example_lcd_st7789(void)
{
    uint8_t  clk_div = 2;
    uint32_t color_565 = 0xF800; //red

    drv_pin_init(pin_config_st7789, sizeof(pin_config_st7789) / sizeof(pin_config_st7789[0]));
    drv_gpio_init(gpio_config_st7789, sizeof(gpio_config_st7789) / sizeof(gpio_config_st7789[0]));

    // Init LCD
    glcd_init(clk_div, (uint32_t) OM_GPIO0, PAD_LCD_RST_ST7789);
    glcd_clear(color_565, 0, 0, LCD_X_RESOLUTION - 1, LCD_Y_RESOLUTION - 1);
    glcd_disp_flush_finish();
}


/** @} */