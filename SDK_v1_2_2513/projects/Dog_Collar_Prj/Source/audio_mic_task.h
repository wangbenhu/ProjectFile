#ifndef __AUDIO_MIC_TASK_H__
#define __AUDIO_MIC_TASK_H__

#include "common_def.h"
#include "FreeRTOS.h"

osThreadId_t vStartAudioMicTask(void);
osStatus_t audio_mic_start(uint32_t duration_ms);
osStatus_t audio_mic_stop(void);

#endif
