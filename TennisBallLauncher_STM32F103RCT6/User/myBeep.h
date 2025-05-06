#ifndef __MYBEEP_H__
#define __MYBEEP_H__

#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "stdarg.h"
#include "stdbool.h"
typedef enum {
	BEEP_CONTINUE_STOP=0,	//停止
	BEEP_CONTINUE,			//持续
	BEEP_INTERVAL,			//间歇
	BEEP_END,
}BeepModeList;

void myBeepSet(BeepModeList mode,uint32_t time);//100ms倍数
void myBeepOn(void);
void myBeepOff(void);
void myBeepTest(void);
#endif
