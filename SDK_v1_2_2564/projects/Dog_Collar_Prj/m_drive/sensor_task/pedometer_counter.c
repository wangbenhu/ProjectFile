#include "pedometer_counter.h"

#define PEDOMETER_COUNTER_MODULO (1ULL << 48)

uint64_t pedometer_step_delta(uint64_t current, uint64_t previous,
                              bool normal_wrap, bool reset_detected)
{
    if (reset_detected || (!normal_wrap && current < previous)) {
        return 0U;
    }

    if (normal_wrap) {
        return PEDOMETER_COUNTER_MODULO - previous + current;
    }

    return current - previous;
}
