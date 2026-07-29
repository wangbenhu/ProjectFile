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
#define AUDIO_TASK_STACK_SIZE (8*1024)

/*********************************************************************
 * TYPEDEFS
 */


/*********************************************************************
 * CONSTANTS
 */


/*********************************************************************
 * LOCAL VARIABLES
 */
static osSemaphoreId_t g_AudioSemaphore = NULL;


/*********************************************************************
 * GLOBAL VARIABLES
 */


/*********************************************************************
 * EXTERN FUNCTIONS
 */

extern void PWM_Audio_Play_Init(void);
/*********************************************************************
 * LOCAL FUNCTIONS
 */
void Audio_Play_DMA_Set(void)
{
	osSemaphoreRelease(g_AudioSemaphore);
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

    Message_t received_msg;
    
//    log_debug("vAudioTask %d started\n", my_task_info->task_id);
	
    PWM_Audio_Play_Init(); // PWM播放的状态
	
	if (g_AudioSemaphore == NULL) {
		g_AudioSemaphore = osSemaphoreNew(1, 0, NULL);
    } else {
        log_debug("vAudioTask already exists, skip creation\r\n");
    }

    for(;;) 
    {
		        // 等待数据信号量（由文件读取任务触发）
        if (osSemaphoreAcquire(g_AudioSemaphore, osWaitForever) == osOK) {
			audio_evt_callback();
			// osDelayUntil(&tick, PERIOD_TICKS);
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
