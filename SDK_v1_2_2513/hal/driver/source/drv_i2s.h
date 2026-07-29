/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup I2S I2S
 * @ingroup  DRIVER
 * @brief    I2S driver
 * @details  I2S driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_I2S_H
#define __DRV_I2S_H
/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_I2S)
#include "om_device.h"
#include "om_driver.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * TYPEDEFS
 */
/**
  \brief  Typedef of I2S role enumerate
 */
typedef enum {
    I2S_ROLE_SLAVE,
    I2S_ROLE_MASTER,
    I2S_ROLE_INVALID
} i2s_role_t;

/**
  \brief  Typedef of I2S direction enumerate
 */
typedef enum {
    I2S_DIR_IGNORE,
    I2S_DIR_RX,
    I2S_DIR_TX,
    I2S_DIR_RX_TX
} i2s_dir_t;

/**
  \brief  Typedef of I2S channel number enumerate
 */
typedef enum {
    I2S_CHN_MONO    = 1U,
    I2S_CHN_STEREO  = 2U
} i2s_chn_t;

/**
  \brief  Typedef of I2S bit width enumerate
 */
typedef enum {
    I2S_BW_12BIT  = 12U,
    I2S_BW_16BIT  = 16U,
    I2S_BW_20BIT  = 20U,
    I2S_BW_24BIT  = 24U
} i2s_bw_t;

/**
  \brief  Typedef of I2S samplerate enumerate
  \note   When use internal codec, only 8k,16k,32k are supported
 */
typedef enum {
    I2S_SR_8K    = 8000U,
    I2S_SR_11K   = 11025U,
    I2S_SR_12K   = 12000U,
    I2S_SR_16K   = 16000U,
    I2S_SR_22K   = 22050U,
    I2S_SR_24K   = 24000U,
    I2S_SR_32K   = 32000U,
    I2S_SR_44P1K = 44100U,
    I2S_SR_48K   = 48000U
} i2s_sr_t;

/**
  \brief  Structure of I2S configuration
 */
typedef struct {
    i2s_role_t  role;
    i2s_dir_t   dir;
    i2s_chn_t   channel;
    i2s_bw_t    bit_width;
    i2s_sr_t    sample_rate;
} i2s_config_t;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief I2S initialization
 *
 * @param[in] om_i2s            I2S device base
 * @param[in] config            I2S config
 *
 * @return None
 *******************************************************************************
 */
void drv_i2s_init(OM_I2S_Type *om_i2s, i2s_config_t *config);

/**
 *******************************************************************************
 * @brief I2S deinitialization
 *
 * @param[in] om_i2s            I2S device base
 *
 * @return None
 *******************************************************************************
 */
void drv_i2s_uninit(OM_I2S_Type *om_i2s);

/**
 *******************************************************************************
 * @brief I2S TX register interrupt callback for play
 *
 * @param[in] om_i2s            I2S device base
 * @param[in] isr_cb            ISR callback function
 *
 * @return None
 *******************************************************************************
 */
void drv_i2s_tx_register_isr_callback(OM_I2S_Type *om_i2s, drv_isr_callback_t isr_cb);

/**
 *******************************************************************************
 * @brief I2S RX register interrupt callback for record
 *
 * @param[in] om_i2s            I2S device base
 * @param[in] isr_cb            ISR callback function
 *
 * @return None
 *******************************************************************************
 */
void drv_i2s_rx_register_isr_callback(OM_I2S_Type *om_i2s, drv_isr_callback_t isr_cb);

/**
 *******************************************************************************
 * @brief Play buffer to I2S
 *
 * @param[in] om_i2s            I2S device base
 * @param[in] buffer            Stream buffer pointer
 * @param[in] size              Stream buffer length
 *
 * @return None
 *******************************************************************************
 */
om_error_t drv_i2s_write_dma(OM_I2S_Type *om_i2s, uint8_t *buffer, uint32_t size);

/**
 *******************************************************************************
 * @brief Record buffer from I2S
 *
 * @param[in] om_i2s            I2S device base
 * @param[in] buffer            Stream buffer pointer
 * @param[in] size              Stream buffer length
 *
 * @return None
 *******************************************************************************
 */
om_error_t drv_i2s_read_dma(OM_I2S_Type *om_i2s, uint8_t *buffer, uint32_t size);

/**
 *******************************************************************************
 * @brief GPDMA chains TX space number
 *
 * @param[in] om_i2s            I2S device base
 *
 * @return Chains space number
 *******************************************************************************
 */
uint8_t drv_i2s_tx_get_chain_space(OM_I2S_Type *om_i2s);

/**
 *******************************************************************************
 * @brief GPDMA chains RX space number
 *
 * @param[in] om_i2s            I2S device base
 *
 * @return Chains space number
 *******************************************************************************
 */
uint8_t drv_i2s_rx_get_chain_space(OM_I2S_Type *om_i2s);

/**
 *******************************************************************************
 * @brief ISR handler
 *******************************************************************************
 */
void drv_i2s_isr(void);


#ifdef __cplusplus
}
#endif

#endif  /* RTE_I2S */

#endif  /* __DRV_I2S_H */

/** @} */
