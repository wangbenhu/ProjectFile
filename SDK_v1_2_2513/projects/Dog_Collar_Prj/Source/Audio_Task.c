/**
 * @file  examples/ble_app_simple_server/src/main.c
 * @brief  simple server
 * @date Wed, Sep  5, 2018  5:19:05 PM
 * @author liqiang
 *
 * @addtogroup APP_SIMPLE_SERVER_MAIN main.c
 * @ingroup APP_SIMPLE_SERVER
 * @details simple server
 *
 * @{
 */

/*********************************************************************
 * INCLUDES
 */
#include "om_driver.h"
#if (CONFIG_SHELL)
#include "shell.h"
#endif
#include "evt.h"
#include "pm.h"
#include "bsp.h"
#include "omble.h"
#include "om_log.h"

#include "common_def.h"

// Controller header
#include "obc.h"

/* Kernel includes. */
#include "cmsis_os2.h"

#include "timers.h"
#include "my_audio.h"

/*********************************************************************
 * MACROS
 */
#define EVENT_SYSTEM_RESERVE_MASK   0x00FF

#define AUDIO_TASK_PRIORITY (osPriorityNormal1)
#define AUDIO_TASK_STACK_SIZE (4*1024)

/*********************************************************************
 * TYPEDEFS
 */


/*********************************************************************
 * CONSTANTS
 */


/*********************************************************************
 * LOCAL VARIABLES
 */
typedef enum {
    AUDIO_EVENT_PLAY_FILE,
    AUDIO_EVENT_PLAY_IFLASH,
    AUDIO_EVENT_STOP,
    AUDIO_EVENT_DMA_READY,
    AUDIO_EVENT_PLAYBACK_COMPLETE,
} audio_event_type_t;

typedef struct {
    audio_event_type_t type;
    uint8_t audio_reset;
    uint8_t iflash_index;
    char filename[128];
} audio_event_t;

#define AUDIO_EVENT_QUEUE_DEPTH  (8U)

static osMessageQueueId_t g_audio_event_queue = NULL;
static volatile bool g_audio_dma_event_pending = false;
static volatile bool g_audio_complete_event_pending = false;


/*********************************************************************
 * GLOBAL VARIABLES
 */


/*********************************************************************
 * EXTERN FUNCTIONS
 */
extern void StopPlayFlagSet(void);
extern void PWM_Audio_Play_Init(void);
/*********************************************************************
 * LOCAL FUNCTIONS
 */
void Audio_Play_DMA_Set(void)
{
	const audio_event_t event = {.type = AUDIO_EVENT_DMA_READY};

	if ((g_audio_event_queue != NULL) && !g_audio_dma_event_pending) {
		g_audio_dma_event_pending = true;
		if (osMessageQueuePut(g_audio_event_queue, &event, 0U, 0U) != osOK) {
			g_audio_dma_event_pending = false;
		}
	}
}

void Audio_Play_Complete_Notify(void)
{
	const audio_event_t event = {.type = AUDIO_EVENT_PLAYBACK_COMPLETE};

	if ((g_audio_event_queue != NULL) && !g_audio_complete_event_pending) {
		g_audio_complete_event_pending = true;
		if (osMessageQueuePut(g_audio_event_queue, &event, 0U, 0U) != osOK) {
			g_audio_complete_event_pending = false;
		}
	}
}

uint8_t Audio_Play_Request(const char *filename, uint8_t audio_reset)
{
	audio_event_t event = {.type = AUDIO_EVENT_PLAY_FILE, .audio_reset = audio_reset};
	size_t filename_len;

	if ((filename == NULL) || (g_audio_event_queue == NULL)) {
		return REPORT_AUDIO_ERROR;
	}
	filename_len = strlen(filename);
	if ((filename_len == 0U) || (filename_len >= sizeof(event.filename))) {
		return REPORT_AUDIO_ERROR;
	}
	memcpy(event.filename, filename, filename_len + 1U);
	return (osMessageQueuePut(g_audio_event_queue, &event, 0U, 0U) == osOK) ? REPORT_OK : REPORT_AUDIO_ERROR;
}

uint8_t Audio_IFlash_Play_Request(uint8_t index, uint8_t audio_reset)
{
	const audio_event_t event = {
		.type = AUDIO_EVENT_PLAY_IFLASH,
		.audio_reset = audio_reset,
		.iflash_index = index,
	};

	if ((index == 0U) || (index > 5U) || (g_audio_event_queue == NULL)) {
		return REPORT_AUDIO_ERROR;
	}
	return (osMessageQueuePut(g_audio_event_queue, &event, 0U, 0U) == osOK) ? REPORT_OK : REPORT_AUDIO_ERROR;
}

uint8_t Audio_Play_Stop_Request(void)
{
    const audio_event_t event = {.type = AUDIO_EVENT_STOP};

    if (g_audio_event_queue == NULL) {
        return REPORT_AUDIO_ERROR;
    }
    return (osMessageQueuePut(g_audio_event_queue, &event, 0U, 0U) == osOK) ? REPORT_OK : REPORT_AUDIO_ERROR;
}
/**
 * @brief  schedule task
 *
 * @param[in] pvParameters  pv parameters
 **/

static void vAudioTask(void *argument)
{
    //TaskInfo_t *my_info = (TaskInfo_t *)pvParameters;
    TaskInfo_t *my_task_info = GetTaskInfo(AUDIO_TASK_ID);
    TaskInfo_t *entry_task_info = GetTaskInfo(ENTRY_TASK_ID);

    audio_event_t event;
	
    if (g_audio_event_queue == NULL) {
        g_audio_event_queue = osMessageQueueNew(AUDIO_EVENT_QUEUE_DEPTH, sizeof(audio_event_t), NULL);
    }
    if (g_audio_event_queue == NULL) {
        log_debug("Audio event queue create failed\r\n");
        return;
    }

    PWM_Audio_Play_Init(); // PWM播放的状态

    for(;;) 
    {
        if (osMessageQueueGet(g_audio_event_queue, &event, NULL, osWaitForever) != osOK) {
            continue;
        }

        switch (event.type) {
        case AUDIO_EVENT_PLAY_FILE:
			(void)Audio_Play_Start(event.filename, event.audio_reset);
            break;
        case AUDIO_EVENT_PLAY_IFLASH:
			(void)Audio_IFlash_Play_Start(event.iflash_index, event.audio_reset);
            break;
        case AUDIO_EVENT_STOP:
            StopPlayFlagSet();
            PWM_Audio_Stop();
            Audio_Play_SD_Disable();
            break;
        case AUDIO_EVENT_DMA_READY:
            g_audio_dma_event_pending = false;
            audio_evt_callback();
            break;
        case AUDIO_EVENT_PLAYBACK_COMPLETE:
            g_audio_complete_event_pending = false;
			PWM_Audio_Stop();
			Audio_Play_SD_Disable();
            break;
        default:
            break;
        }
    }
}

/*********************************************************************
 * CONST VARIABLES
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/**
 * @brief  v start bluetooth task
 **/
osThreadId_t vStartAudioTask(void)
{
    const osThreadAttr_t AudioThreadAttr = {
        .name = "Audio_Task",
        .attr_bits = 0,
        .cb_mem = NULL,
        .cb_size = 0,
        .stack_mem = NULL,
        .stack_size = AUDIO_TASK_STACK_SIZE,
        .priority = AUDIO_TASK_PRIORITY,
        .tz_module = 0,
    };

    // Create pm Task
    return osThreadNew(vAudioTask, NULL, &AudioThreadAttr);
}

/** @} */

// vim: fdm=marker
