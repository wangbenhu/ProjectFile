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
#include "bsp.h"
#include "omble.h"
#include "om_log.h"

#include "common_def.h"

// Controller header
#include "obc.h"

/* Kernel includes. */
#include "cmsis_os2.h"
#include "semphr.h"
#include "cJSON.h"
#include "timers.h"
/*********************************************************************
 * MACROS
 */
#define EVENT_SYSTEM_RESERVE_MASK   0x00FF

#define UART_DATARECV_TASK_PRIORITY (osPriorityNormal)
#define UART_DATARECV_TASK_STACK_SIZE (8192)

#define UART_RECV_DATA_SIZE (6*1024)

// 超时检测配置
#define UART_TIMEOUT_CHECK_INTERVAL  10    // 10ms检测一次
#define UART_TIMEOUT_THRESHOLD       5     // 4次检测无数据 = 40ms超时

// 定义事件标志
#define UART_EVENT_TASK_BLOCK      (1 << 0)      // 任务阻塞标志
#define UART_EVENT_TASK_UNBLOCK    (1 << 1)      // 任务解除阻塞标志

/*********************************************************************
 * TYPEDEFS
 */

typedef struct {
    om_fifo_t          fifo;
    uint8_t             fifo_buffer[UART_RECV_DATA_SIZE];
} shell_env_t;

shell_env_t lte_uartFifo_env;
//extern shell_env_t gps_uartFifo_env;

/*********************************************************************
 * CONSTANTS
 */


/*********************************************************************
 * LOCAL VARIABLES
 */
osSemaphoreId_t UartDataReadySem = NULL;
volatile bool uart_data_pending = false;  // 数据待处理标志
volatile uint8_t uart_timeout_counter = 0; // 超时计数器
volatile bool uart_data_processing = false; // 数据正在处理标志
volatile uint16_t uart_cut_len = 0;         // 50ms静默时刻冻结的整包边界

// 串口任务事件组
static osEventFlagsId_t UartEventId = NULL;

// 串口任务运行状态
static bool uart_task_running = true;

/*********************************************************************
 * GLOBAL VARIABLES
 */



/*********************************************************************
 * EXTERN FUNCTIONS
 */
extern bool is_power_checking;
extern osSemaphoreId_t Cat1PowerCheckSem;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
/**
 * @brief 安全阻塞串口任务
 * @return bool true-成功阻塞 false-阻塞失败（已在阻塞状态）
 */
bool safe_block_uart_task(void)
{
    uint32_t current_flags = osEventFlagsGet(UartEventId);

    if (current_flags & UART_EVENT_TASK_BLOCK) {
        log_debug("[C-UART][STA] task already blocked, skip block\r\n");
        return false;
    }
    
    osEventFlagsClear(UartEventId, UART_EVENT_TASK_UNBLOCK);
    osEventFlagsSet(UartEventId, UART_EVENT_TASK_BLOCK);
    uart_task_running = false;
    return true;
}

/**
 * @brief 安全解除阻塞串口任务
 * @return bool true-成功解除 false-解除失败
 */
bool safe_unblock_uart_task(void)
{
    uint32_t current_flags = osEventFlagsGet(UartEventId);
    
    // 如果不在阻塞状态，不需要解除
    if (!(current_flags & UART_EVENT_TASK_BLOCK)) {
        log_debug("[C-UART][STA] not blocked, skip unblock\r\n");
		
        return false;
    }
    
    osEventFlagsClear(UartEventId, UART_EVENT_TASK_BLOCK);
    osEventFlagsSet(UartEventId, UART_EVENT_TASK_UNBLOCK);
    uart_task_running = true;
    return true;
}

/**
 * @brief 检查任务是否应该阻塞
 * @return bool true-应该阻塞 false-不应该阻塞
 */
bool should_uart_task_block(void)
{
    return (osEventFlagsGet(UartEventId) & UART_EVENT_TASK_BLOCK) != 0;
}

/**
 * @brief 检查任务是否正在运行
 * @return bool true-运行中 false-阻塞中
 */
bool is_uart_task_running(void)
{
    return uart_task_running;
}

/**
 * @brief 等待任务解除阻塞（永久等待）
 * @return NULL
 */
void wait_for_uart_task_unblock(void)
{
    // 等待解除阻塞标志
    osEventFlagsWait(UartEventId, UART_EVENT_TASK_UNBLOCK, osFlagsWaitAny, osWaitForever);
    osEventFlagsClear(UartEventId, UART_EVENT_TASK_UNBLOCK);
    
    uart_task_running = true;
}

static void cat1_uart1_cb(void *om_uart, drv_event_t event, void *rxbuf, void *rx_cnt)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    if (event == DRV_EVENT_COMMON_READ_COMPLETED) {
        
        // 数据存入FIFO
        om_fifo_in(&lte_uartFifo_env.fifo, (uint8_t *)rxbuf, (uint32_t)rx_cnt);
       
        // 设置数据待处理标志，重置超时计数器
        uart_data_pending = true;
        uart_timeout_counter = 0; // 有数据到达，重置计数器
    }
    
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void cat1_uart1_init(void)
{
    uart_config_t uart_cfg = {
        .baudrate       = CAT1_AT_UART_BAUDRATE,
        .flow_control   = UART_FLOW_CONTROL_NONE,
        .data_bit       = UART_DATA_BIT_8,
        .stop_bit       = UART_STOP_BIT_1,
        .parity         = UART_PARITY_NONE,
    };

    // 创建信号量
	if (UartDataReadySem == NULL) {
		    UartDataReadySem = osSemaphoreNew(1, 0, NULL);
    } else {
    }

    
    drv_uart_init(CAT1_AT_UART, &uart_cfg);
    drv_uart_register_isr_callback(CAT1_AT_UART, cat1_uart1_cb);

    om_fifo_init(&lte_uartFifo_env.fifo, lte_uartFifo_env.fifo_buffer, sizeof(lte_uartFifo_env.fifo_buffer));
    drv_uart_read_int(CAT1_AT_UART, NULL, 0);
}

void cat1_uart1_deinit(void)
{
    // 停止UART接收
    drv_uart_read_int(CAT1_AT_UART, NULL, 0);
    
    // 清空FIFO
    om_fifo_reset(&lte_uartFifo_env.fifo);
    
    // 重置状态标志
    uart_data_pending = false;
    uart_data_processing = false;
    uart_timeout_counter = 0;
}

/**
 * @brief  schedule task
 *
 * @param[in] pvParameters  pv parameters
 **/
static void vUartDataRecvTask(void *argument)
{
    //TaskInfo_t *my_info = (TaskInfo_t *)pvParameters;
    TaskInfo_t *my_task_info = GetTaskInfo(UART_DATARECV_ID);
    TaskInfo_t *cat1_task_info = GetTaskInfo(CAT1_UART_TASK_ID);
    
    uint16_t lteFifoLength = 0;
    Message_t received_uart1_msg;

    log_debug("[C-UART][STA] Task %d started\r\n", my_task_info->task_id);

    for(;;) 
    {
        // 检查任务是否应该阻塞
        if (should_uart_task_block()) {
            log_debug("[C-UART][STA] task entering block\r\n");
            wait_for_uart_task_unblock();
        }
        
        // 处理任务间消息
        if(osOK == osMessageQueueGet(my_task_info->queue_handle, &received_uart1_msg, NULL, UART_TIMEOUT_CHECK_INTERVAL)) 
        {
            if(received_uart1_msg.source_id == CAT1_UART_TASK_ID)
            {  
                if(received_uart1_msg.command == TASK_CMD_START)
                {
                    
                    // 初始化UART（如果之前被关闭了）
                    cat1_uart1_init();
                    uart_timeout_counter = 0;
                    uart_data_pending = false;
                    uart_data_processing = false;
                    
                    // 如果处于阻塞状态，解除阻塞
                    if (!is_uart_task_running()) {
                        safe_unblock_uart_task();
                    }
                }
                else if(received_uart1_msg.command == TASK_CMD_STOP)
                {
                    
                    // 反初始化UART
                    cat1_uart1_deinit();
                    uart_data_pending = false;
                    uart_data_processing = false;
                    
                    // 设置阻塞标志
                    safe_block_uart_task();
                }
            }
        }
        
        // 超时检测逻辑 - 只在运行状态下检测
        if (is_uart_task_running() && uart_data_pending && !uart_data_processing) {
            // 有数据待处理，增加超时计数器
            uart_timeout_counter++;
            
            if (uart_timeout_counter >= UART_TIMEOUT_THRESHOLD) {
                uart_cut_len = om_fifo_len(&lte_uartFifo_env.fifo);
                log_debug("[C-UART][WTR] fifo watermark %u/4096\r\n", uart_cut_len); // 压测水位
                uart_data_pending = false;
                uart_data_processing = true;
                uart_timeout_counter = 0;
                osSemaphoreRelease(UartDataReadySem);
            }
        }
        
        // 等待数据处理信号 - 只在运行状态下处理数据
        if (is_uart_task_running() && (osSemaphoreAcquire(UartDataReadySem, 0) == osOK || uart_data_processing)) {
            uart_data_processing = true;
            
            // 数据包接收完成，循环处理FIFO中的所有数据
            while (uart_cut_len > 0 && !om_fifo_is_empty(&lte_uartFifo_env.fifo)) {
                uint16_t available_data = om_fifo_len(&lte_uartFifo_env.fifo);
                if (available_data > uart_cut_len) {
                    available_data = uart_cut_len;
                }
                if (available_data > 0) {
                    // 申请内存来存储FIFO数据
                    uint8_t *lteRecvSubpackage = DEMO_BT_Malloc(available_data + 1);
                    
                    if (lteRecvSubpackage != NULL) 
                    {
                        lteFifoLength = 0;
                        // 一次性取出FIFO中所有数据
                        uint16_t rx_len = 0;
                        uint16_t remaining = available_data;
                        while (!om_fifo_is_empty(&lte_uartFifo_env.fifo) && remaining > 0) {
                            rx_len = om_fifo_out(&lte_uartFifo_env.fifo, 
                                               lteRecvSubpackage + lteFifoLength, 
                                               remaining);
                            if (rx_len == 0) {
                                break; 
                            }
                            lteFifoLength += rx_len;
                            remaining -= rx_len;
                        }
                        
                        uart_cut_len -= lteFifoLength;

                        // 处理完整数据包
                        if (lteFifoLength > 0) {
                            lteRecvSubpackage[lteFifoLength] = '\0';
							if(DEBUG_LOG_ON)
							{
								drv_uart_write(LOG_UART, (uint8_t *)"[C-UART][RCV]", (uint32_t)13, 10);
								drv_uart_write(LOG_UART, (uint8_t *)lteRecvSubpackage, (uint32_t)lteFifoLength, 10);
								log_debug("\r\n");
							}
                            // 检查是否正在电源检测且收到"OK"
                            if (is_power_checking && (strstr((char *)lteRecvSubpackage, "OK") != NULL ||
                                                      strstr((char *)lteRecvSubpackage, "ERROR") != NULL)) {
                                log_debug("[C-UART][STA] Power check response (OK/ERROR)\r\n");
                                osSemaphoreRelease(Cat1PowerCheckSem);
                                DEMO_BT_Free(lteRecvSubpackage);
                                continue;
                            }
                            
                            // 发送数据给CAT1任务处理
                            Message_t msg = {
                                .source_id = my_task_info->task_id,
                                .dest_id = CAT1_UART_TASK_ID,
                                .data = lteRecvSubpackage,
                                .data_length = lteFifoLength
                            };
                                
                            if (osMessageQueuePut(cat1_task_info->queue_handle, &msg, NULL, 0) != osOK) {
                                DEMO_BT_Free(lteRecvSubpackage);
                            }
                        } else {
                            DEMO_BT_Free(lteRecvSubpackage);
                        }
                    }
                }
                
                // 短暂延时，让其他任务有机会运行
                osDelay(1);
            }
            
            // 数据处理完成
            uart_data_processing = false;
        }
    }
}


/**
 * @brief 初始化串口任务RTOS组件
 **/
void uart_rtos_init(void)
{
    // 创建事件组
    UartEventId = osEventFlagsNew(NULL);
    if (UartEventId == NULL) {
		LOG_LOC();
    } else {
    }
    
    // 默认状态为运行（不阻塞）
    uart_task_running = true;
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
osThreadId_t vStartUartDataRecvTask(void)
{
    // 初始化RTOS组件
    uart_rtos_init();
    
    const osThreadAttr_t UartDataRecvThreadAttr = {
        .name = "Uart_DataRecv_Task",
        .attr_bits = 0,
        .cb_mem = NULL,
        .cb_size = 0,
        .stack_mem = NULL,
        .stack_size = UART_DATARECV_TASK_STACK_SIZE,
        .priority = UART_DATARECV_TASK_PRIORITY,
        .tz_module = 0,
    };

    // Create pm Task
    return osThreadNew(vUartDataRecvTask, NULL, &UartDataRecvThreadAttr);
}

/** @} */

// vim: fdm=marker
