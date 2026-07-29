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


#ifndef __DRV_DWT_H
#define __DRV_DWT_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include "om_device.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
extern uint32_t SystemCoreClock;
/// Calculation of ceil value for cycles
#define DRV_DWT_US_2_CYCLES_CEIL(us)     ((SystemCoreClock + 999999U)/ 1000U / 1000U * (us))
/// Calculation of round value for cycles
#define DRV_DWT_US_2_CYCLES_ROUND(us)    ((SystemCoreClock + 500000U) / 1000U / 1000U * (us))
/// Calculation of floor value for cycles
#define DRV_DWT_US_2_CYCLES_FLOOR(us)    (SystemCoreClock / 1000U / 1000U * (us))

/// Calculation of ceil value for cycles
#define DRV_DWT_MS_2_CYCLES_CEIL(ms )    ((SystemCoreClock + 999U)/ 1000U * (ms))
/// Calculation of round value for cycles
#define DRV_DWT_MS_2_CYCLES_ROUND(ms)    ((SystemCoreClock + 500U)/ 1000U * (ms))
/// Calculation of floor value for cycles
#define DRV_DWT_MS_2_CYCLES_FLOOR(ms)    (SystemCoreClock / 1000U * (ms))

#define DRV_DWT_CYCLES_2_US(cycles)      ((cycles) / (SystemCoreClock / 1000000U))

/**
 *******************************************************************************
 * @brief  continus check wait_val using dwt,
 *
 * @param  wait_val wait value
 * @param  to_ms timeout value in millisecond, 0 in case of return immediately with timeout status
 * @param  ret return value, it will return OM_ERROR_OK in to_ms, else return OM_ERROR_TIMEOUT
 *******************************************************************************
 */
#define DRV_DWT_WAIT_MS_UNTIL_TO(wait_val, to_ms, ret)                         \
    do {                                                                       \
        uint32_t to;                                                           \
        ret = OM_ERROR_OK;                                                     \
        to = (uint32_t)(to_ms);                                                \
        if (to != DRV_MAX_DELAY) {                                             \
            uint32_t start, target;                                            \
            start  = drv_dwt_get_cycle();                                      \
            target = DRV_DWT_MS_2_CYCLES_CEIL(to);                             \
            while (wait_val) {                                                 \
                if ((drv_dwt_get_cycle() - start) > target) {                  \
                    ret = OM_ERROR_TIMEOUT;                                    \
                    break;                                                     \
                }                                                              \
            }                                                                  \
        } else {                                                               \
            while (wait_val);                                                  \
        }                                                                      \
    } while(0)

/**
 *******************************************************************************
 * @brief  continus check wait_val using dwt,
 *
 * @param  wait_val wait value
 * @param  to_us timeout value in macrosecond, 0 in case of return immediately with timeout status
 * @param  ret return value, it will return OM_ERROR_OK in to_us, else return OM_ERROR_TIMEOUT
 *******************************************************************************
 */
#define DRV_DWT_WAIT_US_UNTIL_TO(wait_val, to_us, ret)                         \
    do {                                                                       \
        uint32_t to;                                                           \
        ret = OM_ERROR_OK;                                                     \
        to = (uint32_t)(to_us);                                                \
        if (to != DRV_MAX_DELAY) {                                             \
            uint32_t start, target;                                            \
            start  = drv_dwt_get_cycle();                                      \
            target = DRV_DWT_US_2_CYCLES_CEIL(to);                             \
            while (wait_val) {                                                 \
                if ((drv_dwt_get_cycle() - start) > target) {                  \
                    ret = OM_ERROR_TIMEOUT;                                    \
                    break;                                                     \
                }                                                              \
            }                                                                  \
        } else {                                                               \
            while (wait_val);                                                  \
        }                                                                      \
    } while(0)


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief delay some cycles using DWT timer
 *
 * @param cycles Delay cycles
 *******************************************************************************
 */
__STATIC_FORCEINLINE void drv_dwt_delay_cycles(uint32_t cycles)
{
    if (!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk)) {
        COREDEBUG->DEMCR |= COREDEBUG_DEMCR_TRCENA_MASK;
        // enable DWT peripheral, enable Cortex-M DWT Cycle counter
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }

    uint32_t start_cnt = DWT->CYCCNT;
    while ((uint32_t)(DWT->CYCCNT - start_cnt) <= cycles);
}

/**
 *******************************************************************************
 * @brief delay microseconds using DWT timer
 *
 * @param us            Delay microseconds, SystemCoreClock < 64MHz,
 *                      so us must be less than 2^32 / 64 = 2^(32-6) = 2^26
 *                      max 0x4000000/1000(ms)/1000(s) = 67.11s
 *******************************************************************************
 */
__STATIC_FORCEINLINE void drv_dwt_delay_us(uint32_t us)
{
    uint32_t cycles;

    cycles = DRV_DWT_US_2_CYCLES_CEIL(us);
    drv_dwt_delay_cycles(cycles);
}

/**
 *******************************************************************************
 * @brief delay milliseconds using DWT timer
 *
 * @param ms            Delay milliseconds, SystemCoreClock < 64MHz,
 *                      max delay time (2^32 / 64)/1000(ms)/1000(s) = 67.11s
 *******************************************************************************
 */
__STATIC_FORCEINLINE void drv_dwt_delay_ms(uint32_t ms)
{
    uint32_t cycles;

    cycles = DRV_DWT_MS_2_CYCLES_CEIL(ms);
    drv_dwt_delay_cycles(cycles);
}

/**
 *******************************************************************************
 * @brief Get current DWT cycle count
 *
 * @return Current DWT count
 *******************************************************************************
 */
__STATIC_FORCEINLINE uint32_t drv_dwt_get_cycle(void)
{
    return DWT->CYCCNT;
}

#ifdef __cplusplus
}
#endif

#endif  /* __DRV_DWT_H */

/** @} */
