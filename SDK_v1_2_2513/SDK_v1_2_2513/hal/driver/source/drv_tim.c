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
 * @brief    TIMER driver source file
 * @details  TIMER driver source file
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
#include "RTE_driver.h"
#if (RTE_TIM0 || RTE_TIM1 || RTE_TIM2)
#include <stddef.h>
#include "om_device.h"
#include "om_driver.h"


/*******************************************************************************
 * MACROS
 */
#define TIM_NUM                 3
#define TIM_ARR_MAX_32BIT       0xFFFFFFFF
#define TIM_ARR_MAX_16BIT       0xFFFF
#define TIM_PSC_MAX             0xFFFF

/*
 * Used to define drv_env_t and drv_resource_t structures
 */
#define DRV_TIM_DEFINE(NAMEn, namen)                                           \
static tim_env_t namen##_env = {                                               \
    .isr_cb     = NULL,                                                        \
    .gpdma_chan = GPDMA_NUMBER_OF_CHANNELS,                                    \
    .tim_chan   = TIM_CHAN_ALL,                                                \
};                                                                             \
static const drv_resource_t namen##_resource = {                               \
    .cap      = CAP_##NAMEn,                                                   \
    .reg      = OM_##NAMEn,                                                    \
    .env      = (void *)&namen##_env,                                          \
    .irq_num  = NAMEn##_IRQn,                                                  \
    .irq_prio = RTE_##NAMEn##_IRQ_PRIORITY,                                    \
    .gpdma_tx = {                                                              \
        .id   = GPDMA_ID_##NAMEn,                                              \
        .prio = 0,                                                             \
    },                                                                         \
    .gpdma_rx = {                                                              \
        .id   = GPDMA_ID_##NAMEn,                                              \
        .prio = 0,                                                             \
    },                                                                         \
}


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    drv_isr_callback_t      isr_cb;
    #if (RTE_GPDMA)
    uint8_t                 gpdma_chan;
    tim_chan_t              tim_chan;
    #endif  /* (RTE_GPDMA) */
} tim_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
#if (RTE_TIM0)
DRV_TIM_DEFINE(TIM0, tim0);
#endif
#if (RTE_TIM1)
DRV_TIM_DEFINE(TIM1, tim1);
#endif
#if (RTE_TIM2)
DRV_TIM_DEFINE(TIM2, tim2);
#endif


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
/**
 * @brief tim get resource
 *
 * @param[in] om_tim      timer instance
 *
 * @return timer resource pointer
 **/
static const drv_resource_t *tim_get_resource(OM_TIM_Type *om_tim)
{
    static const drv_resource_t *resources[] = {
        #if (RTE_TIM0)
        &tim0_resource,
        #endif
        #if (RTE_TIM1)
        &tim1_resource,
        #endif
        #if (RTE_TIM2)
        &tim2_resource,
        #endif
    };

    for(uint32_t i=0; i<sizeof(resources)/sizeof(resources[0]); i++) {
        if ((uint32_t)om_tim == (uint32_t)resources[i]->reg) {
            return resources[i];
        }
    }

    OM_ASSERT(0);
    return NULL;
}

/**
 * @brief timer calcurate prescaler and auto reload register
 *
 * @param[in] om_tim        timer instance
 * @param[in] period_us     us set
 *
 * @return true: set success, false: set fail.
 **/
static bool tim_calc_psc_arr(OM_TIM_Type *om_tim, uint32_t period_us)
{
    uint32_t clk_mhz;
    uint64_t psc, arr, factor, tim_arr_max;

    if (om_tim == OM_TIM0) {
        tim_arr_max = TIM_ARR_MAX_32BIT;
    } else if (om_tim == OM_TIM1 || om_tim == OM_TIM2) {
        tim_arr_max = TIM_ARR_MAX_16BIT;
    } else {
        return false;
    }

    clk_mhz = drv_rcc_clock_get((rcc_clk_t)(uint32_t)om_tim) / 1000000;
    factor  = period_us / (tim_arr_max + 1);

    psc = clk_mhz * (factor + 1);
    arr = period_us / (factor + 1);

    if ((arr > tim_arr_max) || (psc > TIM_PSC_MAX)) {
        return false;
    }

    om_tim->PSC = (uint32_t)(psc - 1);
    om_tim->ARR = (uint32_t)(arr - 1);

    return true;
}

#if (RTE_GPDMA)
static void tim_gpdma_isr_cb(void *resource, drv_event_t event, gpdma_chain_trans_t *next_chain)
{
    tim_env_t   *env;
    OM_TIM_Type *om_tim;

    env = (tim_env_t *)((const drv_resource_t *)resource)->env;
    om_tim = (OM_TIM_Type *)((const drv_resource_t *)resource)->reg;

    switch (event) {
        case DRV_GPDMA_EVENT_TERMINAL_COUNT_REQUEST:
            if (env->isr_cb) {
                env->isr_cb(om_tim, DRV_EVENT_TIM_GPDMA_COMPLETE, next_chain, NULL);
            }
            break;
        case DRV_GPDMA_EVENT_ABORT:
            break;
        default:
            OM_ASSERT(0);
            break;
    }
}
#endif


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 * @brief tim initialization
 *
 * @param[in] om_tim      timer instance
 *
 * @return status, see@om_error_t
 **/
om_error_t drv_tim_init(OM_TIM_Type *om_tim)
{
    const drv_resource_t    *resource;
    tim_env_t               *env;

    resource = tim_get_resource(om_tim);
    if (resource == NULL) {
        return OM_ERROR_RESOURCES;
    }

    DRV_RCC_RESET((rcc_clk_t)(size_t)resource->reg);

    env = (tim_env_t *)resource->env;
    env->isr_cb = NULL;

    #if (RTE_GPDMA)
    if (resource->cap & CAP_TIM_GPDMA_MASK) {
        env->tim_chan = TIM_CHAN_ALL;
        env->gpdma_chan = GPDMA_NUMBER_OF_CHANNELS;
    }
    #endif

    NVIC_ClearPendingIRQ(resource->irq_num);
    NVIC_SetPriority(resource->irq_num, resource->irq_prio);
    NVIC_EnableIRQ(resource->irq_num);

    return OM_ERROR_OK;
}

/**
 * @brief tim uninitialization
 *
 * @param[in] om_tim      timer instance
 *
 * @return status, see@om_error_t
 **/
om_error_t drv_tim_uninit(OM_TIM_Type *om_tim)
{
    const drv_resource_t *resource;

    resource = tim_get_resource(om_tim);
    if (resource == NULL) {
        return OM_ERROR_RESOURCES;
    }

    DRV_RCC_CLOCK_ENABLE((rcc_clk_t)(size_t)resource->reg, 0U);

    NVIC_DisableIRQ(resource->irq_num);

    return OM_ERROR_OK;
}

#if (RTE_TIM_REGISTER_CALLBACK)
/**
 * @brief tim register callback
 *
 * @param[in] om_tim      timer instance
 * @param[in] isr_cb      callback to register
 *
 * @return None
 **/
void drv_tim_register_isr_callback(OM_TIM_Type *om_tim, drv_isr_callback_t isr_cb)
{
    const drv_resource_t *resource;
    tim_env_t *env;

    resource = tim_get_resource(om_tim);
    if (resource) {
        env = (tim_env_t *)resource->env;
        env->isr_cb = isr_cb;
    }
}
#endif

/**
 * @brief General Purpose Timer Start
 *
 * @param[in] om_tim      tim instance
 * @param[in] config      general timer config
 *
 * @return status, see@om_error_t
 **/
om_error_t drv_tim_gp_start(OM_TIM_Type *om_tim, const tim_gp_config_t *cfg)
{
    const drv_resource_t *resource;

    OM_ASSERT(cfg);

    resource = tim_get_resource(om_tim);
    if (resource == NULL) {
        return OM_ERROR_RESOURCES;
    }

    if (!tim_calc_psc_arr(om_tim, cfg->period_us)) {
        return OM_ERROR_OUT_OF_RANGE;
    }

    // use edge-aligned mode, upcounter, ARR register is buffered
    register_set(&om_tim->CTR1, MASK_4REG(TIM_CTR1_ARPE, 1,
                                          TIM_CTR1_OPM,  0,
                                          TIM_CTR1_CMS,  0,
                                          TIM_CTR1_DIR,  0));
    om_tim->DIER = 0;
    om_tim->CNT  = 0;
    om_tim->SR   = 0;

    om_tim->RCR = cfg->delay_period;
    // set CCxS input as default (ICx->TIx)
    register_set(&om_tim->CCMR1, MASK_2REG(TIM_CCMR1_CC1S, 1,
                                           TIM_CCMR1_CC2S, 1));
    register_set(&om_tim->CCMR2, MASK_2REG(TIM_CCMR2_CC3S, 1,
                                           TIM_CCMR2_CC4S, 1));
    // Set the update generation bit to make the register configuration take effect immediately
    om_tim->EGR |= TIM_EGR_UG_MASK;
    while (!(om_tim->SR & TIM_SR_UIF_MASK));
    om_tim->SR = 0;
    // enable update interrupt
    om_tim->DIER |= TIM_DIER_UIE_MASK;

    // start
    om_tim->CTR1 |= TIM_CTR1_CEN_MASK;

    return OM_ERROR_OK;
}

/**
 * @brief General Purpose Timer stop
 *
 * @param[in] om_tim      timer instance
 *
 * @return None
 **/
void drv_tim_gp_stop(OM_TIM_Type *om_tim)
{
    const drv_resource_t *resource;

    resource = tim_get_resource(om_tim);
    if (resource == NULL) {
        return;
    }

    om_tim->CTR1 &= ~TIM_CTR1_CEN_MASK;
}

/**
 * @brief tim pwm output start
 *
 * @param[in] om_tim      timer instance
 * @param[in] config      tim_pwm_output_config_t
 *
 * @return status see@om_error_t
 **/
om_error_t drv_tim_pwm_output_start(OM_TIM_Type *om_tim, const tim_pwm_output_config_t *cfg)
{
    const drv_resource_t *resource;
    tim_env_t            *env;
    uint32_t              tim_clk, psc;
    uint32_t              ccer = 0;
    uint32_t              trans_size_byte;

    OM_ASSERT(cfg);

    resource = tim_get_resource(om_tim);
    if (resource == NULL) {
        return OM_ERROR_RESOURCES;
    }
    env = (tim_env_t *)resource->env;

    // calculate prescaler val
    tim_clk = drv_rcc_clock_get((rcc_clk_t)(size_t)resource->reg);
    psc     = (tim_clk / cfg->cnt_freq) - 1;
    if (psc > TIM_PSC_MAX) {
        return OM_ERROR_OUT_OF_RANGE;
    }

    // update shadow register
    if (!(om_tim->CTR1 & TIM_CTR1_CEN_MASK)) {
        om_tim->DIER    = 0;
        om_tim->CTR1    = TIM_CTR1_ARPE_MASK;
        om_tim->PSC     = psc;
        om_tim->ARR     = cfg->period_cnt - 1;
    }

    // channel config
    for (uint8_t chan_id = TIM_CHAN_1; chan_id < TIM_CHAN_ALL; chan_id++) {
        switch (chan_id) {
            case TIM_CHAN_1:
                if (cfg->chan[chan_id].en) {
                    // pwm mode 1, upcounter, ccrx preload
                    register_set(&om_tim->CCMR1, MASK_3REG(TIM_CCMR1_OC1M,  6,
                                                           TIM_CCMR1_OC1PE, 1,
                                                           TIM_CCMR1_CC1S,  0));
                    // compare output enable,set pol
                    register_set(&ccer, MASK_2REG(TIM_CCER_CC1E, 1,
                                                  TIM_CCER_CC1P, cfg->chan[chan_id].cfg.pol));
                    // set output compare val
                    om_tim->CCR[chan_id] = cfg->chan[chan_id].cfg.oc_val;
                } else {
                    // set CCnS input
                    register_set(&om_tim->CCMR1, MASK_1REG(TIM_CCMR1_CC1S, 1));
                    // disable compare
                    ccer &= ~TIM_CCER_CC1E_MASK;
                }
                break;
            case TIM_CHAN_2:
                if (cfg->chan[chan_id].en) {
                    // pwm mode 1, upcounter, ccrx preload
                    register_set(&om_tim->CCMR1, MASK_3REG(TIM_CCMR1_OC2M,  6,
                                                           TIM_CCMR1_OC2PE, 1,
                                                           TIM_CCMR1_CC2S,  0));
                    // compare output enable,set pol
                    register_set(&ccer, MASK_2REG(TIM_CCER_CC2E, 1,
                                                  TIM_CCER_CC2P, cfg->chan[chan_id].cfg.pol));
                    // set output compare val
                    om_tim->CCR[chan_id] = cfg->chan[chan_id].cfg.oc_val;
                } else {
                    // set CCnS input
                    register_set(&om_tim->CCMR1, MASK_1REG(TIM_CCMR1_CC2S, 1));
                    // disable compare
                    ccer &= ~TIM_CCER_CC2E_MASK;
                }
                break;
            case TIM_CHAN_3:
                if (cfg->chan[chan_id].en) {
                    // pwm mode 1, upcounter, ccrx preload
                    register_set(&om_tim->CCMR2, MASK_3REG(TIM_CCMR2_OC3M,  6,
                                                           TIM_CCMR2_OC3PE, 1,
                                                           TIM_CCMR2_CC3S,  0));
                    // compare output enable,set pol
                    register_set(&ccer, MASK_2REG(TIM_CCER_CC3E, 1,
                                                  TIM_CCER_CC3P, cfg->chan[chan_id].cfg.pol));
                    // set output compare val
                    om_tim->CCR[chan_id] = cfg->chan[chan_id].cfg.oc_val;
                } else {
                    // set CCnS input
                    register_set(&om_tim->CCMR2, MASK_1REG(TIM_CCMR2_CC3S, 1));
                    // disable compare
                    ccer &= ~TIM_CCER_CC3E_MASK;
                }
                break;
            case TIM_CHAN_4:
                if (cfg->chan[chan_id].en) {
                    // pwm mode 1, upcounter, ccrx preload
                    register_set(&om_tim->CCMR2, MASK_3REG(TIM_CCMR2_OC4M,  6,
                                                           TIM_CCMR2_OC4PE, 1,
                                                           TIM_CCMR2_CC4S,  0));
                    // compare output enable,set pol
                    register_set(&ccer, MASK_2REG(TIM_CCER_CC4E, 1,
                                                  TIM_CCER_CC4P, cfg->chan[chan_id].cfg.pol));
                    // set output compare val
                    om_tim->CCR[chan_id] = cfg->chan[chan_id].cfg.oc_val;
                } else {
                    // set CCnS input
                    register_set(&om_tim->CCMR2, MASK_1REG(TIM_CCMR2_CC4S, 1));
                    // disable compare
                    ccer &= ~TIM_CCER_CC4E_MASK;
                }
                break;
            default:
                break;
        }
    }

    om_tim->CCER = ccer;

    // set dma, only one channel use dma
    #if (RTE_GPDMA)
    if ((resource->cap & CAP_TIM_GPDMA_MASK) && cfg->gpdma_cfg.en) {
        gpdma_config_t dma_config;

        OM_ASSERT(cfg->gpdma_cfg.chain);
        OM_ASSERT(cfg->gpdma_cfg.tim_chan != TIM_CHAN_ALL);

        // dma config param
        dma_config.channel_ctrl = GPDMA_SET_CTRL(GPDMA_ADDR_CTRL_INC, GPDMA_ADDR_CTRL_FIXED,
                                               GPDMA_TRANS_WIDTH_4B, GPDMA_TRANS_WIDTH_4B, GPDMA_BURST_SIZE_1T,
                                               resource->gpdma_tx.prio ? GPDMA_PRIORITY_HIGH : GPDMA_PRIORITY_LOW);
        dma_config.src_id       = GPDMA_ID_MEM;
        dma_config.dst_id       = (gpdma_id_t)resource->gpdma_tx.id;
        dma_config.isr_cb       = tim_gpdma_isr_cb;
        dma_config.cb_param     = (void *)resource;
        dma_config.chain_trans  = NULL;
        dma_config.chain_trans_num = 0U;

        trans_size_byte = cfg->gpdma_cfg.chain[0].size_byte;
        if (cfg->gpdma_cfg.chain[0].ll_ptr != NULL) {
            dma_config.chain_trans = &cfg->gpdma_cfg.chain[0];

            uint32_t index = 0;
            do {
                dma_config.chain_trans_num++;
                dma_config.chain_trans[index].dst_addr = (uint32_t)&om_tim->CCR[cfg->gpdma_cfg.tim_chan];
                if (cfg->gpdma_cfg.chain[index].ll_ptr == NULL
                        || cfg->gpdma_cfg.chain[index].ll_ptr == &cfg->gpdma_cfg.chain[0]) {
                    break;
                }
                index++;
            } while (true);
            dma_config.chain_trans_num--;
            dma_config.chain_trans = &cfg->gpdma_cfg.chain[1];
        } else {
            dma_config.chain_trans = NULL;
        }

        drv_gpdma_channel_config(env->gpdma_chan, &dma_config);

        // need to cfg chain[0] in circular list
        if (dma_config.chain_trans_num) {
            uint8_t src_width = 1 << register_get(&dma_config.channel_ctrl, MASK_POS(GPDMA_CHAN_CTRL_SRCWIDTH));
            cfg->gpdma_cfg.chain[0].trans_size = cfg->gpdma_cfg.chain[0].size_byte / src_width;
            cfg->gpdma_cfg.chain[0].ctrl = cfg->gpdma_cfg.chain[1].ctrl;
        }

        // trigger dma request enable
        om_tim->DIER |= TIM_DIER_UDE_MASK;

        // disable output compare preload when in dma mode
        switch (cfg->gpdma_cfg.tim_chan) {
            case TIM_CHAN_1:
                om_tim->CCMR1 &= ~TIM_CCMR1_OC1PE_MASK;
                break;
            case TIM_CHAN_2:
                om_tim->CCMR1 &= ~TIM_CCMR1_OC2PE_MASK;
                break;
            case TIM_CHAN_3:
                om_tim->CCMR2 &= ~TIM_CCMR2_OC3PE_MASK;
                break;
            case TIM_CHAN_4:
                om_tim->CCMR2 &= ~TIM_CCMR2_OC4PE_MASK;
                break;
            default:
                OM_ASSERT(0);
                break;
        }

        // enable dma channel
        drv_gpdma_channel_enable(env->gpdma_chan, (uint32_t)&om_tim->CCR[cfg->gpdma_cfg.tim_chan], (uint32_t)cfg->gpdma_cfg.chain[0].src_addr, trans_size_byte);
    }
    #endif  /* RTE_GPDMA */

    if (!(om_tim->CTR1 & TIM_CTR1_CEN_MASK)) {
        om_tim->SR  = 0;
        om_tim->EGR |= TIM_EGR_UG_MASK;
        while (!(om_tim->SR & TIM_SR_UIF_MASK));

        om_tim->CNT = 0;
        om_tim->CTR2 = 0;
        om_tim->SR  = 0;

        // open oc and ocn output
        om_tim->BDTR |= TIM_BDTR_MOE_MASK|cfg->dead_time;
        if (env->isr_cb) {
            om_tim->DIER |= TIM_DIER_UIE_MASK;
        }

        // start
        om_tim->CTR1 |= TIM_CTR1_CEN_MASK;
    }

    return OM_ERROR_OK;
}

/**
 * @brief tim pwm complementary output start
 *
 * @param[in] om_tim      timer instance
 * @param[in] config      tim_pwm_complementary_output_config_t
 *
 * @return status see@om_error_t
 **/
om_error_t drv_tim_pwm_complementary_output_start(OM_TIM_Type *om_tim, tim_pwm_complementary_output_config_t *cfg)
{
    const drv_resource_t *resource;
    tim_env_t            *env;
    uint32_t              tim_clk, psc;
    uint32_t              ccer = 0;
    uint32_t              trans_size_byte;

    OM_ASSERT(cfg);

    resource = tim_get_resource(om_tim);
    if (resource == NULL) {
        return OM_ERROR_RESOURCES;
    }
    env = (tim_env_t *)resource->env;

    // calculate prescaler val
    tim_clk = drv_rcc_clock_get((rcc_clk_t)(size_t)resource->reg);
    psc     = (tim_clk / cfg->cnt_freq) - 1;
    if (psc > TIM_PSC_MAX) {
        return OM_ERROR_OUT_OF_RANGE;
    }

    // update shadow register
    if (!(om_tim->CTR1 & TIM_CTR1_CEN_MASK)) {
        om_tim->DIER    = 0;
        om_tim->CTR1    = TIM_CTR1_ARPE_MASK;
        om_tim->PSC     = psc;
        om_tim->ARR     = cfg->period_cnt - 1;
    }

    // channel config
    for (uint8_t chan_id = TIM_CHAN_1; chan_id < TIM_CHAN_ALL; chan_id++) {
        switch (chan_id) {
            case TIM_CHAN_1:
                if (cfg->chan[chan_id].en) {
                    // pwm mode 1, upcounter, ccrx preload
                    register_set(&om_tim->CCMR1, MASK_3REG(TIM_CCMR1_OC1M,  6,
                                                           TIM_CCMR1_OC1PE, 1,
                                                           TIM_CCMR1_CC1S,  0));

                    register_set(&ccer, MASK_4REG(TIM_CCER_CC1E, 1,TIM_CCER_CC1NE, cfg->chan[chan_id].cfg.complementary_output_enable,
                                                  TIM_CCER_CC1P, cfg->chan[chan_id].cfg.pol,TIM_CCER_CC1NP, cfg->chan[chan_id].cfg.pol));
                    // set output compare val
                    om_tim->CCR[chan_id] = cfg->chan[chan_id].cfg.oc_val;
                } else {
                    // set CCnS input
                    register_set(&om_tim->CCMR1, MASK_1REG(TIM_CCMR1_CC1S, 1));
                    // disable compare
                    ccer &= ~TIM_CCER_CC1E_MASK;
                }
                break;
            case TIM_CHAN_2:
                if (cfg->chan[chan_id].en) {
                    // pwm mode 1, upcounter, ccrx preload
                    register_set(&om_tim->CCMR1, MASK_3REG(TIM_CCMR1_OC2M,  6,
                                                           TIM_CCMR1_OC2PE, 1,
                                                           TIM_CCMR1_CC2S,  0));
                    // compare output enable,set pol
                    register_set(&ccer, MASK_4REG(TIM_CCER_CC2E, 1,TIM_CCER_CC2NE, cfg->chan[chan_id].cfg.complementary_output_enable,
                                                  TIM_CCER_CC2P, cfg->chan[chan_id].cfg.pol,TIM_CCER_CC2NP, cfg->chan[chan_id].cfg.pol));
                    // set output compare val
                    om_tim->CCR[chan_id] = cfg->chan[chan_id].cfg.oc_val;
                } else {
                    // set CCnS input
                    register_set(&om_tim->CCMR1, MASK_1REG(TIM_CCMR1_CC2S, 1));
                    // disable compare
                    ccer &= ~TIM_CCER_CC2E_MASK;
                }
                break;
            case TIM_CHAN_3:
                if (cfg->chan[chan_id].en) {
                    // pwm mode 1, upcounter, ccrx preload
                    register_set(&om_tim->CCMR2, MASK_3REG(TIM_CCMR2_OC3M,  6,
                                                           TIM_CCMR2_OC3PE, 1,
                                                           TIM_CCMR2_CC3S,  0));
                    // compare output enable,set pol
                    register_set(&ccer, MASK_4REG(TIM_CCER_CC3E, 1,TIM_CCER_CC3NE, cfg->chan[chan_id].cfg.complementary_output_enable,
                                                  TIM_CCER_CC3P, cfg->chan[chan_id].cfg.pol,TIM_CCER_CC3NP, cfg->chan[chan_id].cfg.pol));
                    // set output compare val
                    om_tim->CCR[chan_id] = cfg->chan[chan_id].cfg.oc_val;
                } else {
                    // set CCnS input
                    register_set(&om_tim->CCMR2, MASK_1REG(TIM_CCMR2_CC3S, 1));
                    // disable compare
                    ccer &= ~TIM_CCER_CC3E_MASK;
                }
                break;
            case TIM_CHAN_4:
                if (cfg->chan[chan_id].en) {
                    // pwm mode 1, upcounter, ccrx preload
                    register_set(&om_tim->CCMR2, MASK_3REG(TIM_CCMR2_OC4M,  6,
                                                           TIM_CCMR2_OC4PE, 1,
                                                           TIM_CCMR2_CC4S,  0));
                    // compare output enable,set pol
                    register_set(&ccer, MASK_2REG(TIM_CCER_CC4E, 1,
                                                  TIM_CCER_CC4P, cfg->chan[chan_id].cfg.pol));
                    // set output compare val
                    om_tim->CCR[chan_id] = cfg->chan[chan_id].cfg.oc_val;
                } else {
                    // set CCnS input
                    register_set(&om_tim->CCMR2, MASK_1REG(TIM_CCMR2_CC4S, 1));
                    // disable compare
                    ccer &= ~TIM_CCER_CC4E_MASK;
                }
                break;
            default:
                break;
        }
    }

    om_tim->CCER = ccer;

    // set dma, only one channel use dma
    #if (RTE_GPDMA)
    if ((resource->cap & CAP_TIM_GPDMA_MASK) && cfg->gpdma_cfg.en) {
        gpdma_config_t dma_config;

        OM_ASSERT(cfg->gpdma_cfg.chain);
        OM_ASSERT(cfg->gpdma_cfg.tim_chan != TIM_CHAN_ALL);

        // dma config param
        dma_config.channel_ctrl = GPDMA_SET_CTRL(GPDMA_ADDR_CTRL_INC, GPDMA_ADDR_CTRL_FIXED,
                                               GPDMA_TRANS_WIDTH_4B, GPDMA_TRANS_WIDTH_4B, GPDMA_BURST_SIZE_1T,
                                               resource->gpdma_tx.prio ? GPDMA_PRIORITY_HIGH : GPDMA_PRIORITY_LOW);
        dma_config.src_id       = GPDMA_ID_MEM;
        dma_config.dst_id       = (gpdma_id_t)resource->gpdma_tx.id;
        dma_config.isr_cb       = tim_gpdma_isr_cb;
        dma_config.cb_param     = (void *)resource;
        dma_config.chain_trans  = NULL;
        dma_config.chain_trans_num = 0U;

        trans_size_byte = cfg->gpdma_cfg.chain[0].size_byte;
        if (cfg->gpdma_cfg.chain[0].ll_ptr != NULL) {
            dma_config.chain_trans = &cfg->gpdma_cfg.chain[0];

            uint32_t index = 0;
            do {
                dma_config.chain_trans_num++;
                dma_config.chain_trans[index].dst_addr = (uint32_t)&om_tim->CCR[cfg->gpdma_cfg.tim_chan];
                if (cfg->gpdma_cfg.chain[index].ll_ptr == NULL
                        || cfg->gpdma_cfg.chain[index].ll_ptr == &cfg->gpdma_cfg.chain[0]) {
                    break;
                }
                index++;
            } while (true);
            dma_config.chain_trans_num--;
            dma_config.chain_trans = &cfg->gpdma_cfg.chain[1];
        } else {
            dma_config.chain_trans = NULL;
        }

        drv_gpdma_channel_config(env->gpdma_chan, &dma_config);

        // need to cfg chain[0] in circular list
        if (dma_config.chain_trans_num) {
            uint8_t src_width = 1 << register_get(&dma_config.channel_ctrl, MASK_POS(GPDMA_CHAN_CTRL_SRCWIDTH));
            cfg->gpdma_cfg.chain[0].trans_size = cfg->gpdma_cfg.chain[0].size_byte / src_width;
            cfg->gpdma_cfg.chain[0].ctrl = cfg->gpdma_cfg.chain[1].ctrl;
        }

        // trigger dma request enable
        om_tim->DIER |= TIM_DIER_UDE_MASK;

        // disable output compare preload when in dma mode
        switch (cfg->gpdma_cfg.tim_chan) {
            case TIM_CHAN_1:
                om_tim->CCMR1 &= ~TIM_CCMR1_OC1PE_MASK;
                break;
            case TIM_CHAN_2:
                om_tim->CCMR1 &= ~TIM_CCMR1_OC2PE_MASK;
                break;
            case TIM_CHAN_3:
                om_tim->CCMR2 &= ~TIM_CCMR2_OC3PE_MASK;
                break;
            case TIM_CHAN_4:
                om_tim->CCMR2 &= ~TIM_CCMR2_OC4PE_MASK;
                break;
            default:
                OM_ASSERT(0);
                break;
        }

        // enable dma channel
        drv_gpdma_channel_enable(env->gpdma_chan, (uint32_t)&om_tim->CCR[cfg->gpdma_cfg.tim_chan], (uint32_t)cfg->gpdma_cfg.chain[0].src_addr, trans_size_byte);
    }
    #endif  /* RTE_GPDMA */

    if (!(om_tim->CTR1 & TIM_CTR1_CEN_MASK)) {
        om_tim->SR  = 0;
        om_tim->EGR |= TIM_EGR_UG_MASK;
        while (!(om_tim->SR & TIM_SR_UIF_MASK));

        om_tim->CNT = 0;
        om_tim->CTR2 = 0;
        om_tim->SR  = 0;

        // open oc and ocn output
        om_tim->BDTR &= 0xFF00;
        om_tim->BDTR |= TIM_BDTR_MOE_MASK|cfg->dead_time;
        if (env->isr_cb) {
            om_tim->DIER |= TIM_DIER_UIE_MASK;
        }

        // start
        om_tim->CTR1 |= TIM_CTR1_CEN_MASK;
    }

    return OM_ERROR_OK;
}

/**
 * @brief pwm output stop
 *
 * @param[in] om_tim      timer instance
 * @param[in] channel     the tiemr channel, if TIM_CHANNEL_ALL, stop the TIMx.
 *
 * @return None
 **/
void drv_tim_pwm_output_stop(OM_TIM_Type *om_tim, tim_chan_t channel)
{
    const drv_resource_t *resource;
    tim_env_t            *env;

    resource = tim_get_resource(om_tim);
    if (resource == NULL) {
        return;
    }
    env = (tim_env_t *)resource->env;

    if (channel == TIM_CHAN_ALL) {
        om_tim->CCR[0] = 0;
        om_tim->CCR[1] = 0;
        om_tim->CCR[2] = 0;
        om_tim->CCR[3] = 0;
        om_tim->CCER   = 0;
        om_tim->CCMR1  = 0;
        om_tim->CCMR2  = 0;
        #if (RTE_GPDMA)
        if ((resource->cap & CAP_TIM_GPDMA_MASK) && env->tim_chan != TIM_CHAN_ALL) {
            drv_gpdma_channel_disable(env->gpdma_chan);
        }
        #endif
    } else {
        // disable compare val
        om_tim->CCR[channel] = 0;

        // disable compare output en
        switch (channel) {
            case TIM_CHAN_1:
                om_tim->CCER  &= ~(TIM_CCER_CC1E_MASK | TIM_CCER_CC1P_MASK);
                om_tim->CCMR1 &= ~(TIM_CCMR1_OC1M_MASK | TIM_CCMR1_OC1PE_MASK);
                break;
            case TIM_CHAN_2:
                om_tim->CCER  &= ~(TIM_CCER_CC2E_MASK | TIM_CCER_CC2P_MASK);
                om_tim->CCMR1 &= ~(TIM_CCMR1_OC2M_MASK | TIM_CCMR1_OC2PE_MASK);
                break;
            case TIM_CHAN_3:
                om_tim->CCER  &= ~(TIM_CCER_CC3E_MASK | TIM_CCER_CC3P_MASK);
                om_tim->CCMR2 &= ~(TIM_CCMR2_OC3M_MASK | TIM_CCMR2_OC3PE_MASK);
                break;
            case TIM_CHAN_4:
                om_tim->CCER  &= ~(TIM_CCER_CC4E_MASK | TIM_CCER_CC4P_MASK);
                om_tim->CCMR2 &= ~(TIM_CCMR2_OC4M_MASK | TIM_CCMR2_OC4PE_MASK);
                break;
            default:
                break;
        }
        #if (RTE_GPDMA)
        if ((resource->cap & CAP_TIM_GPDMA_MASK) && env->tim_chan == channel) {
            drv_gpdma_channel_disable(env->gpdma_chan);
        }
        #endif
    }

    // check disable all channel
    if (!(om_tim->CCER & (TIM_CCER_CC1E_MASK | TIM_CCER_CC2E_MASK | TIM_CCER_CC3E_MASK | TIM_CCER_CC4E_MASK))) {
        // disable counter
        om_tim->CTR1 &= ~TIM_CTR1_CEN_MASK;
    }
}

/**
 * @brief Force PWM channel output
 *
 * @param[in] om_tim      timer instance
 * @param[in] channel     channel index
 * @param[in] level       force output level
 *
 * @return None
 **/
void drv_tim_pwm_force_output(OM_TIM_Type *om_tim, tim_chan_t channel, tim_force_level_t level)
{
    switch (channel) {
        case TIM_CHAN_1:
            register_set(&om_tim->CCMR1, MASK_1REG(TIM_CCMR1_OC1M, level));
            break;
        case TIM_CHAN_2:
            register_set(&om_tim->CCMR1, MASK_1REG(TIM_CCMR1_OC2M, level));
            break;
        case TIM_CHAN_3:
            register_set(&om_tim->CCMR2, MASK_1REG(TIM_CCMR2_OC3M, level));
            break;
        case TIM_CHAN_4:
            register_set(&om_tim->CCMR2, MASK_1REG(TIM_CCMR2_OC4M, level));
            break;
        default:
            break;
    }
}

/**
 * @brief the timer capture mode start
 *
 * @param[in] om_tim        timer instance, NOTE:only tim0 support capature in 6626
 * @param[in] config        timer_capture_config_t
 *
 * @return status, see@om_error_t
 **/
om_error_t drv_tim_capture_start(OM_TIM_Type *om_tim, const tim_capture_config_t *cfg)
{
    const drv_resource_t *resource;
    tim_env_t            *env;
    uint32_t              tim_clk, psc, tim_arr_max;
    uint32_t              trans_size_byte;
    uint32_t              ccer = 0;
    om_error_t            error;

    OM_ASSERT(cfg);

    resource = tim_get_resource(om_tim);
    if (resource == NULL) {
        return OM_ERROR_RESOURCES;
    }
    env = (tim_env_t *)resource->env;

    if (!(resource->cap & CAP_TIM_CAPTURE_MASK)) {
        return OM_ERROR_UNSUPPORTED;
    }

    if (om_tim->CTR1 & TIM_CTR1_CEN_MASK) {
        return OM_ERROR_BUSY;
    }

    tim_clk = drv_rcc_clock_get((rcc_clk_t)(size_t)resource->reg);
    psc     = (tim_clk / cfg->cnt_freq) - 1;
    if (psc > TIM_PSC_MAX) {
        return OM_ERROR_OUT_OF_RANGE;
    }

    if (om_tim == OM_TIM0) {
        tim_arr_max = TIM_ARR_MAX_32BIT;
    } else if (om_tim == OM_TIM1 || om_tim == OM_TIM2) {
        tim_arr_max = TIM_ARR_MAX_16BIT;
    } else {
        return false;
    }

    om_tim->DIER  = 0;
    om_tim->CTR1   = 0;
    om_tim->PSC   = psc;
    om_tim->ARR   = tim_arr_max;
    om_tim->CNT   = 0;
    om_tim->SR    = 0;

    register_set(&om_tim->CCMR1, MASK_2REG(TIM_CCMR1_CC1S, 1,
                                           TIM_CCMR1_CC2S, 1));
    register_set(&om_tim->CCMR2, MASK_2REG(TIM_CCMR2_CC3S, 1,
                                           TIM_CCMR2_CC4S, 1));

    /* capture enable and polarities setup */
    for (tim_chan_t chan_id = TIM_CHAN_1; chan_id < TIM_CHAN_ALL; chan_id++) {
        switch (chan_id) {
            case TIM_CHAN_1:
                if (cfg->chan[chan_id].en) {
                    ccer |= TIM_CCER_CC1E_MASK;
                    if (cfg->chan[chan_id].pol == TIM_CAPTURE_POLARITY_RISING_EDGE) {
                        ccer &= ~TIM_CCER_CC1P_MASK;
                    } else {
                        ccer |= TIM_CCER_CC1P_MASK;
                    }
                    om_tim->DIER |= TIM_DIER_CC1IE_MASK;
                }
                break;
            case TIM_CHAN_2:
                if (cfg->chan[chan_id].en) {
                    ccer |= TIM_CCER_CC2E_MASK;
                    if (cfg->chan[chan_id].pol == TIM_CAPTURE_POLARITY_RISING_EDGE) {
                        ccer &= ~TIM_CCER_CC2P_MASK;
                    } else {
                        ccer |= TIM_CCER_CC2P_MASK;
                    }
                    om_tim->DIER |= TIM_DIER_CC2IE_MASK;
                }
                break;
            case TIM_CHAN_3:
                if (cfg->chan[chan_id].en) {
                    ccer |= TIM_CCER_CC3E_MASK;
                    if (cfg->chan[chan_id].pol == TIM_CAPTURE_POLARITY_RISING_EDGE) {
                        ccer &= ~TIM_CCER_CC3P_MASK;
                    } else {
                        ccer |= TIM_CCER_CC3P_MASK;
                    }
                    om_tim->DIER |= TIM_DIER_CC3IE_MASK;
                }
                break;
            case TIM_CHAN_4:
                if (cfg->chan[chan_id].en) {
                    ccer |= TIM_CCER_CC4E_MASK;
                    if (cfg->chan[chan_id].pol == TIM_CAPTURE_POLARITY_RISING_EDGE) {
                        ccer &= ~TIM_CCER_CC4P_MASK;
                    } else {
                        ccer |= TIM_CCER_CC4P_MASK;
                    }
                    om_tim->DIER |= TIM_DIER_CC4IE_MASK;
                }
                break;
            default:
                break;
        }
    }

    #if (RTE_GPDMA)
    if ((resource->cap & CAP_TIM_GPDMA_MASK) && cfg->gpdma_cfg.en) {
        gpdma_config_t dma_cfg;

        OM_ASSERT(cfg->gpdma_cfg.chain);
        OM_ASSERT(cfg->gpdma_cfg.tim_chan != TIM_CHAN_ALL);

        dma_cfg.channel_ctrl    = GPDMA_SET_CTRL(GPDMA_ADDR_CTRL_FIXED, GPDMA_ADDR_CTRL_INC,
                                               GPDMA_TRANS_WIDTH_4B, GPDMA_TRANS_WIDTH_4B, GPDMA_BURST_SIZE_1T,
                                               resource->gpdma_rx.prio ? GPDMA_PRIORITY_HIGH : GPDMA_PRIORITY_LOW);
        dma_cfg.src_id          = (gpdma_id_t)resource->gpdma_rx.id;
        dma_cfg.dst_id          = GPDMA_ID_MEM;
        dma_cfg.isr_cb          = tim_gpdma_isr_cb;
        dma_cfg.cb_param        = (void *)resource;
        dma_cfg.chain_trans     = NULL;
        dma_cfg.chain_trans_num = 0U;

        trans_size_byte = cfg->gpdma_cfg.chain[0].size_byte;
        if (cfg->gpdma_cfg.chain[0].ll_ptr != NULL) {
            dma_cfg.chain_trans = &cfg->gpdma_cfg.chain[0];

            uint32_t index = 0;
            do {
                dma_cfg.chain_trans_num++;
                dma_cfg.chain_trans[index].src_addr = (uint32_t)&om_tim->CCR[cfg->gpdma_cfg.tim_chan];
                if (cfg->gpdma_cfg.chain[index].ll_ptr == NULL
                        || cfg->gpdma_cfg.chain[index].ll_ptr == &cfg->gpdma_cfg.chain[0]) {
                    break;
                }
                index++;
            } while (true);
            dma_cfg.chain_trans_num--;
            dma_cfg.chain_trans = &cfg->gpdma_cfg.chain[1];
        } else {
            dma_cfg.chain_trans = NULL;
        }

        error = drv_gpdma_channel_config(env->gpdma_chan, &dma_cfg);
        if (error != OM_ERROR_OK) {
            return error;
        }

        // need to cfg chain[0] in circular list
        if (dma_cfg.chain_trans_num) {
            uint8_t src_width = 1 << register_get(&dma_cfg.channel_ctrl, MASK_POS(GPDMA_CHAN_CTRL_SRCWIDTH));
            cfg->gpdma_cfg.chain[0].trans_size = cfg->gpdma_cfg.chain[0].size_byte / src_width;
            cfg->gpdma_cfg.chain[0].ctrl = cfg->gpdma_cfg.chain[1].ctrl;
        }

        switch (cfg->gpdma_cfg.tim_chan) {
            case TIM_CHAN_1:
                om_tim->DIER |= TIM_DIER_CC1DE_MASK;
                break;
            case TIM_CHAN_2:
                om_tim->DIER |= TIM_DIER_CC2DE_MASK;
                break;
            case TIM_CHAN_3:
                om_tim->DIER |= TIM_DIER_CC3DE_MASK;
                break;
            case TIM_CHAN_4:
                om_tim->DIER |= TIM_DIER_CC4DE_MASK;
                break;
            default:
                break;
        }

        env->tim_chan = cfg->gpdma_cfg.tim_chan;

        drv_gpdma_channel_enable(env->gpdma_chan, (uint32_t)cfg->gpdma_cfg.chain[0].dst_addr, (uint32_t)&om_tim->CCR[cfg->gpdma_cfg.tim_chan], trans_size_byte);
    }
    #endif

    om_tim->CTR1 |= TIM_CTR1_CEN_MASK;
    om_tim->DIER |= TIM_DIER_UIE_MASK;
    om_tim->CCER |= ccer;

    return OM_ERROR_OK;
}

/**
 * @brief timer capture mode stop
 *
 * @param[in] om_tim        timer instance
 * @param[in] channel       the tiemr channel, if TIM_CHANNEL_ALL, stop the TIMx.
 *
 * @return None
 **/
void drv_tim_capture_stop(OM_TIM_Type *om_tim, tim_chan_t channel)
{
    const drv_resource_t *resource;
    tim_env_t            *env;

    resource = tim_get_resource(om_tim);
    if (resource == NULL) {
        return;
    }
    env = (tim_env_t *)resource->env;

    if (channel == TIM_CHAN_ALL) {
        om_tim->CCR[0] = 0;
        om_tim->CCR[1] = 0;
        om_tim->CCR[2] = 0;
        om_tim->CCR[3] = 0;
        om_tim->CCER   = 0;
        om_tim->CCMR1  = 0;
        om_tim->CCMR2  = 0;

        #if (RTE_GPDMA)
        if ((resource->cap & CAP_TIM_GPDMA_MASK) && env->tim_chan != TIM_CHAN_ALL) {
            drv_gpdma_channel_disable(env->gpdma_chan);
        }
        #endif
    } else {
        // disable compare val
        om_tim->CCR[channel] = 0;

        // disable compare output en
        switch (channel) {
            case TIM_CHAN_1:
                om_tim->CCER  &= ~(TIM_CCER_CC1E_MASK | TIM_CCER_CC1P_MASK);
                om_tim->CCMR1 &= ~(TIM_CCMR1_OC1M_MASK | TIM_CCMR1_OC1PE_MASK);
                break;
            case TIM_CHAN_2:
                om_tim->CCER  &= ~(TIM_CCER_CC2E_MASK | TIM_CCER_CC2P_MASK);
                om_tim->CCMR1 &= ~(TIM_CCMR1_OC2M_MASK | TIM_CCMR1_OC2PE_MASK);
                break;
            case TIM_CHAN_3:
                om_tim->CCER  &= ~(TIM_CCER_CC3E_MASK | TIM_CCER_CC3P_MASK);
                om_tim->CCMR2 &= ~(TIM_CCMR2_OC3M_MASK | TIM_CCMR2_OC3PE_MASK);
                break;
            case TIM_CHAN_4:
                om_tim->CCER  &= ~(TIM_CCER_CC4E_MASK | TIM_CCER_CC4P_MASK);
                om_tim->CCMR2 &= ~(TIM_CCMR2_OC4M_MASK | TIM_CCMR2_OC4PE_MASK);
                break;
            default:
                break;
        }
        #if (RTE_GPDMA)
        if ((resource->cap & CAP_TIM_GPDMA_MASK) && env->tim_chan == channel) {
            drv_gpdma_channel_disable(env->gpdma_chan);
        }
        #endif
    }

    // check disable all channel
    if (!(om_tim->CCER & (TIM_CCER_CC1E_MASK | TIM_CCER_CC2E_MASK | TIM_CCER_CC3E_MASK | TIM_CCER_CC4E_MASK))) {
        // disable counter
        om_tim->CTR1 &= ~TIM_CTR1_CEN_MASK;
    }
}

/**
 * @brief timer pwm input mode start
 *
 * @param[in] om_tim        timer instance
 * @param[in] config        pwm input mode config
 *
 * @return status, see@om_error_t
 **/
om_error_t drv_tim_pwm_input_start(OM_TIM_Type *om_tim, const tim_pwm_input_config_t *cfg)
{
    const drv_resource_t *resource;
    uint32_t tim_clk, psc, tim_arr_max;

    OM_ASSERT(cfg);

    if (om_tim->CTR1 & TIM_CTR1_CEN_MASK) {
        return OM_ERROR_BUSY;
    }

    resource = tim_get_resource(om_tim);
    if (resource == NULL) {
        return OM_ERROR_RESOURCES;
    }

    tim_clk = drv_rcc_clock_get((rcc_clk_t)(size_t)resource->reg);
    psc     = (tim_clk / cfg->cnt_freq) - 1;
    if (psc > TIM_PSC_MAX) {
        return OM_ERROR_OUT_OF_RANGE;
    }

    if (om_tim == OM_TIM0) {
        tim_arr_max = TIM_ARR_MAX_32BIT;
    } else if (om_tim == OM_TIM1 || om_tim == OM_TIM2) {
        tim_arr_max = TIM_ARR_MAX_16BIT;
    } else {
        return false;
    }

    om_tim->DIER = 0;
    om_tim->CTR1 |= TIM_CTR1_URS_MASK;
    om_tim->PSC  = psc;
    om_tim->ARR  = tim_arr_max;
    om_tim->CNT  = 0;
    om_tim->SR   = 0;

    // IC1->TI1, IC2->TI1
    register_set(&om_tim->CCMR1, MASK_2REG(TIM_CCMR1_CC1S, 1,
                                           TIM_CCMR1_CC2S, 2));
    register_set(&om_tim->CCMR2, MASK_2REG(TIM_CCMR2_CC3S, 1,
                                           TIM_CCMR2_CC4S, 2));
    // CC1 falling, CC2 rising edge
    register_set(&om_tim->CCER, MASK_2REG(TIM_CCER_CC1P, 1,
                                          TIM_CCER_CC2P, 0));
    // select TRGI use TI1FP1, slave mode->reset mode
    register_set(&om_tim->SMCR, MASK_2REG(TIM_SMCR_TS,  5,
                                          TIM_SMCR_SMS, 4));
    // enable CC1,CC2 irq and update event
    om_tim->DIER |= TIM_DIER_CC1IE_MASK | TIM_DIER_CC2IE_MASK | TIM_DIER_UIE_MASK;
    // start
    om_tim->CTR1 |= TIM_CTR1_CEN_MASK;
    // enable capture
    om_tim->CCER |= TIM_CCER_CC1E_MASK | TIM_CCER_CC2E_MASK;

    return OM_ERROR_OK;
}

/**
 * @brief timer pwm input mode stop
 *
 * @param[in] om_tim      timer instance
 *
 * @return None
 **/
void drv_tim_pwm_input_stop(OM_TIM_Type *om_tim)
{
    const drv_resource_t *resource;

    resource = tim_get_resource(om_tim);
    if (resource == NULL) {
        return;
    }

    om_tim->CTR1     = 0;
    om_tim->DIER    = 0;
    om_tim->SR      = 0;
    om_tim->SMCR    = 0;
    om_tim->CCER    = 0;
    om_tim->CCMR1   = 0;
    om_tim->CCMR2   = 0;

}

/**
 * @brief timer interrupt service routine
 *
 * @param[in] om_tim      timer instance
 *
 * @return None
 **/
void drv_tim_isr(OM_TIM_Type *om_tim)
{
    const drv_resource_t *resource;
    tim_env_t            *env;
    uint32_t              status;
    uint32_t              event;

    resource = tim_get_resource(om_tim);
    if (resource == NULL) {
        return;
    }
    env = (tim_env_t *)resource->env;

    // get status and clear
    status = om_tim->SR;
    om_tim->SR = 0;

    if (status & TIM_SR_UIF_MASK) {
        if (env->isr_cb) {
            env->isr_cb(om_tim, DRV_EVENT_TIM_UPDATE, NULL, NULL);
        }
    }

    if (status & TIM_SR_TIF_MASK || status & (TIM_SR_CC1OF_MASK | TIM_SR_CC2OF_MASK | TIM_SR_CC3OF_MASK | TIM_SR_CC4OF_MASK)) {
        event = (status & (TIM_SR_TIF_MASK | TIM_SR_CC1OF_MASK| TIM_SR_CC2OF_MASK| TIM_SR_CC3OF_MASK| TIM_SR_CC4OF_MASK)) << 15;
        if (env->isr_cb) {
            env->isr_cb(om_tim, (drv_event_t)event, NULL, NULL);
        }
    }

    // CCx capture irq, pwm input must use CC1 and CC2
    if (status & TIM_SR_CC1IF_MASK) {
        if (env->isr_cb) {
            env->isr_cb(om_tim, DRV_EVENT_TIM_CC1, (void *)om_tim->CCR[0], (void *)1);
        }
    }
    if (status & TIM_SR_CC2IF_MASK) {
        if (env->isr_cb) {
            env->isr_cb(om_tim, DRV_EVENT_TIM_CC2, (void *)om_tim->CCR[1], (void *)1);
        }
    }
    if (status & TIM_SR_CC3IF_MASK) {
        if (env->isr_cb) {
            env->isr_cb(om_tim, DRV_EVENT_TIM_CC3, (void *)om_tim->CCR[2], (void *)1);
        }
    }
    if (status & TIM_SR_CC4IF_MASK) {
        if (env->isr_cb) {
            env->isr_cb(om_tim, DRV_EVENT_TIM_CC4, (void *)om_tim->CCR[3], (void *)1);
        }
    }
}

#if (RTE_GPDMA)
om_error_t drv_tim_gpdma_channel_allocate(OM_TIM_Type *om_tim)
{
    const drv_resource_t  *resource;
    tim_env_t             *env;

    resource = tim_get_resource(om_tim);
    if (resource == NULL) {
        return OM_ERROR_PARAMETER;
    }
    env = (tim_env_t *)resource->env;

    if (env->gpdma_chan >= GPDMA_NUMBER_OF_CHANNELS) {
        env->gpdma_chan = drv_gpdma_channel_allocate();
        if (env->gpdma_chan >= GPDMA_NUMBER_OF_CHANNELS) {
            return OM_ERROR_RESOURCES;
        }
    }

    return OM_ERROR_OK;
}

om_error_t drv_tim_gpdma_channel_release(OM_TIM_Type *om_tim)
{
    const drv_resource_t  *resource;
    tim_env_t             *env;

    resource = tim_get_resource(om_tim);
    if (resource == NULL) {
        return OM_ERROR_PARAMETER;
    }
    env = (tim_env_t *)resource->env;
    if (env->gpdma_chan < GPDMA_NUMBER_OF_CHANNELS) {
        drv_gpdma_channel_release(env->gpdma_chan);
        env->gpdma_chan = GPDMA_NUMBER_OF_CHANNELS;
    }
    env->tim_chan = TIM_CHAN_ALL;

    return OM_ERROR_OK;
}
#endif

#endif  /* (RTE_TIM0 || RTE_TIM1 || RTE_TIM2) */

/** @} */
