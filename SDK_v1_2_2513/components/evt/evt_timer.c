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
 * @brief    event timer
 * @details  event timer
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
#include "om_driver.h"
#include "evt.h"
#include "evt_timer.h"
#include "om_common.h"
//#include "trc_io.h"


/*******************************************************************************
 * MACROS
 */
#define EVT_TIMER_RESOLUTION            1
#define EVT_TIMER_START_REDUNDANCE      5

#define PL1ST (evt_timer_env.pending_list.head)


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    om_list_t pending_list;
    om_list_t pasted_list;
} evt_timer_env_t;


/*******************************************************************************
 * LOCAL VARIABLES
 */
static evt_timer_env_t evt_timer_env;


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static uint32_t evt_timer_hw_time(void);

/**
 *******************************************************************************
 * @brief  co time compare equal lesser
 *
 * @param[in] me  me
 * @param[in] he  he
 *
 * @return
 *******************************************************************************
 **/
static bool evt_timer_compare_equal_lesser(uint32_t me, uint32_t he)
{
    #if (EVT_TIMER_MAX_TICK == 0xFFFFFFFF)
    return (he - me < EVT_TIMER_MAX_DELAY);
    #else
    return (me > he) ? (me - he > EVT_TIMER_MAX_DELAY) : (he - me < EVT_TIMER_MAX_DELAY);
    #endif
}

/**
 *******************************************************************************
 * @brief evt_timer_past()
 *
 * @param[in] time
 *
 * @return
 *******************************************************************************
 **/
static bool evt_timer_past(evt_timer_t *timer)
{
    return evt_timer_compare_equal_lesser(timer->time, evt_timer_hw_time() + EVT_TIMER_RESOLUTION);
}

/**
 *******************************************************************************
 * @brief  evt timer overflow handler
 *******************************************************************************
 **/
static void evt_timer_overflow_handler(void *om_reg, drv_event_t drv_event, void *param0, void *param1)
{
    evt_set(EVT_TYPE_TIMER);
}

/**
 *******************************************************************************
 * @brief evt_timer_hw_prog()
 *
 * @param[in] timer
 *
 * @return
 *******************************************************************************
 **/
static void evt_timer_hw_prog(evt_timer_t *timer)
{
    if(timer) {
        if(evt_timer_past(timer)) {
            evt_set(EVT_TYPE_TIMER);
        } else {
            uint32_t /*lint !e578*/ time = timer->time, cur_time;
            cur_time = drv_pmu_timer_cnt_get();
            cur_time += EVT_TIMER_START_REDUNDANCE + EVT_TIMER_RESOLUTION;
            if (evt_timer_compare_equal_lesser(time, cur_time))
                time = cur_time + EVT_TIMER_RESOLUTION;
            drv_pmu_timer_control(PMU_TIMER_TRIG_VAL0, PMU_TIMER_CONTROL_SET_TIMER_VAL, (void *)(time));
            drv_pmu_timer_control(PMU_TIMER_TRIG_VAL0, PMU_TIMER_CONTROL_ENABLE, (void*)1);
        }
    } else {
        drv_pmu_timer_control(PMU_TIMER_TRIG_VAL0, PMU_TIMER_CONTROL_ENABLE, 0);
    }
}

/**
 *******************************************************************************
 * @brief  evt timer hw init
 *******************************************************************************
 **/
static uint32_t evt_timer_hw_time(void)
{
    return drv_pmu_timer_cnt_get();
}

/**
 *******************************************************************************
 * @brief  evt timer hw init
 *******************************************************************************
 **/
static void evt_timer_hw_init(void)
{
    drv_pmu_timer_register_isr_callback(PMU_TIMER_TRIG_VAL0, evt_timer_overflow_handler);
    drv_pmu_timer_init();
}

/**
 *******************************************************************************
 * @brief evt_timer_extract_from_pending_list()
 *
 * @param[in] id
 *
 * @return
 *******************************************************************************
 **/
static bool evt_timer_extract_from_pending_list(evt_timer_t *timer)
{
    return om_list_extract(&evt_timer_env.pending_list, &timer->hdr);
}

/**
 *******************************************************************************
 * @brief evt_timer_insert_to_pending_list()
 *
 * @param[in] timer
 *
 * @return
 *******************************************************************************
 **/
static void evt_timer_insert_to_pending_list(evt_timer_t *timer)
{
    om_list_node_t *hdr;

    // browse list
    for (hdr=PL1ST; hdr!=NULL; hdr=hdr->next) {
        if (evt_timer_compare_equal_lesser(timer->time, ((evt_timer_t *)hdr)->time)) {
            break;
        }
    }

    if (hdr) {
        om_list_insert_before(&evt_timer_env.pending_list, hdr, &timer->hdr);
    } else {
        om_list_add(&timer->hdr, &evt_timer_env.pending_list);
    }
}

/**
 *******************************************************************************
 * @brief evt_timer_sche()
 *
 * @return
 *******************************************************************************
 **/
static void evt_timer_sche(void)
{
    evt_timer_t *timer;

    // clear event
    evt_clear(EVT_TYPE_TIMER);

    /*
     * CHECK pending timer list
     */
    OM_CRITICAL_BEGIN();
    while(1) {
        // Peek the first pending
        timer = (evt_timer_t *)PL1ST;

        if (timer == NULL) {
            break;
        }

        // Is pasted ?
        if(!evt_timer_past(timer)) {
            break;
        }

        // Remove it from pending list
        evt_timer_extract_from_pending_list(timer);
        om_list_add(&timer->hdr, &evt_timer_env.pasted_list);
    }
    OM_CRITICAL_END();

    /*
     * CHECK pasted timer list
     */

    // Find the place to insert
    while(1) {
        OM_CRITICAL_BEGIN();
        timer = (evt_timer_t *)om_list_pop(&evt_timer_env.pasted_list);
        OM_CRITICAL_END();

        if (timer == NULL) {
            break;
        }

        if(timer->cb) {
//            TRC_IO(TRC_IO_EVT_TIMER_EXEC_CB, 1);
            /*lint -e{611}*/ ((evt_timer_callback_t)timer->cb)(timer, timer->param);
//            TRC_IO(TRC_IO_EVT_TIMER_EXEC_CB, 0);
        }

        OM_CRITICAL_BEGIN();
        // evt_timer_set may be called, check it
        if (!om_list_find(&evt_timer_env.pending_list, &timer->hdr)) {
            // evt_timer_del may be called, check it
            // Repeat mode
            if (timer->mode == EVT_TIMER_REPEAT) {
                while (evt_timer_past(timer)) {
                    timer->time += timer->delay_tick;
                }

                evt_timer_insert_to_pending_list(timer);
            }
        }
        OM_CRITICAL_END();
    }

    OM_CRITICAL_BEGIN();
    //Should be reprogram hardware timer and start it
    evt_timer_hw_prog((evt_timer_t *)PL1ST);
    OM_CRITICAL_END();
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Setup a software timer with tick
 *
 * @param[in] timer  the timer object (must be static or global variable)
 * @param[in] delay_tick  it uint is 30.52us
 * @param[in] mode  one shot or repeat
 * @param[in] callback  expire callback
 * @param[in] param  params
 *
 * @return false: Set timer fail;
 *         true: Set timer success
 *******************************************************************************
 **/
bool evt_timer_set_tick(evt_timer_t *timer, uint32_t delay_tick, evt_timer_mode_t mode, evt_timer_callback_t callback, void *param)
{
    evt_timer_t *first;
    bool first_in_q;

    if(delay_tick > EVT_TIMER_MAX_DELAY) {
        OM_ASSERT(0);
        return false;
    }

    OM_CRITICAL_BEGIN();

    first      = (evt_timer_t *)PL1ST;
    first_in_q = first == timer;

    om_list_extract(&evt_timer_env.pasted_list, &timer->hdr);
    evt_timer_extract_from_pending_list(timer);

    timer->delay_tick = delay_tick;
    timer->time = evt_timer_hw_time() + delay_tick;
    timer->mode = mode;
    timer->cb = /*lint !e611*/ (void *)callback;
    timer->param = param;

    evt_timer_insert_to_pending_list(timer);

    if(first_in_q || (timer == (evt_timer_t *)PL1ST)) {
        //Should be reprogram hardware timer and start it
        evt_timer_hw_prog((evt_timer_t *)PL1ST);
    }

    OM_CRITICAL_END();

    return true;
}

/**
 *******************************************************************************
 * @brief Setup a software timer with millisecond
 *
 * @param[in] timer  the timer object (must be static or global variable)
 * @param[in] delay  it uint is 1ms
 * @param[in] mode  one shot or repeat
 * @param[in] callback  expire callback
 * @param[in] param  params
 *
 * @return false: Set timer fail;
 *         true: Set timer success
 *******************************************************************************
 **/
bool evt_timer_set(evt_timer_t *timer, uint32_t delay_ms, evt_timer_mode_t mode,
        evt_timer_callback_t callback, void *param) /*lint !e578*/
{
    return evt_timer_set_tick(timer, EVT_TIMER_MS2TICK(delay_ms), mode, callback, param);
}

/**
 *******************************************************************************
 * @brief delete a timer
 *
 * @param[in] timer  the timer object (must be static or global variable)
 *******************************************************************************
 **/
void evt_timer_del(evt_timer_t *timer)
{
    OM_CRITICAL_BEGIN();

    bool first_in_q = timer==(evt_timer_t *)PL1ST;

    if(evt_timer_extract_from_pending_list(timer)) {
        if(PL1ST == NULL || first_in_q)
            evt_timer_hw_prog((evt_timer_t *)PL1ST);
    }

    om_list_extract(&evt_timer_env.pasted_list, &timer->hdr);
    timer->mode = EVT_TIMER_ONE_SHOT;

    OM_CRITICAL_END();
}

/**
 *******************************************************************************
 * @brief  evt timer init
 *******************************************************************************
 **/
void evt_timer_init(void)
{
    om_list_init(&evt_timer_env.pending_list);
    om_list_init(&evt_timer_env.pasted_list);

    // Init PMU timer
    evt_timer_hw_init();

    // Event callback
    evt_callback_set(EVT_TYPE_TIMER, evt_timer_sche);
}

/**
 *******************************************************************************
 * @brief timer dump
 *
 * @note If this function called in co_timer expired callback, the expired timer don't dump
 *
 * @param[in] printf_dump_func  dump function, link printf
 *******************************************************************************
 **/
void evt_timer_dump(void *printf_dump_func)
{
    uint32_t i;
    evt_timer_t *timer;
    evt_timer_t *timer_list_first_tbl[] = {
        (evt_timer_t *)evt_timer_env.pending_list.head,     //lint !e446
        (evt_timer_t *)evt_timer_env.pasted_list.head,      //lint !e446
    };
    void (*__printf)(const char *format, ...) = /*lint -e{611}*/ (void (*)(const char *format, ...))printf_dump_func;

    if (__printf == NULL) {
        return;
    }

    for (i=0; i<sizeof(timer_list_first_tbl)/sizeof(timer_list_first_tbl[0]); ++i) {
        for(timer = timer_list_first_tbl[i]; timer != NULL; timer = (evt_timer_t *)timer->hdr.next) {
            __printf("[TIMER] %08X delay=%dms cb=%08X %s\n",
                    timer, EVT_TIMER_TICK2MS(timer->delay_tick), timer->cb,
                    (timer->mode==EVT_TIMER_ONE_SHOT) ? "1shot" : "repeat");
        }
    }
}

/** @} */
