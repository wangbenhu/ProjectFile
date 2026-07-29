/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup GPIO GPIO
 * @ingroup  DRIVER
 * @brief    GPIO driver
 * @details  GPIO driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_GPIO_H
#define __DRV_GPIO_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_GPIO0 || RTE_GPIO1)
#include <stdint.h>
#include "om_driver.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
#define GPIO_MASK(gpio_idx)       (1U << (gpio_idx))
#define GPIO_LEVEL_LOW            (0)
#define GPIO_LEVEL_HIGH           (1)


/*******************************************************************************
 * TYPEDEFS
 */
/// GPIO Mask: bit field for gpio, 1 indicates valid, 0 indicates invalid
typedef uint32_t gpio_mask_t;

/// GPIO Direction
typedef enum {
    /// GPIO output direction
    GPIO_DIR_OUTPUT   = 0U,
    /// GPIO input direction
    GPIO_DIR_INPUT    = 1U,
} gpio_dir_t;

/// GPIO Interrupt Trigger Type
typedef enum {
    /// GPIO trigger type is falling edge
    GPIO_TRIG_FALLING_EDGE        = 0x0CU,
    /// GPIO trigger type is rising edge
    GPIO_TRIG_RISING_EDGE         = 0x03U,
    /// GPIO trigger type  is both edge
    GPIO_TRIG_RISING_FAILING_EDGE = 0x06U,
    /// GPIO trigger type is low level
    GPIO_TRIG_LOW_LEVEL           = 0x00U,
    /// GPIO trigger type is high level
    GPIO_TRIG_HIGH_LEVEL          = 0x0FU,
    /// GPIO trigger type is none
    GPIO_TRIG_NONE                = 0x10U,
} gpio_trig_type_t;

/// GPIO Configuration
typedef struct {
    /// Pointer to OM_GPIOx
    OM_GPIO_Type          *om_gpio;
    /// GPIO index in a GPIO port, range in [0, 38]
    uint8_t                gpio_idx;
    /// GPIO direction
    gpio_dir_t             dir;
    /// Used when dir config as output, range in [0, 1]
    uint8_t                out_val;
    /// Used when dir config as input
    gpio_trig_type_t       trig_type;
} gpio_config_t;

/// GPIO Control
typedef enum {
    GPIO_CONTROL_CLEAR_INT         = 0U,    /*!< Clear GPIO interrupt, argu is gpio mask, return OM_ERROR_OK */
} gpio_control_t;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Set GPIO direction
 *
 * @param[in] om_gpio        Pointer to GPIO
 * @param[in] gpio_mask      GPIO pin mask
 * @param[in] dir            GPIO direction
 *******************************************************************************
 */
__STATIC_FORCEINLINE void drv_gpio_set_dir(OM_GPIO_Type *om_gpio, gpio_mask_t gpio_mask, gpio_dir_t dir)
{
    if (dir == GPIO_DIR_OUTPUT) {
        om_gpio->OUTENSET = gpio_mask;
    } else {
        om_gpio->OUTENCLR = gpio_mask;
    }
}

/**
 *******************************************************************************
 * @brief Get GPIO direction
 *
 * @param[in] om_gpio        Pointer to GPIO
 * @param[in] gpio_mask      GPIO pin mask
 *
 * @return GPIO direction
 *******************************************************************************
 */
__STATIC_FORCEINLINE gpio_dir_t drv_gpio_get_dir(OM_GPIO_Type *om_gpio, gpio_mask_t gpio_mask)
{
    return (om_gpio->OUTENSET & gpio_mask) ? GPIO_DIR_OUTPUT : GPIO_DIR_INPUT;
}

/**
 *******************************************************************************
 * @brief Read GPIO pin input
 *
 * @param[in] om_gpio        Pointer to GPIO
 * @param[in] gpio_mask      GPIO pin mask
 *
 * @return GPIO pad value
 *******************************************************************************
 */
__STATIC_FORCEINLINE uint32_t drv_gpio_read(OM_GPIO_Type *om_gpio, gpio_mask_t gpio_mask)
{
    return (om_gpio->DATA & gpio_mask);
}

/**
 *******************************************************************************
 * @brief Write GPIO pin output
 *
 * @param[in] om_gpio        Pointer to GPIO
 * @param[in] gpio_mask      GPIO pin mask
 * @param[in] val            GPIO pin value
 *******************************************************************************
 */
__STATIC_FORCEINLINE void drv_gpio_write(OM_GPIO_Type *om_gpio, gpio_mask_t gpio_mask, uint8_t val)
{
    OM_CRITICAL_BEGIN();
    if (val) {
        om_gpio->DATAOUT |= gpio_mask;
    } else {
        om_gpio->DATAOUT &= ~gpio_mask;
    }
    OM_CRITICAL_END();
}

/**
 *******************************************************************************
 * @brief Toggle GPIO pin output
 *
 * @param[in] om_gpio        Pointer to GPIO
 * @param[in] gpio_mask      GPIO pin mask
 *******************************************************************************
 */
__STATIC_FORCEINLINE void drv_gpio_toggle(OM_GPIO_Type *om_gpio, gpio_mask_t gpio_mask)
{
    OM_CRITICAL_BEGIN();
    om_gpio->DATAOUT ^= gpio_mask;
    OM_CRITICAL_END();
}

/**
 *******************************************************************************
 * @brief Read GPIO port input
 *
 * @param[in] om_gpio        Pointer to GPIO
 *
 * @return GPIO port value
 *******************************************************************************
 */
__STATIC_FORCEINLINE uint32_t drv_gpio_port_read(OM_GPIO_Type *om_gpio)
{
    return om_gpio->DATA;
}

/**
 *******************************************************************************
 * @brief Write GPIO port output
 *
 * @param[in] om_gpio        Pointer to GPIO
 * @param[in] port_val       GPIO port value
 *******************************************************************************
 */
__STATIC_FORCEINLINE void drv_gpio_port_write(OM_GPIO_Type *om_gpio, uint32_t port_val)
{
    om_gpio->DATAOUT = port_val;
}

/**
 *******************************************************************************
 * @brief GPIO initialization
 *
 * @param[in] gpio_config      Configuration for GPIO
 * @param[in] gpio_config_num  Configuration number for GPIO
 *******************************************************************************
 */
extern void drv_gpio_init(const gpio_config_t *gpio_config, uint32_t gpio_config_num);

#if (RTE_GPIO_REGISTER_CALLBACK)
/**
 *******************************************************************************
 * @brief Register event callback for GPIO interrupt
 *
 * @param[in] om_gpio        Pointer to GPIO
 * @param[in] isr_cb         Pointer to callback
 *******************************************************************************
 */
extern void drv_gpio_register_isr_callback(OM_GPIO_Type *om_gpio, drv_isr_callback_t  isr_cb);
#endif

/**
 *******************************************************************************
 * @brief The interrupt callback for GPIO driver. It is a weak function. User should define
 *        their own callback in user file, other than modify it in the GPIO driver.
 *
 * @param om_gpio           The GPIO device address
 * @param event             The driver uart event
 *                           - DRV_EVENT_COMMON_GENERAL
 * @param int_status        The interrupt status
 * @param gpio_data         The gpio level data
 *******************************************************************************
 */
extern void drv_gpio_isr_callback(OM_GPIO_Type *om_gpio, drv_event_t event, uint32_t int_status, uint32_t gpio_data);

/**
 *******************************************************************************
 * @brief Set GPIO pin trigger type
 *
 * @param[in] om_gpio        Pointer to GPIO
 * @param[in] gpio_mask      GPIO pin mask, if bit field is 1, then set trigger
 * @param[in] trig           trig type
 *
 * @note When both sleep and GPIO input interrupts are enabled, the GPIO input interrupt must be configured as `GPIO_TRIG_RISING_FAILING_EDGE`.
 *       Because the 2nd edge is used to trigger pm system to detect the sleep state.
 *
 *******************************************************************************
 */
extern void drv_gpio_set_trig(OM_GPIO_Type *om_gpio, gpio_mask_t gpio_mask, gpio_trig_type_t trig);

/**
 *******************************************************************************
 * @brief Get GPIO pin trigger type
 *
 * @param[in] om_gpio        Pointer to GPIO
 * @param[in] gpio_mask      GPIO pad mask, if bit field is 1, then set trigger
 *
 * @return trig type
 *******************************************************************************
 */
extern gpio_trig_type_t drv_gpio_get_trig(OM_GPIO_Type *om_gpio, gpio_mask_t gpio_mask);

/**
 *******************************************************************************
 * @brief Get GPIO base from gpio idx
 *
 * @param idx  Index of GPIO peripheral
 *
 * @return OM_GPIO Type pointer
 *******************************************************************************
 */
static inline OM_GPIO_Type* drv_gpio_idx2base(uint8_t idx)
{
    OM_GPIO_Type *const gpio[] = {
        #if (RTE_GPIO0)
        OM_GPIO0,
        #else
        NULL,
        #endif
        #if (RTE_GPIO1)
        OM_GPIO1,
        #else
        NULL,
        #endif
    };

    return (idx < sizeof(gpio)/sizeof(gpio[0])) ? gpio[idx] : NULL;
}

/**
 *******************************************************************************
 * @brief Control GPIO interface.
 *
 * @param[in] om_gpio        Pointer to GPIO port, Donot GPIO_CONTROL_CLK_DISABLE if using sleep
 * @param[in] control        Operation
 * @param[in] argu           Not used, always set NULL
 *
 * @return                   Control status, always return OM_ERROR_OK
 *******************************************************************************
 */
static inline void *drv_gpio_control(OM_GPIO_Type *om_gpio, gpio_control_t control, void *argu)
{
    switch (control) {
        case GPIO_CONTROL_CLEAR_INT:
            om_gpio->INTSTATUS = (uint32_t)argu;
            break;
        default:
            break;
    }

    return (void *)OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Control GPIO interface.
 *
 * @param[in] om_gpio        Pointer to GPIO port
 *******************************************************************************
 */
extern void drv_gpio_uninit(OM_GPIO_Type *om_gpio);

/**
 *******************************************************************************
 * @brief gpio interrupt service routine
 *******************************************************************************
 */
extern void drv_gpio_isr(void);

#ifdef __cplusplus
}
#endif

#endif  /* (RTE_GPIO0 || RTE_GPIO1) */

#endif  /* __DRV_GPIO_H */


/** @} */
