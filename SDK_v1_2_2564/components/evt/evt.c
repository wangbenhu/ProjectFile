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
 * @brief    event
 * @details  event
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
#include "om_common.h"
#include "om_driver.h"
#include "evt.h"


/*******************************************************************************
 * TYPEDEFS
 */
/// environment structure
typedef struct {
    /// Event field
    volatile uint32_t field;

    /// Callback table
    evt_callback_t callback[EVT_TYPE_NUM];

    #ifdef CONFIG_EVT_RTOS_SUPPORT
    /// callback for evt_set
    evt_callback_t schedule_trigger_callback;
    #endif
} evt_env_t;


/*******************************************************************************
 * GLOBAL VARIABLES
 */
/// environment
static evt_env_t evt_env;


/*******************************************************************************
 * LOCAL FUNCTION DEFINITIONS
 */
__RAM_CODES("PM")
static uint32_t evt_ctz(uint32_t x)
{
    #ifdef __RBIT // armv7+
    return __CLZ(__RBIT(x));
    #else
    int c = __CLZ(x & -x);      //lint !e501 use x&-x to get the lowest bit 1, and clear other bits
    return x ? 31 - c : c;
    #endif
}


/*******************************************************************************
 * EXPORTED FUNCTION DEFINITIONS
 */
void evt_init(void)
{
    memset(&evt_env, 0, sizeof(evt_env));
}

/**
 *******************************************************************************
 * @brief  evt callback set
 *
 * @param[in] evt_type  event type
 * @param[in] callback  callback
 *
 * @return
 *******************************************************************************
 **/
void evt_callback_set(evt_type_t evt_type, evt_callback_t callback)
{
    OM_ASSERT(evt_type < EVT_TYPE_NUM);

    evt_env.callback[evt_type] = callback;
}

/**
 *******************************************************************************
 * @brief  evt callback get
 *
 * @param[in] evt_type  event type
 *******************************************************************************
 **/
evt_callback_t evt_callback_get(evt_type_t evt_type)
{
    return (evt_env.callback[evt_type]);
}

#ifdef CONFIG_EVT_RTOS_SUPPORT
/**
 *******************************************************************************
 * @brief  evt schedule trigger callback set
 *
 * @param[in] callback  callback
 *******************************************************************************
 **/
void evt_schedule_trigger_callback_set(evt_callback_t callback)
{
    /// callback for evt_set
    evt_env.schedule_trigger_callback = callback;
}
#endif

__RAM_CODES("PM")
void evt_set(evt_type_t evt_type)
{
    OM_ASSERT(evt_type < EVT_TYPE_NUM);

    OM_CRITICAL_BEGIN();
    evt_env.field |= (1 << evt_type);
    OM_CRITICAL_END();

    #ifdef CONFIG_EVT_RTOS_SUPPORT
    if (evt_env.schedule_trigger_callback) {
        evt_env.schedule_trigger_callback();
    }
    #endif
}

__RAM_CODES("PM")
void evt_clear(evt_type_t evt_type)
{
    OM_ASSERT(evt_type < EVT_TYPE_NUM);

    OM_CRITICAL_BEGIN();
    evt_env.field &= ~(1u << evt_type);
    OM_CRITICAL_END();
}

uint8_t evt_get(evt_type_t evt_type)
{
    uint8_t state;

    OM_ASSERT(evt_type < EVT_TYPE_NUM);

    OM_CRITICAL_BEGIN();
    state = (evt_env.field >> evt_type) & 1;
    OM_CRITICAL_END();

    return state;
}

__RAM_CODES("PM")
uint32_t evt_get_all(void)
{
    return evt_env.field;
}

void evt_flush(void)
{
    evt_env.field = 0;
}

/**
 *******************************************************************************
 * @brief  evt schedule
 *******************************************************************************
 **/
__RAM_CODES("PM")
void evt_schedule(void)
{
    // Get the volatile value
    uint32_t field = evt_env.field;

    while (field) { // Compiler is assumed to optimize with loop inversion
        // Find highest priority event set
        uint32_t hdl = evt_ctz(field);

        if(evt_env.callback[hdl] != NULL) {
            // Execute corresponding handler
//            TRC_IO(TRC_IO_EVT_EXEC_CB, 1);
            (evt_env.callback[hdl])();
//            TRC_IO(TRC_IO_EVT_EXEC_CB, 0);
        } else {
            OM_ASSERT(0);
        }

        // Update the volatile value
        field = evt_env.field;
    }
}

/**
 *******************************************************************************
 * @brief  evt schedule once
 *
 * @return  not schedule event
 *******************************************************************************
 **/
uint32_t evt_schedule_once(void)
{
    // Get the volatile value
    uint32_t field = evt_env.field;

    if (field) { // Compiler is assumed to optimize with loop inversion
        // Find highest priority event set
        uint8_t hdl = evt_ctz(field);

        if(evt_env.callback[hdl] != NULL) {
            // Execute corresponding handler
//            TRC_IO(TRC_IO_EVT_EXEC_CB, 1);
            (evt_env.callback[hdl])();
//            TRC_IO(TRC_IO_EVT_EXEC_CB, 0);
        } else {
            OM_ASSERT(0);
        }

        // Update the volatile value
        field = evt_env.field;
    }

    return field;
}

/** @} */
