/**
 * @file app_audio.h
 * @brief  
 * @date 2025-04-21
 * @author onmicro(tianyao.yu)
 * 
 */

#ifndef __APP_AUDIO_H__ 
#define __APP_AUDIO_H__ 

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


/*********************************************************************
 * EXTERN FUNCTIONS
 */
void app_audio_init(void);
void app_audio_uint(void);
void app_audio_open(void);
void app_audio_close(void);
void vStarAudioTask(void);
#ifdef __cplusplus
}
#endif

#endif	/* __APP_AUDIO_H__ */
