/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @file     LCD LCD
 * @ingroup  LCD
 * @brief    templete
 * @details  templete, templete for .h header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


#ifndef __GLCD_H
#define __GLCD_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include "autoconf.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
#if(CONFIG_GLCD_GC9C01)
    #define LCD_X_RESOLUTION          454U
    #define LCD_Y_RESOLUTION          454U
    #define LCD_BYTES_PER_PIX         2U
#elif(CONFIG_GLCD_ST7796)
    #define LCD_X_RESOLUTION          320U
    #define LCD_Y_RESOLUTION          480U
    #define LCD_BYTES_PER_PIX         2U
#elif(CONFIG_GLCD_ST7789)
    #define LCD_X_RESOLUTION          240U
    #define LCD_Y_RESOLUTION          320U
    #define LCD_BYTES_PER_PIX         2U
#endif


/*******************************************************************************
 * TYPEDEFS
 */
typedef void (*glcd_cb_t)(void);

typedef enum {
    LCD_ENTER_SLEEP = 0x10,        // enter sleep mode, previous data is invalid
    LCD_EXIT_SLEEP  = 0x11,        // exit sleep mode, screen display black
    LCD_PART_MODE   = 0x12,
    LCD_NORMAL_MODE = 0x13,
    LCD_INV_OFF     = 0x20,
    LCD_INV_ON      = 0x21,
    LCD_DISPLAY_OFF = 0x28,        // display off, the previous data will be kept
    LCD_DISPLAY_ON  = 0x29,        // display on , the previous screen will be displayed
    LCD_TE_OFF      = 0x34,
    LCD_TE_ON       = 0x35,
    LCD_IDLE_OFF    = 0x38,
    LCD_IDLE_ON     = 0x39,
} glcd_control_t;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief lcd initialization
 *
 * @param[in] clk_div the clock divider of the lcd
 * @param[in] port the port of the lcd reset pin
 * @param[in] idx the idx of the lcd reset pin
 *
 * @return None
 *******************************************************************************
 */
extern void glcd_init(uint8_t clk_div, uint32_t port, uint32_t idx);

/**
 *******************************************************************************
 * @brief set screen to specified color
 *
 * @param[in] color color format is rgb888
 * @param[in] x1 the start coord of horizen
 * @param[in] y1 the start coord of column
 * @param[in] x2 the end coord of the horizen
 * @param[in] y2 the end coord of the column
 *
 * @return None
 *******************************************************************************
 */
extern void glcd_clear(uint32_t color, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

/**
 *******************************************************************************
 * @brief set screen to specified color_buf
 *
 * @param[in] color color format is rgb888
 * @param[in] x1 the start coord of horizen
 * @param[in] y1 the start coord of column
 * @param[in] x2 the end coord of the horizen
 * @param[in] y2 the end coord of the column
 *
 * @return None
 *******************************************************************************
 */
extern void glcd_clear_line(uint8_t* color_buf, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

/**
 *******************************************************************************
 * @brief set a rectangle in screen where the next operate will only
 *
 * @param[in] x_start the start coord of horizen
 * @param[in] y_start the start coord of column
 * @param[in] x_end the end coord of the horizen
 * @param[in] y_end the end coord of the column
 *
 * @return None
 *******************************************************************************
 */
extern void glcd_set_disp_window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end);

/**
 *******************************************************************************
 * @brief display color
 *
 * @param[in] color the data to be displayed
 * @param[in] color_len data length, equal to the (pixel * bytes_per_pixel)
 * @return None
 *******************************************************************************
 */
extern void glcd_disp_flush(void *color, uint32_t color_len);

/**
 *******************************************************************************
 * @brief display color in stride mode
 *
 * @param[in] color the data to be displayed
 * @param[in] width
 * @param[in] stride the stride of the data
 * @param[in] lines the lines of the data
 * @return None
 *******************************************************************************
 */
void glcd_disp_flush_stride(void *color, uint32_t width, uint32_t stride, uint32_t lines);

/**
 *******************************************************************************
 * @brief set display brightness. 255 for brightest and 0 for darkest.
 *
 * @param[in] brightness the brightness of the display
 *
 * @return None
 *******************************************************************************
 */
extern void glcd_set_brightness(uint8_t brightness);

/**
 *******************************************************************************
 * @brief set display mode
 *
 * @param[in] control the mode of the display
 *
 * @return None
 *******************************************************************************
 */
extern void glcd_control(glcd_control_t control);

/**
 *******************************************************************************
 * @brief set lcd reg
 *
 * @param[in] reg the reg to be set
 *
 * @return None
 *******************************************************************************
 */
extern void glcd_set_register(uint8_t reg);


/**
 *******************************************************************************
 * @brief set te signal, call this when te is comming
 *
 * @return None
 *******************************************************************************
 */
extern void glcd_set_te_signal();

/**
 *******************************************************************************
 * @brief register a call back function. after lcd_disp_flush/lcd_clear is
 * finished, the call back function will be called.
 *
 * @param[in] cb user function
 *
 * @return None
 *******************************************************************************
 */
extern void glcd_callback_register(glcd_cb_t cb);

/**
 *******************************************************************************
 * @brief wait for lcd_disp_flush/lcd_clear is finished
 *
 * @return None
 *******************************************************************************
 */
extern void glcd_disp_flush_finish(void);

/**
 *******************************************************************************
 * @brief get GC9C01 LCD id
 *
 * @return GC9C01 LCD id
 *******************************************************************************
 */
extern uint32_t glcd_read_id(void);

/**
 *******************************************************************************
 * @brief get GC9C01 LCD status
 *
 * @return GC9C01 LCD status
 *******************************************************************************
  */
extern uint32_t glcd_read_status(void);


#ifdef __cplusplus
}
#endif

#endif  /* __GLCD_H */


/** @} */
