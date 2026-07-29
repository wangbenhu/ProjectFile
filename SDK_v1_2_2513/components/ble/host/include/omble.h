/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @addtogroup OMBLE_COMMON BLE_COMMON
 * @brief OMBLE initialization interface
 * @version
 * Version 1.0
 *  - Initial release
 *
 */
/// @{
#ifndef __OMBLE_API_H__
#define __OMBLE_API_H__
#include <stdint.h>
#include <stdlib.h>
#include "omble_gap.h"
#include "omble_gatt.h"
#include "omble_misc.h"
#include "omble_error.h"

/// BLE Event structure
typedef struct {
    union {
        omble_gap_evt_t  gap;  ///< gap event，refer to @ref omble_gap_evt_t
        omble_gatt_evt_t gatt; ///< gatt event，refer to @ref omble_gatt_evt_t
    };
} omble_evt_t;

/// Protocol stack initialization parameters
struct ob_stack_param {
    uint8_t max_connection;        ///< Maximum number of connections
    uint8_t max_ext_adv_set;       ///< Maximum number of advertising
    uint16_t max_att_mtu;          ///< MAX ATT MTU
    uint16_t max_gatt_serv_num;    ///< MAX GATT service number
    uint16_t max_gatt_write_cache; ///< MAX GATT server prepare write cache length, Use MAX mtu if set to 0
    bool smp_sc_support;           ///< Secure Connection Pairing support [Deprecated]
    uint8_t max_user_event_num;    ///< MAX user event number
    uint8_t max_gatt_buffer_num;   ///< Maximum number of data buffers per GATT connection (1~63, default 63)
    uint8_t max_period_adv_set;    ///< Maximum number of periodic advertising
    uint8_t max_period_adv_sync;   ///< Maximum number of periodic advertising synchronizations
};

/**@brief BLE Event callback type
 * @note All Bluetooth related messages are notified to the upper layer application through this structure
 * @param[in] evt_id  event id，refer to @ref OB_GAP_EVENTS, @ref OB_GATT_EVENTS
 * @param[in] evt  Event Structure，refer to @ref omble_evt_t
 */
typedef void (*omble_event_cb_t)(uint16_t evt_id, const omble_evt_t *evt);

/**@brief Register BLE event callback function
 * @note The callback can be registered multiple times, and each callback function is called in the order of registration.
 * @param[in]  callback   Callback function, refer to @ref omble_event_cb_t
 * @return result，refer to @ref ob_error
 */
uint32_t ob_event_callback_reg(omble_event_cb_t callback);

/**@brief Unregister BLE event callback function
 * @param[in]  callback   callback，refer to @ref omble_event_cb_t
 * @return result，refer to @ref ob_error
 */
uint32_t ob_event_callback_unreg(omble_event_cb_t callback);

/**@brief Abort BLE event processing
 *
 * After calling @ref ob_event_callback_reg multiple times to register BLE event callbacks, other registered callback functions will no longer be called after calling this function
 * @return result，refer to @ref ob_error
 */
uint32_t ob_event_abort(void);

/**@brief optional parameter configuration
 * @param[in]  type  parameter type，refer to @ref ob_opt_cfg_type
 * @param[in]  cfg   parameter data，refer to @ref ob_opt_cfg_t
 * @return result，refer to @ref ob_error
 */
uint32_t ob_opt_cfg_set(uint8_t type, ob_opt_cfg_t *cfg);

/**@brief Get optional parameters
 * @param[in]  type  parameter type，refer to @ref ob_opt_cfg_type
 * @param[out] cfg   parameter data，refer to @ref ob_opt_cfg_t
 * @return result，refer to @ref ob_error
 */
uint32_t ob_opt_cfg_get(uint8_t type, ob_opt_cfg_t *cfg);

/**@brief Initialize the BLE protocol stack
 * @param[in]  param  Protocol stack initialization parameters，refer to @ref ob_stack_param
 * @note This function can only be called once and must be called before calling other Bluetooth interfaces.
 * @return result，refer to @ref ob_error
 */
uint32_t omble_init(struct ob_stack_param *param);

/**@brief Set the user event callback function
 * @param[in]  event_id  Event ID. The id value should be less than the max_user_event_num value in @ref ob_stack_param
 * @param[in]  callback  BLE Protocol stack initialization parameters, refer to @ref ob_stack_param
 * @return result，refer to @ref ob_error
 */
uint32_t omble_set_user_event_callback(uint8_t event_id, void(*callback)(void));

/**@brief Trigger user event callback function
 * @param[in]  event_id  Event ID. The id value should be less than the max_user_event_num value in @ref ob_stack_param
 * @return result，refer to @ref ob_error
 */
uint32_t omble_user_event_trigger(uint8_t event_id);

/**@brief Get Time Tick
 * @return tick
 */
size_t omble_tick_get(void);

/**@brief Calculate the time interval between tick2 and tick1. Unit in 10ms
 * @param[in]  tick1  tick1
 * @param[in]  tick2  tick2
 * @return tick2 - the time interval of tick1, in 10ms
 */
int omble_tick_diff_10ms(size_t tick1, size_t tick2);

/**@brief Get the current memory usage of the BLE host
 */
int ob_mem_use_current(void);

/**@brief Get the maximum memory usage of the BLE host
 */
int ob_mem_use_high(void);

/**@brief Scheduling BLE protocol stack
 */
void omble_schedule(void);

#endif /* __OMBLE_API_H__ */
/// @}
