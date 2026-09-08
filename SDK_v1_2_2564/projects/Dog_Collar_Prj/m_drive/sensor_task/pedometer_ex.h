#ifndef __PEDOMETER_EX_H
#define __PEDOMETER_EX_H

#include <stdint.h>
#include <stdbool.h>
#include "inv_imu_driver.h"
void set_int1_status(void);
void set_step_num(void);
uint64_t get_step_num(void);
void clear_int1_status(void);
int pedometer_init(struct inv_imu_device *dev);
int pedometer_process_irq(struct inv_imu_device *dev);

#endif
