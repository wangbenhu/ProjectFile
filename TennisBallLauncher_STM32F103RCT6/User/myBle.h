#ifndef __MYBLE_H__
#define __MYBLE_H__

#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "stdarg.h"
#include "stdbool.h"
#include "main.h"
#include "flash.h"

//帧类型	帧模式	帧方向	数据长度	数据	校验
// 1B				1B			1B			1B			15B			1B

//帧类型：用来区分数据包是蓝牙设置还是电量还是参数配置指令；

//帧模式：模式是帧类型包里不同指令的选择，例如参数设置指令就有许多种模式；

//长度：数据字段的实际有效数据长度；

//数据：传输有效数据；

//校验：用来校验整包数据的完整性；

#define DATA_MAX_LEN 15



typedef struct{
	uint8_t data_type;
	uint8_t data_mode;
	uint8_t data_direction;
	uint8_t data_len;
	uint8_t data[DATA_MAX_LEN];
	uint8_t crc;
}BLE_APP_DataFrame;

//类型	含义	说明
//01H	蓝牙设置包	设置蓝牙参数
//02H	电量上报	设备端上报电量信息
//03H	击球模式选择	各种发球模式选择
//04H	发球参数设置	球参设置
//05H	设备和APP连接包	用于APP和设备建立连接的校验包
//06H	开关帧	用于控制机器运转的开关

typedef enum
{
	DATA_TYPE_BLE_SET	=0X01,
	DATA_TYPE_POWER_REPORT,
	DATA_TYPE_SHOT_MODE_SELECT,
	DATA_TYPE_SHOT_SET,
	DATA_TYPE_APP_BLE_CONN,
	DATA_TYPE_SWITCH,
	DATA_TYPE_BALL_ERROR,
	
	DATA_TYPE_END,
}DATA_TYPE_LIST;

//类型	含义	说明
//FFH	非表1.2序号3、4帧数据	Default FFH
//01H	水平	击球模式和参数控制
//02H	垂直	
//03H	交叉	
//04H	定点	
//05H	三角球	
//06H	全场随机	
//07H	全场编程	
//08H	抛球练习	
//09H	变轨球	
//0AH	高压球	
//0BH	截击球	
//0CH	月亮球	
//0DH	削切球（下旋）	
//0EH	练习模式开关	（START）开关帧

typedef enum{
	DATA_MODE_FLAT =0X01,
	DATA_MODE_VERTICAL,
	DATA_MODE_INTERSECT,
	DATA_MODE_FIXED,
	DATA_MODE_TRIANGLE,
	DATA_MODE_RANDOM,
	DATA_MODE_CUSTOM,
	DATA_MODE_THROW,
	DATA_MODE_PATH,
	DATA_MODE_HIGH_PRESSURE,
	DATA_MODE_INTERCEPT,
	DATA_MODE_MOON,
	DATA_MODE_UNDERSPIN,
	DATA_MODE_SWITCH,
	
	DATA_MODE_DEFAULT = 0XFF,
}DATA_MODE_LIST;

//帧方向	数据	备注
//APP->DEVICE	52H	APP向设备发请求包，对应54H
//DEVICE->APP	54H	设备向APP发响应包对应52H
//DEVICE->APP	53H	设备向APP发送请求/主动上报包对应55H
//APP->DEVICE	55H	APP向设备下发响应包对应55H


typedef enum{
	DATA_DIRECTION_APP_DEVICE_REQ	=	0X52,
	DATA_DIRECTION_APP_DEVICE_RES	=	0X54,
	DATA_DIRECTION_DEVICE_APP_REQ	=	0X53,
	DATA_DIRECTION_DEVICE_APP_RES	=	0X55,
	
}DATA_DIRECTION_LIST;

typedef void (*pFunc)(uint8_t *ptr, uint16_t len);

typedef void (*App_pFunc)(void);

typedef enum
{
    AT_TEST 	= 0,
		AT_SETNAME,//设置广播name
		AT_RESET,		//重启设置
		AT_GETCON,		//获取BLE连接状态
		AT_ADVI,	//设置广播间隔 default:100
		
		AT_CMD_END,
		
		AT_BLE_CONN,
		AT_BLE_DISCONN,
	
    AT_RECVER_END,
}AT_CMD;
typedef struct 
{
    AT_CMD at_cmd;
    char *str;    
    pFunc exe;
}AT_Rev_Func;//at_cmd_res






typedef struct 
{
    DATA_TYPE_LIST at_cmd; 
    App_pFunc exe;
}APP_Data_Rev_Func;//at_cmd_res


typedef enum
{
	AT_REQ_TEST 	= 0,
	AT_REQ_SETNAME,//设置广播name
	AT_REQ_RESET,		//重启设置
	AT_REQ_GETCON,		//获取BLE连接状态
	AT_REQ_ADVI,	//设置广播间隔 default:100
	
  AT_REQ_END,
}AT_REQ_CMD_LIST;

typedef struct 
{
    AT_CMD list_num;
    char *str;  
		pFunc exe;  
}AT_Send_List;//at_cmd_req

typedef struct 
{
    AT_REQ_CMD_LIST list_num;
    char *str;  
		pFunc exe;  
}AT_Req_List;//at_cmd_req
uint8_t ble_status_get(void);
bool ManuFlag_Get(void);
//void ManuFlag_Set(bool ststus);
uint8_t APP_Data_Res_call_fun(uint8_t APP_Data_Req_Idx);
void ble_data_init(void);
void ble_at_cmd_call_fun(uint8_t at_cmd_index);
void myBleAT_H(void);
void myBleAT_L(void);
void Power_Report_Set(float data);
GPIO_PinState myBleStatusRead(void);
void myMcuToBleSendData(uint8_t *tx_buf, uint8_t len);
void mcuToBleSendATCommand(uint8_t *tx_buf, uint8_t len);
void uartReceiveDataFromBle(void);
#endif
