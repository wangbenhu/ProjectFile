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
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include "om_driver.h"
#if (CONFIG_SHELL)
#include "shell.h"
#endif
#include "evt.h"
#include "pm.h"
#include "bsp.h"
#include "omble.h"
#include "om_log.h"
#include "service_common.h"

#include "cJSON.h"
#include "Comm_Task.h"

// Controller header
#include "obc.h"
#include "imu_ex.h"

/* Kernel includes. */
#include "cmsis_os2.h"

#include "timers.h"

#include "service_tspp_define.h"
#include "lfs_port.h"
#include "littlefs_time_rtc.h"
#include "m_motor.h"
#include "led_task.h"
#include  "common_def.h"
#include "inv_imu_apex.h"

/*********************************************************************
 * MACROS
 */

#define COMM_LOG_DEBUG(format, ...)               	log_debug(format,  ## __VA_ARGS__)
/// log array
#define COMM_LOG_ARRAY(array, len)            do{int __i; for(__i=0;__i<(len);++__i)COMM_LOG_DEBUG("%02X ",((uint8_t *)(array))[__i]);}while(0)

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * CONSTANTS
 */
#define EVENT_SYSTEM_RESERVE_MASK   0x00FF

#define COMM_TASK_PRIORITY (osPriorityNormal)//(osPriorityNormal) osPriorityAboveNormal
#define COMM_TASK_STACK_SIZE (8192)

#define TIMER_SYNCHRONIZE_INTERVAL (10)
#define APP_AUDIO_REALTIME_LEN_MAX (200 * 1024)

// 主动上报定时器参数（从CAT1迁移到COMM）
#define COMM_AUTO_REPORT_INTERVAL_STANDARD_MS  (10 * 60 * 1000UL)  // 常规模式10分钟
#define COMM_AUTO_REPORT_INTERVAL_SEARCH_MS    (10 * 1000UL)       // V1.6寻宠模式10秒(原3s)
#define COMM_BOOT_HIGH_FREQ_INTERVAL_MS        (15 * 1000UL)       // 开机高频15秒
#define COMM_BOOT_HIGH_FREQ_DURATION_MS        (2 * 60 * 1000UL)   // 开机高频持续2分钟

// V1.6 健康定频上报: 固定10s采样一次姿态, 60次(10分钟)组包上报, 不随模式切换变频
#define COMM_HEALTH_TICK_INTERVAL_MS           (10 * 1000UL)       // 姿态采样周期10秒
#define COMM_HEALTH_REPORT_TICKS               (60)                // 60次采样=10分钟上报一次

/* 消息优先级说明: 本移植层osMessageQueuePut的msg_prio被忽略(纯FIFO), 真实优先级机制:
 *   ① 即时上报(设备状态变化): xQueueSendToFront插队首, 最先处理, 且不丢(未消费时100ms节拍自动补发)
 *   ② 被动接收(APP/云指令): 队尾FIFO正常排队
 *   ③ 主动上报(定时auto report/健康定频tick): 取出时若队列有积压则让位重排队尾, 排最后 */
#define MSG_PRIO_PASSIVE_HIGH  3   // 被动接收消息优先级（APP指令/云端指令）
#define MSG_PRIO_ACTIVE_LOW    1   // 主动上报消息优先级
#define COMM_ACTIVE_DEFER_MAX  8   // 主动上报让位次数上限(防饿死/防两条主动消息互相让位死循环)

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint8_t test_start_flag = 0;
static uint8_t usb_updata_flag = 0;
/* ===== 100ms 状态监控 + 分维度防抖 ===== */
/* 快变(低电/搭电): 变化立即上报; 慢变(围栏/姿态/模式/居家): 30s防抖后上报最后一次状态 */
static osTimerId_t state_monitor_timer_id = NULL;
static uint32_t slow_debounce_start_tick = 0;
static bool slow_debounce_pending = false;
static DeviceStateFlags_t slow_debounce_last_state;  // 防抖基准(慢变维度变化时重置)
	static DeviceStateFlags_t slow_pending_state;       // 防抖窗口内最后一次慢变值(到期上报用)
static volatile uint32_t g_instant_pending_mask = 0;   // 定时器写, 主任务读+清零(defer传递)
static DeviceStateFlags_t g_instant_pending_state;      // 定时器设置的待上报状态副本
static DeviceStateFlags_t g_instant_report_state;        // 主任务组包时用的状态(不破坏device_stateFlags)
#define STATE_MONITOR_INTERVAL_MS   100
#define SLOW_DEBOUNCE_MS            30000
/* V1.6 全局变量 + 前置声明(供 update_device_state_flags/check_and_report_instant_state 等早期API使用) */
char g_posture_seq[128] = {0};
uint32_t g_instant_changed_mask = 0;
int dataUpStream_handler(RESPONSE_TYPES type, uint16_t dataResourse);

// ===== 主动上报定时器（从CAT1迁移到COMM）=====
static osTimerId_t commAutoReportTimer_ID = NULL;
static osTimerType_t commAutoReportTimer_type = osTimerPeriodic;
static osTimerAttr_t commAutoReportTimer_attr = {
    .name = "CommAutoReport_Timer",
};
static uint32_t comm_report_interval_ms = COMM_AUTO_REPORT_INTERVAL_STANDARD_MS;
static bool comm_boot_high_freq_active = false;
static uint32_t comm_boot_start_tick = 0;
/* V1.6 防堆积闸门(与即时上报inflight同款): 队列里最多1条在途, 处理不过来时周期合并跳过,
 * 防短周期(1s/3s寻宠)+COMM慢(每条≥1.1s)时AUTO/TICK消息灌满16深队列挤掉被动指令 */
static volatile bool g_auto_report_pending = false;
static volatile bool g_health_tick_pending = false;

// 主动上报定时器回调 — 只做一件事: 把消息入COMM队列
static void CommAutoReportTimerCallback(void *argument)
{
    if (g_auto_report_pending) {
        return;  /* 上一条还没消费: 本周期合并跳过(数据是取出时现采, 堆旧消息无意义) */
    }
    TaskInfo_t *my_task_info = GetTaskInfo(COMM_TASK_ID);
    Message_t auto_msg = {
        .source_id = my_task_info->task_id,
        .dest_id = COMM_TASK_ID,
        .command = TASK_COMM_AUTO_REPORT,
        .data = NULL,
        .data_length = 0
    };
    // 主动上报优先级低(1)，被动接收优先级高(3)
    uint8_t msg_prio = MSG_PRIO_ACTIVE_LOW;
    if (osMessageQueuePut(my_task_info->queue_handle, &auto_msg, msg_prio, 0) == osOK) {
        g_auto_report_pending = true;
    }
}

// 启动/更新主动上报定时器
static void CommStartAutoReportTimer(uint32_t interval_ms)
{
    if (commAutoReportTimer_ID == NULL) {
        commAutoReportTimer_ID = osTimerNew(CommAutoReportTimerCallback, 
                                            commAutoReportTimer_type, NULL, 
                                            &commAutoReportTimer_attr);
    }
    if (commAutoReportTimer_ID) {
        // 周期定时器需要先停再启来改变间隔
        if (osTimerIsRunning(commAutoReportTimer_ID)) {
            osTimerStop(commAutoReportTimer_ID);
        }
        osTimerStart(commAutoReportTimer_ID, osMS2TicksRound(interval_ms));
        comm_report_interval_ms = interval_ms;
    }
}

// 停止主动上报定时器
static void CommStopAutoReportTimer(void)
{
    if (commAutoReportTimer_ID && osTimerIsRunning(commAutoReportTimer_ID)) {
        osTimerStop(commAutoReportTimer_ID);
    }
}

// ===== V1.6 健康定频上报定时器(10s固定周期, 不随模式变频) =====
static osTimerId_t commHealthTimer_ID = NULL;
static osTimerAttr_t commHealthTimer_attr = {
    .name = "CommHealth_Timer",
};
static uint32_t g_health_tick_count = 0;        // 本轮已采样次数
static uint32_t g_health_window_start_ts = 0;   // 采样窗口起始时间戳(healthInfo的startTs)

// 健康定时器回调 — 只发消息入COMM队列, 采样/组包由主任务做(线程安全)
static void CommHealthTimerCallback(void *argument)
{
    if (g_health_tick_pending) {
        return;  /* 上个tick还在队列里: 合并跳过(丢一次姿态采样, 换队列不被灌满) */
    }
    TaskInfo_t *my_task_info = GetTaskInfo(COMM_TASK_ID);
    Message_t health_msg = {
        .source_id = my_task_info->task_id,
        .dest_id = COMM_TASK_ID,
        .command = TASK_COMM_HEALTH_TICK,
        .data = NULL,
        .data_length = 0
    };
    if (osMessageQueuePut(my_task_info->queue_handle, &health_msg, MSG_PRIO_ACTIVE_LOW, 0) == osOK) {
        g_health_tick_pending = true;
    }
}

// 启动健康定频采样(重新开窗: 清空姿态序列+记录窗口起始时间)
static void CommStartHealthTimer(void)
{
    if (commHealthTimer_ID == NULL) {
        commHealthTimer_ID = osTimerNew(CommHealthTimerCallback, osTimerPeriodic, NULL, &commHealthTimer_attr);
    }
    if (commHealthTimer_ID && !osTimerIsRunning(commHealthTimer_ID)) {
        g_health_tick_count = 0;
        g_posture_seq[0] = '\0';
        g_health_window_start_ts = get_timestamp_date(NULL);
        osTimerStart(commHealthTimer_ID, osMS2TicksRound(COMM_HEALTH_TICK_INTERVAL_MS));
    }
}

// 停止健康定频采样(低功耗/休眠)
static void CommStopHealthTimer(void)
{
    if (commHealthTimer_ID && osTimerIsRunning(commHealthTimer_ID)) {
        osTimerStop(commHealthTimer_ID);
    }
}

/* V1.6: 主动上报让位计数(取到AUTO_REPORT时队列有积压则重排队尾, 超限强制执行) */
static uint8_t g_active_defer_cnt = 0;
static volatile bool g_instant_msg_inflight = false;  // 即时消息在途标志(true=已入队未消费)

/* V1.6: 即时上报消息插队首 — 设备状态变化必须让APP第一时间知道, 优先于被动接收和其他主动上报
 * 注: 本移植层osMessageQueuePut的msg_prio被忽略(纯FIFO), 故用xQueueSendToFront实现真插队;
 * 单在途消息模型(防压力洪泛): inflight既是补发依据也是限流阀, 队列中最多1条即时消息;
 *   - 在途期间状态再变: 只更新mask/state(消费时读最新值), 不重复插队, 不会挤占被动指令的队列位
 *   - 入队失败不丢: inflight不置位, mask保留, 100ms监控节拍自动补发 */
static void comm_send_instant_msg_front(void)
{
	if (g_instant_msg_inflight) return;  /* 已有在途消息: 消费时会带走最新mask/state, 无需重复插队 */
	TaskInfo_t *my_task_info = GetTaskInfo(COMM_TASK_ID);
	Message_t msg = { .source_id = my_task_info->task_id, .dest_id = COMM_TASK_ID,
	                  .command = TASK_COMM_INSTANT_REPORT, .data = NULL, .data_length = 0 };
	if (xQueueSendToFront((QueueHandle_t)my_task_info->queue_handle, &msg, 0) == pdTRUE) {
		g_instant_msg_inflight = true;
	}
}

mqtt_function_t comm_func_topic = FUNC_UNKNOWN;
/*********************************************************************
 * GLOBAL VARIABLES
 */
uint32_t test_massage_example = PRODUCTION_TASK_EXAMPLE_NONE;
bool isBleTsppCommplete = false;		   
message_data_t b_message_data;
sensor_test_imu_apex_parameters_t sensor_test_imu_apex_parameters={
.pedo_amp_th 			= APEX_CONFIG3_PEDO_AMP_TH_62_MG,
.pedo_step_cnt_th     	= 0x5,
.pedo_step_det_th     	= 0x2,
.pedo_sb_timer_th     	= APEX_CONFIG4_PEDO_SB_TIMER_TH_150_SAMPLES,
.pedo_hi_enrgy_th     	= APEX_CONFIG4_PEDO_HI_ENRGY_TH_104_MG,
.power_save_time		= APEX_CONFIG2_DMP_POWER_SAVE_TIME_SEL_8_S,
.low_energy_amp_th 		= APEX_CONFIG2_LOW_ENERGY_AMP_TH_SEL_80_MG, 
};//sensor测试

/*********************************************************************
 * EXTERN FUNCTIONS
 */
extern uint8_t Audio_Play_Start(const char *filename,uint8_t audio_reset);
extern uint8_t Audio_IFlash_Play_Start(uint8_t index,uint8_t audio_reset);
extern void led_turn_on_all(void);
extern void led_turn_off_all(void);
extern void led_stop_current_state(void);
extern void unblock_cat1_task(void);
extern bool safe_unblock_uart_task(void);
extern uint8_t PM_GetBatteryCapacity(void);
extern void evt_app_adv_stop(void);
extern uint64_t get_step_num(void);
extern SystemMode_t Entry_Task_Run_Mode_Get(void);
extern uint8_t Message_Cmd_Put(TASK_ID_T source_id,
                                       TASK_ID_T dest_id,
                                       TASK_CMD_T command,
                                       void *data,
                                       uint16_t data_length);
extern uint8_t* DEVICE_GetCat1Version(void);
extern void evt_app_adv_dissconn(void);				
extern int ble_dataUpStream_subPackage(uint8_t *data, tspp_size_t len);	
extern void CurrentModeDataSet(CURRENT_MODE_T mode);
uint32_t calculate_flash_crc(uint32_t start_addr, uint32_t length);	
//0未绑定1成功
extern void uset_bound_state_set(uint8_t data);									   
/*********************************************************************
 * LOCAL FUNCTIONS
 */
void get_sensor_imu_apex_parameters(sensor_test_imu_apex_parameters_t *imu_apex_parameters_tmp)
{
//	if(drv_flash_read(OM_FLASH0, id, buf, *lengthPtr))
//	{
//		
//	}
	imu_apex_parameters_tmp->pedo_amp_th 		= sensor_test_imu_apex_parameters.pedo_amp_th;
	imu_apex_parameters_tmp->low_energy_amp_th 	= sensor_test_imu_apex_parameters.low_energy_amp_th;
	imu_apex_parameters_tmp->pedo_hi_enrgy_th 	= sensor_test_imu_apex_parameters.pedo_hi_enrgy_th;
	imu_apex_parameters_tmp->pedo_sb_timer_th 	= sensor_test_imu_apex_parameters.pedo_sb_timer_th;
	imu_apex_parameters_tmp->pedo_step_cnt_th 	= sensor_test_imu_apex_parameters.pedo_step_cnt_th;
	imu_apex_parameters_tmp->pedo_step_det_th 	= sensor_test_imu_apex_parameters.pedo_step_det_th;
	imu_apex_parameters_tmp->power_save_time 	= sensor_test_imu_apex_parameters.power_save_time;
	
}
int memcpy_s(void* dest, size_t destsz, const void *  src,size_t srcsz)
{
	OM_ASSERT(destsz >= srcsz);
	if(destsz >= srcsz)
	{
	    memcpy(dest, src, srcsz);
	    return 0;
	}
	else
	{
	    return -1;
	}

}

int memset_s(void * dest,size_t destsz,int ch,size_t count)
{
    OM_ASSERT(destsz >= count);
	if(destsz >= count)
	{
	  memset(dest,ch,count);
	  return 0;
	}
	else
	{
        return -1;
	}
}

/**
 * @brief 将字符串MAC地址转换为6字节十六进制数组
 * @param mac_str 字符串
 * @param mac_buf 输出缓冲区
 * @return 成功返回0，失败返回-1
 */
int string_mac_to_bytes(const char *mac_str, uint8_t *mac_buf) {
    char clean_mac[13] = {0};
    int j = 0;

    for (int i = 0; mac_str[i] != '\0' && j < 12; i++) {
        if (isxdigit(mac_str[i])) {
            clean_mac[j++] = mac_str[i];
        }
    }

    if (j != 12) {
        return -1;
    }

    // 转换并反转顺序
    for (int i = 0; i < 6; i++) {
        char hex_byte[3] = {clean_mac[i*2], clean_mac[i*2+1], '\0'};
        mac_buf[i] = (uint8_t)strtoul(hex_byte, NULL, 16);
    }

    for (int i = 0; i < 3; i++) {
        uint8_t temp = mac_buf[i];
        mac_buf[i] = mac_buf[5 - i];
        mac_buf[5 - i] = temp;
    }
    
    return 0;
}
	
/**
 * @brief 获得json的节点对象obj。
 * @param pJsonArray: json节点对象数组。
 * @return 返回值cJSON对象。
 * 返回值是NULL时，表示生成cJSON失败。
 */
cJSON *getJsonObj(CJsonData *pJsonArray)
{
    cJSON *pRet = cJSON_CreateObject();
    int i = 0;
    while (1) {
        switch (pJsonArray[i].eJsonType) {
            case JSON_STR:
                cJSON_AddStringToObject(pRet, pJsonArray[i].szName, (const char *)pJsonArray[i].pvSrc);
                break;
            case JSON_NUM:
                cJSON_AddNumberToObject(pRet, pJsonArray[i].szName, (int)pJsonArray[i].pvSrc);
                break;
            case JSON_OJB:
                cJSON_AddItemToObject(pRet, pJsonArray[i].szName, (cJSON *)pJsonArray[i].pvSrc);
                break;
            case JSON_NO:
            case JSON_EOF:
                return pRet;
            default:
                return pRet;
        }
        i++;
    }
}

osTimerType_t deviceDisconn_Timer_type = osTimerOnce;
osTimerAttr_t deviceDisconn_Timer_attr = {
	.name = "deviceDisconn_Timer",
};
osTimerId_t deviceDisconn_Timer_ID = NULL;
void DisconnTimerCallback(void *argument)
{
//	ob_gap_disconnect(0,0x13);
	log_debug("[COMM][STA] DisconnTimerCallback=%d\r\n",isBleTsppCommplete);
	if(isBleTsppCommplete)
	{
		isBleTsppCommplete = false;
		evt_app_adv_dissconn();
		update_user_bind_status();
	}
	else
	{
		osTimerStart(deviceDisconn_Timer_ID, osMS2TicksRound(1000));
	}
}
void device_disconnect(void)
{
	if (deviceDisconn_Timer_ID == NULL) {
		deviceDisconn_Timer_ID = osTimerNew(DisconnTimerCallback,deviceDisconn_Timer_type,NULL,&deviceDisconn_Timer_attr);
		if(!deviceDisconn_Timer_ID)
			LOG_LOC();
	}
	if(osTimerIsRunning(deviceDisconn_Timer_ID))
	{
		osTimerStop(deviceDisconn_Timer_ID);
	}
	osTimerStart(deviceDisconn_Timer_ID, osMS2TicksRound(1000));
	
}

bool isExist_device_auth(void)
{
    const char *stored_auth = user_bindinfo_get(UBI_AUTH_CODE_ID);
	log_debug("[COMM][STA] isExist_device_auth = %s\r\n",stored_auth);
    // 检查返回的指针是否有效，并且第一个字符不是结束符
    if (stored_auth[0] != '\0') {
        return true;  // 存在数据
    }
    return false;  // 无数据
}

bool isMatch_device_auth(uint8_t *data, uint16_t len)
{
	// 获取设备存储的鉴权码
    const char *stored_auth = user_bindinfo_get(UBI_AUTH_CODE_ID);
	log_debug("[COMM][STA] isMatch_device_auth = %s %d %d\r\n",stored_auth,strlen(stored_auth),len);
    if (!data || !stored_auth) {
        return false;
    }
	
	// 检查长度是否匹配
    size_t stored_len = strlen(stored_auth);
    if (stored_len != len) {
        return false;
    }
	
	// 比较内容
    if (memcmp(data, (const char *)stored_auth, len) == 0) {
        return true;
    }
	else
	{
		return false;
	}
}

bool delete_device_auth(void)
{
 //   user_bindinfo_del(UBI_AUTH_CODE_ID);
    user_bindinfo_delete_field_by_id(UBI_AUTH_CODE_ID);
	//读文件，为空则已经删除完成
	if(!isExist_device_auth())
	{
		return true;
	}
    return false;
}

uint8_t device_auth(uint8_t *data, uint16_t len)
{
	uint8_t ret = 0;
	
	//读文件，为空则是第一次注册
	if(!isExist_device_auth())
	{
		deviceAuthStopTimer();
		COMM_LOG_DEBUG("[COMM][STA] isMatch_device_auth = %s %d\r\n",(const char *)data,len);
		//写入鉴权码到文件系统
		if(user_bindinfo_set((const char *)data, len, UBI_AUTH_CODE_ID) < 0)
		{
			log_debug("[COMM][STA] ret: 0\r\n");
			ret = 0;
		}
		else
			{log_debug("[COMM][STA]ret: 1\r\n");
			ret = 1;
		}
	}
	else
	{
		//匹配
		if(isMatch_device_auth(data, len))
		{
			//匹配，回复鉴权成功
			deviceAuthStopTimer();
			ret = 1;
		}
		else
		{
			ret = 0;
		}
	}
	
	return ret;
}

uint8_t GetReportMode(void)
{
    CURRENT_MODE_T mode = CurrentModeDataGet();
    
    if (mode == CURRENT_MODE_CHARGE) {
        return 1;  // 充电模式
    } else if (mode == CURRENT_MODE_LOW_BATTERY) {
        return 2;  // 低电量模式
    } else {
        return 3;  // 常规模式
    }
}

//更新设备时间
void device_systime(void)
{
	// 获取当前RTC时间戳
	uint32_t current_timestamp = get_timestamp_date(NULL);
	// 获取云端下发的时间戳
    uint32_t cloud_timestamp = b_message_data.appControl_timestamp;
	
	// 检查时间差是否超过10秒
    if (cloud_timestamp - current_timestamp > TIMER_SYNCHRONIZE_INTERVAL)
    {
        rtc_set_time_from_timestamp(cloud_timestamp);
        
        // 更新设备时间戳记录
        b_message_data.device_timestamp = cloud_timestamp;
	}
	else
    {
        // 时间差在允许范围内，不需要对时
        b_message_data.device_timestamp = current_timestamp;
    }
}

void convert_systime()
{
	//获取时间rtc时间戳
	b_message_data.device_timestamp = get_timestamp_date(NULL);

	//更新设备的时间
	rtc_set_time_from_timestamp(b_message_data.appControl_timestamp);

	b_message_data.device_timestamp = get_timestamp_date(NULL);
}
/* ============ V1.6: 刷新状态位 ============ */
static void update_device_state_flags(void)
{
	CURRENT_MODE_T cur_mode = CurrentModeDataGet();
	b_message_data.device_stateFlags.battery = (cur_mode == CURRENT_MODE_LOW_BATTERY) ? STATE_BATT_LOW : STATE_BATT_NORMAL;
	b_message_data.device_stateFlags.charge = (cur_mode == CURRENT_MODE_CHARGE) ? STATE_CHARGE_CHARGING : STATE_CHARGE_UNPLUG;
	if (!b_message_data.device_geofence.valid) b_message_data.device_stateFlags.fence = STATE_FENCE_UNCONFIG;
	else b_message_data.device_stateFlags.fence = STATE_FENCE_SAFE;
	{ uint8_t mlv = get_motion_level();
	  switch (mlv) { case 1: b_message_data.device_stateFlags.posture = STATE_POSTURE_STATIC; break;
	    case 2: b_message_data.device_stateFlags.posture = STATE_POSTURE_WALKING; break;
	    case 3: b_message_data.device_stateFlags.posture = STATE_POSTURE_RUNNING; break;
	    default: b_message_data.device_stateFlags.posture = STATE_POSTURE_OTHER; break; } }
	b_message_data.device_stateFlags.mode = (cur_mode == CURRENT_MODE_SEARCH_PET) ? STATE_MODE_SEARCH : STATE_MODE_STANDARD;
	b_message_data.device_stateFlags.home = (b_message_data.device_wifiConfigState == STATE_HOME_CONFIG_ON) ? STATE_HOME_CONFIG_ON : (b_message_data.device_wifiConfigState == STATE_HOME_CONFIG_OFF) ? STATE_HOME_CONFIG_OFF : STATE_HOME_UNCONFIG;//无效
}

/* ============ 100ms 状态监控回调: 检测+防抖 ============ */
static void state_monitor_callback(void *argument)
{
	(void)argument;

	/* V1.6 有待上报且无在途消息时补发 */
	if (g_instant_pending_mask != 0) {
		comm_send_instant_msg_front();  /* 内部带inflight闸门, 在途时直接返回 */
	}

	update_device_state_flags();
	DeviceStateFlags_t *cur = &b_message_data.device_stateFlags;
	DeviceStateFlags_t *last = &b_message_data.lastReport_stateFlags;

	/* === 快变: 低电/搭电 立即发消息(defer到主任务组包) === */
	if (cur->battery != last->battery || cur->charge != last->charge) {
		uint32_t mask = 0x01;
		if (b_message_data.device_battery != b_message_data.lastReport_battery) mask |= 0x02;
		if (cur->fence == STATE_FENCE_OUTSIDE) mask |= 0x04;
		g_instant_pending_mask = mask;
		g_instant_pending_state = *cur;
		comm_send_instant_msg_front();  /* 插队首; 在途时仅刷新mask/state不重复插队, 失败下节拍补发 */
		return;
	}

	/* === 慢变: 围栏/姿态/模式/居家 30s防抖 === */
	bool slow_changed = (cur->fence != slow_debounce_last_state.fence ||
	                     cur->posture != slow_debounce_last_state.posture ||
	                     cur->mode != slow_debounce_last_state.mode ||
	                     cur->home != slow_debounce_last_state.home);
	if (slow_changed) {
		slow_pending_state = *cur;
		slow_debounce_last_state = *cur;
		if (!slow_debounce_pending) {
			slow_debounce_pending = true;
			slow_debounce_start_tick = osKernelGetTickCount();
		}
	}
	if (slow_debounce_pending) {
		uint32_t elapsed = osKernelGetTickCount() - slow_debounce_start_tick;
		uint32_t deadline_ticks = (uint32_t)((uint64_t)SLOW_DEBOUNCE_MS * osKernelGetTickFreq() / 1000UL);
		if (elapsed >= deadline_ticks) {
			slow_debounce_pending = false;
			if (memcmp(&slow_pending_state, last, sizeof(DeviceStateFlags_t)) != 0) {
				uint32_t mask = 0x01;
				if (slow_pending_state.fence == STATE_FENCE_OUTSIDE) mask |= 0x04;
				g_instant_pending_mask = mask;
				g_instant_pending_state = slow_pending_state;
				comm_send_instant_msg_front();  /* 插队首; 在途时仅刷新mask/state不重复插队, 失败下节拍补发 */
			}
		}
	}
}

/* 启动状态监控定时器(100ms), 应在COMM任务初始化时调用 */
void state_monitor_init(void)
{
	if (state_monitor_timer_id != NULL) return;

	/* 刷新当前状态, 并用其初始化所有快照(防止冷启动/重启后全0快照误报) */
	update_device_state_flags();
	b_message_data.lastReport_stateFlags = b_message_data.device_stateFlags;
	b_message_data.lastReport_battery = b_message_data.device_battery;
	b_message_data.lastReport_posture = (uint8_t)b_message_data.device_stateFlags.posture;
	slow_debounce_last_state = b_message_data.device_stateFlags;
	slow_pending_state = b_message_data.device_stateFlags;
	slow_debounce_pending = false;

	static osTimerAttr_t state_monitor_attr = { .name = "StateMonitor_Timer" };
	state_monitor_timer_id = osTimerNew(state_monitor_callback, osTimerPeriodic, NULL, &state_monitor_attr);
	if (state_monitor_timer_id) {
		osTimerStart(state_monitor_timer_id, osMS2TicksRound(STATE_MONITOR_INTERVAL_MS));
	}
}

/* V1.6: 启动状态监控(活跃模式), 唤醒后重新初始化快照防止休眠期间变化误报 */
void state_monitor_start(void)
{
	if (state_monitor_timer_id == NULL) { state_monitor_init(); return; }  /* 首次启动 */
	if (!osTimerIsRunning(state_monitor_timer_id)) {
		update_device_state_flags();
		b_message_data.lastReport_stateFlags = b_message_data.device_stateFlags;
		b_message_data.lastReport_battery = b_message_data.device_battery;
		b_message_data.lastReport_posture = (uint8_t)b_message_data.device_stateFlags.posture;
		slow_debounce_last_state = b_message_data.device_stateFlags;
		slow_pending_state = b_message_data.device_stateFlags;
		slow_debounce_pending = false;
		osTimerStart(state_monitor_timer_id, osMS2TicksRound(STATE_MONITOR_INTERVAL_MS));
	}
}

/* V1.6: 停止状态监控(低电量/休眠), 允许COMM阻塞进入低功耗 */
void state_monitor_stop(void)
{
	if (state_monitor_timer_id && osTimerIsRunning(state_monitor_timer_id)) {
		osTimerStop(state_monitor_timer_id);
	}
	slow_debounce_pending = false;
}
void check_and_report_instant_state(void)
{
	/* V1.6 defer: 外部事件触发, 只检测+发消息, 组包由主任务做(线程安全) */
	update_device_state_flags();
	DeviceStateFlags_t *cur = &b_message_data.device_stateFlags, *last = &b_message_data.lastReport_stateFlags;
	uint32_t mask = 0;
	if (memcmp(cur, last, sizeof(DeviceStateFlags_t)) != 0) mask |= 0x01;
	if (b_message_data.device_battery != b_message_data.lastReport_battery) mask |= 0x02;
	if (cur->fence == STATE_FENCE_OUTSIDE) mask |= 0x04;
	if (mask == 0) return;
	g_instant_pending_mask = mask;
	g_instant_pending_state = *cur;
	comm_send_instant_msg_front();  /* 插队首, 失败由100ms监控节拍补发, 不丢 */
}
void append_posture_to_seq(uint8_t posture_code)
{
	int len = strlen(g_posture_seq);
	if (len < (int)sizeof(g_posture_seq) - 2) { g_posture_seq[len] = (char)('0' + posture_code); g_posture_seq[len+1] = '\0'; }
}
bool report_health_info(void)
{
	/* V1.6 路由: 蓝牙优先, 4G需MQTT已连接; 都不通返回false, 保留姿态序列下个tick重试 */
	DATA_SOURCE_TYPE ds;
	if (get_ble_status() == BLE_STATE_CONNECTED) {
		ds = DATA_SOURCE_BLE;
	} else if (CAT1_IsMqttConnected()) {
		ds = DATA_SOURCE_4G;
	} else {
		return false;
	}

	b_message_data.device_stepDiffer = get_step_num();
	b_message_data.device_timestamp = get_timestamp_date(NULL);

	//V1.6: 刷新6维状态位
	update_device_state_flags();
	comm_func_topic = FUNC_QUERY_HEALTH;
	dataUpStream_handler(TRACK_REPORT_TYPE_HEALTH_INFO, ds);
	g_posture_seq[0] = '\0';
	return true;
}

void UpdateCurrentDeviceState()
{
	GPS_STATUS_t current_gps;
	
	//更新设备时间
	device_systime();
	
	//GPS定位数据
	GPS_GetCurrentData(&current_gps);
	b_message_data.gps_status = current_gps.gps_status;
	if(b_message_data.gps_status == CHANGE_STATUS_POSITION)
	{
		b_message_data.position_quality = GPS_QUALITY_GOOD;
		strncpy((char*)b_message_data.longitude, (char*)current_gps.longitude, sizeof(current_gps.longitude));
		strncpy((char*)b_message_data.latitude, (char*)current_gps.latitude, sizeof(current_gps.latitude));
	}
	else
	{
		b_message_data.position_quality = GPS_QUALITY_WEAK;  // 默认弱，CreateSatellitesArray 会覆盖
		// 搜星无效时，检查是否有上一次有效数据
        if(current_gps.has_last_valid && 
           strlen((char*)current_gps.last_valid_longitude) > 0 && 
           strlen((char*)current_gps.last_valid_latitude) > 0)
        {
            // 使用上一次有效的经纬度
            strncpy((char*)b_message_data.longitude, (char*)current_gps.last_valid_longitude, sizeof(b_message_data.longitude)-1);
            strncpy((char*)b_message_data.latitude, (char*)current_gps.last_valid_latitude, sizeof(b_message_data.latitude)-1);
            b_message_data.longitude[sizeof(b_message_data.longitude)-1] = '\0';
            b_message_data.latitude[sizeof(b_message_data.latitude)-1] = '\0';
        }
        else
        {
            // 没有有效历史数据，设置为0
            b_message_data.longitude[0] = '0';
            b_message_data.longitude[1] = '\0';
            b_message_data.latitude[0] = '0';
            b_message_data.latitude[1] = '\0';
        }
	}
//    strncpy(b_message_data.timestamp, current_gps.timestamp, sizeof(b_message_data.timestamp));
//    b_message_data.lat_dir = current_gps.lat_dir;
//    b_message_data.lon_dir = current_gps.lon_dir;
	
	// 保存卫星数据到全局消息数据结构中
    b_message_data.satellite_count = current_gps.satellite_count;
    if (current_gps.satellite_count > 0) {
        uint8_t count_to_copy = (current_gps.satellite_count > 20) ? 20 : current_gps.satellite_count;
        memcpy(b_message_data.satellites, current_gps.satellites, count_to_copy * sizeof(SatelliteInfo_t));
    }
	
	//电量
	b_message_data.device_battery = PM_GetBatteryCapacity();

	//当前模式
	b_message_data.device_currentMode =  CurrentModeDataGet();//current_csq.cat1_mode
	
	//cat1信号质量
	b_message_data.device_cat1Csq = CAT1_GetCurrentCsq();
	
	//当前运动状态
	b_message_data.device_motionState = get_motion_level();
	
	//步数增幅
	b_message_data.device_stepDiffer = get_step_num();

	//V1.6: 刷新状态位
	update_device_state_flags();
	

			
}

void UpdateAutoDeviceState()
{
	GPS_STATUS_t current_gps;
	
	//更新设备时间
	b_message_data.device_timestamp = get_timestamp_date(NULL);
	
	//GPS定位数据
	GPS_GetCurrentData(&current_gps);
	b_message_data.gps_status = current_gps.gps_status;
	if(b_message_data.gps_status == CHANGE_STATUS_POSITION)
	{
		b_message_data.position_quality = GPS_QUALITY_GOOD;
		strncpy((char*)b_message_data.longitude, (char*)current_gps.longitude, sizeof(current_gps.longitude));
		strncpy((char*)b_message_data.latitude, (char*)current_gps.latitude, sizeof(current_gps.latitude));
	}
	else
	{
		b_message_data.position_quality = GPS_QUALITY_WEAK;  // 默认弱，CreateSatellitesArray 会覆盖
		// 搜星无效时，检查是否有上一次有效数据
        if(current_gps.has_last_valid && 
           strlen((char*)current_gps.last_valid_longitude) > 0 && 
           strlen((char*)current_gps.last_valid_latitude) > 0)
        {
            // 使用上一次有效的经纬度
            strncpy((char*)b_message_data.longitude, (char*)current_gps.last_valid_longitude, sizeof(b_message_data.longitude)-1);
            strncpy((char*)b_message_data.latitude, (char*)current_gps.last_valid_latitude, sizeof(b_message_data.latitude)-1);
            b_message_data.longitude[sizeof(b_message_data.longitude)-1] = '\0';
            b_message_data.latitude[sizeof(b_message_data.latitude)-1] = '\0';
        }
        else
        {
            // 没有有效历史数据，设置为0
            b_message_data.longitude[0] = '0';
            b_message_data.longitude[1] = '\0';
            b_message_data.latitude[0] = '0';
            b_message_data.latitude[1] = '\0';
        }
	}

	// 保存卫星数据到全局消息数据结构中
    b_message_data.satellite_count = current_gps.satellite_count;
    if (current_gps.satellite_count > 0) {
        uint8_t count_to_copy = (current_gps.satellite_count > 20) ? 20 : current_gps.satellite_count;
        memcpy(b_message_data.satellites, current_gps.satellites, count_to_copy * sizeof(SatelliteInfo_t));
    }
	
	//电量
	b_message_data.device_battery = PM_GetBatteryCapacity();

	//当前模式
	b_message_data.device_currentMode =  CurrentModeDataGet();//current_csq.cat1_mode
	
	//cat1信号质量
	b_message_data.device_cat1Csq = CAT1_GetCurrentCsq();
	
	//当前运动状态
	b_message_data.device_motionState = get_motion_level();
	
	//步数增幅
	b_message_data.device_stepDiffer = get_step_num();

	//V1.6: 刷新6维状态位
	update_device_state_flags();
	
	if(b_message_data.reqId[0] == 0)
	{
		strncpy((char *)b_message_data.reqId, "req_2lnk", sizeof(b_message_data.reqId)-1);
	}

	// GPS坐标变化检测: 与上次上报坐标对比, 无变化则标记 gpsCoordsChanged=false
	{
		static char last_lat[20] = {0};
		static char last_lng[20] = {0};
		const char *lat = (const char*)b_message_data.latitude;
		const char *lng = (const char*)b_message_data.longitude;
		if (b_message_data.gps_status == CHANGE_STATUS_POSITION
		    && (strcmp(lat, last_lat) != 0 || strcmp(lng, last_lng) != 0)) {
			b_message_data.gpsCoordsChanged = true;
			strncpy(last_lat, lat, sizeof(last_lat)-1);
			strncpy(last_lng, lng, sizeof(last_lng)-1);
		} else {
			b_message_data.gpsCoordsChanged = false;
		}
	}
}

void UpdateCurrentDeviceInfo()
{
	uint8_t* ble_mac = DEVICE_GetMac();
	
	//cat1序列号
	const char* cat1_sn = lfs_system_read(SYS_FIRMWARE_LTE_VER_ID);
	if (cat1_sn != NULL)
	{
		memcpy((char*)b_message_data.device_cat1Sn, (char*)cat1_sn, strlen((char *)cat1_sn));
	}
	else {
		b_message_data.device_cat1Sn[0] = '0';
	}
	
	//GNSS序列号
	const char* gnss_sn = lfs_system_read(SYS_FIRMWARE_GNSS_VER_ID);
	if (gnss_sn != NULL)
	{
		memcpy((char*)b_message_data.device_gnss, (char*)gnss_sn, strlen((char *)gnss_sn));
	}
	else {
		b_message_data.device_gnss[0] = '0';
	}
	
	// 获取eSIM SN
	const char* esim_sn = lfs_system_read(SYS_DEVICE_SN_ID);
	if (esim_sn != NULL)
	{
		memcpy((char*)b_message_data.device_esimIccid, esim_sn, strlen((char *)esim_sn));
	}
	else {
		b_message_data.device_esimIccid[0] = '0';
	}
	
	//蓝牙MAC
	if (ble_mac != NULL) {
    strncpy((char*)b_message_data.device_bleMAC, (char*)ble_mac, strlen((char *)ble_mac));
	} else {
		b_message_data.device_bleMAC[0] = '0';

	//V1.6新增: 硬件版本号
	strncpy((char*)b_message_data.device_hardVersion, "0105", sizeof(b_message_data.device_hardVersion)-1);
	//V1.6新增: 设备SN
	{ const char* dsn = lfs_system_read(SYS_DEVICE_SN_ID); if (dsn != NULL) strncpy((char*)b_message_data.device_sn, dsn, sizeof(b_message_data.device_sn)-1); else b_message_data.device_sn[0] = '0'; }
	//V1.6新增: 设备类型
	strncpy((char*)b_message_data.device_model, "MXH001", sizeof(b_message_data.device_model)-1);
	}
	
	//更新设备时间
	device_systime();
}

void UpdateCurrenteNodeBInfo()
{
    // 更新设备时间
    device_systime();
    
    // 基站信息
    CellInfo_t cell_info;
    
    if (CELL_GetCurrentData(&cell_info)) {
		// ========== 服务小区信息 ==========
		// 字符串类型数据
		strncpy(b_message_data.cell_rat, cell_info.cell_rat, sizeof(b_message_data.cell_rat) - 1);
		b_message_data.cell_rat[sizeof(b_message_data.cell_rat) - 1] = '\0';
		
		strncpy(b_message_data.cell_duplex, cell_info.cell_duplex, sizeof(b_message_data.cell_duplex) - 1);
		b_message_data.cell_duplex[sizeof(b_message_data.cell_duplex) - 1] = '\0';
		
		strncpy(b_message_data.carrier, cell_info.carrier, sizeof(b_message_data.carrier) - 1);
		b_message_data.carrier[sizeof(b_message_data.carrier) - 1] = '\0';

		// 整型数据
		b_message_data.mcc = cell_info.mcc;
		b_message_data.mnc = cell_info.mnc;
		
		// ===== 修改：直接复制字符串，不再转换 =====
		// cell_id已经是字符串，直接复制
		strncpy(b_message_data.cell_id, cell_info.cell_id, sizeof(b_message_data.cell_id) - 1);
		b_message_data.cell_id[sizeof(b_message_data.cell_id) - 1] = '\0';
		
		// cell_tac: 直接复制字符串
		strncpy(b_message_data.cell_tac, cell_info.cell_tac, sizeof(b_message_data.cell_tac) - 1);
		b_message_data.cell_tac[sizeof(b_message_data.cell_tac) - 1] = '\0';
		
		b_message_data.cell_earfcn = cell_info.cell_earfcn;
		
		// cell_pcid: 直接复制字符串
		strncpy(b_message_data.cell_pcid, cell_info.cell_pcid, sizeof(b_message_data.cell_pcid) - 1);
		b_message_data.cell_pcid[sizeof(b_message_data.cell_pcid) - 1] = '\0';
	
		b_message_data.cell_band = cell_info.cell_band;
		b_message_data.cell_rsrp = cell_info.cell_rsrp;
		b_message_data.cell_rsrq = cell_info.cell_rsrq;
		b_message_data.cell_rssi = cell_info.cell_rssi;
		b_message_data.cell_sinr = cell_info.cell_sinr;
		/* V1.6: 删除neighbour填充 */
	} else {
		// 整个基站信息获取失败时，清除所有数据和标志
		memset(b_message_data.cell_rat, 0, sizeof(b_message_data.cell_rat));
		memset(b_message_data.cell_duplex, 0, sizeof(b_message_data.cell_duplex));
		memset(b_message_data.carrier, 0, sizeof(b_message_data.carrier));
		/* V1.6: nb memset rm */
		memset(b_message_data.cell_tac, 0, sizeof(b_message_data.cell_tac));
		memset(b_message_data.cell_pcid, 0, sizeof(b_message_data.cell_pcid));
		
		// 清零新增字段
		b_message_data.mcc = 0;
		b_message_data.mnc = 0;
		
		// 原有字段清零
		b_message_data.cell_earfcn = 0;
		b_message_data.cell_band = 0;
		b_message_data.cell_rsrp = 0;
		b_message_data.cell_rsrq = 0;
		b_message_data.cell_rssi = 0;
		b_message_data.cell_sinr = 0;
		/* V1.6: nb zero rm */
	}
}
	
void UpdateCurrentAudioFile()
{
	//更新设备时间
	device_systime();
	
	//音频文件
	
}

void UpdateCurrentWifiSsid(DeviceWifiSsid_t *wifi_data)
{
	 if (wifi_data == NULL) 
    {
        COMM_LOG_DEBUG("[COMM][ERR] Invalid WiFi data in UpdateCurrentWifiSsid\r\n");
        return;
    }
	
	memcpy(&b_message_data.device_wifiScanData, wifi_data, sizeof(DeviceWifiSsid_t));
	//更新设备时间
	device_systime();
}

void UpdateCurrentWarningState()
{
	//更新设备时间
	device_systime();
	
	//当前电量
	b_message_data.device_battery = PM_GetBatteryCapacity();
	
	//当前模式：1 充电模式；2 低电量模式；3：常规模式
	b_message_data.device_currentMode = GetReportMode();
}
	
void UpdateCurrentErrorCode()
{
	//更新设备时间
	device_systime();
	
	//错误码
//	b_message_data.device_exeErrorCode = 0;
}

void productErrorCode()
{
//	b_message_data.product_exeErrorCode = 0;
}

void productGetSn()
{
	lfs_mount_safe();
 	const char* esim_sn = system_info_get(SYS_DEVICE_SN_ID);
	// 获取eSIM SN
	if (esim_sn != NULL) {
    memcpy((char*)b_message_data.device_esimIccid, esim_sn, strlen(esim_sn));
	} else {
		b_message_data.device_esimIccid[0] = '0';
	}
//	lfs_unmount_safe();
}

void productCat1Version()
{
	const char* cat1_version = lfs_system_read(SYS_FIRMWARE_LTE_VER_ID);
	if (cat1_version != NULL)
	{
		memcpy((char*)b_message_data.appProduct_cat1Version, (char*)cat1_version, strlen((char *)cat1_version));
		log_debug("[COMM][STA] cat1_sn: %s, %s\r\n", cat1_version, b_message_data.appProduct_cat1Version);
	}
	else {
		b_message_data.appProduct_cat1Version[0] = '0';
//		COMM_LOG_DEBUG("CAT1 SN not available\n");
	}
}

void productGpsVersion()
{
	uint8_t* gnss_version = DEVICE_GetGnssVersion();
	GPS_STATUS_t current_gps;

	//GPS定位数据
	GPS_GetCurrentData(&current_gps);
	b_message_data.gps_status = current_gps.gps_status;
	
	if (gnss_version != NULL) {
    strncpy((char*)b_message_data.appProduct_gpsVersion, (char*)gnss_version, sizeof(b_message_data.appProduct_gpsVersion));
	} else {
		b_message_data.appProduct_gpsVersion[0] = '0';
	}
}

void productGetMac()
{
    uint8_t* ble_mac = DEVICE_GetMac();
    uint32_t crc32_vlue = 0;
    
    crc32_vlue = calculate_flash_crc(0x4000, 460 * 1024);
    
    // 结果示例: "1234ABCD+AA:BB:CC:DD:EE:FF"
    if (ble_mac != NULL) {
        sprintf((char*)b_message_data.deviceProduct_bleMAC, "%08X+%s", crc32_vlue, (char*)ble_mac);
    } else {
        // 如果没有 MAC 地址，只写入 CRC 和占位符，防止拼接乱码
        sprintf((char*)b_message_data.deviceProduct_bleMAC, "%08X+0", crc32_vlue);
    }
    
    b_message_data.deviceProduct_bleMAC[29] = '\0';
}



void productGetMsg()
{
    uint8_t* ble_mac = DEVICE_GetMac();
    
    // 结果示例: "AA:BB:CC:DD:EE:FF"
    if (ble_mac != NULL) {
        sprintf((char*)b_message_data.deviceProduct_bleMAC, "%s", (char*)ble_mac);
    } else {
        sprintf((char*)b_message_data.deviceProduct_bleMAC, "0");
    }
    
    b_message_data.deviceProduct_bleMAC[29] = '\0';
	
	//电量
	b_message_data.device_battery = PM_GetBatteryCapacity();
	
	///eSim ID
	productGetSn();
}

void UpdateCurrentData()
{
	
}

cJSON* CreateSatellitesArray(void) {
    cJSON *topSatellites = cJSON_CreateArray();
    
    if (b_message_data.satellite_count == 0) {	// || b_message_data.gps_status == 0
        return topSatellites; // 直接返回空数组
    }
    
    // 筛选有信号强度的卫星
    SatelliteInfo_t valid_sats[20];
    uint8_t valid_count = 0;
    
    for (int i = 0; i < b_message_data.satellite_count && i < 20; i++) {
        // 放宽条件：SNR > 0 的卫星才参与排序上报
        if (b_message_data.satellites[i].satellite_id > 0 && 
            b_message_data.satellites[i].snr > 0) {
            valid_sats[valid_count++] = b_message_data.satellites[i];
        }
    }
    
    if (valid_count == 0) {
        return topSatellites; // 没有有效卫星，返回空数组
    }
    
    // 按SNR降序排序
    for (int i = 0; i < valid_count - 1; i++) {
        for (int j = 0; j < valid_count - i - 1; j++) {
            if (valid_sats[j].snr < valid_sats[j + 1].snr) {
                SatelliteInfo_t temp = valid_sats[j];
                valid_sats[j] = valid_sats[j + 1];
                valid_sats[j + 1] = temp;
            }
        }
    }

    // ===== 根据全部有效卫星 SNR 计算定位质量等级（排序后立即计算）=====
    // 只在未定位时计算（定位成功时已是 GPS_QUALITY_GOOD）
    // 均值+达标颗数双重判断，比单颗或单均值更稳定
    if (b_message_data.gps_status != CHANGE_STATUS_POSITION) {
        uint32_t snr_sum = 0;
        uint8_t qualified_count = 0;
        for (int i = 0; i < valid_count; i++) {
            snr_sum += valid_sats[i].snr;
            if (valid_sats[i].snr >= GPS_SNR_THRESHOLD_FAIR) {
                qualified_count++;
            }
        }
        uint8_t avg_snr = (uint8_t)(snr_sum / valid_count);

        // 均值达标 且 至少2颗卫星 SNR >= 阈值 → FAIR
        if (avg_snr >= GPS_SNR_THRESHOLD_AVG_FAIR && qualified_count >= 2) {
            b_message_data.position_quality = GPS_QUALITY_FAIR;
        } else {
            b_message_data.position_quality = GPS_QUALITY_WEAK;
        }

        COMM_LOG_DEBUG("[COMM][STA] GPS quality: avg_snr=%d, qualified=%d/%d, quality=%d\r\n",
                       avg_snr, qualified_count, valid_count,
                       b_message_data.position_quality);
    }

    // 添加最强的卫星（最多3颗，用于云端上报）
    uint8_t report_count = (valid_count > 3) ? 3 : valid_count;
    for (int i = 0; i < report_count; i++) {
        cJSON *satellite = cJSON_CreateObject();
        cJSON_AddNumberToObject(satellite, "id", valid_sats[i].satellite_id);
        cJSON_AddNumberToObject(satellite, "snr", valid_sats[i].snr);
        cJSON_AddItemToArray(topSatellites, satellite);
        
//        COMM_LOG_DEBUG("[COMM][STA] Reporting satellite: ID=%d, SNR=%d\r\n", 
//                 valid_sats[i].satellite_id, valid_sats[i].snr);
    }
    
    return topSatellites;
}

/**
 * @brief 动态构建 deviceState JSON
 * @param includeGPS  true=GPS已定位可含数据, false=GPS弱/无信号(只含isEphemeris)
 * @return cJSON*    deviceState JSON对象
 *
 * 字段规则:
 *   dataType=1(被动查询): 全字段, includeGPS=true强制
 *   dataType=2(周期主动): 全字段, includeGPS=定位状态, lat/lng仅在坐标变化时输出
 *   dataType=3(即时变化): 仅发 deviceStateMask 指定字段
 *
 * flags(告警标志): 始终包含, 0 表示无告警
 * GPS策略: isEphemeris始终包含, lat/lng仅在 gpsCoordsChanged=true 时输出
 */
static cJSON* build_device_state_json(bool includeGPS)
{
    cJSON *deviceStateData = cJSON_CreateObject();
    uint16_t mask = b_message_data.deviceStateMask;
    bool isInstant = (b_message_data.dataType == 3);

    // === flags(告警标志): 始终包含 ===
    cJSON_AddNumberToObject(deviceStateData, "flags", b_message_data.deviceWarningFlags);

    // === 时间戳 ===
    if (!isInstant || (mask & DEVICE_FIELD_TIMESTAMP)) {
        cJSON_AddNumberToObject(deviceStateData, "timestamp", b_message_data.device_timestamp);
    }

    // === dataType 标识 ===
    cJSON_AddNumberToObject(deviceStateData, "dataType", b_message_data.dataType);

    // === 电池 ===
    if (!isInstant) {
        cJSON_AddNumberToObject(deviceStateData, "battery", b_message_data.device_battery);
    } else if (mask & DEVICE_FIELD_BATTERY) {
        cJSON_AddNumberToObject(deviceStateData, "battery", b_message_data.device_battery);
    }

    // === 模式 ===
    if (!isInstant) {
        cJSON_AddNumberToObject(deviceStateData, "currentMode", b_message_data.device_currentMode);
    } else if (mask & DEVICE_FIELD_MODE) {
        cJSON_AddNumberToObject(deviceStateData, "currentMode", b_message_data.device_currentMode);
    }

    // === 运动状态 ===
    if (!isInstant) {
        cJSON_AddNumberToObject(deviceStateData, "motionState", b_message_data.device_motionState);
    } else if (mask & DEVICE_FIELD_MOTION) {
        cJSON_AddNumberToObject(deviceStateData, "motionState", b_message_data.device_motionState);
    }

    // === 步数差 ===
    if (!isInstant) {
        cJSON_AddNumberToObject(deviceStateData, "stepDiffer", b_message_data.device_stepDiffer);
    } else if (mask & DEVICE_FIELD_STEP) {
        cJSON_AddNumberToObject(deviceStateData, "stepDiffer", b_message_data.device_stepDiffer);
    }

    // === GPS: isEphemeris始终含, lat/lng仅在坐标变化时含 ===
    bool wantGPS;
    if (isInstant) {
        wantGPS = (mask & DEVICE_FIELD_GPS) != 0;
    } else {
        wantGPS = includeGPS;
    }
    if (wantGPS) {
        cJSON *gpsData = cJSON_CreateObject();
        cJSON_AddNumberToObject(gpsData, "isEphemeris", b_message_data.position_quality);
        if (b_message_data.gpsCoordsChanged) {
            cJSON_AddStringToObject(gpsData, "latitude", (char *)b_message_data.latitude);
            cJSON_AddStringToObject(gpsData, "longitude", (char *)b_message_data.longitude);
        }
        cJSON *topSatellites = CreateSatellitesArray();
        cJSON_AddItemToObject(gpsData, "topSatellites", topSatellites);
        cJSON_AddItemToObject(deviceStateData, "gps", gpsData);
    }

    // === 信号强度 ===
    if (!isInstant) {
        cJSON_AddNumberToObject(deviceStateData, "cat1Csq", b_message_data.device_cat1Csq);
    } else if (mask & DEVICE_FIELD_CSQ) {
        cJSON_AddNumberToObject(deviceStateData, "cat1Csq", b_message_data.device_cat1Csq);
    }

    // === 版本号: 始终包含 ===
    cJSON_AddStringToObject(deviceStateData, "version", DEVICE_VERSION);

    // === 组装外层 ===
    cJSON *deviceStateValue = cJSON_CreateObject();
    cJSON_AddNumberToObject(deviceStateValue, "ds", b_message_data.dataResourse);
    cJSON_AddStringToObject(deviceStateValue, "cmd", "deviceState");
    cJSON_AddItemToObject(deviceStateValue, "data", deviceStateData);
    cJSON_AddNumberToObject(deviceStateValue, "ts", b_message_data.device_timestamp);
    if (b_message_data.dataType != 1) {
        cJSON_AddStringToObject(deviceStateValue, "reqId", "req_null");
    } else {
        cJSON_AddStringToObject(deviceStateValue, "reqId", (char *)b_message_data.reqId);
    }

    return deviceStateValue;
}

/* ============ V1.6: 状态位转逗号串 + 即时上报差分组包 ============ */
static void build_state_string(const DeviceStateFlags_t *flags, char *out, size_t outlen)
{
    snprintf(out, outlen, "%d,%d,%d,%d,%d,%d", flags->battery, flags->charge, flags->fence, flags->posture, flags->mode, flags->home);
}
static cJSON *build_instant_device_state_data(uint32_t mask)
{
    cJSON *data = cJSON_CreateObject();
    char st[32];
    if (mask & 0x01) { build_state_string(&g_instant_report_state, st, sizeof(st)); cJSON_AddStringToObject(data, "state", st); }
    if (mask & 0x02) { cJSON_AddNumberToObject(data, "battery", b_message_data.device_battery); }
    if (mask & 0x04) { cJSON *g = cJSON_CreateObject(); cJSON_AddStringToObject(g, "latitude", (char *)b_message_data.latitude); cJSON_AddStringToObject(g, "longitude", (char *)b_message_data.longitude); cJSON_AddNumberToObject(g, "isEphemeris", b_message_data.position_quality); cJSON_AddItemToObject(data, "gps", g); }
    if (mask & 0x08) { cJSON_AddNumberToObject(data, "cat1Csq", b_message_data.device_cat1Csq); }
    if (mask & 0x10) { cJSON_AddNumberToObject(data, "stepDiffer", b_message_data.device_stepDiffer); }
    return data;
}

int data_upStream_handler(char **deviceStr, RESPONSE_TYPES type)
{
    int ret = 0;

	cJSON *payload = NULL;
	
    //根据不同的type构造独立的一条json消息
    switch(type)
    {
//        case TRACK_REPORT_TYPE_DEVICE_STATE: 	//被动查询: 整机运行状态（含全部字段）
//			UpdateCurrentDeviceState();
//			payload = build_device_state_json(true);  // includeGPS=true, 被动查询强制包含GPS
//            break;
//		case AUTO_REPORT_TYPE_DEVICE_STATE: 	//主动上报: 整机运行状态（GPS按定位状态动态裁剪）
//			{
//				// dataType=2(周期上报): 更新全字段 + GPS按定位状态动态裁剪
//				// dataType=3(即时变化): 跳过UpdateAuto, 由调用方预填b_message_data仅变化字段
//				if (b_message_data.dataType != 3) {
//					UpdateAutoDeviceState();
//				}
//				bool includeGPS = (b_message_data.gps_status == CHANGE_STATUS_POSITION);
//				payload = build_device_state_json(includeGPS);
//			}
//            break;
		case TRACK_REPORT_TYPE_DEVICE_STATE: 	//整机运行状态(查询回复, V1.6)
		{
			UpdateCurrentDeviceState();
			char stateStr[32];
			build_state_string(&b_message_data.device_stateFlags, stateStr, sizeof(stateStr));
			cJSON *gpsData = cJSON_CreateObject();
			cJSON_AddStringToObject(gpsData, "latitude", (char *)b_message_data.latitude);
			cJSON_AddStringToObject(gpsData, "longitude", (char *)b_message_data.longitude);
			cJSON_AddNumberToObject(gpsData, "isEphemeris", b_message_data.position_quality);
			cJSON *deviceStateData = cJSON_CreateObject();
			cJSON_AddStringToObject(deviceStateData, "state", stateStr);
			cJSON_AddNumberToObject(deviceStateData, "battery", b_message_data.device_battery);
			cJSON_AddNumberToObject(deviceStateData, "stepDiffer", b_message_data.device_stepDiffer);
			cJSON_AddItemToObject(deviceStateData, "gps", gpsData);
			cJSON_AddNumberToObject(deviceStateData, "cat1Csq", b_message_data.device_cat1Csq);
			cJSON_AddStringToObject(deviceStateData, "version", DEVICE_VERSION);
			cJSON *deviceStateValue = cJSON_CreateObject();
			cJSON_AddNumberToObject(deviceStateValue, "ds", b_message_data.dataResourse);
			cJSON_AddStringToObject(deviceStateValue, "cmd", "deviceState");
			cJSON_AddItemToObject(deviceStateValue, "data", deviceStateData);
			cJSON_AddNumberToObject(deviceStateValue, "ts", b_message_data.device_timestamp);
			cJSON_AddStringToObject(deviceStateValue, "reqId", (char *)b_message_data.reqId);
			payload = deviceStateValue;
			break;
		}
		case AUTO_REPORT_TYPE_DEVICE_STATE: 	//主动上报(V1.6: 无version/reqId)
		{
			UpdateAutoDeviceState();
			char autoStateStr[32];
			build_state_string(&b_message_data.device_stateFlags, autoStateStr, sizeof(autoStateStr));
			cJSON *autogpsData = cJSON_CreateObject();
			cJSON_AddStringToObject(autogpsData, "latitude", (char *)b_message_data.latitude);
			cJSON_AddStringToObject(autogpsData, "longitude", (char *)b_message_data.longitude);
			cJSON_AddNumberToObject(autogpsData, "isEphemeris", b_message_data.position_quality);
			cJSON *autodeviceStateData = cJSON_CreateObject();
			cJSON_AddStringToObject(autodeviceStateData, "state", autoStateStr);
			cJSON_AddNumberToObject(autodeviceStateData, "battery", b_message_data.device_battery);
			cJSON_AddNumberToObject(autodeviceStateData, "stepDiffer", b_message_data.device_stepDiffer);
			cJSON_AddItemToObject(autodeviceStateData, "gps", autogpsData);
			cJSON_AddNumberToObject(autodeviceStateData, "cat1Csq", b_message_data.device_cat1Csq);
			cJSON *autodeviceStateValue = cJSON_CreateObject();
			cJSON_AddNumberToObject(autodeviceStateValue, "ds", b_message_data.dataResourse);
			cJSON_AddStringToObject(autodeviceStateValue, "cmd", "deviceState");
			cJSON_AddItemToObject(autodeviceStateValue, "data", autodeviceStateData);
			cJSON_AddNumberToObject(autodeviceStateValue, "ts", b_message_data.device_timestamp);
			payload = autodeviceStateValue;
			break;
		}
		case TRACK_REPORT_TYPE_DEVICE_INFO:		//设备信息(V1.6: cmd->deviceInfo, +hardVersion/sn/model)
		{
			UpdateCurrentDeviceInfo();
			cJSON *deviceInformationData = cJSON_CreateObject();
			cJSON_AddStringToObject(deviceInformationData, "version", DEVICE_VERSION);
			cJSON_AddStringToObject(deviceInformationData, "hardVersion", (char *)b_message_data.device_hardVersion);
			cJSON_AddStringToObject(deviceInformationData, "firmware", DIS_SOFT_VERSION);
			cJSON_AddStringToObject(deviceInformationData, "mac", (char *)b_message_data.device_bleMAC);
			cJSON_AddStringToObject(deviceInformationData, "esimIccid", (char *)b_message_data.device_esimIccid);
			cJSON_AddStringToObject(deviceInformationData, "gnss", (char *)b_message_data.device_gnss);
			cJSON_AddStringToObject(deviceInformationData, "cat1Sn", (char *)b_message_data.device_cat1Sn);
			cJSON_AddStringToObject(deviceInformationData, "sn", (char *)b_message_data.device_sn);
			cJSON_AddStringToObject(deviceInformationData, "model", (char *)b_message_data.device_model);
			cJSON *deviceInformationValue = cJSON_CreateObject();
			cJSON_AddNumberToObject(deviceInformationValue, "ds", b_message_data.dataResourse);
			cJSON_AddStringToObject(deviceInformationValue, "cmd", "deviceInfo");
			cJSON_AddItemToObject(deviceInformationValue, "data", deviceInformationData);
			cJSON_AddNumberToObject(deviceInformationValue, "ts", b_message_data.device_timestamp);
			cJSON_AddStringToObject(deviceInformationValue, "reqId", (char *)b_message_data.reqId);
			payload = deviceInformationValue;
			break;
		}
		case TRACK_REPORT_TYPE_ENODEB_INFO:		//基站信息
		{
			UpdateCurrenteNodeBInfo();
			
			cJSON *eNodeBInfoData = cJSON_CreateObject();
			
			// ========== 服务小区信息 ==========
			// 字符串类型（直接从b_message_data获取）
			cJSON_AddStringToObject(eNodeBInfoData, "cell_rat", b_message_data.cell_rat);
			cJSON_AddStringToObject(eNodeBInfoData, "cell_duplex", b_message_data.cell_duplex);
			
			// MCC/MNC
			cJSON_AddNumberToObject(eNodeBInfoData, "mcc", b_message_data.mcc);
			cJSON_AddNumberToObject(eNodeBInfoData, "mnc", b_message_data.mnc);
			
			// 运营商名称
			cJSON_AddStringToObject(eNodeBInfoData, "carrier", b_message_data.carrier);
			
			// cell_id
			cJSON_AddStringToObject(eNodeBInfoData, "cell_id", b_message_data.cell_id);
			
			// 整型数据
			cJSON_AddNumberToObject(eNodeBInfoData, "cell_earfcn", b_message_data.cell_earfcn);
			
			cJSON_AddStringToObject(eNodeBInfoData, "cell_pcid", b_message_data.cell_pcid);
			cJSON_AddNumberToObject(eNodeBInfoData, "cell_band", b_message_data.cell_band);
			
			// tac
			cJSON_AddStringToObject(eNodeBInfoData, "cell_tac", b_message_data.cell_tac);
			
			// 信号强度相关
			cJSON_AddNumberToObject(eNodeBInfoData, "cell_rsrp", b_message_data.cell_rsrp);
			cJSON_AddNumberToObject(eNodeBInfoData, "cell_rsrq", b_message_data.cell_rsrq);
			cJSON_AddNumberToObject(eNodeBInfoData, "cell_rssi", b_message_data.cell_rssi);
			cJSON_AddNumberToObject(eNodeBInfoData, "cell_sinr", b_message_data.cell_sinr);
			/* V1.6: 删除临近小区(neighbour_*)组包 */
			
			cJSON *eNodeBInfoValue = cJSON_CreateObject();
			cJSON_AddNumberToObject(eNodeBInfoValue, "ds", b_message_data.dataResourse);
			cJSON_AddStringToObject(eNodeBInfoValue, "cmd", "eNodeBInformation");
			cJSON_AddItemToObject(eNodeBInfoValue, "data", eNodeBInfoData);
			cJSON_AddNumberToObject(eNodeBInfoValue, "ts", b_message_data.device_timestamp);
			cJSON_AddStringToObject(eNodeBInfoValue, "reqId", (char *)b_message_data.reqId);

			payload = eNodeBInfoValue;
			break;
		}
		case TRACK_REPORT_TYPE_AUDIO_RECV:  		//语音文件接收
			UpdateCurrentAudioFile();
			cJSON *audioRecvData = cJSON_CreateObject();
				cJSON_AddStringToObject(audioRecvData, "file", "Audio");
				cJSON_AddNumberToObject(audioRecvData, "length", 10);
				cJSON_AddNumberToObject(audioRecvData, "errorCode", b_message_data.device_exeErrorCode);
			cJSON *audioRecvValue = cJSON_CreateObject();
				cJSON_AddNumberToObject(audioRecvValue, "ds", b_message_data.dataResourse);
				cJSON_AddStringToObject(audioRecvValue, "cmd", "audioRecv");
				cJSON_AddItemToObject(audioRecvValue, "data", audioRecvData);
				cJSON_AddNumberToObject(audioRecvValue, "ts",  b_message_data.device_timestamp);
				cJSON_AddStringToObject(audioRecvValue, "reqId", (char *)b_message_data.reqId);
			
			payload = audioRecvValue;
			break;
		case TRACK_REPORT_TYPE_SYSTEM_TIME:
			convert_systime();
			/*同步时间*/
			cJSON *aSystimeData = cJSON_CreateObject();
				cJSON_AddNumberToObject(aSystimeData, "errorCode", b_message_data.device_exeErrorCode);
				
			cJSON *aSystimeValue = cJSON_CreateObject();
				cJSON_AddNumberToObject(aSystimeValue, "ds", b_message_data.dataResourse);
				cJSON_AddStringToObject(aSystimeValue, "cmd", "timeSync");
				cJSON_AddItemToObject(aSystimeValue, "data", aSystimeData);
				cJSON_AddNumberToObject(aSystimeValue, "ts",  b_message_data.device_timestamp);
				cJSON_AddStringToObject(aSystimeValue, "reqId", (char *)b_message_data.reqId);

			payload = aSystimeValue;
			break;
		case TRACK_REPORT_TYPE_WIFI_LIST:
			UpdateCurrentData();	
			
			// 创建data对象
			cJSON *wifiScanData = cJSON_CreateObject();
			cJSON_AddNumberToObject(wifiScanData, "wifiCount", b_message_data.device_wifiScanData.wifi_count);
			
			cJSON *networksArray = cJSON_CreateArray();
			
			for (int i = 0; i < b_message_data.device_wifiScanData.wifi_count; i++) {
				// 调试每个wifi项
				COMM_LOG_DEBUG("Processing wifi %d: ssid='%s'\n", i, b_message_data.device_wifiScanData.wifi_list[i].ssid);
				
				cJSON *wifiItem = cJSON_CreateObject();
				cJSON_AddStringToObject(wifiItem, "ssid", b_message_data.device_wifiScanData.wifi_list[i].ssid);
				cJSON_AddStringToObject(wifiItem, "mac", b_message_data.device_wifiScanData.wifi_list[i].mac);
				cJSON_AddStringToObject(wifiItem, "rssi", b_message_data.device_wifiScanData.wifi_list[i].rssi);
				
				cJSON_AddItemToArray(networksArray, wifiItem);
			}
			
			// 将networks数组添加到data对象
			cJSON_AddItemToObject(wifiScanData, "networks", networksArray);
			
			cJSON *aWifiValue = cJSON_CreateObject();
			cJSON_AddNumberToObject(aWifiValue, "ds", b_message_data.dataResourse);
			cJSON_AddStringToObject(aWifiValue, "cmd", "wifiScan");
			cJSON_AddItemToObject(aWifiValue, "data", wifiScanData);
			cJSON_AddNumberToObject(aWifiValue, "ts", b_message_data.device_timestamp);
			cJSON_AddStringToObject(aWifiValue, "reqId", (char *)b_message_data.reqId);

			payload = aWifiValue;
			break;
		case TRACK_REPORT_TYPE_WARNING_STATE:		//异常模式上报
			UpdateCurrentWarningState();
			cJSON *warningStateData = cJSON_CreateObject();
				cJSON_AddNumberToObject(warningStateData, "battery", b_message_data.device_battery);
				cJSON_AddNumberToObject(warningStateData, "currentMode", b_message_data.device_currentMode);
				
			cJSON *warningStateValue = cJSON_CreateObject();
				cJSON_AddNumberToObject(warningStateValue, "ds", b_message_data.dataResourse);
				cJSON_AddStringToObject(warningStateValue, "cmd", "warningState");
				cJSON_AddItemToObject(warningStateValue, "data", warningStateData);
				cJSON_AddNumberToObject(warningStateValue, "ts",  b_message_data.device_timestamp);
				cJSON_AddStringToObject(warningStateValue, "reqId", (char *)b_message_data.reqId);

			payload = warningStateValue;
			break;
		/* ================ V1.6 新增上报组包 ================ */
		case TRACK_REPORT_TYPE_VOICE_GET:
		{
			cJSON *vg = cJSON_CreateObject();
			cJSON_AddNumberToObject(vg, "voiceID", b_message_data.appControl_voiceId);
			cJSON_AddNumberToObject(vg, "index", b_message_data.appControl_voiceIndex);
			cJSON *vgv = cJSON_CreateObject();
			cJSON_AddNumberToObject(vgv, "ds", b_message_data.dataResourse);
			cJSON_AddStringToObject(vgv, "cmd", "voiceGet");
			cJSON_AddItemToObject(vgv, "data", vg);
			cJSON_AddNumberToObject(vgv, "ts", b_message_data.device_timestamp);
			cJSON_AddStringToObject(vgv, "reqId", (char *)b_message_data.reqId);
			payload = vgv; break;
		}
		case TRACK_REPORT_TYPE_WIFI_GET:
		{
			cJSON *wg = cJSON_CreateObject();
			cJSON_AddNumberToObject(wg, "state", b_message_data.device_wifiConfigState);
			cJSON_AddStringToObject(wg, "ssid", (b_message_data.appControl_wifiSsid ? (char *)b_message_data.appControl_wifiSsid : ""));
			cJSON_AddStringToObject(wg, "mac", (b_message_data.appControl_wifiMac ? (char *)b_message_data.appControl_wifiMac : ""));
			cJSON_AddNumberToObject(wg, "lat", b_message_data.device_wifiLat);
			cJSON_AddNumberToObject(wg, "lon", b_message_data.device_wifiLon);
			cJSON *wgv = cJSON_CreateObject();
			cJSON_AddNumberToObject(wgv, "ds", b_message_data.dataResourse);
			cJSON_AddStringToObject(wgv, "cmd", "wifiSsidGet");
			cJSON_AddItemToObject(wgv, "data", wg);
			cJSON_AddNumberToObject(wgv, "ts", b_message_data.device_timestamp);
			cJSON_AddStringToObject(wgv, "reqId", (char *)b_message_data.reqId);
			payload = wgv; break;
		}
		case TRACK_REPORT_TYPE_GEOFENCE_GET:
		{
			cJSON *fg = cJSON_CreateObject();
			cJSON_AddNumberToObject(fg, "fenceId", b_message_data.device_geofence.fenceId);
			cJSON_AddNumberToObject(fg, "switch", b_message_data.device_geofence.switch_on);
			cJSON_AddNumberToObject(fg, "isSet", b_message_data.device_geofence.isSet);
			cJSON *fsa = cJSON_CreateArray(); 
			cJSON *fda = cJSON_CreateArray();
			for (int i = 0; i < GEOFENCE_POINT_MAX; i++) {
				cJSON *ps = cJSON_CreateObject(); cJSON_AddNumberToObject(ps, "lat", b_message_data.device_geofence.fenceS[i].lat); cJSON_AddNumberToObject(ps, "lon", b_message_data.device_geofence.fenceS[i].lon); cJSON_AddItemToArray(fsa, ps);
				cJSON *pd = cJSON_CreateObject(); cJSON_AddNumberToObject(pd, "lat", b_message_data.device_geofence.fenceD[i].lat); cJSON_AddNumberToObject(pd, "lon", b_message_data.device_geofence.fenceD[i].lon); cJSON_AddItemToArray(fda, pd);
			}
			cJSON_AddItemToObject(fg, "fenceS", fsa); 
			cJSON_AddItemToObject(fg, "fenceD", fda);
			cJSON *fgv = cJSON_CreateObject();
			cJSON_AddNumberToObject(fgv, "ds", b_message_data.dataResourse);
			cJSON_AddStringToObject(fgv, "cmd", "geofenceGet");
			cJSON_AddItemToObject(fgv, "params", fg);
			cJSON_AddNumberToObject(fgv, "ts", b_message_data.device_timestamp);
			cJSON_AddStringToObject(fgv, "reqId", (char *)b_message_data.reqId);
			payload = fgv; break;
		}
		case TRACK_REPORT_TYPE_GEOFENCE_CONFIG:
		case TRACK_REPORT_TYPE_DEVICE_LIGHT:
		case TRACK_REPORT_TYPE_DEVICE_CONFIG:
		{
			const char *cn = "geofenceConfig";
			if (type == TRACK_REPORT_TYPE_DEVICE_LIGHT) cn = "deviceLight";
			else if (type == TRACK_REPORT_TYPE_DEVICE_CONFIG) cn = "deviceConfig";
			cJSON *ad = cJSON_CreateObject();
			cJSON_AddNumberToObject(ad, "errorCode", b_message_data.device_exeErrorCode);
			cJSON *av = cJSON_CreateObject();
			cJSON_AddNumberToObject(av, "ds", b_message_data.dataResourse);
			cJSON_AddStringToObject(av, "cmd", cn);
			cJSON_AddItemToObject(av, "data", ad);
			cJSON_AddNumberToObject(av, "ts", b_message_data.device_timestamp);
			cJSON_AddStringToObject(av, "reqId", (char *)b_message_data.reqId);
			payload = av; break;
		}
		case TRACK_REPORT_TYPE_HEALTH_INFO:
		{
			cJSON *hd = cJSON_CreateObject();
			cJSON_AddNumberToObject(hd, "startTs", g_health_window_start_ts);
			cJSON_AddStringToObject(hd, "actionCode", g_posture_seq);
			cJSON_AddNumberToObject(hd, "stepDiffer", b_message_data.device_stepDiffer);
			cJSON *hv = cJSON_CreateObject();
			cJSON_AddNumberToObject(hv, "ds", b_message_data.dataResourse);
			cJSON_AddStringToObject(hv, "cmd", "healthInfo");
			cJSON_AddItemToObject(hv, "data", hd);
			cJSON_AddNumberToObject(hv, "ts", b_message_data.device_timestamp);
			/* V1.6: 定频主动上报无reqId */
			payload = hv; break;
		}
		case INSTANT_REPORT_TYPE_DEVICE_STATE:
		{
			cJSON *id = build_instant_device_state_data(g_instant_changed_mask);
			cJSON *iv = cJSON_CreateObject();
			cJSON_AddNumberToObject(iv, "ds", b_message_data.dataResourse);
			cJSON_AddStringToObject(iv, "cmd", "deviceState");
			cJSON_AddItemToObject(iv, "data", id);
			cJSON_AddNumberToObject(iv, "ts", b_message_data.device_timestamp);
			/* V1.6: 主动上报不带reqId(reqId仅APP/云端查询时下发, 设备回复时沿用) */
			payload = iv; break;
		}
		case TRACK_REPORT_TYPE_ERRORCODE:			//回复结果码
				UpdateCurrentErrorCode();
			cJSON *errorCodeData = cJSON_CreateObject();
				cJSON_AddNumberToObject(errorCodeData, "errorCode", b_message_data.device_exeErrorCode);
				
			cJSON *errorCodeValue = cJSON_CreateObject();
				cJSON_AddNumberToObject(errorCodeValue, "ds", b_message_data.dataResourse);
				cJSON_AddStringToObject(errorCodeValue, "cmd", (char *)b_message_data.cmd);
				cJSON_AddItemToObject(errorCodeValue, "data", errorCodeData);
				cJSON_AddNumberToObject(errorCodeValue, "ts",  b_message_data.device_timestamp);
				cJSON_AddStringToObject(errorCodeValue, "reqId", (char *)b_message_data.reqId);

			payload = errorCodeValue;
				break;
		case PRODUCT_REPORT_TYPE_ERRORCODE:			//回复产测结果码
				productErrorCode();
			cJSON *productErrorCodeData = cJSON_CreateObject();
				cJSON_AddNumberToObject(productErrorCodeData, "errorCode", b_message_data.product_exeErrorCode);
				
			cJSON *productErrorCodeValue = cJSON_CreateObject();
				cJSON_AddNumberToObject(productErrorCodeValue, "ds", b_message_data.dataResourse);
				cJSON_AddStringToObject(productErrorCodeValue, "cmd", (char *)b_message_data.cmd);
				cJSON_AddItemToObject(productErrorCodeValue, "data", productErrorCodeData);

			payload = productErrorCodeValue;
				break;
		case PRODUCT_REPORT_TYPE_SN:			//回复SN
				productGetSn();
			cJSON *productsn = cJSON_CreateObject();
				cJSON_AddStringToObject(productsn, "sn", (char *)b_message_data.device_esimIccid);
				
			cJSON *productsnValue = cJSON_CreateObject();
				cJSON_AddNumberToObject(productsnValue, "ds", b_message_data.dataResourse);
				cJSON_AddStringToObject(productsnValue, "cmd", (char *)b_message_data.cmd);
				cJSON_AddItemToObject(productsnValue, "data", productsn);

			payload = productsnValue;
				break;
		case PRODUCT_REPORT_TYPE_CAT1_VER:			//CAT1版本号
				productCat1Version();
			cJSON *productVersion = cJSON_CreateObject();
				cJSON_AddStringToObject(productVersion, "version", (char *)b_message_data.appProduct_cat1Version);
				
			cJSON *productVersionValue = cJSON_CreateObject();
				cJSON_AddNumberToObject(productVersionValue, "ds", b_message_data.dataResourse);
				cJSON_AddStringToObject(productVersionValue, "cmd", (char *)b_message_data.cmd);
				cJSON_AddItemToObject(productVersionValue, "data", productVersion);

			payload = productVersionValue;
				break;
	case PRODUCT_REPORT_TYPE_GPS_VER:			//GPS版本和搜星标志
				productGpsVersion();
			cJSON *productGps = cJSON_CreateObject();
				cJSON_AddStringToObject(productGps, "version", (char *)b_message_data.appProduct_gpsVersion);
				cJSON_AddNumberToObject(productGps, "isEphemeris", b_message_data.gps_status);
			cJSON *productGpsValue = cJSON_CreateObject();
				cJSON_AddNumberToObject(productGpsValue, "ds", b_message_data.dataResourse);
				cJSON_AddStringToObject(productGpsValue, "cmd", "productGps"); //(char *)b_message_data.cmd
				cJSON_AddItemToObject(productGpsValue, "data", productGps);

			payload = productGpsValue;
				break;
		case PRODUCT_REPORT_TYPE_LTE_CONN:			//LTE连接结果
				productErrorCode();
			cJSON *productLteErrorCodeData = cJSON_CreateObject();
				cJSON_AddNumberToObject(productLteErrorCodeData, "errorCode", b_message_data.product_exeErrorCode);
				
			cJSON *productLteErrorCodeValue = cJSON_CreateObject();
				cJSON_AddNumberToObject(productLteErrorCodeValue, "ds", b_message_data.dataResourse);
				cJSON_AddStringToObject(productLteErrorCodeValue, "cmd", "productLteConn");  //(char *)b_message_data.cmd
				cJSON_AddItemToObject(productLteErrorCodeValue, "data", productLteErrorCodeData);

			payload = productLteErrorCodeValue;
				break;
		case PRODUCT_REPORT_TYPE_BLEMAC:			//回复蓝牙MAC地址
				productGetMac();
			cJSON *productMac = cJSON_CreateObject();
				cJSON_AddStringToObject(productMac, "mac", (char *)b_message_data.deviceProduct_bleMAC);
				
			cJSON *productMacValue = cJSON_CreateObject();
				cJSON_AddNumberToObject(productMacValue, "ds", b_message_data.dataResourse);
				cJSON_AddStringToObject(productMacValue, "cmd", (char *)b_message_data.cmd);
				cJSON_AddItemToObject(productMacValue, "data", productMac);

			payload = productMacValue;
				break;
		case PRODUCT_REPORT_TYPE_DEVICEMSG:			//获取设备参数
				productGetMsg();
			cJSON *productMsg = cJSON_CreateObject();
				cJSON_AddStringToObject(productMsg, "mac", (char *)b_message_data.deviceProduct_bleMAC);
				cJSON_AddStringToObject(productMsg, "version", DEVICE_VERSION);
				cJSON_AddNumberToObject(productMsg, "battery", b_message_data.device_battery);
				cJSON_AddStringToObject(productMsg, "sn", (char *)b_message_data.device_esimIccid);
		
			cJSON *productMsgValue = cJSON_CreateObject();
				cJSON_AddNumberToObject(productMsgValue, "ds", b_message_data.dataResourse);
				cJSON_AddStringToObject(productMsgValue, "cmd", (char *)b_message_data.cmd);
				cJSON_AddItemToObject(productMsgValue, "data", productMsg);

			payload = productMsgValue;
				break;
		
		default:
			
        break;
		  }

#if 1
	char *jsonData = cJSON_PrintUnformatted(payload);

    if ((jsonData == NULL))
    {
        goto EXIT;
    }
	
    // 添加 malloc 失败检查
	*deviceStr = (char *)DEMO_BT_Malloc(strlen(jsonData) + 1);
	if (*deviceStr == NULL)
	{
		COMM_LOG_DEBUG("[COMM][ERR] malloc failed for size: %d\r\n", strlen(jsonData) + 1);
		goto EXIT;
	}

	memset_s(*deviceStr, strlen(jsonData) + 1, 0, strlen(jsonData) + 1);
	memcpy_s(*deviceStr, strlen(jsonData) + 1, jsonData, strlen(jsonData));

    ret = 1;
	
EXIT:
    if(payload != NULL)
    {
//		COMM_LOG_DEBUG("payload\n");
        cJSON_Delete(payload);
		
    }
    if(jsonData != NULL)
    {
//		COMM_LOG_DEBUG("jsonData\n");
        DEMO_BT_Free(jsonData);
        jsonData = NULL;
    }

    return ret;
#else
    size_t jsonLength = 0;
// 第一步：计算需要的缓冲区大小
    jsonLength = cJSON_GetArraySize(payload) * 256;
    char *tempBuffer = (char *)DEMO_BT_Malloc(jsonLength);
    if (tempBuffer == NULL) {
        COMM_LOG_DEBUG("malloc for temp buffer failed\n");
        goto EXIT;
    }

    // 第二步：使用预分配缓冲区打印JSON
    int printResult = cJSON_PrintPreallocated(payload, tempBuffer, jsonLength, 0);
    if (!printResult) {
        COMM_LOG_DEBUG("cJSON_PrintPreallocated failed, buffer too small?\n");
        DEMO_BT_Free(tempBuffer);
        goto EXIT;
    }

    // 第三步：获取实际长度并分配目标内存
    size_t actualLength = strlen(tempBuffer);
    *deviceStr = (char *)DEMO_BT_Malloc(actualLength + 1);
    if (*deviceStr == NULL) {
        COMM_LOG_DEBUG("DEMO_BT_Malloc failed for size: %d", actualLength + 1);
        DEMO_BT_Free(tempBuffer);
        goto EXIT;
    }

    // 第四步：复制数据到目标内存
    memset(*deviceStr, 0, actualLength + 1);
    memcpy(*deviceStr, tempBuffer, actualLength);
	(*deviceStr)[actualLength] = '\0'; // 确保字符串正确终止
	
    // 释放临时缓冲区
    DEMO_BT_Free(tempBuffer);

//    COMM_LOG_DEBUG("JSON length: %d\n", actualLength);
//    size_t freeHeapAfter = xPortGetFreeHeapSize();
//    COMM_LOG_DEBUG("After JSON - Free heap: %u bytes\n", freeHeapAfter);

    ret = 1;

EXIT:
    if (payload != NULL) {
        cJSON_Delete(payload);
    }
    return ret;
#endif
}


static int SendDataPacket(uint8_t *data, size_t len, TASK_ID_T dest_task_id, TASK_CMD_T command)
{
    TaskInfo_t *my_task_info = GetTaskInfo(COMM_TASK_ID);
	int ret = 0;
	
    char *temp_data = (char *)DEMO_BT_Malloc(len);
    if (temp_data == NULL) {
        COMM_LOG_DEBUG("[COMM][ERR] Memory failed\r\n");
        return 0;
    }
    
    // 拷贝数据
    memset(temp_data, 0, len);
    memcpy(temp_data, data, len);
	
	if(command == TASK_COMM_DATAJSON)
	{
		ble_dataUpStream_subPackage((unsigned char *)temp_data, len);
		
		ret = 1;
	}
	
//    Message_t send_msg = {
//        .source_id = my_task_info->task_id,
//        .dest_id = dest_task_id,
//        .command = command,
//        .data = temp_data,
//        .data_length = len
//    };

//    if(osOK != osMessageQueuePut(GetTaskInfo(dest_task_id)->queue_handle, 
//                     &send_msg, NULL, 0))
//    {
//        DEMO_BT_Free(temp_data);
//       LOG_LOC();
//    }
//    else
//    {
//		
//        ret = 1;
//    }
    
    return ret;
}



static int SendMqttDataPacket(uint8_t *data, size_t len, TASK_ID_T dest_task_id, 
                              TASK_CMD_T command, mqtt_function_t topic)
{
    TaskInfo_t *my_task_info = GetTaskInfo(COMM_TASK_ID);
    int ret = 0;
    
    // 分配内存：topic大小 + 数据长度
    size_t total_len = sizeof(mqtt_function_t) + len;
    uint8_t *temp_data = (uint8_t *)DEMO_BT_Malloc(total_len);
    if (temp_data == NULL) {
        COMM_LOG_DEBUG("[COMM][ERR] Mqtt memory failed\r\n");
        return 0;
    }
    
    // 打包数据：前sizeof(mqtt_function_t)字节放topic，后面放原始数据
    memcpy(temp_data, &topic, sizeof(mqtt_function_t));
    if (data != NULL && len > 0) {
        memcpy(temp_data + sizeof(mqtt_function_t), data, len);
    }
    
    // 发送消息
    Message_t send_msg = {
        .source_id = my_task_info->task_id,
        .dest_id = dest_task_id,
        .command = command,
        .data = temp_data,
        .data_length = total_len
    };

    if (osOK != osMessageQueuePut(GetTaskInfo(dest_task_id)->queue_handle, 
                                  &send_msg, NULL, 0))
    {
        DEMO_BT_Free(temp_data);
        LOG_LOC();
    }
    else
    {
        ret = 1;
    }
    
    return ret;
}

static void SendMessageToTask(TASK_CMD_T commad, TASK_ID_T dest_task_id, void *data)
{
	TaskInfo_t *my_task_info = GetTaskInfo(COMM_TASK_ID);
	
    Message_t send_msg = {
        .source_id = my_task_info->task_id,
        .dest_id = dest_task_id,
		.command = commad,
		.data = data
    };
 
    if(osOK != osMessageQueuePut(GetTaskInfo(dest_task_id)->queue_handle, 
                     &send_msg, NULL, 0))
	{
		LOG_LOC();
	}
}

/***************************************************************
* 函数名称: lte_dataUpStream_handler
* 说    明: LTE端上行数据发送（原始字符串格式）
***************************************************************/
int dataUpStream_handler(RESPONSE_TYPES type, uint16_t dataResourse)
{
	//等待其他任务更新
	osDelay(osMS2TicksRound(1000));
	
    int ret = 0;
    char *jsonStr = NULL;
	b_message_data.dataResourse = dataResourse;  /* ds统一按实际发送链路填(回复场景与请求来源一致, 主动场景防残留) */
    // 组包原始JSON字符串
    data_upStream_handler(&jsonStr, type);
    if (jsonStr == NULL || strlen(jsonStr) == 0) {
        return ret;
    }

    size_t json_len = strlen(jsonStr);
	
    if(dataResourse == DATA_SOURCE_4G)
    {
		COMM_LOG_DEBUG("[COMM][STA] Data source 4G\r\n");
        // 确保发送的是正确的数据段
        SendMqttDataPacket((unsigned char *)jsonStr, json_len, CAT1_UART_TASK_ID, TASK_COMM_DATAJSON, comm_func_topic);
    }
    else if(dataResourse == DATA_SOURCE_BLE)
    {
        COMM_LOG_DEBUG("[COMM][STA] Data source ble\r\n");
        SendDataPacket((unsigned char *)jsonStr, json_len, BLE_SCHEDULE_TASK_ID, TASK_COMM_DATAJSON);
    }

    if (jsonStr != NULL) {
        DEMO_BT_Free(jsonStr);
    }
    
    return ret;
}

int dataDownStream_handler(APP_CONTROL_Type controlType,uint16_t dataResourse)
{
	int ret = 0;

	/* M4充电模式白名单: 只放行主动上报触发和BLE鉴权(通道握手, 不放行则鉴权超时断链, BLE上报通道也没了);
	 * 其余所有APP/云端下发控制指令(震动/模式/音频/查询等, 4G和BLE来的都算)一律不执行不回复。
	 * 解析阶段仅暂存字段无副作用, 在此单点拦截即可; BLE流控信号量在外层照常释放, 框架不受影响 */
	if (CurrentModeDataGet() == CURRENT_MODE_CHARGE
	    && controlType != CONTROL_AUTO_DEVICE_STATE
	    && controlType != CONTROL_SET_DEVICE_AUTH)
	{
		COMM_LOG_DEBUG("[COMM][STA] M4 charge: downlink cmd %d blocked\r\n", controlType);
		return 0;
	}

	switch(controlType)
	{
		case CONTROL_CHECK_DEVICE_STATE:	//查询整机运行状态
			COMM_LOG_DEBUG("[COMM][STA] DEVICE_STATE\r\n");
			//发送消息给pm（电量）
			//SendMessageToTask(TASK_DATA_VAL_UPDATA, PM_TASK_ID, NULL);
			//直接获取电量数据
			
			//发送消息给六轴（运动状态）
			//SendMessageToTask(TASK_DATA_VAL_UPDATA, SENSOR_TASK_ID, NULL);
			//直接获取运动数据
		
			//发送消息给ENTRY
			//SendMessageToTask(TASK_DATA_VAL_UPDATA, ENTRY_TASK_ID, NULL);
			//直接获取工作模式数据
		
			//发送消息给GPS
			//SendMessageToTask(TASK_DATA_VAL_UPDATA, GNSS_UART_TASK_ID, NULL);
			//直接获取GPS数据
			
			//发送消息给4G（信号质量）
//			SendMessageToTask(TASK_DATA_VAL_UPDATA, CAT1_UART_TASK_ID, NULL);
			
			ret = dataUpStream_handler(TRACK_REPORT_TYPE_DEVICE_STATE, dataResourse);
			controlType = CONTROL_NO_TASK;
			break;
		case CONTROL_AUTO_DEVICE_STATE:	//上报整机运行状态
			COMM_LOG_DEBUG("[COMM][STA] AUTO_STATE\r\n");
			
			ret = dataUpStream_handler(AUTO_REPORT_TYPE_DEVICE_STATE, dataResourse);
			controlType = CONTROL_NO_TASK;
			break;
		case CONTROL_CHECK_DEVICE_INFO:		//查询设备信息
			COMM_LOG_DEBUG("[COMM][STA] DEVI_INFO\r\n");
			//发送消息给蓝牙 （MAC）
			//SendMessageToTask(TASK_DATA_DEVICEINFO, BLE_SCHEDULE_TASK_ID, NULL);
			//获取蓝牙MAC的接口
		
			//发送消息给4G（序列号）
//			SendMessageToTask(TASK_DATA_DEVICEINFO, CAT1_UART_TASK_ID, NULL);
			//发送消息给GPS（序列号）
//			SendMessageToTask(TASK_DATA_DEVICEINFO, GNSS_UART_TASK_ID, NULL);
		
			ret = dataUpStream_handler(TRACK_REPORT_TYPE_DEVICE_INFO, dataResourse);
			controlType = CONTROL_NO_TASK;
			break;
		case CONTROL_CHECK_ENODEB_INFO:		//查询基站信息
			COMM_LOG_DEBUG("[COMM][STA] ENOD_INF\r\n");
//			//发送消息给4G（基站信息）
//			SendMessageToTask(TASK_CAT1_QUERY_ENODEB, CAT1_UART_TASK_ID, NULL);
		
			ret = dataUpStream_handler(TRACK_REPORT_TYPE_ENODEB_INFO, dataResourse);
			controlType = CONTROL_NO_TASK;
			break;
		case CONTROL_SET_DEVICE_MODE:		//设置设备模式
			COMM_LOG_DEBUG("[COMM][STA] DEVI_MODE, mode: %d\r\n");
			//发送entry（模式） &b_message_data.appControl_currentMode需要修改：模式以及GPS周期等数据
			if(b_message_data.appControl_currentMode.mode == 2)
			{
				CurrentModeDataSet(CURRENT_MODE_SEARCH_PET);
			}
			else
			{
				CurrentModeDataSet(CURRENT_MODE_STANDARD);
			}
			SendMessageToTask(TASK_SYSTEM_MODE, CAT1_UART_TASK_ID, &b_message_data.appControl_currentMode);
			SendMessageToTask(TASK_SYSTEM_MODE, GNSS_UART_TASK_ID, &b_message_data.appControl_currentMode);
			/* V1.6改动点13: 模式切换重启主动上报定时器(先stop再start), 配置表覆盖周期 */
			{ uint32_t _miv = (b_message_data.appControl_currentMode.mode == 2) ? COMM_AUTO_REPORT_INTERVAL_SEARCH_MS : COMM_AUTO_REPORT_INTERVAL_STANDARD_MS;
			  if (b_message_data.device_config.valid) _miv = (b_message_data.appControl_currentMode.mode == 2) ? (b_message_data.device_config.find * 1000) : (b_message_data.device_config.normal * 1000);
			  CommStartAutoReportTimer(_miv); }
			if(b_message_data.appControl_vibrationSwitch && b_message_data.appControl_vibrationTime > 0)
			{
				//发送震动和灯效控制
				m_motor_set(MOTOR_TIME_RUN,b_message_data.appControl_vibrationTime);
			}
			b_message_data.device_exeErrorCode = 0;
			ret = dataUpStream_handler(TRACK_REPORT_TYPE_ERRORCODE, dataResourse);
			controlType = CONTROL_NO_TASK;
			break;
		case CONTROL_SET_VIBRATE_CMD:			//设置发送震动
			COMM_LOG_DEBUG("[COMM][STA] VIBRATE_CMD\r\n");
			if(b_message_data.appControl_vibrateTime > 0)
			{
				b_message_data.device_exeErrorCode = 0;
				//发送震动和灯效控制
				m_motor_set(MOTOR_TIME_RUN,b_message_data.appControl_vibrateTime);
			}
			else
			{
				b_message_data.device_exeErrorCode = 1;
			}
			ret = dataUpStream_handler(TRACK_REPORT_TYPE_ERRORCODE, dataResourse);
			controlType = CONTROL_NO_TASK;
			break;
		case CONTROL_SET_AUDIO_REALTIME:			//发送实时语音
			if(b_message_data.appControl_audioRealTimeLen > APP_AUDIO_REALTIME_LEN_MAX)
			{
				b_message_data.device_exeErrorCode = 1;
				ret = dataUpStream_handler(TRACK_REPORT_TYPE_ERRORCODE, dataResourse);
			}
			else
			{
				//发送消息给CAT1存储数据
//				SendDataPacket(b_message_data.appControl_audioRealTimeUrl, strlen((char*)b_message_data.appControl_audioRealTimeUrl), CAT1_UART_TASK_ID, TASK_AUDIO_REALTIME);
			}
			controlType = CONTROL_NO_TASK;
			break;
		case CONTROL_SET_AUDIO_CMD:			//设置发送语音播放
			COMM_LOG_DEBUG("[COMM][STA] AUD_CMD\r\n");
//			Audio_Play_Start("/audio/record/tmp.wav",1);
			uint8_t return_data_tmp = 0;
		return_data_tmp = Audio_IFlash_Play_Start(b_message_data.appControl_audioIndex,1);
		
//			return_data_tmp = Audio_Play_Start("/audio/player/p1.wav",1);
			b_message_data.device_exeErrorCode = return_data_tmp;
			ret = dataUpStream_handler(TRACK_REPORT_TYPE_ERRORCODE, dataResourse);
			controlType = CONTROL_NO_TASK;
			break;
		case CONTROL_SET_AUDIO_FILE:		//设置语音文件
			COMM_LOG_DEBUG("[COMM][STA] AUD_FILE\r\n");
			b_message_data.device_exeErrorCode = 0;
			ret = dataUpStream_handler(TRACK_REPORT_TYPE_ERRORCODE, dataResourse);
			controlType = CONTROL_NO_TASK;
			break;
		case CONTROL_SET_AUDIO_RECV:		//设置接收语音文件
			COMM_LOG_DEBUG("[COMM][STA] AUD_RECV\r\n");
			//发送消息给AUDIO（音频文件）
//			SendMessageToTask(TASK_DATA_VAL_UPDATA, AUDIO_TASK_ID);
			
			ret = dataUpStream_handler(TRACK_REPORT_TYPE_AUDIO_RECV, dataResourse);
			controlType = CONTROL_NO_TASK;
			break;
		case CONTROL_SET_NOBARK:			//设置止吠
			COMM_LOG_DEBUG("[COMM][STA] NOBARK\r\n");
			//马达震动数据控制
			
			//灯效控制
			b_message_data.device_exeErrorCode = 0;
			ret = dataUpStream_handler(TRACK_REPORT_TYPE_ERRORCODE, dataResourse);
			controlType = CONTROL_NO_TASK;
			break;
		case CONTROL_SET_TIME_SYNCHRONIZE:	//设置同步时间
			COMM_LOG_DEBUG("[COMM][STA] SYNCHRONIZE\r\n");
			ret = dataUpStream_handler(TRACK_REPORT_TYPE_SYSTEM_TIME, dataResourse);
			controlType = CONTROL_NO_TASK;
			break;
		case CONTROL_SET_WIFI_SCAN:			//设置WiFi指令
			COMM_LOG_DEBUG("[COMM][STA] WIFI_MAC start\r\n");
			SendMessageToTask(TASK_START_WIFISCAN, CAT1_UART_TASK_ID, NULL);
		
			//注释，在接收消息队列里处理
//			ret = dataUpStream_handler(TRACK_REPORT_TYPE_WIFI_LIST, dataResourse);
			controlType = CONTROL_NO_TASK;
			break;
		case CONTROL_SET_WIFI_SSID:			//配置wifi ssid
			COMM_LOG_DEBUG("[COMM][STA] WIFI_SSID\r\n");
			user_bindinfo_set((const char *)b_message_data.appControl_wifiMac, strlen((char *)b_message_data.appControl_wifiMac), UBI_WIFI_MAC_ID);
			user_bindinfo_set((const char *)b_message_data.appControl_wifiSsid, strlen((char *)b_message_data.appControl_wifiSsid), UBI_WIFI_MAC_ID);
			b_message_data.device_exeErrorCode = 0;
			ret = dataUpStream_handler(TRACK_REPORT_TYPE_ERRORCODE, dataResourse);
			controlType = CONTROL_NO_TASK;
			break;
		case CONTROL_SET_DEVICE_AUTH:		//鉴权指令
			COMM_LOG_DEBUG("[COMM][STA] DEVI_AUTH\r\n");
			isBleTsppCommplete = false;
			if(device_auth(b_message_data.authData, strlen((char *)b_message_data.authData)))
			{
				uset_bound_state_set(1);
				b_message_data.device_exeErrorCode = 0;
			}
			else
			{
				b_message_data.device_exeErrorCode = 1;
				device_disconnect();
			}
			ret = dataUpStream_handler(TRACK_REPORT_TYPE_ERRORCODE, dataResourse);
			controlType = CONTROL_NO_TASK;
			break;
		case CONTROL_SET_DEVICE_DELETE:		//设置删除设备
			COMM_LOG_DEBUG("[COMM][STA] DEVI_DELE\r\n");
			isBleTsppCommplete = true;
			if(delete_device_auth())
			{
				b_message_data.device_exeErrorCode = 0;
				uset_bound_state_set(0);
				device_disconnect();
				
			}
			else
			{
				b_message_data.device_exeErrorCode = 1;
			}
			ret = dataUpStream_handler(TRACK_REPORT_TYPE_ERRORCODE, dataResourse);
					/* ============ V1.6 新增控制执行 ============ */
		case CONTROL_QUERY_VOICE:
			b_message_data.appControl_voiceIndex = 0;
			ret = dataUpStream_handler(TRACK_REPORT_TYPE_VOICE_GET, dataResourse);
			controlType = CONTROL_NO_TASK; break;
		case CONTROL_QUERY_WIFI:
			b_message_data.device_wifiConfigState = STATE_HOME_CONFIG_OFF;
			ret = dataUpStream_handler(TRACK_REPORT_TYPE_WIFI_GET, dataResourse);
			controlType = CONTROL_NO_TASK; break;
		case CONTROL_QUERY_FENCE:
			ret = dataUpStream_handler(TRACK_REPORT_TYPE_GEOFENCE_GET, dataResourse);
			controlType = CONTROL_NO_TASK; break;
		case CONTROL_CONFIG_FENCE:
			/* V1.6: 围栏开关无独立数据包, switch字段已在geofenceConfig解析时写入device_geofence.switch_on */
			b_message_data.device_exeErrorCode = 0;
			ret = dataUpStream_handler(TRACK_REPORT_TYPE_GEOFENCE_CONFIG, dataResourse);
			controlType = CONTROL_NO_TASK; break;
		case CONTROL_SET_LIGHT:
			
			if(b_message_data.appControl_lightColor != 0 && b_message_data.appControl_lightTime > 0)
			{
				b_message_data.device_exeErrorCode = 0;
				//TODO: 灯效 API
				
			}
			else
			{
				b_message_data.device_exeErrorCode = 1;
			}
		
			ret = dataUpStream_handler(TRACK_REPORT_TYPE_DEVICE_LIGHT, dataResourse);
			controlType = CONTROL_NO_TASK; break;
		case CONTROL_SET_DEVICE_CONFIG:
			b_message_data.device_exeErrorCode = 0;
			ret = dataUpStream_handler(TRACK_REPORT_TYPE_DEVICE_CONFIG, dataResourse);
			controlType = CONTROL_NO_TASK; break;
			break;
		default:
			break;
	}
	return ret;
}
//ERROR 类型cmd回复
void Production_Result_Report(eProductionReportCod error_code)
{
	test_massage_example = PRODUCTION_TASK_EXAMPLE_NONE;//重置
	b_message_data.product_exeErrorCode = error_code;
	dataUpStream_handler(PRODUCT_REPORT_TYPE_ERRORCODE, b_message_data.dataResourse);
}
void Production_Lte_Result_Report(eProductionReportCod error_code)
{
	test_massage_example = PRODUCTION_TASK_EXAMPLE_NONE;//重置
	b_message_data.product_exeErrorCode = error_code;
	dataUpStream_handler(PRODUCT_REPORT_TYPE_LTE_CONN, b_message_data.dataResourse);
}
//
void Production_Param_Result_Report(RESPONSE_TYPES type)
{
	test_massage_example = PRODUCTION_TASK_EXAMPLE_NONE;//重置
	dataUpStream_handler(type, b_message_data.dataResourse);
}
//

int deviceResetDownStream_handler(PRODUCT_CONTROL_Type productControlType,uint16_t dataResourse)
{
	int ret = 0;
	if((Entry_Task_Run_Mode_Get() != MODE_M4) || (test_massage_example != PRODUCTION_TASK_EXAMPLE_NONE))
	{
		b_message_data.product_exeErrorCode = REPORT_OTHER_ERROR;
		ret = dataUpStream_handler(PRODUCT_REPORT_TYPE_ERRORCODE, dataResourse);
		return ret;
	}
	if(productControlType == PRODUCT_RESET)
	{
		deviceAuthStopTimer();
		safe_unblock_uart_task();
		unblock_cat1_task();
		SendMessageToTask(TASK_FACTORY_RESET, CAT1_UART_TASK_ID, NULL);
		productControlType = PRODUCT_NO_TASK;
		ret = 1;
	}
	return ret;
}

static void usb_updata_flag_set(uint8_t data)
{
	usb_updata_flag = data;
}
uint8_t usb_updata_flag_get(void)
{
	return usb_updata_flag;
}
int productDataDownStream_handler(PRODUCT_CONTROL_Type productControlType,uint16_t dataResourse)
{
	int ret = 0;
	if((Entry_Task_Run_Mode_Get() != MODE_M4) || (test_massage_example != PRODUCTION_TASK_EXAMPLE_NONE))//插电模式 不能重入 
	{
		b_message_data.product_exeErrorCode = REPORT_OTHER_ERROR;
		ret = dataUpStream_handler(PRODUCT_REPORT_TYPE_ERRORCODE, dataResourse);
		return ret;
	}
	if(test_start_flag != 1 && productControlType != PRODUCT_START)//当没点击进入的时候不允许触发其它指令
	{
		b_message_data.product_exeErrorCode = REPORT_OTHER_ERROR;
		ret = dataUpStream_handler(PRODUCT_REPORT_TYPE_ERRORCODE, dataResourse);
		return ret;
	}
	uint8_t ble_mac_bytes[6] = {0};
	//关闭鉴权超时定时器
	switch(productControlType)
	{
		//产测协议
		case PRODUCT_START:			//产测开始 PASS
			COMM_LOG_DEBUG("[COMM][STA] *******Product Satrt*******\r\n");
			deviceAuthStopTimer();
			test_start_flag	=1;
			Message_Cmd_Put(COMM_TASK_ID,ENTRY_TASK_ID,TASK_TEST_START,NULL,0);
			productControlType = PRODUCT_NO_TASK;
		
			break;
		case PRODUCT_LFSINIT:		//文件系统注册PASS
			COMM_LOG_DEBUG("[COMM][STA] *******PRODUCT_LFSINIT*******\r\n");
			test_massage_example = PRODUCTION_TASK_EXAMPLE_1;
			Message_Cmd_Put(COMM_TASK_ID,TEST_TASK_ID,TASK_CMD_TEST_EXAMPLE,&test_massage_example,sizeof(test_massage_example));
			productControlType = PRODUCT_NO_TASK;
			break;
		case PRODUCT_LED:		//LED测试
			COMM_LOG_DEBUG("[COMM][STA] *******PRODUCT_LED*******\r\n");
			test_massage_example = PRODUCTION_TASK_EXAMPLE_2;
			Message_Cmd_Put(COMM_TASK_ID,TEST_TASK_ID,TASK_CMD_TEST_EXAMPLE,&test_massage_example,sizeof(test_massage_example));
			productControlType = PRODUCT_NO_TASK;
			break;
		case PRODUCT_MOTOR:		//电机测试
			COMM_LOG_DEBUG("[COMM][STA] *******PRODUCT_MOTOR*******\r\n");
			test_massage_example = PRODUCTION_TASK_EXAMPLE_3;
			Message_Cmd_Put(COMM_TASK_ID,TEST_TASK_ID,TASK_CMD_TEST_EXAMPLE,&test_massage_example,sizeof(test_massage_example));
			productControlType = PRODUCT_NO_TASK;
			break;
		case PRODUCT_AUDIO:		//喇叭测试
			COMM_LOG_DEBUG("[COMM][STA] *******PRODUCT_AUDIO*******\r\n");
			test_massage_example = PRODUCTION_TASK_EXAMPLE_4;
			Message_Cmd_Put(COMM_TASK_ID,TEST_TASK_ID,TASK_CMD_TEST_EXAMPLE,&test_massage_example,sizeof(test_massage_example));
			productControlType = PRODUCT_NO_TASK;
			break;
		case PRODUCT_LTE_NO_CACERT:		//LTE测试（不带证书）
			COMM_LOG_DEBUG("[COMM][STA] *******PRODUCT_LTE*******\r\n");
			test_massage_example = PRODUCTION_TASK_EXAMPLE_5;
			Message_Cmd_Put(COMM_TASK_ID,TEST_TASK_ID,TASK_CMD_TEST_EXAMPLE,&test_massage_example,sizeof(test_massage_example));
			productControlType = PRODUCT_NO_TASK;
			break;
		case PRODUCT_GPS:		//GPS测试
			COMM_LOG_DEBUG("[COMM][STA] *******PRODUCT_GPS*******\r\n");
			test_massage_example = PRODUCTION_TASK_EXAMPLE_6;
			Message_Cmd_Put(COMM_TASK_ID,TEST_TASK_ID,TASK_CMD_TEST_EXAMPLE,&test_massage_example,sizeof(test_massage_example));
			productControlType = PRODUCT_NO_TASK;
			break;
		case PRODUCT_SENDORCFG:		//sensor配置
			COMM_LOG_DEBUG("[COMM][STA] *******PRODUCT_SENDORCFG*******\r\n");
			//待补充代码
			productControlType = PRODUCT_NO_TASK;
			break;
		case PRODUCT_SENDOR:		//sensor测试
			COMM_LOG_DEBUG("[COMM][STA] *******PRODUCT_SENDOR*******\r\n");
			test_massage_example = PRODUCTION_TASK_EXAMPLE_7;
			Message_Cmd_Put(COMM_TASK_ID,TEST_TASK_ID,TASK_CMD_TEST_EXAMPLE,&test_massage_example,sizeof(test_massage_example));
			productControlType = PRODUCT_NO_TASK;
			break;
		case PRODUCT_ADC:		//ADC测试
			COMM_LOG_DEBUG("[COMM][STA] *******PRODUCT_ADC*******\r\n");
			test_massage_example = PRODUCTION_TASK_EXAMPLE_8;
			Message_Cmd_Put(COMM_TASK_ID,TEST_TASK_ID,TASK_CMD_TEST_EXAMPLE,&test_massage_example,sizeof(test_massage_example));
			productControlType = PRODUCT_NO_TASK;
			break;
		case PRODUCT_GET_SN:		//读取SN测试
			COMM_LOG_DEBUG("[COMM][STA] *******PRODUCT_GET_SN*******\r\n");
	
//			uint8_t test_massage_example = PRODUCTION_TASK_EXAMPLE_8;
//			Message_Cmd_Put(COMM_TASK_ID,TEST_TASK_ID,TASK_CMD_TEST_EXAMPLE,&test_massage_example,1);
			
			ret = dataUpStream_handler(PRODUCT_REPORT_TYPE_SN, dataResourse);
			productControlType = PRODUCT_NO_TASK;
			break;
		case PRODUCT_SET_MAC:		//蓝牙MAC配置
			//转化为十六进制格式
			string_mac_to_bytes((char *)b_message_data.appProduct_bleMac, ble_mac_bytes);
			product_set_addr(ble_mac_bytes);
		
			ret = dataUpStream_handler(PRODUCT_REPORT_TYPE_ERRORCODE, dataResourse);
			productControlType = PRODUCT_NO_TASK;
			break;
		case PRODUCT_GET_MAC:		//蓝牙MAC读取
			
			ret = dataUpStream_handler(PRODUCT_REPORT_TYPE_BLEMAC, dataResourse);
			productControlType = PRODUCT_NO_TASK;
			break;
		case PRODUCT_CACERT:		//CA证书写入
			COMM_LOG_DEBUG("[COMM][STA] *******PRODUCT_CACERT*******\r\n");
			test_massage_example = PRODUCTION_TASK_EXAMPLE_10;
			Message_Cmd_Put(COMM_TASK_ID,TEST_TASK_ID,TASK_CMD_TEST_EXAMPLE,&test_massage_example,sizeof(test_massage_example));
			productControlType = PRODUCT_NO_TASK;
			break;
		case PRODUCT_CLIENT:		//CLIENT写入
			COMM_LOG_DEBUG("[COMM][STA] *******PRODUCT_CLIENT*******\r\n");
			test_massage_example = PRODUCTION_TASK_EXAMPLE_11;
			Message_Cmd_Put(COMM_TASK_ID,TEST_TASK_ID,TASK_CMD_TEST_EXAMPLE,&test_massage_example,sizeof(test_massage_example));
			productControlType = PRODUCT_NO_TASK;
			break;
		case PRODUCT_USERKEY:		//Userkey入
			COMM_LOG_DEBUG("[COMM][STA] *******PRODUCT_USERKEY*******\r\n");
			test_massage_example = PRODUCTION_TASK_EXAMPLE_12;
			Message_Cmd_Put(COMM_TASK_ID,TEST_TASK_ID,TASK_CMD_TEST_EXAMPLE,&test_massage_example,sizeof(test_massage_example));
			productControlType = PRODUCT_NO_TASK;
			break;
		case PRODUCT_LTE_WITH_CACERT://带证书测试
			COMM_LOG_DEBUG("[COMM][STA] *******PRODUCT_LTE_WITH_CACERT*******\r\n");
			test_massage_example = PRODUCTION_TASK_EXAMPLE_13;
			Message_Cmd_Put(COMM_TASK_ID,TEST_TASK_ID,TASK_CMD_TEST_EXAMPLE,&test_massage_example,sizeof(test_massage_example));
//			ret = dataUpStream_handler(PRODUCT_REPORT_TYPE_ERRORCODE, dataResourse);
			productControlType = PRODUCT_NO_TASK;
			break;
		case PRODUCT_LTE_UPDATA:	//LTE差分包升级
			COMM_LOG_DEBUG("[COMM][STA] *******PRODUCT_LTE_UPDATA*******\r\n");
			test_massage_example = PRODUCTION_TASK_EXAMPLE_14;
			Message_Cmd_Put(COMM_TASK_ID,TEST_TASK_ID,TASK_CMD_TEST_EXAMPLE,&test_massage_example,sizeof(test_massage_example));
//			ret = dataUpStream_handler(PRODUCT_REPORT_TYPE_ERRORCODE, dataResourse);
			productControlType = PRODUCT_NO_TASK;
			break;
		case PRODUCT_LTE_USB_UPDATA:	//LTEUSB升级
			COMM_LOG_DEBUG("[COMM][STA] *******PRODUCT_LTE_USB_UPDATA*******\r\n");
			test_massage_example = PRODUCTION_TASK_EXAMPLE_15;
			usb_updata_flag_set(b_message_data.appControl_lteUpdataSwitch);
			Message_Cmd_Put(COMM_TASK_ID,TEST_TASK_ID,TASK_CMD_TEST_EXAMPLE,&test_massage_example,sizeof(test_massage_example));
			//ret = dataUpStream_handler(PRODUCT_REPORT_TYPE_ERRORCODE, dataResourse);
			productControlType = PRODUCT_NO_TASK;
			break;
		//获取设备参数
		case PRODUCT_GET_DEVICEMSG:	//获取设备参数
			COMM_LOG_DEBUG("[COMM][STA] *******PRODUCT_GET_DEVICEMSG*******\r\n");
		
			ret = dataUpStream_handler(PRODUCT_REPORT_TYPE_DEVICEMSG, dataResourse);
			productControlType = PRODUCT_NO_TASK;
			break;
		//LTE/GPS复合测试
		case PRODUCT_MULTI:
			COMM_LOG_DEBUG("[COMM][STA] *******PRODUCT_MULTI*******\r\n");
		
			test_massage_example = PRODUCTION_TASK_EXAMPLE_17;
			Message_Cmd_Put(COMM_TASK_ID,TEST_TASK_ID,TASK_CMD_TEST_EXAMPLE,&test_massage_example,sizeof(test_massage_example));

			//TEST处理
		
			productControlType = PRODUCT_NO_TASK;
			break;
		case PRODUCT_STOP:			//产测结束
			COMM_LOG_DEBUG("[COMM][STA] *******Product Stop*******\r\n");
			deviceAuthStartTimer();
			test_massage_example = PRODUCTION_TASK_EXAMPLE_9;
			Message_Cmd_Put(COMM_TASK_ID,TEST_TASK_ID,TASK_CMD_TEST_EXAMPLE,&test_massage_example,sizeof(test_massage_example));
			productControlType = PRODUCT_NO_TASK;
			test_start_flag =0;
			break;
		
		case PRODUCT_AGING_TEST:		//老化测试
			COMM_LOG_DEBUG("[COMM][STA] *******PRODUCT_AGING_TEST*******\r\n");
			test_massage_example = PRODUCTION_TASK_EXAMPLE_16;
			Message_Cmd_Put(COMM_TASK_ID,TEST_TASK_ID,TASK_CMD_TEST_EXAMPLE,&test_massage_example,sizeof(test_massage_example));
			productControlType = PRODUCT_NO_TASK;
			break;
		default:
			break;
	}
	return ret;
}

/***************************************************************
* 函数名称: data_downStream_handler
* 说    明: 下行数据处理
***************************************************************/
int data_downStream_handler(unsigned char * msg, unsigned int len)
{
//    cJSON* cjson_status = NULL;
	int ret = 0;

    cJSON *cjson_buff = cJSON_Parse((char*)msg);
    if (cjson_buff == NULL) {
        cJSON_Delete(cjson_buff);
        return ret;
    }

    // 2. 解析出serviceId
	cJSON *itemSid = cJSON_GetObjectItem(cjson_buff, "cmd");
//    method = cJSON_GetObjectItem(root, "method");
    if (!itemSid) {
        cJSON_Delete(itemSid);
        return ret;
    }

    // 3. 解析出数据上报后平台回复的上报状态
	memset(b_message_data.cmd, 0, sizeof(b_message_data.cmd));
	strncpy((char *)b_message_data.cmd, itemSid->valuestring, sizeof(b_message_data.cmd)-1);

	/* M4充电模式白名单: 仅放行BLE鉴权(deviceAuth, 通道握手, 不放行则鉴权超时断链)和产测协议(product*);
	 * 其余所有APP/云端指令(震动/模式/音频/查询等, BLE和4G来的都算)在解析前整包丢弃。
	 * 必须拦在解析前: geofenceConfig/deviceConfig等指令解析阶段就会改写RAM配置,
	 * 只堵执行层会留副作用; 主动上报不经过本函数, 不受影响 */
	if (CurrentModeDataGet() == CURRENT_MODE_CHARGE
	    && strcmp(itemSid->valuestring, "deviceAuth") != 0
	    && strncmp(itemSid->valuestring, "product", 7) != 0)
	{
		COMM_LOG_DEBUG("[COMM][STA] M4 charge: cmd %s dropped\r\n", itemSid->valuestring);
		cJSON_Delete(cjson_buff);
		return 0;
	}

	//①整机运行状态
//	COMM_LOG_DEBUG("itemSid->valuestring22: %s\n",itemSid->valuestring);
    if (!strcmp(itemSid->valuestring, "deviceState")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *params = cJSON_GetObjectItem(cjson_buff, "params");
			cJSON *timestamp = cJSON_GetObjectItem(cjson_buff, "ts");
			cJSON *reqId = cJSON_GetObjectItem(cjson_buff, "reqId");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			if (params != NULL && (strcmp(params->string, "params") == 0)) 
			{	
				cJSON *dataType = cJSON_GetObjectItem(params, "dataType");
				cJSON *dataTime = cJSON_GetObjectItem(params, "time");
				cJSON *dataNum = cJSON_GetObjectItem(params, "dataNum");
				
				if (dataType != NULL && (strcmp(dataType->string, "dataType") == 0)) 
				{
//					int dataTypeSta = cJSON_GetObjectItem(deviceStateData, "dataType")->valueint;
					b_message_data.dataType = dataType->valueint;
				}
				if (dataTime != NULL && (strcmp(dataTime->string, "time") == 0)) 
				{
//					int dataTimeSta = cJSON_GetObjectItem(data, "time")->valueint;
					b_message_data.appControl_historyTime = dataTime->valueint;
					
				}
				if(dataNum != NULL && (strcmp(dataNum->string, "dataNum") == 0))
				{
//					int dataNumSta = cJSON_GetObjectItem(data, "dataNum")->valueint;
					b_message_data.appControl_dataNum = dataNum->valueint;
				}
			}
			if (timestamp != NULL && (strcmp(timestamp->string, "ts") == 0)) 
			{
//				int timestampSta = cJSON_GetObjectItem(deviceStateData, "timestamp")->valueint;
				b_message_data.appControl_timestamp = timestamp->valueint;
			} 
			if (reqId != NULL && (strcmp(reqId->string, "reqId") == 0)) 
			{
//				int cmdSta = cJSON_GetObjectItem(deviceStateData, "cmd")->valueint;
//				b_message_data.reqId = (uint8_t *)reqId->valuestring;
				memset(b_message_data.reqId, 0, sizeof(b_message_data.reqId));
				strncpy((char *)b_message_data.reqId, reqId->valuestring, sizeof(b_message_data.reqId)-1);
			}
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			comm_func_topic = FUNC_QUERY_DEVICE_STATE;
			ret = dataDownStream_handler(CONTROL_CHECK_DEVICE_STATE, b_message_data.dataResourse);
//        ret = 1;	//BLE_SendCustomData(CUSTOM_SEC_DATA, (unsigned char *)buff, len);
    }
	//②查询设备信息
	else if (!strcmp(itemSid->valuestring, "deviceInfo"))
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *timestamp = cJSON_GetObjectItem(cjson_buff, "ts");
			cJSON *reqId = cJSON_GetObjectItem(cjson_buff, "reqId");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			if (timestamp != NULL && (strcmp(timestamp->string, "ts") == 0)) 
			{
//				int timestampSta = cJSON_GetObjectItem(deviceInformationData, "timestamp")->valueint;
				b_message_data.appControl_timestamp = timestamp->valueint;
			} 
			if (reqId != NULL && (strcmp(reqId->string, "reqId") == 0)) 
			{
//				int cmdSta = cJSON_GetObjectItem(deviceInformationData, "cmd")->valueint;
//				b_message_data.reqId = (uint8_t *)reqId->valuestring;
				memset(b_message_data.reqId, 0, sizeof(b_message_data.reqId));
				strncpy((char *)b_message_data.reqId, reqId->valuestring, sizeof(b_message_data.reqId)-1);
			}
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			comm_func_topic = FUNC_UNKNOWN;
			ret = dataDownStream_handler(CONTROL_CHECK_DEVICE_INFO, b_message_data.dataResourse);
    }
	//③设置设备模式
	else if (!strcmp(itemSid->valuestring, "deviceMode")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *params = cJSON_GetObjectItem(cjson_buff, "params");
			cJSON *timestamp = cJSON_GetObjectItem(cjson_buff, "ts");
			cJSON *reqId = cJSON_GetObjectItem(cjson_buff, "reqId");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			if (params != NULL && (strcmp(params->string, "params") == 0))
			{
				/* V1.6: deviceMode params 只保留 setMode(1标准/2寻宠) */
				cJSON *setMode = cJSON_GetObjectItem(params, "setMode");
				if (setMode != NULL && (strcmp(setMode->string, "setMode") == 0))
				{
					b_message_data.appControl_currentMode.mode = setMode->valueint;
				}
			}
			if (timestamp != NULL && (strcmp(timestamp->string, "ts") == 0)) 
			{
//				int timestampSta = cJSON_GetObjectItem(deviceModeData, "timestamp")->valueint;
				b_message_data.appControl_timestamp = timestamp->valueint;
			}
			if (reqId != NULL && (strcmp(reqId->string, "reqId") == 0)) 
			{
//				int cmdSta = cJSON_GetObjectItem(eNodeBInformationData, "cmd")->valueint;
//				b_message_data.reqId = (uint8_t *)reqId->valuestring;
				memset(b_message_data.reqId, 0, sizeof(b_message_data.reqId));
				strncpy((char *)b_message_data.reqId, reqId->valuestring, sizeof(b_message_data.reqId)-1);
			}
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			comm_func_topic = FUNC_CONTROL_DEVICE_MODE;
			ret = dataDownStream_handler(CONTROL_SET_DEVICE_MODE, b_message_data.dataResourse);
    }
	//④查询基站信息
	else if (!strcmp(itemSid->valuestring, "eNodeBInformation")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *timestamp = cJSON_GetObjectItem(cjson_buff, "ts");
			cJSON *reqId = cJSON_GetObjectItem(cjson_buff, "reqId");

			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			if (timestamp != NULL && (strcmp(timestamp->string, "ts") == 0)) 
			{
//				int timestampSta = cJSON_GetObjectItem(eNodeBInformationData, "timestamp")->valueint;
				b_message_data.appControl_timestamp = timestamp->valueint;
			} 
			if (reqId != NULL && (strcmp(reqId->string, "reqId") == 0)) 
			{
//				int cmdSta = cJSON_GetObjectItem(eNodeBInformationData, "cmd")->valueint;
//				b_message_data.reqId = (uint8_t *)reqId->valuestring;
				memset(b_message_data.reqId, 0, sizeof(b_message_data.reqId));
				strncpy((char *)b_message_data.reqId, reqId->valuestring, sizeof(b_message_data.reqId)-1);
			}

			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			comm_func_topic = FUNC_QUERY_ENODEB;
			ret = dataDownStream_handler(CONTROL_CHECK_ENODEB_INFO, b_message_data.dataResourse);
    }
	//④发送语音指令
	else if (!strcmp(itemSid->valuestring, "audioCmdSend")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *params = cJSON_GetObjectItem(cjson_buff, "params");
			cJSON *timestamp = cJSON_GetObjectItem(cjson_buff, "ts");
			cJSON *reqId = cJSON_GetObjectItem(cjson_buff, "reqId");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			if (params != NULL && (strcmp(params->string, "params") == 0)) 
			{	
				cJSON *audioSwitch = cJSON_GetObjectItem(params, "audioSwitch");
//				cJSON *audio = cJSON_GetObjectItem(params, "audio");
				if (audioSwitch != NULL && (strcmp(audioSwitch->string, "audioSwitch") == 0)) 
				{
					int cmdSta = cJSON_GetObjectItem(params, "audioSwitch")->valueint;
					if(cmdSta == 1)
					{
						cJSON *audio = cJSON_GetObjectItem(params, "audio");
						if (audio != NULL && (strcmp(audio->string, "audio") == 0)) 
						{	
							cJSON *index = cJSON_GetObjectItem(audio, "index");
							cJSON *sound = cJSON_GetObjectItem(audio, "sound");
							if (index != NULL && (strcmp(index->string, "index") == 0)) 
							{
	//							int IndexSta = cJSON_GetObjectItem(Index, "Index")->valueint;
								b_message_data.appControl_audioIndex = index->valueint;
							}
							if(sound != NULL && (strcmp(sound->string, "sound") == 0))
							{
	//							int frequenceSta = cJSON_GetObjectItem(Sound, "Sound")->valueint;
								b_message_data.appControl_audioSound = sound->valueint;
								
							}
						}
					}
				}
			}
			if (timestamp != NULL && (strcmp(timestamp->string, "ts") == 0)) 
			{
//				int timestampSta = cJSON_GetObjectItem(cjson_buff, "ts")->valueint;
				b_message_data.appControl_timestamp = timestamp->valueint;
			} 
			if (reqId != NULL && (strcmp(reqId->string, "reqId") == 0)) 
			{
//				int cmdSta = cJSON_GetObjectItem(audioCmdSendData, "cmd")->valueint;
//				b_message_data.reqId = (uint8_t *)reqId->valuestring;
				memset(b_message_data.reqId, 0, sizeof(b_message_data.reqId));
				strncpy((char *)b_message_data.reqId, reqId->valuestring, sizeof(b_message_data.reqId)-1);
			}
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			comm_func_topic = FUNC_CONTROL_AUDIOCMD;
			ret = dataDownStream_handler(CONTROL_SET_AUDIO_CMD, b_message_data.dataResourse);
    }
	//④发送震动
	else if (!strcmp(itemSid->valuestring, "deviceVibrate")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *params = cJSON_GetObjectItem(cjson_buff, "params");
			cJSON *timestamp = cJSON_GetObjectItem(cjson_buff, "ts");
			cJSON *reqId = cJSON_GetObjectItem(cjson_buff, "reqId");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			if (params != NULL && (strcmp(params->string, "params") == 0)) 
			{	
				cJSON *vibrateTime = cJSON_GetObjectItem(params, "vibrateTime");
				if (vibrateTime != NULL && (strcmp(vibrateTime->string, "vibrateTime") == 0)) 
				{
					
					b_message_data.appControl_vibrateTime = vibrateTime->valueint;
				}
			}
			if (timestamp != NULL && (strcmp(timestamp->string, "ts") == 0)) 
			{
//				int timestampSta = cJSON_GetObjectItem(cjson_buff, "ts")->valueint;
				b_message_data.appControl_timestamp = timestamp->valueint;
			} 
			if (reqId != NULL && (strcmp(reqId->string, "reqId") == 0)) 
			{
//				int cmdSta = cJSON_GetObjectItem(audioCmdSendData, "cmd")->valueint;
//				b_message_data.reqId = (uint8_t *)reqId->valuestring;
				memset(b_message_data.reqId, 0, sizeof(b_message_data.reqId));
				strncpy((char *)b_message_data.reqId, reqId->valuestring, sizeof(b_message_data.reqId)-1);
			}
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			comm_func_topic = FUNC_CONTROL_VIBRATE;
			ret = dataDownStream_handler(CONTROL_SET_VIBRATE_CMD, b_message_data.dataResourse);
    }
	//发送实时语音
//	else if (!strcmp(itemSid->valuestring, "audioRealtime")) 
//    {
//			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
//			cJSON *params = cJSON_GetObjectItem(cjson_buff, "params");
//			cJSON *timestamp = cJSON_GetObjectItem(cjson_buff, "ts");
//			cJSON *reqId = cJSON_GetObjectItem(cjson_buff, "reqId");
//			
//			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
//			{
//				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
//				b_message_data.dataResourse = dataResourseSta;
//			} 
//			if (params != NULL && (strcmp(params->string, "params") == 0)) 
//			{	
//				cJSON *url = cJSON_GetObjectItem(params, "url");
//				cJSON *length = cJSON_GetObjectItem(params, "length");
//				if (url != NULL && (strcmp(url->string, "url") == 0)) 
//				{
//					b_message_data.appControl_audioRealTimeUrl = (uint8_t *)url->valuestring;
//				}
//				if (length != NULL && (strcmp(length->string, "length") == 0)) 
//				{
//					b_message_data.appControl_audioRealTimeLen = length->valueint;
//				}
//			}
//			if (timestamp != NULL && (strcmp(timestamp->string, "ts") == 0)) 
//			{
//				b_message_data.appControl_timestamp = timestamp->valueint;
//			} 
//			if (reqId != NULL && (strcmp(reqId->string, "reqId") == 0)) 
//			{
//				memset(b_message_data.reqId, 0, sizeof(b_message_data.reqId));
//				strncpy((char *)b_message_data.reqId, reqId->valuestring, sizeof(b_message_data.reqId)-1);
//			}
//			else
//			{
//				//ERROR
//				ret = 0;
//				goto EXIT;
//			}
//			comm_func_topic = FUNC_CONTROL_AUDIOREALTIME;
//			ret = dataDownStream_handler(CONTROL_SET_AUDIO_REALTIME, b_message_data.dataResourse);
//    }
//	//④配置语音文件
//	else if (!strcmp(itemSid->valuestring, "audioFile")) 
//    {
//        cJSON *audioFileData = cJSON_GetObjectItem(cjson_buff, "characteristics");
//		if(audioFileData !=NULL)
//		{
//			cJSON *dataResourse = cJSON_GetObjectItem(audioFileData, "dataResourse");
//			cJSON *timestamp = cJSON_GetObjectItem(audioFileData, "timestamp");
//			cJSON *reqId = cJSON_GetObjectItem(audioFileData, "reqId");
//			cJSON *audio = cJSON_GetObjectItem(audioFileData, "audio");
//			
//			if (dataResourse != NULL && (strcmp(dataResourse->string, "dataResourse") == 0)) 
//			{
//				int dataResourseSta = cJSON_GetObjectItem(audioFileData, "dataResourse")->valueint;
//				b_message_data.dataResourse = dataResourseSta;
//			} 
//			if (timestamp != NULL && (strcmp(timestamp->string, "timestamp") == 0)) 
//			{
//				int timestampSta = cJSON_GetObjectItem(audioFileData, "timestamp")->valueint;
//				b_message_data.appControl_timestamp = timestamp->valueint;
//			} 
//			if (reqId != NULL && (strcmp(reqId->string, "reqId") == 0)) 
//			{
////				int cmdSta = cJSON_GetObjectItem(audioCmdSendData, "cmd")->valueint;
////				b_message_data.reqId = (uint8_t *)reqId->valuestring;
//				memset(b_message_data.reqId, 0, sizeof(b_message_data.reqId));
//				strncpy((char *)b_message_data.reqId, reqId->valuestring, sizeof(b_message_data.reqId)-1);
//			}
//			if (audio != NULL && (strcmp(audio->string, "audio") == 0)) 
//			{
//				cJSON *file = cJSON_GetObjectItem(audio, "file");
//				cJSON *length = cJSON_GetObjectItem(audio, "length");
//				cJSON *sound = cJSON_GetObjectItem(audio, "sound");

//				if (file != NULL && (strcmp(file->string, "file") == 0)) 
//				{	
////					int fileSta = cJSON_GetObjectItem(audio, "file")->valueint;
//					b_message_data.appControl_audioConfigFile = file->valuestring;
//					
//				}
//				if (length != NULL && (strcmp(length->string, "length") == 0)) 
//				{	
//					
////					int lengthSta = cJSON_GetObjectItem(audio, "length")->valueint;
//					b_message_data.appControl_audioConfigLength = length->valueint;
//					
//				}
////				if (sound != NULL && (strcmp(sound->string, "sound") == 0)) 
////				{	
////					
////					int soundSta = cJSON_GetObjectItem(audio, "sound")->valueint;
////						
////				}
//			}
//			else
//			{
//				//ERROR
//				ret = 0;
//				goto EXIT;
//			}
//			ret = dataDownStream_handler(CONTROL_SET_AUDIO_FILE, b_message_data.dataResourse);
//		}
//    }
	//④接收语音文件指令
	else if (!strcmp(itemSid->valuestring, "audioRecv")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *params = cJSON_GetObjectItem(cjson_buff, "params");
			cJSON *timestamp = cJSON_GetObjectItem(cjson_buff, "ts");
			cJSON *reqId = cJSON_GetObjectItem(cjson_buff, "reqId");
			
//			COMM_LOG_DEBUG("333:%s\n", dataResourse->string);
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 	//dataResourse != NULL && (strcmp(dataResourse->string, "dataResourse") == 0)
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			}
			if (params != NULL && (strcmp(params->string, "params") == 0)) 
			{	
				cJSON *audioTime = cJSON_GetObjectItem(params, "audioTime");
				if (audioTime != NULL && (strcmp(audioTime->string, "audioTime") == 0)) 
				{
//					b_message_data.appControl_historyTime = dataTime->valueint;
				}
			}
			if (timestamp != NULL && (strcmp(timestamp->string, "ts") == 0)) 
			{
//				int timestampSta = cJSON_GetObjectItem(audioRecvData, "timestamp")->valueint;
				b_message_data.appControl_timestamp = timestamp->valueint;
			} 
			if (reqId != NULL && (strcmp(reqId->string, "reqId") == 0)) 
			{
//				int cmdSta = cJSON_GetObjectItem(audioCmdSendData, "cmd")->valueint;
//				b_message_data.reqId = (uint8_t *)reqId->valuestring;
				memset(b_message_data.reqId, 0, sizeof(b_message_data.reqId));
				strncpy((char *)b_message_data.reqId, reqId->valuestring, sizeof(b_message_data.reqId)-1);
			}
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			comm_func_topic = FUNC_UNKNOWN;
			ret = dataDownStream_handler(CONTROL_SET_AUDIO_RECV, b_message_data.dataResourse);
    }
//	//④设置止吠指令
//	else if (!strcmp(itemSid->valuestring, "noBarkControl")) 
//    {
//        cJSON *noBarkControlData = cJSON_GetObjectItem(cjson_buff, "characteristics");
//		if(noBarkControlData !=NULL)
//		{
//			cJSON *dataResourse = cJSON_GetObjectItem(noBarkControlData, "dataResourse");
//			cJSON *timestamp = cJSON_GetObjectItem(noBarkControlData, "timestamp");
//			cJSON *reqId = cJSON_GetObjectItem(noBarkControlData, "reqId");
//			cJSON *audioTime = cJSON_GetObjectItem(noBarkControlData, "audioTime");
//			cJSON *vibrationSwitch = cJSON_GetObjectItem(noBarkControlData, "vibrationSwitch");
//			cJSON *vibration = cJSON_GetObjectItem(noBarkControlData, "vibration");
//			cJSON *lightSwitch = cJSON_GetObjectItem(noBarkControlData, "lightSwitch");
//			cJSON *light = cJSON_GetObjectItem(noBarkControlData, "light");
//			
//			if (dataResourse != NULL && (strcmp(dataResourse->string, "dataResourse") == 0)) 
//			{
//				int dataResourseSta = cJSON_GetObjectItem(noBarkControlData, "dataResourse")->valueint;
//				b_message_data.dataResourse = dataResourseSta;
//			} 
//			if (timestamp != NULL && (strcmp(timestamp->string, "timestamp") == 0)) 
//			{
////				int timestampSta = cJSON_GetObjectItem(noBarkControlData, "timestamp")->valueint;
//				b_message_data.appControl_timestamp = timestamp->valueint;
//			} 
//			if (reqId != NULL && (strcmp(reqId->string, "reqId") == 0)) 
//			{
////				int cmdSta = cJSON_GetObjectItem(audioCmdSendData, "cmd")->valueint;
////				b_message_data.reqId = (uint8_t *)reqId->valuestring;
//				memset(b_message_data.reqId, 0, sizeof(b_message_data.reqId));
//				strncpy((char *)b_message_data.reqId, reqId->valuestring, sizeof(b_message_data.reqId)-1);
//			}
//			if (vibrationSwitch != NULL && (strcmp(vibrationSwitch->string, "vibrationSwitch") == 0)) 
//			{
//				int vibrationSwitchSta = cJSON_GetObjectItem(noBarkControlData, "vibrationSwitch")->valueint;
//				
//				//开关开启才解析
//				if(vibrationSwitchSta == 1)
//				{
//					cJSON *vibration = cJSON_GetObjectItem(noBarkControlData, "vibration");
//					if (vibration != NULL && (strcmp(vibration->string, "vibration") == 0)) 
//					{	
//						cJSON *time = cJSON_GetObjectItem(vibration, "time");
//						cJSON *frequence = cJSON_GetObjectItem(vibration, "frequence");
//						if (time != NULL && (strcmp(time->string, "time") == 0)) 
//						{
////							int timeSta = cJSON_GetObjectItem(vibration, "time")->valueint;
//							b_message_data.appControl_vibrationTime = time->valueint;
//							
//						}
//						if(frequence != NULL && (strcmp(frequence->string, "frequence") == 0))
//						{
////							int frequenceSta = cJSON_GetObjectItem(vibration, "frequence")->valueint;
//							b_message_data.appControl_vibrationFrequence = frequence->valueint;
//							
//						}
//					}
//				}
//			}
//			if (lightSwitch != NULL && (strcmp(lightSwitch->string, "lightSwitch") == 0)) 
//			{
//				int lightSwitchSta = cJSON_GetObjectItem(noBarkControlData, "lightSwitch")->valueint;
//				
//				//开关开启才解析
//				if(lightSwitchSta == 1)
//				{
//					cJSON *light = cJSON_GetObjectItem(noBarkControlData, "light");
//					if (light != NULL && (strcmp(light->string, "light") == 0)) 
//					{	
//						cJSON *color = cJSON_GetObjectItem(light, "color");
//						cJSON *frequence = cJSON_GetObjectItem(light, "frequence");
//						if (color != NULL && (strcmp(color->string, "color") == 0)) 
//						{
////							int colorSta = cJSON_GetObjectItem(light, "color")->valueint;
//							b_message_data.appControl_lightColor = color->valueint;
//						}
//						if(frequence != NULL && (strcmp(frequence->string, "frequence") == 0))
//						{
////							int frequenceSta = cJSON_GetObjectItem(light, "frequence")->valueint;
//							b_message_data.appControl_lightFrequence = frequence->valueint;
//						}
//					}
//				}
//			}
//			else
//			{
//				//ERROR
//				ret = 0;
//				goto EXIT;
//			}
//			ret = dataDownStream_handler(CONTROL_SET_NOBARK, b_message_data.dataResourse);
//		}
//    }
	//④同步时间设置
	else if (!strcmp(itemSid->valuestring, "timeSync")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *params = cJSON_GetObjectItem(cjson_buff, "params");
			cJSON *ts = cJSON_GetObjectItem(cjson_buff, "ts");
			cJSON *reqId = cJSON_GetObjectItem(cjson_buff, "reqId");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			if (params != NULL && (strcmp(params->string, "params") == 0)) 
			{	
				cJSON *timestamp = cJSON_GetObjectItem(params, "timestamp");
				if (timestamp != NULL && (strcmp(timestamp->string, "timestamp") == 0)) 
				{
//					b_message_data.appControl_historyTime = dataTime->valueint;
				}
			}
			if (ts != NULL && (strcmp(ts->string, "ts") == 0)) 
			{
//				int timestampSta = cJSON_GetObjectItem(timeSynchronizeData, "timestamp")->valueint;
				b_message_data.appControl_timestamp = ts->valueint;
			} 
			if (reqId != NULL && (strcmp(reqId->string, "reqId") == 0)) 
			{
//				int cmdSta = cJSON_GetObjectItem(audioCmdSendData, "cmd")->valueint;
//				b_message_data.reqId = (uint8_t *)reqId->valuestring;
				memset(b_message_data.reqId, 0, sizeof(b_message_data.reqId));
				strncpy((char *)b_message_data.reqId, reqId->valuestring, sizeof(b_message_data.reqId)-1);
			}
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			comm_func_topic = FUNC_CONFIG_TIMESYNC;
			ret = dataDownStream_handler(CONTROL_SET_TIME_SYNCHRONIZE, b_message_data.dataResourse);
    }
	//④WiFi scan开启
	else if (!strcmp(itemSid->valuestring, "wifiScan")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *timestamp = cJSON_GetObjectItem(cjson_buff, "ts");
			cJSON *reqId = cJSON_GetObjectItem(cjson_buff, "reqId");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			if (timestamp != NULL && (strcmp(timestamp->string, "ts") == 0)) 
			{
//				int timestampSta = cJSON_GetObjectItem(wifiMacConfigData, "timestamp")->valueint;
				b_message_data.appControl_timestamp = timestamp->valueint;
			} 
			if (reqId != NULL && (strcmp(reqId->string, "reqId") == 0)) 
			{
//				int cmdSta = cJSON_GetObjectItem(audioCmdSendData, "cmd")->valueint;
//				b_message_data.reqId = (uint8_t *)reqId->valuestring;
				memset(b_message_data.reqId, 0, sizeof(b_message_data.reqId));
				strncpy((char *)b_message_data.reqId, reqId->valuestring, sizeof(b_message_data.reqId)-1);
			}
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			comm_func_topic = FUNC_CONFIG_WIFIMAC;
			ret = dataDownStream_handler(CONTROL_SET_WIFI_SCAN, b_message_data.dataResourse);
    }
	//④WiFi设置 ssid
	else if (!strcmp(itemSid->valuestring, "wifiSsidConfig")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *params = cJSON_GetObjectItem(cjson_buff, "params");
			cJSON *timestamp = cJSON_GetObjectItem(cjson_buff, "ts");
			cJSON *reqId = cJSON_GetObjectItem(cjson_buff, "reqId");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			if (params != NULL && (strcmp(params->string, "params") == 0)) 
			{	
				cJSON *ssid = cJSON_GetObjectItem(params, "ssid");
				cJSON *mac = cJSON_GetObjectItem(params, "mac");
				if (ssid != NULL && (strcmp(ssid->string, "ssid") == 0)) 
				{
					b_message_data.appControl_wifiSsid = (uint8_t *)ssid->valuestring;
				}
				if (mac != NULL && (strcmp(mac->string, "mac") == 0)) 
				{
					b_message_data.appControl_wifiMac = (uint8_t *)mac->valuestring;
				cJSON *wifiLat = cJSON_GetObjectItem(params, "lat");
				cJSON *wifiLon = cJSON_GetObjectItem(params, "lon");
				if (wifiLat != NULL) b_message_data.appControl_wifiLat = wifiLat->valuedouble;
				if (wifiLon != NULL) b_message_data.appControl_wifiLon = wifiLon->valuedouble;
				}
			}
			if (timestamp != NULL && (strcmp(timestamp->string, "ts") == 0)) 
			{
//				int timestampSta = cJSON_GetObjectItem(wifiMacConfigData, "timestamp")->valueint;
				b_message_data.appControl_timestamp = timestamp->valueint;
			} 
			if (reqId != NULL && (strcmp(reqId->string, "reqId") == 0)) 
			{
//				int cmdSta = cJSON_GetObjectItem(audioCmdSendData, "cmd")->valueint;
//				b_message_data.reqId = (uint8_t *)reqId->valuestring;
				memset(b_message_data.reqId, 0, sizeof(b_message_data.reqId));
				strncpy((char *)b_message_data.reqId, reqId->valuestring, sizeof(b_message_data.reqId)-1);
			}
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			comm_func_topic = FUNC_CONFIG_WIFIMAC;
			ret = dataDownStream_handler(CONTROL_SET_WIFI_SSID, b_message_data.dataResourse);
    }
	//④设备鉴权指令
	else if (!strcmp(itemSid->valuestring, "deviceAuth")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *params = cJSON_GetObjectItem(cjson_buff, "params");
			cJSON *timestamp = cJSON_GetObjectItem(cjson_buff, "ts");
			cJSON *reqId = cJSON_GetObjectItem(cjson_buff, "reqId");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			}
			if (params != NULL && (strcmp(params->string, "params") == 0)) 
			{	
				cJSON *authData = cJSON_GetObjectItem(params, "authData");
				if (authData != NULL && (strcmp(authData->string, "authData") == 0)) 
				{
					memset(b_message_data.authData, 0, sizeof(b_message_data.authData));
					memcpy(b_message_data.authData, (uint8_t *)authData->valuestring, strlen(authData->valuestring));	
				}
			}
			if (timestamp != NULL && (strcmp(timestamp->string, "ts") == 0)) 
			{
//				int timestampSta = cJSON_GetObjectItem(deviceAuthData, "timestamp")->valueint;
				b_message_data.appControl_timestamp = timestamp->valueint;
			} 
			if (reqId != NULL && (strcmp(reqId->string, "reqId") == 0)) 
			{
//				int cmdSta = cJSON_GetObjectItem(audioCmdSendData, "cmd")->valueint;
//				b_message_data.reqId = (uint8_t *)reqId->valuestring;
				memset(b_message_data.reqId, 0, sizeof(b_message_data.reqId));
				strncpy((char *)b_message_data.reqId, reqId->valuestring, sizeof(b_message_data.reqId)-1);
			}
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			comm_func_topic = FUNC_UNKNOWN;
			ret = dataDownStream_handler(CONTROL_SET_DEVICE_AUTH, b_message_data.dataResourse);
    }
	//④设备删除
	else if (!strcmp(itemSid->valuestring, "deviceDelete")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *timestamp = cJSON_GetObjectItem(cjson_buff, "ts");
			cJSON *reqId = cJSON_GetObjectItem(cjson_buff, "reqId");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			if (timestamp != NULL && (strcmp(timestamp->string, "ts") == 0)) 
			{
//				int timestampSta = cJSON_GetObjectItem(cjson_buff, "ts")->valueint;
				b_message_data.appControl_timestamp = timestamp->valueint;
			} 
			if (reqId != NULL && (strcmp(reqId->string, "reqId") == 0)) 
			{
//				int cmdSta = cJSON_GetObjectItem(audioCmdSendData, "cmd")->valueint;
//				b_message_data.reqId = (uint8_t *)reqId->valuestring;
				memset(b_message_data.reqId, 0, sizeof(b_message_data.reqId));
				strncpy((char *)b_message_data.reqId, reqId->valuestring, sizeof(b_message_data.reqId)-1);
			}
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			comm_func_topic = FUNC_CONFIG_DEVICEDELTE;
			ret = dataDownStream_handler(CONTROL_SET_DEVICE_DELETE, b_message_data.dataResourse);
    }
	//=============== V1.6 新增指令解析 ===============
	else if (!strcmp(itemSid->valuestring, "voiceGet"))
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *params = cJSON_GetObjectItem(cjson_buff, "params");
			cJSON *timestamp = cJSON_GetObjectItem(cjson_buff, "ts");
			cJSON *reqId = cJSON_GetObjectItem(cjson_buff, "reqId");
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) b_message_data.dataResourse = dataResourse->valueint;
			if (params != NULL && (strcmp(params->string, "params") == 0)) { cJSON *voiceID = cJSON_GetObjectItem(params, "voiceID"); if (voiceID != NULL) b_message_data.appControl_voiceId = voiceID->valueint; }
			if (timestamp != NULL) b_message_data.appControl_timestamp = timestamp->valueint;
			if (reqId != NULL && (strcmp(reqId->string, "reqId") == 0)) { memset(b_message_data.reqId, 0, sizeof(b_message_data.reqId)); strncpy((char *)b_message_data.reqId, reqId->valuestring, sizeof(b_message_data.reqId)-1); } else { ret = 0; goto EXIT; }
			comm_func_topic = FUNC_QUERY_VOICE;
			ret = dataDownStream_handler(CONTROL_QUERY_VOICE, b_message_data.dataResourse);
    }
	else if (!strcmp(itemSid->valuestring, "wifiSsidGet"))
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *timestamp = cJSON_GetObjectItem(cjson_buff, "ts");
			cJSON *reqId = cJSON_GetObjectItem(cjson_buff, "reqId");
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) b_message_data.dataResourse = dataResourse->valueint;
			if (timestamp != NULL) b_message_data.appControl_timestamp = timestamp->valueint;
			if (reqId != NULL && (strcmp(reqId->string, "reqId") == 0)) { memset(b_message_data.reqId, 0, sizeof(b_message_data.reqId)); strncpy((char *)b_message_data.reqId, reqId->valuestring, sizeof(b_message_data.reqId)-1); } else { ret = 0; goto EXIT; }
			comm_func_topic = FUNC_CONFIG_WIFIMAC;//FUNC_QUERY_WIFI;
			ret = dataDownStream_handler(CONTROL_QUERY_WIFI, b_message_data.dataResourse);
    }
	else if (!strcmp(itemSid->valuestring, "geofenceGet"))
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *timestamp = cJSON_GetObjectItem(cjson_buff, "ts");
			cJSON *reqId = cJSON_GetObjectItem(cjson_buff, "reqId");
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) b_message_data.dataResourse = dataResourse->valueint;
			if (timestamp != NULL) b_message_data.appControl_timestamp = timestamp->valueint;
			if (reqId != NULL && (strcmp(reqId->string, "reqId") == 0)) { memset(b_message_data.reqId, 0, sizeof(b_message_data.reqId)); strncpy((char *)b_message_data.reqId, reqId->valuestring, sizeof(b_message_data.reqId)-1); } else { ret = 0; goto EXIT; }
			comm_func_topic = FUNC_CONFIG_FENCE;
			ret = dataDownStream_handler(CONTROL_QUERY_FENCE, b_message_data.dataResourse);
    }
	else if (!strcmp(itemSid->valuestring, "geofenceConfig"))
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *params = cJSON_GetObjectItem(cjson_buff, "params");
			cJSON *timestamp = cJSON_GetObjectItem(cjson_buff, "ts");
			cJSON *reqId = cJSON_GetObjectItem(cjson_buff, "reqId");
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) b_message_data.dataResourse = dataResourse->valueint;
			if (params != NULL && (strcmp(params->string, "params") == 0)) {
				cJSON *fenceId = cJSON_GetObjectItem(params, "fenceId");
				cJSON *action = cJSON_GetObjectItem(params, "action");
				cJSON *fswitch = cJSON_GetObjectItem(params, "switch");
				cJSON *isSet = cJSON_GetObjectItem(params, "isSet");
				cJSON *fenceS = cJSON_GetObjectItem(params, "fenceS");
				cJSON *fenceD = cJSON_GetObjectItem(params, "fenceD");
				if (fenceId) b_message_data.device_geofence.fenceId = fenceId->valueint;
				if (action) b_message_data.appControl_fenceAction = action->valueint;
				if (fswitch) b_message_data.device_geofence.switch_on = fswitch->valueint;
				if (isSet) b_message_data.device_geofence.isSet = isSet->valueint;
				if (fenceS && cJSON_IsArray(fenceS)) { int i=0; cJSON *p; cJSON_ArrayForEach(p, fenceS) { if (i>=GEOFENCE_POINT_MAX) break; cJSON *la=cJSON_GetObjectItem(p,"lat"); cJSON *lo=cJSON_GetObjectItem(p,"lon"); if(la) b_message_data.device_geofence.fenceS[i].lat=la->valuedouble; if(lo) b_message_data.device_geofence.fenceS[i].lon=lo->valuedouble; i++; } }
				if (fenceD && cJSON_IsArray(fenceD)) { int i=0; cJSON *p; cJSON_ArrayForEach(p, fenceD) { if (i>=GEOFENCE_POINT_MAX) break; cJSON *la=cJSON_GetObjectItem(p,"lat"); cJSON *lo=cJSON_GetObjectItem(p,"lon"); if(la) b_message_data.device_geofence.fenceD[i].lat=la->valuedouble; if(lo) b_message_data.device_geofence.fenceD[i].lon=lo->valuedouble; i++; } }
				b_message_data.device_geofence.valid = 1;
			}
			if (timestamp != NULL) b_message_data.appControl_timestamp = timestamp->valueint;
			if (reqId != NULL && (strcmp(reqId->string, "reqId") == 0)) { memset(b_message_data.reqId, 0, sizeof(b_message_data.reqId)); strncpy((char *)b_message_data.reqId, reqId->valuestring, sizeof(b_message_data.reqId)-1); } else { ret = 0; goto EXIT; }
			comm_func_topic = FUNC_CONFIG_FENCE;
			ret = dataDownStream_handler(CONTROL_CONFIG_FENCE, b_message_data.dataResourse);
    }
	else if (!strcmp(itemSid->valuestring, "deviceLight"))
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *params = cJSON_GetObjectItem(cjson_buff, "params");
			cJSON *timestamp = cJSON_GetObjectItem(cjson_buff, "ts");
			cJSON *reqId = cJSON_GetObjectItem(cjson_buff, "reqId");
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) b_message_data.dataResourse = dataResourse->valueint;
			if (params != NULL && (strcmp(params->string, "params") == 0)) { cJSON *color = cJSON_GetObjectItem(params, "color"); cJSON *ltime = cJSON_GetObjectItem(params, "time"); if (color) b_message_data.appControl_lightColor = color->valueint; if (ltime) b_message_data.appControl_lightTime = ltime->valueint; }
			if (timestamp != NULL) b_message_data.appControl_timestamp = timestamp->valueint;
			if (reqId != NULL && (strcmp(reqId->string, "reqId") == 0)) { memset(b_message_data.reqId, 0, sizeof(b_message_data.reqId)); strncpy((char *)b_message_data.reqId, reqId->valuestring, sizeof(b_message_data.reqId)-1); } else { ret = 0; goto EXIT; }
			comm_func_topic = FUNC_CONTROL_LIGHT;
			ret = dataDownStream_handler(CONTROL_SET_LIGHT, b_message_data.dataResourse);
    }
	else if (!strcmp(itemSid->valuestring, "deviceConfig"))
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *params = cJSON_GetObjectItem(cjson_buff, "params");
			cJSON *timestamp = cJSON_GetObjectItem(cjson_buff, "ts");
			cJSON *reqId = cJSON_GetObjectItem(cjson_buff, "reqId");
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) b_message_data.dataResourse = dataResourse->valueint;
			if (params != NULL && (strcmp(params->string, "params") == 0)) {
				cJSON *find = cJSON_GetObjectItem(params, "find"); cJSON *normal = cJSON_GetObjectItem(params, "normal"); cJSON *normalGps = cJSON_GetObjectItem(params, "normalGps"); cJSON *fenceAlert = cJSON_GetObjectItem(params, "fenceAlert"); cJSON *vibrate = cJSON_GetObjectItem(params, "vibrate");
				if (find) b_message_data.device_config.find = find->valueint;
				if (normal) b_message_data.device_config.normal = normal->valueint;
				if (normalGps) b_message_data.device_config.normalGps = normalGps->valueint;
				if (fenceAlert) b_message_data.device_config.fenceAlert = fenceAlert->valueint;
				if (vibrate) b_message_data.device_config.vibrate = vibrate->valueint;
				b_message_data.device_config.valid = 1;
			}
			if (timestamp != NULL) b_message_data.appControl_timestamp = timestamp->valueint;
			if (reqId != NULL && (strcmp(reqId->string, "reqId") == 0)) { memset(b_message_data.reqId, 0, sizeof(b_message_data.reqId)); strncpy((char *)b_message_data.reqId, reqId->valuestring, sizeof(b_message_data.reqId)-1); } else { ret = 0; goto EXIT; }
			comm_func_topic = FUNC_UNKNOWN;
			ret = dataDownStream_handler(CONTROL_SET_DEVICE_CONFIG, b_message_data.dataResourse);
    }

	//产测协议 
	else if (!strcmp(itemSid->valuestring, "productStart")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			ret = productDataDownStream_handler(PRODUCT_START, b_message_data.dataResourse);
    }
	//文件系统注册
	else if (!strcmp(itemSid->valuestring, "productLfsInit")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			ret = productDataDownStream_handler(PRODUCT_LFSINIT, b_message_data.dataResourse);
    }
	//led测试
	else if (!strcmp(itemSid->valuestring, "productLed")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			ret = productDataDownStream_handler(PRODUCT_LED, b_message_data.dataResourse);
    }
	//电机测试
	else if (!strcmp(itemSid->valuestring, "productMotor")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			ret = productDataDownStream_handler(PRODUCT_MOTOR, b_message_data.dataResourse);
    }
	//老化测试
	else if (!strcmp(itemSid->valuestring, "productAsing")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			ret = productDataDownStream_handler(PRODUCT_AGING_TEST, b_message_data.dataResourse);
    }
	//喇叭测试
	else if (!strcmp(itemSid->valuestring, "productAudio")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			ret = productDataDownStream_handler(PRODUCT_AUDIO, b_message_data.dataResourse);
    }
	//lte测试
	else if (!strcmp(itemSid->valuestring, "productLteVer")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			ret = productDataDownStream_handler(PRODUCT_LTE_NO_CACERT, b_message_data.dataResourse);
    }
	//gps测试
	else if (!strcmp(itemSid->valuestring, "productGps")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			ret = productDataDownStream_handler(PRODUCT_GPS, b_message_data.dataResourse);
    }
	//sensor配置
	else if (!strcmp(itemSid->valuestring, "productSensorCfg")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *params = cJSON_GetObjectItem(cjson_buff, "params");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			if (params != NULL && (strcmp(params->string, "params") == 0)) 
			{	
				cJSON *amp = cJSON_GetObjectItem(params, "amp");
				cJSON *stepCnt = cJSON_GetObjectItem(params, "stepCnt");
				cJSON *stepDet = cJSON_GetObjectItem(params, "stepDet");
				cJSON *sbTimer = cJSON_GetObjectItem(params, "sbTimer");
				cJSON *hiEnergy = cJSON_GetObjectItem(params, "hiEnergy	");
				cJSON *powerTime = cJSON_GetObjectItem(params, "powerTime");
				cJSON *lowEnergy = cJSON_GetObjectItem(params, "lowEnergy");
				
				if (amp != NULL && (strcmp(amp->string, "amp") == 0)) 
				{
					b_message_data.product_amp = amp->valueint;
				}
				if (stepCnt != NULL && (strcmp(stepCnt->string, "stepCnt") == 0)) 
				{
					b_message_data.product_stepCnt = stepCnt->valueint;
				}
				if (stepDet != NULL && (strcmp(stepDet->string, "stepDet") == 0)) 
				{
					b_message_data.product_stepDet = stepDet->valueint;
				}
				if (sbTimer != NULL && (strcmp(sbTimer->string, "sbTimer") == 0)) 
				{
					b_message_data.product_sbTimer = sbTimer->valueint;
				}
				if (hiEnergy != NULL && (strcmp(hiEnergy->string, "hiEnergy") == 0)) 
				{
					b_message_data.product_hiEnergy = hiEnergy->valueint;
				}
				if (powerTime != NULL && (strcmp(powerTime->string, "powerTime") == 0)) 
				{
					b_message_data.product_powerTime = powerTime->valueint;
				}
				if (lowEnergy != NULL && (strcmp(lowEnergy->string, "lowEnergy") == 0)) 
				{
					b_message_data.product_lowEnergy = lowEnergy->valueint;
				}
			}
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			ret = productDataDownStream_handler(PRODUCT_SENDORCFG, b_message_data.dataResourse);
    }
	//sensor测试
	else if (!strcmp(itemSid->valuestring, "productSensor")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			ret = productDataDownStream_handler(PRODUCT_SENDOR, b_message_data.dataResourse);
    }
	//ADC测试
	else if (!strcmp(itemSid->valuestring, "productAdc")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			ret = productDataDownStream_handler(PRODUCT_ADC, b_message_data.dataResourse);
    }
	//MAC配置
	else if (!strcmp(itemSid->valuestring, "productSetMac")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *params = cJSON_GetObjectItem(cjson_buff, "params");
		
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			if (params != NULL && (strcmp(params->string, "params") == 0)) 
			{	
				cJSON *mac = cJSON_GetObjectItem(params, "mac");
				
				if (mac != NULL && (strcmp(mac->string, "mac") == 0)) 
				{
					memset(b_message_data.appProduct_bleMac, 0, strlen((char *)b_message_data.appProduct_bleMac));
					memcpy(b_message_data.appProduct_bleMac, (char *)mac->valuestring, strlen((char *)mac->valuestring));
					b_message_data.appProduct_bleMac[12] ='\0';
				}
			}
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			ret = productDataDownStream_handler(PRODUCT_SET_MAC, b_message_data.dataResourse);
    }
	//MAC读取
	else if (!strcmp(itemSid->valuestring, "productGetMac")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
		
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			ret = productDataDownStream_handler(PRODUCT_GET_MAC, b_message_data.dataResourse);
    }
	//SN获取
	else if (!strcmp(itemSid->valuestring, "productGetSn")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			ret = productDataDownStream_handler(PRODUCT_GET_SN, b_message_data.dataResourse);
    }
	//CA证书写入
	else if (!strcmp(itemSid->valuestring, "productCacert")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *params = cJSON_GetObjectItem(cjson_buff, "params");
		
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			if (params != NULL && (strcmp(params->string, "params") == 0)) 
			{	
				cJSON *cacert = cJSON_GetObjectItem(params, "value");
				cJSON *length = cJSON_GetObjectItem(params, "length");
				
				if (length != NULL && (strcmp(length->string, "length") == 0)) 
				{
					b_message_data.appProduct_cacertLen = length->valueint;
				}
				if (cacert != NULL && (strcmp(cacert->string, "value") == 0)) 
				{
					memset(b_message_data.appProduct_cacert, 0, 2048);
					memcpy(b_message_data.appProduct_cacert, (uint8_t *)cacert->valuestring, b_message_data.appProduct_cacertLen);
				}
			}
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			ret = productDataDownStream_handler(PRODUCT_CACERT, b_message_data.dataResourse);
    }
	//CLINET写入
	else if (!strcmp(itemSid->valuestring, "productClient")) 
    {
		COMM_LOG_DEBUG("itemSid->valuestring: %s\n",itemSid->valuestring);
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *params = cJSON_GetObjectItem(cjson_buff, "params");
		
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			if (params != NULL && (strcmp(params->string, "params") == 0)) 
			{	
				cJSON *client = cJSON_GetObjectItem(params, "value");
				cJSON *length = cJSON_GetObjectItem(params, "length");
				
				if (length != NULL && (strcmp(length->string, "length") == 0)) 
				{
					b_message_data.appProduct_cacertLen = length->valueint;
				}
				if (client != NULL && (strcmp(client->string, "value") == 0)) 
				{
					memset(b_message_data.appProduct_cacert, 0, 2048);
					memcpy(b_message_data.appProduct_cacert, (uint8_t *)client->valuestring, b_message_data.appProduct_cacertLen);
				}
			}
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			ret = productDataDownStream_handler(PRODUCT_CLIENT, b_message_data.dataResourse);
    }
	//USERKEY写入
	else if (!strcmp(itemSid->valuestring, "productUserkey")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *params = cJSON_GetObjectItem(cjson_buff, "params");
		
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			if (params != NULL && (strcmp(params->string, "params") == 0)) 
			{	
				cJSON *userkey = cJSON_GetObjectItem(params, "value");
				cJSON *length = cJSON_GetObjectItem(params, "length");
				
				if (length != NULL && (strcmp(length->string, "length") == 0)) 
				{
					b_message_data.appProduct_cacertLen = length->valueint;
				}
				if (userkey != NULL && (strcmp(userkey->string, "value") == 0)) 
				{
					memset(b_message_data.appProduct_cacert, 0, 2048);
					memcpy(b_message_data.appProduct_cacert, (uint8_t *)userkey->valuestring, b_message_data.appProduct_cacertLen);
				}
			}
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			ret = productDataDownStream_handler(PRODUCT_USERKEY, b_message_data.dataResourse);
    }
	//LTE 带证书测试 
	else if (!strcmp(itemSid->valuestring, "productLteConn")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
		
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			ret = productDataDownStream_handler(PRODUCT_LTE_WITH_CACERT, b_message_data.dataResourse);
    }
	//LTE 差分包升级
	else if (!strcmp(itemSid->valuestring, "lteUpdata")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *params = cJSON_GetObjectItem(cjson_buff, "params");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			if (params != NULL && (strcmp(params->string, "params") == 0)) 
			{	
				cJSON *url = cJSON_GetObjectItem(params, "url");
				if (url != NULL && (strcmp(url->string, "url") == 0)) 
				{
//					b_message_data.appControl_lteFirmwareUrl = (uint8_t *)url->valuestring;
					memset(b_message_data.appControl_lteFirmwareUrl, 0, 512);
					memcpy(b_message_data.appControl_lteFirmwareUrl, (uint8_t *)url->valuestring, strlen(url->valuestring));
				}
			}
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			ret = productDataDownStream_handler(PRODUCT_LTE_UPDATA, b_message_data.dataResourse);
    }
	//LTE USB升级
	else if (!strcmp(itemSid->valuestring, "lteUsbUpdata")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			cJSON *params = cJSON_GetObjectItem(cjson_buff, "params");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			if (params != NULL && (strcmp(params->string, "params") == 0)) 
			{	
				cJSON *switch1 = cJSON_GetObjectItem(params, "switch");
				if (switch1 != NULL && (strcmp(switch1->string, "switch") == 0)) 
				{
					b_message_data.appControl_lteUpdataSwitch = switch1->valueint;
				}
			}
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			ret = productDataDownStream_handler(PRODUCT_LTE_USB_UPDATA, b_message_data.dataResourse);
    }
	//设备参数获取
	else if (!strcmp(itemSid->valuestring, "productGetMsg")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
		
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			ret = productDataDownStream_handler(PRODUCT_GET_DEVICEMSG, b_message_data.dataResourse);
    }
	//LTE/GPS复合测试
	else if (!strcmp(itemSid->valuestring, "productMulti")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
		
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			ret = productDataDownStream_handler(PRODUCT_MULTI, b_message_data.dataResourse);
    }
	//恢复出厂设置
	else if (!strcmp(itemSid->valuestring, "deviceReset")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			ret = deviceResetDownStream_handler(PRODUCT_RESET, b_message_data.dataResourse);
    }
	//产测结束
	else if (!strcmp(itemSid->valuestring, "productStop")) 
    {
			cJSON *dataResourse = cJSON_GetObjectItem(cjson_buff, "ds");
			
			if (dataResourse != NULL && (strcmp(dataResourse->string, "ds") == 0)) 
			{
				int dataResourseSta = cJSON_GetObjectItem(cjson_buff, "ds")->valueint;
				b_message_data.dataResourse = dataResourseSta;
			} 
			else
			{
				//ERROR
				ret = 0;
				goto EXIT;
			}
			ret = productDataDownStream_handler(PRODUCT_STOP, b_message_data.dataResourse);
    }
	
EXIT:
	cJSON_Delete(cjson_buff);
    cjson_buff = NULL;
    return ret;
}

/***************************************************************
* 函数名称: data_downStream_decrypt
* 说    明: 下行数据解密
***************************************************************/
int data_downStream_decrypt(unsigned char * msg, unsigned int len)
{
	int ret = 0;
	//解密后组JSON包
	ret = data_downStream_handler(msg, len);//data_downStream_package(msg, len);
	
	return ret;
}

/**
 * @brief  schedule task
 *
 * @param[in] pvParameters  pv parameters
 **/
static void vCommTask(void *argument)
{
    //TaskInfo_t *my_info = (TaskInfo_t *)pvParameters;
    TaskInfo_t *my_task_info = GetTaskInfo(COMM_TASK_ID);
    TaskInfo_t *cat1_task_info = GetTaskInfo(CAT1_UART_TASK_ID);
	TaskInfo_t *entry_task_info = GetTaskInfo(ENTRY_TASK_ID);
	
    Message_t received_comm_msg;
    uint8_t msg_prio;

    COMM_LOG_DEBUG("[COMM][STA] Task %d started\r\n", my_task_info->task_id);
    
    //启动主动上报定时器（每次触发时用CAT1_IsMqttConnected()实时检查，不通则跳过）
    comm_boot_high_freq_active = true;
    comm_boot_start_tick = osKernelGetTickCount();
    CommStartAutoReportTimer(COMM_BOOT_HIGH_FREQ_INTERVAL_MS);
    state_monitor_init();  /* V1.6: 启动100ms状态监控+防抖 */
    CommStartHealthTimer();  /* V1.6: 启动健康定频采样(10s一次, 满10分钟上报healthInfo) */
    
    for(;;) 
    {
        // 接收消息（阻塞等待）
        if(osOK == osMessageQueueGet(my_task_info->queue_handle,&received_comm_msg, &msg_prio, portMAX_DELAY))
        {
			// ===== 被动接收: BLE或CAT1发来的JSON指令 =====
			if((received_comm_msg.source_id == CAT1_UART_TASK_ID || received_comm_msg.source_id == BLE_SCHEDULE_TASK_ID)
				&& received_comm_msg.command == TASK_COMM_DATAJSON)
			{
				if(received_comm_msg.data != NULL)
				{
					// 下行数据解密 + 解析 + 回包（整个流程串行完成后再取下一条消息）
					// BLE下行包在本队列串行排队, 处理完自然取下一条, 无需向EVT回放行信号
					data_downStream_decrypt(received_comm_msg.data, received_comm_msg.data_length);
					
					// 释放消息内存
					if(received_comm_msg.source_id == CAT1_UART_TASK_ID || received_comm_msg.source_id == BLE_SCHEDULE_TASK_ID)
					{
						DEMO_BT_Free(received_comm_msg.data);
					}
				}
			}
			
			// ===== 主动上报: COMM定时器触发 =====
			if(received_comm_msg.source_id == COMM_TASK_ID && received_comm_msg.command == TASK_COMM_AUTO_REPORT)
			{
				/* V1.6 优先级③: 队列有积压(即时/被动待处理)则让位重排队尾, 超限或重排失败则立即执行防饿死
				 * (重排时消息仍在队列里, g_auto_report_pending保持true, 定时器不会另塞新消息) */
				if (osMessageQueueGetCount(my_task_info->queue_handle) > 0 && g_active_defer_cnt < COMM_ACTIVE_DEFER_MAX
				    && osMessageQueuePut(my_task_info->queue_handle, &received_comm_msg, MSG_PRIO_ACTIVE_LOW, 0) == osOK) {
					g_active_defer_cnt++;
					continue;
				}
				g_active_defer_cnt = 0;
				g_auto_report_pending = false;  /* 消息真正消费, 开闸允许定时器投下一条 */

				COMM_LOG_DEBUG("[COMM][STA] Auto report triggered\r\n");
				
				// 开机高频阶段检查: 2分钟后降频
				if(comm_boot_high_freq_active) {
					uint32_t elapsed_ticks = osKernelGetTickCount() - comm_boot_start_tick;
					uint32_t duration_ticks = (uint32_t)((uint64_t)COMM_BOOT_HIGH_FREQ_DURATION_MS * osKernelGetTickFreq() / 1000UL);
					if(elapsed_ticks >= duration_ticks) {
						comm_boot_high_freq_active = false;
						// 切换到常规模式间隔
						CURRENT_MODE_T current_mode = CurrentModeDataGet();
						if(current_mode == CURRENT_MODE_SEARCH_PET) {
							{ uint32_t _iv = COMM_AUTO_REPORT_INTERVAL_SEARCH_MS; if (b_message_data.device_config.valid) _iv = b_message_data.device_config.find * 1000; CommStartAutoReportTimer(_iv); }
						} else {
							{ uint32_t _iv = COMM_AUTO_REPORT_INTERVAL_STANDARD_MS; if (b_message_data.device_config.valid) _iv = b_message_data.device_config.normal * 1000; CommStartAutoReportTimer(_iv); }
						}
					}
				}
				
			// 路由决策: 主动上报优先走BLE，BLE没连接走4G
			uint16_t data_source;
			if(get_ble_status() == BLE_STATE_CONNECTED) {
				data_source = DATA_SOURCE_BLE;
				COMM_LOG_DEBUG("[COMM][STA] Auto report via BLE\r\n");
			} else if (CAT1_IsMqttConnected()) {
				data_source = DATA_SOURCE_4G;
				COMM_LOG_DEBUG("[COMM][STA] Auto report via 4G\r\n");
			} else {
				// BLE未连接且MQTT未连接 → 跳过本次上报
				COMM_LOG_DEBUG("[COMM][STA] Auto report skipped: no BLE & no MQTT\r\n");
				continue;  // 跳过，等下一次定时器触发
			}
				
		comm_func_topic = FUNC_AUTO_DEVICE_STATE;
		dataDownStream_handler(CONTROL_AUTO_DEVICE_STATE, data_source);
	}
			// ===== V1.6: 即时上报(defer): 100ms定时器检测到变化后发消息, 主任务组包 =====
			else if(received_comm_msg.source_id == COMM_TASK_ID && received_comm_msg.command == TASK_COMM_INSTANT_REPORT)
			{
				g_instant_msg_inflight = false;  /* 在途消息已消费(无论mask是否有效) */
				uint32_t mask = g_instant_pending_mask;
				if (mask == 0) continue;  /* 已消费或过期 */
				g_instant_pending_mask = 0;  /* 消费 */
				/* 路由: 蓝牙优先, 4G需MQTT已连; 都不通则放回mask等链路恢复(100ms节拍自动补发, 不丢) */
				DATA_SOURCE_TYPE ds;
				if (get_ble_status() == BLE_STATE_CONNECTED) { ds = DATA_SOURCE_BLE; }
				else if (CAT1_IsMqttConnected()) { ds = DATA_SOURCE_4G; }
				else { g_instant_pending_mask |= mask; continue; }
				g_instant_report_state = g_instant_pending_state;  /* 组包用的状态副本 */
				g_instant_changed_mask = mask;  /* ★组包函数按此全局掩码裁剪data字段, 漏赋值会组出空data{} */
				b_message_data.device_timestamp = get_timestamp_date(NULL);  /* 当前时间, 不沿用上次指令ts */
				/* ★先更新快照再发送: dataUpStream_handler内有1s延时, 若快照后更新,
				 * 延时窗口内100ms监控仍判cur!=last → 同一变化重复触发连发两遍 */
				b_message_data.lastReport_stateFlags = g_instant_pending_state;
				b_message_data.lastReport_battery = b_message_data.device_battery;
				b_message_data.lastReport_posture = (uint8_t)g_instant_pending_state.posture;
				comm_func_topic = FUNC_REPORT_INSTANT_STATE;
				dataUpStream_handler(INSTANT_REPORT_TYPE_DEVICE_STATE, ds);
			}
			// ===== V1.6: 健康定频tick: 10s采样姿态, 满10分钟上报healthInfo(蓝牙优先路由) =====
			else if(received_comm_msg.source_id == COMM_TASK_ID && received_comm_msg.command == TASK_COMM_HEALTH_TICK)
			{
				g_health_tick_pending = false;  /* tick已消费, 开闸允许下个10s tick投递 */
				update_device_state_flags();
				append_posture_to_seq((uint8_t)b_message_data.device_stateFlags.posture);
				g_health_tick_count++;
				if (g_health_tick_count >= COMM_HEALTH_REPORT_TICKS) {
					/* 满10分钟: 上报(蓝牙优先, 4G需MQTT); 优先级③: 队列有积压则本次不上报,
					 * 无链路同理, 姿态数据保留下个tick(10s后)重试 */
					if (osMessageQueueGetCount(my_task_info->queue_handle) == 0 && report_health_info()) {
						g_health_tick_count = 0;
						g_health_window_start_ts = get_timestamp_date(NULL);
					}
				}
			}

	// ===== CAT1发来的旧式主动上报（兼容过渡期）=====
			if(received_comm_msg.source_id == CAT1_UART_TASK_ID)
			{
				if(received_comm_msg.command == TASK_START_WIFISCAN)
				{
					if(received_comm_msg.data != NULL)
					{
						DeviceWifiSsid_t *wifi_data = (DeviceWifiSsid_t *)received_comm_msg.data;
						 // 更新设备WiFi数据
						UpdateCurrentWifiSsid(wifi_data);
						
						dataUpStream_handler(TRACK_REPORT_TYPE_WIFI_LIST, DATA_SOURCE_BLE);
				
						// 释放消息内存
						DEMO_BT_Free(received_comm_msg.data);
						received_comm_msg.data = NULL;
					
					}
				}
//				if(received_comm_msg.command == TASK_DATA_VAL_UPDATA)
//				{
//					dataUpStream_handler(TRACK_REPORT_TYPE_DEVICE_STATE, DATA_SOURCE_4G);
//				}
//				if(received_comm_msg.command == TASK_CAT1_QUERY_ENODEB)
//				{
//					dataUpStream_handler(TRACK_REPORT_TYPE_ENODEB_INFO, DATA_SOURCE_4G);
//				}
				if(received_comm_msg.command == TASK_AUDIO_REALTIME)
				{
					b_message_data.device_exeErrorCode = 0;
					dataUpStream_handler(TRACK_REPORT_TYPE_ERRORCODE, DATA_SOURCE_4G);
				}
			}
			// 接收entry发送的系统模式数据
            if((received_comm_msg.source_id == ENTRY_TASK_ID))
            {
				/* V1.6: ENTRY 发来的状态监控 START/STOP 控制(统一入口, 不再在 MODE_REPORT 里判断) */
				if(received_comm_msg.command == TASK_STATE_MONITOR_START) {
					state_monitor_start();
					CommStartHealthTimer();   /* V1.6: 健康定频与状态监控同步启停 */
				} else if(received_comm_msg.command == TASK_STATE_MONITOR_STOP) {
					state_monitor_stop();
					CommStopHealthTimer();
				} 
//				else 
//				if(received_comm_msg.command == TASK_COMM_MODE_REPORT)
//				{
//					if(get_ble_status() == BLE_STATE_CONNECTED)
//					{
//						dataUpStream_handler(TRACK_REPORT_TYPE_WARNING_STATE, DATA_SOURCE_4G);
//					}
//				}
			}
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
osThreadId_t vStartCommTask(void)
{
    const osThreadAttr_t CommThreadAttr = {
        .name = "Comm_Task",
        .attr_bits = 0,
        .cb_mem = NULL,
        .cb_size = 0,
        .stack_mem = NULL,
        .stack_size = COMM_TASK_STACK_SIZE,
        .priority = COMM_TASK_PRIORITY,
        .tz_module = 0,
    };

    // Create pm Task
    return osThreadNew(vCommTask, NULL, &CommThreadAttr);
}

/** @} */

// vim: fdm=marker
