#ifndef __GNSS_UART_TASK_H
#define __GNSS_UART_TASK_H
#include "FreeRTOS.h"
#include "queue.h"

typedef void(*pFunc)(uint8_t *str, uint16_t len);
typedef enum {
	GPS_BACKUP = 0,	  //进入backup
	GPS_BACKUP_EXIT,
	GPS_FIRMWARE,     //查询固件号
	GPS_TURN_DOUBLE,	//切换双频
	GPS_TURN_SIMPLE,  //切换单频
	GPS_SCANNING,     //设置搜索模式
	GPS_TOTAL_COLD,    //完全冷启动
	GPS_COLD,          //冷启动
	GPS_WARM,  					//温启动
	GPS_HOT,            //热启动
	GPS_SET_OUT,        //设置输出语句
	GPS_SET_PQTMUNIQID,
	
	// 双频相关指令（拆分为独立步骤）
	GPS_DUAL_BAND_READ,    // 读取双频状态
	GPS_DUAL_BAND_LOCK,    // 锁定休眠模式 (PAIR382,1)
	GPS_DUAL_BAND_OFF,     // 关闭GNSS (PAIR003)
	GPS_DUAL_BAND_ENABLE,  // 启用双频 (PAIR104,1)
	GPS_DUAL_BAND_RESTART, // 重启GNSS (PAIR002)
	GPS_DUAL_BAND_COLD,    // 冷启动确保状态一致
} gps_at_type_t;


typedef enum
{	
	RECV_GPS_PQTMUNIQID,
	RECV_GPS_PQTMVERNO,
	RECV_GPS_GNGLL,
	RECV_GPS_GPGLL,
    RECV_GPS_GNRMC,
	RECV_GPS_GPRMC,
    RECV_GPS_GNVTG,
    RECV_GPS_GNGGA,
	RECV_GPS_GPGGA,
    RECV_GPS_GNGSA,
    RECV_GPS_GPGSV,
    RECV_GPS_GLGSV,
    RECV_GPS_GAGSV,
    RECV_GPS_GBGSV,
	RECV_GPS_PAIR105,     // 双频状态查询响应
	
    RECV_AT_END
}RECV_AT_CMD;
typedef struct
{
	RECV_AT_CMD recvAtCmd;
	char *str;
	pFunc exe;
}Recv_At_Func;


// GPS数据处理结果
typedef enum {
    GPS_RESPONSE_NONE = 0,     // 未处理
    GPS_RESPONSE_SUCCESS,      // 成功响应
    GPS_RESPONSE_ERROR,        // 错误响应
    GPS_RESPONSE_AUTO_REPORT,  // 自动上报数据
    GPS_RESPONSE_UNKNOWN       // 未知响应
} GpsResponseResult;

typedef enum {
    GPS_CMD_NONE,
    GPS_CMD_LOW_POWER,
    GPS_CMD_HIGH_ACCURACY,
} gpsCmdType_t;


typedef enum {
    GPS_STATE_IDLE,      // 空闲可立即发送
    GPS_STATE_TX,        // 指令已发送
    GPS_STATE_RX         // 接收数据中
} gps_state_t;

typedef enum {
	
    MODE_HOME,       // 居家模式
    MODE_BACKUP,    // 户外模式
    MODE_PET_FIND    // 寻宠模式
} Mode_t;

typedef struct {
	  gps_state_t  gps_state;
	  Mode_t mode;         // 当前模式
} CmdUartState_t;

// GPS工作模式
typedef enum {
    GPS_MODE_NORMAL = 0,    // 常规模式
    GPS_MODE_PET_FIND = 1,  // 寻宠模式
    GPS_MODE_HOME = 2       // 居家模式（GPS 完全断电，不走周期状态机）
} gps_work_mode_t;

// GPS硬件状态
typedef enum {
    GPS_HW_OFF = 0,         // 关机/BACKUP状态
    GPS_HW_ON = 1           // 开机/退出BACKUP状态
} gps_hw_state_t;

// 状态机状态
typedef enum {
    GPS_STATE_DEFAULT = 0,         // 空闲/关机状态
    GPS_STATE_SEARCHING = 1,    // 搜星中（1分钟定时器）
    GPS_STATE_SLEEPING = 2,     // 休眠中（10分钟定时器）
    GPS_STATE_ACTIVE = 3        // 持续工作（寻宠模式）
} gps_fsm_state_t;

// 定时器类型
typedef enum {
    TIMER_NONE = 0,
    TIMER_1MIN_SEARCH = 1,      // 1分钟搜星定时器
    TIMER_10MIN_SLEEP = 2       // 10分钟休眠定时器
} gps_timer_type_t;


struct ProductionGpsFlags {
    unsigned int gps_flag_get_version : 1; 
    unsigned int gps_reserved : 7;
};

void gpsCmd_parse_task(void *pvParameters);
void gps_data_parsing(uint8_t *beltRecvSubpackage, uint16_t parsing_fifo_length);
void initGps(void);

void handle_home_mode(void);
void handle_outdoor_mode(void);
void handle_pet_find_mode(void);

#endif 