#ifndef __M_MOTOR_H
#define __M_MOTOR_H


#include <stdint.h>

//#define MOTOR_ALWAYS_RUN		(1<<0)
//	#define MOTOR_INTERVAL_RUN		(1<<1)
//	#define MOTOR_TIME_RUN			(1<<2)
//	#define MOTOR_STOP_RUN			(1<<3)
	
typedef enum{
	MOTOR_ALWAYS_RUN		=(1<<0),
	MOTOR_INTERVAL_RUN		=(1<<1),
	MOTOR_TIME_RUN			=(1<<2),
	MOTOR_STOP_RUN			=(1<<3),
	
}Motor_Run_Mode;
void m_motor_init(void);
void m_motor_set(Motor_Run_Mode type,uint32_t run_time);
void set_motor_id(void);
uint8_t m_motor_is_run_get(void);
#endif 
