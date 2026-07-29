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
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

/*********************************************************************
 * MACROS
 */
#if(1)

#define STACK_MONITOR_TASK_PRIORITY    (osPriorityNormal)
#define STACK_MONITOR_TASK_STACK_SIZE  (2048)   /* 原来1024太小，91%危险，改为2048 */

#define MAX_TASK_NUM                   24       /* 系统允许的最大任务数 */
#define STACK_MONITOR_INTERVAL_MS      10 *60 * 1000    /* 打印间隔：10min */

/* ARM Cortex-M: StackType_t = uint32_t，4字节 */
#define STACK_WORD_TO_BYTES(w)   ((uint32_t)(w) * (uint32_t)sizeof(StackType_t))
/*********************************************************************
 * 任务名 → 栈大小映射表
 *
 * 规则：与各任务文件中 osThreadAttr_t.stack_size 保持一致。
 * 新增/修改任务时同步更新此表，否则该任务将显示 total=0。
 * 未在表中的任务显示 Total=?, Peak%=? 但仍显示 HWM 供参考。
 *********************************************************************/
typedef struct {
    const char *name;
    uint32_t    stack_bytes;
} TaskStackEntry_t;

/* CONFIG_SHELL_STACK_SIZE = 4096（来自 autoconf.h） */
static const TaskStackEntry_t g_task_stack_table[] = {
    { "Stack_Monitor",            2048          },
    { "GNSS_UART_Task",           9216          },
    { "GNSS_UartTTT_Da",          4096          },  /* GNSS_UartTTT_DataRecv_Task 截断为16字符 */
    { "CAT1_UART_Task",           5120          },
    { "Uart_DataRecv_T",          12288         },  /* Uart_DataRecv_Task 截断 */
    { "Entry_Task",               10240         },
    { "Comm_Task",                8192          },
    { "Audio_Task",               8192          },
    { "Sensor_Task",              2048          },
    { "PM_Task",                  1024          },
    { "MotorTask",                1024          },
    { "BLE_Task",                 4096          },
    { "Led_Task",                 2048          },
    { "shell",                    4096          },
    { "IDLE",                     0             },  /* IDLE 任务由内核管理，不计算 */
    { "Tmr Svc",                  0             },  /* Timer Service 由内核管理 */
};
#define STACK_TABLE_SIZE  (sizeof(g_task_stack_table) / sizeof(g_task_stack_table[0]))

/*********************************************************************
 * LOCAL VARIABLES
 *********************************************************************/
/* 注：堆历史最小值由 heap_4.c 内部的 xMinimumEverFreeBytesRemaining 自动维护，
 * 通过 xPortGetMinimumEverFreeHeapSize() 读取，无需在此重复追踪。 */

/*********************************************************************
 * LOCAL FUNCTIONS
 *********************************************************************/

/**
 * @brief 在映射表中查找任务名对应的栈大小
 * @param  name   任务名（TaskStatus_t.pcTaskName，最长16字符含\0）
 * @return 栈字节数，0表示未在表中或无需统计
 */
static uint32_t prvLookupStackSize(const char *name)
{
    uint32_t i;
    if (name == NULL) return 0;
    for (i = 0; i < STACK_TABLE_SIZE; i++) {
        if (strncmp(g_task_stack_table[i].name, name, configMAX_TASK_NAME_LEN) == 0) {
            return g_task_stack_table[i].stack_bytes;
        }
    }
    return 0;  /* 未找到 */
}

/**
 * @brief 将 FreeRTOS eTaskState 转成单字符
 *   r = Running(运行中)  R = Ready(就绪)  B = Blocked(阻塞)
 *   S = Suspended(挂起)  D = Deleted(已删除)
 */
static char prvStateChar(eTaskState s)
{
    switch (s) {
        case eRunning:   return 'r';
        case eReady:     return 'R';
        case eBlocked:   return 'B';
        case eSuspended: return 'S';
        case eDeleted:   return 'D';
        default:         return '?';
    }
}

/**
 * @brief  打印堆栈监控报告（每 STACK_MONITOR_INTERVAL_MS 调用一次）
 *
 * 核心数据来源：uxTaskGetSystemState()
 *   TaskStatus_t.usStackHighWaterMark = 历史最小剩余（单位：word = 4字节）
 *   → HWM_bytes = usStackHighWaterMark × 4
 *   → PeakUsed  = TotalStack - HWM_bytes
 *   → Peak%     = PeakUsed × 100 / TotalStack
 *
 * 堆统计：
 *   xPortGetFreeHeapSize()           = 当前剩余字节
 *   xPortGetMinimumEverFreeHeapSize() = 历史最小剩余（内核自动追踪）
 *   configTOTAL_HEAP_SIZE            = 堆总大小（FreeRTOSConfig.h）
 *   HeapPeak% = (Total - HistMin) × 100 / Total
 */
static void PrintStackReport(void)
{
    static TaskStatus_t s_task_buf[MAX_TASK_NUM];
    UBaseType_t task_count, i;
    uint32_t total_run_time;

    /* -------------------------------------------------------
     * 堆统计
     * ------------------------------------------------------- */
    uint32_t heap_total    = (uint32_t)configTOTAL_HEAP_SIZE;
    uint32_t heap_free_now = (uint32_t)xPortGetFreeHeapSize();
    uint32_t heap_free_min = (uint32_t)xPortGetMinimumEverFreeHeapSize(); /* 历史最小剩余 */
    uint32_t heap_used_now = heap_total - heap_free_now;
    uint32_t heap_peak_used= heap_total - heap_free_min;                  /* 历史峰值使用 */
    uint32_t heap_used_pct = (heap_used_now  * 100u) / heap_total;
    uint32_t heap_peak_pct = (heap_peak_used * 100u) / heap_total;

    /* 告警标记 */
    const char *heap_warn = "";
    if      (heap_peak_pct >= 90) heap_warn = " !!CRIT!!";
    else if (heap_peak_pct >= 80) heap_warn = " !WARN!";
    else if (heap_peak_pct >= 70) heap_warn = " >70%";

    /* 一次性获取所有任务快照（需要 configUSE_TRACE_FACILITY=1） */
    task_count = uxTaskGetSystemState(s_task_buf, MAX_TASK_NUM, &total_run_time);

    log_debug("\r\n========== STACK MONITOR ==========\r\n");
    log_debug("Tasks: %u\r\n", (unsigned)task_count);
    log_debug("------- HEAP -----------------------------------------------\r\n");
    log_debug("Total : %6u B\r\n",  (unsigned)heap_total);
    log_debug("Now   : %6u B free  (%u B used,  %u%%)\r\n",
              (unsigned)heap_free_now, (unsigned)heap_used_now, (unsigned)heap_used_pct);
    log_debug("Peak  : %6u B free  (%u B used,  %u%%)%s  <-- HWM\r\n",
              (unsigned)heap_free_min, (unsigned)heap_peak_used, (unsigned)heap_peak_pct,
              heap_warn);
    log_debug("---------------------------------------------------------------\r\n");
    log_debug("%-16s St Pr  Total(B)  HWM(B)  Used(B)  Peak%%\r\n", "Task");
    log_debug("%-16s -- --  --------  ------  -------  -----\r\n", "----");

    for (i = 0; i < task_count; i++) {
        TaskStatus_t *t = &s_task_buf[i];

        /* HWM_bytes：历史最小剩余字节（即高水位对应的"还剩多少"） */
        uint32_t hwm_bytes   = STACK_WORD_TO_BYTES(t->usStackHighWaterMark);

        /* 从映射表查总栈大小 */
        uint32_t total_bytes = prvLookupStackSize(t->pcTaskName);

        char state_ch = prvStateChar(t->eCurrentState);

        if (total_bytes == 0) {
            /* 内核任务(IDLE/Tmr Svc)或未在表中的任务：只显示 HWM，不计算百分比 */
            log_debug("%-16s %c  %2u  %8s  %6u  %7s  %5s\r\n",
                      t->pcTaskName,
                      state_ch,
                      (unsigned)t->uxCurrentPriority,
                      "N/A",
                      (unsigned)hwm_bytes,
                      "N/A",
                      "N/A");
            continue;
        }

        /* 峰值使用字节 */
        uint32_t peak_used = (total_bytes > hwm_bytes) ? (total_bytes - hwm_bytes) : 0;
        /* 峰值使用百分比 */
        uint32_t peak_pct  = (peak_used * 100u) / total_bytes;

        /* 告警标记 */
        const char *warn = "";
        if      (peak_pct >= 90) warn = " !!CRIT!!";
        else if (peak_pct >= 80) warn = " !WARN!";
        else if (peak_pct >= 70) warn = " >70%";

        log_debug("%-16s %c  %2u  %8u  %6u  %7u  %4u%%%s\r\n",
                  t->pcTaskName,
                  state_ch,
                  (unsigned)t->uxCurrentPriority,
                  (unsigned)total_bytes,   /* 总栈大小 */
                  (unsigned)hwm_bytes,     /* 高水位：历史最小剩余（字节） */
                  (unsigned)peak_used,     /* 峰值已使用（字节） */
                  (unsigned)peak_pct,      /* 峰值占比 */
                  warn);
    }

    log_debug("---------------------------------------------------------------\r\n");
	log_debug("system_reboot_reason = %d\r\n",m_system_get_reboot_reason());
    log_debug("HWM=Historical minimum remaining  Used=TotalStack-HWM\r\n");
    log_debug("===============================================================\r\n");
}

/**
 * @brief  堆栈监控主任务
 */
static void vStackMonitorTask(void *argument)
{
    (void)argument;

    /* 上电后稍等系统稳定 */
    osDelay(3000);

    for (;;)
    {
        PrintStackReport();
        osDelay(STACK_MONITOR_INTERVAL_MS);
    }
}

/**
 * @brief  RTOS 空闲钩子 —— 低堆告警
 *         堆高水位由 heap_4.c 内部 xMinimumEverFreeBytesRemaining 自动维护，
 *         此处只做临界告警（< 1KB）。
 */
void vApplicationIdleHook(void)
{
    static uint32_t idle_counter = 0u;

    idle_counter++;
    if (idle_counter >= 1000u) {
        idle_counter = 0u;
        if (xPortGetFreeHeapSize() < 1024u) {
            log_debug("[STACK][WAR] %u bytes !!\r\n",
                      (unsigned)xPortGetFreeHeapSize());
        }
    }
}

/**
 * @brief  启动堆栈监控任务
 * @return 任务句柄，NULL 表示创建失败
 */
osThreadId_t vStartStackMonitorTask(void)
{
    const osThreadAttr_t attr = {
        .name       = "Stack_Monitor",
        .attr_bits  = 0,
        .cb_mem     = NULL,
        .cb_size    = 0,
        .stack_mem  = NULL,
        .stack_size = STACK_MONITOR_TASK_STACK_SIZE,
        .priority   = STACK_MONITOR_TASK_PRIORITY,
        .tz_module  = 0,
    };

    return osThreadNew(vStackMonitorTask, NULL, &attr);
}

#endif  /* #if(1) */
/** @} */
