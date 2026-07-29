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
#include "lfs_port.h"
#include "cJSON.h"
#include "imu_ex.h"
#include "pedometer_ex.h"
#include "lfs_port.h"
#include "test_audio.h"
#include "m_motor.h"
#include "my_audio.h"
#include "led_task.h"
#include "Comm_Task.h"
#include "m_battery.h"
/*********************************************************************
 * MACROS
 */

#define EVENT_SYSTEM_RESERVE_MASK   0x00FF

#define TEST_TASK_PRIORITY (osPriorityNormal)
#define TEST_TASK_STACK_SIZE (10240)

#define TEST_LED_TIME 2000//unit 
#define TEST_MOTOR_TIME 5
#define TEST_ADC_TIME 2000
#define TETS_SENSOR_TIME (3000)
#define TEST_ADC_ELECTRIC (1267)
#define TEST_ADC_ELECTRIC_OFFSET (66)
#define TEST_AGING_TIME (60*1000)//unit ms
#define TSTS_ASING_LED_TIME (3*1000)
/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * CONSTANTS
 */


/*********************************************************************
 * LOCAL VARIABLES
 */
static osSemaphoreId_t xSemAgingTestTask_Time = NULL;//ADC电池电压读取完成的判�?
static uint8_t test_task_cmd_status = TASK_CMD_END; 

static uint8_t error_code_tmp=REPORT_ERROR_CODE_MAX;

static uint32_t production_mode_tmp = 0;

static uint8_t test_mutual_flag = 0;//做测试�?�例的互斥操作，开始和上报结束标志
static uint8_t production_start_flag = 0;


static uint8_t cat1_power_down_replay_result_flag = 0; 


static uint8_t asing_mode_entry_flag = 0;

static uint8_t asing_led_set = 0;
static uint8_t asing_led_clear_flag = 0;
/*********************************************************************
 * GLOBAL VARIABLES
 */
osTimerId_t Test_Timer_ID = NULL;
osTimerId_t Asing_Test_Timer_ID = NULL;
osTimerAttr_t Test_Timer_attr = {
	.name = "Test_Timer",
};
osTimerAttr_t Asing_Test_Timer_attr = {
	.name = "Asing_Test_Timer",
};
osEventFlagsId_t g_Production_Task_Flags;

/*********************************************************************
 * EXTERN FUNCTIONS
 */
//0/finish other/failed
extern void Production_Result_Report(uint8_t error_code);
extern uint8_t Message_Cmd_Put(TASK_ID_T source_id,
                                       TASK_ID_T dest_id,
                                       TASK_CMD_T command,
                                       void *data,
                                       uint16_t data_length);
// LED全亮
extern void led_turn_on_all(void);
// LED全灭
extern void led_turn_off_all(void); 
									   
extern uint32_t get_gpadc_read_data(void);	
extern void pm_vbattery_get(void);
extern uint8_t get_sensor_xyz_data(void);
extern void m4_to_production_config(void);
extern void unblock_cat1_task(void);
extern bool safe_unblock_uart_task(void);
									   														   
extern void user_initiative_reboot_fun(void);

extern void sensor_task_stop(void);
extern void sensor_task_start(void);
extern bool safe_unblock_gps_task(void);
extern bool safe_unblock_gnss_uart_task(void);		
extern void Production_Param_Result_Report(RESPONSE_TYPES type);		
extern void user_initiative_test_reboot_fun(void);			
extern void product_flag_clear(void);
extern void RepetFlagSet(void);
extern uint8_t RepetFlagGet(void);
extern void evt_app_adv_stop(void);
extern void StopPlayFlagSet(void);
extern void Production_Lte_Result_Report(eProductionReportCod error_code);
/*********************************************************************
 * LOCAL FUNCTIONS
 */


/**
 * @brief  设置触发事件
 *
 * @param[in] event  事件标志
 **/
void Test_Task_Set_Event(uint32_t event)
{
	if(event < PRODUCTION_TASK_EXAMPLE_START || event >= PRODUCTION_TASK_EXAMPLE_END)
	{
		return;
	}
    osEventFlagsSet(g_Production_Task_Flags, event);
}
/*
* @brief  测试任务定时器回调函数
* @param[in] argument  无
* @return  无
*/
void AsingTestTimerCallback(void *argument)
{
	if(asing_led_set)
	{
		led_control(4, _WHITE, false, 0);
		led_control(4, _RED, true,0);
		asing_led_set=0;
	}
	else
	{
		led_control(4, _WHITE, true, 0);
		led_control(4, _RED, false,0);
		asing_led_set=1;
	}
}
void TestTimerCallback(void *argument)
{
	log_debug("production_mode_tmp = %d %d\r\n",production_mode_tmp,asing_mode_entry_flag);
	switch(asing_mode_entry_flag)//进入测试模式
	{
		case 1://开灯 关电机 关喇叭
			StopPlayFlagSet();//停止播放
			m_motor_set(MOTOR_STOP_RUN,0);//电机停止
		
			asing_led_clear_flag=2;//开启LED定时器
			led_control(4, _WHITE, true, 0);
			led_control(4, _RED, false,0);
			asing_led_set=1;//重置LED状态为亮
			break;
		case 2://关灯 关电机 开喇叭
			RepetFlagSet();//设置重�?�播�?
			Audio_Play_Start("/audio/player/p1.wav",1);
			m_motor_set(MOTOR_STOP_RUN,0);
			asing_led_clear_flag =1;
			break;
		case 3://关喇叭 开电机 关灯
			StopPlayFlagSet();//停止播放
			m_motor_set(MOTOR_ALWAYS_RUN,0);//电机持续�?�?
			asing_led_clear_flag=1;
			break;
		default:asing_mode_entry_flag = 0;
			break;	
	}
	if(asing_mode_entry_flag)//测试模式结束
	{
		asing_mode_entry_flag = (asing_mode_entry_flag%3)+1;//切换测试模式
		return;
	}
	
	if(production_mode_tmp == PRODUCTION_TASK_EXAMPLE_2)
	{
		led_turn_off_all();
		error_code_tmp=REPORT_OK;
	}
	else if(production_mode_tmp == PRODUCTION_TASK_EXAMPLE_3)
	{
		error_code_tmp=REPORT_OK;
	}
	else if(production_mode_tmp == PRODUCTION_TASK_EXAMPLE_4)
	{
		error_code_tmp=REPORT_OK;
		if(lfs_file_exists_and_empty("/audio/record/tmp.wav") == 2)
		{
			lfs_clear_file_content("/audio/record/tmp.wav");
		}
		
		lfs_unmount_safe();
	}
	else if(production_mode_tmp == PRODUCTION_TASK_EXAMPLE_7)
	{
		//判断三轴数据
		if(get_sensor_xyz_data())
		{
			error_code_tmp=REPORT_OK;
			//成功
		}
		else
		{
			error_code_tmp=REPORT_SENSOR_ERROR;
			//失败
		}
		log_debug("PRODUCTION_TASK_EXAMPLE_7 = %d\r\n",error_code_tmp);
		sensor_task_stop();
	}
	else{
	
	}
	osEventFlagsSet(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);
}

uint8_t get_product_errorcode(void)
{
	return error_code_tmp;
}
void RestartTimer(void) {
    if (Test_Timer_ID != NULL) {
        osTimerStop(Test_Timer_ID);   // 
        osTimerDelete(Test_Timer_ID); // 
        Test_Timer_ID = NULL;          // 
    }
    Test_Timer_ID = osTimerNew(TestTimerCallback, osTimerPeriodic, NULL, &Test_Timer_attr);
	osTimerStart(Test_Timer_ID, osMS2TicksRound(TEST_AGING_TIME));
}
/**
 * @brief  schedule task
 *
 * @param[in] pvParameters  pv parameters
 **/
static void vTestTask(void *argument)
{
    //TaskInfo_t *my_info = (TaskInfo_t *)pvParameters;
    TaskInfo_t *my_task_info = GetTaskInfo(TEST_TASK_ID);
    TaskInfo_t *entry_task_info = GetTaskInfo(ENTRY_TASK_ID);

    Message_t received_msg;
    
    log_debug("Task %d started\n", my_task_info->task_id);
	
	if (xSemAgingTestTask_Time == NULL) {
		xSemAgingTestTask_Time = osSemaphoreNew(1, 0, NULL);
    } else {
        log_debug("Semaphore already exists, skip creation\n");
    }
	
#if (CONFIG_TEST_PRODUCTION)
	g_Production_Task_Flags = osEventFlagsNew(NULL);
    if (g_Production_Task_Flags == NULL) {
      log_debug("g_Production_Task_Flags creat failed\r\n");
    }
	else
	{
		log_debug("g_Production_Task_Flags creat finish\r\n");
	}
#endif
	Test_Timer_ID = osTimerNew(TestTimerCallback,osTimerOnce,NULL,&Test_Timer_attr);
	Asing_Test_Timer_ID =  osTimerNew(AsingTestTimerCallback,osTimerPeriodic,NULL,&Asing_Test_Timer_attr);
    for(;;) 
    {
		if(asing_led_clear_flag)
		{
			if(asing_led_clear_flag == 1)
			{
				if(osTimerIsRunning(Asing_Test_Timer_ID))
				{
					osTimerStop(Asing_Test_Timer_ID);
					log_debug("AsingTestTimerCallback stopped by task\r\n");
				}	
				led_turn_off_all();
			}
			if(asing_led_clear_flag == 2)
			{
				osTimerStart(Asing_Test_Timer_ID, osMS2TicksRound(TSTS_ASING_LED_TIME));
			}
			asing_led_clear_flag=0;
		}
    
		osSemaphoreRelease(xSemAgingTestTask_Time);

        // 接收消息
        if(osOK == osMessageQueueGet(my_task_info->queue_handle,&received_msg, NULL, 100)) //portMAX_DELAY
        {
            log_debug("Task %d received from %d: %lu\n", 
                  my_task_info->task_id, received_msg.source_id, received_msg.command);
            // 回�?�消�?给Entry任务
            if((received_msg.source_id == ENTRY_TASK_ID))
            { 
				if(received_msg.command == TASK_CMD_START)
				{
					if(test_task_cmd_status == TASK_CMD_START)//重入互斥
						continue ;
					test_task_cmd_status=TASK_CMD_START;
					
					production_start_flag = 1;
					Test_Task_Set_Event(PRODUCTION_TASK_EXAMPLE_START);
				}
				if(received_msg.command == TASK_CMD_STOP)
				{
					if(test_task_cmd_status == TASK_CMD_STOP)//重入互斥
						continue ;
					test_task_cmd_status=TASK_CMD_STOP;
					production_start_flag=0;
					osSemaphoreAcquire(xSemAgingTestTask_Time,osWaitForever);
				}
            }
			else if(received_msg.source_id == CAT1_UART_TASK_ID)
			{
				if(received_msg.command == TASK_CMD_CAT1_TEST_REPLAY)
				{
					log_debug("TASK_CMD_CAT1_TEST_REPLAY\r\n");
					//siminfo_set_sn(received_msg.data,received_msg.data_length);
					system_info_set(received_msg.data,received_msg.data_length,SYS_DEVICE_SN_ID);
					osDelay(osMS2TicksRound(5));
					
					if(strlen(system_info_get(SYS_DEVICE_SN_ID)) <= 0)
					{
						cat1_power_down_replay_result_flag = 0;
					//	Message_Cmd_Put(TEST_TASK_ID,CAT1_UART_TASK_ID,TASK_CMD_CAT1_TEST_START,NULL,0);
						
					}
					else
					{
						cat1_power_down_replay_result_flag=1;
//						error_code_tmp=REPORT_OK;
//						osEventFlagsSet(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);
					}	
					DEMO_BT_Free(received_msg.data);
					Message_Cmd_Put(TEST_TASK_ID,CAT1_UART_TASK_ID,TASK_CMD_STOP,NULL,0);
				}
				if(received_msg.command == TASK_STOP_REPLY)
				{
					log_debug("TASK_STOP_REPLY = %d\r\n",production_mode_tmp);
					if(production_mode_tmp == PRODUCTION_TASK_EXAMPLE_5 || production_mode_tmp == PRODUCTION_TASK_EXAMPLE_13\
						|| production_mode_tmp == PRODUCTION_TASK_EXAMPLE_17)
					{
						if(production_mode_tmp == PRODUCTION_TASK_EXAMPLE_5)
						{
							Production_Param_Result_Report(PRODUCT_REPORT_TYPE_CAT1_VER);
							production_mode_tmp = 0;
						}
						if(production_mode_tmp == PRODUCTION_TASK_EXAMPLE_13)
						{
							log_debug("PRODUCTION_TASK_EXAMPLE_13 =%d\r\n",cat1_power_down_replay_result_flag);
							if(cat1_power_down_replay_result_flag)
							{
								error_code_tmp=REPORT_OK;
							}
							else
							{
								error_code_tmp=REPORT_CAT1_TEST_ERROR;
							}
							
							osEventFlagsSet(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);
						}
						if(production_mode_tmp == PRODUCTION_TASK_EXAMPLE_17)
						{
							log_debug("PRODUCTION_TASK_EXAMPLE_17 =%d\r\n",cat1_power_down_replay_result_flag);
							if(cat1_power_down_replay_result_flag)
							{
								error_code_tmp=REPORT_OK;
							}
							else
							{
								error_code_tmp=REPORT_CAT1_TEST_ERROR;
							}
							Production_Lte_Result_Report(error_code_tmp);
							production_mode_tmp=0;
						}
						//LTE带证书测试通过
					}
					else if(production_mode_tmp ==PRODUCTION_TASK_EXAMPLE_1)//文件系统创建测试//三个证书写入测试
					{
					//卸载文件系统
						lfs_unmount_safe();
						if(cat1_power_down_replay_result_flag)
						{
							error_code_tmp=REPORT_OK;
						}
						else
						{
							error_code_tmp=REPORT_LFS_MOUNTED_ERROR;
							
						}
						osEventFlagsSet(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);
					}
					else
					{
						
					}	
					//osEventFlagsSet(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);
				}
				if(received_msg.command == TASK_CAT1_AWS_CACERT_REPLY)
				{
					log_debug("TASK_CAT1_AWS_CACERT_REPLY\r\n");
					Message_Cmd_Put(TEST_TASK_ID,CAT1_UART_TASK_ID,TASK_CMD_STOP,NULL,0);
					log_debug("*(uint8_t *)received_msg.data = %d %d\r\n",*(uint8_t *)received_msg.data,received_msg.data);
					uint8_t tmp_data = *(uint8_t *)received_msg.data;
					if(tmp_data)
					{
						error_code_tmp = REPORT_CA_WRITE_ERROR;
					}
					else
					{
						error_code_tmp=REPORT_OK;	
					}				
//					osEventFlagsSet(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);
				}
				if(received_msg.command == TASK_CAT1_AWS_CLIENT_REPLY)
				{
					log_debug("TASK_CAT1_AWS_CLIENT_REPLY\r\n");
					uint8_t tmp_data = *(uint8_t *)received_msg.data;
					if(tmp_data)
					{
						error_code_tmp = REPORT_CLIENT_ERROR;
					}
					else
					{
						error_code_tmp=REPORT_OK;	
					}
//					osEventFlagsSet(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);
					
					Message_Cmd_Put(TEST_TASK_ID,CAT1_UART_TASK_ID,TASK_CMD_STOP,NULL,0);
				}
				if(received_msg.command == TASK_CAT1_AWS_USERKEY_REPLY)
				{	
					log_debug("TASK_CAT1_AWS_USERKEY_REPLY\r\n");
					uint8_t tmp_data = *(uint8_t *)received_msg.data;
					if(tmp_data)
					{
						error_code_tmp = REPORT_USERKEY_ERROR;
					}
					else
					{
						error_code_tmp=REPORT_OK;
						
					}
//					osEventFlagsSet(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);

					Message_Cmd_Put(TEST_TASK_ID,CAT1_UART_TASK_ID,TASK_CMD_STOP,NULL,0);
				}
				if(received_msg.command == TASK_CMD_CAT1_VERSION_REPLY)
				{
					log_debug("received_msg.command == TASK_CMD_CAT1_VERSION_REPLY\r\n");
					Message_Cmd_Put(TEST_TASK_ID,CAT1_UART_TASK_ID,TASK_CMD_STOP,NULL,0);
				}
				if(received_msg.command == TASK_CAT1_AWS_WITHCA_TEST_REPLY)
				{
					
					Message_Cmd_Put(TEST_TASK_ID,CAT1_UART_TASK_ID,TASK_CMD_STOP,NULL,0);
					uint8_t tmp_data = *(uint8_t *)received_msg.data;
					log_debug("TASK_CAT1_AWS_WITHCA_TEST_REPLY = %d\r\n",tmp_data);
					if(tmp_data)
					{
						cat1_power_down_replay_result_flag = 0;
					}
					else
					{
						cat1_power_down_replay_result_flag = 1;
					}
				}
				
			}
			else if((received_msg.source_id == COMM_TASK_ID) && (received_msg.command == TASK_CMD_TEST_EXAMPLE))
			{
				log_debug("TASK_CMD_TEST_EXAMPLE = %d %#x %d\r\n",*(uint32_t*)received_msg.data,*(uint32_t*)received_msg.data,production_start_flag);
				Test_Task_Set_Event(*(uint32_t*)received_msg.data);
			}
			else if(received_msg.source_id == GNSS_UART_TASK_ID)
			{
				if(received_msg.command == TASK_PRODUCT_GPS_TEST_REPLY)
				{
					log_debug("received_msg.command == TASK_PRODUCT_GPS_TEST_REPLY\r\n");
					Message_Cmd_Put(TEST_TASK_ID,GNSS_UART_TASK_ID,TASK_CMD_STOP,NULL,0);
					Production_Param_Result_Report(PRODUCT_REPORT_TYPE_GPS_VER);
					production_mode_tmp = 0;
				}
			}
            else
            {
                //handle other logic
            }
        }	

	if(production_start_flag)
	{
		if(get_ble_status() != BLE_STATE_CONNECTED &&RepetFlagGet() == 0)
		{
			user_initiative_reboot_fun();
		}
#if (CONFIG_TEST_PRODUCTION)
		uint32_t os_event_flags;
			/* 等待所有任务�?�置就绪标志，无限期等待 */
		os_event_flags = osEventFlagsWait(g_Production_Task_Flags, 
                            ALL_PRODUCTION_TASK_EXAMPLE, 
                            osFlagsWaitAny, 
                            10);
//		log_debug("osEventFlagsWait = %#x\r\n",os_event_flags);
		if (os_event_flags & osFlagsError) {
			// 如果�?超时，可以不做�?�理，或者其他错�?处理
			if (os_event_flags == osFlagsErrorTimeout) {
//				log_debug("timeout\r\n");
				// 超时，�?�常情况，继�?�?�?
			} else {
				// 其他错�??，需要�?�理
//				log_debug("EventFlags error: %#x\r\n", os_event_flags);
			}
		}
		else
		{
			if (os_event_flags & PRODUCTION_TASK_EXAMPLE_START) {
				log_debug("PRODUCTION_TASK_EXAMPLE_START\r\n");
			
				//SENSOR START
				//默�?�状态数�?�?0 代表task没运�?
				//并�?�取步数 和运动状�?
				osEventFlagsClear(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_START);
				error_code_tmp=REPORT_OK;
				osEventFlagsSet(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);
			}
			if(os_event_flags & PRODUCTION_TASK_EXAMPLE_1)
			{
				osEventFlagsClear(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_1);
				//文件系统初�?�化
				if(lfs_init()!=0)
				{
					error_code_tmp=REPORT_LFS_INIT_ERROR;
					osEventFlagsSet(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);
				}
				else
				{		
									//初�?�化系统数据
					safe_unblock_uart_task();
					unblock_cat1_task();
					Message_Cmd_Put(TEST_TASK_ID,CAT1_UART_TASK_ID,TASK_CMD_CAT1_TEST_START,NULL,0);	
					log_debug("PRODUCTION_TASK_EXAMPLE_1 start\r\n");	
					production_mode_tmp = PRODUCTION_TASK_EXAMPLE_1;
				}
			}
			if(os_event_flags & PRODUCTION_TASK_EXAMPLE_2)
			{	
				osEventFlagsClear(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_2);
				if(test_mutual_flag)
				{	
					error_code_tmp=REPORT_REPETITION_ERROR;
					osEventFlagsSet(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);
				}
				else
				{
					test_mutual_flag=1;
					production_mode_tmp = PRODUCTION_TASK_EXAMPLE_2;
					
					if(osTimerIsRunning(Test_Timer_ID))
					{
						osTimerStop(Test_Timer_ID);
					}
					osTimerStart(Test_Timer_ID, osMS2TicksRound(TEST_LED_TIME));

					led_turn_on_all();
				}
			
				log_debug("PRODUCTION_TASK_EXAMPLE_2 start = %d\r\n",test_mutual_flag);
			}
			if(os_event_flags & PRODUCTION_TASK_EXAMPLE_3)//电机测试
			{
				if(test_mutual_flag)
				{	
					error_code_tmp=REPORT_REPETITION_ERROR;
					osEventFlagsSet(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);
				}
				else
				{
					test_mutual_flag=1;
					production_mode_tmp = PRODUCTION_TASK_EXAMPLE_3;
					
					if(osTimerIsRunning(Test_Timer_ID))
					{
						osTimerStop(Test_Timer_ID);
					}
					osTimerStart(Test_Timer_ID, osMS2TicksRound(TEST_MOTOR_TIME*1000+500));
					m_motor_set(MOTOR_TIME_RUN,TEST_MOTOR_TIME);
				}
				//关闭sensor task
				osEventFlagsClear(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_3);
				log_debug("PRODUCTION_TASK_EXAMPLE_3 start\r\n");
				
			}
			if(os_event_flags & PRODUCTION_TASK_EXAMPLE_16)//老化测试
			{
				if(test_mutual_flag)
				{	
					error_code_tmp=REPORT_ASING_ERROR;
					osEventFlagsSet(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);
				}
				else
				{
					test_mutual_flag=1;
					production_mode_tmp = PRODUCTION_TASK_EXAMPLE_16;
					
					evt_app_adv_stop();//关闭蓝牙
					
					//挂载文件系统
					lfs_mount_safe();
					
					if(is_lfs_mounted())
					{				
						if(lfs_file_exists_and_empty("/audio/player/p1.wav") == 2)
						{
							lfs_clear_file_content("/audio/player/p1.wav");
						}
						
						log_write_with_rotation("/audio/player/p1.wav",audio_data,audio_data_size);
					//	lfs_list_dir("/");
						RepetFlagSet();//设置重�?�播�?
						Audio_Play_Start("/audio/player/p1.wav",1);
						
						asing_mode_entry_flag = 3;
						
						RestartTimer();
						
						error_code_tmp = REPORT_OK;
						osEventFlagsSet(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);
					}
					else
					{
						error_code_tmp=REPORT_ASING_ERROR;
						osEventFlagsSet(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);
					}
				}
				//关闭sensor task
				osEventFlagsClear(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_16);
				log_debug("PRODUCTION_TASK_EXAMPLE_3 start\r\n");
				
			}
			if(os_event_flags & PRODUCTION_TASK_EXAMPLE_17)
			{
				//解锁
				safe_unblock_uart_task();
				unblock_cat1_task();
				Message_Cmd_Put(TEST_TASK_ID,CAT1_UART_TASK_ID,TASK_CAT1_AWS_WITHCA_TEST,NULL,0);
				//解锁
				safe_unblock_gps_task();
				safe_unblock_gnss_uart_task();
				Message_Cmd_Put(TEST_TASK_ID,GNSS_UART_TASK_ID,TASK_PRODUCT_GPS_TEST,NULL,0);

				//清除
				osEventFlagsClear(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_17);	
				production_mode_tmp =PRODUCTION_TASK_EXAMPLE_17; 
				log_debug("PRODUCTION_TASK_EXAMPLE_17 start\r\n");
				
			}
			if(os_event_flags & PRODUCTION_TASK_EXAMPLE_4)
			{
				if(test_mutual_flag)
				{	
					error_code_tmp=REPORT_REPETITION_ERROR;
					osEventFlagsSet(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);
				}
				else
				{
					test_mutual_flag=1;
#if (AUDIO_FORMAT_DEFALT)
					uint32_t delay_time_tmp = (((uint32_t)calculate_playback_time(audio_data_size,8000,8)))*5+2;
#else
					uint32_t delay_time_tmp = (((uint32_t)calculate_playback_time(audio_data_size,16000,16)))*5+1;
#endif
					if(delay_time_tmp>30)
					{	
						error_code_tmp=REPORT_AUDIO_ERROR;
						osEventFlagsSet(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);
					}
					//挂载文件系统
					lfs_mount_safe();
					
					if(is_lfs_mounted())
					{				
						if(lfs_file_exists_and_empty("/audio/player/p1.wav") == 2)
						{
							lfs_clear_file_content("/audio/player/p1.wav");
						}
						
						log_write_with_rotation("/audio/player/p1.wav",audio_data,audio_data_size);
						lfs_list_dir("/");
						Audio_Play_Start("/audio/player/p1.wav",1);
//						Audio_IFlash_Play_Start(3);
						if(osTimerIsRunning(Test_Timer_ID))
						{	
							osTimerStop(Test_Timer_ID);
						}
						
						osTimerStart(Test_Timer_ID, osMS2TicksRound(delay_time_tmp*1000));
						production_mode_tmp = PRODUCTION_TASK_EXAMPLE_4;
					}
					else
					{
						error_code_tmp=REPORT_LFS_MOUNTED_ERROR;
						osEventFlagsSet(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);
					}
				}
				//发送获取状态和步数接口 均为默�?��?
				osEventFlagsClear(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_4);
				log_debug("PRODUCTION_TASK_EXAMPLE_4 start\r\n");
			}
			if(os_event_flags & PRODUCTION_TASK_EXAMPLE_5)//LTE测试
			{
				safe_unblock_uart_task();
				unblock_cat1_task();
				Message_Cmd_Put(TEST_TASK_ID,CAT1_UART_TASK_ID,TASK_CAT1_AWS_NO_TEST,NULL,0);
				//发送获取状态和步数接口 均为默�?��?
				osEventFlagsClear(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_5);
				production_mode_tmp =PRODUCTION_TASK_EXAMPLE_5; 
				log_debug("PRODUCTION_TASK_EXAMPLE_5 start\r\n");
			}
			if(os_event_flags & PRODUCTION_TASK_EXAMPLE_6)//GPS测试
			{
				safe_unblock_gps_task();
				safe_unblock_gnss_uart_task();
				Message_Cmd_Put(TEST_TASK_ID,GNSS_UART_TASK_ID,TASK_PRODUCT_GPS_TEST,NULL,0);
				//发送获取状态和步数接口 均为默�?��?
				osEventFlagsClear(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_6);
				production_mode_tmp =PRODUCTION_TASK_EXAMPLE_6; 
				log_debug("PRODUCTION_TASK_EXAMPLE_6 start\r\n");
			}
			if(os_event_flags & PRODUCTION_TASK_EXAMPLE_7)//sensor测试
			{
				if(test_mutual_flag)
				{
					error_code_tmp=REPORT_REPETITION_ERROR;
					osEventFlagsSet(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);
				}
				else
				{
					production_mode_tmp = PRODUCTION_TASK_EXAMPLE_7;
					test_mutual_flag=1;
					//初�?�胡sensor
					sensor_task_start();
					if(osTimerIsRunning(Test_Timer_ID))
					{
						osTimerStop(Test_Timer_ID);
					}
						
					osTimerStart(Test_Timer_ID, osMS2TicksRound(TETS_SENSOR_TIME));
				}
				//发送获取状态和步数接口 均为默�?��?
				osEventFlagsClear(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_7);
				log_debug("PRODUCTION_TASK_EXAMPLE_7 start\r\n");
			}
			if(os_event_flags & PRODUCTION_TASK_EXAMPLE_8)
			{
				log_debug("PRODUCTION_TASK_EXAMPLE_8 start = %d\r\n",test_mutual_flag);
				if(test_mutual_flag)
				{	
					error_code_tmp=REPORT_REPETITION_ERROR;
					osEventFlagsSet(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);
				}
				else
				{
					test_mutual_flag=1;
					uint32_t adc_vlue_tmp = r_PowerOn_BatteryLevel();
//					if(adc_vlue_tmp > (TEST_ADC_ELECTRIC+TEST_ADC_ELECTRIC_OFFSET) || adc_vlue_tmp < (TEST_ADC_ELECTRIC-TEST_ADC_ELECTRIC_OFFSET))
					if(adc_vlue_tmp > 0 && adc_vlue_tmp<= 1450 )
					{
						error_code_tmp=REPORT_OK;
					}
					else
					{
						error_code_tmp=REPORT_ADC_ERROR;
					}	
					osEventFlagsSet(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);
					production_mode_tmp = PRODUCTION_TASK_EXAMPLE_8;
				}

				osEventFlagsClear(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_8);
			}
			if(os_event_flags & PRODUCTION_TASK_EXAMPLE_9)
			{	
				product_flag_clear();
				lfs_unmount_safe();	
				error_code_tmp=REPORT_OK;
				production_start_flag=0;
				test_mutual_flag=0;
				Production_Result_Report(error_code_tmp);
				user_initiative_test_reboot_fun();
				production_mode_tmp =PRODUCTION_TASK_EXAMPLE_9; 
				osEventFlagsClear(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_9);
			}
			if(os_event_flags & PRODUCTION_TASK_EXAMPLE_10)
			{	
				log_debug("PRODUCTION_TASK_EXAMPLE_10 = %d\r\n",test_mutual_flag);
				osEventFlagsClear(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_10);	
			//解锁
				safe_unblock_uart_task();
				unblock_cat1_task();
			//cmd
				Message_Cmd_Put(TEST_TASK_ID,CAT1_UART_TASK_ID,TASK_CAT1_AWS_CACERT,NULL,0);
				production_mode_tmp =PRODUCTION_TASK_EXAMPLE_10; 				
			}
			if(os_event_flags & PRODUCTION_TASK_EXAMPLE_11)
			{	
				log_debug("PRODUCTION_TASK_EXAMPLE_11 = %d\r\n",test_mutual_flag);
				osEventFlagsClear(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_11);	
				production_mode_tmp =PRODUCTION_TASK_EXAMPLE_11; 
			//解锁
				safe_unblock_uart_task();
				unblock_cat1_task();
			//cmd
				Message_Cmd_Put(TEST_TASK_ID,CAT1_UART_TASK_ID,TASK_CAT1_AWS_CLIENT,NULL,0);	
			}
			if(os_event_flags & PRODUCTION_TASK_EXAMPLE_12)
			{	
				log_debug("PRODUCTION_TASK_EXAMPLE_12 = %d\r\n",test_mutual_flag);
				osEventFlagsClear(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_12);	
			//解锁
				safe_unblock_uart_task();
				unblock_cat1_task();
				production_mode_tmp =PRODUCTION_TASK_EXAMPLE_12; 
			//cmd
				Message_Cmd_Put(TEST_TASK_ID,CAT1_UART_TASK_ID,TASK_CAT1_AWS_USERKEY,NULL,0);	
			}
			if(os_event_flags & PRODUCTION_TASK_EXAMPLE_13)
			{	
				
				safe_unblock_uart_task();
				unblock_cat1_task();
				Message_Cmd_Put(TEST_TASK_ID,CAT1_UART_TASK_ID,TASK_CAT1_AWS_WITHCA_TEST,NULL,0);
				production_mode_tmp =PRODUCTION_TASK_EXAMPLE_13; 
				log_debug("PRODUCTION_TASK_EXAMPLE_13\r\n");
				osEventFlagsClear(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_13);	
			}
			if(os_event_flags & PRODUCTION_TASK_EXAMPLE_14)
			{
				safe_unblock_uart_task();
				unblock_cat1_task();
				Message_Cmd_Put(TEST_TASK_ID,CAT1_UART_TASK_ID,TASK_CAT1_UPDATA,NULL,0);
				error_code_tmp=REPORT_OK;
				osEventFlagsSet(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);
				production_mode_tmp =PRODUCTION_TASK_EXAMPLE_14; 
				log_debug("PRODUCTION_TASK_EXAMPLE_14\r\n");
				osEventFlagsClear(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_14);
			}		
			if(os_event_flags & PRODUCTION_TASK_EXAMPLE_15)
			{
				
				if(test_mutual_flag)
				{	
					error_code_tmp=REPORT_REPETITION_ERROR;
					osEventFlagsSet(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);
				}
				else
				{
					test_mutual_flag=1;
					if(usb_updata_flag_get())
					{
						safe_unblock_uart_task();
						unblock_cat1_task();
						Message_Cmd_Put(TEST_TASK_ID,CAT1_UART_TASK_ID,TASK_CAT1_USB_UPDATA,NULL,0);
						
					}
					else
					{
						Message_Cmd_Put(TEST_TASK_ID,CAT1_UART_TASK_ID,TASK_CMD_STOP,NULL,0);
					}
					error_code_tmp=REPORT_OK;
					osEventFlagsSet(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);
					log_debug("PRODUCTION_TASK_EXAMPLE_15 =%d\r\n",usb_updata_flag_get());
				}
				
				production_mode_tmp =PRODUCTION_TASK_EXAMPLE_15; 
				osEventFlagsClear(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_15);	

			}
			if(os_event_flags & PRODUCTION_TASK_EXAMPLE_REPORT)
			{
				Production_Result_Report(error_code_tmp);
			
				osEventFlagsClear(g_Production_Task_Flags, PRODUCTION_TASK_EXAMPLE_REPORT);
				
				test_mutual_flag=0;
				production_mode_tmp = 0;
				log_debug("PRODUCTION_TASK_EXAMPLE_REPORT start\r\n");
			}
			
		}
	}
#endif 
		osSemaphoreAcquire(xSemAgingTestTask_Time,osWaitForever);
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
osThreadId_t vStartTestTask(void)
{
    const osThreadAttr_t TestThreadAttr = {
        .name = "Test_Task",
        .attr_bits = 0,
        .cb_mem = NULL,
        .cb_size = 0,
        .stack_mem = NULL,
        .stack_size = TEST_TASK_STACK_SIZE,
        .priority = TEST_TASK_PRIORITY,
        .tz_module = 0,
    };

    // Create pm Task
    return osThreadNew(vTestTask, NULL, &TestThreadAttr);
}

/** @} */

// vim: fdm=marker
