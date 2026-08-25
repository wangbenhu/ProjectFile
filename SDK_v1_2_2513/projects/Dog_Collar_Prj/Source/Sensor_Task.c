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
#include "imu_ex.h"

// Controller header
#include "obc.h"

/* Kernel includes. */
#include "cmsis_os2.h"

#include "timers.h"
#include "pedometer_ex.h"
#include "atomic_embedded.h"
/*********************************************************************
 * MACROS
 */
#define EVENT_SYSTEM_RESERVE_MASK   0x00FF

#define SENSOR_TASK_PRIORITY (osPriorityNormal)
#define SENSOR_TASK_STACK_SIZE (1024*2)  //1000

#define WORK_OUT_MOTION_LEVEL_PERIOD_MS  1000*60  //ms 计算运动强度周期

osTimerId_t sensor_test_timer;

/*********************************************************************
 * TYPEDEFS
 */




/*********************************************************************
 * CONSTANTS
 */
typedef enum {
    SENSOR_STATE_STOPPED = 0,
    SENSOR_STATE_RUNNING,
} sensor_run_state_t;
typedef enum {
    SENSOR_BLOCK_NONE = 0,
    SENSOR_BLOCK_WAITING,
} sensor_block_state_t;
/*********************************************************************
 * LOCAL VARIABLES
 */
static osSemaphoreId_t g_SensorSemaphore = NULL;//低功耗是否进入的标志
 
static osTimerType_t Sensor_Motion_Detect_Timer_type = osTimerPeriodic; //osTimerOnce,  osTimerPeriodic
static osTimerAttr_t Sensor_Motion_Detect_Timer_attr = {
	.name = "Sensor_Motion_Detect_Timer",
};

static osSemaphoreId_t g_SensorGetFifoSemaphore = NULL;//
																			
static uint8_t sensor_task_cmd_status = TASK_CMD_END; 			
/* 传感器任务状态（必须 volatile，防止优化问题） */
static atomic_u8_t g_sensor_blocked = SENSOR_BLOCK_NONE;   // 1 = 阻塞
																						
/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERN FUNCTIONS
 */
extern uint8_t PM_GetBatteryCapacity(void);		
extern uint8_t Message_Cmd_Put(TASK_ID_T source_id,
                                       TASK_ID_T dest_id,
                                       TASK_CMD_T command,
                                       void *data,
                                       uint16_t data_length);
extern int imu_fifo_timeout_recover(void);
extern state_posture_t get_pet_ai_result_class_id(void);
/*********************************************************************
 * LOCAL FUNCTIONS
 */
/*
* @brief  传感器任务获取FIFO事件
 */
 void SensorGetFifoEvent(void)
{
	if (g_SensorGetFifoSemaphore == NULL) {
        return;
    }

    if (sensor_task_cmd_status != TASK_CMD_START) {
        return;  // STOP/休眠期间丢弃 FIFO 事件
    }

    (void)osSemaphoreRelease(g_SensorGetFifoSemaphore);

}
void sensor_task_start(void)
{
	log_debug("sensor_task_start:%d,%d\r\n",g_sensor_blocked,SENSOR_BLOCK_WAITING);
	if(g_sensor_blocked == SENSOR_BLOCK_WAITING)
	{
		g_sensor_blocked = SENSOR_BLOCK_NONE;
		osSemaphoreRelease(g_SensorSemaphore);
	}
    Message_Cmd_Put(ENTRY_TASK_ID,SENSOR_TASK_ID,TASK_CMD_START,NULL, 0);
}
void sensor_task_stop(void)
{
	if(sensor_task_cmd_status != TASK_CMD_STOP)
	{
		if(g_sensor_blocked == SENSOR_BLOCK_WAITING)
		{
			g_sensor_blocked = SENSOR_BLOCK_NONE;
			osSemaphoreRelease(g_SensorSemaphore);
		}
		Message_Cmd_Put(ENTRY_TASK_ID,SENSOR_TASK_ID,TASK_CMD_STOP,NULL, 0);
	}
}
void my_sensor_timer_callback(void *argument) {
	log_debug("get_step_num = %ld\r\n",get_step_num());
	log_debug("get_pet_ai_result_class_id = %d\r\n",get_pet_ai_result_class_id());
}
void Sensor_Init(void)
{
	log_debug("Sensor_Init\r\n");
}
/**
 * @brief  schedule task
 *
 * @param[in] pvParameters  pv parameters
 **/
static void vSensorTask(void *argument)
{
    //TaskInfo_t *my_info = (TaskInfo_t *)pvParameters;
    TaskInfo_t *my_task_info = GetTaskInfo(SENSOR_TASK_ID);
    TaskInfo_t *entry_task_info = GetTaskInfo(ENTRY_TASK_ID);

    Message_t received_msg;
    sensor_test_timer = osTimerNew(my_sensor_timer_callback, osTimerPeriodic, NULL, NULL);
	if (g_SensorSemaphore == NULL) {
		g_SensorSemaphore = osSemaphoreNew(1, 0, NULL);
    } else {
        log_debug("vSensorTask already exists, skip creation\r\n");
    }
    if (g_SensorGetFifoSemaphore == NULL) {
		g_SensorGetFifoSemaphore = osSemaphoreNew(1, 0, NULL);
    } else {
        log_debug("vSensorTask already exists, skip creation\r\n");
    }
 //   log_debug("Task %d started\n", my_task_info->task_id);
    for(;;) 
    {
		osSemaphoreRelease(g_SensorSemaphore);
        // 接收消息
        if(osOK == osMessageQueueGet(my_task_info->queue_handle,&received_msg, NULL, 0))
        {
            log_debug("vSensorTask %d received from:%d,%lu,%d\r\n", 
                  my_task_info->task_id, received_msg.source_id, received_msg.command,sensor_task_cmd_status);
            // 回复消息给Entry任务
            if((received_msg.source_id == ENTRY_TASK_ID) && (received_msg.command == TASK_CMD_START))
            {  
				if(sensor_task_cmd_status == TASK_CMD_START)//重入互斥
						continue ;
				sensor_task_cmd_status=TASK_CMD_START;
				
				//接收到TASK_CMD_START后，初始化sensor为WOM模式
				imu_init(); //只配置一次	
				osTimerStart(sensor_test_timer, osMS2TicksRound(5000));				
            }
            else if((received_msg.source_id == ENTRY_TASK_ID) && (received_msg.command == TASK_CMD_STOP))
            {
				if(sensor_task_cmd_status == TASK_CMD_STOP)//重入互斥
					continue ;
				sensor_task_cmd_status=TASK_CMD_STOP;
				
				osTimerStop(sensor_test_timer);
				clear_int1_status();
                imu_enter_sleep_mode();          // 内部显式 disable pedometer
                /* 清掉已积压的 FIFO 唤醒令牌 */
                while (osSemaphoreAcquire(g_SensorGetFifoSemaphore, 0) == osOK) {
                }
				osSemaphoreAcquire(g_SensorSemaphore, osWaitForever);
            }
        }
        if(sensor_task_cmd_status == TASK_CMD_START)
        {
            if (osSemaphoreAcquire(g_SensorGetFifoSemaphore, osMS2TicksRound(1000)) == osOK) {
            get_imu_data();//获取IMU数据
            if(get_imu_state())//判断是否有初始化
            {
                set_step_num();//设置步数
            }
            } else {
                imu_fifo_timeout_recover();
            }
        }
        else
        {
            osDelay(osMS2TicksRound(1000));
        }
		/* 进入阻塞前标记 */
//        atomic_store_u8(&g_sensor_blocked, SENSOR_BLOCK_WAITING);
		g_sensor_blocked = SENSOR_BLOCK_WAITING;
		osSemaphoreAcquire(g_SensorSemaphore, osWaitForever);
		g_sensor_blocked = SENSOR_BLOCK_NONE;
		 /* 被唤醒 */
//        atomic_store_u8(&g_sensor_blocked, SENSOR_BLOCK_NONE);
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
osThreadId_t vStartSensorTask(void)
{
    const osThreadAttr_t SensorThreadAttr = {
        .name = "Sensor_Task",
        .attr_bits = 0,
        .cb_mem = NULL,
        .cb_size = 0,
        .stack_mem = NULL,
        .stack_size = SENSOR_TASK_STACK_SIZE,
        .priority = SENSOR_TASK_PRIORITY,
        .tz_module = 0,
    };

    // Create pm Task
    return osThreadNew(vSensorTask, NULL, &SensorThreadAttr);
}

/** @} */

// vim: fdm=marker
