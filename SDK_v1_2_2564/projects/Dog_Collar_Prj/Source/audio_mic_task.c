#include "audio_mic_task.h"
#include "app_audio.h"
#include "om_driver.h"
#include <math.h>

#define AUDIO_MIC_TASK_STACK_SIZE (10U * 2048U)
#define AUDIO_MIC_TASK_PRIORITY   (osPriorityNormal)
#define AUDIO_MIC_EVT_COMMAND     (1U << 0)
#define AUDIO_MIC_EVT_DMA_READY   (1U << 1)

typedef enum { AUDIO_MIC_CMD_START, AUDIO_MIC_CMD_STOP } audio_mic_cmd_type_t;
typedef struct { audio_mic_cmd_type_t type; uint32_t duration_ms; } audio_mic_cmd_t;

static osThreadId_t audio_mic_thread;
static osEventFlagsId_t audio_mic_events;
static osMessageQueueId_t audio_mic_commands;

typedef struct { int16_t b0, b1, b2, a1, a2; int32_t state[2]; } audio_biquad_t;
typedef struct { audio_biquad_t bp; int32_t env; int32_t gain; } audio_agc_t;
static audio_agc_t speech_agc;

static void speech_band_agc_init(audio_agc_t *agc)
{
    float w0 = 2.0f * 3.14159265358979323846f * 1700.0f / 8000.0f;
    float alpha = sinf(w0) / (2.0f * 0.67f);
    agc->bp.b0 = (int16_t)(alpha * 32767.0f); agc->bp.b1 = 0;
    agc->bp.b2 = (int16_t)(-alpha * 32767.0f);
    agc->bp.a1 = (int16_t)(-2.0f * cosf(w0) * 32767.0f);
    agc->bp.a2 = (int16_t)((1.0f - alpha) * 32767.0f);
    agc->bp.state[0] = agc->bp.state[1] = agc->env = 0; agc->gain = 32768;
}

static int16_t speech_band_agc_process(audio_agc_t *agc, int16_t sample)
{
    int32_t w = (int32_t)sample - ((agc->bp.a1 * agc->bp.state[0]) >> 15) - ((agc->bp.a2 * agc->bp.state[1]) >> 15);
    int32_t out = ((agc->bp.b0 * w) >> 15) + ((agc->bp.b1 * agc->bp.state[0]) >> 15) + ((agc->bp.b2 * agc->bp.state[1]) >> 15);
    int32_t abs_out = out < 0 ? -out : out;
    agc->bp.state[1] = agc->bp.state[0]; agc->bp.state[0] = w;
    agc->env += (abs_out - agc->env) >> (abs_out > agc->env ? 3 : 7);
    if (agc->env > 100) { int32_t wanted = ((int32_t)16384 << 15) / agc->env; if (wanted > 196608) wanted = 196608; agc->gain += (wanted - agc->gain) >> 5; }
    return (int16_t)__SSAT((((int32_t)sample * agc->gain >> 15) * 3), 16);
}

static void audio_mic_dma_notify(void *context)
{
    osEventFlagsSet((osEventFlagsId_t)context, AUDIO_MIC_EVT_DMA_READY);
}

static void audio_mic_process_block(const app_audio_rx_block_t *block)
{
    const int16_t *pcm16 = (const int16_t *)block->data;
    uint8_t pcm8[120];
    uint32_t samples = block->length / 2U;
    for (uint32_t i = 0U; i < samples; ++i) pcm8[i] = (uint8_t)(speech_band_agc_process(&speech_agc, pcm16[i]) >> 8);
    drv_uart_write(OM_UART0, pcm8, samples, 10U);
}

static void audioMic_task(void *argument)
{
    bool recording = false;
    uint32_t deadline = 0U;
    (void)argument;
  //  audio_mic_events = osEventFlagsNew(NULL);
  //  audio_mic_commands = osMessageQueueNew(4U, sizeof(audio_mic_cmd_t), NULL);
    app_audio_driver_init(audio_mic_dma_notify, audio_mic_events);

    for (;;) {
        int32_t remaining = deadline ? (int32_t)(deadline - osKernelGetTickCount()) : -1;
        uint32_t timeout = recording && deadline ? (remaining > 0 ? (uint32_t)remaining : 0U) : osWaitForever;
        uint32_t events = osEventFlagsWait(audio_mic_events, AUDIO_MIC_EVT_COMMAND | AUDIO_MIC_EVT_DMA_READY,
                                            osFlagsWaitAny, timeout);
        audio_mic_cmd_t cmd;
        while (osMessageQueueGet(audio_mic_commands, &cmd, NULL, 0U) == osOK) {
            if (cmd.type == AUDIO_MIC_CMD_START) {
                if (!recording) { speech_band_agc_init(&speech_agc); app_audio_driver_start(); recording = true; }
                deadline = cmd.duration_ms ? osKernelGetTickCount() + osMS2TicksRound(cmd.duration_ms) : 0U;
            } else if (recording) {
                app_audio_driver_stop(); recording = false; deadline = 0U;
            }
        }
        if (recording && deadline && (int32_t)(osKernelGetTickCount() - deadline) >= 0) {
            app_audio_driver_stop(); recording = false; deadline = 0U;
        }
        if (recording && (events & AUDIO_MIC_EVT_DMA_READY)) {
            app_audio_rx_block_t block;
            while (app_audio_driver_get_completed(&block)) {
                audio_mic_process_block(&block);
                app_audio_driver_release(block.slot);
            }
        }
    }
}

/**
 * @brief Start audio mic recording
 * @param duration_ms Recording duration in ms (0 = continuous)
 * @return osOK on success, osErrorResource on failure
 */
osStatus_t audio_mic_start(uint32_t duration_ms)
{
    audio_mic_cmd_t cmd = { AUDIO_MIC_CMD_START, duration_ms };
    if (audio_mic_commands == NULL || osMessageQueuePut(audio_mic_commands, &cmd, 0U, 0U) != osOK) return osErrorResource;
    return (osEventFlagsSet(audio_mic_events, AUDIO_MIC_EVT_COMMAND) & osFlagsError) ? osError : osOK;
}

osStatus_t audio_mic_stop(void)
{
    audio_mic_cmd_t cmd = { AUDIO_MIC_CMD_STOP, 0U };
    if (audio_mic_commands == NULL || osMessageQueuePut(audio_mic_commands, &cmd, 0U, 0U) != osOK) return osErrorResource;
    return (osEventFlagsSet(audio_mic_events, AUDIO_MIC_EVT_COMMAND) & osFlagsError) ? osError : osOK;
}

osThreadId_t vStartAudioMicTask(void)
{
    const osThreadAttr_t attr = { 
		.name = "AudioMic_Task",
	.stack_size = AUDIO_MIC_TASK_STACK_SIZE, 
	.priority = AUDIO_MIC_TASK_PRIORITY 
	};
    if (audio_mic_thread == NULL) {
        /* Create queue and event flags before thread to avoid race */
        audio_mic_events = osEventFlagsNew(NULL);
        audio_mic_commands = osMessageQueueNew(4U, sizeof(audio_mic_cmd_t), NULL);
        audio_mic_thread = osThreadNew(audioMic_task, NULL, &attr);
    }
    return audio_mic_thread;
}
