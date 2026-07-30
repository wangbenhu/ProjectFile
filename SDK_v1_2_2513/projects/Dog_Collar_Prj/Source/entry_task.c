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

// Controller header
#include "obc.h"

/* Kernel includes. */
#include "cmsis_os2.h"
#include "imu_ex.h"

#include "board_define.h"
#include "common_def.h"

#include "m_motor.h"

#include "../source/drv_rng.h"
#include "nvds.h"
#include "led_task.h"
#include "my_audio.h"
#include "lfs_port.h"
#include "m_battery.h"
#include "om_device.h"

/*day task list
* 1.I/O重新更改。   pass
* 2.电池电压采集和充电检测。
* 3.电机驱动     pass
* 4.LED灯驱动
* 5.完善低功耗
* 6.系统文件的备份区操作。
* 7.文件系统的写限制。
* 8.最新硬件回板测试，的测试task。
* 9.GNSS进入低功耗前发送stop每次都会回复reply，待修复。
* 10.CAT1进入低功耗前发送stop只会第一次有reply回复。
* */
/** Notes:
模式	模式说明	            进入条件	        外设状态	        备注 	
---------------------------------------------------------------------------------------------
M1	   启动默认模式，           开机启动	        均未开启	        充电插拔均复位
       用于模式切换      
---------------------------------------------------------------------------------------------         	    	               
M2	   极低功耗模式（           1.用户未注册        1.ble处于ULTRA_     只能通过（USB充电）唤醒
       保电模式、船运、         2.电量低于10%           DEEP_SLEEP      出厂模式）低功耗模式下，检查外设IO状态 
                                                2.GPS：OFF            
                                                3.LTE：OFF
                                                4.外设关闭           
 --------------------------------------------------------------------------------------------                                                        	                                                                       	            
M3	    低电压模式（省电模式）	 1.用户已注册，       1. BLE开启正常广播
                            未充电、10<=电量<20%  2. 灯开启、6轴开启
                                                3. 其它外设关闭
                                                4. GPS:OFF
                                                5. LTE:OFF	
----------------------------------------------------------------------------------------------    	
M4	    充电模式	          检测到充电	       1.BLE：ON
                                                2.6轴开启
                                                3.LED开启
                                                3.GPS:OFF
                                                4.LTE:OFF	
----------------------------------------------------------------------------------------------    	
M5	    正常模式	          1.电量>20%          1.寻宠
                            2.用户已注册          2.待机
---------------------------------------------------------------------------------------------- 

++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            M5模式设置
-----------------------------------------------------------------------------------------------   
场景	    定义	                 前置条件	         各模块功能	       目标续航	        备注
标准	    日常使用期间，默认         寻踪：关闭           GPS：5min/次     双电池：7 天     Wifi现在不考虑 
            场景，系统功耗一 般。      GPS：开启           4G：On，被动                    （GPS的不同开启时间间隔的功耗差）
                                    4G：开启            蓝牙：On
                                    蓝牙：未连接 
                                    WiFi：未搜索 
                                    电池电量：≥20%	
-------------------------------------------------------------------------------------------------- 
运动        当外出遛狗，开启寻 踪       蓝牙：未连接         GPS：1s/次	     单电池：         需要主 动开启
（寻宠）     功 能 / 电 子 围 栏 时，   4G：开启            4G：on          双电池：连续30h
            进入运动状态，需要主动开    GPS：开启           蓝牙：On
            启实时追踪并及时上报宠物    WiFi：未搜索 
            位置，系统功耗最高。	   电池电量：≥20%       
 ---------------------------------------------------------------------------------------------------   
 
 进入GPIO31脚在默认模式下都是拉低,在进入M2的时候需要先给每个任务发送stop命令,
 等每个任务都响应stop命令后,再将GIO31拉高.
 **/
/*********************************************************************
 * MACROS
 */
#define EVENT_SYSTEM_RESERVE_MASK   0x00FF

#define AGING_TEST_REBOOT_TIME (2*60*60*1000)
#define AGING_TEST_TIME (5*60*1000)

#define DEVICE_TEST_FLAG (1)

#define AGING_TEST_REBOOT_COUNT (AGING_TEST_REBOOT_TIME/AGING_TEST_TIME)

#define DEFAULT_SN_DATA "SN202510281859"
 
#define WDT_TIMEOUT_MS (10 * 1000)  //10sec

#define MAX_TASK_NUM   20   // 系统允许的最大任务数目

#define ENTRY_TASK_PRIORITY (osPriorityNormal)//(osPriorityNormal) osPriorityAboveNormal
#define ENTRY_TASK_STACK_SIZE (4*1024)

#define PM_UPDATE_INTERVAL (1)//UNIT/SEC
/* 事件标志定义 - 每个任务占用1位 */
#define SLEEP_GNSS_READY_FLAG    (1UL << 0)
#define SLEEP_CAT1_READY_FLAG    (1UL << 1) 
#define ALL_SLEEP_ENTRY_READY     (SLEEP_GNSS_READY_FLAG | SLEEP_CAT1_READY_FLAG)


#define BATTERY_LOW_VALUE_MIN 	0	//%
#define BATTERY_LOW_VALUE_MAX 	20	//%
#define BATTERY_LOW_VALUE_FULL 	100	//%

#define CONFIG_DEBUG_DELAY_ENABLE (0)
//设置是否开启MAC死等
#define SYS_POWER_ON_MAC_WAIT_FLAG  (0)

/*********************************************************************
 * TYPEDEFS
 */

COMM_GET_STATUS_t commCurrModeStatus = {
    .currentMode = CURRENT_MODE_DEFAULT,
    .BAT_Capacity = 0,
};

/* RTOS对象句柄 */
osEventFlagsId_t g_sleepEntryReadyFlags;
//模式控制 M1/M2/M3/M4/M5
SystemMode_t FORE_MODE_STATUS =  MODE_ERROR;

//低功耗允许进入标志
pm_id_t PM_ID_ENTRY_SLEEP = PM_ID_USER + 1; 
/**
 * 系统功耗状态枚举 - 详细版
 */
typedef enum {
    // 状态0: 默认状态
    SYSTEM_POWER_STATE_DEFAULT = 0,    // 默认状态，系统正常运行
                                        // 可以响应事件，可以进入低功耗
    
    // 状态1: 满足关机条件
    SYSTEM_POWER_STATE_SHUTDOWN_READY, // 已满足关机条件
                                        // 系统正在准备关机流程
                                        // 可以执行清理操作，但不再响应新事件
    
    // 状态2: 关机状态
    SYSTEM_POWER_STATE_SHUTDOWN        // 已关机
                                        // 系统停止运行
                                        // 需要外部触发才能重新启动
} SystemPowerState_t;
/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
//static uint8_t system_run_mode = MODE_M1;
static uint8_t pm_task_timer = PM_UPDATE_INTERVAL;

static uint32_t aging_time_count = 0;

static osSemaphoreId_t xSemAgingTest = NULL;

    // 2. 定义定时器属性（通常使用默认值）
const osTimerAttr_t myTimer_attributes = {
    .name = "MyPeriodicTimer" // 给定时器起个名字，方便调试
};
static uint8_t gps_status =0;

static uint8_t low_power_off = 0;
static uint8_t s_Charge_Stop_Flag = 0;
static uint8_t entry_low_sleep_flag = SYSTEM_POWER_STATE_DEFAULT;

static uint8_t wdt_start_flag = 0;
static uint8_t device_reset_flag = 0;
static uint8_t product_entry_flag = 0; //产测进入标志
static uint8_t user_bound_flag = 0xFF;
static uint8_t cat1_del_device_flag = 0;
/*********************************************************************
 * GLOBAL VARIABLES
 */


/*********************************************************************
 * EXTERN FUNCTIONS
 */
extern osThreadId_t vStartBLEScheduleTask(void);
extern osThreadId_t vStartBleTsppTask(void);
extern osThreadId_t vStartPMTask(void);
extern osThreadId_t vStartCAT1UartTask(void);
extern osThreadId_t vStartUartDataRecvTask(void);
extern osThreadId_t vStartGNSSUartTask(void);
extern osThreadId_t vStartUartGnssDataRecvTask(void);
extern osThreadId_t vStartCommTask(void);
extern osThreadId_t vStartAudioTask(void);
extern osThreadId_t vStartAssistTask(void);
extern osThreadId_t vStartSensorTask(void);
extern osThreadId_t vStartTestTask(void);
extern osThreadId_t vStartMotorTask(void);
extern osThreadId_t vStartStackMonitorTask(void);

extern uint8_t PM_GetBatteryCapacity(void);
extern void lom_power_mode_disable(void);

extern BleState_t get_ble_status(void);

extern void set_address_init(void);

extern CHARGE_STATUS_T PM_GetChargeStatus(void);

void Entry_Control_GPS_Start_Power(void);

void Entry_Control_GPS_Stop_Power(void);
void evt_app_adv_start(void);
extern void unblock_cat1_task(void);
extern bool safe_unblock_uart_task(void);
// 使用现有的函数组合实现
//extern om_error_t wakeup_and_reconfigure(void);
extern  om_error_t drv_oflash_deep_power_down(
    OM_OSPI_Type *om_flash);

extern void sensor_task_stop(void);
extern void sensor_task_start(void);

extern void nvds_addr_save(uint8_t *addr);
extern void update_adv_data_with_mac(uint8_t *mac_addr);
extern void Production_Result_Report(uint8_t error_code);

extern void evt_app_high_speed_adv_start(void);

bool is_flash_mac_valid(void);
void ENTRY_Control_GPS_Start(void);
/*******************************************************************************
 * LOCAL FUNCTIONS
 */
/*
* @brief    set lte power status
* @details  set lte power status
* @version
* Version 1.0
*  - Initial release
*/
void PowerOffSystem(void);
//0未绑定1成功
void uset_bound_state_set(uint8_t data)
{
	user_bound_flag = data;
}
uint8_t uset_bound_state_get(void)
{
	return user_bound_flag;
}
uint8_t product_flag_get(void)
{
	return product_entry_flag;
}
void product_flag_set(void)
{
	 product_entry_flag = 1;
}
void product_flag_clear(void)
{
	 product_entry_flag = 0;
}
/**
 * 获取entry_low_sleep_flag状态
 * @return 当前状态
 */
SystemPowerState_t get_entry_low_sleep_flag(void) {
    return entry_low_sleep_flag;
}

/**
 * 设置entry_low_sleep_flag状态
 * @param state 新状态
 * @return 0-成功，其他-失败
 */
int set_entry_low_sleep_flag(SystemPowerState_t state) {
    // 验证状态值
    if (state > SYSTEM_POWER_STATE_SHUTDOWN && state < SYSTEM_POWER_STATE_DEFAULT) {
        return -1; // 无效状态
    }
    entry_low_sleep_flag = state;

    return 0;
}
//*****wdt fun start******//
/*
* @brief: 设置看门狗标志位
* @param: void
* @return: void
*/
void wdt_flag_set(void)
{
	wdt_start_flag = 1;
}
/*
* @brief: 获取看门狗标志位
* @param: void
* @return: uint8_t: 看门狗标志位
*/
uint8_t wdt_flag_get(void)
{
	return wdt_start_flag;
}
/*
* @brief: 清除看门狗标志位
* @param: void
* @return: void
*/
void wdt_flag_clear(void)
{
	wdt_start_flag = 0;
}
/*
* @brief: 初始化看门狗
* @param: void
* @return: void
*/
void m_wdt_init(void)
{
	drv_wdt_init(WDT_TIMEOUT_MS);
	wdt_flag_set();
}
/*
* @brief: 停止看门狗
* @param: void
* @return: void
*/
void m_wdt_stop(void)
{
	drv_wdt_init(0);
	wdt_flag_clear();
}
/*
* @brief: 喂狗
* @param: void
* @return: void
*/
void wdt_feed(void)
{
	if(wdt_flag_get())
	{
		drv_wdt_keep_alive();//喂狗
	}
}
//*****wdt fun end******//

int TaskInfo_InitTask(TaskInfo_t *task_info,
                      uint8_t task_id,
                      uint32_t queue_length,
                      uint32_t message_size,
                      osThreadId_t (*start_task_func)(void));

uint8_t LTE_Power_Status_Set(uint8_t status)
{
    return nvds_put(NVDS_TAG_LTE_POWER_STATUS, 1, &status);
}/*
* @brief    get lte power status
* @details  get lte power status
* @version
* Version 1.0
*  - Initial release
*/
uint8_t LTE_Power_Status_Get(uint8_t *status)
{
	return nvds_get(NVDS_TAG_LTE_POWER_STATUS,(nvds_tag_len_t *)1,status);;
}
uint8_t ltepower_test_flag=0;
/*
* @brief    test lte power status
* @details  test lte power status
* @version
* Version 1.0
*  - Initial release
*/
void ltepower_test(void)
{
	ltepower_test_flag++;
	if(ltepower_test_flag>=100)
	{
		ltepower_test_flag =0;
	}
	uint8_t tmp_data=0;
	log_debug("ltepower_test 1 = %d %d\r\n",LTE_Power_Status_Set(ltepower_test_flag),ltepower_test_flag);
	log_debug("ltepower_test 2 = %d\r\n",LTE_Power_Status_Get(&tmp_data));
	log_debug("ltepower_test 3 = %d\r\n",tmp_data);
}

SET_MODE_PARE_T SetModePare = {
    .ChargeStatus = CHARGE_STATUS_INVALID,
    .UserStatus = LFS_CREATE_FAILED,
    .BatteryLevel = BATTERY_LEVEL_ERROR,
};
/*
* @brief: 发送命令消息
* @param: source_id: 源任务ID
* @param: dest_id: 目标任务ID
* @param: command: 命令
* @param: data: 数据指针
* @param: data_length: 数据长度
* @return: uint8_t: 发送状态
*/
uint8_t Message_Cmd_Put(TASK_ID_T source_id,
                                       TASK_ID_T dest_id,
                                       TASK_CMD_T command,
                                       void *data,
                                       uint16_t data_length)
{
    uint8_t return_data = 0;
    TaskInfo_t* pTaskInfo = NULL;
    Message_t msg;
    msg.source_id   = source_id;
    msg.dest_id     = dest_id;
    msg.command     = command;
    msg.data        = data;
    msg.data_length = data_length;

	pTaskInfo = GetTaskInfo(msg.dest_id);
	// 添加队列监控
	uint32_t queue_count = osMessageQueueGetCount(pTaskInfo->queue_handle);
	uint32_t queue_capacity = osMessageQueueGetCapacity(pTaskInfo->queue_handle);
	log_debug("Message_Cmd_Put:%lu,%lu messages\r\n", queue_count, queue_capacity);
	
	if(osOK != osMessageQueuePut(pTaskInfo->queue_handle, &msg, NULL, 0))
	{
		LOG_LOC();
        return_data = 1;
	}
    return return_data;
}

/**
 * @brief 设置 pm_task_timer
 * @param value 新的定时器值
 * @return true 表示设置成功，false 表示非法值
 */
bool PM_SetTaskTimer(uint8_t value)
{
    if (value == 0 || value > BATTERY_COLLECT_INTERVAL_TIME_MAX) {
        // 假设定时器值范围是 1~100
        return false;
    }
    pm_task_timer = value;
	Message_Cmd_Put(ENTRY_TASK_ID,PM_TASK_ID,TASK_CMD_START,&pm_task_timer,1);
	
    return true;
}
/**
 * @brief 获取 pm_task_timer
 * @return 当前定时器值
 */
uint8_t PM_GetTaskTimer(void)
{
    return pm_task_timer;
}
/**
 * @brief 获取充电状态
 * @return: uint8_t: 充电状态
 */
uint8_t get_charge_status(void)
{
	uint8_t charge_status = 0;
	if(SetModePare.ChargeStatus == CHARGE_STATUS_CHARGING || SetModePare.ChargeStatus == CHARGE_STATUS_FULL)
	{
		charge_status = 1;
	}
	return charge_status;
}
/*
 * @brief: 决定系统运行模式
 * @param: SetModePare: 模式参数
 * @return: SystemMode_t: 系统运行模式
 */
SystemMode_t decide_mode(SET_MODE_PARE_T SetModePare)
{
	
   SystemMode_t mode_tmp = MODE_M1;
    if(SetModePare.ChargeStatus== CHARGE_STATUS_NO_CHARGE)//未充电
    {
        if(SetModePare.UserStatus == LFS_USER_INFO_GET_SUCCEED)
        {
            switch(SetModePare.BatteryLevel)
            {
                case BATTERY_LEVEL_EMPTY:
                    mode_tmp = MODE_M2;
                    break;
                case BATTERY_LEVEL_LOW:
                    mode_tmp = MODE_M3;
                    break;
                case BATTERY_LEVEL_MEDIUM:
                case BATTERY_LEVEL_FULL:
                    mode_tmp = MODE_M5;
                    break;
                default:
                    mode_tmp = MODE_M1;
                    break;
            }
        }
        else
        {
            mode_tmp = MODE_M2;
        }
    }
	else if(get_charge_status())
    {
        mode_tmp = MODE_M4;
    }
    else
    {
        mode_tmp = MODE_M1;  
    }  
	if(mode_tmp != FORE_MODE_STATUS)
		log_debug("decide_mode:%d,%d,%d\r\n",SetModePare.ChargeStatus,SetModePare.UserStatus,SetModePare.BatteryLevel);
 
    return mode_tmp;
}
/*
* @brief: 获取电池等级
* @param: percent: 电池百分比
* @return: BATTERY_LEVEL_T: 电池等级
*/
BATTERY_LEVEL_T get_battery_level(uint8_t percent) {
    if ( percent <= BATTERY_LOW_VALUE_MIN) {
        return BATTERY_LEVEL_EMPTY;
    } else if ((percent>=BATTERY_LOW_VALUE_MIN) && (percent <BATTERY_LOW_VALUE_MAX)) {
        return BATTERY_LEVEL_LOW;
    } else if ((percent>=BATTERY_LOW_VALUE_MAX) && (percent < BATTERY_LOW_VALUE_FULL)) {
        return BATTERY_LEVEL_MEDIUM;
    } else if (percent >= BATTERY_LOW_VALUE_FULL) {
        return BATTERY_LEVEL_FULL;
    } else {
        return BATTERY_LEVEL_ERROR;
    }
}
/*
* @brief: 上报通信模式
* @param: mode: 系统运行模式
* @return: void
*/
void COMM_MODE_REPORT(SystemMode_t mode)
{
    switch(mode)
    {
        case MODE_M1:
            commCurrModeStatus.currentMode = CURRENT_MODE_DEFAULT;
            break;
        case MODE_M2:
            commCurrModeStatus.currentMode = CURRENT_MODE_LOW_BATTERY;
            break;
        case MODE_M3:
            commCurrModeStatus.currentMode = CURRENT_MODE_LOW_BATTERY;
            break;
        case MODE_M4:
            commCurrModeStatus.currentMode = CURRENT_MODE_CHARGE;
            break;
        case MODE_M5:
			commCurrModeStatus.currentMode = CURRENT_MODE_STANDARD;
            break;
        default:
            commCurrModeStatus.currentMode = CURRENT_MODE_DEFAULT;
            break;
    }
    commCurrModeStatus.BAT_Capacity = PM_GetBatteryCapacity();
//    Message_Cmd_Put(ENTRY_TASK_ID,COMM_TASK_ID,TASK_COMM_MODE_REPORT,(COMM_GET_STATUS_t*)&commCurrModeStatus,0);
}
/*
* @brief: 获取当前模式数据
* @param: void
* @return: CURRENT_MODE_T: 当前模式数据
*/
CURRENT_MODE_T CurrentModeDataGet(void)
{
//    commCurrModeStatus.BAT_Capacity = PM_GetBatteryCapacity();
    return commCurrModeStatus.currentMode;
}
void CurrentModeDataSet(CURRENT_MODE_T mode)
{
//    commCurrModeStatus.BAT_Capacity = PM_GetBatteryCapacity();
   commCurrModeStatus.currentMode = mode;
}

/*
* @brief: 模式M1处理函数 默认不会进入该模式
* @param: void
* @return: void
*/
typedef enum {
    MODE_OP_END = 0,
    MODE_OP_MODE_REPORT,
    MODE_OP_BLE_ADV_STOP,
    MODE_OP_BLE_NORMAL_ADV_START,
    MODE_OP_BLE_HIGH_SPEED_ADV_START,
    MODE_OP_PM_STOP_MESSAGE,
    MODE_OP_PM_SET_INTERVAL,
    MODE_OP_SENSOR_SAFE_START,
    MODE_OP_SENSOR_SAFE_STOP,
    MODE_OP_SENSOR_RAW_STOP_MESSAGE,
    MODE_OP_LED_STOP_MESSAGE,
    MODE_OP_CAT1_START_MESSAGE,
    MODE_OP_CAT1_STOP_MESSAGE,
    MODE_OP_GNSS_POWER_ON,
    MODE_OP_GNSS_POWER_OFF,
    MODE_OP_GNSS_ENTER_BACKUP,
    MODE_OP_MONITOR_START,
    MODE_OP_MONITOR_STOP,
    MODE_OP_SLEEP_PREVENT,
    MODE_OP_BOARD_DEINIT_TEST,
    MODE_OP_LFS_UNMOUNT,
    MODE_OP_M2_POWER_BRANCH,
} ModeOperation_t;

typedef struct {
    ModeOperation_t operation;
    uint32_t argument;
} ModeStep_t;

static const ModeStep_t mode_m1_steps[] = {
    { MODE_OP_END, 0 },
};

static const ModeStep_t mode_m2_steps[] = {
    { MODE_OP_MODE_REPORT,      MODE_M2 },
    { MODE_OP_BLE_ADV_STOP,     0 },
    { MODE_OP_PM_STOP_MESSAGE,  0 },
    { MODE_OP_SENSOR_SAFE_STOP, 0 },
    { MODE_OP_LED_STOP_MESSAGE, 0 },
    { MODE_OP_M2_POWER_BRANCH,  0 },
    { MODE_OP_MONITOR_STOP,     0 },
    { MODE_OP_END,              0 },
};

static const ModeStep_t mode_m3_steps[] = {
    { MODE_OP_MODE_REPORT,          MODE_M3 },
    { MODE_OP_CAT1_STOP_MESSAGE,    0 },
    { MODE_OP_GNSS_POWER_OFF,       0 },
    { MODE_OP_BLE_NORMAL_ADV_START, 0 },
    { MODE_OP_PM_SET_INTERVAL,      10 },
    { MODE_OP_SENSOR_SAFE_STOP,     0 },
    { MODE_OP_BOARD_DEINIT_TEST,    0 },
    { MODE_OP_LFS_UNMOUNT,          0 },
    { MODE_OP_MONITOR_STOP,         0 },
    { MODE_OP_END,                  0 },
};

static const ModeStep_t mode_m4_steps[] = {
    { MODE_OP_MODE_REPORT,              MODE_M4 },
    { MODE_OP_CAT1_STOP_MESSAGE,        0 },
    { MODE_OP_GNSS_ENTER_BACKUP,        0 },
    { MODE_OP_BLE_HIGH_SPEED_ADV_START, 0 },
    { MODE_OP_PM_SET_INTERVAL,          10 },
    { MODE_OP_SENSOR_RAW_STOP_MESSAGE,  0 },
    { MODE_OP_MONITOR_START,            0 },
    { MODE_OP_END,                      0 },
};

static const ModeStep_t mode_m5_steps[] = {
    { MODE_OP_SLEEP_PREVENT,        0 },
    { MODE_OP_MODE_REPORT,          MODE_M5 },
    { MODE_OP_CAT1_START_MESSAGE,   0 },
    { MODE_OP_GNSS_POWER_ON,        0 },
    { MODE_OP_BLE_NORMAL_ADV_START, 0 },
    { MODE_OP_SENSOR_SAFE_START,    0 },
    { MODE_OP_PM_SET_INTERVAL,      10 },
    { MODE_OP_MONITOR_START,        0 },
    { MODE_OP_END,                  0 },
};

static void ModeManager_ExecuteOperation(const ModeStep_t *step);
static void ModeManager_ExecuteSteps(const ModeStep_t *steps);

void ModeM1Handler(void)
{
    log_debug("...Mode M1 running...\r\n");
    ModeManager_ExecuteSteps(mode_m1_steps);
}
extern void AudioHal_Drive_DeInit(void);
extern void board_deinit(void);
void all_drvice_gpio_deinit(void)
{
	//喇叭
	AudioHal_Drive_DeInit();
	//LED 
	drv_i2c_uninit(OM_I2C0);
	//sensor
	drv_spi_uninit(OM_SPI0);
	
	//uart
//	drv_uart_uninit(OM_UART0);
//	drv_uart_uninit(OM_UART1);
//	drv_uart_uninit(OM_UART2);
	//GPIO4
	board_deinit();
//	drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));
//    drv_gpio_init(gpio_config, sizeof(gpio_config) / sizeof(gpio_config[0]));
}
/*
* @brief  关闭系统
* @param  无
* @return 无
*/
void PowerOffSystem(void)
{
	int ret = 0;
	log_debug("PowerOffSystem:%d\r\n",FORE_MODE_STATUS);
    if(FORE_MODE_STATUS == MODE_M2)
    {
		//关闭看门狗
		m_wdt_stop();
		//延迟进入低功耗 需要考虑看门狗和delay的先后关系
		osDelay(1000);
	//	log_debug("***lfs_unmount_safe\r\n");
		//卸载文件系统
		ret = lfs_unmount_safe();
		  if (ret != 0) {
			log_debug("lfs_unmount_safe failed\r\n");
        /* 记录错误，不能贸然让 Flash 睡眠 */
    }
		//外部flash进入低功耗
		ret = drv_oflash_deep_power_down(OM_OSPI1);
		  if (ret != 0) {
			log_debug("drv_oflash_deep_power_down failed\r\n");
        /* 记录错误，不能贸然让 Flash 睡眠 */
    }
	/* 4. 等待 XM25QH128A 的 tDP */
		drv_dwt_delay_us(5);
//		log_debug("osEventFlagsClear******\r\n");

		//关闭所有外设的状态
		all_drvice_gpio_deinit();

        //关闭电机
        m_motor_set(MOTOR_STOP_RUN,0);
		//关闭电源
        lom_power_mode_disable();
		//允许进入低功耗
		pm_sleep_allow(PM_ID_ENTRY_SLEEP);	
		
		//osSemaphoreAcquire(xSemAgingTest, osWaitForever);
    }
    else{
        low_power_off = 1;
    }
}
/*
* @brief: 用户主动重启
* @param: void
* @return: void
*/
void user_initiative_reboot_fun(void)
{
	log_debug(" user_initiative_reboot_fun !!\r\n");
	osDelay(100);//task引用
	drv_pmu_reset(PMU_REBOOT_FROM_SOFT_RESET_USER);
	while(1);
}
/*
* @brief: 用户主动重启生产测试模式使用
* @param: void
* @return: void
*/
void user_initiative_test_reboot_fun(void)
{
	log_debug("user_initiative_test_reboot_fun !!\r\n");
	osDelay(5000);//task引用
	drv_pmu_reset(PMU_REBOOT_FROM_SOFT_RESET_USER);
	while(1);
}
/*
* @brief: 模式M2处理函数 
    开启低功耗sleep模式 ultra_deepsleep模式，设置唤醒源头，CAT1，GNSS，BLE，PM，LED,sensor，音频任务发送stop，，收到
* @param: void
* @return: void
*/extern void evt_app_adv_stop(void);
void ModeM2Handler(void)
{
    log_debug("...Mode M2 running...:%d\r\n", low_power_off);
    ModeManager_ExecuteSteps(mode_m2_steps);
}
/*
* @brief: 模式M3处理函数
* @param: void
* @return: void
*/

static const gpio_config_t gpio_config_deinit_test[] = {
    // 六轴
    {OM_GPIO1, SPI_CS, 		GPIO_DIR_OUTPUT, GPIO_LEVEL_LOW, GPIO_TRIG_NONE},
    {OM_GPIO1, SPI_SCK, 	GPIO_DIR_OUTPUT, GPIO_LEVEL_LOW, GPIO_TRIG_NONE},
    {OM_GPIO1, SPI_MOSI, 	GPIO_DIR_OUTPUT, GPIO_LEVEL_LOW, GPIO_TRIG_NONE},
    {OM_GPIO1, SPI_MISO, 	GPIO_DIR_OUTPUT, GPIO_LEVEL_LOW, GPIO_TRIG_NONE},
    {OM_GPIO0, IMU_INT1, 	GPIO_DIR_OUTPUT, GPIO_LEVEL_LOW, GPIO_TRIG_NONE},
//	 {OM_GPIO0, LED_SCL_IO,    GPIO_DIR_INPUT, GPIO_LEVEL_LOW, GPIO_TRIG_NONE},
//    {OM_GPIO0, LED_SDA_IO,    GPIO_DIR_INPUT, GPIO_LEVEL_LOW, GPIO_TRIG_NONE},
};
static const pin_config_t pin_config_deinit_test[] = {
//	{LED_SCL_IO, {PINMUX_GPIO_MODE_CFG}, PMU_PIN_MODE_FLOAT, PMU_PIN_DRIVER_CURRENT_NORMAL},
//	{LED_SDA_IO, {PINMUX_GPIO_MODE_CFG}, PMU_PIN_MODE_FLOAT, PMU_PIN_DRIVER_CURRENT_NORMAL},
	{SPI_CS,  				{PINMUX_GPIO_MODE_CFG},  	PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {SPI_SCK, 				{PINMUX_GPIO_MODE_CFG}, 	PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {SPI_MOSI,  			{PINMUX_GPIO_MODE_CFG},		PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {SPI_MISO, 				{PINMUX_GPIO_MODE_CFG},	 	PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
	{IMU_INT1,  			{PINMUX_GPIO_MODE_CFG}, 	PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
};
void board_deinit_test(void)
{
	drv_pin_init(pin_config_deinit_test, sizeof(pin_config_deinit_test) / sizeof(pin_config_deinit_test[0]));
	drv_gpio_init(gpio_config_deinit_test, sizeof(gpio_config_deinit_test) / sizeof(gpio_config_deinit_test[0]));
}
static void ModeManager_ExecuteOperation(const ModeStep_t *step)
{
    switch (step->operation)
    {
        case MODE_OP_MODE_REPORT:
            COMM_MODE_REPORT((SystemMode_t)step->argument);
            break;
        case MODE_OP_BLE_ADV_STOP:
            evt_app_adv_stop();
            break;
        case MODE_OP_BLE_NORMAL_ADV_START:
            evt_app_adv_start();
            break;
        case MODE_OP_BLE_HIGH_SPEED_ADV_START:
            evt_app_high_speed_adv_start();
            break;
        case MODE_OP_PM_STOP_MESSAGE:
            Message_Cmd_Put(ENTRY_TASK_ID, PM_TASK_ID, TASK_CMD_STOP, NULL, 0);
            break;
        case MODE_OP_PM_SET_INTERVAL:
            PM_SetTaskTimer((uint8_t)step->argument);
            break;
        case MODE_OP_SENSOR_SAFE_START:
            sensor_task_start();
            break;
        case MODE_OP_SENSOR_SAFE_STOP:
            sensor_task_stop();
            break;
        case MODE_OP_SENSOR_RAW_STOP_MESSAGE:
            Message_Cmd_Put(ENTRY_TASK_ID, SENSOR_TASK_ID, TASK_CMD_STOP, NULL, 0);
            break;
        case MODE_OP_LED_STOP_MESSAGE:
            Message_Cmd_Put(ENTRY_TASK_ID, LED_TASK_ID, TASK_CMD_STOP, NULL, 0);
            break;
        case MODE_OP_CAT1_START_MESSAGE:
            Message_Cmd_Put(ENTRY_TASK_ID, CAT1_UART_TASK_ID, TASK_CMD_START, NULL, 0);
            break;
        case MODE_OP_CAT1_STOP_MESSAGE:
            Message_Cmd_Put(ENTRY_TASK_ID, CAT1_UART_TASK_ID, TASK_CMD_STOP, NULL, 0);
            break;
        case MODE_OP_GNSS_POWER_ON:
            Entry_Control_GPS_Start_Power();
            break;
        case MODE_OP_GNSS_POWER_OFF:
            Entry_Control_GPS_Stop_Power();
            break;
        case MODE_OP_GNSS_ENTER_BACKUP:
            ENTRY_Control_GPS_Start();
            break;
        case MODE_OP_MONITOR_START:
            Message_Cmd_Put(ENTRY_TASK_ID, COMM_TASK_ID, TASK_STATE_MONITOR_START, NULL, 0);
            break;
        case MODE_OP_MONITOR_STOP:
            Message_Cmd_Put(ENTRY_TASK_ID, COMM_TASK_ID, TASK_STATE_MONITOR_STOP, NULL, 0);
            break;
        case MODE_OP_SLEEP_PREVENT:
            pm_sleep_prevent(PM_ID_ENTRY_SLEEP);
            break;
        case MODE_OP_BOARD_DEINIT_TEST:
            board_deinit_test();
            break;
        case MODE_OP_LFS_UNMOUNT:
            lfs_unmount_safe();
            break;
        case MODE_OP_M2_POWER_BRANCH:
            if (low_power_off != 1)
            {
                Message_Cmd_Put(ENTRY_TASK_ID,
                                CAT1_UART_TASK_ID,
                                TASK_CMD_STOP,
                                NULL,
                                0);
                Entry_Control_GPS_Stop_Power();
            }
            else
            {
                PowerOffSystem();
            }
            break;
        case MODE_OP_END:
        default:
            break;
    }
}

static void ModeManager_ExecuteSteps(const ModeStep_t *steps)
{
    uint32_t index = 0;

    while (steps[index].operation != MODE_OP_END)
    {
        ModeManager_ExecuteOperation(&steps[index]);
        index++;
    }
}

void ModeM3Handler(void)
{
    log_debug("...Mode M3 running...:%d\r\n", PM_GetBatteryCapacity());
    ModeManager_ExecuteSteps(mode_m3_steps);
}
/*
* @brief: 获取FreeRTOS随机数
* @param: void
* @return: uint32_t
*/
int get_freertos_random_5_to_30(void) {
    // 使用FreeRTOS的滴答计数作为随机源
    TickType_t ticks = xTaskGetTickCount();
    
    // 简单的伪随机算法
    static uint32_t state = 0;
    state = (state * 1103515245UL + 12345UL) & 0x7FFFFFFF;
    
    // 结合滴答计数增加随机性
    uint32_t combined = state ^ (uint32_t)ticks;
    
    return (combined % 26) + 5;
}
/*
* @brief: 获取质量随机数
* @param: void
* @return: uint32_t
*/
int get_quality_random_binary(void) {
    static uint32_t state = 0;
    
    // 使用Xorshift算法，随机性更好
    if (state == 0) {
        state = xTaskGetTickCount() ^ (uint32_t)&state;
    }
    
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    
    return state & 1;
}
void m4_to_production_config(void)
{
	log_debug("m4_to_production_config = %d %d\r\n",FORE_MODE_STATUS,entry_low_sleep_flag);
	if(FORE_MODE_STATUS == MODE_M4)
	{
		SystemPowerState_t entry_low_flag = get_entry_low_sleep_flag();
		
		if(entry_low_flag == SYSTEM_POWER_STATE_SHUTDOWN_READY)
		{
			Message_Cmd_Put(ENTRY_TASK_ID,TEST_TASK_ID,TASK_CMD_START,NULL,0);
			led_force_stop_all();
			set_entry_low_sleep_flag(SYSTEM_POWER_STATE_SHUTDOWN);
		}
		else if(entry_low_flag == SYSTEM_POWER_STATE_SHUTDOWN)
		{
			log_debug("SYSTEM_POWER_STATE_SHUTDOWN\r\n");
		}
		else
		{
			//关闭看门狗
			m_wdt_stop();
			//初始化TEST_TASK_ID任务
			TaskInfo_InitTask(task_info,TEST_TASK_ID,QUEUE_LENGTH,MESSAGE_SIZE,vStartTestTask);
			//Message_Cmd_Put(ENTRY_TASK_ID,TEST_TASK_ID,TASK_CMD_START,NULL,0);
			set_entry_low_sleep_flag(SYSTEM_POWER_STATE_SHUTDOWN_READY);
			product_flag_set();	
		}
	}
}
/*
* @brief: 模式M4处理函数
* @param: void
* @return: void
*/
void ModeM4Handler(void)
{
    log_debug("...Mode M4 running...:%d\r\n", littlefs_create_flag_get());
    drv_pmu_retention_reg2_set(0);
    ModeManager_ExecuteSteps(mode_m4_steps);
}

/*
* @brief: 模式M5处理函数
* @param: void
* @return: void
*/
void ModeM5Handler(void)
{
    log_debug("...Mode M5 running...\r\n");
    ModeManager_ExecuteSteps(mode_m5_steps);
}



void ModeErrorHandler(void) {
    log_debug("...Mode ERROR running...\r\n");
}

BAT_STATUS_t  GetBatStatus(void)
{
    BAT_STATUS_t bat_status_tmp;
    bat_status_tmp.charge_status = SetModePare.ChargeStatus;
    bat_status_tmp.BAT_Capacity = PM_GetBatteryCapacity();
    return bat_status_tmp;
}

// 切换模式
void TaskManager_SetMode(SET_MODE_PARE_T mode_pare) {
//	log_debug("TaskManager_SetMode = %d\r\n",mode_pare.ChargeStatus);
    SystemMode_t mode_tmp = decide_mode(mode_pare);

    // 根据模式开启任务
    if(mode_tmp == FORE_MODE_STATUS)//保证只进入一次
    {
        return;
    } 
	
/*************2505121102 start**************
·FixID：2505121102 
·因为电池部分做了电量只会下降不会抬升的处理，所以理论上模式切换只会从M5->M3->M2没有考虑到M2->M3->M5情况，
	所以添加该部分逻辑，只要是异常情况发生就触发重启逻辑，保证当前状态的可控性。（另外电池部分的电量逻辑
	也需要优化）
*/
	//避免模式反复
	if(mode_tmp == MODE_M5)
	{
		if((FORE_MODE_STATUS == MODE_M2) || (FORE_MODE_STATUS == MODE_M3))
		{
			user_initiative_reboot_fun();
		}
	}
	else if(mode_tmp == MODE_M3)
	{
		if(FORE_MODE_STATUS == MODE_M2)
		{
			user_initiative_reboot_fun();
		}
	}
	else
	{
		
	}
/*************2505121102 end***************/
	FORE_MODE_STATUS = mode_tmp;
    switch (mode_tmp) {
        case MODE_M1:
		commCurrModeStatus.currentMode = CURRENT_MODE_DEFAULT;
           ModeM1Handler();
            break;
        case MODE_M2:
           ModeM2Handler();
            break;
        case MODE_M3:
            ModeM3Handler();
            break;
        case MODE_M4:
            ModeM4Handler();
            break;
        case MODE_M5:
            ModeM5Handler();
            break;
        default:
			commCurrModeStatus.currentMode = CURRENT_MODE_DEFAULT;
            ModeErrorHandler();
            break;
    }
   
	log_debug("mode %d charge %d user %d battery %d\r\n", mode_tmp,mode_pare.ChargeStatus,mode_pare.UserStatus,mode_pare.BatteryLevel);
}

/*
* @brief: 设置充电状态
* @param: pare: 模式参数
* @param: charge: 充电状态
* @return: bool: 是否设置成功
*/
bool SetModePare_SetCharge(CHARGE_STATUS_T charge)
{
    if((charge < CHARGE_STATUS_NO_CHARGE) || (charge > CHARGE_STATUS_INVALID))
        return false;
    SetModePare.ChargeStatus = charge;
	log_debug("SetModePare.ChargeStatus = %d\r\n",SetModePare.ChargeStatus);
    return true;
}
/*
* @brief: 设置用户状态
* @param: pare: 模式参数
* @param: user: 用户状态
* @return: bool: 是否设置成功
*/
bool SetModePare_SetUser(ReadLfsCreatFlag user)
{
    if ((user < LFS_CREATE_SUCCEED) || (user > LFS_USER_INFO_GET_FAILED))
        return false;
    SetModePare.UserStatus = user;
    return true;
}
/*
* @brief: shell测试设置电池电量
* @param: pare: 模式参数
* @param: battery: 电池电量
* @return: bool: 是否设置成功
*/
bool SetModePare_SetBattery(uint8_t battery)
{
    if (battery > BATTERY_LOW_VALUE_FULL)
        return false;
    SetModePare.BatteryLevel = get_battery_level(battery);
    return true;
}
/*
* @brief: 获取用户激活状态
* @param: void
* @return: USER_STATUS_T: 用户激活状态
*/
ReadLfsCreatFlag GetUser_ActivatedState(void)
{
    ReadLfsCreatFlag user_activated = littlefs_create_flag_get();
	//获取鉴权码是否又
    return user_activated;
}
/*
* @brief: 检查用户信息是否存在
* @param: void
* @return: bool: 用户信息是否存在
*/
bool isUserInfoExisting(void)
{
	if(strlen(user_bindinfo_get(UBI_AUTH_CODE_ID))<=0)
	{
		return false;
	}
	else
	{
		return true;
	}
    
}
/*
* @brief: 检查是否在生产制造模式
* @param: void
* @return: bool: 
*/
bool isProductionMake(void)
{
	ReadLfsCreatFlag production_mode = littlefs_create_flag_get();
	
	log_debug("isProductionMake:%d,%d\r\n",decide_mode(SetModePare),production_mode);
	if(decide_mode(SetModePare)==MODE_M4)
	{
		if((production_mode == LFS_SN_GET_FAILED) || (production_mode == LFS_CREATE_FAILED))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	return false;
}

void show_all_tasks(void)
{
    TaskStatus_t *pxTaskStatusArray;
    volatile UBaseType_t uxArraySize, x;
    uint32_t ulTotalRunTime;

    uxArraySize = uxTaskGetNumberOfTasks();
    pxTaskStatusArray = pvPortMalloc( uxArraySize * sizeof(TaskStatus_t) );

    if(pxTaskStatusArray != NULL)
    {
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);

        for(x = 0; x < uxArraySize; x++)
        {
            log_debug("Task: %s\tState: %u\tPriority: %u\tStack: %u\tNum: %u\r\n",
                   pxTaskStatusArray[x].pcTaskName,
                   pxTaskStatusArray[x].eCurrentState,
                   pxTaskStatusArray[x].uxCurrentPriority,
                   pxTaskStatusArray[x].usStackHighWaterMark,
                   (unsigned int)pxTaskStatusArray[x].xTaskNumber);
        }
        vPortFree(pxTaskStatusArray);
    }
}

/**
 * @brief  初始化 task_info 数组中的 task_id 字段
 * @param  task_info    任务信息数组指针
 * @param  entry_id     起始任务 ID（通常是 ENTRY_TASK_ID）
 * @param  end_id       结束任务 ID（通常是 END_TASK_ID）
 */
void TaskInfo_InitIDs(TaskInfo_t *task_info, uint8_t entry_id, uint8_t end_id)
{
    for (uint8_t i = entry_id; i < end_id; i++) {
        task_info[i].task_id = i;
    }
       // 创建Entry任务的消息队列
    task_info[ENTRY_TASK_ID].queue_handle = osMessageQueueNew(QUEUE_LENGTH,MESSAGE_SIZE,NULL);
    if(task_info[ENTRY_TASK_ID].queue_handle == NULL)
        LOG_LOC();
}
/**
 * @brief  为指定任务创建消息队列并启动任务
 * @param  task_info       任务信息数组指针
 * @param  task_id         任务 ID
 * @param  queue_length    队列长度
 * @param  message_size    消息大小
 * @param  start_task_func 任务启动函数（返回 osThreadId_t 任务句柄）
 * @return 0 成功，非 0 失败
 */
int TaskInfo_InitTask(TaskInfo_t *task_info,
                      uint8_t task_id,
                      uint32_t queue_length,
                      uint32_t message_size,
                      osThreadId_t (*start_task_func)(void))
{
    // 1. 创建消息队列
    task_info[task_id].queue_handle = osMessageQueueNew(queue_length,
                                                        message_size,
                                                        NULL);
    if (task_info[task_id].queue_handle == NULL) {
        LOG_LOC(); // 自定义错误处理
        return -1;
    }

    // 2. 启动任务
    task_info[task_id].task_handle = start_task_func();
    if (task_info[task_id].task_handle == NULL) {
       LOG_LOC();
        return -2;
    }

    // 3. 填充 task_id 字段
    task_info[task_id].task_id = task_id;

    return 0; // 成功
}

extern void PM_Charge_Battery_Init(void);
/*
* 其它外设初始化
* */

void peripheral_other_init(void)
{
    // 防止进入低功耗模式
	//pm_sleep_prevent(PM_ID_ENTRY_SLEEP);
	//初始化看门狗
	m_wdt_init();
	//时间戳初始化
	lfs_timestamp_init();
	
	lfs_mount_safe();
	
//	 lfs_init();

	//电机初始化
	m_motor_init();
	//设置蓝牙随机MAC
	
	if(is_flash_mac_valid() == false)
	{
		set_address_init();
	}
	//LED初始化
	led_init();
	
	//sensor初始化
	
	//音频初始化
	
      /* 创建事件标志组 用于GPIO31关闭电源逻辑*/
    g_sleepEntryReadyFlags = osEventFlagsNew(NULL);
    if (g_sleepEntryReadyFlags == NULL) {
        log_debug("Failed to create event flags!\r\n");
        return;
    }

	//USB cdc功能初始化
	//usbd_cdc_acm_init();
	PM_Charge_Battery_Init();
//	enTestModeEventFlagsId = osEventFlagsNew(NULL);
}

//进入backup模式
void ENTRY_Control_GPS_Start(void)
{
	 log_debug("ENTRY_Control_GPS_Start\r\n");
	Message_Cmd_Put(ENTRY_TASK_ID,GNSS_UART_TASK_ID,TASK_GPS_STOP,NULL,0);
}
//GNSS开机
void Entry_Control_GPS_Start_Power(void)
{
	log_debug("Entry_Control_GPS_Start_Power\r\n");
	Message_Cmd_Put(ENTRY_TASK_ID,GNSS_UART_TASK_ID,TASK_CMD_START,NULL,0);
}
//GNSS关机
void Entry_Control_GPS_Stop_Power(void)
{
	Message_Cmd_Put(ENTRY_TASK_ID,GNSS_UART_TASK_ID,TASK_CMD_STOP,NULL,0);
}


/* 获取 OS 任务信息 */
void get_task_state(void) {
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

//拔掉充电器后处理
void Entry_Control_Stop_Power_Charge(void)
{
	lfs_unmount_safe();//卸载文件系统
	user_initiative_reboot_fun();
}
//拔掉充电器状态更新
void Entry_Control_Stop_Power_Charge_FlagSet(void)
{
	s_Charge_Stop_Flag = 1;
    osSemaphoreRelease(xSemAgingTest);//释放entry 退出阻塞
}
//拔掉充电状态获取
uint8_t Entry_Control_Stop_Power_Charge_FlagGet(void)
{
	return s_Charge_Stop_Flag;
}

/*
*用户解除绑定
*/
void User_Unbind_Fun(void)
{
	user_bindinfo_delete_field_by_id(UBI_ALL_DATA);
	//user_bindinfo_del(UBI_ALL_DATA);
	littlefs_create_flag_set(LFS_USER_INFO_GET_FAILED);
	SetModePare_SetUser(GetUser_ActivatedState());
	TaskManager_SetMode(SetModePare);
}
/*
* 返回ENTRY的功耗模式
*/
SystemMode_t Entry_Task_Run_Mode_Get(void)
{
	return FORE_MODE_STATUS;
}
/*
*恢复出厂设置
*/
void Factory_Data_Reset(void)
{
	//格式刷文件系统
	lfs_format_safe();
	log_debug("device_reset_flag = %d\r\n",device_reset_flag);
	if(device_reset_flag)
	{
		user_initiative_reboot_fun();
	}
	else
	{
		device_reset_flag=1;
		//延时5s
		osDelay(osMS2TicksRound(5000));
			//关闭CAT1_UART_TASK_ID任务
		Message_Cmd_Put(ENTRY_TASK_ID,CAT1_UART_TASK_ID,TASK_CMD_STOP,NULL,0);
	}
}

/**
 * 检查Flash中是否存在有效的MAC地址
 * @param mac_addr 存储MAC地址的缓冲区（6字节）
 * @param flash_addr Flash中存储MAC的地址
 * @return true: MAC有效 false: MAC无效
 */
bool is_flash_mac_valid(void)
{
    // 方法1: 检查是否为全0或全F（未编程状态）
    bool all_zero = true;
    bool all_ff = true;
	bool char_error = true;
     uint8_t local_addr[6] = {0};
	   nvds_tag_len_t len = 6;
	 //说明NVDS无地址
    if(NVDS_OK != nvds_get(NVDS_TAG_BD_ADDRESS, &len, local_addr)) 
    {
         return false;  // 组播地址无效
    }
	else
	{
		for (int i = 0; i < 6; i++) {
			if (local_addr[i] != 0x00) {
				all_zero = false;
			}
			if (local_addr[i] != 0xFF) {
				all_ff = false;
			}
		
		}
		if (all_zero || all_ff) {
			return false;  // 全0或全F表示无效
		}
		log_debug("productAddress: %02X:%02X:%02X:%02X:%02X:%02X\r\n", 
		  local_addr[5], local_addr[4], local_addr[3], 
		  local_addr[2], local_addr[1], local_addr[0]);
		update_adv_data_with_mac(local_addr);
		nvds_addr_save(local_addr);
		return true;  // 通过所有检查，MAC有效
	}
}

/**
 * @brief LED控制
 * @param 
 */
void m_led_user_handler(void)
{
   //LED状态
	if(FORE_MODE_STATUS == MODE_M4)
	{
		if(get_ble_status() == BLE_STATE_CONNECTED)//连接灭红灯闪烁
		{
			red_led_status_reset();
		//	white_led_status_reset();
		}
		else//非连接亮红灯闪烁
		{
			//20260310修改将蓝牙未连接时候的闪烁改为常量，修改前2s闪烁
//				red_led_start(LED_STATE_PAIRING);//闪烁
			//修改后常量20260415需求更改
//			red_led_start(R_LED_STATE_FINDING_DEVICE);//常量
	//		//修改后常量20260417需求更改增加用户绑定取消灯亮
			//log_debug("uset_bound_state_get() = %d %d\r\n",uset_bound_state_get(),GetUser_ActivatedState());
			if(uset_bound_state_get() == 0xFF)
			{
				uint8_t user_get_flag=GetUser_ActivatedState();
				if(user_get_flag == LFS_USER_INFO_GET_FAILED ||user_get_flag == LFS_CREATE_FAILED)
				{
					red_led_start(R_LED_STATE_FINDING_DEVICE);
				}
			}
			else if(uset_bound_state_get() == 0)
			{
				red_led_start(R_LED_STATE_FINDING_DEVICE);
			}
			else
			{
				
			}
		}
			
		if(PM_GetBatteryCapacity() == 100)//满电白色全亮
		{
			white_led_start(W_LED_STATE_FULLY_CHARGED,0);
			
		}
		else//否则点亮显示呼吸
		{
			white_led_start(W_LED_STATE_CHARGING_ALL,PM_GetBatteryCapacity());
		}
	}
	
	//注释了M3模式的低电量显示和M3之后的寻宠模式灯效关闭，在M3进入接口统一关闭所有LED
	else{
		
		uint8_t battery_tmp = PM_GetBatteryCapacity();
//		if(battery_tmp> BATTERY_LOW_VALUE_MAX)//工作指示灯
//		{
//			white_led_start(W_LED_STATE_RUNING,0);
//		}
//		else
//		{
//			white_led_status_reset();
//		}		
		if(( battery_tmp> BATTERY_LOW_VALUE_MIN) && (battery_tmp < BATTERY_LOW_VALUE_MAX))
		{	
			red_led_start(R_LED_STATE_LOW_POWER);	//低电量提示
			white_led_status_reset();
		}
		else
		{
			if(CurrentModeDataGet()==CURRENT_MODE_SEARCH_PET)
			{
				white_led_start(W_LED_STATE_FINDING,0);
				//red_led_start(R_LED_STATE_FINDING_PET);//寻宠模式灯
			}
			else if(CURRENT_MODE_STANDARD)
			{
				white_led_status_reset();
				//red_led_status_reset();
			}
			else
			{
				
			}
		}
	}    
}
/**
 * @brief 开机MAC检查，如果没有则等待设置
 * @return void
 */
void boot_mac_check_and_wait(void)
{
	if(is_flash_mac_valid() == false)
	{
		while(1)
		{
			char send_no_mac[]="NO_MAC!!!\r\n";
			drv_uart_write(OM_UART0,(uint8_t *)send_no_mac,strlen(send_no_mac),10);
			log_debug("NO_MAC!!!\r\n");
			osDelay(osMS2TicksRound(500));
			wdt_feed();//喂狗
		}
	}
}



void Entry_Input_Init(void)
{
	 // 防止进入低功耗模式
	pm_sleep_prevent(PM_ID_ENTRY_SLEEP);

    //其它外设初始化
	peripheral_other_init();
	
#if (SYS_POWER_ON_MAC_WAIT_FLAG)
	boot_mac_check_and_wait();//识别MAC无是否等待
#endif
	
	#if (CONFIG_DEBUG_DELAY_ENABLE)
    osDelay(osMS2TicksRound(3000));
#endif
    log_debug("drv_pmu_reboot_reason = %d\r\n", drv_pmu_reboot_reason());
    //设置用户状态
	uset_bound_state_set(0xFF);
    SetModePare_SetUser(GetUser_ActivatedState());
	SetModePare_SetBattery(PM_GetBatteryCapacity());//设置电量
	SetModePare_SetCharge(Get_ChargeIO_Status()); //设置充电状态
	/* 测试 实际需要获取充电状态和读取电量*/
	cat1_del_device_flag = 0;
	
}

void Entry_Input_Task_Init(void)
{
    //初始化任务ID和entry task
    TaskInfo_InitIDs(task_info,ENTRY_TASK_ID,END_TASK_ID);
    //初始化BLE_SCHEDULE_TASK_ID任务
    TaskInfo_InitTask(task_info,BLE_SCHEDULE_TASK_ID,QUEUE_LENGTH,MESSAGE_SIZE,vStartBLEScheduleTask);

    //初始化PM_TASK_ID任务
    TaskInfo_InitTask(task_info,PM_TASK_ID,QUEUE_LENGTH,MESSAGE_SIZE,vStartPMTask);
    //初始化CAT1_UART_TASK_ID任务
	TaskInfo_InitTask(task_info,CAT1_UART_TASK_ID,QUEUE_LENGTH,MESSAGE_SIZE,vStartCAT1UartTask);
    //初始化UART_DATARECV_ID任务
	TaskInfo_InitTask(task_info,UART_DATARECV_ID,QUEUE_LENGTH,MESSAGE_SIZE,vStartUartDataRecvTask);
    //初始化GNSS_UART_TASK_ID任务
	TaskInfo_InitTask(task_info,GNSS_UART_TASK_ID,QUEUE_LENGTH,MESSAGE_SIZE,vStartGNSSUartTask);
	//初始化GNSS_UART_TASK_ID任务
	TaskInfo_InitTask(task_info,UART_GNSSDATARECV_ID,QUEUE_LENGTH,MESSAGE_SIZE,vStartUartGnssDataRecvTask);
    //初始化COMM_TASK_ID任务
    TaskInfo_InitTask(task_info,COMM_TASK_ID,QUEUE_LENGTH,MESSAGE_SIZE,vStartCommTask);
    //初始化AUDIO_TASK_ID任务
    TaskInfo_InitTask(task_info,AUDIO_TASK_ID,QUEUE_LENGTH,MESSAGE_SIZE,vStartAudioTask);
    //初始化ASSIST_TASK_ID任务
	//TaskInfo_InitTask(task_info,ASSIST_TASK_ID,QUEUE_LENGTH,MESSAGE_SIZE,vStartAssistTask);
    //初始化SENSOR_TASK_ID任务
    TaskInfo_InitTask(task_info,SENSOR_TASK_ID,QUEUE_LENGTH,MESSAGE_SIZE,vStartSensorTask);
    //初始化TEST_TASK_ID任务
    TaskInfo_InitTask(task_info,LED_TASK_ID,QUEUE_LENGTH,MESSAGE_SIZE,vStartLedTask); 
    //初始化MOTOR_TASK_ID任务
    TaskInfo_InitTask(task_info,MOTOR_TASK_ID,QUEUE_LENGTH,MESSAGE_SIZE,vStartMotorTask);
	
	TaskInfo_InitTask(task_info,STACK_TASK_ID,QUEUE_LENGTH,MESSAGE_SIZE,vStartStackMonitorTask);
  
	 //获取当前初始化的全部task信息
	get_task_state();
}

/**
 * @brief Handle message queue for entry task
 * @param pEntryTaskInfo Pointer to entry task information
 * @param received_msg Pointer to message structure for receiving messages
 * @return osStatus_t Status of message queue operation
 */
static osStatus_t EntryTask_HandleMessageQueue(TaskInfo_t* pEntryTaskInfo, Message_t* received_msg)
{
    osStatus_t status = osMessageQueueGet(pEntryTaskInfo->queue_handle, received_msg, NULL, 500);
    if(osOK == status)
    {
        log_debug("EntryTask_HandleMessageQueue:%d,%lu\r\n", received_msg->source_id, received_msg->command);
        if((received_msg->command == TASK_REPORT_MOTION_LEVEL) && (received_msg->source_id == SENSOR_TASK_ID))
        {
			log_debug("%s report sensor motion level: %d\r\n",__func__, get_motion_level());
        }
                
        if(received_msg->command == TASK_INFO_CHARGE)
        {
            BAT_STATUS_t tmp;
			tmp.BAT_Capacity=((BAT_STATUS_t*)received_msg->data)->BAT_Capacity;
			tmp.charge_status=((BAT_STATUS_t*)received_msg->data)->charge_status;
			log_debug("TASK_INFO_CHARGE :%d,%d\r\n",tmp.BAT_Capacity,tmp.charge_status);
            SetModePare_SetCharge(tmp.charge_status);
            TaskManager_SetMode(SetModePare);
            
        }
        if(received_msg->source_id == COMM_TASK_ID)
        {
            if(received_msg->command == TASK_TEST_START)
            {
                //正常M4模式进入生产制造模式
                m4_to_production_config();
            }
        }
        if(received_msg->source_id == GNSS_UART_TASK_ID)
        {
            if(received_msg->command == TASK_STOP_REPLY)//GNSS进入backup和关机时候进入
            {
                //处理回复
                /* 设置就绪标志 */
                osEventFlagsSet(g_sleepEntryReadyFlags, SLEEP_GNSS_READY_FLAG);
            }
        }
        
        if(received_msg->source_id == CAT1_UART_TASK_ID)
        {
            if(received_msg->command == TASK_STOP_REPLY)
            {
				if(FORE_MODE_STATUS == MODE_M4)
				{
					log_debug("received_msg->command == TASK_STOP_REPLY\r\n");
					safe_unblock_uart_task();
					unblock_cat1_task();
					 Message_Cmd_Put(ENTRY_TASK_ID,CAT1_UART_TASK_ID,TASK_CMD_START,NULL,0);
				}
				//处理回复
				if(device_reset_flag)
				{
					osEventFlagsSet(g_sleepEntryReadyFlags, SLEEP_GNSS_READY_FLAG);
				}
				/* 设置就绪标志 */
				osEventFlagsSet(g_sleepEntryReadyFlags, SLEEP_CAT1_READY_FLAG);
            }
            if(received_msg->command == TASK_FACTORY_RESET_REPLY)
            {
                device_reset_flag=0;
                Factory_Data_Reset();
            }
			if(received_msg->command == TASK_CAT1_DELETE_DEVICE)
			{
				cat1_del_device_flag=1;
			}
        }
    }
    return status;
}
static void vEntryTask(void *argument)
{
    uint32_t uxBits = 0;
	TaskInfo_t* pEntryTaskInfo = GetTaskInfo(ENTRY_TASK_ID);
	
    const uint32_t xTicksToWait = 0;//0: return immediately//osMS2TicksRound( 50 );
    osStatus_t status = osOK;
    const uint32_t xTicksToTaskDelay = osMS2TicksRound( 5 );

    Message_t msg_to_send;
    Message_t received_msg;
  //初始化参数和外设
	Entry_Input_Init();
	//初始化task
	Entry_Input_Task_Init();
	//初始化用于低功耗阻塞TASK的信号量
	if (xSemAgingTest == NULL) {
		xSemAgingTest = osSemaphoreNew(1, 0, NULL);
    } else {
        log_debug("Semaphore already exists, skip creation\r\n");
    }
	if(get_charge_status())
	{
		m_motor_set(MOTOR_TIME_RUN,1);
	}
    while(1)
    {
        if(Entry_Control_Stop_Power_Charge_FlagGet() == 1){
            //拔掉充电器后处理
            Entry_Control_Stop_Power_Charge();
        }
//更新电量百分比
		uint8_t user_flag_tmp = GetUser_ActivatedState();
		uint8_t bat_cap_tmp = PM_GetBatteryCapacity();
		SetModePare_SetBattery(bat_cap_tmp);
//更新用户状态
		SetModePare_SetUser(user_flag_tmp);
		
		
		if(FORE_MODE_STATUS ==MODE_M3 || FORE_MODE_STATUS == MODE_M5)
		{
			if(user_flag_tmp == LFS_USER_INFO_GET_FAILED)
			{
				//log_debug("user_flag_tmp = LFS_USER_INFO_GET_FAILED\r\n");
				if(cat1_del_device_flag ==1)
				{
					//log_debug("cat1_del_device_flag = 1\r\n");
					TaskManager_SetMode(SetModePare);
				}
			}
			else
			{
				TaskManager_SetMode(SetModePare);
			}
		}
		else
		{
			TaskManager_SetMode(SetModePare);
		}
	//	log_debug("PM_GetBatteryCapacity() = %d\r\n",PM_GetBatteryCapacity());
        
//释放低功耗阻塞信号量
		osSemaphoreRelease(xSemAgingTest);
		
		m_led_user_handler();//LED处理函数
	
		wdt_feed();//喂狗
		//信号量的处理需要时间片500ms
		EntryTask_HandleMessageQueue(pEntryTaskInfo,&received_msg);
		
		uint32_t os_event_flags;
		os_event_flags = osEventFlagsWait(g_sleepEntryReadyFlags, 
                            ALL_SLEEP_ENTRY_READY, 
                            osFlagsWaitAll, 
                            0);
		if (os_event_flags & osFlagsError) {
			//log_debug("EventFlags error: %#x\r\n", os_event_flags);
		// 如果是超时，可以不做处理，或者其他错误处理
			if (os_event_flags == osFlagsErrorTimeout) {
				// 超时，正常情况，继续循环
			} else {
				// 其他错误，需要处理
				
			}
		} 
		else
		{
			if ((os_event_flags & ALL_SLEEP_ENTRY_READY) == ALL_SLEEP_ENTRY_READY) {
				log_debug("All tasks reported ready! Shutting down system... = %d,%d,%d\r\n",FORE_MODE_STATUS,isProductionMake(),device_reset_flag);
				if(FORE_MODE_STATUS == MODE_M4)
				{
					if(device_reset_flag)
					{
						device_reset_flag=0;
						user_initiative_reboot_fun();	
					}
					else
					{
						m4_to_production_config();
						device_reset_flag = 1;
					}
				}
				else
				{
					/* 执行关机逻辑 */
					PowerOffSystem();
						
					//如果先进入M2模式关闭外设后再进入的LTE和GNSS关闭完成这里，则可以直接阻塞，
					//否则需要往下走到if(low_power_off ==1 && FORE_MODE_STATUS == MODE_M2)进行阻塞
					if(FORE_MODE_STATUS == MODE_M2)
					{
						osSemaphoreAcquire(xSemAgingTest, osWaitForever); 
					}
				}
			}
		}
		//
		if(((low_power_off == 1) && (FORE_MODE_STATUS == MODE_M2)) ||
			(get_entry_low_sleep_flag() == SYSTEM_POWER_STATE_SHUTDOWN))//M3->M2模式切换
		{
			osSemaphoreAcquire(xSemAgingTest, osWaitForever); 
		}
		
        osSemaphoreAcquire(xSemAgingTest, osWaitForever);
    }
}

/*********************************************************************
 * CONST VARIABLES
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */
extern void osEventLittleFsFlagsInit(void);
/**
 * @brief  v start bluetooth task
 **/
osThreadId_t vStartEntryTask(void)
{

	osEventLittleFsFlagsInit();

    const osThreadAttr_t EntryThreadAttr = {
        .name = "Entry_Task",
        .attr_bits = 0,
        .cb_mem = NULL,
        .cb_size = 0,
        .stack_mem = NULL,
        .stack_size = ENTRY_TASK_STACK_SIZE,
        .priority = ENTRY_TASK_PRIORITY,
        .tz_module = 0,
    };


    task_info[ENTRY_TASK_ID].task_handle = osThreadNew(vEntryTask, NULL, &EntryThreadAttr);


    if(task_info[ENTRY_TASK_ID].task_handle == NULL)
    {
       LOG_LOC();
    }

    
    return task_info[ENTRY_TASK_ID].task_handle;
}

/** @} */

// vim: fdm=marker
