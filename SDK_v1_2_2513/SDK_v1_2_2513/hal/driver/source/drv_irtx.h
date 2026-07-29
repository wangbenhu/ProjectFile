/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup IRTX IRTX
 * @ingroup  DRIVER
 * @brief    IRTX driver
 * @details  IRTX driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_IRTX_H
#define __DRV_IRTX_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_IRTX)
#include <stdint.h>
#include "om_driver.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * TYPEDEFS
 */
/// irtx carrier polarity
typedef enum {
    /// carrier begin with high level
    IRTX_CARRIER_POLARITY_HIGH  = 0x0,
    /// carrier begin with low level, duty cycle will change if config polarity low
    IRTX_CARRIER_POLARITY_LOW   = 0x1,
} irtx_carrier_polarity_t;

/// irtx output invert enable
typedef enum {
    /// do not invert output wave
    IRTX_OUTPUT_INVERT_DISABLE = 0x0,
    /// invert output wave, duty cycle will change if config invert enable
    IRTX_OUTPUT_INVERT_ENABLE  = 0x1,
} irtx_output_invert_t;

/// irtx config
typedef struct {
    uint16_t                carrier_freq;
    /* config carrier duty cycle, ranging in[0,1000]‰ when carrier polarity high and output invert disable
    * if output invert enable, actual duty cycle will be (1000 - carrier_duty_cycle) / 1000
    * if carrier polarity low, actual duty cycle will be (1000 - carrier_duty_cycle) / 1000
    */
    uint16_t                carrier_duty_cycle;
    irtx_carrier_polarity_t polarity;
    irtx_output_invert_t    invert;
} irtx_config_t;

/// irtx get wave code for high level or low level
typedef enum {
    // low wave code
    IRTX_WAVE_LEVEL_LOW,
    // high wave code
    IRTX_WAVE_LEVEL_HIGH,
} irtx_wave_level_t;

/// irtx code data type
typedef uint16_t irtx_code_t;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief init irtx
 *
 * @param[in]  irtx_config    Configuration for irtx
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_irtx_init(const irtx_config_t *irtx_config);

/**
 *******************************************************************************
 * @brief set wave code according to high and low level length
 * @param[in] wave_len_us   wave level length in us
 * @param[in] wave_level     wave level is high or low
 *
 * @return return wave code, @ref irtx_code_t
 *******************************************************************************
 */
extern irtx_code_t drv_irtx_get_wave_code(const uint32_t wave_len_us, const irtx_wave_level_t wave_level);

/**
 *******************************************************************************
 * @brief write coded data by dma and will enter dma interrupt when write done. it can work only in dma mode
 * @param[in] buff           coded data buff to be written in irtx_code_t data type
 * @param[in] buff_num       buff number of irtx_code_t type to be written
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_irtx_write_int(const irtx_code_t *buff, const uint32_t buff_num);

/**
 *******************************************************************************
 * @brief it will be called by irtx interrupt serve
 *
 *******************************************************************************
 */
extern void drv_irtx_isr(void);

#if (RTE_IRTX_REGISTER_CALLBACK)
/**
 *******************************************************************************
 * @brief Register event callback for transmit dma mode
 *
 * @param[in] cb            Pointer to callback
 *******************************************************************************
 */
extern void drv_irtx_register_isr_callback(drv_isr_callback_t cb);
#endif

/**
 *******************************************************************************
 * @brief The interrupt callback for IRTX driver. It is a weak function. User should define
 *        their own callback in user file, other than modify it in the IRTX driver.
 *
 * @param[in] event        The driver irtx event
 *                           - DRV_EVENT_IRTX_PWM_INT_PNUM_INT
 *                           - DRV_EVENT_IRTX_PWM_INT_DMA_INT
 *                           - DRV_EVENT_IRTX_PWM_INT_CYCLE_DONE_INT
 *                           - DRV_EVENT_IRTX_FIFO_CNT
 *                           - DRV_EVENT_IRTX_FIFO_EMPTY_INT
 *******************************************************************************
 */
extern void drv_irtx_isr_callback(drv_event_t event);

#ifdef __cplusplus
}
#endif


#endif  /* (RTE_IRTX) */

#endif  /* __DRV_IRTX_H */


/** @} */
