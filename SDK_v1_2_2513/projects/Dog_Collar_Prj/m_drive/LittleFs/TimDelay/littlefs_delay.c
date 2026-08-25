#include "littlefs_delay.h"
#include "cmsis_os2.h"
#include <stdbool.h>
#include "om_log.h"


#define TIM_LOG_DEBUG(format, ...)               	om_log(OM_LOG_INFO, format,  ## __VA_ARGS__)


typedef struct {
    uint32_t flags;       // 返回的事件标志
    uint32_t remain_ms;   // 剩余时间（毫秒）
    int32_t  status;      // 返回状态（>=0 正常，<0 错误码）
} EventWaitResult;

#define TIME_DELAY_FLAG 	(1 << 1)	//littlefs初始化完成标志

osEventFlagsId_t m_tim_event_id;
/**
 * @brief 可中途取消的事件等待
 * @param event_id    事件对象 ID
 * @param flags       要等待的事件标志
 * @param options     等待选项（osFlagsWaitAny/osFlagsWaitAll 等）
 * @param timeout_ms  最大等待时间（毫秒）
 * @param cancel_cb   取消回调函数，返回 true 表示要取消等待
 * @param check_interval_ms 每次检查的间隔（毫秒）
 */
EventWaitResult EventFlagsWaitCancellable(osEventFlagsId_t event_id,
                                          uint32_t flags,
                                          uint32_t options,
                                          uint32_t timeout_ms,
                                          bool (*cancel_cb)(void),
                                          uint32_t check_interval_ms)
{
    EventWaitResult result = {0};
    uint32_t tick_start = osKernelGetTickCount();
    uint32_t tick_freq = osKernelGetTickFreq();

    uint32_t elapsed_ms = 0;
    while (elapsed_ms < timeout_ms) {
        if (cancel_cb && cancel_cb()) {
            // 被取消
            result.status = osFlagsErrorResource; // 自定义：表示被取消
            result.flags = 0;
            result.remain_ms = (elapsed_ms >= timeout_ms) ? 0 : (timeout_ms - elapsed_ms);
            return result;
        }

        // 剩余时间
        uint32_t remain = timeout_ms - elapsed_ms;
        if (remain > check_interval_ms) remain = check_interval_ms;

        int32_t ret = osEventFlagsWait(event_id, flags, options, remain);
        if (ret >= 0) {
            // 事件触发
            uint32_t tick_end = osKernelGetTickCount();
            uint32_t elapsed_ticks = tick_end - tick_start;
            elapsed_ms = (elapsed_ticks * 1000U) / tick_freq;
            result.status = ret;
            result.flags = (uint32_t)ret;
            result.remain_ms = (elapsed_ms >= timeout_ms) ? 0 : (timeout_ms - elapsed_ms);
            return result;
        } else if (ret == osFlagsErrorTimeout) {
            // 本轮没触发，继续等
        } else {
            // 其他错误
            result.status = ret;
            result.flags = 0;
            result.remain_ms = 0;
            return result;
        }

        // 更新时间
        uint32_t tick_now = osKernelGetTickCount();
        elapsed_ms = ((tick_now - tick_start) * 1000U) / tick_freq;
    }

    // 超时
    result.status = osFlagsErrorTimeout;
    result.flags = 0;
    result.remain_ms = 0;
    return result;
}
volatile bool urgent_task_flag=false;
bool cancel_check(void) {
    // 这里可以检查某个标志位 / 全局变量 / 按键 / 中断信号
   // extern 
    return urgent_task_flag;
}
void TimerCallback(void *argument)
{
	
}
void my_task(void *argument) {
	
	if(m_tim_event_id!=NULL)
	{
		EventWaitResult r = EventFlagsWaitCancellable(m_tim_event_id,
													  TIME_DELAY_FLAG,
													  osFlagsWaitAny,
													  5000,         // 最多等 1 秒
													  cancel_check, // 检查取消
													  10);          // 每 10ms 检查一次

		if (r.status >= 0) {
			TIM_LOG_DEBUG("evt single: 0x%08lx remainder %lu ms\r\n", r.flags, r.remain_ms);
			
			
		} else if (r.status == osFlagsErrorTimeout) {
			osStatus_t status = osEventFlagsDelete(m_tim_event_id);
			if (status != osOK) {
				
				// 删除失败，处理错误
			}
			TIM_LOG_DEBUG("timeout\r\n");
			
			
			
			
		} else if (r.status == osFlagsErrorResource) {
			TIM_LOG_DEBUG("wait	cancel %lu ms\r\n", r.remain_ms);
			osTimerId_t timer_id = osTimerNew(TimerCallback, osTimerOnce, argument, NULL);
			urgent_task_flag=false;
		} else {
			TIM_LOG_DEBUG("error: %ld\r\n", r.status);
		}
	}
}
static void vTimeScheduleTask(void *argument)
{
	
	while(1)
	{
		my_task(argument);
	}
}

void vTimeStartRtcTask(void)
{
    const osThreadAttr_t timThreadAttr = {
        .name = NULL,
        .attr_bits = 0,
        .cb_mem = NULL,
        .cb_size = 0,
        .stack_mem = NULL,
        .stack_size = 2048,
        .priority = osPriorityNone,
        .tz_module = 0,
    };
m_tim_event_id = osEventFlagsNew(NULL);
	
	osSemaphoreId_t semaphore = osSemaphoreNew(1, 1, NULL);
if (semaphore == NULL) {
    // 创建失败，处理错误
}
    // Create ble Task
    osThreadNew(vTimeScheduleTask, semaphore, &timThreadAttr);
}