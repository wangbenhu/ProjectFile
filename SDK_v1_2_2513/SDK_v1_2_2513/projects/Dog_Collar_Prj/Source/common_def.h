#ifndef __common_def__
#define __common_def__

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include "om_log.h"
/* Kernel includes. */
#include "cmsis_os2.h"
#include "lfs_port.h"
#include "ble_state_machine.h"

#define DEBUG_LOG_ON 2

#if (DEBUG_LOG_ON == 1)
	#define log_batt_debug(...) om_log(OM_LOG_INFO, ##__VA_ARGS__)
	#define log_debug(...) 
#elif (DEBUG_LOG_ON == 2)
	#define log_debug(...) om_log(OM_LOG_INFO, ##__VA_ARGS__)
	#define log_batt_debug(...) 
#else
	#define log_debug(...)
	#define log_batt_debug(...) 
#endif

#define NUM_TASKS END_TASK_ID  // 要创建的子任务数量
#define QUEUE_LENGTH 16
#define MESSAGE_SIZE sizeof(Message_t)

#define BLE_DATA_MTU_MAX	240
#define BLE_DATA_BUFFER_MAX	2048
#define MQTT_DATA_MTU_MAX	6144
#define MQTT_DATA_URC	2048
#define BATTERY_COLLECT_INTERVAL_TIME_MAX (60)	//unit/s

#define UART_GNSS_RECV_DATA_SIZE (2048)
typedef enum{
    CHARGE_STATUS_NO_CHARGE = 0,//未在充电
    CHARGE_STATUS_CHARGING,//正在充电未满
    CHARGE_STATUS_FULL,//充电满,代表充电中，但是充满信号已经指示
    CHARGE_STATUS_INVALID,
}CHARGE_STATUS_T;

typedef enum {
    MODE_M1 = 0,
    MODE_M2,
    MODE_M3,
    MODE_M4,
    MODE_M5,
    MODE_ERROR,
} SystemMode_t;

#define CONFIG_TEST_PRODUCTION (1)
#if (CONFIG_TEST_PRODUCTION)
	#define PRODUCTION_TASK_EXAMPLE_NONE    	(0)
    #define PRODUCTION_TASK_EXAMPLE_START    	(1UL << 0)//进入生产测试模式
    #define PRODUCTION_TASK_EXAMPLE_1    		(1UL << 1) //文件系统初始化
    #define PRODUCTION_TASK_EXAMPLE_2    		(1UL << 2)//LED测试
    #define PRODUCTION_TASK_EXAMPLE_3    		(1UL << 3) //电机测试
    #define PRODUCTION_TASK_EXAMPLE_4    		(1UL << 4)//喇叭测试
    #define PRODUCTION_TASK_EXAMPLE_5    		(1UL << 5) //LTE不带证书
    #define PRODUCTION_TASK_EXAMPLE_6   	 	(1UL << 6)//GNSS测试
    #define PRODUCTION_TASK_EXAMPLE_7   	 	(1UL << 7) //sensor配置 测试
    #define PRODUCTION_TASK_EXAMPLE_8   	 	(1UL << 8) //ADC测试
    #define PRODUCTION_TASK_EXAMPLE_9   	 	(1UL << 9) //产测结束
	#define PRODUCTION_TASK_EXAMPLE_REPORT   	(1UL << 10) //测试结果上报

	#define PRODUCTION_TASK_EXAMPLE_10   	(1UL << 11) //CA证书写入
	#define PRODUCTION_TASK_EXAMPLE_11  	(1UL << 12) //CLIENT写入
	#define PRODUCTION_TASK_EXAMPLE_12  	(1UL << 13) //Userkey写入

	#define PRODUCTION_TASK_EXAMPLE_13  	(1UL << 14) //带证书测试
	
	
	#define PRODUCTION_TASK_EXAMPLE_14  	(1UL << 15) //DFOTA 差分升级
	#define PRODUCTION_TASK_EXAMPLE_15  	(1UL << 16) //USB升级
	#define PRODUCTION_TASK_EXAMPLE_16  	(1UL << 17) //老化测试
	#define PRODUCTION_TASK_EXAMPLE_17  	(1UL << 18) //老化测试
	#define PRODUCTION_TASK_EXAMPLE_END  	(1UL << 19) //

#define ALL_PRODUCTION_TASK_EXAMPLE	(PRODUCTION_TASK_EXAMPLE_START | PRODUCTION_TASK_EXAMPLE_1|\
	PRODUCTION_TASK_EXAMPLE_2 | PRODUCTION_TASK_EXAMPLE_3 | PRODUCTION_TASK_EXAMPLE_4 |\
	PRODUCTION_TASK_EXAMPLE_5 | PRODUCTION_TASK_EXAMPLE_6 | PRODUCTION_TASK_EXAMPLE_7 |\
	PRODUCTION_TASK_EXAMPLE_8 | PRODUCTION_TASK_EXAMPLE_9 | PRODUCTION_TASK_EXAMPLE_REPORT |\
	PRODUCTION_TASK_EXAMPLE_10 | PRODUCTION_TASK_EXAMPLE_11 | PRODUCTION_TASK_EXAMPLE_12 |\
	PRODUCTION_TASK_EXAMPLE_13 | PRODUCTION_TASK_EXAMPLE_14 | PRODUCTION_TASK_EXAMPLE_15 |\
	PRODUCTION_TASK_EXAMPLE_16 | PRODUCTION_TASK_EXAMPLE_17 | PRODUCTION_TASK_EXAMPLE_END)
#endif
typedef enum{
	REPORT_OK = 0,
	REPORT_MOTOR_ERROR,
	REPORT_AUDIO_ERROR,
	REPORT_SENSOR_ERROR,
	REPORT_ADC_ERROR,	
	REPORT_REPETITION_ERROR,
	REPORT_CA_WRITE_ERROR,
	REPORT_CLIENT_ERROR,
	REPORT_USERKEY_ERROR,
	REPORT_LFS_INIT_ERROR,
	REPORT_CAT1_CMD_ERROR,
	REPORT_OTHER_ERROR,
	REPORT_LFS_MOUNTED_ERROR,
	REPORT_CAT1_TEST_ERROR,
	REPORT_PLAY_AUDIO_INDEX_ERROR,
	REPORT_ASING_ERROR,
	REPORT_ERROR_CODE_MAX = 255,
	
}eProductionReportCod;
typedef enum{
    ENTRY_TASK_ID = 0,
    BLE_SCHEDULE_TASK_ID,
    PM_TASK_ID,
    CAT1_UART_TASK_ID,
	UART_DATARECV_ID,		//add to received uart data
    GNSS_UART_TASK_ID,
	UART_GNSSDATARECV_ID,
    COMM_TASK_ID,
    AUDIO_TASK_ID,
    ASSIST_TASK_ID,
    SENSOR_TASK_ID,
    TEST_TASK_ID,
	MOTOR_TASK_ID,
	LED_TASK_ID,
    END_TASK_ID,
}TASK_ID_T;

typedef enum{
    TASK_CMD_START,
    TASK_CMD_STOP,
    TASK_CMD_REPLY,

	// ===== 即时上报/路由新增命令 =====
	TASK_COMM_AUTO_REPORT,
	TASK_COMM_INSTANT_REPORT,
	TASK_STATE_MONITOR_START,   //V1.6: ENTRY通知COMM启动状态监控(活跃模式)
	TASK_STATE_MONITOR_STOP,    //V1.6: ENTRY通知COMM停止状态监控(低功耗/休眠)    //V1.6: 100ms状态监控检测到变化, defer到主任务组包          //COMM定时器触发的主动上报（蓝牙优先路由）
	TASK_COMM_HEALTH_TICK,      //V1.6: 健康定频10s采样tick(满10分钟组包上报healthInfo)
	TASK_COMM_BLE_SEND_DONE,        //COMM处理完BLE数据包后通知EVT释放信号量

	TASK_CMD_CAT1_TEST_START,//发起CAT1的SN读取
	TASK_CMD_CAT1_TEST_REPLAY,//返回CAT1的读写结果
	
    TASK_INFO_CHARGE, 		//PM task上报电池、充电状态
	TASK_SYSTEM_MODE,   	//comm task上报系统模式
    TASK_GPS_MODE_REPLY,          //gps数据获取回复
	TASK_STOP_REPLY,
    TASK_TIMER_VAL_UPDATE,   //更新PM task的定时器周期
	TASK_COMM_DATAJSON,
	TASK_DATA_VAL_UPDATA,	//请求更新数据
	TASK_DATA_DEVICEINFO,	//查询设备信息
	TASK_CMD_CONTROL,		//控制指令
	TASK_GPS_CHANGEMODE  ,   //更改gps模式
	TASK_GPS_STOP,
	TASK_GPS_START,
	TASK_REPORT_MOTION_LEVEL, //上报运动强度
	TASK_READ_BATTERY, 		//读取电量
    TASK_COMM_MODE_REPORT,  //上报通信模式
//	TASK_CAT1_DEVICE_STATE_REPORT,  //主动上报设备状态（CAT1触发，已弃用，由COMM定时器接管）
	
	TASK_CAT1_QUERY_ENODEB,		//查询基站信息
	TASK_START_WIFISCAN,  //下发wifi scan设置指令
	TASK_AUDIO_REALTIME,		//发送实时音频存储播放
	TASK_CAT1_AWS_NO_TEST,		//读取CAT1版本号
	TASK_CAT1_AWS_WITHCA_TEST,	//读取CAT1通信（带证书）
	TASK_CAT1_AWS_CACERT,		//写入CA证书
	TASK_CAT1_AWS_CLIENT,		//写入CLIENT
	TASK_CAT1_AWS_USERKEY,		//写入USERKEY
	TASK_CAT1_UPDATA,			//LTE差分升级
	TASK_CAT1_USB_UPDATA,		//LTE USB升级
	TASK_FACTORY_RESET,			//恢复出厂设置：删除UFS证书
	
	TASK_PRODUCT_GPS_TEST,		//读取GPS版本号和搜星标志
	TASK_CMD_CAT1_VERSION_REPLY,	//CAT1不带通信
	TASK_CAT1_AWS_WITHCA_TEST_REPLY,//CAT1带通信
	TASK_PRODUCT_GPS_TEST_REPLY,	//GNSS测试回复
	TASK_CAT1_AWS_CACERT_REPLY,		//返回CA证书写入结果
	TASK_CAT1_AWS_CLIENT_REPLY,		//写入CLIENT
	TASK_CAT1_AWS_USERKEY_REPLY,		//写入USERKEY
	TASK_CAT1_UPDATA_REPLY,
	TASK_CAT1_USB_UPDATA_REPLY,
	TASK_FACTORY_RESET_REPLY,			//恢复出厂设置REPLY
	
	TASK_CAT1_AWS_TEST,						//弃用
	TASK_DEVICE_RESET,//恢复出场设置 		弃用
	
	TASK_CAT1_AWS_TEST_REPLY,				//弃用
	TASK_CAT1_AWS_NO_TEST_REPLY,			//弃用
	
	CERT_PRODUCT_ERROR,
	TASK_LED_SWITCH_STATE,   // 切换LED模式（蓝牙配对、充电、低电量、追踪、唤醒）
	TASK_LED_CONTROL,		// 单独控制led常亮、常灭
	TASK_LED_BLINK,			// 单独控制led闪烁
	TASK_LED_BREATH,		// 单独控制led呼吸
	TASK_LED_RUNNING,		// 单独控制led跑马
	TASK_LED_REMOVE_STATE,	 // 停止led某个状态
	
	TASK_TEST_START,	 // 测试开始
	
	TASK_CAT1_DELETE_DEVICE,	 //删除设备成功
	
    TASK_CMD_TEST_EXAMPLE,
    TASK_CMD_END
}TASK_CMD_T;

//typedef enum{
//    USER_STATUS_ACTIVE,
//    USER_STATUS_INACTIVE,
//    USER_STATUS_INVALID,
//}USER_STATUS_T;

typedef enum{
	GPIO_STATUS_LOW =0,
	GPIO_STATUS_HIGH,
}GPIO_STATUS_T;


typedef enum {
    BATTERY_LEVEL_EMPTY = 0,    // 0% ~ 10%      严重低电，快关机   <=3.49V    
    BATTERY_LEVEL_LOW,         // 10% ~ 20%     低电量           3.64 ~ 3.50
    BATTERY_LEVEL_MEDIUM,      // 21% ~ 99%     中等电量         3.65 ~4.14
    BATTERY_LEVEL_FULL,        // 100%          满电             >=4.15V  
    BATTERY_LEVEL_ERROR,
} BATTERY_LEVEL_T;

// GPS搜星质量等级（用于云端上报）
typedef enum {
    GPS_QUALITY_WEAK = 0,       // 弱信号 - SNR低，信号非常不好
    GPS_QUALITY_FAIR = 1,       // 一般 - 无星历但SNR较高，可能在窗边恢复快
    GPS_QUALITY_GOOD = 2,       // 良好 - 有星历，信号好
} gps_position_quality_t;

// SNR阈值定义（dB-Hz），未定位时用于判断搜星质量等级
#define GPS_SNR_THRESHOLD_AVG_FAIR  20   // SNR平均 >= 此值：一般
#define GPS_SNR_THRESHOLD_FAIR  25		 //达标信号质量

typedef enum{
    CHARGE_STATUS_VOID,//未获得定位信息
//    CHARGE_STATUS_STAR_HUNTING,//正在寻星
	CHANGE_STATUS_POSITION    //定位
}CHARGE_STATUS_GPS;
typedef struct{
    CHARGE_STATUS_T  ChargeStatus;
    ReadLfsCreatFlag   UserStatus;   
    BATTERY_LEVEL_T  BatteryLevel;
}SET_MODE_PARE_T;

// 卫星信息结构体
typedef struct {
    uint8_t satellite_id;      // 卫星标识号
    uint8_t snr;               // 信噪比/载噪比 (0-99)
} SatelliteInfo_t;
typedef struct {
    uint8_t gps_status;        // 定位状态
    uint8_t longitude[20];         // 原始经度字符串（如"11706.913200"）
    uint8_t latitude[20];          // 原始纬度字符串（如"3149.332006"）
    uint8_t timestamp[12];       // 原始时间戳字符串（如"093423.000"）
    uint8_t lat_dir;            // 南北纬（N=0, S=1）
    uint8_t lon_dir;            // 东西经（E=0, W=1）
	 BaseType_t need_update_gnss;        // GNSS数据更新使能标志（pdTRUE=需更新）
	// 新增卫星信息字段
    uint8_t satellite_count;   // 可见卫星数量
    SatelliteInfo_t satellites[20]; // 卫星信息数组（最多20颗）
    uint8_t used_satellites;   // 定位使用的卫星数量
    gps_position_quality_t position_quality;  // 定位质量等级（0=弱,1=一般,2=良好）
	
	uint8_t last_valid_longitude[20]; 
    uint8_t last_valid_latitude[20];  
    bool has_last_valid;
} GPS_STATUS_t;

// 定义基站信息结构体
typedef struct {
    // 服务小区信息 - 严格匹配JSON格式
    char cell_rat[8];               // "LTE"
    char cell_duplex[8];            // "FDD" 或 "TDD"
    uint16_t mcc;                    // 460
    uint16_t mnc;                    // 11
    char carrier[32];                // 运营商名称（需从AT+COPS?获取）
    char cell_id[20];                 // "149881123" (十六进制转十进制后的字符串)
    uint32_t cell_earfcn;            // 1650
    char cell_pcid[20];              // 460
    uint8_t cell_band;               // 3
    char cell_tac[20];                // c
    int16_t cell_rsrp;               // -95
    int16_t cell_rsrq;               // -8
    int16_t cell_rssi;               // -75
    uint16_t cell_sinr;              // 23
	/* V1.6: 删除临近小区(neighbour_*)字段 */
    
} CellInfo_t;
/* ============ V1.6 新增: 设备状态位(deviceState 的 state 逗号字段) ============ */
typedef enum { 
	STATE_BATT_INVALID=0, 
	STATE_BATT_NORMAL=1, 
	STATE_BATT_LOW=2 
} state_battery_t;
typedef enum {
	STATE_CHARGE_INVALID=0, 
	STATE_CHARGE_CHARGING=1, 
	STATE_CHARGE_UNPLUG=2 
} state_charge_t;
typedef enum {
	STATE_FENCE_INVALID=0, 
	STATE_FENCE_UNCONFIG=1, 
	STATE_FENCE_SAFE=2, 
	STATE_FENCE_DANGER=3, 
	STATE_FENCE_OUTSIDE=4 
} state_fence_t;
typedef enum {
	STATE_POSTURE_INVALID=0, 
	STATE_POSTURE_STATIC=1, 
	STATE_POSTURE_LYING=2, 
	STATE_POSTURE_RUNNING=3, 
	STATE_POSTURE_SITTING=4, 
	STATE_POSTURE_SNIFFING=5, 
	STATE_POSTURE_WALKING=6, 
	STATE_POSTURE_OTHER=9 
} state_posture_t;
typedef enum {
	STATE_MODE_INVALID=0, 
	STATE_MODE_STANDARD=1, 
	STATE_MODE_SEARCH=2 
} state_mode_t;
typedef enum {
	STATE_HOME_INVALID=0, 
	STATE_HOME_UNCONFIG=1, 
	STATE_HOME_CONFIG_OFF=2, 
	STATE_HOME_CONFIG_ON=3 
} state_home_t;
typedef struct {
	state_battery_t battery; 
	state_charge_t charge; 
	state_fence_t fence; 
	state_posture_t posture;
	state_mode_t mode; 
	state_home_t home; 
} DeviceStateFlags_t;

/* ============ V1.6 新增: 围栏 ============ */
#define GEOFENCE_POINT_MAX  10
typedef struct { 
	double lat; 
	double lon; 
} GeofencePoint_t;
typedef struct { 
	uint8_t fenceId; 
	uint8_t switch_on; 
	uint8_t isSet; 
	GeofencePoint_t fenceS[GEOFENCE_POINT_MAX]; 
	GeofencePoint_t fenceD[GEOFENCE_POINT_MAX]; 
	uint8_t valid; 
} Geofence_t;

/* ============ V1.6 新增: 配置表(只走蓝牙) ============ */
typedef struct { 
	uint32_t find; 				//寻宠周期
	uint32_t normal; 			//常规周期
	uint32_t normalGps; 		//GPS开关周期
	uint32_t fenceAlert; 		//围栏告警周期
	uint32_t vibrate; 			//震动时间
	uint8_t valid; 				
} DeviceConfig_t;


typedef struct {
    CHARGE_STATUS_T charge_status;  // 充电状态
    uint8_t BAT_Capacity;  //0~100, 电池容量
} BAT_STATUS_t;

typedef enum
{
	CURRENT_MODE_DEFAULT = 0,
	CURRENT_MODE_STANDARD = 1,
	CURRENT_MODE_SEARCH_PET = 2,
	CURRENT_MODE_LOW_BATTERY = 3,
	CURRENT_MODE_CHARGE = 4
}CURRENT_MODE_T;
typedef struct {
    CURRENT_MODE_T currentMode;  // 当前模式 default：0 标准：1 寻宠：2 充电：3 低电量：4
    uint8_t BAT_Capacity;  //0~100, 电池容量
} COMM_GET_STATUS_t;
// 自定义消息结构体
typedef struct {
    uint8_t source_id;  // 消息来源
    uint8_t dest_id;    // 消息目标
    uint8_t command;   ///命令
    void* data;      // 消息数据
	uint16_t data_length;      // 消息数据长度
} Message_t;


// 任务和队列句柄存储结构
typedef struct {
    osThreadId_t task_handle;
    osMessageQueueId_t queue_handle;
    uint8_t task_id;
} TaskInfo_t;
/**
 * @brief 应用控制模式参数结构体
 * @details 包含当前模式和GPS时间间隔
 */
typedef struct {
	uint8_t mode;									//当前模式：1标准模式；2寻宠模式
	uint32_t gps_interval;  						//GPS时间间隔UNIT:S
} AppControlMode_t;
typedef enum{
    MODE_STANDARD = 1,       //标准模式
    MODE_SEARCH_PET,        //寻宠模式
} MODE_M5_t;


//定义BLE连接鉴权状态
typedef enum {
    BLE_AUTH_STATE_IDLE = 0,      // 空闲状态（未连接或已断开）
    BLE_AUTH_STATE_WAITING,  // 等待鉴权状态（连接后10秒内）
    BLE_AUTH_STATE_SUCCESS,  // 鉴权成功状态
} ble_auth_state_t;
extern ble_auth_state_t ble_auth_state;

//wifi扫描列表
typedef struct {
    char ssid[32];      // SSID名称
    char mac[18];       // MAC地址 (格式: "112233445566")
    char rssi[8];       // 信号强度 (格式: "-45")
} WifiInfo_t;

// 定义wifi列表结构体
typedef struct {
    uint16_t wifi_count;        	// wifi数量                    
    WifiInfo_t wifi_list[10];      // wifi列表指针
} DeviceWifiSsid_t;


// 定义功能类型枚举
typedef enum {
    FUNC_QUERY_DEVICE_STATE,
	FUNC_AUTO_DEVICE_STATE,
    FUNC_CONTROL_DEVICE_MODE,
    FUNC_QUERY_ENODEB,
    FUNC_CONTROL_AUDIOCMD,
    FUNC_CONFIG_TIMESYNC,
    FUNC_CONFIG_WIFIMAC,
	FUNC_CONFIG_DEVICEDELTE,
	FUNC_REPORT_WARNINGSTATE,
	FUNC_CONTROL_VIBRATE,
	FUNC_CONTROL_LIGHT,
	FUNC_CONFIG_FENCE,
	FUNC_QUERY_HEALTH,
	/* V1.6 新增 */
	FUNC_QUERY_VOICE,
	FUNC_REPORT_INSTANT_STATE,
	FUNC_PRODUCT_TEST,
	
    FUNC_UNKNOWN
} mqtt_function_t;

extern TaskInfo_t task_info[NUM_TASKS];


char *strnstr(const char *haystack, const char *needle, size_t len);
osMessageQueueId_t* GetTaskQueue(TASK_ID_T task_id);
TaskInfo_t* GetTaskInfo(TASK_ID_T task_id);

BleState_t get_ble_status(void);
bool CAT1_IsMqttConnected(void);
	

void err_lock(const char *func, int line);
#define LOG_LOC()  err_lock(__func__, __LINE__);
/******GPS定位数据 updata******/
void GPS_GetCurrentData(GPS_STATUS_t* gps_data);

/******CAT1数据 updata******/
uint8_t CAT1_GetCurrentCsq(void);

/******CAT1基站信息******/
bool CELL_GetCurrentData(CellInfo_t *cell_info_out);

/******系统模式获取******/
CURRENT_MODE_T CurrentModeDataGet(void);

uint8_t* DEVICE_GetCat1Sn(void);
uint8_t* DEVICE_GetCat1Version(void);

//uint8_t* DEVICE_GetEsimSn(void);
uint8_t* DEVICE_GetMac(void);
uint8_t* DEVICE_GetGnssSn(void);
uint8_t* DEVICE_GetGnssVersion(void);

/*******蓝牙鉴权超时定时器********/
void deviceAuthStartTimer(void);
void deviceAuthStopTimer(void);

void product_set_addr(uint8_t *address);
bool SetModePare_SetUser(ReadLfsCreatFlag user);
bool SetModePare_SetBattery(uint8_t battery);
bool SetModePare_SetCharge(CHARGE_STATUS_T charge);
pmu_reboot_reason_t m_system_get_reboot_reason(void);
void user_initiative_reboot_fun(void);
SystemMode_t Entry_Task_Run_Mode_Get(void);
__STATIC_INLINE void drv_pmu_retention_reg2_set(uint16_t value)
{
  OM_CRITICAL_BEGIN();
  register_set(&OM_PMU->RSVD_SW_REG[0], PMU_RSVD_SW_REG_USER_RETENTION_MASK,(value<<PMU_RSVD_SW_REG_USER_RETENTION_POS));
  OM_CRITICAL_END();
}
__STATIC_INLINE uint16_t drv_pmu_retention_reg2_get(void)
{
    return ((OM_PMU->RSVD_SW_REG[0]&PMU_RSVD_SW_REG_USER_RETENTION_MASK)>>PMU_RSVD_SW_REG_USER_RETENTION_POS);  
}
#endif

