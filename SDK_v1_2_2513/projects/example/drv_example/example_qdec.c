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
 * @brief    example for using qdec
 * @details  example for using qdec: decoding of digital waveform from off-chip
 *           quadrature encoder
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
#define PAD_QDEC_XA   17
#define PAD_QDEC_XB   18
#define PAD_QDEC_LED  19

#define MUX_QDEC_XA   PINMUX_PAD17_QDEC_XA_CFG
#define MUX_QDEC_XB   PINMUX_PAD18_QDEC_XB_CFG
#define MUX_QDEC_LED  PINMUX_PAD19_QDEC_LED_CFG
/*******************************************************************************
 * TYPEDEFS
 */


/*******************************************************************************
 * CONST & VARIABLES
 */
static volatile uint8_t qdec_stop = 0;
static volatile int cnt = 0;

static pin_config_t pin_config[] = {
    {PAD_QDEC_XA,  {MUX_QDEC_XA},  PMU_PIN_MODE_PU, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_QDEC_XB,  {MUX_QDEC_XB},  PMU_PIN_MODE_PU, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_QDEC_LED, {MUX_QDEC_LED}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
};

/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static void qdec_callback(void *om_qdec, drv_event_t drv_event, void *p_acc, void *p_accdbl)
{
    int acc = (int)p_acc;
    int accdbl = (int)p_accdbl;

    if (drv_event & DRV_EVENT_QDEC_SAMPLERDY) {
        if (0 == accdbl) {
            cnt += acc;
        }
        om_printf("[EVENT_SAMPLERDY] acc = %d, accdbl = %d, cnt = %d\n", acc, accdbl, cnt);
        if (cnt > 100 || cnt < -100) {
            drv_qdec_control(OM_QDEC, QDEC_CONTROL_STOP, NULL);
        }
    }
    if (drv_event & DRV_EVENT_QDEC_REPORTRDY) {
        if (0 == accdbl) {
            cnt += acc;
        }
        om_printf("[EVENT_REPORTRDY] acc = %d, accdbl = %d, cnt = %d\n", acc, accdbl, cnt);
        if (cnt > 100 || cnt < -100) {
            drv_qdec_control(OM_QDEC, QDEC_CONTROL_STOP, NULL);
        }
    }
    if (drv_event & DRV_EVENT_QDEC_ACCOF) {
        om_printf("[EVENT_ACCOF]\n");
        drv_qdec_control(OM_QDEC, QDEC_CONTROL_STOP, NULL);
    }
    if (drv_event & DRV_EVENT_QDEC_DBLRDY) {
        om_printf("[EVENT_DBLRDY] acc = %d, accdbl = %d\n", acc, accdbl);
    }
    if (drv_event & DRV_EVENT_QDEC_STOPPED) {
        om_printf("[EVENT_STOPPED]\n");
        qdec_stop = 1;
        cnt = 0;
    }
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief example of using qdec
 *
 *******************************************************************************
 */
void example_qdec(void)
{
    drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));
    drv_pmu_pin_pullup_set(PAD_QDEC_XA, PMU_PIN_PULLUP_4M);
    drv_pmu_pin_pullup_set(PAD_QDEC_XB, PMU_PIN_PULLUP_4M);

    qdec_stop = 0;
    const qdec_config_t qdec_cfg = {
        .dbf_en          = true,
        .led_pol         = QDEC_LED_POL_LOW,
        .pin_sel_led     = QDEC_PIN_SEL_LED_DISCONNECT,
        .report_per      = QDEC_REPORT_PER_160,
        .sample_per      = QDEC_SAMPLE_PER_256US,
        .led_pre         = 0,
    };

    drv_qdec_init(OM_QDEC, &qdec_cfg);
    drv_qdec_register_isr_callback(OM_QDEC, qdec_callback);
    drv_qdec_control(OM_QDEC, QDEC_CONTROL_INT_ENABLE, (void *)(QDEC_INTEN_REPORTRDY_MASK | QDEC_INTEN_STOPPED_MASK));
    drv_qdec_control(OM_QDEC, QDEC_CONTROL_START, NULL);

    while(!qdec_stop);
}

/**
 *******************************************************************************
 * @brief example of using qdec with led output signal
 *
 *******************************************************************************
 */
void example_qdec_with_led(void)
{
    drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));
    drv_pmu_pin_pullup_set(PAD_QDEC_XA, PMU_PIN_PULLUP_4M);
    drv_pmu_pin_pullup_set(PAD_QDEC_XB, PMU_PIN_PULLUP_4M);

    qdec_stop = 0;
    const qdec_config_t qdec_cfg = {
        .dbf_en          = false,
        .led_pol         = QDEC_LED_POL_HIGH,
        .pin_sel_led     = QDEC_PIN_SEL_LED_CONNECT,
        .report_per      = QDEC_REPORT_PER_160,
        .sample_per      = QDEC_SAMPLE_PER_256US,
        .led_pre         = 50,
    };

    drv_qdec_init(OM_QDEC, &qdec_cfg);
    drv_qdec_register_isr_callback(OM_QDEC, qdec_callback);
    drv_qdec_control(OM_QDEC, QDEC_CONTROL_INT_ENABLE, (void *)(QDEC_INTEN_REPORTRDY_MASK | QDEC_INTEN_STOPPED_MASK));
    drv_qdec_control(OM_QDEC, QDEC_CONTROL_START, NULL);

    while(!qdec_stop);
}

/** @} */