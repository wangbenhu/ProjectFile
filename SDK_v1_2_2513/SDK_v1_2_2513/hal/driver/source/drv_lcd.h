/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup LCD LCD
 * @ingroup  DRIVER
 * @brief    LCD driver
 * @details  LCD driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


#ifndef __DRV_LCD_H
#define __DRV_LCD_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_LCD)
#include <om_device.h>
#include "om_driver.h"

#ifdef  __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * TYPEDEFS
 */
/**
  \brief  Typedef of LCD control enumerate
 */
typedef enum {
    LCD_CONTROL_SET_DATA_WIDTH = 0U,    /**< With 1 argument range in 8/16/32   8: common command and data,
                                                                                16: for rgb565
                                                                                32: for rgb666 or rgb888 */
    LCD_CONTROL_TX_DATA_MODE,
    LCD_CONTROL_DATA_2_LANE_EN,         /**< only the LCD_DATA_MODE_3WM1_PARALLEL mode requires enabling this bit. for other modes, it should not be enabled */
    LCD_CONTROL_SET_CMD_BIT,            /**< with 1 argument, range in 1/2/4, x lines to send command */
    LCD_CONTROL_SET_DATA_BIT,           /**< with 1 argument, range in 1/2/4, x lines to data bits */
    LCD_CONTROL_SET_BUF_MODE_666_888,   /**< No argument, for rgb666 or rgb888, trule value is 3 */
    LCD_CONTROL_SET_BUF_MODE_COMMON,    /**< No argument, value is 4 */
    LCD_CONTROL_KEEP_CS,
    LCD_CONTROL_RELEASE_CS,
    LCD_CONTROL_SET_DUMMY_CYCLES,       /**< With 1 argument, dummy cycles. range in[0, 0x7F] */
    LCD_CONTROL_SET_CS_CONFIG,          /**< With 1 argument, pointer to lcd_cs_config_t */
} lcd_control_t;

/**
  \brief  Typedef of LCD rgb mode enumerate
 */
typedef enum {
    LCD_RGB_MODE_565 = 1U,
    LCD_RGB_MODE_666 = 2U,
    LCD_RGB_MODE_888 = 3U,
} lcd_rgb_mode_t;

/**
  \brief  Typedef of LCD data mode enumerate
 */
typedef enum {
    LCD_DATA_MODE_FLASH_LIKE = 0U,    /// target lcd like flash
    LCD_DATA_MODE_3WM1 = 1U,          /// target lcd using one more bit in cmd/data to differentiate whether it is cmd or data
    LCD_DATA_MODE_3WM2 = 2U,          /// target lcd add an additional data cable for receiving purposes.3WM1_PARALLEL can enable data_2_lan to send data in 2 lines paralleled, 3WM2 can not.
    LCD_DATA_MODE_4WM1 = 3U,          /// target lcd have one CMD line and one DATA line. The high or low state of the CMD line determines whether the DATA line carries data or a command.
    LCD_DATA_MODE_4WM2 = 4U,          /// target lcd add an additional data cable for receiving purposes.
    LCD_DATA_MODE_3WM1_PARALLEL = 5U, /// target lcd 3WM1_PARALLEL can enable data_2_lan to send data in 2 lines paralleled, 3WM2 can not.
} lcd_data_mode_t;

/**
  \brief  Typedef of LCD spi mode enumerate
 */
typedef enum {
    LCD_SPI_MODE_0 = 0U,              // Mode 0: CPOL=0, CPHA=0
    LCD_SPI_MODE_1 = 1U,              // Mode 1: CPOL=0, CPHA=1
    LCD_SPI_MODE_2 = 2U,              // Mode 2: CPOL=1, CPHA=0
    LCD_SPI_MODE_3 = 3U,              // Mode 3: CPOL=1, CPHA=1
} lcd_spi_mode_t;

/**
  \brief  Structure of LCD configuration
 */
typedef struct {
    lcd_rgb_mode_t  rgb_mode;
    lcd_data_mode_t data_mode;
    uint8_t         dly_sample;
    lcd_spi_mode_t  spi_mode;
    uint8_t         clk_div;
} lcd_config_t;

/**
  \brief  Structure of LCD CS configuration
 */
typedef struct {
    uint8_t cs_recover;
    uint8_t cs_hold;
    uint8_t cs_setup;
    uint8_t transp_head_ignore_cnt;
    uint8_t cs_pol;
} lcd_cs_config_t;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief lcd init
 *
 * @param[in] config     config lcd
 *******************************************************************************
 */
extern void drv_lcd_init(const lcd_config_t* config);

#if (RTE_LCD_REGISTER_CALLBACK)
/**
 *******************************************************************************
 * @brief Register isr callback for LCD transfer completed
 *
 * @param[in] isr_cb     Pointer to callback
 *
 *******************************************************************************
 */
extern void drv_lcd_register_isr_callback(drv_isr_callback_t isr_cb);
#endif

/**
 *******************************************************************************
 * @brief The interrupt callback for LCD driver. It is a weak function. User should define
 *        their own callback in user file, other than modify it in the LCD driver.
 *******************************************************************************
 */
extern void drv_lcd_isr_callback(void);

/**
 *******************************************************************************
 * @brief lcd write
 *
 * @param[in] cmd        command to be send
 * @param[in] cmd_bits   how many command bit to be send
 * @param[in] data       data to be send
 * @param[in] data_len   how many data to be send
 *******************************************************************************
 */
extern void drv_lcd_write(uint8_t *cmd, uint8_t cmd_bits, const uint8_t *data, uint16_t data_len);

/**
 *******************************************************************************
 * @brief lcd write int
 *
 * @param[in] cmd        command to be send
 * @param[in] cmd_bits   how many command bit to be send
 * @param[in] data       data to be send
 * @param[in] data_len   how many data to be send
 *******************************************************************************
 */
extern void drv_lcd_write_int(uint8_t *cmd, uint8_t cmd_bits, uint8_t *data, uint16_t data_len);

/**
 *******************************************************************************
 * @brief lcd read
 *
 * @param[in] cmd        command to be send
 * @param[in] cmd_bits   how many command bit to be send
 * @param[in] data       data to be read
 * @param[in] data_len   how many data to be read
 *******************************************************************************
 */
extern void drv_lcd_read(uint8_t *cmd, uint8_t cmd_bits, uint8_t *data, uint16_t data_len);

/**
 *******************************************************************************
 * @brief lcd read int
 *
 * @param[in] cmd        command to be send
 * @param[in] cmd_bits   how many command bit to be send
 * @param[in] data       data to be read
 * @param[in] data_len   how many data to be read
 *******************************************************************************
 */
extern void drv_lcd_read_int(uint8_t *cmd, uint8_t cmd_bits, uint8_t *data, uint16_t data_len);

/**
 *******************************************************************************
 * @brief control of lcd
 *
 * @param[in] control    control of lcd
 * @param[in] argu       config param
 *******************************************************************************
 */
extern void drv_lcd_control(lcd_control_t control, void *argu);

/**
 *******************************************************************************
 * @brief lcd interrupt service routine
 *******************************************************************************
 */
extern void drv_lcd_isr(void);


#ifdef  __cplusplus
}
#endif

#endif  /* (RTE_LCD) */

#endif  /* __DRV_LCD_H */

/** @} */
