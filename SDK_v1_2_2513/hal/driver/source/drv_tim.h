/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup TIMER TIMER
 * @ingroup  DRIVER
 * @brief    TIMER driver
 * @details  TIMER driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_TIM_H
#define __DRV_TIM_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_TIM0 || RTE_TIM1 || RTE_TIM2)
#include <stdint.h>
#include "om_driver.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * TYPEDEFS
 */
/// TIM Channel
typedef enum {
    /// TIM Channel1
    TIM_CHAN_1      = 0U,
    /// TIM Channel2
    TIM_CHAN_2      = 1U,
    /// TIM Channel3
    TIM_CHAN_3      = 2U,
    /// TIM Channel4
    TIM_CHAN_4      = 3U,

    TIM_CHAN_ALL    = 4U,
} tim_chan_t;

/// TIM Active Level
typedef enum {
    /// Specifies active level is high
    TIM_PWM_POL_ACTIVE_HIGH = 0U,
    /// Specifies active level is low
    TIM_PWM_POL_ACTIVE_LOW  = 1U,
} tim_pwm_pol_t;

/// TIM Force Level
typedef enum {
    /// Specifies force level is inactive
    TIM_FORCE_LEVEL_INACTIVE = 4U,
    /// Specifies force level is active
    TIM_FORCE_LEVEL_ACTIVE   = 5U,
} tim_force_level_t;

/// TIM Capture Polarity
typedef enum {
    /// Capture at rising edge
    TIM_CAPTURE_POLARITY_RISING_EDGE  = 0U,
    /// Capture at falling edge
    TIM_CAPTURE_POLARITY_FALLING_EDGE = 1U,
} tim_capture_polarity_t;

/// TIM General Purpose Configuration
typedef struct {
    /// period(us)
    uint32_t period_us;
    uint32_t delay_period;
} tim_gp_config_t;

/// TIM PWM Channel Configuration
typedef struct {
    /// PWM polarities
    tim_pwm_pol_t pol;
    /// Output compare value
    uint32_t      oc_val;
    //complementary_output_enable
    bool complementary_output_enable;
} tim_pwm_chan_config_t;

#if (RTE_GPDMA)
/// TIM PWM DMA Configuration
typedef struct {
    /// dma enable or not
    uint8_t en;
    /// tim channel
    tim_chan_t tim_chan;
    /// DMA Chain
    gpdma_chain_trans_t *chain;
} tim_pwm_gpdma_config_t;

/// TIM Capture DMA Configuration
typedef struct {
    /// dma enable or not
    uint8_t en;
    /// tim channel
    tim_chan_t tim_chan;
    /// DMA Chain
    gpdma_chain_trans_t *chain;
} tim_cap_gpdma_config_t;
#endif

/// TIM PWM Output Configuration
typedef struct {
    uint32_t cnt_freq;                      /*!< Frequency for every count */
    uint32_t period_cnt;                    /*!< Period counter */
    uint16_t dead_time;                     /*!< dead_time generate step */
    struct {
        uint8_t en;                         /*!< Channel enable or not */
        tim_pwm_chan_config_t cfg;          /*!< PWM channel configuration */
    } chan[TIM_CHAN_ALL];
    #if (RTE_GPDMA)
    tim_pwm_gpdma_config_t gpdma_cfg;           /*!< if use gpdma, pwm gpdma configuration, only one channel use dma */
    #endif
} tim_pwm_output_config_t;

/// TIM Capture Configuration
typedef struct {
    uint32_t cnt_freq;                      /*!< Frequency for every count */
    struct {
        uint8_t en;                         /*!< Channel enable or not */
        tim_capture_polarity_t pol;         /*!< Channel capture polarity configuration */
    } chan[TIM_CHAN_ALL];
    #if (RTE_GPDMA)
    tim_cap_gpdma_config_t gpdma_cfg;           /*!< if use gpdma, capture gpdma configuration, only one channel use dma  */
    #endif
} tim_capture_config_t;

/// TIM PWM Input Configuration
typedef struct {
    uint32_t cnt_freq;                      /*!< Frequency for every count */
} tim_pwm_input_config_t;

/// TIM mode
typedef enum {
    /// Work as timer mode
    TIM_TIMER_MODE,
    /// Work as PWM mode
    TIM_PWM_MODE,
    /// Work as CAP mode
    TIM_CAP_MODE,
} tim_mode_t;

typedef enum {
    UP_COUNT   = 0,
    DOWN_COUNT = 1,
} tim_cnt_mode_t;

/// TIM PWM Channel Configuration
typedef struct {
    /// PWM polarities
    tim_pwm_pol_t pol;
    /// Output compare value
    uint32_t      oc_val;
    //complementary_output_enable
    bool complementary_output_enable;
} tim_pwm_complementary_chan_config_t;

/// TIM PWM Output Configuration
typedef struct {
    uint32_t cnt_freq;                      /*!< Frequency for every count */
    uint32_t period_cnt;                    /*!< Period counter */
    uint16_t dead_time;                     /*!< dead_time generate step */
    struct {
        uint8_t en;                         /*!< Channel enable or not */
        tim_pwm_complementary_chan_config_t cfg;          /*!< PWM channel configuration */
    } chan[TIM_CHAN_ALL];
    #if (RTE_GPDMA)
    tim_pwm_gpdma_config_t gpdma_cfg;           /*!< if use gpdma, pwm gpdma configuration, only one channel use dma */
    #endif
} tim_pwm_complementary_output_config_t;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Get TIM base from tim idx
 *
 * @param idx  Index of TIM peripheral
 *
 * @return OM_TIM Type pointer
 *******************************************************************************
 */
static inline OM_TIM_Type* drv_tim_idx2base(uint8_t idx)
{
    OM_TIM_Type *const tim[] = {
        #if (RTE_TIM0)
        OM_TIM0,
        #else
        NULL,
        #endif
        #if (RTE_TIM1)
        OM_TIM1,
        #else
        NULL,
        #endif
        #if (RTE_TIM2)
        OM_TIM2,
        #else
        NULL,
        #endif
    };

    return (idx < sizeof(tim)/sizeof(tim[0])) ? tim[idx] : NULL;
}

/**
 * @brief Tim initialization
 *
 * @param[in] om_tim      Pointer to TIM
 *
 * @return errno
 **/
extern om_error_t drv_tim_init(OM_TIM_Type *om_tim);

/**
 * @brief Tim uninitialization
 *
 * @param[in] om_tim      Pointer to TIM
 *
 * @return errno
 **/
extern om_error_t drv_tim_uninit(OM_TIM_Type *om_tim);

#if (RTE_TIM_REGISTER_CALLBACK)
/**
 * @brief Register tim callback
 *
 * @param[in] om_tim      Pointer to TIM
 * @param[in] isr_cb      callback
 **/
extern void drv_tim_register_isr_callback(OM_TIM_Type *om_tim, drv_isr_callback_t isr_cb);
#endif

/**
 *******************************************************************************
 * @brief Allocate timer dma channel
 *
 * @param[in] om_tim    Pointer to timer
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_tim_gpdma_channel_allocate(OM_TIM_Type *om_tim);

/**
 *******************************************************************************
 * @brief Release timer dma channel
 *
 * @param[in] om_tim    Pointer to timer
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_tim_gpdma_channel_release(OM_TIM_Type *om_tim);

/**
 * @brief General Purpose Timer Start
 *
 * @param[in] om_tim      Pointer to TIM
 * @param[in] cfg         general purpose timer config
 *
 * @return errno
 **/
extern om_error_t drv_tim_gp_start(OM_TIM_Type *om_tim, const tim_gp_config_t *cfg);

/**
 * @brief General Purpose Timer stop
 *
 * @param[in] om_tim      Pointer to TIM
 **/
extern void drv_tim_gp_stop(OM_TIM_Type *om_tim);

/**
 *******************************************************************************
 * @brief Change PWM period
 *
 * @param[in] om_tim        Pointer to TIM
 * @param[in] period_cnt    The timer automatic reloading value, timer0 MAX:0xFFFFFFFF  timer1and2 MAX:0xFFFF
 *******************************************************************************
 **/
__STATIC_FORCEINLINE void drv_tim_pwm_change_period_cnt(OM_TIM_Type *om_tim, uint32_t period_cnt)
{
    om_tim->ARR = period_cnt;
}

/**
 *******************************************************************************
 * @brief Change PWM channel output compare val
 *
 * @param[in] om_tim        Pointer to TIM
 * @param[in] channel       PWM channel index
 * @param[in] oc_val        The timer new output compare val, timer0 MAX:0xFFFFFFFF  timer1and2 MAX:0xFFFF, should be less than period_count
 *******************************************************************************
 **/
__STATIC_FORCEINLINE void drv_tim_pwm_change_oc_val(OM_TIM_Type *om_tim, tim_chan_t channel, uint32_t oc_val)
{
    om_tim->CCR[channel] = oc_val;
}

/**
 * @brief TIM PWM output start, use pwm mode1 by default, channel is active as long as TIMx_CNT <
 * TIMx_CCR else inactive.
 *
 * @param[in] om_tim        Pointer to TIM
 * @param[in] cfg           TIM PWM Output Configuration
 *
 * @return errno
 **/
extern om_error_t drv_tim_pwm_output_start(OM_TIM_Type *om_tim, const tim_pwm_output_config_t *cfg);

/**
 * @brief TIM PWM output stop
 *
 * @param[in] om_tim        Pointer to TIM
 * @param[in] channel       The timer channel, if TIMER_CHANNEL_ALL, stop the TIMx.
 **/
extern void drv_tim_pwm_output_stop(OM_TIM_Type *om_tim, tim_chan_t channel);

/**
 * @brief Force PWM channel output
 *
 * @param[in] om_tim        Pointer to TIM
 * @param[in] channel       Channel index
 * @param[in] level         Force output level
 **/
extern void drv_tim_pwm_force_output(OM_TIM_Type *om_tim, tim_chan_t channel, tim_force_level_t level);

/**
 * @brief Timer capture start
 *
 * @param[in] om_tim        Pointer to TIM, NOTE:only TIM0 support capature in OM6626
 * @param[in] cfg           TIM Capture Configuration
 *
 * @return errno
 **/
extern om_error_t drv_tim_capture_start(OM_TIM_Type *om_tim, const tim_capture_config_t *cfg);

/**
 * @brief Timer capture stop
 *
 * @param[in] om_tim        Pointer to TIM
 * @param[in] channel       The tiemr channel, if TIMER_CHANNEL_ALL, stop the TIMx.
 **/
extern void drv_tim_capture_stop(OM_TIM_Type *om_tim, tim_chan_t channel);

/**
 * @brief Timer PWM input mode start
 *
 * @param[in] om_tim        Pointer to TIM
 * @param[in] cfg           TIM PWM Input Configuration
 *
 * @return errno
 **/
extern om_error_t drv_tim_pwm_input_start(OM_TIM_Type *om_tim, const tim_pwm_input_config_t *cfg);

/**
 * @brief Timer PWM input mode stop
 *
 * @param[in] om_tim      Pointer to TIM
 **/
extern void drv_tim_pwm_input_stop(OM_TIM_Type *om_tim);

/**
 * @brief Timer interrupt service routine
 *
 * @param[in] om_tim      Pointer to TIM
 **/
extern void drv_tim_isr(OM_TIM_Type *om_tim);

/**
 * @brief TIM PWM complementary output start, use pwm mode1 by default, channel is active as long as TIMx_CNT <
 * TIMx_CCR else inactive.
 *
 * @param[in] om_tim        Pointer to TIM
 * @param[in] cfg           TIM PWM Output Configuration
 *
 * @return errno
 **/
extern om_error_t drv_tim_pwm_complementary_output_start(OM_TIM_Type *om_tim, tim_pwm_complementary_output_config_t *cfg);

#ifdef __cplusplus
}
#endif

#endif  /* (RTE_TIM0 || RTE_TIM1 || RTE_TIM2) */

#endif  /* __DRV_TIM_H */


/** @} */
