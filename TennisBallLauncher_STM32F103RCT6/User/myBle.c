#include "myBle.h"
#include "usart.h"
#include "stm32f1xx_it.h"
#include <string.h>
#include "myApp.h"
#include "crc8.h"
/*
notes:
1.data_type_ble_set_req_handle 暂未实现name动态修改和flash操作
2.ble_status_task_handle();//鉴权任务 app适配失败暂时注释
*/



extern struct ble_uart_type bleUart;

static bool ManuFlag=false;//产测模式标志
static float power_report_data=0.0;//电池电量
BLE_APP_DataFrame BLE_APP_RES_DATA;
BLE_APP_DataFrame APP_BLE_REQ_DATA;



enum{
	BLE_STATUS_START					=	0,
	BLE_SET_ADV_NAME_REQ,
	BLE_SET_ADV_NAME_RES,
	BLE_SET_ADV_ADVI_REQ,
	BLE_SET_ADV_ADVI_RES,
	
	BLE_TASK_APP_RUN,	//蓝牙空闲广播
	
	BLE_TASK_CONN_START,
	BLE_TASK_CONN_OK,
	BLE_TASK_CONN_ERROR,
	
	BLE_TASK_CONN_CHANGE_NAME_REQ,
	BLE_TASK_CONN_CHANGE_NAME_RES,
	
	BLE_STATUS_END,
	
}Ble_Task_Status_List;

static uint8_t ble_task_status = BLE_STATUS_START;//蓝牙任务状态机

enum{
	BLE_STATE = 0,				//蓝牙处于广播状态
	BLE_CONN_NOTAUTH ,		//蓝牙处于连接状态但是没鉴权
	BLE_CONN_AUTHOK,			//蓝牙正式建立成功
	BLE_END,
}Ble_Status_List;

static struct
{
	uint8_t adv_name_set[USER_NAME_MAX];		//广播name	:default:fffffffffff		11B >=48  <=57 hex
	uint16_t adv_disc;											//广播间隔defaule :100							2B 				hex
	uint8_t ble_status;											//连接状态	default :0广播 1连接
	uint8_t at_set_tmp;											//req指令的id值用于快速匹配对应的res
	uint8_t app_ble_conn_ok;								//连接正式建立标志
	uint8_t at_req_flag;										//0:active 1：run
	
	uint8_t at_send_interval_count;
	
	uint8_t set_name_flag;
	
	uint8_t set_mode_flag;
	
	uint8_t set_switch_flag;
	
	uint8_t set_shot_flag;
	
}Ble_Data_Struct={{0},50,BLE_STATE,AT_REQ_END,0,0,0,0,0,0,0};

static void at_ble_conn_res_handle(uint8_t *str, uint16_t len);
static void at_ble_disconn_res_handle(uint8_t *str, uint16_t len);
static void at_test_res_handle(uint8_t *str, uint16_t len);
static void at_setname_res_handle(uint8_t *str, uint16_t len);
static void at_reset_res_handle(uint8_t *str, uint16_t len);
static void at_getcon_res_handle(uint8_t *str, uint16_t len);
static void at_advi_res_handle(uint8_t *str, uint16_t len);

static void at_test_req_handle(uint8_t *str, uint16_t len);
static void at_setname_req_handle(uint8_t *str, uint16_t len);
static void at_reset_req_handle(uint8_t *str, uint16_t len);
static void at_getcon_req_handle(uint8_t *str, uint16_t len);
static void at_advi_req_handle(uint8_t *str, uint16_t len);
static void at_disc_req_handle(uint8_t *str, uint16_t len);

void data_type_ble_set_req_handle(void);
void data_type_shot_mode_select_req_handle(void);
void data_type_shot_set_req_handle(void);
void data_type_app_ble_conn_req_handle(void);
void data_type_switch_req_handle(void);

void data_type_ble_set_res_handle(void);
void data_type_shot_mode_select_res_handle(void);
void data_type_shot_set_res_handle(void);
void data_type_app_ble_conn_res_handle(void);
void data_type_switch_res_handle(void);
void data_type_power_report_res_handle(void);
void data_type_ball_error_res_handle(void);
static bool ble_status_set(uint8_t status);

static const APP_Data_Rev_Func APP_Data_Req_Func_List[] =
{
	{DATA_TYPE_BLE_SET,           					data_type_ble_set_req_handle					},
	{DATA_TYPE_POWER_REPORT,          			NULL																	},
	{DATA_TYPE_SHOT_MODE_SELECT,          	data_type_shot_mode_select_req_handle	},
	{DATA_TYPE_SHOT_SET,           					data_type_shot_set_req_handle					},
	{DATA_TYPE_APP_BLE_CONN,           			data_type_app_ble_conn_req_handle			},
	{DATA_TYPE_SWITCH,          						data_type_switch_req_handle						},
	{DATA_TYPE_BALL_ERROR,          				NULL																	},
		
	{DATA_TYPE_END,           							NULL																	},		
};

static const APP_Data_Rev_Func APP_Data_Res_Func_List[] =
{
	{DATA_TYPE_BLE_SET,           		data_type_ble_set_res_handle					},
	{DATA_TYPE_POWER_REPORT,        	data_type_power_report_res_handle			},
	{DATA_TYPE_SHOT_MODE_SELECT,      data_type_shot_mode_select_res_handle	},
	{DATA_TYPE_SHOT_SET,           		data_type_shot_set_res_handle					},
	{DATA_TYPE_APP_BLE_CONN,        	data_type_app_ble_conn_res_handle			},
	{DATA_TYPE_SWITCH,          			data_type_switch_res_handle						},
	{DATA_TYPE_BALL_ERROR,          	data_type_ball_error_res_handle						},
		                                                                 
	{DATA_TYPE_END,           				NULL																		},
};                                                                   	



//res_list_init
static const AT_Rev_Func at_cmd_func[] =
{
    {AT_TEST,           		"",           			at_test_res_handle				},
		{AT_SETNAME,           	"+S_NAME=",         at_setname_res_handle			},
		{AT_RESET,           		"",         				at_reset_res_handle				},	
    {AT_GETCON,           	"+CON=",           	at_getcon_res_handle			},
		{AT_ADVI,           		"+S_ADVI=",         at_advi_res_handle				},
		
		{AT_CMD_END,           		NULL,         NULL				},
		{AT_BLE_CONN,          	"+CONNECTED",   		at_ble_conn_res_handle		},
		{AT_BLE_DISCONN,        "+DISCONNECTED",  	at_ble_disconn_res_handle	},
//	低功耗开启
    {AT_RECVER_END, 								NULL, 								NULL						}
};
////req_list_init
static const AT_Req_List AT_Send_ID[] =
{
    {AT_REQ_TEST,           		"AT",							at_test_req_handle			},
		{AT_REQ_SETNAME,           	"AT+NAME=",				at_setname_req_handle		},
		{AT_REQ_RESET,           		"AT+RESET",				at_reset_req_handle			},
    {AT_REQ_GETCON,           	"AT+GETCON?",			at_getcon_req_handle		},
		{AT_REQ_ADVI,           		"AT+ADVI=",				at_advi_req_handle			},
//	低功耗开启
    {AT_REQ_END,							 	NULL,							NULL										}
};
/*
*********************************************************************************************************
* 函 数 名:Power_Report_Set
* 功能说明: 电池电量设置
* 形  参:   电压值
* 返 回 值: 无
*********************************************************************************************************
*/
void Power_Report_Set(float data)
{
	power_report_data=data;
}

/*
*********************************************************************************************************
* 函 数 名:ManuFlag_Set
* 功能说明: APP任务状态机设置
* 形  参:   ststus 产测状态 true:产测模式 false:运行模式
* 返 回 值: 无
*********************************************************************************************************
*/
void ManuFlag_Set(bool ststus)
{
	ManuFlag=ststus;
}


/*
*********************************************************************************************************
* 函 数 名:ManuFlag_Get
* 功能说明: APP任务状态机设置
* 形  参:   无
* 返 回 值: 产测状态
*********************************************************************************************************
*/
bool ManuFlag_Get(void)
{
	return ManuFlag;
}

 uint8_t adv_data_get(void)
{
	uint8_t flash_get_name[USER_NAME_MAX]={0};
	uint8_t return_data=0;
	
	return_data = FLASH_EEPROM_Read(flash_get_name,USER_NAME_MAX,USER_CUSTOM_DATA_FLASH);
//		debug_uart_printf("11111111111 = ");
//	for(uint8_t i=0;i<USER_NAME_MAX;i++)
//	{
//		debug_uart_printf(" %02x ",flash_get_name[i]);
//	}
//	debug_uart_printf("\r\n");
	for(uint8_t i=0;i<USER_NAME_MAX;i++)
	{
		if(flash_get_name[i]<48 || flash_get_name[i]>57)
		{
			
			return_data=1;
			break;
		}
	}
	if(return_data)
	{
		for(uint8_t i=0;i<USER_NAME_MAX;i++)
		{
			Ble_Data_Struct.adv_name_set[i]=0x46;
		}
		
		ManuFlag_Set(true);
	}
	else{
		memcpy(Ble_Data_Struct.adv_name_set,flash_get_name,USER_NAME_MAX);
		ManuFlag_Set(false);
	}
//	debug_uart_printf("11111111111 = ");
//	for(uint8_t i=0;i<USER_NAME_MAX;i++)
//	{
//		debug_uart_printf(" %02x ",Ble_Data_Struct.adv_name_set[i]);
//	}
//	debug_uart_printf("\r\n");
	
	return return_data;
}

/******************************
*************************
* Function name : data_type_ble_set_req_handle
* Description   : ble广播参数设置
* Parameter     : void
* Return        : void
********************************************************/
void data_type_ble_set_req_handle(void)
{
	uint8_t set_name_buffer[USER_NAME_MAX]={0};
	if(APP_BLE_REQ_DATA.data_len==USER_NAME_MAX)
	{
		Ble_Data_Struct.set_name_flag=1;
		for(int i=0;i<USER_NAME_MAX;i++)
		{
			if(APP_BLE_REQ_DATA.data[i]>='0'&&APP_BLE_REQ_DATA.data[i]<='9')
			{
				set_name_buffer[i]=APP_BLE_REQ_DATA.data[i];
			}		
			else
			{
				Ble_Data_Struct.set_name_flag=0;
				break;
			}
				
		}
	}
	else
	{
		Ble_Data_Struct.set_name_flag=0;
	}
	if(Ble_Data_Struct.set_name_flag)
	{
		memcpy(Ble_Data_Struct.adv_name_set,set_name_buffer,USER_NAME_MAX);
	}
	APP_Data_Res_call_fun(DATA_TYPE_BLE_SET);
	debug_uart_printf("data_type_ble_set_req_handle\r\n");
	
	//暂时不生效调试
	
}
/*******************************************************
* Function name : data_type_shot_mode_select_req_handle
* Description   : 击球模式设置
* Parameter     : void
* Return        : void
********************************************************/
void data_type_shot_mode_select_req_handle(void)
{	
	//数据上报APP
	Ble_Data_Struct.set_mode_flag=PlayTennis_ModeControl_DataSet(APP_BLE_REQ_DATA.data_mode,APP_BLE_REQ_DATA.data,APP_BLE_REQ_DATA.data_len);
	
	APP_Data_Res_call_fun(DATA_TYPE_SHOT_MODE_SELECT);
	
	debug_uart_printf("data_type_shot_mode_select_req_handle\r\n");
}
/*******************************************************
* Function name : data_type_shot_set_req_handle
* Description   : 击球参数设置
* Parameter     : void
* Return        : void
********************************************************/
void data_type_shot_set_req_handle(void)
{
	uint8_t tmp_data=0;
	if(APP_BLE_REQ_DATA.data_len==3)
	{
		if(BallData_Set(APP_BLE_REQ_DATA.data[0],APP_BLE_REQ_DATA.data[1],APP_BLE_REQ_DATA.data[2]))
		{
			Ble_Data_Struct.set_shot_flag=1;
		}
		else{
			Ble_Data_Struct.set_shot_flag=0;
		}
	}
	else
	{
		Ble_Data_Struct.set_shot_flag=0;	
	}

	APP_Data_Res_call_fun(DATA_TYPE_SHOT_SET);
	debug_uart_printf("data_type_shot_set_req_handle\r\n");
}
/*******************************************************
* Function name : data_type_app_ble_conn_req_handle
* Description   : 连接建立 权限鉴别
* Parameter     : void
* Return        : void
********************************************************/
void data_type_app_ble_conn_req_handle(void)
{
	if(memcmp(APP_BLE_REQ_DATA.data,"YES",3))
	{
		ble_status_set(BLE_CONN_NOTAUTH);
	}
	else
	{
		ble_status_set(BLE_CONN_AUTHOK);
	}
	 debug_uart_printf("data_type_app_ble_conn_req_handle\r\n");
	
}
/*******************************************************
* Function name : data_type_switch_req_handle
* Description   : 开关
* Parameter     : void
* Return        : void
********************************************************/
void data_type_switch_req_handle(void)
{
	uint8_t tmp_data=0;
	if(APP_BLE_REQ_DATA.data_len==1)
	{
		if(APP_BLE_REQ_DATA.data[0]==0x5A || APP_BLE_REQ_DATA.data[0]==0xA5)
		{
				if(SwitchSet(tmp_data))
				{
								Ble_Data_Struct.set_switch_flag=1;
				}
				else{
							Ble_Data_Struct.set_switch_flag=0;
				}

		}
		else
		{
			Ble_Data_Struct.set_switch_flag=0;	
		}
	}
	else
	{
		Ble_Data_Struct.set_switch_flag=0;	
	}

	APP_Data_Res_call_fun(DATA_TYPE_SWITCH);
	 debug_uart_printf("data_type_switch_req_handle\r\n");
}


uint8_t APP_Data_Req_call_fun(uint8_t APP_Data_Req_Idx)
{
	uint8_t return_data=0;
	if(APP_Data_Req_Idx>=DATA_TYPE_BLE_SET && APP_Data_Req_Idx<DATA_TYPE_END)
	{
		for(uint8_t i=0;i<DATA_TYPE_END-1;i++)
		{
			if(APP_Data_Req_Idx==APP_Data_Req_Func_List[i].at_cmd)
			{
				APP_Data_Req_Func_List[i].exe();
				return_data=1;
				return return_data;
			}
		}
	}
	return return_data;
}

/*******************************************************
* Function name : data_type_ble_set_res_handle
* Description   : ble广播参数设置res
* Parameter     : void
* Return        : void
********************************************************/
void data_type_ble_set_res_handle(void)
{
	uint8_t set_res_name_buffer[20]={0};
		memset(&BLE_APP_RES_DATA.data_type,0,sizeof(BLE_APP_DataFrame));
		BLE_APP_RES_DATA.data_type=DATA_TYPE_BLE_SET;
	
		BLE_APP_RES_DATA.data_mode=DATA_MODE_DEFAULT;
		BLE_APP_RES_DATA.data_direction=DATA_DIRECTION_APP_DEVICE_RES;
		
	if(Ble_Data_Struct.set_name_flag)
	{
		Ble_Data_Struct.set_name_flag=0;
		BLE_APP_RES_DATA.data_len = USER_NAME_MAX;
		memcpy(BLE_APP_RES_DATA.data,Ble_Data_Struct.adv_name_set,USER_NAME_MAX);
	}
	else
	{
			BLE_APP_RES_DATA.data_len = 5;
		memcpy(BLE_APP_RES_DATA.data,"ERROR",5);
	}
	memcpy(set_res_name_buffer,&BLE_APP_RES_DATA.data_type,sizeof(BLE_APP_DataFrame)-1);
	BLE_APP_RES_DATA.crc = CheckSum_Count_Get(set_res_name_buffer,sizeof(BLE_APP_DataFrame)-1);
	myMcuToBleSendData(&BLE_APP_RES_DATA.data_type,sizeof(BLE_APP_DataFrame));
	
	debug_uart_printf("data_type_ble_set_res_handle\r\n");
}
/*******************************************************
* Function name : data_type_shot_mode_select_res_handle
* Description   : 击球模式设置res
* Parameter     : void
* Return        : void
********************************************************/
void data_type_shot_mode_select_res_handle(void)
{
		uint8_t set_res_mode_buffer[20]={0};
		memset(&BLE_APP_RES_DATA.data_type,0,sizeof(BLE_APP_DataFrame));
		BLE_APP_RES_DATA.data_type=DATA_TYPE_BLE_SET;
	
		BLE_APP_RES_DATA.data_mode=APP_BLE_REQ_DATA.data_type;
		BLE_APP_RES_DATA.data_direction=DATA_DIRECTION_APP_DEVICE_RES;
		
	if(Ble_Data_Struct.set_mode_flag)
	{
		Ble_Data_Struct.set_mode_flag=0;
		BLE_APP_RES_DATA.data_len = APP_BLE_REQ_DATA.data_len;
		memcpy(BLE_APP_RES_DATA.data,APP_BLE_REQ_DATA.data,BLE_APP_RES_DATA.data_len);
	}
	else
	{
			BLE_APP_RES_DATA.data_len = 5;
		memcpy(BLE_APP_RES_DATA.data,"ERROR",5);
	}
	memcpy(set_res_mode_buffer,&BLE_APP_RES_DATA.data_type,sizeof(BLE_APP_DataFrame)-1);
	BLE_APP_RES_DATA.crc = CheckSum_Count_Get(set_res_mode_buffer,sizeof(BLE_APP_DataFrame)-1);
	myMcuToBleSendData(&BLE_APP_RES_DATA.data_type,sizeof(BLE_APP_DataFrame));
	
	debug_uart_printf("data_type_shot_mode_select_res_handle\r\n");
}
/*******************************************************
* Function name : data_type_shot_set_res_handle
* Description   : 击球参数设置res
* Parameter     : void
* Return        : void
********************************************************/
void data_type_shot_set_res_handle(void)
{
		uint8_t set_shotres_mode_buffer[20]={0};
		memset(&BLE_APP_RES_DATA.data_type,0,sizeof(BLE_APP_DataFrame));
		BLE_APP_RES_DATA.data_type=DATA_TYPE_SHOT_SET;
	
		BLE_APP_RES_DATA.data_mode=APP_BLE_REQ_DATA.data_type;
		BLE_APP_RES_DATA.data_direction=DATA_DIRECTION_APP_DEVICE_RES;
	if(Ble_Data_Struct.set_shot_flag)
	{
		Ble_Data_Struct.set_shot_flag=0;
		BLE_APP_RES_DATA.data_len = APP_BLE_REQ_DATA.data_len;
		memcpy(BLE_APP_RES_DATA.data,APP_BLE_REQ_DATA.data,BLE_APP_RES_DATA.data_len);
	}
	else
	{
		BLE_APP_RES_DATA.data_len = 5;
		memcpy(BLE_APP_RES_DATA.data,"ERROR",5);
	}	
	memcpy(set_shotres_mode_buffer,&BLE_APP_RES_DATA.data_type,sizeof(BLE_APP_DataFrame)-1);
	BLE_APP_RES_DATA.crc = CheckSum_Count_Get(set_shotres_mode_buffer,sizeof(BLE_APP_DataFrame)-1);
	myMcuToBleSendData(&BLE_APP_RES_DATA.data_type,sizeof(BLE_APP_DataFrame));
	debug_uart_printf("data_type_shot_set_res_handle\r\n");
}
/*******************************************************
* Function name : data_type_app_ble_conn_res_handle
* Description   : 连接建立 权限鉴别请求
* Parameter     : void
* Return        : void
********************************************************/
void data_type_app_ble_conn_res_handle(void)
{
	
		uint8_t connect_buffer[20]={0};
		memset(&BLE_APP_RES_DATA.data_type,0,sizeof(BLE_APP_DataFrame));
		
		BLE_APP_RES_DATA.data_type=DATA_TYPE_APP_BLE_CONN;
	
		BLE_APP_RES_DATA.data_mode=DATA_MODE_DEFAULT;
		
		BLE_APP_RES_DATA.data_direction=DATA_DIRECTION_DEVICE_APP_REQ;
//		
		BLE_APP_RES_DATA.data_len =13;
		memcpy(BLE_APP_RES_DATA.data,"HUGEZUISHUAI?",13);
		
		//	debug_uart_printf("power =%f %d %s\r\n",power_report_data,BLE_APP_RES_DATA.data_len,&BLE_APP_RES_DATA.data[0]);
	
	memcpy(connect_buffer,&BLE_APP_RES_DATA.data_type,sizeof(BLE_APP_DataFrame)-1);
	BLE_APP_RES_DATA.crc = CheckSum_Count_Get(connect_buffer,sizeof(BLE_APP_DataFrame)-1);
		myMcuToBleSendData(&BLE_APP_RES_DATA.data_type,sizeof(BLE_APP_DataFrame));
	
	
	 debug_uart_printf("data_type_app_ble_conn_res_handle\r\n");
		
}
/*******************************************************
* Function name : data_type_switch_res_handle
* Description   : 开关res
* Parameter     : void
* Return        : void
********************************************************/
void data_type_switch_res_handle(void)
{
		uint8_t set_switchres_mode_buffer[20]={0};
		memset(&BLE_APP_RES_DATA.data_type,0,sizeof(BLE_APP_DataFrame));
		BLE_APP_RES_DATA.data_type=DATA_TYPE_SWITCH;
	
		BLE_APP_RES_DATA.data_mode=APP_BLE_REQ_DATA.data_type;
		BLE_APP_RES_DATA.data_direction=DATA_DIRECTION_APP_DEVICE_RES;
	
	if(Ble_Data_Struct.set_switch_flag)
	{
		Ble_Data_Struct.set_switch_flag=0;
		BLE_APP_RES_DATA.data_len = APP_BLE_REQ_DATA.data_len;	
		memcpy(BLE_APP_RES_DATA.data,APP_BLE_REQ_DATA.data,BLE_APP_RES_DATA.data_len);
	}
	else
	{
					BLE_APP_RES_DATA.data_len = 5;
		memcpy(BLE_APP_RES_DATA.data,"ERROR",5);
	}
	
		memcpy(set_switchres_mode_buffer,&BLE_APP_RES_DATA.data_type,sizeof(BLE_APP_DataFrame)-1);
	BLE_APP_RES_DATA.crc = CheckSum_Count_Get(set_switchres_mode_buffer,sizeof(BLE_APP_DataFrame)-1);
	myMcuToBleSendData(&BLE_APP_RES_DATA.data_type,sizeof(BLE_APP_DataFrame));
	 debug_uart_printf("data_type_switch_res_handle\r\n");
}

uint8_t float_to_str(float num,uint8_t *change_buffer)
{
	uint32_t rmp_date=(num*10),return_len=0;
	uint8_t str_buffer[DATA_MAX_LEN]={0};
	int j=0;
	while(rmp_date)
	{
		str_buffer[return_len++]=((rmp_date%10)+'0');
		rmp_date=rmp_date/10;
	}
	for(int i =return_len-1;i>=1;i--)
	{
		change_buffer[j++]=str_buffer[i];
	}
	change_buffer[j++]=0x2E;//'.'
	change_buffer[j++]=str_buffer[0];//'.'
	return return_len+1;
}
/*******************************************************
* Function name : data_type_power_report_res_handle
* Description   : 电池电量上报
* Parameter     : void
* Return        : void
********************************************************/
void data_type_power_report_res_handle(void)
{
	if(ManuFlag_Get()||ble_status_get()==BLE_CONN_NOTAUTH)//产测模式
	{
		//NO ACTIVE
	}
	else{
	uint8_t crc_buffer[20]={0};
	
		memset(&BLE_APP_RES_DATA.data_type,0,sizeof(BLE_APP_DataFrame));
		BLE_APP_RES_DATA.data_type=DATA_TYPE_POWER_REPORT;
	
		BLE_APP_RES_DATA.data_mode=DATA_MODE_DEFAULT;
		BLE_APP_RES_DATA.data_direction=DATA_DIRECTION_DEVICE_APP_REQ;
//		
		BLE_APP_RES_DATA.data_len = float_to_str(power_report_data,BLE_APP_RES_DATA.data);
		//	debug_uart_printf("power =%f %d %s\r\n",power_report_data,BLE_APP_RES_DATA.data_len,&BLE_APP_RES_DATA.data[0]);
	
	memcpy(crc_buffer,&BLE_APP_RES_DATA.data_type,sizeof(BLE_APP_DataFrame)-1);
	BLE_APP_RES_DATA.crc = CheckSum_Count_Get(crc_buffer,sizeof(BLE_APP_DataFrame)-1);
		myMcuToBleSendData(&BLE_APP_RES_DATA.data_type,sizeof(BLE_APP_DataFrame));
	}
	//debug_uart_printf("data_type_power_report_res_handle\r\n");
}
/*******************************************************
* Function name : data_type_ball_error_res_handle
* Description   : 缺球异常
* Parameter     : void
* Return        : void
********************************************************/
void data_type_ball_error_res_handle(void)
{
	uint8_t set_switchres_mode_buffer[20]={0};
	memset(&BLE_APP_RES_DATA.data_type,0,sizeof(BLE_APP_DataFrame));
	BLE_APP_RES_DATA.data_type=DATA_TYPE_BALL_ERROR;
	
	BLE_APP_RES_DATA.data_mode=DATA_MODE_DEFAULT;
	BLE_APP_RES_DATA.data_direction=DATA_DIRECTION_DEVICE_APP_REQ;
	
		BLE_APP_RES_DATA.data_len =6;
		memcpy(BLE_APP_RES_DATA.data,"BALLNO",6);
	
		memcpy(set_switchres_mode_buffer,&BLE_APP_RES_DATA.data_type,sizeof(BLE_APP_DataFrame)-1);
	BLE_APP_RES_DATA.crc = CheckSum_Count_Get(set_switchres_mode_buffer,sizeof(BLE_APP_DataFrame)-1);
	myMcuToBleSendData(&BLE_APP_RES_DATA.data_type,sizeof(BLE_APP_DataFrame));
	 debug_uart_printf("data_type_ball_error_res_handle\r\n");
}
uint8_t APP_Data_Res_call_fun(uint8_t APP_Data_Req_Idx)
{
	uint8_t return_data=0;
	if(APP_Data_Req_Idx>=DATA_TYPE_BLE_SET && APP_Data_Req_Idx<DATA_TYPE_END)
	{
		for(uint8_t i=0;i<DATA_TYPE_END-1;i++)
		{
			if(APP_Data_Req_Idx==APP_Data_Res_Func_List[i].at_cmd)
			{
				APP_Data_Res_Func_List[i].exe();
				return_data=1;
				return return_data;
			}
		}
	}
	return return_data;
}

void adv_data_set(uint8_t *data,uint32_t len)
{
	uint8_t tmp=0;
	if(len!=USER_NAME_MAX)
	{
			return ;
	}
	for(uint32_t i=0;i<len;i++)
	{
		if(data[i]<48 ||data[i]>57)
		{
			return ;
		}
	}
	FLASH_EEPROM_Write_test(data,len,Flash_User_Name_ID);
}

uint8_t num_change_str(uint8_t *data,uint16_t mtu)
{
	uint8_t mtu_len=0;
	uint8_t tmp[5];
	uint8_t i=0;
	uint8_t flag=0;
	while(mtu/10)
	{
		tmp[mtu_len]=(mtu%10)+48;
		mtu_len++;
		mtu/=10;
	}
	tmp[mtu_len]=mtu%10+48;
		mtu_len++;
	i=mtu_len-1;
	while(i)
	{
		data[flag]=tmp[i];
		flag++;
		i--;
	}
	data[flag]=tmp[i];
//	log_debug("num_change_str = %s\r\n",data);
	return mtu_len;
}



/*******************************************************
* Function name : str_tran_u16
* Description   : 字符串转数字 例：0xFF
* Parameter     : data 参数字符串
                  len 参数字符串长度
* Return        : 返回数字
********************************************************/
uint16_t str_tran_u16(uint8_t *data,uint8_t len)
{
	uint16_t return_data=0;
	uint8_t i=0;
	uint8_t tmp[3]="0x";
	if(!memcmp(data,tmp,2))
	{
		for(i=2;i<len;i++)
		{
			return_data|=((data[i]-(data[i]>65?55:48))<<((len-i-1)*4));
		}
	}
	return return_data;
}

/*******************************************************
* Function name : str_tran_u32
* Description   : 字符串转数字
* Parameter     : data 参数字符串
                  len 参数字符串长度
* Return        : 返回数字
********************************************************/
uint32_t str_tran_u32(uint8_t *data,uint8_t len)
{
	uint32_t return_data=0;
	uint8_t i=0;
	for(i=0;i<len;i++)
	{
		return_data*=10;
		return_data+=(data[i]-48);
	}
	return return_data;
}
/*******************************************************
* Function name : ble_status_get
* Description   : 获取蓝牙状态
* Parameter     : void
* Return        : uint8_t @Ble_Status_List
********************************************************/
uint8_t ble_status_get(void)
{
	return Ble_Data_Struct.ble_status;
}
/*******************************************************
* Function name : ble_status_set
* Description   : 设置蓝牙状态
* Parameter     : status 状态@Ble_Status_List
* Return        : bool 状态设置是否成功 true:成功  false：失败
********************************************************/
static bool ble_status_set(uint8_t status)
{
	if(status>=BLE_STATE && status<BLE_STATUS_END)
	{
		Ble_Data_Struct.ble_status=status;
		return true;
	}
	return false;
}
/*******************************************************
* Function name : ble_data_init
* Description   : 蓝牙参数初始化
* Parameter     : void
* Return        : void
********************************************************/
void ble_data_init(void)
{
	adv_data_get();
}

/*********************蓝牙状态机维护START************************/
void ble_task_status_set(uint8_t status)
{
	if(status>=BLE_STATUS_START && status<=BLE_STATUS_END)
	{
		ble_task_status=status;
	}
}
uint8_t ble_task_status_get(void)
{
	return ble_task_status;
}
//uint8_t RRTR=0xff;
void ble_task_handle(void)
{
//	if(ble_task_status!=RRTR)
//	{
//		RRTR=ble_task_status;
//		debug_uart_printf("ble_task_status = %d\r\n",ble_task_status);
//	}
	switch(ble_task_status)
	{
		case BLE_STATUS_START:
			{
				myBleAT_L();
				ble_task_status_set(BLE_SET_ADV_NAME_REQ);	
			}
		break;
		case BLE_SET_ADV_NAME_REQ:
			{
				
					ble_task_status_set(BLE_SET_ADV_NAME_RES);
				ble_at_cmd_call_fun(AT_SETNAME);
			
			}
			break;
		case BLE_SET_ADV_NAME_RES:
			break;
		case BLE_SET_ADV_ADVI_REQ:
			{
				Ble_Data_Struct.at_send_interval_count++;
				if(Ble_Data_Struct.at_send_interval_count>=100)
				{
					Ble_Data_Struct.at_send_interval_count=0;
					ble_at_cmd_call_fun(AT_ADVI);
					ble_task_status_set(BLE_SET_ADV_ADVI_RES);
					debug_uart_printf("BLE_SET_ADV_ADVI_RES\r\n");
				}
			}
			break;
		case BLE_SET_ADV_ADVI_RES:
			break;
		case BLE_TASK_APP_RUN:
			break;	
		case BLE_TASK_CONN_START:
			break;	
		case BLE_TASK_CONN_OK:
			break;	
		case BLE_TASK_CONN_ERROR:
			break;	
		default:
			break;
	}
}
/*********************蓝牙状态机维护END************************/

/*******************************************************
* Function name : at_cmd_search
* Description   : 查询对应指令
* Parameter     : str 匹配字符串
                  len 匹配字符串长度
* Return        : void
********************************************************/
uint16_t at_cmd_search(uint8_t *ptr, uint16_t len)
{
    uint16_t n = 0, index;
    int i;
    for(i = 0; at_cmd_func[i].at_cmd < AT_CMD_END; i++)
    {
			if(Ble_Data_Struct.at_set_tmp!=AT_CMD_END)
			{
				if(i!=Ble_Data_Struct.at_set_tmp)
					continue;
			}
        n = strlen(at_cmd_func[i].str);
			//debug_uart_printf(" i = %d n=%d len=%d %s %s\r\n ",i,n,len,ptr,at_cmd_func[i].str);
        //修改：n—>len，匹配输入的字符串及个数而非数据库中字符串及个数
        //修改：需要判断n和len是否相等
        if(n == len)
        {
            if(!memcmp(ptr, at_cmd_func[i].str, len))    //无符号的uint_t转换，比较
            {
                index = i;
                break;
            }
        }
			else if(len > n)
			{
				if(!memcmp(ptr, at_cmd_func[i].str, n)) 
				{
					if(at_cmd_func[i].str[n-1] == '='&& ptr[n-1]=='=')
					{
						index = i;
						break;
					}
				}
			}
			else
			{
				
			}
    }

    if(i >= AT_CMD_END)
    {
        index = AT_CMD_END;
    }
	
    return index;
}
/*******************************************************
* Function name : at_test
* Description   : AT指令错误
* Parameter     : void
* Return        : void
********************************************************/
static void at_error(void)
{
	uint8_t error_data[]="ERROR!\r\n";
  debug_uart_printf("AT_%s",error_data);
}
/*******************************************************
* Function name : at_test_cmd_analysis
* Description   : 解析AT指令
* Parameter     : str 匹配字符串
                  len 匹配字符串长度
* Return        : void
********************************************************/
void at_test_cmd_analysis(uint8_t *ptr,uint32_t len)
{
//	debug_uart_printf("at_cmd_index = %s %d\r\n",ptr,len);
	uint8_t at_cmd_index = AT_RECVER_END;
	bool error_flag=false;
	GO_AT_ERROR:
	if(error_flag)
	{
		at_error();
		return ;
	}
	if(ptr==NULL||len==0)
	{
		error_flag=true;
		goto GO_AT_ERROR;
	}
	if((ptr[0] == 'O') && (ptr[1] == 'K') &&(ptr[len-1])=='\n'&&(ptr[len-2])=='\r')  //识别指令格式
	{
		
		at_cmd_index = at_cmd_search(&ptr[2],len-4);
		//debug_uart_printf("at_cmd_index = %d\r\n",at_cmd_index);
		if(at_cmd_index>=0 && at_cmd_index<AT_CMD_END)
		{
			at_cmd_func[at_cmd_index].exe(&ptr[3],len-5);
		}
		else
		{
			error_flag=true;
			goto GO_AT_ERROR;
		}
	}
	else 
	{
		if((ptr[0] == '+')&&(ptr[len-1])=='\n'&&(ptr[len-2])=='\r')
		{
			uint8_t i=0;
			for(i = AT_BLE_CONN; at_cmd_func[i].at_cmd < AT_RECVER_END; i++)
			{
				if(len-2==strlen(at_cmd_func[i].str))
				{
					if(!memcmp(at_cmd_func[i].str,ptr,len-2))
					{
						at_cmd_func[i].exe(NULL,NULL);
						return;
					}
					else{
					}
				}
			}
		}
	}
	Ble_Data_Struct.at_set_tmp=AT_REQ_END;
}


/*******************************************************
* Function name : ble_at_cmd_call_fun
* Description   : 蓝牙模块的指令配置
* Parameter     : at_cmd_index 发送指令REQ的ID
* Return        : void
********************************************************/
void ble_at_cmd_call_fun(uint8_t at_cmd_index)
{
	switch(at_cmd_index)
	{
		case AT_TEST :
		//	AT_Send_ID[at_cmd_index].exe(NULL,NULL);
		//	break;
		case AT_SETNAME :
		//	break;
		case AT_RESET :
		//	break;
		case AT_GETCON :
		//	break;
		case AT_ADVI :
			if(Ble_Data_Struct.at_set_tmp==AT_REQ_END)
			{
				Ble_Data_Struct.at_set_tmp=at_cmd_index;
				AT_Send_ID[at_cmd_index].exe(NULL,NULL);
				//Ble_Data_Struct.at_req_flag=1;
			}
			break;
			
			
		default: 
				break;										
	}
	
}


/*
AT指令选择引脚(默认上拉状态)：
已连接：
低电平(AT指令模式)
高电平(透传模式)
未连接：
高电平或低电平均为AT指令模式
*/
/*
*********************************************************************************************************
* 函 数 名: myBleAT_H
* 功能说明: AT指令引脚拉高
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myBleAT_H(void)
{
    HAL_GPIO_WritePin(AT_EN_GPIO_Port, AT_EN_Pin, GPIO_PIN_SET);
}
/*
*********************************************************************************************************
* 函 数 名: myBleAT_L
* 功能说明: AT指令引脚拉低
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myBleAT_L(void)
{
    HAL_GPIO_WritePin(AT_EN_GPIO_Port, AT_EN_Pin, GPIO_PIN_RESET);
}

/*
蓝牙连接状态输出引脚：
已连接（高电平）
未连接（低电平）
*/
/*
*********************************************************************************************************
* 函 数 名: myBleStatusRead
* 功能说明: 蓝牙连接状态检测
* 形  参:   无
* 返 回 值: High--已连接；Low--未连接
*********************************************************************************************************
*/
GPIO_PinState myBleStatusRead(void)
{
    return HAL_GPIO_ReadPin(BLE_STATUS_GPIO_Port, BLE_STATUS_Pin);
}

/*
*********************************************************************************************************
* 函 数 名: myMcuToBleSendData
* 功能说明: MCU向蓝牙模块发送数据
* 形  参:   tx_buf:发送的数据
            len:  发送数据长度
* 返 回 值: 无
*********************************************************************************************************
*/
void myMcuToBleSendData(uint8_t *tx_buf, uint8_t len)
{
    uartSendToBle(tx_buf, len);
}
/*
*********************************************************************************************************
* 函 数 名: myMcuToBleSendATCommand
* 功能说明: mcu向主机发送AT指令
* 形  参:   tx_buf:发送的数据
            len:  发送数据长度
* 返 回 值: 无
*********************************************************************************************************
*/
void mcuToBleSendATCommand(uint8_t *tx_buf, uint8_t len)
{
    uartSendToBle(tx_buf, len);
}
/******************at_cmd_req_handle_fun_list*********************************/
/*******************************************************
* Function name : at_test_req_handle
* Description   : AT指令测试
* Parameter     : str 参数字符串
                  len 参数字符串长度
* Return        : void
********************************************************/
static void at_test_req_handle(uint8_t *str, uint16_t len)
{	
	debug_uart_printf("at_test_req_handle\r\n");
		uint16_t send_data_len=strlen(AT_Send_ID[AT_TEST].str);
		uint8_t uart_send_buffer[10]={0};
		memcpy(uart_send_buffer,AT_Send_ID[AT_TEST].str,send_data_len);
		uart_send_buffer[send_data_len++]=0x0D;
		uart_send_buffer[send_data_len++]=0x0A;
	
//debug_uart_printf("%s %d\r\n",uart_send_buffer,send_data_len);
	mcuToBleSendATCommand(uart_send_buffer,send_data_len);
}
/*******************************************************
* Function name : at_setname_req_handle
* Description   : 设置name
* Parameter     : str 参数字符串
                  len 参数字符串长度
* Return        : void
********************************************************/
static void at_setname_req_handle(uint8_t *str, uint16_t len)
{
	uint8_t name_data[13]={"QFFFFFFFFFFF"};
			debug_uart_printf("at_setname_req_handle\r\n");
		uint16_t send_data_len=strlen(AT_Send_ID[AT_SETNAME].str);
		uint8_t uart_send_buffer[50]={0};
		memcpy(uart_send_buffer,AT_Send_ID[AT_SETNAME].str,send_data_len);
//			debug_uart_printf("222 = %s %s\r\n",name_data,Ble_Data_Struct.adv_name_set);
		memcpy(&name_data[1],Ble_Data_Struct.adv_name_set,USER_NAME_MAX);
//		debug_uart_printf("333 = %s\r\n",name_data);
	memcpy(&uart_send_buffer[send_data_len],name_data,sizeof(name_data)-1);
		send_data_len+=(USER_NAME_MAX+1);
		uart_send_buffer[send_data_len++]=0x0D;
		uart_send_buffer[send_data_len++]=0x0A;
	mcuToBleSendATCommand(uart_send_buffer,send_data_len);
}
/*******************************************************
* Function name : at_reset_req_handle
* Description   : 重启ble设备
* Parameter     : str 参数字符串
                  len 参数字符串长度
* Return        : void
********************************************************/
static void at_reset_req_handle(uint8_t *str, uint16_t len)
{
			debug_uart_printf("at_reset_req_handle\r\n");
	
		uint16_t send_data_len=strlen(AT_Send_ID[AT_RESET].str);
		uint8_t uart_send_buffer[50]={0};
		memcpy(uart_send_buffer,AT_Send_ID[AT_RESET].str,send_data_len);
		uart_send_buffer[send_data_len++]=0x0D;
		uart_send_buffer[send_data_len++]=0x0A;
	
	
	mcuToBleSendATCommand(uart_send_buffer,send_data_len);
}
/*******************************************************
* Function name : at_getcon_req_handle
* Description   : 获取连接状态
* Parameter     : str 参数字符串
                  len 参数字符串长度
* Return        : void
********************************************************/
static void at_getcon_req_handle(uint8_t *str, uint16_t len)
{
//			debug_uart_printf("at_getcon_req_handle\r\n");
		uint16_t send_data_len=strlen(AT_Send_ID[AT_GETCON].str);
		uint8_t uart_send_buffer[50]={0};
		memcpy(uart_send_buffer,AT_Send_ID[AT_GETCON].str,send_data_len);
		uart_send_buffer[send_data_len++]=0x0D;
		uart_send_buffer[send_data_len++]=0x0A;
	
	//str_tran_u32();
		
		
	mcuToBleSendATCommand(uart_send_buffer,send_data_len);
}
/*******************************************************
* Function name : at_advi_req_handle
* Description   : 设置广播间隔参数
* Parameter     : str 参数字符串
                  len 参数字符串长度
* Return        : void
********************************************************/
static void at_advi_req_handle(uint8_t *str, uint16_t len)
{
	uint8_t advi_data[4]={0};
	uint16_t advi_tmp=0;
	
		debug_uart_printf("at_advi_req_handle\r\n");
	advi_tmp= num_change_str(advi_data,Ble_Data_Struct.adv_disc);
	
	uint16_t send_data_len=strlen(AT_Send_ID[AT_ADVI].str);
	uint8_t uart_send_buffer[50]={0};
	memcpy(uart_send_buffer,AT_Send_ID[AT_ADVI].str,send_data_len);
	memcpy(&uart_send_buffer[send_data_len],advi_data,advi_tmp);
	send_data_len+=advi_tmp;
	uart_send_buffer[send_data_len++]=0x0D;
	uart_send_buffer[send_data_len++]=0x0A;
	
	debug_uart_printf("\r\n%s %d\r\n",uart_send_buffer,send_data_len);
	mcuToBleSendATCommand(uart_send_buffer,send_data_len);
	
	
}

/****************************************at_cmd_req_handle_fun_list_end*************************/
/******************at_cmd_res_handle_fun_list*********************************/
/*******************************************************
* Function name : at_test_res_handle
* Description   : 设置test指令response
* Parameter     : str 参数字符串
                  len 参数字符串长度
* Return        : void
********************************************************/
static void at_test_res_handle(uint8_t *str, uint16_t len)
{
	debug_uart_printf("at_test_res_handle\r\n");
}

/*******************************************************
* Function name : at_setname_res_handle
* Description   : 设置广播name指令response
* Parameter     : str 参数字符串
                  len 参数字符串长度
* Return        : void
********************************************************/
static void at_setname_res_handle(uint8_t *str, uint16_t len)
{
		debug_uart_printf("at_setname_res_handle\r\n");
	
	uint8_t status_tmp = ble_task_status_get();
	if(status_tmp==BLE_TASK_CONN_CHANGE_NAME_RES)
	{
		ble_task_status_set(BLE_TASK_APP_RUN);
	}
	else if(status_tmp==BLE_SET_ADV_NAME_RES)
	{
		ble_task_status_set(BLE_SET_ADV_ADVI_REQ);
	}
	else{
	
	}
}
/*******************************************************
* Function name : at_reset_res_handle
* Description   : 设置重启指令response
* Parameter     : str 参数字符串
                  len 参数字符串长度
* Return        : void
********************************************************/
static void at_reset_res_handle(uint8_t *str, uint16_t len)
{
		debug_uart_printf("at_reset_res_handle\r\n");
}
/*******************************************************
* Function name : at_getcon_res_handle
* Description   : 获取蓝牙连接状态指令response
* Parameter     : str 参数字符串
                  len 参数字符串长度
* Return        : void
********************************************************/
static void at_getcon_res_handle(uint8_t *str, uint16_t len)
{
//		debug_uart_printf("at_getcon_res_handle\r\n");
	//ble_status_set();
}
/*******************************************************
* Function name : at_advi_res_handle
* Description   : 设置广播间隔参数response
* Parameter     : str 参数字符串
                  len 参数字符串长度
* Return        : void
********************************************************/
static void at_advi_res_handle(uint8_t *str, uint16_t len)
{
		debug_uart_printf("at_advi_res_handle\r\n");
	ble_task_status_set(BLE_TASK_APP_RUN);
	myBleAT_H();
}
/*******************************************************
* Function name : at_ble_conn_res_handle
* Description   : 连接主动上报
* Parameter     : str 参数字符串
                  len 参数字符串长度
* Return        : void
********************************************************/
static void at_ble_conn_res_handle(uint8_t *str, uint16_t len)
{
	ble_status_set(BLE_CONN_NOTAUTH);
	
	

//HAL_Delay(1000);
//		mcuToBleSendATCommand("AT+DISC\r\n",9);
//	BLE_APP_RES_DATA.data_len = float_to_str(24.3,&BLE_APP_RES_DATA.data[0]);
//			debug_uart_printf("power =%f %d %s\r\n",power_report_data,BLE_APP_RES_DATA.data_len,BLE_APP_RES_DATA.data);
	debug_uart_printf("at_ble_conn_res_handle\r\n");
	
}

/*******************************************************
* Function name : at_ble_disconn_res_handle
* Description   : 断开连接主动上报
* Parameter     : str 参数字符串
                  len 参数字符串长度
* Return        : void
********************************************************/
static void at_ble_disconn_res_handle(uint8_t *str, uint16_t len)
{
	//状态机重置
	ble_task_status_set(BLE_TASK_APP_RUN);
	//鉴权任务重置
	Ble_Data_Struct.app_ble_conn_ok=0;
	ble_status_set(BLE_STATE);
	//end
	debug_uart_printf("at_ble_disconn_res_handle\r\n");
}

/****************************************at_cmd_res_handle_fun_list_end*************************/



void ble_report_error(void)
{
	uint8_t report_error_data[]="ERROR!\r\n";
	if(Ble_Data_Struct.ble_status)
	{
		mcuToBleSendATCommand(report_error_data,8);
	}
}

void recver_data_check(uint8_t *data,uint8_t len)
{
	uint8_t  data_check_tmp=0;
	uint8_t return_error_tmp=0;
	uint8_t tmp_buffer[sizeof(BLE_APP_DataFrame)]={0},tmp_len=len;
	memcpy(tmp_buffer,data,len);
	
	RECVER_DATA_CHECK_ERROR:
	if(return_error_tmp)
	{
	//	debug_uart_printf("CRC = %d %d %x %x\r\n",return_error_tmp,len-1,APP_BLE_REQ_DATA.crc,data[len-1]);
		memset(&APP_BLE_REQ_DATA.data_type,0,sizeof(BLE_APP_DataFrame));
		ble_report_error();
	
		return ;
	}
	if(len==20)
	{
		APP_BLE_REQ_DATA.crc = CheckSum_Count_Get(tmp_buffer,tmp_len-1);
		if(data[len-1]==APP_BLE_REQ_DATA.crc)
		{
			
			APP_BLE_REQ_DATA.data_type=data[data_check_tmp++];
			if(APP_BLE_REQ_DATA.data_type<DATA_TYPE_BLE_SET || APP_BLE_REQ_DATA.data_type>DATA_TYPE_SWITCH)
			{
				return_error_tmp=3;
				goto RECVER_DATA_CHECK_ERROR;
			}
			APP_BLE_REQ_DATA.data_mode=data[data_check_tmp++];
			if(APP_BLE_REQ_DATA.data_mode<DATA_MODE_FLAT || APP_BLE_REQ_DATA.data_mode>DATA_MODE_SWITCH)
			{
				if(APP_BLE_REQ_DATA.data_mode!=DATA_MODE_DEFAULT)
				{
					return_error_tmp=4;
					goto RECVER_DATA_CHECK_ERROR;
				}
			}
			APP_BLE_REQ_DATA.data_direction=data[data_check_tmp++];
			if(APP_BLE_REQ_DATA.data_direction<DATA_DIRECTION_APP_DEVICE_REQ || APP_BLE_REQ_DATA.data_direction>DATA_DIRECTION_DEVICE_APP_RES)
			{
				return_error_tmp=5;
				goto RECVER_DATA_CHECK_ERROR;
			}			
			
			APP_BLE_REQ_DATA.data_len=data[data_check_tmp++];
			if(APP_BLE_REQ_DATA.data_len<=0||APP_BLE_REQ_DATA.data_len>DATA_MAX_LEN)
			{
				return_error_tmp=6;
				goto RECVER_DATA_CHECK_ERROR;
			}
			memcpy(APP_BLE_REQ_DATA.data,&data[data_check_tmp],APP_BLE_REQ_DATA.data_len);
			
			APP_Data_Req_call_fun(APP_BLE_REQ_DATA.data_type);//回调
			
		}
		else
		{
			return_error_tmp=1;
			goto RECVER_DATA_CHECK_ERROR;
		}
	}
	else
	{
		return_error_tmp=2;
		goto RECVER_DATA_CHECK_ERROR;
	}
}	
static uint32_t connect_timer_count=0;
void ble_status_task_handle(void)
{
	switch(ble_status_get())
	{
		case BLE_STATE:
		{
		
		}break;
		case BLE_CONN_NOTAUTH:
		{
			connect_timer_count++;
			if(connect_timer_count>=1000)
			{
				connect_timer_count=0;
				ble_status_set(BLE_STATE);
				uint8_t send_data[]="AT+DISC\r\n";
				mcuToBleSendATCommand(send_data,9);
			}
			else if(connect_timer_count==100)			
			{
					APP_Data_Res_call_fun(DATA_TYPE_APP_BLE_CONN);
				
			}
			else{
			}
		}break;
		case BLE_CONN_AUTHOK:
		{
			if(connect_timer_count)
			{
				connect_timer_count=0;
			}
			Ble_Data_Struct.app_ble_conn_ok=1;
		}break;
		default:
			break;
	}
	
}
/*
*********************************************************************************************************
* 函 数 名: uartReceiveDataFromBle
* 功能说明: 串口接收来自ble的数据
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void uartReceiveDataFromBle(void)
{
	
    static uint32_t SysCnt = 0;
    uint8_t temp = 0;
    temp = api_GetSysTickCount20ms();
	
    if (SysCnt != temp)
    {
        SysCnt = temp;
			
			if(!Ble_Data_Struct.app_ble_conn_ok&&ManuFlag_Get()==false)
			{
			//	ble_status_task_handle();//鉴权任务
			}
			
        if (true == bleUart.g_ble_rec_filish_flag) // 串口数据接收完成
        {
						
					for(int i=0;i<bleUart.g_ble_uart_rx_len;i++)
					{
						debug_uart_printf(" %x",bleUart.ble_uart_rx_buf[i]);
					}
					debug_uart_printf("%d %d\r\n",bleUart.g_ble_uart_rx_len,ble_task_status_get());
            // 数据解析
            // motorControlDataResolve(bleUart.ble_uart_rx_buf, bleUart.g_ble_uart_rx_len); // 推杆控制器数据解析
				
					if(ble_task_status_get()!=BLE_TASK_CONN_ERROR)//不是连接错误
					{
						if(bleUart.ble_uart_rx_buf[bleUart.g_ble_uart_rx_len-2]=='\r' && bleUart.ble_uart_rx_buf[bleUart.g_ble_uart_rx_len-1]=='\n'\
						&&(bleUart.ble_uart_rx_buf[0]=='+'||bleUart.ble_uart_rx_buf[0]=='O'))
						{
							at_test_cmd_analysis(bleUart.ble_uart_rx_buf,bleUart.g_ble_uart_rx_len);
						}
						else
						{
							recver_data_check(bleUart.ble_uart_rx_buf,bleUart.g_ble_uart_rx_len);
						}
					}

           bleUart.g_ble_rec_filish_flag = false;
        }
				
				ble_task_handle();//状态机处理
    }
}
