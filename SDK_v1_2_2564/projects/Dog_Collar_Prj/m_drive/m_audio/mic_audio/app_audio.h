/**
 * @file app_audio.h
 * @brief  
 * @date 2025-04-21
 * @author onmicro(tianyao.yu)
 * 
 */

#ifndef __APP_AUDIO_H__ 
#define __APP_AUDIO_H__ 

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */


/*********************************************************************
 * MACROS
 */


/*********************************************************************
 * TYPEDEFS
 */


/*********************************************************************
 * EXTERN VARIABLES
 */


typedef struct {
    const uint8_t *data;
    uint16_t length;
    uint8_t slot;
} app_audio_rx_block_t;

typedef void (*app_audio_rx_notify_t)(void *context);

/* Driver APIs: runtime start/stop is owned by audio_mic_task.c only. */
void app_audio_driver_init(app_audio_rx_notify_t notify, void *context);
bool app_audio_driver_start(void);
bool app_audio_driver_stop(void);
bool app_audio_driver_get_completed(app_audio_rx_block_t *block);
bool app_audio_driver_release(uint8_t slot);
uint32_t app_audio_driver_get_drop_count(void);
#ifdef __cplusplus
}
#endif

#endif	/* __APP_AUDIO_H__ */
