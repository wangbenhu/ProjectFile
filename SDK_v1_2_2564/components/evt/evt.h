/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup EVT EVT
 * @ingroup  COMPONENTS
 * @brief    event
 * @details  event
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __EVT_H
#define __EVT_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
// #include "evt_timer.h"

#ifdef  __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
#define CONFIG_EVT_RTOS_SUPPORT


/*******************************************************************************
 * TYPEDEFS
 */
/// Format of an event callback function
typedef void (*evt_callback_t)(void);

/// event type
typedef enum {
    EVT_TYPE_RESERVE,                       ///< Reserved
    EVT_TYPE_BLE,                           ///< Event type for BLE
    EVT_TYPE_TIMER,                         ///< Event type for Timer
    EVT_TYPE_SHELL,                         ///< Event type for Shell

    EVT_TYPE_USR_FIRST = 16,                ///< Event type user first
	EVT_TYPE_BLE_OP = EVT_TYPE_USR_FIRST,	///< Event type user ble
    EVT_TYPE_USR_LAST = 31,                 ///< Event type user last

    EVT_TYPE_NUM,                           ///< Event number
} evt_type_t;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Initialize Kernel event module.
 *******************************************************************************
 */
extern void evt_init(void);

/**
 *******************************************************************************
 * @brief Register an event callback.
 *
 * @param[in]  evt_type       Event type.
 * @param[in]  callback         Pointer to callback function.
 *******************************************************************************
 */
extern void evt_callback_set(evt_type_t evt_type, evt_callback_t callback);

/**
 *******************************************************************************
 * @brief Get an event callback.
 *
 * @param[in]  evt_type       Event type.
 *
 * @return                      callback
 *******************************************************************************
 */
extern evt_callback_t evt_callback_get(evt_type_t evt_type);

#ifdef CONFIG_EVT_RTOS_SUPPORT
/**
 *******************************************************************************
 * @brief Register an event set callback.
 *
 * @param[in]  callback         Pointer to callback function.
 *******************************************************************************
 */
extern void evt_schedule_trigger_callback_set(evt_callback_t callback);
#endif

/**
 *******************************************************************************
 * @brief Set an event
 *
 * This primitive sets one event. It will trigger the call to the corresponding event
 * handler in the next scheduling call.
 *
 * @param[in]  evt_type      Event to be set.
 *******************************************************************************
 */
extern void evt_set(evt_type_t evt_type);

/**
 *******************************************************************************
 * @brief Clear an event
 *
 * @param[in]  evt_type      Event to be cleared.
 *******************************************************************************
 */
extern void evt_clear(evt_type_t evt_type);

/**
 *******************************************************************************
 * @brief Get the status of an event
 *
 * @param[in]  evt_type      Event to get.
 *
 * @return                     Event status (0: not set / 1: set)
 *******************************************************************************
 */
extern uint8_t evt_get(evt_type_t evt_type);

/**
 *******************************************************************************
 * @brief Get all event status
 *
 * @return                     Events bit field
 *******************************************************************************
 */
extern uint32_t evt_get_all(void);

/**
 *******************************************************************************
 * @brief Flush all pending events.
 *******************************************************************************
 */
extern void evt_flush(void);

/**
 *******************************************************************************
 * @brief Event scheduler entry point.
 *
 * This primitive is the entry point of Kernel event scheduling.
 *******************************************************************************
 */
extern void evt_schedule(void);

/**
 *******************************************************************************
 * @brief Event scheduler once entry point.
 *
 * This primitive is the entry point of Kernel event scheduling.
 *******************************************************************************
 */
extern uint32_t evt_schedule_once(void);

#ifdef  __cplusplus
}
#endif

#endif  /* __EVT_H */

/** @} */
