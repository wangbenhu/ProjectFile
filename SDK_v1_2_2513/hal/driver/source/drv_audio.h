/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup AUDIO AUDIO
 * @ingroup  DRIVER
 * @brief    AUDIO driver
 * @details  AUDIO driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_AUDIO_H
#define __DRV_AUDIO_H
/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_AUDIO)
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
  \brief  Typedef of analog ADC gain enumerate
 */
typedef enum {
    AUDIO_ADC_GAIN_N6DB,
    AUDIO_ADC_GAIN_0DB,
    AUDIO_ADC_GAIN_6DB,
    AUDIO_ADC_GAIN_12DB
} audio_adc_gain_t;

/**
  \brief  Typedef of analog PGA gain enumerate
 */
typedef enum {
    AUDIO_PGA_GAIN_N3DB,
    AUDIO_PGA_GAIN_0DB,
    AUDIO_PGA_GAIN_3DB,
    AUDIO_PGA_GAIN_6DB,
    AUDIO_PGA_GAIN_9DB,
    AUDIO_PGA_GAIN_12DB,
    AUDIO_PGA_GAIN_15DB,
    AUDIO_PGA_GAIN_18DB,
    AUDIO_PGA_GAIN_21DB,
    AUDIO_PGA_GAIN_24DB,
    AUDIO_PGA_GAIN_27DB,
    AUDIO_PGA_GAIN_30DB,
} audio_pga_gain_t;

/**
  \brief  Typedef of digist gain before filter enumerate
 */
typedef enum {
    AUDIO_ADC_CIC_SCALE_0DB,
    AUDIO_ADC_CIC_SCALE_2DB,
    AUDIO_ADC_CIC_SCALE_3P5DB,
    AUDIO_ADC_CIC_SCALE_6DB_1,
    AUDIO_ADC_CIC_SCALE_6DB_2,
    AUDIO_ADC_CIC_SCALE_8DB,
    AUDIO_ADC_CIC_SCALE_9P5DB,
    AUDIO_ADC_CIC_SCALE_12DB
} audio_adc_cic_scale_t;

/**
  \brief  Typedef of audio state enumerate
 */
typedef enum {
    AUDIO_STATE_IDLE        = 0x0,
    AUDIO_STATE_UNINITED    = 0x0,
    AUDIO_STATE_INITED      = 0x1,
    AUDIO_STATE_PLAY        = 0x2,
    AUDIO_STATE_RECORD      = 0x4,
} audio_state_t;

/**
  \brief  Typedef of config for internal or external codec
 */
#if RTE_AUDIO_USE_INTERNAL
typedef struct {
    /// Placeholder
    uint8_t                 reserve;
} audio_play_config_t;

typedef struct {
    i2s_chn_t               channel;
    i2s_bw_t                bit_width;
    i2s_sr_t                sample_rate;

    uint8_t                 volume;                     /**< volume: 0x64 means 0dB, 0.5dB step, max 24.5dB */

    audio_adc_gain_t        gain_adc;
    audio_pga_gain_t        gain_pga;
    audio_adc_cic_scale_t   gain_cic;
} audio_record_config_t;

typedef struct {
    uint8_t                 at;                         /**< attack time[0-F], 0=6ms, 1=12ms and so on */
    uint8_t                 dt;                         /**< decay  time[0-F], 0=24ms,1=48ms and so on */
    uint8_t                 ht;                         /**< hold   time[0-F], 0=0ms, 1=2.67ms and so on */
    uint8_t                 tar_gain;                   /**< target gain[0x00-0xFF],0xFF=0dB */
    uint8_t                 ngth_gain;                  /**< noise gate gain[0x00-0xFF],0xFF=0dB */
    uint8_t                 ntype;                      /**< noise gate type, 0=gain held constant,1=mute adc output */
    uint8_t                 noise_jitter;               /**< noise threshold margin */
    uint8_t                 target_jitter;              /**< target margin */
} audio_alc_config_t;

#else /* RTE_AUDIO_USE_INTERNAL */

typedef struct {
    i2s_chn_t               channel;
    i2s_bw_t                bit_width;
    i2s_sr_t                sample_rate;

    uint8_t                 volume;                     /**< volume: 0x79 means 0dB, dec 1 means dec 1dB */
} audio_play_config_t;

typedef struct {
    i2s_chn_t               channel;
    i2s_bw_t                bit_width;
    i2s_sr_t                sample_rate;

    uint8_t                 volume;                     /**< volume: 0xC3 means 0dB, dec 1 means dec 0.5dB */
} audio_record_config_t;

typedef struct {
    /// Placeholder
    uint8_t                 reserve;
} audio_alc_config_t;

#endif /* RTE_AUDIO_USE_INTERNAL */


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Audio initialization
 *******************************************************************************
 */
void drv_audio_init(void);

/**
 *******************************************************************************
 * @brief Audio deinitialization
 *******************************************************************************
 */
void drv_audio_uninit(void);

/**
 *******************************************************************************
 * @brief Audio play start
 *
 * @param[in] config            Play config
 *
 * @return see @ref om_error_t
 *******************************************************************************
 */
om_error_t drv_audio_play_start(audio_play_config_t *config);

/**
 *******************************************************************************
 * @brief Audio play stop
 *
 * @return see @ref om_error_t
 *******************************************************************************
 */
om_error_t drv_audio_play_stop(void);

/**
 *******************************************************************************
 * @brief Audio record start
 *
 * @param[in] config            Record config
 *
 * @return see @ref om_error_t
 *******************************************************************************
 */
om_error_t drv_audio_record_start(audio_record_config_t *config);

/**
 *******************************************************************************
 * @brief Audio record stop
 *
 * @return see @ref om_error_t
 *******************************************************************************
 */
om_error_t drv_audio_record_stop(void);

/**
 *******************************************************************************
 * @brief Audio get work state
 *
 * @return see @ref audio_state_t
 *******************************************************************************
 */
audio_state_t drv_audio_work_state(void);

/**
 *******************************************************************************
 * @brief ISR handler
 *******************************************************************************
 */
void drv_audio_isr(void);


/*******************************************************************************
 * INLINE FUNCTIONS
 */
__STATIC_FORCEINLINE void drv_audio_set_adc_vol(uint8_t vol)
{
    audio_state_t state = drv_audio_work_state();
    if(state < AUDIO_STATE_INITED) {
        return;
    }

#if RTE_AUDIO_USE_INTERNAL
    // Wait updated last time
    while(OM_AUDIO->ADC_VOL_CTRL & AU_VOL_UPDATE_MASK);
    // Disable mute and unmute flow, bypass
    register_set(&OM_AUDIO->ADC_VOL_CTRL, MASK_3REG(AU_VOL_ADCUNMU, 0, AU_VOL_ADCMU, 0, AU_VOL_MUTE_BYPASS, 1));
    // Set new volume
    register_set(&OM_AUDIO->ADC_VOL_CTRL, MASK_2REG(AU_VOL_LADC, vol, AU_VOL_UPDATE, 1));
#else
    (void)vol;
#endif /* RTE_AUDIO_USE_INTERNAL */
}

__STATIC_FORCEINLINE uint8_t drv_audio_get_adc_vol(void)
{
    audio_state_t state = drv_audio_work_state();
    if(state < AUDIO_STATE_INITED) {
        return 0;
    }

#if RTE_AUDIO_USE_INTERNAL
    return (uint8_t)register_get(&OM_AUDIO->ADC_VOL_CTRL, MASK_POS(AU_VOL_LADC));
#else
    return 0;
#endif /* RTE_AUDIO_USE_INTERNAL */
}

__STATIC_FORCEINLINE void drv_audio_set_dac_vol(uint8_t vol)
{
    audio_state_t state = drv_audio_work_state();
    if(state < AUDIO_STATE_INITED) {
        return;
    }

#if RTE_AUDIO_USE_INTERNAL
    (void)vol;
#else
    (void)vol;
#endif /* RTE_AUDIO_USE_INTERNAL */
}

__STATIC_FORCEINLINE uint8_t drv_audio_get_dac_vol(void)
{
    audio_state_t state = drv_audio_work_state();
    if(state < AUDIO_STATE_INITED) {
        return 0;
    }

#if RTE_AUDIO_USE_INTERNAL
    return 0;
#else
    return 0;
#endif /* RTE_AUDIO_USE_INTERNAL */
}

__STATIC_FORCEINLINE void drv_audio_adc_mute(bool mute)
{
    audio_state_t state = drv_audio_work_state();
    if(state < AUDIO_STATE_INITED) {
        return;
    }

#if RTE_AUDIO_USE_INTERNAL
    register_set(&OM_AUDIO->ADC_VOL_CTRL, MASK_5REG(AU_VOL_MUTE_RATE, 5, AU_VOL_MUTE_RATE, 5, AU_VOL_MUTE_BYPASS, 0, AU_VOL_ADCUNMU, !mute, AU_VOL_ADCMU, mute));
#else
    (void)mute;
#endif /* RTE_AUDIO_USE_INTERNAL */
}

__STATIC_FORCEINLINE void drv_audio_dac_mute(bool mute)
{
    audio_state_t state = drv_audio_work_state();
    if(state < AUDIO_STATE_INITED) {
        return;
    }

#if RTE_AUDIO_USE_INTERNAL
    (void)mute;
#else
    (void)mute;
#endif /* RTE_AUDIO_USE_INTERNAL */
}

__STATIC_FORCEINLINE void drv_audio_alc_config(bool alc_en, bool ng_en, audio_alc_config_t *config)
{
    audio_state_t state = drv_audio_work_state();
    if(state < AUDIO_STATE_INITED) {
        return;
    }

#if RTE_AUDIO_USE_INTERNAL
    if(config) {
        register_set(&OM_AUDIO->ALC_CTRL_1, MASK_4REG(AU_ALC_ATTACK,        (config->at & 0x0F),
                                                      AU_ALC_DECAY,         (config->dt & 0x0F),
                                                      AU_ALC_HOLD,          (config->ht & 0x0F),
                                                      AU_ALC_NOISE_TYPE,    (config->ntype & 0x01)));

        register_set(&OM_AUDIO->ALC_CTRL_2, MASK_4REG(AU_ALC_TARGET,        config->tar_gain,
                                                      AU_ALC_TARGET_JITTER, config->target_jitter,
                                                      AU_ALC_NGTH,          config->ngth_gain,
                                                      AU_ALC_NOISE_JITTER,  config->noise_jitter));
    }

    register_set(&OM_AUDIO->ALC_CTRL_1, MASK_2REG(AU_ALC_DIG_EN, alc_en, AU_ALC_NOISE_EN, ng_en));

#else
#endif /* RTE_AUDIO_USE_INTERNAL */
}


#ifdef __cplusplus
}
#endif

#endif  /* RTE_AUDIO */

#endif  /* __DRV_AUDIO_H */

/** @} */
