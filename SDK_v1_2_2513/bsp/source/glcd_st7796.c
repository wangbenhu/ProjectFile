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
 * @brief    glcd driver
 * @details  graphics lcd st7796 driver with 4WM1 and SPI mode0
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
#include "om_driver.h"
#include "autoconf.h"
#if (CONFIG_GLCD_ST7796)
#include "glcd.h"
#include "string.h"
#include <stdint.h>
#include "stdio.h"
#include "om_mem.h"
#include "om_compiler.h"
#if (CONFIG_FREE_RTOS)
#include "cmsis_os2.h"
#endif

/*******************************************************************************
 * MACROS
 */
/// hardware limitation is 65535, but we use psram page size to support accross page read
#define GLCD_MAX_DATA_BYTES     (320*102*2)                         // usually psram page size
#define GLCD_DELAY              DRV_DELAY_MS
#define GLCD_REG_DELAY(ms)      0xFF, ms
#define GLCD_KEEP_CS            0
/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    volatile uint8_t lcd_finish;               // 0: lcd is busy, 1: lcd is idle (finish)
    volatile uint8_t flush_finish;              // 0: flush is busy, 1: flush is idle (finish)
    volatile uint8_t display_buffer_mode;       // 0: display buffer is increasing, 1: display buffer is stable, 2: stride mode
    uint32_t stride;                            // display buffer stride
    uint32_t width;                             // display buffer width
    uint8_t* display_buffer;                    // function glcd_disp_flush get user buffer stored in this variable
    uint32_t display_number_bytes;              // function glcd_disp_flush get user buffer length stored in this variable
    uint32_t max_number_bytes;                  // function glcd_write_memory once write max_number_bytes data to lcd
    uint8_t* clear_data_buffer;                 // function glcd_clear use this malloc buffer to clear display
    glcd_cb_t st7796_cb;                        // user callback function
    uint8_t clk_div;                            // clk_div
    uint32_t reset_pin_port;
    uint32_t reset_pin_idx;
    volatile uint8_t te_iscoming;               // te cycle
} st7796_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
static st7796_env_t glcd_env = {
    .lcd_finish = 1,
    .flush_finish = 1,
    .st7796_cb = NULL,
    .display_buffer = NULL,
    .display_buffer_mode = 0,
    .display_number_bytes = 0,
    .max_number_bytes = GLCD_MAX_DATA_BYTES,
    .clear_data_buffer = NULL,
};

/**
 *  call GLCD_REG_DELAY(0) even if no need to delay
 */
static uint8_t glcd_init_seq[] = {
    0x01, 0x11, GLCD_REG_DELAY(120),
    0x02, 0x36, 0x48, GLCD_REG_DELAY(1),
    0x02, 0x3A, 0x55, GLCD_REG_DELAY(1),
    0x02, 0xF0, 0xC3, GLCD_REG_DELAY(1),
    0x02, 0xF0, 0x96, GLCD_REG_DELAY(1),
    0x02, 0xB4, 0x02, GLCD_REG_DELAY(1),
    0x02, 0xB7, 0xC6, GLCD_REG_DELAY(1),
    0x03, 0xC0, 0xC0, 0x00, GLCD_REG_DELAY(1),
    0x02, 0xC1, 0x13, GLCD_REG_DELAY(1),
    0x02, 0xC2, 0xA7, GLCD_REG_DELAY(1),
    0x02, 0xC5, 0x21, GLCD_REG_DELAY(1),
    0x09, 0xE8, 0x40, 0x8A, 0x1B, 0x1B, 0x23, 0x0A, 0xAC, 0x33, GLCD_REG_DELAY(1),
    0x0f, 0xE0,0xD2,0x05,0x08,0x06,0x05,0x02,0x2A,0x44,0x46,0x39,0x15,0x15,0x2D,0x32, GLCD_REG_DELAY(1),
	0x0f, 0xE1,0x96,0x08,0x0C,0x09,0x09,0x25,0x2E,0x43,0x42,0x35,0x11,0x11,0x28,0x2E, GLCD_REG_DELAY(1),
    0x02, 0xF0, 0x3C, GLCD_REG_DELAY(1),
    0x02, 0xF0, 0x69, GLCD_REG_DELAY(120),
    0x01, 0x21, GLCD_REG_DELAY(1),
    0x01, 0x29, GLCD_REG_DELAY(1),
};


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static void st7796_callback(glcd_cb_t* cb);

static void glcd_raw_init(uint8_t clk_div)
{
    lcd_config_t lcd_config = {
        .clk_div = clk_div,
        .spi_mode = LCD_SPI_MODE_0,
        .rgb_mode = LCD_RGB_MODE_565,
        .data_mode = LCD_DATA_MODE_4WM1,
    };
    drv_lcd_init(&lcd_config);
    drv_lcd_register_isr_callback((drv_isr_callback_t)st7796_callback);
}

static void glcd_hw_reset(void)
{
    drv_gpio_write((OM_GPIO_Type *)glcd_env.reset_pin_port, 1 << glcd_env.reset_pin_idx, GPIO_LEVEL_HIGH);
    GLCD_DELAY(50);
    drv_gpio_write((OM_GPIO_Type *)glcd_env.reset_pin_port, 1 << glcd_env.reset_pin_idx, GPIO_LEVEL_LOW);
    GLCD_DELAY(100);
    drv_gpio_write((OM_GPIO_Type *)glcd_env.reset_pin_port, 1 << glcd_env.reset_pin_idx, GPIO_LEVEL_HIGH);
    GLCD_DELAY(50);
}

/**
 *  the param and must be in uncachable ram
 */
static void glcd_write_reg(uint8_t reg, const uint8_t *param, uint16_t param_bytes)
{
    while(!glcd_env.lcd_finish);
    drv_lcd_control(LCD_CONTROL_SET_DATA_BIT, (void *)1);
    drv_lcd_control(LCD_CONTROL_SET_DATA_WIDTH, (void *)8);
    uint8_t cmd[] = {reg, 0x00, 0x00, 0x00};
    drv_lcd_write(cmd, 8, param, param_bytes);
    drv_lcd_control(LCD_CONTROL_SET_DATA_BIT, (void *)1);
    drv_lcd_control(LCD_CONTROL_SET_DATA_WIDTH, (void *)16);
}

static void glcd_write_reg_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    static uint8_t coord_dma[4];
    coord_dma[1] = x1 & 0xFF;
    coord_dma[0] = (x1 >> 8) & 0xFF;
    coord_dma[3] = x2 & 0xFF;
    coord_dma[2] = (x2 >> 8) & 0xFF;

    uint8_t reg = 0x2A;
    glcd_write_reg(reg, coord_dma, 4);

    coord_dma[1] = y1 & 0xFF;
    coord_dma[0] = (y1 >> 8) & 0xFF;
    coord_dma[3] = y2 & 0xFF;
    coord_dma[2] = (y2 >> 8) & 0xFF;
    reg = 0x2B;
    glcd_write_reg(reg, coord_dma, 4);
}

//The instruction issuance and return of the read register command of st7796 are in QSPI 1-line 8bit mode
static void glcd_read_reg(uint8_t reg, uint8_t *data, uint16_t data_bytes)
{
    while(!glcd_env.lcd_finish);
    drv_lcd_control(LCD_CONTROL_SET_DATA_BIT, (void *)1);
    drv_lcd_control(LCD_CONTROL_SET_DATA_WIDTH, (void *)8);
    uint8_t cmd[] = {reg, 0x00, 0x00, 0x00};
    drv_lcd_read(cmd, 8, data, data_bytes);
    drv_lcd_control(LCD_CONTROL_SET_DATA_BIT, (void *)1);
    drv_lcd_control(LCD_CONTROL_SET_DATA_WIDTH, (void *)16);
}

static void glcd_write_memory(uint8_t write_command, uint8_t* data, uint16_t data_bytes)
{
    uint8_t cmd[] = {write_command, 0x00, 0x00, 0x00};
    while(!glcd_env.lcd_finish);
    glcd_env.lcd_finish = 0;
    drv_lcd_write_int(cmd, 8, data, data_bytes);
}

/**
 * @brief Initialize the lcd hardware by sending glcd_init_seq[]
 */
static void glcd_init_sequence(void)
{
    uint8_t reg;
    uint8_t cmd_lens;
    uint8_t data_bytes;
    uint32_t i = 0;
    while(i < sizeof(glcd_init_seq)) {
        cmd_lens = glcd_init_seq[i];
        data_bytes = cmd_lens - 1;
        reg = glcd_init_seq[i+1];
        glcd_write_reg(reg, &(glcd_init_seq[i+2]), data_bytes);
        i += (cmd_lens + 1);
        if(glcd_init_seq[i] == 0xff) {
            GLCD_DELAY(glcd_init_seq[i+1]);
            i+=2;
        }
    }
}

// lcd call back
static void st7796_callback(glcd_cb_t* cb)
{
    glcd_env.lcd_finish = 1;

    if(glcd_env.display_number_bytes > 0) {   // still have data to transfer
        if(((uint32_t)glcd_env.display_buffer >= 0x70000000) && GLCD_KEEP_CS) {     // if data from psram , cs need to be reset to support cross page
            register_set(&OM_LCD->COMMAND, MASK_1REG(LCD_COMMAND_KEEP_CS, 1));
        }
        uint32_t current_send_bytes;
        uint32_t disp_buf_incress_len;
        if(glcd_env.display_buffer_mode == 1) {
            current_send_bytes = glcd_env.display_number_bytes > glcd_env.max_number_bytes? glcd_env.max_number_bytes : glcd_env.display_number_bytes;
            disp_buf_incress_len = current_send_bytes;
            glcd_env.display_number_bytes -= current_send_bytes;
        } else if(glcd_env.display_buffer_mode == 2) {
            current_send_bytes = glcd_env.width;
            disp_buf_incress_len = glcd_env.stride;
            glcd_env.display_number_bytes -= 1;
        } else {
            current_send_bytes = glcd_env.display_number_bytes > glcd_env.max_number_bytes? glcd_env.max_number_bytes : glcd_env.display_number_bytes;
            disp_buf_incress_len = 0;
            glcd_env.display_number_bytes -= current_send_bytes;
        }
        glcd_write_memory(0x3C, glcd_env.display_buffer, current_send_bytes);   // call glcd_write_memory will reset cs.
        glcd_env.display_buffer += disp_buf_incress_len;
    } else {
        if(glcd_env.st7796_cb) {          // run st7796 call back if registered
            glcd_env.st7796_cb();
        }
        if(glcd_env.clear_data_buffer) {  // run clear data buffer if glcd_clear
            //om_mem_free(OM_MEM(0), glcd_env.clear_data_buffer);
            glcd_env.clear_data_buffer = NULL;
        }
        glcd_env.flush_finish = 1;        // flush finish
    }
}

static void glcd_wait_te(uint8_t timeout_ms)
{
    while(!glcd_env.te_iscoming) {
        #if (CONFIG_FREE_RTOS)
            osDelay(1);
        #else
            DRV_DELAY_MS(1);
        #endif
        timeout_ms -= 1;
        if(timeout_ms == 0) {
            break;
        }
    }
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
//Although the st7796 chip supports this instruction, this LCD module does not support it.There is also no support for reading back LCD screen data.
uint32_t glcd_read_id(void) {
    uint8_t reg_adr = 0x04;
    static uint8_t aligned_buffer[4] __attribute__((aligned(4)));  // 静态变量强制对齐
    uint32_t lcd_id;

    glcd_read_reg(reg_adr, aligned_buffer, 4);
    lcd_id = *(uint32_t *)aligned_buffer;
    return lcd_id;
}

//Although the st7796 chip supports this instruction, this LCD module does not support it.There is also no support for reading back LCD screen data.
uint32_t glcd_read_status(void) {
    uint8_t reg_adr = 0x09;
    static uint8_t aligned_buffer[5] __attribute__((aligned(4)));  // 静态变量强制对齐
    uint32_t lcd_id;

    glcd_read_reg(reg_adr, aligned_buffer, 5);
    lcd_id = *(uint32_t *)aligned_buffer;
    return lcd_id;
}

void glcd_set_brightness(uint8_t brightness)
{
    uint8_t reg = 0x51;
    uint8_t data = brightness;
    glcd_write_reg(reg, &data, 1);
}

void glcd_control(glcd_control_t control)
{
    if(control & LCD_DISPLAY_ON) {
        glcd_raw_init(glcd_env.clk_div);
    }
    glcd_write_reg(control, 0, 0);
}

void glcd_init(uint8_t clk_div, uint32_t port, uint32_t idx)
{
    glcd_raw_init(clk_div);
    glcd_env.clk_div = clk_div;
    glcd_env.reset_pin_port = port;
    glcd_env.reset_pin_idx = idx;
    glcd_hw_reset();
    glcd_init_sequence();
}

void glcd_set_disp_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    glcd_write_reg_window(x1, y1, x2, y2);
}

void glcd_disp_flush(void *color, uint32_t color_len)
{
    glcd_env.te_iscoming = 0;
    glcd_wait_te(17);
    // send data to lcd, data over than GLCD_MAX_DATA_BYTES will transfer in irq.
    // GLCD_MAX_DATA_BYTES usually is a psram page size.
    while(!glcd_env.flush_finish);
    glcd_env.flush_finish = 0;
    uint32_t left_data_bytes = color_len;
    uint32_t data_index = 0;
    uint32_t current_send_bytes = left_data_bytes < GLCD_MAX_DATA_BYTES? left_data_bytes : GLCD_MAX_DATA_BYTES;
    left_data_bytes -= current_send_bytes;
    data_index += current_send_bytes;
    glcd_env.display_number_bytes = left_data_bytes;
    glcd_env.display_buffer = (uint8_t*)color + data_index;
    glcd_env.display_buffer_mode = 1;
    glcd_env.max_number_bytes = GLCD_MAX_DATA_BYTES;
    if(((uint32_t)glcd_env.display_buffer >= 0x70000000) && GLCD_KEEP_CS) {     // if data from psram , cs need to be reset to support cross page
        register_set(&OM_LCD->COMMAND, MASK_1REG(LCD_COMMAND_KEEP_CS, 1));
    }
    glcd_write_memory(0x2C, color, current_send_bytes);
}


void glcd_disp_flush_stride(void *color, uint32_t width, uint32_t stride, uint32_t lines)
{
    glcd_env.te_iscoming = 0;
    glcd_wait_te(17);
    // send data to lcd, data over than GLCD_MAX_DATA_BYTES will transfer in irq.
    // GLCD_MAX_DATA_BYTES usually is a psram page size.
    while(!glcd_env.flush_finish);
    glcd_env.flush_finish = 0;
    uint32_t current_send_bytes = width;
    glcd_env.stride = stride;
    glcd_env.width = width;
    glcd_env.display_number_bytes = lines - 1;
    glcd_env.display_buffer = (uint8_t*)color + stride;
    glcd_env.display_buffer_mode = 2;
    glcd_env.max_number_bytes = GLCD_MAX_DATA_BYTES;
    if(((uint32_t)glcd_env.display_buffer >= 0x70000000) && GLCD_KEEP_CS) {     // if data from psram , cs need to be reset to support cross page
        register_set(&OM_LCD->COMMAND, MASK_1REG(LCD_COMMAND_KEEP_CS, 1));
    }
    glcd_write_memory(0x2C, color, current_send_bytes);
}

void glcd_set_register(uint8_t reg)
{
    glcd_write_reg(reg, 0, 0);
}

// call this function if te is coming
void glcd_set_te_signal()
{
    glcd_env.te_iscoming = 1;
}

void glcd_callback_register(glcd_cb_t cb)
{
    glcd_env.st7796_cb = cb;
}

void glcd_clear(uint32_t color, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    uint32_t pixel_width = x2 - x1 + 1;
    uint32_t pixel_height = y2 - y1 + 1;
    glcd_env.clear_data_buffer  = om_mem_malloc(OM_MEM(0), pixel_width * LCD_BYTES_PER_PIX);
    for(uint32_t i = 0; i < pixel_width * LCD_BYTES_PER_PIX / 2; i++) {
        ((uint16_t*)glcd_env.clear_data_buffer)[i] = color;       /*lint !e2445 */
    }
    while(!glcd_env.flush_finish);
    glcd_env.flush_finish = 0;
    uint32_t left_data_bytes = pixel_width * pixel_height * LCD_BYTES_PER_PIX;
    glcd_set_disp_window(x1, y1, x2, y2);
    uint32_t current_send_bytes = left_data_bytes < pixel_width * LCD_BYTES_PER_PIX? left_data_bytes : pixel_width * LCD_BYTES_PER_PIX;
    left_data_bytes -= current_send_bytes;
    glcd_env.display_number_bytes = left_data_bytes;
    glcd_env.display_buffer = glcd_env.clear_data_buffer;
    glcd_env.display_buffer_mode = 0;
    glcd_env.max_number_bytes = pixel_width * LCD_BYTES_PER_PIX;
    glcd_write_memory(0x2C, glcd_env.clear_data_buffer, current_send_bytes);
}

void glcd_clear_line(uint8_t* color_buf, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    uint32_t pixel_width = x2 - x1 + 1;
    uint32_t pixel_height = y2 - y1 + 1;
    glcd_env.clear_data_buffer  = om_mem_malloc(OM_MEM(0), pixel_width * LCD_BYTES_PER_PIX);
    for(uint32_t i = 0; i < pixel_width * LCD_BYTES_PER_PIX / 2; i++) {
        ((uint16_t*)glcd_env.clear_data_buffer)[i] = color_buf[i];   /*lint !e2445 */
    }
    while(!glcd_env.flush_finish);
    glcd_env.flush_finish = 0;
    uint32_t left_data_bytes = pixel_width * pixel_height * LCD_BYTES_PER_PIX;
    glcd_set_disp_window(x1, y1, x2, y2);
    uint32_t current_send_bytes = left_data_bytes < pixel_width * LCD_BYTES_PER_PIX? left_data_bytes : pixel_width * LCD_BYTES_PER_PIX;
    left_data_bytes -= current_send_bytes;
    glcd_env.display_number_bytes = left_data_bytes;
    glcd_env.display_buffer = glcd_env.clear_data_buffer;
    glcd_env.display_buffer_mode = 0;
    glcd_env.max_number_bytes = pixel_width * LCD_BYTES_PER_PIX;
    glcd_write_memory(0x2C, glcd_env.clear_data_buffer, current_send_bytes);
}

void glcd_disp_flush_finish()
{
    while(!glcd_env.flush_finish);
}

#endif /* CONFIG_GLCD_ST7796 */
