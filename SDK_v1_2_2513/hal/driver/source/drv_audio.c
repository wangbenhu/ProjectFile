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
 * @brief    AUDIO driver source file
 * @details  AUDIO driver source file
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
#if (RTE_AUDIO)
#include "om_device.h"
#include "om_driver.h"


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/// Exern functions for internal use
extern void drv_audio_inside_init(void);
extern void drv_audio_inside_uninit(void);
extern om_error_t drv_audio_inside_record_start(audio_record_config_t *config);
extern om_error_t drv_audio_inside_record_stop(void);
extern audio_state_t drv_audio_inside_work_state(void);
extern void drv_audio_inside_isr(void);

extern void drv_audio_outside_init(void);
extern void drv_audio_outside_uninit(void);
extern om_error_t drv_audio_outside_play_start(audio_play_config_t *config);
extern om_error_t drv_audio_outside_play_stop(void);
extern om_error_t drv_audio_outside_record_start(audio_record_config_t *config);
extern om_error_t drv_audio_outside_record_stop(void);
extern audio_state_t drv_audio_outside_work_state(void);


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
void drv_audio_init(void)
{
    #if RTE_AUDIO_USE_INTERNAL
    drv_audio_inside_init();
    #else
    drv_audio_outside_init();
    #endif
}

void drv_audio_uninit(void)
{
    #if RTE_AUDIO_USE_INTERNAL
    drv_audio_inside_uninit();
    #else
    drv_audio_outside_uninit();
    #endif
}

om_error_t drv_audio_play_start(audio_play_config_t *config)
{
    #if RTE_AUDIO_USE_EXTERNAL
    return drv_audio_outside_play_start(config);
    #else
    (void)config;
    return OM_ERROR_UNSUPPORTED;
    #endif
}

om_error_t drv_audio_play_stop(void)
{
    #if RTE_AUDIO_USE_EXTERNAL
    return drv_audio_outside_play_stop();
    #else
    return OM_ERROR_UNSUPPORTED;
    #endif
}

om_error_t drv_audio_record_start(audio_record_config_t *config)
{
    #if RTE_AUDIO_USE_INTERNAL
    return drv_audio_inside_record_start(config);
    #else
    return drv_audio_outside_record_start(config);
    #endif
}

om_error_t drv_audio_record_stop(void)
{
    #if RTE_AUDIO_USE_INTERNAL
    return drv_audio_inside_record_stop();
    #else
    return drv_audio_outside_record_stop();
    #endif
}

audio_state_t drv_audio_work_state(void)
{
    #if RTE_AUDIO_USE_INTERNAL
    return drv_audio_inside_work_state();
    #else
    return drv_audio_outside_work_state();
    #endif
}

void drv_audio_isr(void)
{
    #if RTE_AUDIO_USE_INTERNAL
    drv_audio_inside_isr();
    #else
    while(1);
    #endif
}


#endif /* RTE_AUDIO */

/** @} */