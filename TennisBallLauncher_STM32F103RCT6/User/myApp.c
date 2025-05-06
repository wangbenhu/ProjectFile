#include "myApp.h"
#include "myLowSpeedMotor.h"
#include "tim.h"
#include "myRemoteControl.h"
#include "myPower.h"
#include "myBeep.h"
#include "myHighSpeedMotor.h"
#include "usart.h"
#include "myLedSwitch.h"
#include "stm32f1xx_it.h"
#include "myBle.h"
myApp myAppStr;
extern stm32f1xx_it driveInt;

#define PROGRAM_MAX_BALL_NUM 11
#define MAX_TIME_OUT (30*1000) //UNIT/MS

enum 
{
	BALL_MODE_LEVEL_LOOP = 1,		//水平循环
	BALL_MODE_VERTICAL_LOOP ,		//垂直循环
	BALL_MODE_CROSS_LANE ,			//交叉球
	BALL_MODE_DIXED_POINT ,			//定点球
	BALL_MODE_TRIANGLE ,				//三角球
	BALL_MODE_RANDOM_PLACE ,		//全场随机
	BALL_MODE_PROGRAM_PLACE ,		//全场编程
	BALL_MODE_THROW ,						//抛球练习
	BALL_MODE_TRACK_CHANGE ,		//变轨球
	BALL_MODE_VOLTAGE_HIGH ,		//高压球
	BALL_MODE_INTERCEPT_STRIKE ,//截击球
	BALL_MODE_MOON ,						//月亮球(上旋)
	BALL_MODE_CUT ,							//削切球（下旋）
	BALL_MODE_END,//
}PlayTennis_Mode_List;



//发球模式控制
static struct {
	uint8_t PlayTennis_Mode;
	uint8_t PlayTennis_Mode_Data[DATA_MAX_LEN];
	uint8_t PlayTennis_Mode_DataLen;
}PlayTennis_ModeControl={BALL_MODE_END,{0},0};

struct {
	float vertical_angle;		//垂直角度
	float standard_angle;		//水平角度
}ball_position_list[29]={
{137.7,168},			//1#
{137.7,160.6},		//2#
{137.7,153.9},		//3#
{137.7,146.5},		//4#
{137.7,138.4},		//5#
{137.7,130.7},		//6#
{137.7,120.8},		//7#

{129.3,168},			//8#
{129.3,160.6},		//9#
{129.3,153.9},		//10#
{129.3,146.5},		//11#
{129.3,138.4},		//12#
{129.3,130.7},		//13#
{129.3,120.8},		//14#

{122,168},				//15#
{122,160.6},			//16#
{122,153.9},			//17#
{122,146.5},			//18#
{122,138.4},			//19#
{122,130.7},			//20#
{122,120.8},			//21#

{112.4,168},			//22#
{112.4,160.6},		//23#
{112.4,153.9},		//24#
{112.4,146.5},		//25#
{112.4,138.4},		//26#
{112.4,130.7},		//27#
{112.4,120.8},		//28#

{81.1,146.5},			//高压球

};//球位表
	
enum{
	myApp_Task_Strtus_Init=0,
	myApp_Task_Strtus_Run,
	myApp_Task_Strtus_ManuMode,
	
	myApp_Task_Strtus_End,
	
}myApp_Task_Strtus_List;//用户状态机列表
	


static uint8_t myApp_Task_Strtus=myApp_Task_Strtus_Init;//用户任务状态机
static struct
{
	uint8_t local_ball_position;			//当前球位
	uint8_t beep_start_flag;					//蜂鸣器运行标志
	uint8_t high1_motor_elect_acq;		//高速电机1电流采集
	uint8_t high2_motor_elect_acq;		//高速电机2电流采集
	uint8_t vertical_motor_elect_acq;	//垂直电机电流采集
	uint8_t standard_motor_elect_acq;	//水平电机电流采集
	uint8_t ble_rercver_start_flag;		//蓝牙是否开启
	uint8_t vertical_motor_coder_acq;	//垂直编码器
	uint8_t standard_motor_coder_acq;	//水平编码器
	uint8_t LedSwitch_count;					//光电开关检测是否有球//pass
	uint32_t Reflector_count;					//漫反射器检测是否发球 30s超时 用来计时
	
	uint8_t rehearsal_switch;					//1 start 0 stop
	
	uint8_t ball_freq;								//1-9
	uint8_t ball_speed;								//1-11 
	uint8_t ball_rotation;						//0-12 1-6上旋 7-12下旋 0不旋转
}AppTmpVariate={11,0,0,0,0,0,1,0,0,1,0,0,6,7,0};

/*水平循环
款两线球	8、14
三线球		8、11、14
水平摆动	8、10、9、11、10、12、14、13、12、9
速度设置 	出球7挡 旋转0挡位
*/
static struct
{
	uint8_t ball_location;//当前球
	uint8_t mode_select;//三种球路控制  0,1,2
	uint8_t ball_path_widetwo[2];
	uint8_t ball_path_threeline[3];
	uint8_t ball_path_levelswing[10];
}BallMode_LevelLoop_Variate={0,0,{8,14},{8,11,14},{8,10,9,11,10,12,14,13,12,9}};


static struct
{
	uint8_t mode_select;//三种球路控制  0,1,2
	uint8_t ball_path_widetwo[2];
	uint8_t ball_path_threeline[3];
	uint8_t ball_path_levelswing[4];

}BallMode_VerticalLoop_Variate={0,{4,25},{4,11,25},{4,11,18,25}};
static struct
{
	uint8_t mode_select;//四种球路控制  0,1,2，3
	uint8_t left_shallow_middle_deep[2];
	uint8_t right_shallow_middle_deep[2];
	uint8_t left_deep_right_shallow[2];
	uint8_t left_shallow_right_deep[2];
}BallMode_CrossLane_Variate={0,{1,18},{7,18},{15,7},{1,21}};
static struct
{
	uint8_t mode_select;//三种球路控制  0,1,2
	uint8_t backhand_exercise;
	uint8_t flat_stroke;
	uint8_t fore_exercise;
}BallMode_FixedPoint_Variate={0,8,11,14};
static struct
{
	uint8_t mode_select;//二球路控制  0,1,2，3
	uint8_t inverted_triangle[3];
	uint8_t equilateral_triangle[3];
}BallMode_Triangle_Variate={0,{1,18,7},{15,4,21}};
static struct
{
	uint8_t ball_location;//球位置
	//uint8_t max_random[20];
}BallMode_RandomPlace_Variate={0};
static struct
{
	uint8_t ball_num;//二球路控制  0,1,2，3
	uint8_t max_programplace[20];
	uint8_t ball_speed;	//速度
	uint8_t ball_spiral;	//旋
	uint8_t ball_altitude;//高度
	uint8_t ball_frequency;//频率
	
}BallMode_ProgramPlace_Variate={0,{0}};
static struct
{
	
	uint8_t ball_location;//球位置
	uint8_t speed_set;
}BallMode_Throw_Variate={4,2};
static struct
{
	uint8_t ball_location;//球位置
	uint8_t ball_list[9];//球
}BallMode_TrackChange_Variate={0,{9,11,13,16,18,20,23,25,26}};
static struct
{
	uint8_t mode_select;//三路球  0,1,2
	uint8_t one_high_pressure;
	uint8_t two_high_pressure[2];
	uint8_t three_high_pressure[3];
}BallMode_Voltage_High_Variate={0,4,{2,6},{2,4,6}};
static struct
{
	uint8_t mode_select;//三路球  0,1,2
	uint8_t backhand_ball;
	uint8_t midfield_ball;
	uint8_t forehand_ball;
}BallMode_Intercept_Strike_Variate={9,11,13};
static struct
{
	uint8_t ball_location;//球位置
	uint8_t rotation_gear;//1-6 上旋
}BallMode_Moon_Variate={18,0};
static struct
{
	uint8_t ball_location;//球位置
	uint8_t rotation_gear;//1-6下旋
}BallMode_Cut_Variate={18,0};//

/*
*********************************************************************************************************
* 函 数 名:PlayTennis_ModeControl_DataSet
* 功能说明: 模式数据设置
* 形  参:   data 
* 返 回 值: 成功/失败 1/0
*********************************************************************************************************
*/
uint8_t PlayTennis_ModeControl_DataSet(uint8_t type,uint8_t *data,uint8_t data_len)
{
	uint8_t return_data=0,error_flag=0;
	ERROR_TO:
	if(error_flag)
	{
		return 0;
	}
	
	debug_uart_printf("PlayTennis_ModeControl_DataSet = %d %d\r\n",type,data_len);
	switch(type)
	{
		case BALL_MODE_LEVEL_LOOP://水平循环
		{
			if(data_len==1)
			{
				if(data[0]>=1 && data[0]<=3)
				{
					BallMode_LevelLoop_Variate.mode_select=data[0];
				}
				else{
					goto ERROR_TO;
				}
				
			}
			else{
				goto ERROR_TO;
			}	
		}
			break;
		case BALL_MODE_VERTICAL_LOOP://垂直循环
		{
			if(data_len==1)
			{
				if(data[0]>=1 && data[0]<=3)
				{
					BallMode_VerticalLoop_Variate.mode_select=data[0];
				}
				else{
					goto ERROR_TO;
				}
				
			}
			else{
				goto ERROR_TO;
			}	
		}
			break;
		case BALL_MODE_CROSS_LANE://交叉球
		{
			if(data_len==1)
			{
				if(data[0]>=1 && data[0]<=4)
				{
					BallMode_CrossLane_Variate.mode_select=data[0];
				}
				else{
					goto ERROR_TO;
				}
				
			}
			else{
				goto ERROR_TO;
			}	
		}
			break;
		case BALL_MODE_DIXED_POINT://定点球
		{
			if(data_len==1)
			{
				if(data[0]>=1 && data[0]<=3)
				{
					BallMode_FixedPoint_Variate.mode_select=data[0];
				}
				else{
					goto ERROR_TO;
				}
				
			}
			else{
				goto ERROR_TO;
			}	
		}
			break;
		case BALL_MODE_TRIANGLE://三角球
		{
			if(data_len==1)
			{
				if(data[0]>=1 && data[0]<=2)
				{
					BallMode_Triangle_Variate.mode_select=data[0];
				}
				else{
					goto ERROR_TO;
				}
				
			}
			else{
				goto ERROR_TO;
			}	
			
		}
			break;
		case BALL_MODE_RANDOM_PLACE://全场随机 06H
		{
			
			if(data_len==1)
			{
				if(data[0]>=1 && data[0]<=28)
				{
					BallMode_RandomPlace_Variate.ball_location=data[0];
				}
				else{
					goto ERROR_TO;
				}
				
			}
			else{
				goto ERROR_TO;
			}	
			
			
		}
			break;
		case BALL_MODE_PROGRAM_PLACE:	//全场编程//07H
		{
			if(data_len>0 && data_len<=DATA_MAX_LEN)
			{
				if(data[0]>=1 && data[0]<=PROGRAM_MAX_BALL_NUM)
				{	
					BallMode_ProgramPlace_Variate.ball_num					=	data[0];
					for(int i=0;i<BallMode_ProgramPlace_Variate.ball_num;i++)
					{
						BallMode_ProgramPlace_Variate.max_programplace[i]=data[i+1];
					}
					BallMode_ProgramPlace_Variate.ball_speed				=data[BallMode_ProgramPlace_Variate.ball_num+1];
					BallMode_ProgramPlace_Variate.ball_spiral				=data[BallMode_ProgramPlace_Variate.ball_num+2];
					BallMode_ProgramPlace_Variate.ball_frequency		=data[BallMode_ProgramPlace_Variate.ball_num+3];
				}
				else{
					goto ERROR_TO;
				}
				
			}
			else{
				goto ERROR_TO;
			}	
			
			
		}
			break;
		case BALL_MODE_THROW:
		{
			if(data_len==2)
			{
//				if(data[0]==0x5A || data[0]==0xA5)
//				{
					BallMode_Throw_Variate.speed_set=data[1];
//				}
//				else{
//					goto ERROR_TO;
//				}
				
			}
			else{
				goto ERROR_TO;
			}	
		}
			break;
		case BALL_MODE_TRACK_CHANGE://变轨球
		{
			
		}
			break;
		case BALL_MODE_VOLTAGE_HIGH:
		{
			if(data_len==1)
			{
				if(data[0]>=1 && data[0]<=3)
				{
					BallMode_Voltage_High_Variate.mode_select=data[0];
				}
				else{
					goto ERROR_TO;
				}
				
			}
			else{
				goto ERROR_TO;
			}	
		}
			break;
		case BALL_MODE_INTERCEPT_STRIKE:
		{
				if(data_len==1)
			{
				if(data[0]>=1 && data[0]<=3)
				{
					BallMode_Intercept_Strike_Variate.mode_select=data[0];
				}
				else{
					goto ERROR_TO;
				}
				
			}
			else{
				goto ERROR_TO;
			}	
		}
			break;
		case BALL_MODE_MOON:
		{
			if(data_len==2)
			{
//				if(data[0]==0x5A || data[0]==0xA5)
//				{
					BallMode_Moon_Variate.rotation_gear=data[1];
//				}
//				else{
//					goto ERROR_TO;
//				}
				
			}
			else{
				goto ERROR_TO;
			}	
		}
			break;
		case BALL_MODE_CUT:
		{
			if(data_len==2)
			{
//				if(data[0]==0x5A || data[0]==0xA5)
//				{
					BallMode_Cut_Variate.rotation_gear=data[1];
//				}
//				else{
//					goto ERROR_TO;
//				}
				
			}
			else{
				goto ERROR_TO;
			}	
		}
			break;
		default:
			goto ERROR_TO;
			break;		
	}
	PlayTennis_ModeControl.PlayTennis_Mode=type;
	memcpy(PlayTennis_ModeControl.PlayTennis_Mode_Data,data,data_len);
	PlayTennis_ModeControl.PlayTennis_Mode_DataLen=data_len;
	return 0;
}

/*
*********************************************************************************************************
* 函 数 名:myBeepStatusSet
* 功能说明: Beep任务状态机设置
* 形  参:   data 
* 返 回 值: 无
*********************************************************************************************************
*/
void myBeepStatusSet(uint8_t data)
{
	if(data)
	{
		debug_uart_printf("beep on \r\n");
	}
	else{
		debug_uart_printf("beep off \r\n");
	}
	AppTmpVariate.beep_start_flag=data;
}

/*
*********************************************************************************************************
* 函 数 名:myAppStatusSet
* 功能说明: APP任务状态机设置
* 形  参:   data @myApp_Task_Strtus_List
* 返 回 值: 无
*********************************************************************************************************
*/
void myAppStatusSet(uint8_t data)
{
	if(data>=myApp_Task_Strtus_Init && data<myApp_Task_Strtus_End)
	{
		myApp_Task_Strtus = data;
	}
}

/*
*********************************************************************************************************
* 函 数 名:serveFrequencySet
* 功能说明: 设置发球频率
* 形  参:   uint8_t grade：1-9档;
* 返 回 值: 无
*********************************************************************************************************
*/
void serveFrequencySet(uint8_t grade)
{
    myAppStr.serveFrequency = grade;
}
/*
*********************************************************************************************************
* 函 数 名:SwitchSet
* 功能说明: 练习模式开关
* 形  参:   status 1/on 0/off
* 返 回 值: 1/0 成功/失败
*********************************************************************************************************
*/
uint8_t SwitchSet(uint8_t status)
{
	uint8_t return_data = 0;
	if(status==0x5A||status==0xA5)
	{
    AppTmpVariate.rehearsal_switch = status;
		return_data=1;
	}
	return return_data;
}

/*
*********************************************************************************************************
* 函 数 名:BallData_Set
* 功能说明: 球参设置
* 形  参:   freq：频率 1-9
						speed：速度 1-11
						rotation：旋转0-12
* 返 回 值: 1/0 成功/失败
*********************************************************************************************************
*/
uint8_t BallData_Set(uint8_t freq,uint8_t speed,uint8_t rotation)
{
	uint8_t return_data = 0;
	if(freq>=1&&freq<=9)
	{
		if(speed>=1&&speed<=11)
		{
			if(rotation>=0&&rotation<=12)
			{
				AppTmpVariate.ball_freq = freq;
				AppTmpVariate.ball_speed = speed;
				AppTmpVariate.ball_rotation = rotation;
				return_data=1;
			}
		}
	}
	return return_data;
}

/*
*********************************************************************************************************
* 函 数 名:serveSpeedSet
* 功能说明: 设置发球速度
* 形  参:   uint8_t speed：1-11档;
* 返 回 值: 无
*********************************************************************************************************
*/
void serveSpeedSet(uint8_t speed)
{
    switch (speed)
    {
    case SERVESPEED_1:
        myHighSpeedMotor1Clockwise(50);
        myHighSpeedMotor2Counterclockwise(50);
        break;
    case SERVESPEED_2:
        myHighSpeedMotor1Clockwise(50);
        myHighSpeedMotor2Counterclockwise(50);
        break;
    case SERVESPEED_3:
        myHighSpeedMotor1Clockwise(50);
        myHighSpeedMotor2Counterclockwise(50);
        break;
    case SERVESPEED_4:
        myHighSpeedMotor1Clockwise(50);
        myHighSpeedMotor2Counterclockwise(50);
        break;
    case SERVESPEED_5:
        myHighSpeedMotor1Clockwise(50);
        myHighSpeedMotor2Counterclockwise(50);
        break;
    case SERVESPEED_6:
        myHighSpeedMotor1Clockwise(50);
        myHighSpeedMotor2Counterclockwise(50);
        break;
    case SERVESPEED_7:
        myHighSpeedMotor1Clockwise(50);
        myHighSpeedMotor2Counterclockwise(50);
        break;
    case SERVESPEED_8:
        myHighSpeedMotor1Clockwise(50);
        myHighSpeedMotor2Counterclockwise(50);
        break;
    case SERVESPEED_9:
        myHighSpeedMotor1Clockwise(50);
        myHighSpeedMotor2Counterclockwise(50);
        break;
    case SERVESPEED_10:
        myHighSpeedMotor1Clockwise(50);
        myHighSpeedMotor2Counterclockwise(50);
        break;
    case SERVESPEED_11:
        myHighSpeedMotor1Clockwise(50);
        myHighSpeedMotor2Counterclockwise(50);
        break;
    }
}
/*
*********************************************************************************************************
* 函 数 名: verticalAngleSet
* 功能说明: 设置上下角度(垂直角度)
* 形  参:   uint8_t angle
* 返 回 值: 无
*********************************************************************************************************
*/
void verticalAngleSet(float angle)
{

    myAppStr.verticalAngle = angle;
#if 0
    float current_angle = 0.0;
    while (1)
    {
        // 读取当前角度
        current_angle = read_Encoder1_Angle();

        // 计算PID输出
        float control_output = myLowSpeedMotor2Positional_PID(angle, current_angle);

        // 设置PWM输出
        if (control_output >= 0)
        {
            myLowSpeedMotor2Clockwise((uint16_t)control_output); // 正转
        }
        else
        {
            myLowSpeedMotor2Counterclockwise((uint16_t)(-control_output)); // 反转
        }

        // 打印调试信息（可选）
        debug_uart_printf("Target: %.2f, Current: %.2f, Output: %.2f\n", angle, current_angle, control_output);

        HAL_Delay(10); // 延时
    }
#endif
}
/*
*********************************************************************************************************
* 函 数 名: horizontalAngleSet
* 功能说明: 设置左右角度(水平角度)
* 形  参:   uint8_t angle
* 返 回 值: 无
*********************************************************************************************************
*/
void horizontalAngleSet(float angle)
{
    myAppStr.horizontalAngle = angle;
#if 0
    float current_angle2 = 0.0;
    while (1)
    {

        // 读取当前角度
        current_angle2 = read_Encoder2_Angle();

        // 计算PID输出
        float control_output2 = myLowSpeedMotor3Positional_PID(myAppStr.horizontalAngle, current_angle2);

        // 设置PWM输出
        if (control_output2 >= 0)
        {
            myLowSpeedMotor3Clockwise((uint16_t)control_output2); // 正转
        }
        else
        {
            myLowSpeedMotor3Counterclockwise((uint16_t)(-control_output2)); // 反转
        }
        // 打印调试信息（可选）
        debug_uart_printf("Target2: %.2f, Current2: %.2f, Output2: %.2f\n", myAppStr.horizontalAngle, current_angle2, control_output2);
        HAL_Delay(10); // 延时
    }
#endif
}
/*
*********************************************************************************************************
* 函 数 名: myAppTaskInit
* 功能说明: 所有任务初始化
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myAppTaskInit(void)
{
    Timer7_Start(); // 定时器7初始化：1ms间隔--用于分配时间片
    Timer6_Start();
    myLowSpeedMotor1Init();        // 减速电机1初始化
    myLowSpeedMotor2Init();        // 减速电机2初始化
    myLowSpeedMotor3Init();        // 减速电机3初始化
    myHighSpeedMotor1Init();       // 高速电机1初始化
    myHighSpeedMotor2Init();       // 高速电机2初始化
    myLedSwitchOn();               // 光电开关打开
    serveFrequencySet(SERVEFRE_1); // 设置发球频率1档
}
/*
*********************************************************************************************************
* 函 数 名: myAppLedSwitch_Trigger
* 功能说明: 光电开关触发
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myAppLedSwitch_Trigger(void)
{
   AppTmpVariate.LedSwitch_count++;
}
/*
*********************************************************************************************************
* 函 数 名: myReflector_Trigger
* 功能说明: 漫反射触发
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myReflector_Trigger(void)
{
   AppTmpVariate.LedSwitch_count--;
}
/*
*********************************************************************************************************
* 函 数 名: myAppMode_TrIGGER
* 功能说明: 击球模式触发
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myAppMode_Trigger(void)
{
	 static uint32_t SysCnt = 0;
    uint8_t temp = 0;
	float verticalAngle_tmp=0.0,horizontalAngle_tmp=0.0;
	
    //temp = api_GetSysTickCount10ms();
	
   // if (SysCnt != temp)
    //{
       // SysCnt = temp;
		 switch(PlayTennis_ModeControl.PlayTennis_Mode)
		{
			case BALL_MODE_LEVEL_LOOP:				//水平循环
				//AppTmpVariate.LedSwitch_count=1;//测试使用
				if(AppTmpVariate.LedSwitch_count>0)
				{  myHighSpeedMotor1Clockwise(50);
					myHighSpeedMotor2Clockwise(100);
					debug_uart_printf("------ = %d\r\n",BallMode_LevelLoop_Variate.mode_select);
					switch(BallMode_LevelLoop_Variate.mode_select)
					{
						case 1:
							if(BallMode_LevelLoop_Variate.ball_location>=sizeof(BallMode_LevelLoop_Variate.ball_path_widetwo))
							{
								BallMode_LevelLoop_Variate.ball_location=0;
							}
							horizontalAngle_tmp=ball_position_list[BallMode_LevelLoop_Variate.ball_path_levelswing[BallMode_LevelLoop_Variate.ball_location++]].standard_angle;
							verticalAngle_tmp=ball_position_list[BallMode_LevelLoop_Variate.ball_path_levelswing[BallMode_LevelLoop_Variate.ball_location++]].vertical_angle;
						break;
						case 2:
							
							break;
						case 3:
							
							break;
						default:
							break;
					}

					verticalAngleSet(verticalAngle_tmp);
					horizontalAngleSet(horizontalAngle_tmp);
					
					AppTmpVariate.Reflector_count+=10;
					if(AppTmpVariate.Reflector_count>=MAX_TIME_OUT)
					{
							APP_Data_Res_call_fun(DATA_TYPE_BALL_ERROR);//上报异常
						AppTmpVariate.Reflector_count=0;
					}
					
					debug_uart_printf("---BALL_MODE_LEVEL_LOOP---\r\n");
					
					PlayTennis_ModeControl.PlayTennis_Mode=BALL_MODE_END;//退出
				}
				 break;
			case BALL_MODE_VERTICAL_LOOP:			//垂直循环
				
				 break;
			case BALL_MODE_CROSS_LANE:				//交叉球
				 break;                 			
			case BALL_MODE_DIXED_POINT:				//定点球
				 break;
			case BALL_MODE_TRIANGLE:					//三角球
				 break;
			case BALL_MODE_RANDOM_PLACE:			//全场随机
				 break;                       
			case BALL_MODE_PROGRAM_PLACE:     //全场编程
				 break;                       
			case BALL_MODE_THROW:      				//抛球练习
				 break;                       
			case BALL_MODE_TRACK_CHANGE:      //变轨球
				 break;                      
			case BALL_MODE_VOLTAGE_HIGH:      //高压球
				 break;                      
			case BALL_MODE_INTERCEPT_STRIKE:  //截击球
				 break;                       
			case BALL_MODE_MOON:      				//月亮球(上旋)
				 break;                       
			case BALL_MODE_CUT:								//削切球（下旋）
				 break;
			default:
				break;
		}
	//}
}
/*
*********************************************************************************************************
* 函 数 名: myAppTaskHandle
* 功能说明: 所有任务执行  采用时间片形式
* 形  参:   无
* 返 回 值: 无
*********************************************************************************************************
*/
void myAppTaskHandle(void)
{	
	//debug_uart_printf("myApp_Task_Strtus = %d\r\n",myApp_Task_Strtus);
	switch(myApp_Task_Strtus)//用户状态机
	{
		case myApp_Task_Strtus_Init:
			//开机蜂鸣器1s
			myBeepSet(BEEP_CONTINUE,1000);

			//myHighSpeedMotor1Clockwise(100);
			//myHighSpeedMotor2Clockwise(100);

			serveSpeedSet(SERVESPEED_2);//开机设置二挡
		//初始化球位置
	//	horizontalAngleSet(AppTmpVariate.local_ball_position-1);
		//verticalAngleSet(AppTmpVariate.local_ball_position-1);
		
//		AppTmpVariate.high1_motor_elect_acq=1;
//		AppTmpVariate.high2_motor_elect_acq=1;
		
			//AppTmpVariate.vertical_motor_coder_acq=1;
			//AppTmpVariate.standard_motor_coder_acq=1;
		
//		AppTmpVariate.standard_motor_elect_acq = 1;
//		AppTmpVariate.vertical_motor_elect_acq = 1;
			debug_uart_printf("myApp_Task_Strtus_Idle\r\n");
		
		myAppStatusSet(myApp_Task_Strtus_Run);	
		//	myAppStatusSet(myApp_Task_Strtus_ManuMode);	
			break;
		case myApp_Task_Strtus_Run://空闲态
		
				batVolHandle(); // 电池电压检测
			break;
		case myApp_Task_Strtus_ManuMode://产测模式//只判断一条 并且不发送鉴权包！！！！！！！！！！！！！！！！
		//	if(ManuFlag_Get())
			//{
			//	uartReceiveDataFromBle();//蓝牙数据接收
			//}
			//else{
			serveSpeedSet(AppTmpVariate.ball_speed);//默认
		serveFrequencySet(AppTmpVariate.ball_freq);
		
				myAppStatusSet(myApp_Task_Strtus_Run);	//切换运行态
			//}
			break;
		
		default:
			break;
	}
	AppTmpVariate.rehearsal_switch=0x5A;//测试
	
	if(AppTmpVariate.rehearsal_switch==0x5A)
	{
		myAppMode_Trigger();
	}
	if(AppTmpVariate.beep_start_flag)
	{
		myBeepTest();
	}
	if(AppTmpVariate.high1_motor_elect_acq)
	{
		if(myHighSpeedMotor1Current()>=0.0)//采集高速电机1电流
		{
			AppTmpVariate.high1_motor_elect_acq=0;
		}
	}	
	if(AppTmpVariate.high2_motor_elect_acq)
	{
		if(myHighSpeedMotor2Current()>=0.0)//采集高速电机2电流
		{
			AppTmpVariate.high2_motor_elect_acq=0;
		}
	}
	if(AppTmpVariate.standard_motor_elect_acq)
	{
		//水平电流采集
		AppTmpVariate.standard_motor_elect_acq=0;
	}
	if(AppTmpVariate.vertical_motor_elect_acq)
	{
			//垂直电流采集
		AppTmpVariate.vertical_motor_elect_acq=0;
	}
	if(AppTmpVariate.ble_rercver_start_flag)
	{
		 uartReceiveDataFromBle();//蓝牙数据接收
	}
	if(AppTmpVariate.vertical_motor_coder_acq)
	{
		read_Encoder1_Angle();//读取编码器的值后需要判断
		AppTmpVariate.vertical_motor_coder_acq=0;
	}
	if(AppTmpVariate.standard_motor_coder_acq)
	{
		read_Encoder2_Angle();//水平编码器
		AppTmpVariate.standard_motor_coder_acq=0;
	}
	
	
    /*-------------------测试代码------------------------------*/
//    static bool runFir = false;
//    if (runFir == false)
//    {
//        // debug_uart_printf("高速电机1启动......\r\n");
      //  myHighSpeedMotor1Clockwise(50);
//        //   HAL_Delay(5000);
//        // debug_uart_printf("高速电机1高速运行......\r\n");
//        //  myHighSpeedMotor1Clockwise(80);
//        //  HAL_Delay(5000);
//        // debug_uart_printf("高速电机1超高速运行......\r\n");
//        //  myHighSpeedMotor1Clockwise(90);
//        //  debug_uart_printf("高速电机2启动......\r\n");
       //  myHighSpeedMotor2Clockwise(100);
//        //  HAL_Delay(5000);
//        //  debug_uart_printf("高速电机2高速运行......\r\n");
//        //  myHighSpeedMotor2Clockwise(800);
//        // debug_uart_printf("减速电机1启动......\r\n");
//        // myLowSpeedMotor1Clockwise(80);
//        //   HAL_Delay(5000);
//        //   debug_uart_printf("减速电机1高速启动......\r\n");
//        //   myLowSpeedMotor1Clockwise(1000);
//        // debug_uart_printf("减速电机2启动......\r\n");
//        // myLowSpeedMotor2Clockwise(100);
//        //    // HAL_Delay(5000);
//        //    // debug_uart_printf("减速电机2高速启动......\r\n");
//        //    // myLowSpeedMotor2Clockwise(1000);
//        //    debug_uart_printf("减速电机3启动......\r\n");
//        //    myLowSpeedMotor3Clockwise(500);
//        //    HAL_Delay(5000);
//        //    debug_uart_printf("减速电机3高速启动......\r\n");
//        //    myLowSpeedMotor3Clockwise(1000);
//        // verticalAngleSet(137.7);
//        // HAL_Delay(5000);
//        // verticalAngleSet(129.3);
//        // HAL_Delay(5000);
//        // verticalAngleSet(122);
//        // HAL_Delay(5000);
//        // verticalAngleSet(112.4);
//        // HAL_Delay(5000);
//        // verticalAngleSet(81.1);
//        // HAL_Delay(5000);
//       // verticalAngleSet(90);
//      //  horizontalAngleSet(146.5);
//        // myLowSpeedMotor1Current(); // 减速电机1电流检测
//        // myLowSpeedMotor2Current(); // 减速电机2电流检测
//        // myLowSpeedMotor3Current(); // 减速电机3电流检测
//        // myHighSpeedMotor1Current(); // 高速电机1电流检测
//        // myHighSpeedMotor2Current(); // 高速电机2电流检测
//        //   myBeepTest();
//        runFir = true;
//    }
    /*-------------------测试代码------------------------------*/
//uartReceiveDataFromBle();//蓝牙数据接收
		
   // myRemoteControlIntHandle(); // 遥控器接收
   // myLedSwitchIntHandle(); // 光电开关
}
