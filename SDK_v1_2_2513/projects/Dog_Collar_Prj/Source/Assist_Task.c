/**
 * @file  stack_monitor_Task.c
 * @brief  FreeRTOS 任务堆栈监控 —— 每10秒打印一次各任务剩余、高水位和使用占比
 *
 * ======================== 设计说明 ========================
 *
 * 问题根因（为什么之前全部显示 ? 或 0%）：
 *   1. osThreadGetStackSpace() 在本平台的实现是：
 *         sz = uxTaskGetStackHighWaterMark(hTask) * sizeof(StackType_t)
 *      即它返回的是"历史最小剩余字节"（高水位），不是实时剩余，
 *      与 TaskStatus_t.usStackHighWaterMark × 4 完全相同。
 *      因此 Remain == HWM，做差永远是0，Peak% 永远是0。
 *
 *   2. FreeRTOS 的 TaskStatus_t 中没有"栈总大小"字段，
 *      pxStackBase 只是栈底地址，不能直接推算 total_size。
 *
 * 正确方案：
 *   用任务名→栈大小映射表，将各任务在创建时声明的 stack_size
 *   硬编码到监控模块中，配合 uxTaskGetStackHighWaterMark 计算：
 *
 *     HWM_bytes  = usStackHighWaterMark × sizeof(StackType_t)
 *                = 历史最小剩余字节（越小越危险）
 *     Peak_used  = total_size - HWM_bytes
 *     Peak%      = (Peak_used × 100) / total_size
 *
 * 输出列说明：
 *   Total(B) : 任务创建时分配的栈总字节数
 *   HWM(B)   : 历史最小剩余字节（High Water Mark，越小越危险）
 *   Used(B)  : 峰值使用字节 = Total - HWM
 *   Peak%    : 峰值使用百分比
 *
 * ==========================================================
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
/*********************************************************************
 * MACROS
 */
#if(1)
#define EVENT_SYSTEM_RESERVE_MASK   0x00FF

#define ASSIST_TASK_PRIORITY (osPriorityNormal)
#define ASSIST_TASK_STACK_SIZE (2048)

/*********************************************************************
 * TYPEDEFS
 */

#define WDT_TIMEOUT_MS (10 * 1000)  //10sec

#define MAX_TASK_NUM   20   // 系统允许的最大任务数目


#define ASSIST_CYCLE_TIME (10*60*1000)//unit ms/10min

/*********************************************************************
 * CONSTANTS
 */


/*********************************************************************
 * LOCAL VARIABLES
 */
static osSemaphoreId_t xSemAssist = NULL;

// 混合喂狗策略
typedef struct {
    const char *task_name;
    uint32_t last_feed_time;
    bool is_alive;
    uint32_t required_interval;
} task_watchdog_info_t;

static task_watchdog_info_t task_watchdogs[] = {
    {"ENTRY", 0, true, 100},
    {"PM", 0, true, 500},
};
/*********************************************************************
 * GLOBAL VARIABLES
 */


/*********************************************************************
 * EXTERN FUNCTIONS
 */

//extern void get_task_state(void);
/*********************************************************************
 * LOCAL FUNCTIONS
 */
/* 获取 OS 任务信息 */
void get_assist_task_state(void) {
    osThreadId_t thread_id[MAX_TASK_NUM];
    uint32_t count, i;

    // 获取任务总数
    count = osThreadEnumerate(thread_id, MAX_TASK_NUM);
    log_debug("+++++++++++++++++++++++++++++++++++++++++++++\r\n");
	log_debug("TASK NUM: %lu\r\n", (unsigned long)count);
	size_t freeHeapBefore = xPortGetFreeHeapSize();
	log_debug("CMSIS-RTOS_V2 ALL HEAP : %lu\r\n", freeHeapBefore);
    log_debug("TASK NAME  STATUS  PRIORITY REMAINING STACK\r\n");
    log_debug("------------------------------------------\r\n");

    for (i = 0; i < count; i++) {
        const char *name = osThreadGetName(thread_id[i]);
        osThreadState_t state = osThreadGetState(thread_id[i]);
        osPriority_t prio = osThreadGetPriority(thread_id[i]);
        uint32_t stack_space = osThreadGetStackSpace(thread_id[i]);

        char state_ch;
        switch (state) {
            case osThreadInactive: state_ch = 'I'; break;  // 未激活
            case osThreadReady:    state_ch = 'R'; break;  // 就绪
            case osThreadRunning:  state_ch = 'r'; break;  // 运行
            case osThreadBlocked:  state_ch = 'B'; break;  // 阻塞
            case osThreadTerminated: state_ch = 'D'; break; // 删除
            default: state_ch = '?'; break;
        }

        log_debug("%-12s %-6c %-8d %-8lu\r\n",
            name ? name : "NULL",
            state_ch,
            (int)prio,
            (unsigned long)stack_space);
    }

    log_debug("TASK STATUS INFO: r-RUN R-READY B-BLOCK I-INACTIVE D-DELETE\r\n");
	log_debug("------------------------------------------\r\n");
}



// 任务注册自己的喂狗点
void register_task_feed(const char *task_name)
{
    for (int i = 0; i < sizeof(task_watchdogs)/sizeof(task_watchdogs[0]); i++) {
        if (strcmp(task_watchdogs[i].task_name, task_name) == 0) {
            task_watchdogs[i].last_feed_time = osKernelGetTickCount();
            task_watchdogs[i].is_alive = true;
            break;
        }
    }
}
void handle_task_failure(void)
{
	log_debug("ERROR TASK \r\n");
}
/**
 * @brief  schedule task
 *
 * @param[in] pvParameters  pv parameters
 **/
static void vAssistTask(void *argument)
{

    TaskInfo_t *my_task_info = GetTaskInfo(ASSIST_TASK_ID);
    TaskInfo_t *entry_task_info = GetTaskInfo(ENTRY_TASK_ID);
    Message_t received_msg;
//	if (xSemAssist == NULL) {
//		xSemAssist = osSemaphoreNew(1, 0, NULL);
//    } else {
//        log_debug("Semaphore already exists, skip creation\n");
//    }
    
//    log_debug("Task %d started\n", my_task_info->task_id);
    
    for(;;) 
    {
		          //获取当前初始化的全部task信息
		get_assist_task_state();
		osDelay(osMS2TicksRound(ASSIST_CYCLE_TIME));
//        bool all_tasks_healthy = true;
//        uint32_t current_time = osKernelGetTickCount();
//        
//        for (int i = 0; i < sizeof(task_watchdogs)/sizeof(task_watchdogs[0]); i++) {
//            if (current_time - task_watchdogs[i].last_feed_time > task_watchdogs[i].required_interval * 2) {
//                log_debug("任务 %s 可能卡死\n", task_watchdogs[i].task_name);
//                task_watchdogs[i].is_alive = false;
//                all_tasks_healthy = false;
//            }
//        }
//        
//        if (all_tasks_healthy) {
//            drv_wdt_keep_alive();//喂狗
//            // log_debug("系统健康，喂狗成功\n");
//        } else {
//            // 有任务异常，可以选择：
//            // 1. 不喂狗让系统重启
//            // 2. 尝试恢复异常任务
//            // 3. 进入安全模式
//            handle_task_failure();
//        }
//		
//		osSemaphoreRelease(xSemAssist);
//          //获取当前初始化的全部task信息
//		get_task_state();
//        
//        // 接收消息
//        if(osOK == osMessageQueueGet(my_task_info->queue_handle,&received_msg, NULL, 1000))
//        {
//            log_debug("Task %d received from %d: %lu\n", 
//                  my_task_info->task_id, received_msg.source_id, received_msg.command);
//            // 回复消息给Entry任务
//            if((received_msg.source_id == ENTRY_TASK_ID) && (received_msg.command == TASK_CMD_START))
//			{
//				//初始化看门狗
//				drv_wdt_init(WDT_TIMEOUT_MS);
//            }
//			else if((received_msg.source_id == ENTRY_TASK_ID) && (received_msg.command == TASK_CMD_STOP))
//			{
//						//关闭看门狗
//				drv_wdt_init(0);
//				 osSemaphoreAcquire(xSemAssist, osWaitForever);
//			}
//            else
//            {
//                //handle other logic
//            }
//        }
//		 osSemaphoreAcquire(xSemAssist, osWaitForever);
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
osThreadId_t vStartAssistTask(void)
{
    const osThreadAttr_t AssistThreadAttr = {
        .name = "Assist_Task",
        .attr_bits = 0,
        .cb_mem = NULL,
        .cb_size = 0,
        .stack_mem = NULL,
        .stack_size = ASSIST_TASK_STACK_SIZE,
        .priority = ASSIST_TASK_PRIORITY,
        .tz_module = 0,
    };


    return osThreadNew(vAssistTask, NULL, &AssistThreadAttr);
}
#endif
/** @} */

// vim: fdm=marker
