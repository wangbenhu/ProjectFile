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
 * @brief    example for using gpio
 * @details  example for using gpio: read, write, trig
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
/// Test pad for gpio output
#define PAD_GPIO_WRITE          14
/// Test pad for gpio input
#define PAD_GPIO_READ           15
/// Test pad for gpio input trigger
#define PAD_GPIO_TRIG           16


/*******************************************************************************
 * CONST & VARIABLES
 */
/// Pinmux Configuration
static pin_config_t pin_config[] = {
    {PAD_GPIO_WRITE, {PINMUX_GPIO_MODE_CFG}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_GPIO_READ,  {PINMUX_GPIO_MODE_CFG}, PMU_PIN_MODE_PU, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_GPIO_TRIG,  {PINMUX_GPIO_MODE_CFG}, PMU_PIN_MODE_PU, PMU_PIN_DRIVER_CURRENT_NORMAL},
};

/// GPIO Configuration
static gpio_config_t gpio_config[] = {
    {OM_GPIO0, PAD_GPIO_WRITE,  GPIO_DIR_OUTPUT, GPIO_LEVEL_LOW, GPIO_TRIG_NONE},
    {OM_GPIO0, PAD_GPIO_READ,   GPIO_DIR_INPUT,  GPIO_LEVEL_LOW, GPIO_TRIG_NONE},
    {OM_GPIO0, PAD_GPIO_TRIG,   GPIO_DIR_INPUT,  GPIO_LEVEL_LOW, GPIO_TRIG_RISING_FAILING_EDGE},
};


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief gpio trigger callback
 *
 * @param[in] om_gpio       Pointer to GPIO
 * @param[in] event         Event
 *                          - DRV_EVENT_COMMON_GENERAL
 * @param[in] int_status    Indicate which pin generated the interrupt
 * @param[in] gpio_data     gpio level data
 *
 *******************************************************************************
 */
static void test_gpio_cb(void *om_gpio, drv_event_t event, void *int_status, void *gpio_data)
{
    om_printf("gpio trigger happens, int_status:%x, gpio_data:%x\r\n", (uint32_t)int_status, (uint32_t)gpio_data);
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief example of using gpio
 *        1. make the pin output high
 *        2. read the pin level
 *        3. configure pin double edge trigger, print gpio information when triggered.
 *
 *******************************************************************************
 */
void example_gpio(void)
{
    drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));
    drv_gpio_init(gpio_config, sizeof(gpio_config) / sizeof(gpio_config[0]));

    // output high level
    drv_gpio_write(OM_GPIO0, GPIO_MASK(PAD_GPIO_WRITE), GPIO_LEVEL_HIGH);

    // read gpio level
    uint32_t level = drv_gpio_read(OM_GPIO0, GPIO_MASK(PAD_GPIO_READ));
    om_printf("pin%d level is %x\r\n", PAD_GPIO_READ, level);

    // trigger: both edge
    drv_gpio_register_isr_callback(OM_GPIO0, test_gpio_cb);
}


/** @} */