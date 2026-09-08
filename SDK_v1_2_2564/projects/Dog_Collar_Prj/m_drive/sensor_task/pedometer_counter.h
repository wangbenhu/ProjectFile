#ifndef PEDOMETER_COUNTER_H
#define PEDOMETER_COUNTER_H

#include <stdbool.h>
#include <stdint.h>

uint64_t pedometer_step_delta(uint64_t current, uint64_t previous,
                              bool normal_wrap, bool reset_detected);

#endif
