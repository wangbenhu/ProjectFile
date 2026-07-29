#ifndef __IMU_EX_H__
#define __IMU_EX_H__
#include <stdint.h>
#include <stdbool.h>

/* Initial WOM threshold to be applied to IMU in mg */
#define WOM_THRESHOLD_INITIAL_MG 255*4  //mg

//定义达到运动程度需要产生的中断个数/分钟
#define MOTION_LESS_INT_NUM			0  	//静止
#define MOTION_1_INT_NUM				20	//>0 && < MOTION_1_INT_NUM: 轻微运动 ; > MOTION_2_INT_NUM: 剧烈运动

typedef enum
{
		MOTION_LESS,  //静止
		MOTION_S, //轻度
		MOTION_L,	//重度
}Motion_Level;

void imu_init(void);
int get_imu_data(void);
int config_wom_int1(void);
void set_motion_level(void);
bool judge_motion_level_change(void);
bool get_imu_state(void);
Motion_Level get_motion_level(void);
Motion_Level get_last_motion_level(void);
int imu_enter_sleep_mode(void);
bool judge_int_at_Z(void);


#endif

