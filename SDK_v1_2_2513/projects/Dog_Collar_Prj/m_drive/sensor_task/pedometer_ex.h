#ifndef __PEDOMETER_EX_H
#define __PEDOMETER_EX_H

#include <stdint.h>
#include <stdbool.h>

int pedometer_init(void);
void set_int1_status(void);
void set_step_num(void);
uint64_t get_step_num(void);
void clear_int1_status(void);


#endif
