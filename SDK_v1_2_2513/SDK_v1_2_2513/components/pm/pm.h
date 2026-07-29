/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup PM PM
 * @ingroup  COMPONENTS
 * @brief    power manager for system
 * @details  power manager for system
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


#ifndef __PM_H
#define __PM_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdbool.h>
#include <stdint.h>
#include "om_error.h"


#ifdef  __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * TYPEDEFS
 */
/// PM ID defined
typedef enum {
    PM_ID_BLE,              ///< BLE
    PM_ID_24G,              ///< 2.4G
    PM_ID_USB,              ///< USB
    PM_ID_USER,             ///< USER

    PM_ID_NUM = 32U,        ///< Number of PM ID
} pm_id_t;

/// PM checker priority
typedef enum {
    PM_CHECKER_PRIORITY_LOWEST   = 0,     ///< Lowest
    PM_CHECKER_PRIORITY_LOW      = 1,     ///< Low
    PM_CHECKER_PRIORITY_NORMAL   = 2,     ///< Normal
    PM_CHECKER_PRIORITY_HIGH     = 3,     ///< High
    PM_CHECKER_PRIORITY_HIGHEST  = 4,     ///< Highest
} pm_checker_priority_t;

/// PM status
typedef enum {
    /// All modules are alive, and CPU run with full speed
    PM_STATUS_ACTIVE,
    /// All modules are alive, but CPU clock is gating
    PM_STATUS_IDLE,
    /// Power down most of module(CPU, Peripheral, etc),
    /// but 32K is alive, only gpio and sleep-timer can wake up chip
    PM_STATUS_SLEEP,
    /// Power down most of module(CPU, Peripheral, etc),
    /// If 32K is not alive, only gpio can wake up chip
    PM_STATUS_DEEP_SLEEP,
} pm_status_t;

/// Sleep state
typedef enum {
    /// Event with sleep entering.
    /// The output of IO can be controlled.
    PM_SLEEP_ENTRY,
    /// Event with sleep leaving, CHIP is working on 'High Speed Internal Clock (RC32M)'.
    /// The output of IO can not be controlled.
    PM_SLEEP_LEAVE_TOP_HALF_HSI,
    /// Event with sleep leaving, CHIP is working on 'High Speed External Clock (XTAL32M)'.
    /// The output of IO can not be controlled.
    PM_SLEEP_LEAVE_TOP_HALF,
    /// Event with sleep leaving, CHIP is working on 'High Speed External Clock (XTAL32M)'.
    /// The output of IO can be controlled.
    PM_SLEEP_LEAVE_BOTTOM_HALF,

    /// Alias for PM_SLEEP_ENTRY, just for driver REG store
    PM_SLEEP_STORE = PM_SLEEP_ENTRY,
    /// Alias for PM_SLEEP_LEAVE_TOP_HALF_HSI, just for driver REG restore
    PM_SLEEP_RESTORE_HSI = PM_SLEEP_LEAVE_TOP_HALF_HSI,
    /// Alias for PM_SLEEP_LEAVE_TOP_HALF, just for driver REG restore
    PM_SLEEP_RESTORE_HSE = PM_SLEEP_LEAVE_TOP_HALF,
} pm_sleep_state_t;

/**
 *******************************************************************************
 * @brief  pm checker callback t
 *
 * @return @ref pm_status_t
 *******************************************************************************
 **/
typedef pm_status_t (*pm_checker_callback_t) (void);

/**
 *******************************************************************************
 * @brief sleep state event callback
 *
 * @param sleep_state  current sleep state @ref pm_sleep_state_t
 * @param power_status  power status, only PM_STATUS_SLEEP and PM_STATUS_DEEP_SLEEP are valid @ref pm_status_t
 *******************************************************************************
 **/
typedef void (*pm_sleep_callback_t)(pm_sleep_state_t sleep_state, pm_status_t power_status);


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
#if (CONFIG_PM)
/**
 *******************************************************************************
 * @brief  pm init
 *******************************************************************************
 */
extern void pm_init(void);

/**
 * @brief  system sleep
 *
 * @param[in] status  status
 **/
extern void pm_sleep(pm_status_t status);

/**
 *******************************************************************************
 * @brief  pm sleep checker check
 *
 * @return status
 *******************************************************************************
 **/
extern pm_status_t pm_sleep_checker_check(void);

/**
 *******************************************************************************
 * @brief  system sleep min time set
 *
 * @param[in] tick_32k  32k tick
 *******************************************************************************
 **/
extern void pm_sleep_min_time_set(uint16_t tick_32k);

/**
 *******************************************************************************
 * @brief  pm sleep min time get
 *
 * @return  32k tick
 *******************************************************************************
 */
extern uint16_t pm_sleep_min_time_get(void);

/**
 *******************************************************************************
 * @brief  pm sleep ultra sleep mode enable
 *
 * @param[in] enable  enable
 *******************************************************************************
 */
extern void pm_sleep_ultra_sleep_mode_enable(bool enable);

/**
 *******************************************************************************
 * @brief  pm sleep ultra sleep mode enable
 *
 * @return  is enabled
 *******************************************************************************
 */
extern bool pm_sleep_ultra_sleep_mode_is_enabled(void);

/**
 *******************************************************************************
 * @brief  pm sleep enable or disable
 *
 * @param enable  enable
 *******************************************************************************
 **/
extern void pm_sleep_enable(bool enable);

/**
 *******************************************************************************
 * @brief  system sleep notify user callback register
 *
 * @param notify_cb  sleep notify cb
 *******************************************************************************
 **/
extern void pm_sleep_notify_user_callback_register(pm_sleep_callback_t notify_cb);

/**
 *******************************************************************************
 * @brief  pm power manage, it shall be called in critical section(disable global interrupt)
 *******************************************************************************
 **/
extern void pm_power_manage(void);

#endif  /* CONFIG_PM */

/**
 *******************************************************************************
 * @brief  pm sleep checker register
 *
 * @param priority    priority
 * @param checker_cb  checker cb
 *
 * @return  register status
 * -OM_ERROR_OK            register success
 * -OM_ERROR_OUT_OF_RANGE  register failed
 *******************************************************************************
 **/
extern om_error_t pm_sleep_checker_callback_register(pm_checker_priority_t priority, pm_checker_callback_t checker_cb);

/**
 *******************************************************************************
 * @brief  system sleep store restore callback register
 *
 * @param store_cb  sleep callback
 *
 * @return  register status
 * -OM_ERROR_OK   register success
 * -OM_ERROR_OUT_OF_RANGE register failed
 *******************************************************************************
 **/
extern om_error_t pm_sleep_store_restore_callback_register(pm_sleep_callback_t store_cb);

/**
 *******************************************************************************
 * @brief  pm sleep prevent
 *
 * @param id  id, reference @ref pm_id_t
 *******************************************************************************
 **/
extern void pm_sleep_prevent(pm_id_t id);

/**
 *******************************************************************************
 * @brief  pm sleep allow
 *
 * @param id  id, reference @ref pm_id_t
 *******************************************************************************
 **/
extern void pm_sleep_allow(pm_id_t id);

#ifdef  __cplusplus
}
#endif

#endif  /* __PM_H */


/** @} */
