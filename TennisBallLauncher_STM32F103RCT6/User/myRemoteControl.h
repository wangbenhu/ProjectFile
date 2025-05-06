#ifndef __MYREMOTECONTROL_H__
#define __MYREMOTECONTROL_H__

#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "stdarg.h"
#include "stdbool.h"
typedef struct
{
    uint8_t keyVal;
} myRemoteControl_str;

void myRemoteControlIntHandle(void);
#endif
