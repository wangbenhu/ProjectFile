/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup RGB RGB
 * @ingroup  DRIVER
 * @brief    RGB driver
 * @details  RGB driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_RGB_H
#define __DRV_RGB_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_RGB)
#include <stdint.h>
#include "om_driver.h"

#ifdef  __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * TYPEDEFS
 */
/// RGB format
typedef enum {
    /// RGB format
    RGB_FORMAT_RGB  = 0U,
    /// GRB format
    RGB_FORMAT_GRB  = 1U,
    /// RGBW format
    RGB_FORMAT_RGBW = 2U,
    /// GRBW format
    RGB_FORMAT_GRBW = 3U,
} rgb_format_t;

/// RGB idle level
typedef enum {
    /// idle level low
    RGB_IDLE_LEVEL_LOW  = 0U,
    /// idle level hign
    RGB_IDLE_LEVEL_HIGH = 1U,
} rgb_idle_level_t;

/// RGB config
typedef struct {
    /// extra date
    uint32_t ext_data;
    /// RGB format
    rgb_format_t rgb_format;
    /// idle level
    rgb_idle_level_t idle_level;
    /// data period
    uint16_t data_per;
    /// reset period
    uint16_t reset_per;
    /// duty of data bit '0'
    uint16_t duty_zero;
    /// duty of data bit '1'
    uint16_t duty_one;
    /// clock prescaler
    uint16_t psc;
    /// free format of RGB data enable
    bool free_format_en;
    /// free format data bits
    uint8_t free_format_bits;
    /// extra data enable
    bool ext_data_en;
    /// extra date bits
    uint8_t ext_data_bits;
    /// tx reset enable
    bool tx_reset_en;
} rgb_config_t;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Initialize the RGB with parameters in rgb_config_t
 *
 * @param rgb_cfg    The configuration structure pointer, see @ref rgb_config_t
 *
 * @return           status, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_rgb_init(const rgb_config_t *rgb_cfg);

/**
 *******************************************************************************
 * @brief Uninitialize the RGB
 *
 *******************************************************************************
 */
extern void drv_rgb_uninit(void);

/**
 *******************************************************************************
 * @brief  rgb write by polling
 *
 * @param data      RGB data pointer
 * @param data_len  The date length in word
 *
 * @return status, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_rgb_write(uint32_t *data, uint16_t data_len);

/**
 *******************************************************************************
 * @brief  rgb write by interrupt
 *
 * @param data      RGB data pointer
 * @param data_len  The date length in word
 *
 * @return status, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_rgb_write_int(uint32_t *data, uint16_t data_len);

#if (RTE_GPDMA)
/**
 *******************************************************************************
 * @brief Allocate rgb dma channel
 *
 * @return status, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_rgb_gpdma_channel_allocate(void);

/**
 *******************************************************************************
 * @brief Release rgb dma channel
 *
 * @return status, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_rgb_gpdma_channel_release(void);

/**
 *******************************************************************************
 * @brief  rgb write by GPDMA
 *
 * @param data      RGB data pointer
 * @param data_len  The date length in word
 *
 * @return status:
 *    - OM_ERROR_OK:         Begin to transmit
 *    - others:              No
 *******************************************************************************
 */
extern om_error_t drv_rgb_write_dma(uint32_t *data, uint16_t data_len);
#endif

#if (RTE_RGB_REGISTER_CALLBACK)
/**
 *******************************************************************************
 * @brief Register interrupt service routine callback for specified RGB device interrupt
 *
 * @param om_rgb        The RGB device address
 * @param isr_cb          The event callback function, see @ref drv_isr_callback_t
 *
 * @return None
 *******************************************************************************
 */
extern void drv_rgb_register_isr_callback(drv_isr_callback_t isr_cb);
#endif

/**
 *******************************************************************************
 * @brief The interrupt callback for RGB driver. It is a weak function. User should define
 *        their own callback in user file, other than modify it in the RGB driver.
 *
 * @param event The driver event
 * @param data  RGB data pointer
 * @param num   The data buffer valid data number, units in word
 *
 *******************************************************************************
 */
extern void drv_rgb_isr_callback(drv_event_t event, uint32_t *data, uint16_t num);

/**
 *******************************************************************************
 * @brief The RGB interrupt service routine function, should be called in RGB IRQHandler
 *******************************************************************************
 */
extern void drv_rgb_isr(void);


#ifdef  __cplusplus
}
#endif

#endif  /* (RTE_RGB) */

#endif /* __DRV_RGB_H */


/** @} */
