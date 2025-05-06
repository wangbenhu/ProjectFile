#ifndef __MYAPP_H__
#define __MYAPP_H__

#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "stdarg.h"
#include "stdbool.h"

#define SERVEFRE_1 0x00 // 发球频率 由减速电机1控制
#define SERVEFRE_2 0x01
#define SERVEFRE_3 0x02
#define SERVEFRE_4 0x03
#define SERVEFRE_5 0x04
#define SERVEFRE_6 0x05
#define SERVEFRE_7 0x06
#define SERVEFRE_8 0x07
#define SERVEFRE_9 0x08

#define SERVESPEED_1 0x00
#define SERVESPEED_2 0x01
#define SERVESPEED_3 0x02
#define SERVESPEED_4 0x03
#define SERVESPEED_5 0x04
#define SERVESPEED_6 0x05
#define SERVESPEED_7 0x06
#define SERVESPEED_8 0x07
#define SERVESPEED_9 0x08
#define SERVESPEED_10 0x09
#define SERVESPEED_11 0x0A

/*垂直角度*/
#define V_ANGLE_1 137.7
#define V_ANGLE_2 129.3
#define V_ANGLE_3 122
#define V_ANGLE_4 112.4
#define V_ANGLE_5 81.1
/*水平角度*/
#define H_ANGLE_1 168
#define H_ANGLE_2 160.6
#define H_ANGLE_3 153.9
#define H_ANGLE_4 146.5
#define H_ANGLE_5 138.4
#define H_ANGLE_6 130.7
#define H_ANGLE_7 120.8

typedef struct
{
    uint8_t serveFrequency; // 发球频率：1-9档
    uint8_t serveSpeed;     // 发球速度：1-11档
    float verticalAngle;    // 设置垂直角度
    float horizontalAngle;  // 设置水平角度
} myApp;

uint8_t PlayTennis_ModeControl_DataSet(uint8_t type,uint8_t *data,uint8_t data_len);
void myBeepStatusSet(uint8_t data);
void ManuFlag_Set(bool ststus);
void myAppTaskInit(void);
void myReflector_Trigger(void);
uint8_t BallData_Set(uint8_t freq,uint8_t speed,uint8_t rotation);
uint8_t SwitchSet(uint8_t status);
void myAppLedSwitch_Trigger(void);
void myAppTaskHandle(void);
#endif
