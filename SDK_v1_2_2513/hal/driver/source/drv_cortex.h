/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup CORTEX CORTEX
 * @ingroup  DRIVER
 * @brief    CORTEX driver
 * @details  CORTEX driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_CORTEX_H
#define __DRV_CORTEX_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#include <stdint.h>
#include "om_device.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
/**
 * @brief disable global interrupt for enter critical code
 */
#define OM_CRITICAL_BEGIN()                     \
    do {                                        \
        /*lint -save -e578 */                   \
        uint32_t _primask = __get_PRIMASK();    \
        /*lint –restore */                      \
        __disable_irq();

/**
 * @brief  restore global interrupt for exit critical code
 */
#define OM_CRITICAL_END()                       \
        if (!_primask) {                        \
            __enable_irq();                     \
        }                                       \
    } while(0)

/**
 * @brief disable global interrupt for enter critical code
 */
#define OM_CRITICAL_BEGIN_EX(irq_save_is_disabled) \
    do {                                        \
        /*lint -save -e578 */                   \
        irq_save_is_disabled = __get_PRIMASK(); \
        /*lint –restore */                      \
        __disable_irq();                        \
    } while (0)

/**
 * @brief  restore global interrupt for exit critical code
 */
#define OM_CRITICAL_END_EX(irq_save_is_disabled) \
    do {                                        \
        if (!irq_save_is_disabled) {            \
            __enable_irq();                     \
        }                                       \
    } while(0)

/**
 * @brief disable global interrupt for enter critical code
 */
#define OM_ENTER_CRITICAL_EXCEPT(irqn)                          \
    do {                                                        \
        /*lint -save -e578 */                                   \
        uint32_t __irq_save = __get_BASEPRI();                  \
        __set_BASEPRI_MAX((irqn) << (8U - __NVIC_PRIO_BITS));   \
        /*lint –restore */                                      \

/**
 * @brief  restore global interrupt for exit critical code
 */
#define OM_EXIT_CRITICAL_EXCEPT()                               \
        __set_BASEPRI(__irq_save);                              \
    } while(0)

/**
 * @brief disable global interrupt for enter critical code
 */
#define OM_ENTER_CRITICAL_EXCEPT_EX(irqn, irq_save)             \
    do {                                                        \
        /*lint -save -e578 */                                   \
        irq_save = __get_BASEPRI();                             \
        __set_BASEPRI_MAX((irqn) << (8U - __NVIC_PRIO_BITS));   \
        /*lint –restore */                                      \
    } while (0)

/**
 * @brief  restore global interrupt for exit critical code
 */
#define OM_EXIT_CRITICAL_EXCEPT_EX(irq_save)                    \
    do {                                                        \
        __set_BASEPRI(irq_save);                                \
    } while(0)


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief  drv irq is ext pending
 *
 * @return pending ?
 *******************************************************************************
 */
__STATIC_FORCEINLINE bool drv_irq_is_any_ext_pending(void)
{
    uint32_t nvic_reg_num = EXTERNAL_IRQn_Num/32 + ((EXTERNAL_IRQn_Num%32) ? 1 : 0) ;
    uint32_t i;

    // check systick pending
    if ((SCB->ICSR & SCB_ICSR_PENDSTSET_Msk)
            && (SysTick->CTRL & SysTick_CTRL_ENABLE_Msk) && (SysTick->CTRL & SysTick_CTRL_TICKINT_Msk)) {
        return true;
    }
    // check external irq pending
    for (i=0; i<nvic_reg_num; ++i) {
        // NVIC_GetPendingIRQ & NVIC_GetEnableIRQ
        uint32_t ispr, iser;
        ispr = NVIC->ISPR[i];
        iser = NVIC->ISER[i];
        if (ispr & iser) {
            return true;
        }
    }

    return false;
}

#ifdef __cplusplus
}
#endif

#endif  /* __DRV_CORTEX_H */

/** @} */
