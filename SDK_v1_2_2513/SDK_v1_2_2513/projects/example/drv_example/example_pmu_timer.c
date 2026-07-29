/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup DOC DOC
 * @ingroup  DOCUMENT
 * @brief    example for using pmu timer
 * @details  example for using pmu timer: set pmu timer and get pmu timer cnt
 * @version
 *
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


/*******************************************************************************
 * INCLUDES
 */
#include "om_driver.h"


/*******************************************************************************
 * MACROS
 */


/*******************************************************************************
 * TYPEDEFS
 */


/*******************************************************************************
 * CONST & VARIABLES
 */
static volatile uint32_t int0_cnt, int1_cnt;


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static void pmu_timer_val0_test_callback(void *om_reg, drv_event_t drv_event, void *param0, void *param1)
{
    int0_cnt++;
    om_printf("pmu timer, int0_cnt:%d\r\n", int0_cnt);
}

static void pmu_timer_val1_test_callback(void *om_reg, drv_event_t drv_event, void *param0, void *param1)
{
    int1_cnt++;
    om_printf("pmu timer, int1_cnt:%d\r\n", int1_cnt);
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief example of using pmu timer
 *
 *******************************************************************************
 */
void example_pmu_timer(void)
{
    uint32_t cnt_val;

    int0_cnt = 0;
    int1_cnt = 0;

    drv_pmu_timer_register_isr_callback(PMU_TIMER_TRIG_VAL0, pmu_timer_val0_test_callback);
    drv_pmu_timer_register_isr_callback(PMU_TIMER_TRIG_VAL1, pmu_timer_val1_test_callback);
    drv_pmu_timer_control(PMU_TIMER_TRIG_VAL0, PMU_TIMER_CONTROL_ENABLE, (void*)1);
    drv_pmu_timer_control(PMU_TIMER_TRIG_VAL1, PMU_TIMER_CONTROL_ENABLE, (void*)1);
    drv_pmu_timer_init();

    cnt_val = drv_pmu_timer_cnt_get();
    om_printf("cnt_val0:%ld\r\n", cnt_val);
    drv_pmu_timer_control(PMU_TIMER_TRIG_VAL0, PMU_TIMER_CONTROL_SET_TIMER_VAL, (void *)(cnt_val + 10 * 1000));
    drv_pmu_timer_control(PMU_TIMER_TRIG_VAL1, PMU_TIMER_CONTROL_SET_TIMER_VAL, (void *)(cnt_val + 10 * 1000));
    while (int0_cnt < 1 || int1_cnt < 1);

    cnt_val = drv_pmu_timer_cnt_get();
    om_printf("cnt_val1:%ld\r\n", cnt_val);
    drv_pmu_timer_control(PMU_TIMER_TRIG_VAL0, PMU_TIMER_CONTROL_SET_TIMER_VAL, (void *)(cnt_val + 20 * 1000));
    drv_pmu_timer_control(PMU_TIMER_TRIG_VAL1, PMU_TIMER_CONTROL_SET_TIMER_VAL, (void *)(cnt_val + 20 * 1000));
    while (int0_cnt < 2 || int1_cnt < 2);

    cnt_val = drv_pmu_timer_cnt_get();
    om_printf("cnt_val2:%ld\r\n", cnt_val);
    drv_pmu_timer_control(PMU_TIMER_TRIG_VAL0, PMU_TIMER_CONTROL_SET_TIMER_VAL, (void *)(cnt_val + 30 * 1000));
    drv_pmu_timer_control(PMU_TIMER_TRIG_VAL1, PMU_TIMER_CONTROL_SET_TIMER_VAL, (void *)(cnt_val + 30 * 1000));
    while (int0_cnt < 3 || int1_cnt < 3);

    cnt_val = drv_pmu_timer_cnt_get();
    om_printf("cnt_val3:%ld\r\n", cnt_val);

    drv_pmu_timer_control(PMU_TIMER_TRIG_VAL0, PMU_TIMER_CONTROL_ENABLE, (void*)0);
    drv_pmu_timer_control(PMU_TIMER_TRIG_VAL1, PMU_TIMER_CONTROL_ENABLE, (void*)0);
}


/** @} */