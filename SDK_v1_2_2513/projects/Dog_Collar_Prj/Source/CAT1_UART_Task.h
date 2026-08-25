#ifndef __CAT1_UART_Task__
#define __CAT1_UART_Task__

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include "om_log.h"
/* Kernel includes. */
#include "cmsis_os2.h"

#define AT_CMD_QUEUE_SIZE 100


#define MQTT_CONN_PASSWORD		1		//0 - 证书连接；1 - 账户密码连接
#define MQTT_HOST_NAME_TEST		"45.192.104.38"	//"mqtt.meexhpet.com"
#define MQTT_TPORT_TEST			(1883)
#define MQTT_USER				"test2"	//topic/example"
#define MQTT_PASSWORD			"test123.*"	//topic/example"

#define MQTT_CLIENT_IDX			(0)
#define MQTT_CLIENT_ID			"89430103223249468197"
#define MQTT_HOST_NAME			"mqtt.meexhpet.com"//"mqtt.meexhpet.com"//"pet-mqtt.tokscan.com"//"broker.emqx.io"	//"pet-mqtt.tokscan.com"	//"broker.emqx.io"
#define MQTT_TPORT				(8883)//(8884)//(1883)	//(8884)	//8883
#define MQTT_MES_ID				1	//(1)
#define MQTT_TOPIC_SUB			"notify"	//topic/example"
#define MQTT_TOPIC_PUB			"notify1"	//topic/example"

//#define MQTT_PUBMESSAGE_DATA		"{\"serviceId\":\"deviceInformation\",\"characteristics\":{\"dataResourse\": \"BLE/4G\",\"timestamp\":\"2025-08-01T16:30:45Z\",\"cmd\":\"queryDeviceInfo\"}}"
#define MQTT_SUBSCRIBE_QOS		(0)
#define MQTT_MSG_ISRETAIN			(0)
#define MQTT_CFG_TYPE			"keepalive"
#define MQTT_KEEPALIVETIME		(30 * 60)
#define WIFISCAN_TIME		(12000)
#define WIFISCAN_ROUND		(1)
#define WIFISCAN_NUM		(6)
#define WIFISCAN_TIMEOUT		(5)
#define WIFISCAN_PRIORITY		(0)
#define WIFISCAN_SSID_TYPE		(1)

#define HTTP_READFILE_BLOCKSIZE		(8192)
#define HTTP_READFILE_AUDIONAME		"audio_realtime"

#define MQTT_QUEUE_SIZE 10


typedef enum {
	LTE_TEST = 0,
	LTE_QFLST,
	LTE_QFUPL_CACERT,
	LTE_QFUPL_CLIENT,
	LTE_QFUPL_USERKEY,
	LTE_QMTCFG_RECV,
	//配置SSL
	LTE_QMTCFG_SSL,
	LTE_QMTCFG_SELECT_VERSION,
	LTE_QMTCFG_VERSION,
	LTE_QSSLCFG_SECLEVEL,
	LTE_QSSLCFG_SSLVERSION,
	LTE_QSSLCFG_CIPHERSUITE,
	LTE_QSSLCFG_SNI,
	LTE_QSSLCFG_IGNOREMULTICA,
	LTE_QSSLCFG_IGNOREINVALIDCA,
	LTE_QSSLCFG_CACERT,
	LTE_QSSLCFG_CLIENTCERT,
	LTE_QSSLCFG_CLIENTKEY,
	LTE_QFUPL_CACERT_DATA,
	LTE_QFUPL_CLIENT_DATA,
	LTE_QFUPL_USERKEY_DATA,
	// 新增删除证书命令
    LTE_QFDEL_CACERT,
    LTE_QFDEL_CLIENT, 
    LTE_QFDEL_USERKEY,
    LTE_CFUN_REBOOT,
	LTE_ENTER_SLEEP,
	LTE_CFUN_SHUTDOWN,	
	LTE_ATE0,
	LTE_DATAFORMAT,
	LTE_VERSION,
	LTE_CREG,    
	LTE_CGATT,	
    LTE_CPIN,
	LTE_CSQ,
	LTE_GET_COPS,
	LTE_QENG_SERVE,
	LTE_QENG_NEIGHBOUR,
	LTE_RANDIS,
	LTE_MAIN_RING,
	LTE_CIMI,
	LTE_SET_APN,
	LTE_GET_APN,
	LTE_GET_CGACT,
	LTE_SET_CGACT,
	LTE_DEQIACT,
	LTE_MQTT_PDP,
	LTE_QIACT,
	LTE_GET_QIACT,
	LTE_COPS,
	LTE_MQTT_OPEN,
	LTE_MQTT_CLOSE,
	LTE_MQTT_CONN,
	LTE_MQTT_ISSTATE,
	LTE_MQTT_DISCONN,
	LTE_MQTT_SESSION,
	LTE_MQTT_SUB_PRODUCT_TEST,
	LTE_MQTT_SUB_CHECK,
	LTE_MQTT_SUB_DEVICESTATUE,
	LTE_MQTT_SUB_DEVICEMODE,
	LTE_MQTT_SUB_ENODEB,
//	LTE_MQTT_SUB_AUDIOREALTIME,
	LTE_MQTT_SUB_AUDIOCMD,
	LTE_MQTT_SUB_DEVICEDELTE,
	LTE_MQTT_SUB_WARNINGSTATE,
	LTE_MQTT_SUB_VIBRATE,
	/* V1.6 新增订阅主题 */
	LTE_MQTT_SUB_TIMESYNC,
	LTE_MQTT_SUB_WIFIMAC,
	LTE_MQTT_SUB_FENCE,
	LTE_MQTT_SUB_VOICE,
	LTE_MQTT_SUB_LIGHT,
	LTE_MQTT_USUB_DEVICESTATUE,
	LTE_MQTT_USUB_DEVICEMODE,
	LTE_MQTT_USUB_ENODEB,
//	LTE_MQTT_USUB_AUDIOREALTIME,
	LTE_MQTT_USUB_AUDIOCMD,
	LTE_MQTT_USUB_DEVICEDELTE,
	LTE_MQTT_USUB_WARNINGSTATE,
	LTE_MQTT_USUB_VIBRATE,
	/* V1.6 新增取消订阅主题 */
	LTE_MQTT_USUB_TIMESYNC,
	LTE_MQTT_USUB_WIFIMAC,
	LTE_MQTT_USUB_FENCE,
	LTE_MQTT_USUB_VOICE,
	LTE_MQTT_USUB_LIGHT,
	LTE_MQTT_PUB,
	LTE_MQTT_PRODUCT_PUB,
	LTE_MQTT_PUBMESSAGEDATA,
	LTE_MQTT_PRODUCT_PUBMESSAGEDATA,
	LTE_KEEPALIVE,
	LTE_MQTT_MSG,
	LTE_DEVICE_GSN,
	LTE_DEVICE_QCCID,
	LTE_WIFI_SCAN,
//	LTE_SET_APN1,
//	LTE_QIACT1,
	LTE_QHTTPCFG_PDP,
	LTE_QHTTPCFG_HEADER,
	LTE_QSSLCFG_HTTPS_SSLCTXID,
	LTE_QSSLCFG_HTTPS_SECLEVEL,
	LTE_QSSLCFG_HTTPS_SSLVERSION,
	LTE_QSSLCFG_HTTPS_CIPHERSUITE,
	LTE_QHTTP_QSSLCFG_CLIENTKEY,
	LTE_QHTTPURL,
	LTE_QHTTPURL_DATA,
	LTE_QHTTGET,
	LTE_QFDEL_AUDIO,
	LTE_QHTTPREADFILE,
	LTE_QFOPEN,
	LTE_QFREAD,
	LTE_QFCLOSE,
	LTE_QFDEL,
	LTE_FIRMWARE_UPDATA

} lte_at_type_t;


// 定义LTE数据处理结果
typedef enum {
    RESPONSE_NONE = 0,     // 未处理
    RESPONSE_SUCCESS,      // 成功响应
    RESPONSE_ERROR,        // 错误响应
	RESPONSE_COMPLETE,
//    RESPONSE_RETRY,  // 自动上报数据
//    RESPONSE_UNKNOWN       // 未知响应
} ResponseResult;


typedef enum
{
	/*网络*/
	RECV_AT_CREG = 0,
	RECV_AT_CGATT,
	
	RECV_AT_CPIN,
	RECV_AT_CSQ,
	RECV_AT_COPS,
	RECV_AT_QENG,
	
	/*GPS、音频指令*/
//	RECV_AT_IMQTTSTATE,
	RECV_AT_QMTOPEN,
	RECV_AT_QMTCONN,
	RECV_AT_QMTSUB,
	RECV_AT_QMTUNS,
	RECV_AT_QMTCFG,
	RECV_AT_QMTCLOSE,
	RECV_AT_QMTDISCS,
	RECV_AT_QMTPING,
	RECV_AT_QMTRECV,
	RECV_AT_CGDCONT,
	RECV_AT_QMTSTAT,
	RECV_AT_QMTPUBEX,
	
	RECV_AT_WIFISCAN,
	
	RECV_AT_QHTTPREADFILE,
	RECV_AT_QFLST,
	RECV_AT_QFDEL,
	RECV_AT_QFOPEN,
	RECV_AT_QFCLOSE,
	RECV_AT_CONNECT,
	RECV_AT_QHTTPGET,
	RECV_AT_QFUPL,
	
	RECV_AT_GSN,
	RECV_AT_QCCID,
	RECV_AT_UPDATA,
	RECV_AT_CGACT,
	RECV_AT_QIURC,
	
	RECV_AT_ERROR,
	RECV_AT_END
}RECV_AT_CMD;

//数据接收的类型
typedef enum {
    DATA_TYPE_UNKNOWN,
    DATA_TYPE_COMMAND_RESPONSE,  // 命令响应（OK, ERROR等）
    DATA_TYPE_URC                 // 主动上报数据（URC，如+QMTRECV）
} data_type_t;

// 添加URC命令列表
static const char* urc_commands[] = {
    "+QMTRECV",     // MQTT接收数据
    "+QMTSTAT",     // MQTT状态变化
    "+QIND",        // 固件升级主动上报
    "+QFUPL",       // 文件上传完成
	"RDY",         // 模块开机就绪URC
	"boot.rom",
    NULL
};

//CAT1 查询信号质量
typedef enum{
    SIGNAL_IS_FAIL,			//未检测到信号
	SIGNAL_IS_DETECTABLE   	//信号正常范围
}SIGNAL_STATUS_t;

typedef struct {
    uint16_t cat1_signal;        			 //信号质量
	SIGNAL_STATUS_t cat1_signal_status;          // 信号监测状态
} CAT1_STATUS_t;

	
typedef struct {
    uint8_t *data;
    uint16_t data_len;
    char topic[128];
} mqtt_packet_t;


typedef struct {
    uint8_t httpUrlData[512];     // 输入的网址数据
    uint16_t httpUrlLength;       // 输入的网址的长度
    char     filename[64];        // 文件名
    uint16_t file_size;           // 读取一包文件大小
    uint16_t file_handle;         // 文件句柄
    uint32_t read_offset;         // 读取偏移量
	 uint32_t read_size;			//总文件大小
} CAT1_HttpData_t;

typedef struct {
    uint8_t firmwareUrlData[512];     // 固件的网址数据
    uint16_t firmwareUrlLength;       // 固件的网址的长度
} CAT1_Updata_t;

typedef enum {
	CACERT_UNKNOWN =0,
    CACERT_CA,
	CACERT_CLIENT,
	CACERT_USERKEY
    
} CACERT_Type_t;

typedef enum {
    CAT1_CONFIG_OLD = 0,  // 使用旧的AT参数和证书名称
    CAT1_CONFIG_NEW = 1   // 使用新的AT参数和证书名称
} cat1_config_type_t;

//证书写入的状态机
typedef enum {
    CERT_STATE_IDLE = 0,
    CERT_STATE_WRITING,
    CERT_STATE_WAITING_RESPONSE,
    CERT_STATE_COMPLETE,
    CERT_STATE_ERROR
} CertWriteState_t;


struct ProductionFlags {
    unsigned int flag_set_sn : 1; 
	unsigned int flag_get_version : 1;
	unsigned int flag_cat1_conn : 1;
    unsigned int flag_set_cacert : 1;
    unsigned int flag_set_client : 1;
    unsigned int flag_set_userkey : 1;
	unsigned int flag_factory_reset : 1;
	unsigned int flag_set_lte_updata : 1;
	
	unsigned int flag_set_lte_usb_updata : 1;
    unsigned int reserved : 7; 		// 保留7个bit
};

/**
 * @brief 解除阻塞并重启 CAT1 任务与 UART 接收任务
 * @param source_id 发起重启的源任务 ID (如 ENTRY_TASK_ID)
 * @return uint8_t 0-成功，其他-失败
 */
uint8_t cat1_task_restart(TASK_ID_T source_id);

#endif