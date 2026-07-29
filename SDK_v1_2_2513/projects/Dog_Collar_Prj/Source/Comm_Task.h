#ifndef __Comm_Task__
#define __Comm_Task__

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include "om_log.h"
/* Kernel includes. */
#include "cmsis_os2.h"
#include "common_def.h"

//下行数据接收存储
typedef struct {
	
	uint8_t 		cmd[32];						//功能ID
	uint8_t   		dataResourse;					//1:ble;	2:4g
	uint8_t 		dataType;						//数据类型：历史数据还是实时数据
	
	//APP下发
	uint32_t		appControl_timestamp;			//时间戳 UTC
	uint8_t			reqId[20];						//reqId
	uint32_t		appControl_historyTime;			//历史数据时间
	uint16_t		appControl_dataNum;				//历史数据数量
	//设置模式		
	AppControlMode_t	appControl_currentMode;
	uint8_t			appControl_vibrationSwitch;		//震动开关
	uint16_t		appControl_vibrationTime;		//震动时间
	uint8_t			appControl_vibrationFrequence;	//震动频率
	uint8_t			appControl_lightSwitch;			//灯开关
	uint8_t			appControl_lightColor;			//灯颜色
	uint8_t			appControl_lightFrequence;		//灯频率
	//发送实时语音
	uint8_t			*appControl_audioRealTimeUrl;	//实时语音URL
	uint32_t		appControl_audioRealTimeLen;	//实时音频总长度
	//发送语音指令
	uint8_t			appControl_audioSwitch;			//语音播放开关
	uint8_t			appControl_audioIndex;			//播放语音的索引
	uint8_t			appControl_audioSound;			//声音大小
	//震动时间
	uint16_t		appControl_vibrateTime;			//配置的震动时间
	//配置音频文件
	uint8_t			*appControl_audioConfigFile;	//配置的音频文件
	uint16_t		appControl_audioConfigLength;	//音频文件大小
	//接收语音文件时间
	uint16_t		appControl_audioRecvTime;		//接收语音的时长		

	//设备上报
	uint32_t		device_timestamp;				//设备的时间戳
	uint8_t			*device_wifiRssi;				//WiFi信号
	uint16_t		device_battery;					//电池电量
	uint8_t			device_currentMode;				//设备当前模式
	uint8_t			device_motionState;				//运动状态
	uint32_t		device_stepDiffer;				//步数增幅
	
	//GPS定位信息
    uint8_t 		gps_status;        				// 定位状态
    uint8_t 		longitude[20];        			// 原始经度字符串（如"11706.913200"）
    uint8_t 		latitude[20];         			// 原始纬度字符串（如"3149.332006"）
    uint8_t 		timestamp[12];        			// 原始时间戳字符串（如"093423.000"）
    uint8_t 		lat_dir;           				// 南北纬（N=0, S=1）
    uint8_t 		lon_dir;           				// 东西经（E=0, W=1）
	// 卫星数据
    uint8_t 		satellite_count;
    SatelliteInfo_t satellites[20];
    gps_position_quality_t position_quality;  // 定位质量等级（0=弱,1=一般,2=良好）
	
	uint16_t		device_cat1Csq;					//cat1信号质量
	uint8_t			device_bleMAC[13];				//蓝牙MAC
	uint8_t			device_esimIccid[40];			//eSIM卡号
	uint8_t			device_gnss[40];				//GNSS序列号
	uint8_t			device_cat1Sn[50];				//CAT1序列号
	//基站信息
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
    char cell_tac[20];                // 4041
    int16_t cell_rsrp;               // -95
    int16_t cell_rsrq;               // -8
    int16_t cell_rssi;               // -75
    uint16_t cell_sinr;              // 23
	/* V1.6: 删除 message_data_t 的 neighbour_* 字段 */
	
	//设置WIFI MAC
	uint8_t			*appControl_wifiSsid;			//设置WiFi的SSID
	uint8_t			*appControl_wifiMac;			//设置WiFi的MAC
	DeviceWifiSsid_t device_wifiScanData;
	
	uint8_t			authData[32];					//authData
	//错误码
	uint8_t			device_exeErrorCode;			//错误码

	// ===== 告警标志 + 字段掩码(2026-07-14) =====
	uint8_t			deviceWarningFlags;				//告警位图(bit0=低电, bit1=充电, bit2=拔电, bit3=围栏,...)
	uint16_t		deviceStateMask;				//deviceState JSON字段掩码(dataType=3时生效), 见DeviceFieldMask_t
	uint8_t			device_chargeStatus;			//充电状态(0=未充电, 1=充电中)
	bool			gpsCoordsChanged;				//GPS经纬度相比上次是否有变化(UpdateAutoDeviceState中设置)
	//CAT1版本号
	uint8_t			appProduct_cat1Version[50];		//CAT1版本号
	uint8_t			appProduct_gpsVersion[50];		//GNSS版本号
	
	//AWS 证书
	uint8_t			appProduct_cacert[2048];	
	uint32_t		appProduct_cacertLen;	

	//蓝牙MAC配置
	uint8_t			appProduct_bleMac[13];
	uint32_t			deviceProduct_bleMAC[30];

	//LTE差分包URL
	uint8_t			appControl_lteFirmwareUrl[512];	//LTE差分包URL
	//LTE USB升级开关
	uint8_t			appControl_lteUpdataSwitch;
	
	//sensor配置的参数
	uint8_t			product_amp;
	uint8_t			product_stepCnt;
	uint8_t			product_stepDet;
	uint8_t			product_sbTimer;
	uint8_t			product_hiEnergy;
	uint8_t			product_powerTime;
	uint8_t			product_lowEnergy;
	
	//产测错误码
	uint8_t			product_exeErrorCode;			//错误码
	
	/* ================ V1.6 协议新增字段 ================ */
	uint8_t			device_hardVersion[16];
	uint8_t			device_sn[32];
	uint8_t			device_model[16];
	DeviceStateFlags_t	device_stateFlags;
	DeviceStateFlags_t	lastReport_stateFlags;
	uint16_t		lastReport_battery;
	uint8_t			lastReport_posture;
	uint8_t			device_postureActionCode;
	Geofence_t		device_geofence;
	uint8_t			appControl_fenceAction;
	DeviceConfig_t	device_config;
	uint8_t			appControl_voiceId;
	uint8_t			appControl_voiceIndex;
	uint8_t			appControl_lightTime;
	double			appControl_wifiLat;
	double			appControl_wifiLon;
	double			device_wifiLat;
	double			device_wifiLon;
	uint8_t			device_wifiConfigState;

} message_data_t;
extern message_data_t b_message_data;

typedef enum {
    DATA_SOURCE_BLE = 1,
    DATA_SOURCE_4G = 2
} DATA_SOURCE_TYPE;

/* 设备状态上报字段掩码 (即时上报 dataType=3 时使用, OR组合) */
typedef enum {
    DEVICE_FIELD_TIMESTAMP    = (1 << 0),   // 时间戳
    DEVICE_FIELD_BATTERY      = (1 << 1),   // 电量
    DEVICE_FIELD_MODE         = (1 << 2),   // 模式
    DEVICE_FIELD_MOTION       = (1 << 3),   // 运动状态
    DEVICE_FIELD_STEP         = (1 << 4),   // 步数差
    DEVICE_FIELD_GPS          = (1 << 5),   // GPS(仅在定位有效+坐标变化时含lat/lng)
    DEVICE_FIELD_CSQ          = (1 << 6),   // 信号强度
} DeviceFieldMask_t;

// 告警位图定义(deviceWarningFlags), 与 DeviceFieldMask_t 配合使用
#define WARNING_FLAG_LOW_BATTERY    (1 << 0)   // 低电量(≤20%)
#define WARNING_FLAG_CHARGING       (1 << 1)   // 充电中
#define WARNING_FLAG_POWER_DISCONN  (1 << 2)   // 拔电触发
#define WARNING_FLAG_FENCE_ALERT    (1 << 3)   // 围栏告警


/* 蓝牙/云端发送指令功能的类型 */
typedef enum {
	CONTROL_CHECK_DEVICE_STATE,				//查询整机运行状态
	CONTROL_AUTO_DEVICE_STATE,
    CONTROL_CHECK_DEVICE_INFO,				//查询设备信息
	CONTROL_CHECK_ENODEB_INFO,				//查询基站信息
	CONTROL_SET_DEVICE_MODE,				//设置设备模式
    CONTROL_SET_AUDIO_CMD,					//设置发送语音播放
	CONTROL_SET_VIBRATE_CMD,				//设置发送震动
	CONTROL_SET_AUDIO_REALTIME,				//发送实时语音
    CONTROL_SET_AUDIO_FILE,					//设置语音文件
    CONTROL_SET_AUDIO_RECV,					//设置接收语音文件
	CONTROL_SET_NOBARK,						//设置止吠
	CONTROL_SET_TIME_SYNCHRONIZE,			//设置同步时间
	CONTROL_SET_WIFI_SCAN,					//开启WiFi指令
	CONTROL_SET_WIFI_SSID,					//配置WiFi SSID
	CONTROL_SET_DEVICE_AUTH,				//鉴权指令
	CONTROL_SET_DEVICE_DELETE,				//设置删除设备

	/* V1.6 新增控制类型 */
	CONTROL_QUERY_VOICE,
	CONTROL_QUERY_WIFI,
	CONTROL_QUERY_FENCE,
	CONTROL_CONFIG_FENCE,					//V1.6: 围栏设置/删除/开关
	CONTROL_SET_LIGHT,
	CONTROL_SET_DEVICE_CONFIG,

	CONTROL_NO_TASK
} APP_CONTROL_Type;
extern APP_CONTROL_Type controlType;


/*上报的数据指令*/
typedef enum _RESPONSE_TYPES_
{
	TRACK_REPORT_TYPE_DEVICE_STATE,			//上报整机运行状态
	AUTO_REPORT_TYPE_DEVICE_STATE,
	TRACK_REPORT_TYPE_DEVICE_INFO,			//上报设备信息
	TRACK_REPORT_TYPE_ENODEB_INFO,			//上报基站信息
	TRACK_REPORT_TYPE_AUDIO_RECV,			//回复语音采集文件
	TRACK_REPORT_TYPE_WIFI_LIST,			//回复wifi设置列表
	TRACK_REPORT_TYPE_SYSTEM_TIME,			//时间同步
	TRACK_REPORT_TYPE_WARNING_STATE,		//上报异常模式
	
	/* V1.6 新增上报类型 */
	TRACK_REPORT_TYPE_VOICE_GET,
	TRACK_REPORT_TYPE_WIFI_GET,
	TRACK_REPORT_TYPE_GEOFENCE_GET,
	TRACK_REPORT_TYPE_GEOFENCE_CONFIG,
	TRACK_REPORT_TYPE_DEVICE_LIGHT,
	TRACK_REPORT_TYPE_DEVICE_CONFIG,
	TRACK_REPORT_TYPE_HEALTH_INFO,
	INSTANT_REPORT_TYPE_DEVICE_STATE,

	TRACK_REPORT_TYPE_ERRORCODE,			//回复成功/错误码
	
	PRODUCT_REPORT_TYPE_ERRORCODE,			//产测成功/错误码
	PRODUCT_REPORT_TYPE_SN,					//回复SN
	PRODUCT_REPORT_TYPE_LTE_CONN,			//测试回复
	PRODUCT_REPORT_TYPE_CAT1_VER,			//回复CAT1版本号
	PRODUCT_REPORT_TYPE_GPS_VER,			//回复GPS版本号
	PRODUCT_REPORT_TYPE_BLEMAC,				//回复蓝牙MAC地址
	PRODUCT_REPORT_TYPE_DEVICEMSG,			//回复设备参数
	
	TRACK_REPORT_NO_TASK
}RESPONSE_TYPES;


/* 蓝牙/云端发送指令功能的类型 */
typedef enum {
	//产测协议
	PRODUCT_START,							//产测开始
	PRODUCT_LFSINIT,						//文件系统注册
	PRODUCT_LED,							//LED测试
	PRODUCT_MOTOR,							//电机测试
	PRODUCT_AUDIO,							//喇叭测试
	PRODUCT_LTE_NO_CACERT,					//LTE测试
	PRODUCT_GPS,							//GPS测试
	PRODUCT_SENDORCFG,							//SENSOR配置
	PRODUCT_SENDOR,							//SENSOR测试
	PRODUCT_ADC,							//ADC测试
	PRODUCT_GET_SN,
	PRODUCT_SET_MAC,
	PRODUCT_GET_MAC,
	PRODUCT_CACERT,
	PRODUCT_CLIENT,
	PRODUCT_USERKEY,
	PRODUCT_LTE_WITH_CACERT,
	PRODUCT_LTE_UPDATA,						//LTE差分包升级
	PRODUCT_LTE_USB_UPDATA,					//LTEUSB升级
	PRODUCT_GET_DEVICEMSG,					//获取设备参数
	PRODUCT_STOP,							//产测结束
	PRODUCT_MULTI,							//LTE/GPS复合测试
	PRODUCT_RESET,							//恢复出厂设置
	PRODUCT_AGING_TEST,						//老化测试
	
	PRODUCT_NO_TASK
} PRODUCT_CONTROL_Type;
extern PRODUCT_CONTROL_Type productControlType;

typedef enum
{
    MODE_CHARGE = 1,      // 充电模式
    MODE_LOW_BATTERY = 2, // 低电量模式  
    MODE_NORMAL = 3       // 常规模式
} REPORT_MODE_T;
uint8_t usb_updata_flag_get(void);
uint8_t usb_updata_flag_get(void);

/* ============ V1.6 新增API声明 ============ */
void check_and_report_instant_state(void);
void append_posture_to_seq(uint8_t posture_code);
bool report_health_info(void);   /* V1.6: 蓝牙优先/4G需MQTT, 无链路返回false */
void state_monitor_init(void);
void state_monitor_start(void);
void state_monitor_stop(void);
extern char g_posture_seq[128];
#endif
