/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup GPADC GPADC
 * @ingroup  DRIVER
 * @brief    GPADC driver
 * @details  GPADC driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_GPADC_H
#define __DRV_GPADC_H
/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_GPADC)
#include <stdint.h>
#include "om_driver.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * TYPEDEFS
 */
/// GPADC channels P
typedef enum {
    /// temperature channel
    GPADC_CH_P_TEMPERATURE = (1<<0U),
    /// VBAT channel
    GPADC_CH_P_VBAT        = (1<<1U),
    /// GPIO2 channel
    GPADC_CH_P_GPIO2       = (1<<4U),
    /// GPIO3 channel
    GPADC_CH_P_GPIO3       = (1<<5U),
    /// GPIO7 channel
    GPADC_CH_P_GPIO7       = (1<<6U),
    /// GPIO8 channel
    GPADC_CH_P_GPIO8       = (1<<7U),
    /// GPIO11 channel
    GPADC_CH_P_GPIO11      = (1<<8U),
    /// GPIO12 channel
    GPADC_CH_P_GPIO12      = (1<<9U),
    /// GPIO14 channel
    GPADC_CH_P_GPIO14      = (1<<10U),
    /// GPIO15 channel
    GPADC_CH_P_GPIO15      = (1<<11U),
} drv_gpadc_channel_p_t;

/// GPADC channels N
typedef enum {
    /// VBAT channel
    GPADC_CH_N_AVSS        = 3U,
    /// GPIO2 channel
    GPADC_CH_N_GPIO2       = 4U,
    /// GPIO3 channel
    GPADC_CH_N_GPIO3       = 5U,
    /// GPIO7 channel
    GPADC_CH_N_GPIO7       = 6U,
    /// GPIO8 channel
    GPADC_CH_N_GPIO8       = 7U,
    /// GPIO11 channel
    GPADC_CH_N_GPIO11      = 8U,
    /// GPIO12 channel
    GPADC_CH_N_GPIO12      = 9U,
    /// GPIO14 channel
    GPADC_CH_N_GPIO14      = 10,
    /// GPIO15 channel
    GPADC_CH_N_GPIO15      = 11,
} drv_gpadc_channel_n_t;

/// GPADC mode
typedef enum {
    /// single ended mode
    GPADC_MODE_SINGLE  = 0U,
    /// differential mode
    GPADC_MODE_DIFF    = 1U,
} drv_gpadc_mode_t;

/// GPADC gain
typedef enum {
    /// gain: 1, external reference
    GPADC_GAIN_1_EXTERNAL_REF            = 0U,
    /// gain: 1/3, internal reference
    GPADC_GAIN_1_3_INTERNAL_REF          = 1U,
    /// gain: 1, internal reference
    GPADC_GAIN_1_INTERNAL_REF            = 2U,

    GPADC_GAIN_MAX,
} drv_gpadc_gain_t;

/// GPADC summation number
typedef enum {
    /// summation number: 2
    GPADC_SUM_NUM_2   = 0U,
    /// summation number: 4
    GPADC_SUM_NUM_4   = 1U,
    /// summation number: 8
    GPADC_SUM_NUM_8   = 2U,
    /// summation number: 16
    GPADC_SUM_NUM_16  = 3U,
    /// summation number: 32
    GPADC_SUM_NUM_32  = 4U,
    /// summation number: 64
    GPADC_SUM_NUM_64  = 5U,
    /// summation number: 128
    GPADC_SUM_NUM_128 = 6U,
    /// summation number: 256
    GPADC_SUM_NUM_256 = 7U,

    GPADC_SUM_NUM_MAX,
} drv_gpadc_sum_num_t;

/// GPADC sampling cycles
typedef enum {
    /// sampling cycles: 0
    GPADC_SAMPLING_CYCLES_0   = 0U,
    /// sampling cycles: 2
    GPADC_SAMPLING_CYCLES_2   = 1U,
    /// sampling cycles: 8
    GPADC_SAMPLING_CYCLES_8   = 2U,
    /// sampling cycles: 16
    GPADC_SAMPLING_CYCLES_16  = 3U,
    /// sampling cycles: 32
    GPADC_SAMPLING_CYCLES_32  = 4U,
    /// sampling cycles: 64
    GPADC_SAMPLING_CYCLES_64  = 5U,
    /// sampling cycles: 80
    GPADC_SAMPLING_CYCLES_80  = 6U,
    /// sampling cycles: 128
    GPADC_SAMPLING_CYCLES_128 = 7U,
    /// sampling cycles: 256
    GPADC_SAMPLING_CYCLES_256 = 8U,
    /// sampling cycles: 512
    GPADC_SAMPLING_CYCLES_512 = 9U,
    /// sampling cycles: 1024
    GPADC_SAMPLING_CYCLES_1024 = 10U,
    /// sampling cycles: 2048
    GPADC_SAMPLING_CYCLES_2048 = 11U,
    /// sampling cycles: 4096
    GPADC_SAMPLING_CYCLES_4096 = 12U,

    GPADC_SAMPLING_CYCLES_MAX,
} drv_gpadc_sampling_cycles_t;

/// GPADC calib sel
typedef enum {
    GPADC_CALIB_FT_SINGLE = 0,
    GPADC_CALIB_CP_1      = 1,
    GPADC_CALIB_CP_2      = 2,
    GPADC_CALIB_FT_DIFF_1 = 3,
    GPADC_CALIB_FT_DIFF_2 = 4,
} drv_gpadc_calib_sel_t;

/// GPADC control
typedef enum {
    GPADC_CONTROL_ENABLE_CLOCK                      = 0U,    /**< enable GPADC clock */
    GPADC_CONTROL_DISABLE_CLOCK                     = 1U,    /**< enable GPADC clock */
    GPADC_CONTROL_SET_PARAM                         = 2U,    /**< set calibration paramters */
    GPADC_CONTROL_IS_BUSY                           = 3U,    /**< check GPADC is busy */
    GPADC_CONTROL_READ_TEMPERATURE                  = 4U,    /**< read temperature */
    GPADC_CONTROL_SET_CALIB_TEMPER                  = 5U,    /**< set temperature during calibration */
    GPADC_CONTROL_TEMPERATURE_COMPEN                = 6U,    /**< check GPADC is busy */
    GPADC_CONTROL_SET_PARAM_WITHOUT_POWER_ON_PARAM  = 7U,    /**< set calibration paramters without power on paramters */
    GPADC_CONTROL_CHANNEL_CFG                       = 8U,    /**< config GPADC channels */
    GPADC_CONTROL_GET_GAIN_ERROR                    = 9U,    /**< get gain error */
    GPADC_CONTROL_GET_VOS                           = 10U,   /**< get vos */
    GPADC_CONTROL_SET_AUTO_COMPEN                   = 11U,   /**< set AUTO COMPEN */
} drv_gpadc_control_t;

/// GPADC temperature compensation
typedef struct {
    /// GPADC temperature data
    int16_t  temp_data;
    /// GPADC vbat date
    int16_t  vbat_data;
} drv_gpadc_temperature_compen_t;

/// GPADC config
typedef struct {
    /// GPADC channels P select
    uint16_t                    channel_p;
    /// GPADC channel N select
    drv_gpadc_channel_n_t       channel_n;
    /// GPADC mode
    drv_gpadc_mode_t            mode;
    /// GPADC gain
    drv_gpadc_gain_t            gain;
    /// GPADC summation number
    drv_gpadc_sum_num_t         sum_num;
    /// GPADC sampling cycles
    drv_gpadc_sampling_cycles_t sampling_cycles;
} drv_gpadc_config_t;

/// GPADC calibration parameters in efuse
typedef struct {
    uint16_t gain_error;
    uint16_t vos;
    uint16_t vos_temp;
} __PACKED drv_gpadc_calib_t;

/// GPADC calibration parameters in flash
typedef struct {
    drv_gpadc_calib_t data[GPADC_GAIN_MAX];
} __PACKED drv_gpadc_flash_calib_t;

/// GPADC extral calibration parameters in flash
typedef struct {
    uint16_t gain_error_vbat;
} __PACKED drv_gpadc_flash_calib_ex_1_t;

/// GPADC extral calibration parameters in flash
typedef struct {
    uint16_t vbg_code_trim_1_vbat_3p3v;
    uint16_t vbg_code_trim_3_vbat_3p3v;
} __PACKED drv_gpadc_flash_calib_ex_2_t;

/// GPADC extral calibration parameters in flash
typedef struct {
    uint16_t vbg_code_trim_1_vbat_2p3v;
    uint16_t vbg_code_trim_3_vbat_2p3v;
    int16_t temperature_vbat_3p3v;
    int16_t temperature_vbat_2p3v;
} __PACKED drv_gpadc_flash_calib_ex_3_t;

/// GPADC extral calibration parameters in flash
typedef struct {
    drv_gpadc_calib_t data_diff[GPADC_GAIN_MAX-1];
} __PACKED drv_gpadc_flash_calib_ex_4_t;

/// GPADC extral calibration parameters in flash
typedef struct {
    uint16_t vbg_code_trim_1_vbat_3p3v_diff;
    uint16_t vbg_code_trim_3_vbat_3p3v_diff;
} __PACKED drv_gpadc_flash_calib_ex_5_t;

/// GPADC extral calibration parameters in flash
typedef struct {
    uint16_t vbg_code_trim_1_vbat_3p3v_gain_1;
    uint16_t vbg_code_trim_3_vbat_3p3v_gain_1;
} __PACKED drv_gpadc_flash_calib_ex_6_t;

/// GPADC extral calibration parameters in flash
typedef struct {
    uint16_t vbg_code_trim_1_vbat_3p3v_gain_1_diff;
    uint16_t vbg_code_trim_3_vbat_3p3v_gain_1_diff;
} __PACKED drv_gpadc_flash_calib_ex_7_t;

/// GPADC calibration parameters
typedef struct {
    const drv_gpadc_calib_t             *efuse;
    const drv_gpadc_flash_calib_t       *flash;
    const drv_gpadc_flash_calib_ex_1_t  *flash_ex_1;
    const drv_gpadc_flash_calib_ex_2_t  *flash_ex_2;
    const drv_gpadc_flash_calib_ex_3_t  *flash_ex_3;
    const drv_gpadc_flash_calib_ex_4_t  *flash_ex_4;
    const drv_gpadc_calib_t             *efuse_ex_1;
    const drv_gpadc_flash_calib_ex_5_t  *flash_ex_5;
    const drv_gpadc_flash_calib_ex_6_t  *flash_ex_6;
    const drv_gpadc_flash_calib_ex_7_t  *flash_ex_7;
    const void                          *reserved[2];
} drv_gpadc_cpft_calib_t;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief GPADC interrupt service routine
 *******************************************************************************
 **/
extern void drv_gpadc_isr(void);

/**
 *******************************************************************************
 * @brief GPADC initialization
 *
 * @param[in] config         Configuration for GPADC
 *
 * @return status:
 *    - OM_ERROR_OK:         init done
 *    - others:              No
 *******************************************************************************
 */
extern om_error_t drv_gpadc_init(const drv_gpadc_config_t *config);

/**
 *******************************************************************************
 * @brief Register isr callback for transmit/receive by interrupt & gpdma mode
 *
 * @param[in] cb       Pointer to callback
 *******************************************************************************
 */
extern void drv_gpadc_register_isr_callback(drv_isr_callback_t cb);

/**
 *******************************************************************************
 * @brief Prepare receive number of data by block mode
 *
 * @param[in] channel_p      gpadc channels P
 * @param[in] data           Pointer where data to receive from gpadc.when reading the temperature,
 *                           for example,read 265 for 26.5 degrees Celsius.when the voltage is read,
 *                           give an example,2900 indicates 2900 millivolts.
 * @param[in] num            Number of data from gpadc channel_p
 *
 * @return status:
 *    - OM_ERROR_OK:         receive done
 *    - others:              No
 *******************************************************************************
 */
om_error_t drv_gpadc_read(uint16_t channel_p, int16_t *data, uint16_t num);

/**
 *******************************************************************************
 * @brief Prepare receive number of data by interrupt mode,
 *
 * @param[in] channel_p      gpadc channels P
 * @param[in] data           Pointer where data to receive from gpadc
 * @param[in] num            Number of data from gpadc receiver
 *
 * @return status:
 *    - OM_ERROR_OK:         Begin to receive
 *    - others:              No
 *******************************************************************************
 */
om_error_t drv_gpadc_read_int(uint16_t channel_p, int16_t *data, uint16_t num);

#if (RTE_GPDMA)
/**
 *******************************************************************************
 * @brief Allocate gpdma channel for gpadc
 *
 * @return status:
 *    - OM_ERROR_OK:         Allocate ok
 *    - others:              No
 *******************************************************************************
 */
om_error_t drv_gpadc_gpdma_channel_allocate(void);

/**
 *******************************************************************************
 * @brief Release gpdma channel for gpadc
 *
 * @return status:
 *    - OM_ERROR_OK:         Release ok
 *    - others:              No
 *******************************************************************************
 */
om_error_t drv_gpadc_gpdma_channel_release(void);

/**
 *******************************************************************************
 * @brief Prepare receive number of data by GPDMA mode
 *
 * @param[in] channel_p      gpadc channels P
 * @param[in] data           Pointer where data to receive from gpadc
 * @param[in] num            Number of data from gpadc receiver
 *
 * @return status:
 *    - OM_ERROR_OK:         Begin to receive
 *    - others:              No
 *******************************************************************************
 */
om_error_t drv_gpadc_read_dma(uint16_t channel_p, int16_t *data, uint16_t num);
#endif  /* (RTE_GPDMA) */

/**
 *******************************************************************************
 * @brief Control GPADC interface.
 *
 * @param[in] control        Operation
 * @param[in] argu           Operation argument
 *
 * @return status:           Control status
 *******************************************************************************
 */
extern void *drv_gpadc_control(drv_gpadc_control_t control, void *argu);

#if (RTE_GPADC_CALIB_EN)
/**
 *******************************************************************************
 * @brief Calibrate GPADC and initialize GPADC compensation parameters.
 *
 * @param[in] gain                  gpadc gain
 * @param[in] calib_sel             gpadc select calibration phase
 * @param[out] pvalue               Pointer to @p drv_gpadc_calib_t object where the
 *                                  calibration parameters are to be returned.
 * @param[out] pvalue_vbat          Pointer to @p drv_gpadc_flash_calib_ex_1_t object where the
 *                                  calibration extra parameters are to be returned.
 * @param[out] pvalue_power_on_3p3v Pointer to @p drv_gpadc_flash_calib_ex_2_t object where the
 *                                  calibration extra parameters are to be returned.
 * @param[out] pvalue_power_on_2p3v Pointer to @p drv_gpadc_flash_calib_ex_3_t object where the
 *                                  calibration extra parameters are to be returned.
 *
 * @return status:
 *    - OM_ERROR_OK:                Calibrate done
 *    - others:                     No
 *******************************************************************************
 */
om_error_t drv_gpadc_calibrate(drv_gpadc_calib_t *pvalue, drv_gpadc_flash_calib_ex_1_t *pvalue_vbat,
                               drv_gpadc_flash_calib_ex_2_t *pvalue_power_on_3p3v, drv_gpadc_flash_calib_ex_3_t *pvalue_power_on_2p3v,
                               drv_gpadc_gain_t gain, drv_gpadc_calib_sel_t calib_sel);
#endif

#ifdef __cplusplus
}
#endif

#endif  /* RTE_GPADC */
#endif  /* __DRV_GPADC_H */


/** @} */
