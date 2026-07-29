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
 * @brief    GPIO driver source file
 * @details  GPIO driver source file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_GPIO0 || RTE_GPIO1)
#include <stdint.h>
#include "om_device.h"
#include "om_driver.h"


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    drv_isr_callback_t  isr_cb;
    uint32_t            gpio_using;  // bit field, each bit indicates a gpio
} gpio_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
#if (RTE_GPIO0)
static gpio_env_t gpio0_env = {
    .isr_cb   = NULL,
    .gpio_using = 0U,
};
static const drv_resource_t gpio0_resource = {
    .cap      = CAP_GPIO0,
    .reg      = (void *)OM_GPIO0,
    .irq_num  = GPIO_IRQn,
    .irq_prio = RTE_GPIO_IRQ_PRIORITY,
    .env      = (void *)(&gpio0_env),
};
#endif

#if (RTE_GPIO1)
static gpio_env_t gpio1_env = {
    .isr_cb   = NULL,
    .gpio_using = 0U,
};
static const drv_resource_t gpio1_resource = {
    .cap      = CAP_GPIO1,
    .reg      = (void *)OM_GPIO1,
    .irq_num  = GPIO_IRQn,
    .irq_prio = RTE_GPIO_IRQ_PRIORITY,
    .env      = (void *)(&gpio1_env),
};
#endif

static const drv_resource_t *gpio_resources[] = {
    #if (RTE_GPIO0)
    &gpio0_resource,
    #endif
    #if (RTE_GPIO1)
    &gpio1_resource,
    #endif
};


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
/* GPIO0 & GPIO1 using the same clk gate control & the same NVIC interrupt */
static uint32_t gpio_is_using(void)
{
    gpio_env_t  *env;
    for (uint32_t i=0; i<sizeof(gpio_resources)/sizeof(gpio_resources[0]); i++) {
        env = (gpio_env_t *)(gpio_resources[i]->env);
        if (env->gpio_using) {
            return 1U;
        }
    }

    return 0U;
}

static const drv_resource_t *gpio_get_resource(OM_GPIO_Type *om_gpio)
{
    for (uint32_t i=0; i<sizeof(gpio_resources)/sizeof(gpio_resources[0]); i++) {
        if ((uint32_t)om_gpio == (uint32_t)gpio_resources[i]->reg) {
            return gpio_resources[i];
        }
    }

    OM_ASSERT(0);
    return NULL;
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
void drv_gpio_init(const gpio_config_t *gpio_config, uint32_t gpio_config_num)
{
    const drv_resource_t *resource;
    OM_GPIO_Type         *om_gpio;
    gpio_env_t           *env;

    if (gpio_config_num == 0U) {
        return;
    }
    OM_ASSERT(gpio_config);
    OM_ASSERT(gpio_config->gpio_idx < CAP_DIG_PAD_NUM);
    OM_ASSERT(gpio_config->dir <= GPIO_DIR_INPUT);
    OM_ASSERT(gpio_config->trig_type <= GPIO_TRIG_NONE);

    if (!gpio_is_using()) {
        OM_CRITICAL_BEGIN();
        DRV_RCC_CLOCK_ENABLE(RCC_CLK_GPIO0, 1U);
        // Clear and Enable GPIO IRQ
        NVIC_ClearPendingIRQ(GPIO_IRQn);
        NVIC_SetPriority(GPIO_IRQn, RTE_GPIO_IRQ_PRIORITY);
        NVIC_EnableIRQ(GPIO_IRQn);
        OM_CRITICAL_END();
    }

    for (uint32_t i = 0; i < gpio_config_num; ++i) {
        OM_ASSERT(gpio_config[i].om_gpio);
        resource = gpio_get_resource(gpio_config[i].om_gpio);
        if (resource) {
            env = (gpio_env_t *)(resource->env);
            om_gpio = (OM_GPIO_Type *)(resource->reg);

            OM_CRITICAL_BEGIN();
            uint8_t gpio_idx = gpio_config[i].gpio_idx;
            if (OM_GPIO1 == om_gpio) {
                gpio_idx = gpio_config[i].gpio_idx - 32;
            }
            env->gpio_using |= (1U << gpio_idx);
            drv_gpio_set_dir(om_gpio, 1U << gpio_idx, gpio_config[i].dir);
            if(gpio_config[i].dir == GPIO_DIR_OUTPUT) {
                drv_gpio_write(om_gpio, 1U << gpio_idx, gpio_config[i].out_val);
            } else {
                drv_gpio_set_trig(om_gpio, 1U << gpio_idx, gpio_config[i].trig_type);
            }
            OM_CRITICAL_END();
        }
    }
}

#if (RTE_GPIO_REGISTER_CALLBACK)
void drv_gpio_register_isr_callback(OM_GPIO_Type *om_gpio, drv_isr_callback_t isr_cb)
{
    const drv_resource_t *resource;
    gpio_env_t           *env;

    resource = gpio_get_resource(om_gpio);
    if (resource) {
        env = (gpio_env_t *)(resource->env);
        env->isr_cb = isr_cb;
    }
}
#endif

__WEAK void drv_gpio_isr_callback(OM_GPIO_Type *om_gpio, drv_event_t event, uint32_t int_status, uint32_t gpio_data)
{
    #if (RTE_GPIO_REGISTER_CALLBACK)
    const drv_resource_t *resource;
    gpio_env_t           *env;

    resource = gpio_get_resource(om_gpio);
    if (resource) {
        env = (gpio_env_t *)(resource->env);
        if (env->isr_cb != NULL) {
            env->isr_cb(om_gpio, event, (void *)int_status, (void *)gpio_data);
        }
    }
    #endif
}

void drv_gpio_set_trig(OM_GPIO_Type *om_gpio, gpio_mask_t gpio_mask, gpio_trig_type_t trig)
{
    om_gpio->INTSTATUS  = gpio_mask;    /* clear irq status */

    switch(trig) {
        case GPIO_TRIG_NONE:
            om_gpio->INTENCLR   = gpio_mask;
            break;
        case GPIO_TRIG_FALLING_EDGE:
            om_gpio->INTTYPESET = gpio_mask;
            om_gpio->INTPOLCLR  = gpio_mask;
            om_gpio->INTBOTHCLR = gpio_mask;
            om_gpio->INTENSET   = gpio_mask;
            break;
        case GPIO_TRIG_RISING_EDGE:
            om_gpio->INTTYPESET = gpio_mask;
            om_gpio->INTPOLSET  = gpio_mask;
            om_gpio->INTBOTHCLR = gpio_mask;
            om_gpio->INTENSET   = gpio_mask;
            break;
        case GPIO_TRIG_RISING_FAILING_EDGE:
            om_gpio->INTTYPESET = gpio_mask;
            om_gpio->INTPOLCLR  = gpio_mask;
            om_gpio->INTBOTHSET = gpio_mask;
            om_gpio->INTENSET   = gpio_mask;
            break;
        case GPIO_TRIG_LOW_LEVEL:
            om_gpio->INTTYPECLR = gpio_mask;
            om_gpio->INTPOLCLR  = gpio_mask;
            om_gpio->INTBOTHCLR = gpio_mask;
            om_gpio->INTENSET   = gpio_mask;
            break;
        case GPIO_TRIG_HIGH_LEVEL:
            om_gpio->INTTYPECLR = gpio_mask;
            om_gpio->INTPOLSET  = gpio_mask;
            om_gpio->INTBOTHCLR = gpio_mask;
            om_gpio->INTENSET   = gpio_mask;
            break;
        default:
            OM_ASSERT(0U);
            break;
    }
}

gpio_trig_type_t drv_gpio_get_trig(OM_GPIO_Type *om_gpio, gpio_mask_t gpio_mask)
{
    if (om_gpio->INTENSET & gpio_mask) {
        if ((om_gpio->INTTYPESET & gpio_mask) && (om_gpio->INTPOLSET & gpio_mask)) {
            return (om_gpio->INTBOTHSET & gpio_mask) ? GPIO_TRIG_RISING_FAILING_EDGE : GPIO_TRIG_RISING_EDGE;
        } else if (((om_gpio->INTTYPESET & gpio_mask) == 0) && (om_gpio->INTPOLSET & gpio_mask)) {
            return GPIO_TRIG_HIGH_LEVEL;
        } else if (((om_gpio->INTPOLSET & gpio_mask) == 0) && (om_gpio->INTTYPESET & gpio_mask)) {
            return (om_gpio->INTBOTHSET & gpio_mask) ? GPIO_TRIG_RISING_FAILING_EDGE : GPIO_TRIG_FALLING_EDGE;
        } else if (((om_gpio->INTPOLSET & gpio_mask) == 0) && ((om_gpio->INTTYPESET & gpio_mask) == 0)) {
            return GPIO_TRIG_LOW_LEVEL;
        }
    }

    return GPIO_TRIG_NONE;
}

void drv_gpio_uninit(OM_GPIO_Type *om_gpio)
{
    if (!gpio_is_using()) {
        DRV_RCC_CLOCK_ENABLE(RCC_CLK_GPIO0, 0U);
    }
}

void drv_gpio_isr(void)
{
    for (uint32_t i=0; i<sizeof(gpio_resources)/sizeof(gpio_resources[0]); i++) {
        uint32_t int_status;
        OM_GPIO_Type *om_gpio;

        om_gpio = (OM_GPIO_Type *)(gpio_resources[i]->reg);
        int_status = om_gpio->INTSTATUS;
        if (int_status) {
            om_gpio->INTSTATUS = int_status;
            drv_gpio_isr_callback(om_gpio, DRV_EVENT_COMMON_GENERAL, int_status, om_gpio->DATA);
        }
    }

    // disable wakeup irq event
    drv_pmu_pin_wakeup_out_of_date();
}

#endif  /* (RTE_GPIO0 || RTE_GPIO1) */


/** @} */
