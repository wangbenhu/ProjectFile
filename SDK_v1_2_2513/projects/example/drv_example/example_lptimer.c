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
 * @brief    example for using lp timer
 * @details  example for using lp timers: free running mode, buffered mode, one
 *           shot mode, double mode
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
/// Test pad for lp timer out0
#define PAD_LPTIM_OUT0          18
/// Test pad for lp timer out1
#define PAD_LPTIM_OUT1          19
/// Test pad for lp timer counter
#define PAD_LPTIM_COUNT         14

/// Test mux for lp timer out0
#define MUX_LPTIM_OUT0          PINMUX_PAD18_LPTIM_OUT0_CFG
/// Test mux for lp timer out1
#define MUX_LPTIM_OUT1          PINMUX_PAD19_LPTIM_OUT1_CFG
/// Test mux for lp timer counter
#define MUX_LPTIM_COUNT         PINMUX_PAD14_GPIO_MODE_CFG


/*******************************************************************************
 * TYPEDEFS
 */


/*******************************************************************************
 * CONST & VARIABLES
 */


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief lp timer counter callback, when the lp timer generates a REP0 event
 * or an UNDER_FLOW event, toggle the PAD_LPTIM_COUNT
 *
 * @param[in] om_lptim     Pointer to lp timer
 * @param[in] event        LP timer event
 * @param[in] buff         Pointer to data
 * @param[in] num          Number of data
 *
 *******************************************************************************
 */
static void lptim_count_callback(void *om_lptim, drv_event_t event, void *buff, void *num)
{
    if ((event & DRV_EVENT_LPTIM_REP0) && (((OM_LPTIM_Type *)om_lptim)->INTE & LPTIM_INTE_REP0_EN_MASK)){
        drv_gpio_toggle(OM_GPIO0, BITMASK(PAD_LPTIM_COUNT));
        drv_lptim_control(om_lptim, LPTIM_CONTROL_STOP, NULL);
    }
    if ((event & DRV_EVENT_LPTIM_UNDER_FLOW) && (((OM_LPTIM_Type *)om_lptim)->INTE & LPTIM_INTE_UF_EN_MASK)){
        drv_gpio_toggle(OM_GPIO0, BITMASK(PAD_LPTIM_COUNT));
    }
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief example of using lp timer as counter with oneshot mode.
 *in this mode the count is stoped when sep0 is reduced to 0.
 * toggle the pin level when the lp timer reaches 3 seconds
 *
 *******************************************************************************
 */
void example_lptim_oneshot_count(void)
{
    uint8_t i = 1;
    lptim_one_shot_config_t    mode_cfg;

    pin_config_t pin_cfg[] = {
        {PAD_LPTIM_COUNT, {MUX_LPTIM_COUNT}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    };
    gpio_config_t gpio_cfg[] = {
        {OM_GPIO0, PAD_LPTIM_COUNT, GPIO_DIR_OUTPUT, GPIO_LEVEL_LOW, GPIO_TRIG_NONE},
    };
    drv_pin_init(pin_cfg, sizeof(pin_cfg) / sizeof(pin_cfg[0]));
    drv_gpio_init(gpio_cfg, sizeof(gpio_cfg) / sizeof(gpio_cfg[0]));

    mode_cfg.presclar       = LPTIM_PRESC_DIV1;
    mode_cfg.rep0_val       = 2;//8bits reg,max 0xFF(255).the value 0 and 255 represents a loop count of 1 and 256 rounds.so the final number needs to be subtracted by one.
    mode_cfg.rep2_val       = 0;
    mode_cfg.top_en         = LPTIM_TOP_ENABLE,
    mode_cfg.top_val        = 32*1000-1;//16bits reg,max 0xFFFF(65535).the value 32 represents one millisecond.the final number needs to be subtracted by one.,
    mode_cfg.compare_val0   = 0;
    mode_cfg.compare_val1   = 0;
    mode_cfg.compare_val2   = 0;
    mode_cfg.compare_val3   = 0;
    mode_cfg.stop_mode      = LPTIM_STOP_MODE_STOPBOTH;
    drv_lptim_one_shot_init(OM_LPTIM, &mode_cfg);
    drv_lptim_register_isr_callback(OM_LPTIM, lptim_count_callback);

    drv_lptim_control(OM_LPTIM, LPTIM_CONTROL_INT_EN, (void *)(LPTIM_INTE_REP0_EN_MASK | LPTIM_INTE_UF_EN_MASK));
    drv_lptim_control(OM_LPTIM, LPTIM_CONTROL_POWER_IN_SLEEP, &i);
    drv_pmu_32k_enable_in_deep_sleep(true);

    drv_lptim_control(OM_LPTIM, LPTIM_CONTROL_START, NULL);

    drv_gpio_toggle(OM_GPIO0, BITMASK(PAD_LPTIM_COUNT));
}

/**
 *******************************************************************************
 * @brief example of using lp timer as counter with free_running mode.
 *in this mode the count is continuous until a stop command is sent.
 *toggle the pin level when the lp timer reaches 2 seconds
 *
 *******************************************************************************
 */
void example_lptim_free_running_count(void)
{
    uint8_t i = 1;
    lptim_free_running_config_t    mode_cfg;

    pin_config_t pin_cfg[] = {
        {PAD_LPTIM_COUNT, {MUX_LPTIM_COUNT}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    };
    gpio_config_t gpio_cfg[] = {
        {OM_GPIO0, PAD_LPTIM_COUNT, GPIO_DIR_OUTPUT, GPIO_LEVEL_LOW, GPIO_TRIG_NONE},
    };
    drv_pin_init(pin_cfg, sizeof(pin_cfg) / sizeof(pin_cfg[0]));
    drv_gpio_init(gpio_cfg, sizeof(gpio_cfg) / sizeof(gpio_cfg[0]));

    mode_cfg.presclar       = LPTIM_PRESC_DIV1;
    mode_cfg.top_en         = LPTIM_TOP_ENABLE;
    mode_cfg.top_val        = 32*2000-1;//16bits reg,max 0xFFFF(65535).the value 32 represents one millisecond.the final number needs to be subtracted by one.
    mode_cfg.compare_val0   = 0;
    mode_cfg.compare_val1   = 0;
    mode_cfg.compare_val2   = 0;
    mode_cfg.compare_val3   = 0;
    mode_cfg.stop_mode      = LPTIM_STOP_MODE_FREE;
    drv_lptim_free_running_init(OM_LPTIM, &mode_cfg);
    drv_lptim_register_isr_callback(OM_LPTIM, lptim_count_callback);

    drv_lptim_control(OM_LPTIM, LPTIM_CONTROL_INT_EN, (void *)LPTIM_INTE_UF_EN_MASK);
    drv_lptim_control(OM_LPTIM, LPTIM_CONTROL_POWER_IN_SLEEP, &i);
    drv_pmu_32k_enable_in_deep_sleep(true);

    drv_lptim_control(OM_LPTIM, LPTIM_CONTROL_START, NULL);

    drv_gpio_toggle(OM_GPIO0, BITMASK(PAD_LPTIM_COUNT));
}

/**
 *******************************************************************************
 * @brief example of continuous pulse generation using lp timer
 *
 *******************************************************************************
 */
void example_lptim_free_running_pwm(void)
{
    uint8_t i = 1;
    lptim_free_running_config_t    mode_cfg;
    lptim_out_config_t             out_cfg;

    mode_cfg.presclar       = LPTIM_PRESC_DIV1;
    mode_cfg.top_en         = LPTIM_TOP_ENABLE;
    mode_cfg.top_val        = 500;
    mode_cfg.compare_val0   = 250;
    mode_cfg.compare_val1   = 0;
    mode_cfg.compare_val2   = 0;
    mode_cfg.compare_val3   = 0;
    mode_cfg.stop_mode      = LPTIM_STOP_MODE_FREE;
    drv_lptim_free_running_init(OM_LPTIM, &mode_cfg);

    pin_config_t pin_cfg_cnt [] = {
        {PAD_LPTIM_OUT0,  {MUX_LPTIM_OUT0},  PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    };
    drv_pin_init(pin_cfg_cnt, sizeof(pin_cfg_cnt) / sizeof(pin_cfg_cnt[0]));
    out_cfg.pol         = LPTIM_POL_IDLE_LOW;
    out_cfg.action      = LPTIM_UFOA_PWM;
    drv_lptim_outx_config(OM_LPTIM, LPTIM_CHAN_OUT0, &out_cfg);

    drv_lptim_control(OM_LPTIM, LPTIM_CONTROL_POWER_IN_SLEEP, &i);
    drv_pmu_32k_enable_in_deep_sleep(true);

    drv_lptim_control(OM_LPTIM, LPTIM_CONTROL_START, NULL);
}

/**
 *******************************************************************************
 * @brief example of generating (rep0+1) pulses using lp time
 *
 *******************************************************************************
 */
void example_lptim_oneshot_pwm(void)
{
    uint8_t i = 1;
    lptim_one_shot_config_t    mode_cfg;
    lptim_out_config_t         out_cfg;

    mode_cfg.presclar       = LPTIM_PRESC_DIV1;
    mode_cfg.rep0_val       = 3;
    mode_cfg.rep2_val       = 0;
    mode_cfg.top_en         = LPTIM_TOP_ENABLE,
    mode_cfg.top_val        = 500;
    mode_cfg.compare_val0   = 250;
    mode_cfg.compare_val1   = 0;
    mode_cfg.compare_val2   = 0;
    mode_cfg.compare_val3   = 0;
    mode_cfg.stop_mode      = LPTIM_STOP_MODE_STOPBOTH;
    drv_lptim_one_shot_init(OM_LPTIM, &mode_cfg);

    pin_config_t pin_cfg_cnt [] = {
        {PAD_LPTIM_OUT0,  {MUX_LPTIM_OUT0},  PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    };
    drv_pin_init(pin_cfg_cnt, sizeof(pin_cfg_cnt) / sizeof(pin_cfg_cnt[0]));
    out_cfg.pol         = LPTIM_POL_IDLE_LOW;
    out_cfg.action      = LPTIM_UFOA_PWM;
    drv_lptim_outx_config(OM_LPTIM, LPTIM_CHAN_OUT0, &out_cfg);

    drv_lptim_control(OM_LPTIM, LPTIM_CONTROL_POWER_IN_SLEEP, &i);
    drv_pmu_32k_enable_in_deep_sleep(true);

    drv_lptim_control(OM_LPTIM, LPTIM_CONTROL_START, NULL);
}

/**
 *******************************************************************************
 * @brief example of using lp timer to generate a specified number of pulses
 * with a specified period
 *
 *******************************************************************************
 */
void example_lptim_buffered_pwm(void)
{
    uint8_t i = 1;
    lptim_buffered_config_t    mode_cfg;
    lptim_out_config_t         out_cfg;

    mode_cfg.presclar       = LPTIM_PRESC_DIV1;
    mode_cfg.rep0_val       = 3;
    mode_cfg.rep1_val       = 0;
    mode_cfg.rep2_val       = 0;
    mode_cfg.rep3_val       = 0;
    mode_cfg.top_en         = LPTIM_TOP_ENABLE;
    mode_cfg.top_val        = 500;
    mode_cfg.buftop_en      = LPTIM_BUFTOP_ENABLE;
    mode_cfg.buftop_val     = 500;
    mode_cfg.compare_val0   = 250;
    mode_cfg.compare_val1   = 0;
    mode_cfg.compare_val2   = 0;
    mode_cfg.compare_val3   = 0;
    mode_cfg.stop_mode      = LPTIM_STOP_MODE_STOPBOTH;
    drv_lptim_buffered_init(OM_LPTIM, &mode_cfg);

    pin_config_t pin_cfg_cnt [] = {
        {PAD_LPTIM_OUT0,  {MUX_LPTIM_OUT0},  PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    };
    drv_pin_init(pin_cfg_cnt, sizeof(pin_cfg_cnt) / sizeof(pin_cfg_cnt[0]));
    out_cfg.pol         = LPTIM_POL_IDLE_LOW;
    out_cfg.action      = LPTIM_UFOA_PWM;
    drv_lptim_outx_config(OM_LPTIM, LPTIM_CHAN_OUT0, &out_cfg);

    drv_lptim_control(OM_LPTIM, LPTIM_CONTROL_POWER_IN_SLEEP, &i);
    drv_pmu_32k_enable_in_deep_sleep(true);

    drv_lptim_control(OM_LPTIM, LPTIM_CONTROL_START, NULL);
}

/**
 *******************************************************************************
 * @brief example of using lp timer to generate two-way signals
 *
 *******************************************************************************
 */
void example_lptim_double_pwm(void)
{
    uint8_t i = 1;
    lptim_double_config_t      mode_cfg;
    lptim_out_config_t         out_cfg;

    mode_cfg.presclar       = LPTIM_PRESC_DIV1;
    mode_cfg.rep0_val       = 3;
    mode_cfg.rep1_val       = 5;
    mode_cfg.rep2_val       = 0;
    mode_cfg.rep3_val       = 0;
    mode_cfg.top_en         = LPTIM_TOP_ENABLE;
    mode_cfg.top_val        = 500;
    mode_cfg.compare_val0   = 50;
    mode_cfg.compare_val1   = 250;
    mode_cfg.compare_val2   = 0;
    mode_cfg.compare_val3   = 0;
    mode_cfg.stop_mode      = LPTIM_STOP_MODE_STOPBOTH;
    drv_lptim_double_init(OM_LPTIM, &mode_cfg);

    pin_config_t pin_cfg_cnt [] = {
        {PAD_LPTIM_OUT0,  {MUX_LPTIM_OUT0},  PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
        {PAD_LPTIM_OUT1,  {MUX_LPTIM_OUT1},  PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    };
    drv_pin_init(pin_cfg_cnt, sizeof(pin_cfg_cnt) / sizeof(pin_cfg_cnt[0]));
    out_cfg.pol         = LPTIM_POL_IDLE_LOW;
    out_cfg.action      = LPTIM_UFOA_PWM;
    drv_lptim_outx_config(OM_LPTIM, LPTIM_CHAN_OUT0, &out_cfg);
    drv_lptim_outx_config(OM_LPTIM, LPTIM_CHAN_OUT1, &out_cfg);

    drv_lptim_control(OM_LPTIM, LPTIM_CONTROL_POWER_IN_SLEEP, &i);
    drv_pmu_32k_enable_in_deep_sleep(true);

    drv_lptim_control(OM_LPTIM, LPTIM_CONTROL_START, NULL);
}


/** @} */