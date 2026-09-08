/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup PMU_TIMER PMU_TIMER
 * @ingroup  DRIVER
 * @brief    PMU_TIMER driver
 * @details  PMU_TIMER driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_PMU_TIMER_H
#define __DRV_PMU_TIMER_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_PMU_TIMER)
#include "om_driver.h"
#include "om_device.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
/// Max Tick
#define PMU_TIMER_MAX_TICK                          0xFFFFFFFFU
/// Convert ms to tick
#define PMU_TIMER_MS2TICK(time)                     ((uint32_t)((((uint64_t)(time)) << 12) / 125))
/// Convert us to tick
#define PMU_TIMER_US2TICK(time)                     ((uint32_t)((((uint64_t)(time)) << 9) / 15625))
/// Convert tick to ms
#define PMU_TIMER_TICK2MS(tick)                     ((uint32_t)((((uint64_t)(tick)) * 125) >> 12))
// Convert tick to us
#define PMU_TIMER_TICK2US(tick)                     ((uint32_t)((((uint64_t)(tick)) * 15625) >> 9))

//#define PMU_TIMER_DEBUG


/*******************************************************************************
 * TYPEDEFS
 */
/// Trigger Source
typedef enum {
    /// Trigger for VAL0
    PMU_TIMER_TRIG_VAL0,
    /// Trigger for VAL1
    PMU_TIMER_TRIG_VAL1,
    /// Trigger for none
    PMU_TIMER_TRIG_NONE,
} pmu_timer_trig_t;

/// PMU Timer Control
typedef enum {
    PMU_TIMER_CONTROL_ENABLE,                     /*!< Enable PMU timer, argu is 0: indicate disable; others indicate enable */
    PMU_TIMER_CONTROL_GET_TIMER_VAL,              /*!< Get PMU timer val */
    PMU_TIMER_CONTROL_SET_TIMER_VAL,              /*!< Set PMU timer val */
    PMU_TIMER_CONTROL_GET_OVERFLOW,               /*!< Get PMU timer interrupt flag, argu is NULL, return overflow */
    PMU_TIMER_CONTROL_SET_TIMER_INCR,             /*!< Set PMU timer ppm regsiter, argu is timer incr, return OM_ERROR_OK */
} pmu_timer_control_t;

/// PMU Timer Env
typedef struct {
    /// trigger callback for val0
    drv_isr_callback_t    isr0_cb;
    /// trigger callback for val1
    drv_isr_callback_t    isr1_cb;

    /// Workaround pmu timer write-delay-read issue for val0
    __IO uint32_t         TIMER_SET0;
    /// Workaround pmu timer write-delay-read issue for val1
    __IO uint32_t         TIMER_SET1;

#ifdef PMU_TIMER_DEBUG
    struct {
        __IO bool       is_enabled;
        __IO uint32_t   value;
        __IO uint32_t   cur_cnt_when_set;
        __IO uint32_t   cur_cnt_when_overflow;
    }record[2];
#endif
} pmu_timer_env_t;


/*******************************************************************************
 * EXTERN VARIABLES
 */
extern pmu_timer_env_t pmu_timer_env;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief initialize pmu_timer
 *******************************************************************************
 */
extern void drv_pmu_timer_init(void);

/**
 *******************************************************************************
 * @brief setting pmu timer trig
 *
 * @param trig_type    trigger type
 * @param trig_cnt     pmu timer trig count
 *******************************************************************************
 */
extern void drv_pmu_timer_trig_set(pmu_timer_trig_t trig_type, uint32_t trig_cnt);

/**
 *******************************************************************************
 * @brief get pmu timer left time(ms)
 *
 * @param[in] trig_type    trigger type
 *
 * @return
 *  - PMU_TIMER_MAX_TICK : invalid time
 *  - others             : left time(tick)
 *******************************************************************************
 */
extern uint32_t drv_pmu_timer_left_time_get(pmu_timer_trig_t trig_type);

/**
 *******************************************************************************
 * @brief pmu timer register callback
 *
 * @param[in] trig_type     trigger type
 * @param[in] cb            callback
 *******************************************************************************
 */
extern void drv_pmu_timer_register_isr_callback(pmu_timer_trig_t trig_type, drv_isr_callback_t cb);

/**
 *******************************************************************************
 * @brief The interrupt callback for PMU Timer driver. It is a weak function. User should define
 *        their own callback in user file, other than modify it in the PMU Timer driver.
 *
 * @param trig_type         Trigger type
 * @param timer_set         Timer set value
 *******************************************************************************
 */
extern void drv_pmu_timer_isr_callback(pmu_timer_trig_t trig_type, uint32_t timer_set);

/**
 *******************************************************************************
 * @brief pmu timer interrupt service routine
 *
 *******************************************************************************
 */
extern void drv_pmu_timer_isr(void);

/**
 *******************************************************************************
 * @brief calculate counter increment by pmu timer frequency
 *
 * @param[in] real_freq    real frequency
 *
 * @return incr
 *******************************************************************************
 */
__STATIC_FORCEINLINE uint32_t pmu_timer_calc_incr_by_real_freq(uint32_t real_freq)
{
    uint32_t old_incr, new_incr;
    uint64_t tmp_val;
    uint32_t tmp_val_lo, tmp_val_hi;

    old_incr = OM_PMU->TIMER_PPM & 0x1FFFF;

    tmp_val = (uint64_t)old_incr * (((uint64_t)32768 << 16) / real_freq);

    // binary point position is at [bit32 . bit31]
    tmp_val_lo = tmp_val & 0xFFFFFFFFU;
    tmp_val_hi = (tmp_val >> 32) & 0xFFFFFFFFU;

    new_incr = ((tmp_val_hi & 0x1) << 16) | (tmp_val_lo >> 16);

    OM_ASSERT(new_incr != 0U);

    return new_incr;
}

/**
 *******************************************************************************
 * @brief calculate counter increment by recording pmu timer's count value within time_us
 *
 * @param[in] time_us       record time
 * @param[in] pmu_tim_cnt   record count
 *
 * @return incr
 *******************************************************************************
 */
__STATIC_FORCEINLINE uint32_t pmu_timer_calc_incr_by_time(uint32_t time_us, uint32_t pmu_tim_cnt)
{
    return pmu_timer_calc_incr_by_real_freq((uint64_t)(pmu_tim_cnt) * 1000000 / time_us);
}

/**
 *******************************************************************************
 * @brief get pmu timer count
 *
 * @return current count
 *******************************************************************************
 */
__STATIC_FORCEINLINE uint32_t drv_pmu_timer_cnt_get(void)
{
    return OM_PMU->TIMER_READ;
}

/**
 *******************************************************************************
 * @brief pmu timer control
 *
 * @param[in] trig_type     trigger type
 * @param[in] control       control type
 * @param[in] argu          control content
 *
 * @return control status
 *******************************************************************************
 */
__STATIC_FORCEINLINE void *drv_pmu_timer_control(pmu_timer_trig_t trig_type, pmu_timer_control_t control, void *argu)
{
    OM_PMU_Type *pmu_timer = OM_PMU;

    switch (control) {
        case PMU_TIMER_CONTROL_ENABLE:
            do {
                uint32_t int_mask;
                int_mask = (trig_type == PMU_TIMER_TRIG_VAL0) ? PMU_TIMER_CTRL_PMU_TIMER_INT_VAL0_EN_MASK : PMU_TIMER_CTRL_PMU_TIMER_INT_VAL1_EN_MASK;
                OM_CRITICAL_BEGIN();
                register_set(&(pmu_timer->TIMER_CTRL), int_mask, ((uint32_t)argu) ? int_mask : 0);
                #if (PMU_TIMER_DEBUG)
                pmu_timer_env.record[trig_type].is_enabled = (uint32_t)argu ? true : false;
                #endif
                OM_CRITICAL_END();
            } while(0);
            break;
        case PMU_TIMER_CONTROL_GET_TIMER_VAL:
            if (trig_type == PMU_TIMER_TRIG_VAL0) {
                return (void *)pmu_timer_env.TIMER_SET0;
            } else if (trig_type == PMU_TIMER_TRIG_VAL1) {
                return (void *)pmu_timer_env.TIMER_SET1;
            }
            break;
        case PMU_TIMER_CONTROL_SET_TIMER_VAL:
            OM_CRITICAL_BEGIN();
            if (trig_type == PMU_TIMER_TRIG_VAL0) {
                pmu_timer->TIMER_SET0 = pmu_timer_env.TIMER_SET0 = (uint32_t)argu;
                REGW1(&pmu_timer->TIMER_CTRL, PMU_TIMER_CTRL_PMU_TIMER_INT_VAL0_EN_MASK);
            } else if (trig_type == PMU_TIMER_TRIG_VAL1) {
                pmu_timer->TIMER_SET1 = pmu_timer_env.TIMER_SET1 = (uint32_t)argu;
                REGW1(&pmu_timer->TIMER_CTRL, PMU_TIMER_CTRL_PMU_TIMER_INT_VAL1_EN_MASK);
            }
#ifdef PMU_TIMER_DEBUG
            pmu_timer_env.record[trig_type].is_enabled = true;
            pmu_timer_env.record[trig_type].cur_cnt_when_set = drv_pmu_timer_cnt_get();
            pmu_timer_env.record[trig_type].cur_cnt_when_overflow = 0;
            pmu_timer_env.record[trig_type].value = trig_type==PMU_TIMER_TRIG_VAL0 ? pmu_timer_env.TIMER_SET0 : pmu_timer_env.TIMER_SET1;
#endif
            OM_CRITICAL_END();
            break;
        case PMU_TIMER_CONTROL_GET_OVERFLOW:
            if (trig_type == PMU_TIMER_TRIG_VAL0) {
                return (void *)(pmu_timer->TIMER_CTRL & PMU_TIMER_CTRL_PMU_TIMER_INT0_MASK);
            } else if (trig_type == PMU_TIMER_TRIG_VAL1) {
                return (void *)(pmu_timer->TIMER_CTRL & PMU_TIMER_CTRL_PMU_TIMER_INT1_MASK);
            }
            break;
        case PMU_TIMER_CONTROL_SET_TIMER_INCR:
            pmu_timer->TIMER_PPM = (uint32_t)argu;
            break;
        default:
            break;
    }

    return (void *)((uint32_t)OM_ERROR_OK);
}


#ifdef __cplusplus
}
#endif

#endif  /* RTE_PMU_TIMER */

#endif  /* __DRV_PMU_TIMER_H */


/** @} */
