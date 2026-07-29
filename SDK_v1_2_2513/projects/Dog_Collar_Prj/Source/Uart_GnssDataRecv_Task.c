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
#include "cJSON.h"
#include "timers.h"
/*********************************************************************
 * MACROS
 */
#define EVENT_SYSTEM_RESERVE_MASK   0x00FF

#define UART_GNSSDATARECV_TASK_PRIORITY (osPriorityNone)
#define UART_GNSSDATARECV_TASK_STACK_SIZE (4096)

// 阻塞控制相关定义
#define UART_GNSS_EVENT_TASK_BLOCK      (1 << 0)      // 任务阻塞标志
#define UART_GNSS_EVENT_TASK_UNBLOCK    (1 << 1)      // 任务解除阻塞标志

/*********************************************************************
 * TYPEDEFS
 */

typedef struct {
    om_fifo_t          fifo;
    uint8_t             fifo_buffer[UART_GNSS_RECV_DATA_SIZE];
} shell_env_t;

shell_env_t gps_uartFifo_env;

osTimerId_t  GnssUartTimeoutTimer = NULL;
osSemaphoreId_t GnssUartDataReadySem = NULL;

// 添加阻塞控制变量
static osEventFlagsId_t GnssUartEventId = NULL;
static bool gnss_uart_task_running = true;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
bool safe_block_gnss_uart_task(void)
{
    uint32_t current_flags = osEventFlagsGet(GnssUartEventId);
    
    if (current_flags & UART_GNSS_EVENT_TASK_BLOCK) {
        log_debug("[G-UART][STA] task already blocked, skip block\r\n");
        return false;
    }
    
    osEventFlagsClear(GnssUartEventId, UART_GNSS_EVENT_TASK_UNBLOCK);
    osEventFlagsSet(GnssUartEventId, UART_GNSS_EVENT_TASK_BLOCK);
    gnss_uart_task_running = false;
    return true;
}

bool safe_unblock_gnss_uart_task(void)
{
    uint32_t current_flags = osEventFlagsGet(GnssUartEventId);
    
    if (!(current_flags & UART_GNSS_EVENT_TASK_BLOCK)) {
        log_debug("[G-UART][STA] task not blocked, skip unblock\r\n");
        return false;
    }
    
    osEventFlagsClear(GnssUartEventId, UART_GNSS_EVENT_TASK_BLOCK);
    osEventFlagsSet(GnssUartEventId, UART_GNSS_EVENT_TASK_UNBLOCK);
    gnss_uart_task_running = true;
    return true;
}

bool should_gnss_uart_task_block(void)
{
    return (osEventFlagsGet(GnssUartEventId) & UART_GNSS_EVENT_TASK_BLOCK) != 0;
}

void wait_for_gnss_uart_task_unblock(void)
{
    osEventFlagsWait(GnssUartEventId, UART_GNSS_EVENT_TASK_UNBLOCK, osFlagsWaitAny, osWaitForever);
    osEventFlagsClear(GnssUartEventId, UART_GNSS_EVENT_TASK_UNBLOCK);
    gnss_uart_task_running = true;
}

// 超时定时器回调
static void GnssUartTimeoutCallback(void *argument)
{
    osSemaphoreRelease(GnssUartDataReadySem); // 通知任务数据接收完成
}

static void gps_uart2_cb(void *om_uart, drv_event_t event, void *rxbuf, void *rx_cnt)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    if (event == DRV_EVENT_COMMON_READ_COMPLETED) {
        // 只在任务运行时处理数据
        if (gnss_uart_task_running) {
            // 数据存入FIFO
            om_fifo_in(&gps_uartFifo_env.fifo, (uint8_t *)rxbuf, (uint32_t)rx_cnt);
            
            // 重置超时定时器（50ms）
//            osTimerStop(GnssUartTimeoutTimer);
//            osTimerStart(GnssUartTimeoutTimer, 50U); // 50ms超时
            
            // 如果这是第一个数据包，通知任务
            if (osSemaphoreGetCount(GnssUartDataReadySem) == 0) {
                osSemaphoreRelease(GnssUartDataReadySem);
            }
        }
    }
    
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


void gps_uart2_init(void)
{
    uart_config_t uart_cfg = {
        .baudrate       = GPS_AT_UART_BAUDRATE,
        .flow_control   = UART_FLOW_CONTROL_NONE,
        .data_bit       = UART_DATA_BIT_8,
        .stop_bit       = UART_STOP_BIT_1,
        .parity         = UART_PARITY_NONE,
    };

	// 创建信号量和定时器
	if (GnssUartDataReadySem == NULL) {
		GnssUartDataReadySem = osSemaphoreNew(1, 0, NULL);
    } else {
    }
	
    // 创建定时器
    const osTimerAttr_t timer_attr = {
        .name = "UartTimeout",
        .attr_bits = 0,
        .cb_mem = NULL,
        .cb_size = 0,
    };
    if (GnssUartTimeoutTimer == NULL)
	{
		GnssUartTimeoutTimer = osTimerNew(GnssUartTimeoutCallback, osTimerOnce, NULL, &timer_attr);
		if (GnssUartTimeoutTimer != NULL) {
		} 
		else 
		{
		}
	} else 
	{
	}
	
    drv_uart_init(GPS_AT_UART, &uart_cfg);
    drv_uart_register_isr_callback(GPS_AT_UART, gps_uart2_cb);
  	om_fifo_init(&gps_uartFifo_env.fifo, gps_uartFifo_env.fifo_buffer, sizeof(gps_uartFifo_env.fifo_buffer));
    drv_uart_read_int(GPS_AT_UART, NULL, 0);
}

void gps_uart2_deinit(void)
{
    // 停止UART接收
    drv_uart_read_int(GPS_AT_UART, NULL, 0);
    
    // 清空FIFO
    om_fifo_reset(&gps_uartFifo_env.fifo);
    
    // 停止定时器
    osTimerStop(GnssUartTimeoutTimer);
}

void vUartGnssDataRecvTask(void *pvParameters)
{
    uint16_t GpsFifoLength = 0;
    
    TaskInfo_t *my_task_info = GetTaskInfo(UART_GNSSDATARECV_ID);  
    TaskInfo_t *gnss_task_info = GetTaskInfo(GNSS_UART_TASK_ID);
    
    Message_t received_uart2_msg;
    
    log_debug("[G-UART][STA] Task %d started\r\n", my_task_info->task_id);
    
    while (1) {
        // 检查任务是否应该阻塞
        if (should_gnss_uart_task_block()) {
            log_debug("[G-UART][STA] task entering block\r\n");
            wait_for_gnss_uart_task_unblock();
        }
        
        if(osOK == osMessageQueueGet(my_task_info->queue_handle,&received_uart2_msg, NULL, 100))
        {
            // 回复消息给GNSS_UART_TASK_ID任务
            if(received_uart2_msg.source_id == GNSS_UART_TASK_ID)
            {  
                if(received_uart2_msg.command == TASK_CMD_START)
                {
                    //初始化IO
                    gps_uart2_init();
                    safe_unblock_gnss_uart_task();
                }
                else if(received_uart2_msg.command == TASK_CMD_STOP)
                {
                    //关机,配置IO
                    gps_uart2_deinit();
                    safe_block_gnss_uart_task();
                }
            }
        }
        
        if (osSemaphoreAcquire(GnssUartDataReadySem, osWaitForever) == osOK) {
            vTaskDelay(pdMS_TO_TICKS(60));
            uint16_t available_gps_data = om_fifo_len(&gps_uartFifo_env.fifo);
            if (available_gps_data > 0) {
                uint8_t *GpsRecvSubpackage = DEMO_BT_Malloc(available_gps_data + 1);
                
                if (GpsRecvSubpackage != NULL) 
                {
                    GpsFifoLength = 0;
                    
                    // 一次性取出FIFO中所有数据
                    uint16_t rx_len = 0;
                    uint16_t remaining = available_gps_data;
                    while (!om_fifo_is_empty(&gps_uartFifo_env.fifo) && 
                           remaining > 0) {
                        rx_len = om_fifo_out(&gps_uartFifo_env.fifo, 
                                           GpsRecvSubpackage + GpsFifoLength, 
                                           remaining);
                        if (rx_len == 0) {
                            break; 
                        }
                        
                        GpsFifoLength += rx_len;
                        remaining -= rx_len;
                    }
                 //   drv_uart_write(LOG_UART, (uint8_t *)GpsRecvSubpackage, (uint32_t)GpsFifoLength, 10);
                    // 处理完整数据包
                    if (GpsFifoLength > 0) {
                        GpsRecvSubpackage[GpsFifoLength] = '\0';  // 添加字符串终止符
                        
                        Message_t msg = {
                            .source_id = my_task_info->task_id,
                            .dest_id = GNSS_UART_TASK_ID,
                            .data = GpsRecvSubpackage,
                            .data_length = GpsFifoLength
                        };

                        if (osMessageQueuePut(gnss_task_info->queue_handle, &msg, NULL, 0) != osOK) {
                            DEMO_BT_Free(GpsRecvSubpackage);
                        }
                    }
                }
            }
        }
    }
}


/**
 * @brief 初始化串口任务RTOS组件
 **/
void gpsUart_rtos_init(void)
{
	// 初始化事件组
    GnssUartEventId = osEventFlagsNew(NULL);
    if (GnssUartEventId == NULL) {
		LOG_LOC();
//        err_lock(1);
    }
    
    // 默认状态为运行
    gnss_uart_task_running = true;
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
osThreadId_t vStartUartGnssDataRecvTask(void)
{
    gpsUart_rtos_init();
    
    const osThreadAttr_t UartGnssDataRecvThreadAttr = {
        .name = "GNSS_UartTTT_DataRecv_Task",
        .attr_bits = 0,
        .cb_mem = NULL,
        .cb_size = 0,
        .stack_mem = NULL,
        .stack_size = UART_GNSSDATARECV_TASK_STACK_SIZE,
        .priority = UART_GNSSDATARECV_TASK_PRIORITY,
        .tz_module = 0,
    };

    // Create pm Task
    return osThreadNew(vUartGnssDataRecvTask, NULL, &UartGnssDataRecvThreadAttr);
}

/** @} */

// vim: fdm=marker