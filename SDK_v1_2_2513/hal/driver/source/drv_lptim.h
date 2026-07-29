/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup LPTIMER LPTIMER
 * @ingroup  DRIVER
 * @brief    LPTIMER driver
 * @details  LPTIMER driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_LPTIM_H
#define __DRV_LPTIM_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_LPTIM)
#include <stdint.h>
#include "om_driver.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * TYPEDEFS
 */
/// LP Timer Control
typedef enum {
    LPTIM_CONTROL_ENABLE,                  /*!< When argu is NULL, disable lptim, and vice versa */
    LPTIM_CONTROL_START,                   /*!< Start LP Timer */
    LPTIM_CONTROL_STOP,                    /*!< Stop LP Timer */
    LPTIM_CONTROL_CLEAR,                   /*!< Clear LP Timer counter, and load top val */
    LPTIM_CONTROL_CTO0,                    /*!< Clear toggle out0 to its idle state */
    LPTIM_CONTROL_CTO1,                    /*!< Clear toggle out1 to its idle state */
    LPTIM_CONTROL_CTO2,                    /*!< Clear toggle out2 to its idle state */
    LPTIM_CONTROL_CTO3,                    /*!< Clear toggle out3 to its idle state */
    LPTIM_CONTROL_CNT_SET,                 /*!< Set counter val, val is in range [0~0xFFFF] */
    LPTIM_CONTROL_TOP_SET,                 /*!< Set top val, val is in range [0~0xFFFF] */
    LPTIM_CONTROL_REP0_SET,                /*!< Set rep0 val, val is in range [0~0xFF] */
    LPTIM_CONTROL_REP1_SET,                /*!< Set rep1 val, val is in range [0~0xFF] */
    LPTIM_CONTROL_REP2_SET,                /*!< Set rep0 val, val is in range [0~0xFF] */
    LPTIM_CONTROL_REP3_SET,                /*!< Set rep1 val, val is in range [0~0xFF] */
    LPTIM_CONTROL_POWER_IN_SLEEP,          /*!< When argu is not NULL, power on lptim in sleep, and vice versa */
    LPTIM_CONTROL_INT_EN,                  /*!< Control enable or disable lptim interrupt, argu is the interrupt en combinations */
} lptim_control_t;

/// LP Timer Stop Mode：Repeat Mode for two group output channel 23 and 01
typedef enum {
    /// Free Mode: When started, the LETIMER counts down until it is stopped by software
    LPTIM_STOP_MODE_FREE           = 0U,
    /// stop01 Mode: The counter counts only while group 01 are running. When group01 stop running, the counter stops
    LPTIM_STOP_MODE_STOP_01        = 1U,
    /// stopor Mode: When group01 or group23 stop running, the counter stops.
    LPTIM_STOP_MODE_STOP_OR        = 2U,
    /// stopboth Mode:  When group01 and group23 both stop running, the counter stops.
    LPTIM_STOP_MODE_STOPBOTH       = 3U,
} lptim_stop_mode_t;

/// LP Timer Reption Mode
typedef enum {
    /// Free Running Mode: run until it is stopped
    LPTIM_MODE_FREE_RUNNING    = 0U,
    /// One Shot Mode: run as long as REP0 != 0
    LPTIM_MODE_ONE_SHOT        = 1U,
    /// Buffered Mode: run as long as REP0 != 0, REP1 will be loaded into REP0 once if REP1 != 0
    LPTIM_MODE_BUFFERED        = 2U,
    /// Double Mode: run as long as REP0 != 0 or REP1 != 0
    LPTIM_MODE_DOUBLE          = 3U,
} lptim_rep_mode_t;

/// Decide whether to load TOP val into Counter on each underflow
typedef enum {
    /// Load 0xFFFFFF into Counter
    LPTIM_TOP_DISABLE          = 0U,
    /// Load TOP val into Counter
    LPTIM_TOP_ENABLE           = 1U,
} lptim_top_en_t;

/// Decide whether to load TOPBUF val into TOP when REP0 is about to decrement to 0
typedef enum {
    /// BUFTOP is used
    LPTIM_BUFTOP_DISABLE       = 0U,
    /// BUFTOP is not used
    LPTIM_BUFTOP_ENABLE        = 1U,
} lptim_buftop_en_t;

/// LP Timer Clock Frequency Division
typedef enum {
    LPTIM_PRESC_DIV1           = 0U,
    LPTIM_PRESC_DIV2           = 1U,
    LPTIM_PRESC_DIV4           = 2U,
    LPTIM_PRESC_DIV8           = 3U,
    LPTIM_PRESC_DIV16          = 4U,
    LPTIM_PRESC_DIV32          = 5U,
    LPTIM_PRESC_DIV64          = 6U,
    LPTIM_PRESC_DIV128         = 7U,
    LPTIM_PRESC_DIV256         = 8U,
} lptim_presc_t;

/// LP Timer Channel
typedef enum {
    /// Channel 0
    LPTIM_CHAN_OUT0,
    /// Channel 1
    LPTIM_CHAN_OUT1,
    /// Channel 2
    LPTIM_CHAN_OUT2,
    /// Channel 3
    LPTIM_CHAN_OUT3,
} lptim_chan_out_t;

/// IDLE Level Defination
typedef enum {
    /// Define outx idle level to low
    LPTIM_POL_IDLE_LOW          = 0U,
    /// Define outx idle level to high
    LPTIM_POL_IDLE_HIGH         = 1U,
} lptim_out_pol_t;

/// LP Timer UnderFlow Out Action Defination
typedef enum {
    /// OUTx is held at its idle value
    LPTIM_UFOA_NONE            = 0U,
    /// OUTx is toggled on COMPx match, DIGITAL BUG: the pin state eventually returns to the IDLE state
    LPTIM_UFOA_TOGGLE          = 1U,
    /// OUTx is held active for one LP Timer clock cycle on COMP0/1 match, then return to its IDLE state
    LPTIM_UFOA_PULSE           = 2U,
    /// OUTx is set to IDLE on CNT underflow, and active on compare match COMP0/1
    LPTIM_UFOA_PWM             = 3U,
} lptim_underflow_out_action_t;

/// LP Timer interrupt enable type
typedef enum {
    LPTIM_INTE_COMP0_EN   = (0x1U << 0),
    LPTIM_INTE_COMP1_EN   = (0x1U << 1),
    LPTIM_INTE_UF_EN      = (0x1U << 2),
    LPTIM_INTE_REP0_EN    = (0x1U << 3),
    LPTIM_INTE_REP1_EN    = (0x1U << 4),
} lptim_int_en_t;

/// LP Timer OUT Configuration
typedef struct {
    /// OUT Polarity
    lptim_out_pol_t                pol;
    /// OUT Action
    lptim_underflow_out_action_t   action;
} lptim_out_config_t;

/// LP Timer Free Running Configuration
typedef struct {
    /// Frequency Division
    lptim_presc_t       presclar;
    /// Use TOP val or not
    lptim_top_en_t      top_en;
    /// TOP val
    uint16_t            top_val;
    /// Output Compare value0
    uint16_t            compare_val0;
    /// Output Compare value1
    uint16_t            compare_val1;
    /// Output Compare value2
    uint16_t            compare_val2;
    /// Output Compare value3
    uint16_t            compare_val3;
    /// stop mode
    uint16_t            stop_mode;
} lptim_free_running_config_t;

/// LP Timer One Shot Configuration
typedef struct {
    /// Frequency Division
    lptim_presc_t       presclar;
    /// Reption value0
    uint8_t             rep0_val;
    /// Reption value2
    uint8_t             rep2_val;
    /// Use TOP val or not
    lptim_top_en_t      top_en;
    /// TOP val
    uint16_t            top_val;
    /// Output Compare value0
    uint16_t            compare_val0;
    /// Output Compare value1
    uint16_t            compare_val1;
    /// Output Compare value2
    uint16_t            compare_val2;
    /// Output Compare value3
    uint16_t            compare_val3;
    /// stop mode
    uint16_t            stop_mode;
} lptim_one_shot_config_t;

/// LP Timer Buffered Configuration
typedef struct {
    /// Frequency Division
    lptim_presc_t       presclar;
    /// Reption value0
    uint8_t             rep0_val;
    /// Reption value1
    uint8_t             rep1_val;
    /// Reption value2
    uint8_t             rep2_val;
    /// Reption value3
    uint8_t             rep3_val;
    /// Use TOP val or not
    lptim_top_en_t      top_en;
    /// TOP val
    uint16_t            top_val;
    /// Use BUFTOP val or not
    lptim_buftop_en_t   buftop_en;
    /// BUFTOP val
    uint16_t            buftop_val;
    /// Output Compare value0
    uint16_t            compare_val0;
    /// Output Compare value1
    uint16_t            compare_val1;
    /// Output Compare value2
    uint16_t            compare_val2;
    /// Output Compare value3
    uint16_t            compare_val3;
    /// stop mode
    uint16_t            stop_mode;
} lptim_buffered_config_t;

/// LP Timer Double Configuration
typedef struct {
    /// Frequency Division
    lptim_presc_t       presclar;
    /// Reption value0
    uint8_t             rep0_val;
    /// Reption value1
    uint8_t             rep1_val;
    /// Reption value2
    uint8_t             rep2_val;
    /// Reption value3
    uint8_t             rep3_val;
    /// Use TOP val or not
    lptim_top_en_t      top_en;
    /// TOP val
    uint16_t            top_val;
    /// Output Compare value0
    uint16_t            compare_val0;
    /// Output Compare value1
    uint16_t            compare_val1;
    /// Output Compare value2
    uint16_t            compare_val2;
    /// Output Compare value3
    uint16_t            compare_val3;
    /// stop mode
    uint16_t            stop_mode;
} lptim_double_config_t;

/// LP Timer multimode Configuration
typedef struct {
    /// Frequency Division
    lptim_presc_t       presclar;
    /// Reption mode01
    uint8_t             rep01_mode;
    /// Reption mode23
    uint8_t             rep23_mode;
    /// Reption value0
    uint8_t             rep0_val;
    /// Reption value1
    uint8_t             rep1_val;
    /// Reption value2
    uint8_t             rep2_val;
    /// Reption value3
    uint8_t             rep3_val;
    /// Use TOP val or not
    lptim_top_en_t      top_en;
    /// TOP val
    uint16_t            top_val;
    /// Use BUFTOP val or not
    lptim_buftop_en_t   buftop_en;
    /// BUFTOP val
    uint16_t            buftop_val;
    /// Output Compare value0
    uint16_t            compare_val0;
    /// Output Compare value1
    uint16_t            compare_val1;
    /// Output Compare value2
    uint16_t            compare_val2;
    /// Output Compare value3
    uint16_t            compare_val3;
    /// stop mode
    uint16_t            stop_mode;
} lptim_multimode_config_t;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Get LPTIM base from lptim idx
 *
 * @param idx  Index of LPTIM peripheral
 *
 * @return OM_LPTIM Type pointer
 *******************************************************************************
 */
static inline OM_LPTIM_Type* drv_lptim_idx2base(uint8_t idx)
{
    OM_LPTIM_Type *const lptim[] = {
        #if (RTE_LPTIM)
        OM_LPTIM,
        #else
        NULL,
        #endif
    };

    return (idx < sizeof(lptim)/sizeof(lptim[0])) ? lptim[idx] : NULL;
}

#if (RTE_LPTIM_REGISTER_CALLBACK)
/**
 * @brief LP Timer register callback
 *
 * @param[in] om_lptim      Pointer to LP Timer
 * @param[in] isr_cb         callback
 *
 **/
extern void drv_lptim_register_isr_callback(OM_LPTIM_Type *om_lptim, drv_isr_callback_t isr_cb);
#endif

/**
 *******************************************************************************
 * @brief The interrupt callback for LP Timer driver. It is a weak function. User should define
 *        their own callback in user file, other than modify it in the LP Timer driver.
 *
 * @param om_lptim          The LP Timer device address
 * @param event             The driver uart event
 *                           - DRV_EVENT_LPTIM_COMP0
                             - DRV_EVENT_LPTIM_COMP1
                             - DRV_EVENT_LPTIM_COMP2
                             - DRV_EVENT_LPTIM_COMP3
                             - DRV_EVENT_LPTIM_UNDER_FLOW
                             - DRV_EVENT_LPTIM_REP0
                             - DRV_EVENT_LPTIM_REP1
                             - DRV_EVENT_LPTIM_REP2
                             - DRV_EVENT_LPTIM_REP3
 * @param param0              The  pointer of param0
 * @param param1              The  pointer of param1
 *******************************************************************************
 */
extern void drv_lptim_isr_callback(OM_LPTIM_Type *om_lptim, drv_event_t event, void *param0, void *param1);

/**
 * @brief LP Timer free-running config
 *
 * @param[in] om_lptim      Pointer to LP Timer
 * @param[in] cfg           free-running config
 *
 * @return errno
 **/
extern om_error_t drv_lptim_free_running_init(OM_LPTIM_Type *om_lptim, const lptim_free_running_config_t *cfg);

/**
 * @brief LP Timer one shot config
 *
 * @param[in] om_lptim      Pointer to LP Timer
 * @param[in] cfg           one shot config
 *
 * @return errno
 **/
extern om_error_t drv_lptim_one_shot_init(OM_LPTIM_Type *om_lptim, const lptim_one_shot_config_t *cfg);

/**
 * @brief LP Timer buffered config
 *
 * @param[in] om_lptim      Pointer to LP Timer
 * @param[in] cfg           buffered config
 *
 * @return errno
 **/
extern om_error_t drv_lptim_buffered_init(OM_LPTIM_Type *om_lptim, const lptim_buffered_config_t *cfg);

/**
 * @brief LP Timer double config
 * @note Digital Bug: if clear rep0/rep1's interrup flag, the out0/1 will continue
 * output, now just support output the max times of repetition
 *
 * @param[in] om_lptim      Pointer to LP Timer
 * @param[in] cfg           double config
 *
 * @return errno
 **/
extern om_error_t drv_lptim_double_init(OM_LPTIM_Type *om_lptim, const lptim_double_config_t *cfg);

/**
 * @brief lp tim multimode config
 *
 * @param[in] om_lptim     lp timer instance
 * @param[in] cfg           multimode config
 *
 * @return status, see@ref om_error_t
 **/
extern om_error_t drv_lptim_multimode_init(OM_LPTIM_Type *om_lptim, const lptim_multimode_config_t *cfg);

/**
 * @brief LP Timer out config
 *
 * @param[in] om_lptim      Pointer to LP Timer
 * @param[in] chan_outx     output channel(0,1)
 * @param[in] cfg           out config
 *
 * @return errno
 **/
extern om_error_t drv_lptim_outx_config(OM_LPTIM_Type *om_lptim, lptim_chan_out_t chan_outx, const lptim_out_config_t *cfg);

/**
 * @brief LP Timer control
 *
 * @param[in] om_lptim      Pointer to LP Timer
 * @param[in] ctrl          control command
 * @param[in] argu          argument
 *
 * @return control status
 *
 **/
extern void *drv_lptim_control(OM_LPTIM_Type *om_lptim, lptim_control_t ctrl, void *argu);

/**
 * @brief lp tim interrupt service routine
 *
 * @param[in] om_lptim     Pointer to LP Timer
 **/
extern void drv_lptim_isr(OM_LPTIM_Type *om_lptim);


#ifdef __cplusplus
}
#endif

#endif  /* (RTE_LPTIM) */

#endif  /* __DRV_LPTIM_H */


/** @} */