#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include "om_log.h"
#include "common_def.h"

TaskInfo_t task_info[NUM_TASKS] = {0};  // 存储所有任务信息
 //QueueHandle_t task_queue[NUM_TASKS] = {0}; 	   //任务的消息队列
TaskInfo_t* GetTaskInfo(TASK_ID_T task_id)
{

    if(task_id >= END_TASK_ID)
       LOG_LOC();
    return &task_info[task_id];
}

osMessageQueueId_t* GetTaskQueue(TASK_ID_T task_id)
{

    if(task_id >= END_TASK_ID)
    	LOG_LOC();

    return &task_info[task_id].queue_handle;
}

/**
 * @brief strnstr	封装的字符串比较函数
 * @param haystack  比较字符串1
 * @param needle    比较字符串2（源字符串）
  * @param len      比较的字符串长度
 * @return char     1-成功，0-失败
 */
char *strnstr(const char *haystack, const char *needle, size_t len) {
    if (!haystack || !needle || len == 0) return NULL;
    
    size_t needle_len = strlen(needle);
    if (needle_len == 0) return (char *)haystack;
    
    for (size_t i = 0; i < len && haystack[i]; i++) {
        if (i + needle_len > len) break;  // 鍓╀綑闀垮害涓嶈冻
        if (strncmp(&haystack[i], needle, needle_len) == 0) {
            return (char *)&haystack[i];
        }
    }
    return NULL;
}
__attribute__((unused))
void err_lock(const char *func, int line)
{
	while(1)
	{
		log_debug("ERROR LOCATION : [%s:%d]\r\n", func, line);  
		DRV_DELAY_MS(500);
	};
}
