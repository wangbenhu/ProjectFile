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
 * @brief    LPTIMER driver source file
 * @details  LPTIMER driver source file
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
#if (RTE_LPTIM)
#include <stddef.h>
#include "om_driver.h"


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    drv_isr_callback_t  isr_cb;
} lptim_env_t;


#if (RTE_LPTIM_REGISTER_CALLBACK)
/*******************************************************************************
 * CONST & VARIABLES
 */
static lptim_env_t lptim_env;


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Register isr callback for SPI command is complete by LPTIM interrupt mode
 *
 * @param[in] om_lptim      Pointer to LPTIM
 * @param[in] cb          Pointer to callback
 *
 *******************************************************************************
 */
void drv_lptim_register_isr_callback(OM_LPTIM_Type *om_lptim, drv_isr_callback_t cb)
{
    (void)om_lptim;
    lptim_env.isr_cb = cb;
}
#endif

__WEAK void drv_lptim_isr_callback(OM_LPTIM_Type *om_lptim, drv_event_t event, void *param0, void *param1)
{
    #if (RTE_LPTIM_REGISTER_CALLBACK)
    if (lptim_env.isr_cb) {
        lptim_env.isr_cb(om_lptim, event, param0, param1);
    }
    #endif
}

/**
 * @brief lp tim free-running config
 *
 * @param[in] om_lptim     lp timer instance
 * @param[in] cfg           free-running config
 *
 * @return status, see@om_error_t
 **/
om_error_t drv_lptim_free_running_init(OM_LPTIM_Type *om_lptim, const lptim_free_running_config_t *cfg)
{
    // open lptim clk and reset registers
    DRV_RCC_RESET(RCC_CLK_LPTIM);

    // enable lptim
    drv_lptim_control(om_lptim, LPTIM_CONTROL_ENABLE, (void *)1);

    // config mode, top_en, prescaler
    register_set(&om_lptim->CTRL, MASK_5REG(LPTIM_CTRL_STOPMODE,  cfg->stop_mode,
                                            LPTIM_CTRL_REPMODE01, LPTIM_MODE_FREE_RUNNING,
                                            LPTIM_CTRL_REPMODE23, LPTIM_MODE_FREE_RUNNING,
                                            LPTIM_CTRL_CNTTOPEN,  cfg->top_en,
                                            LPTIM_CTRL_CNTPRESC,  cfg->presclar));

    // set top val
    drv_lptim_control(om_lptim, LPTIM_CONTROL_TOP_SET, (void *)(uint32_t)cfg->top_val);

    // set compare val
    om_lptim->COMP0 = cfg->compare_val0;
    om_lptim->COMP1 = cfg->compare_val1;
    om_lptim->COMP2 = cfg->compare_val2;
    om_lptim->COMP3 = cfg->compare_val3;

   // enable interrupt
    // om_lptim->INTE |= LPTIM_INTE_UF_EN_MASK | LPTIM_INTE_COMP0_EN_MASK | LPTIM_INTE_COMP1_EN_MASK | LPTIM_INTE_COMP2_EN_MASK | LPTIM_INTE_COMP3_EN_MASK;

    return OM_ERROR_OK;
}

/**
 * @brief lp tim one shot config
 *
 * @param[in] om_lptim     lp timer instance
 * @param[in] cfg           one shot config
 *
 * @return status, see@om_error_t
 **/
om_error_t drv_lptim_one_shot_init(OM_LPTIM_Type *om_lptim, const lptim_one_shot_config_t *cfg)
{
    // open lptim clk and reset registers
    DRV_RCC_RESET(RCC_CLK_LPTIM);

    // enable lptim
    drv_lptim_control(om_lptim, LPTIM_CONTROL_ENABLE, (void *)1);

    // config mode, top_en, prescaler
    register_set(&om_lptim->CTRL, MASK_5REG(LPTIM_CTRL_STOPMODE,  cfg->stop_mode,
                                            LPTIM_CTRL_REPMODE01, LPTIM_MODE_ONE_SHOT,
                                            LPTIM_CTRL_REPMODE23, LPTIM_MODE_ONE_SHOT,
                                            LPTIM_CTRL_CNTTOPEN,  cfg->top_en,
                                            LPTIM_CTRL_CNTPRESC,  cfg->presclar));

    // set top val
    drv_lptim_control(om_lptim, LPTIM_CONTROL_TOP_SET, (void *)(uint32_t)cfg->top_val);
    // set rep0 val
    drv_lptim_control(om_lptim, LPTIM_CONTROL_REP0_SET, (void *)(uint32_t)cfg->rep0_val);
    // set rep2 val
    drv_lptim_control(om_lptim, LPTIM_CONTROL_REP2_SET, (void *)(uint32_t)cfg->rep2_val);

    // set compare val
    om_lptim->COMP0 = cfg->compare_val0;
    om_lptim->COMP1 = cfg->compare_val1;
    om_lptim->COMP2 = cfg->compare_val2;
    om_lptim->COMP3 = cfg->compare_val3;

   // enable interrupt
    // om_lptim->INTE |= LPTIM_INTE_UF_EN_MASK | LPTIM_INTE_COMP0_EN_MASK | LPTIM_INTE_COMP1_EN_MASK | LPTIM_INTE_COMP2_EN_MASK | LPTIM_INTE_COMP3_EN_MASK | LPTIM_INTE_REP0_EN_MASK | LPTIM_INTE_REP2_EN_MASK;

    return OM_ERROR_OK;
}

/**
 * @brief lp tim buffered config
 *
 * @param[in] om_lptim     lp timer instance
 * @param[in] cfg           buffered config
 *
 * @return status, see@om_error_t
 **/
om_error_t drv_lptim_buffered_init(OM_LPTIM_Type *om_lptim, const lptim_buffered_config_t *cfg)
{
    // open lptim clk and reset registers
    DRV_RCC_RESET(RCC_CLK_LPTIM);

    // enable lptim
    drv_lptim_control(om_lptim, LPTIM_CONTROL_ENABLE, (void *)1);

    // config mode, top_en, prescaler
    register_set(&om_lptim->CTRL, MASK_6REG(LPTIM_CTRL_STOPMODE,  cfg->stop_mode,
                                            LPTIM_CTRL_REPMODE01, LPTIM_MODE_BUFFERED,
                                            LPTIM_CTRL_REPMODE23, LPTIM_MODE_BUFFERED,
                                            LPTIM_CTRL_CNTTOPEN,  cfg->top_en,
                                            LPTIM_CTRL_BUFTOP,    cfg->buftop_en,
                                            LPTIM_CTRL_CNTPRESC,  cfg->presclar));
    // set top val
    drv_lptim_control(om_lptim, LPTIM_CONTROL_TOP_SET, (void *)(uint32_t)cfg->top_val);
    // set topbuff
    om_lptim->TOPBUFF = cfg->buftop_val;
    // set rep0,rep1,rep2,rep3 val
    drv_lptim_control(om_lptim, LPTIM_CONTROL_REP0_SET, (void *)(uint32_t)cfg->rep0_val);
    drv_lptim_control(om_lptim, LPTIM_CONTROL_REP1_SET, (void *)(uint32_t)cfg->rep1_val);
    drv_lptim_control(om_lptim, LPTIM_CONTROL_REP2_SET, (void *)(uint32_t)cfg->rep2_val);
    drv_lptim_control(om_lptim, LPTIM_CONTROL_REP3_SET, (void *)(uint32_t)cfg->rep3_val);

    // set compare val
    om_lptim->COMP0 = cfg->compare_val0;
    om_lptim->COMP1 = cfg->compare_val1;
    om_lptim->COMP2 = cfg->compare_val2;
    om_lptim->COMP3 = cfg->compare_val3;

   // enable interrupt
    // om_lptim->INTE |= LPTIM_INTE_UF_EN_MASK | LPTIM_INTE_COMP0_EN_MASK | LPTIM_INTE_COMP1_EN_MASK | LPTIM_INTE_COMP2_EN_MASK | LPTIM_INTE_COMP3_EN_MASK | LPTIM_INTE_REP0_EN_MASK | LPTIM_INTE_REP2_EN_MASK;

    return OM_ERROR_OK;
}

/**
 * @brief lp tim double config
 * NOTE: digital bug:if clear rep0/rep1's interrup flag, the out0/1 will continue
 * output, now just support output the max times of repetition
 *
 * @param[in] om_lptim     lp timer instance
 * @param[in] cfg           double config
 *
 * @return status, see@om_error_t
 **/
om_error_t drv_lptim_double_init(OM_LPTIM_Type *om_lptim, const lptim_double_config_t *cfg)
{
    // open lptim clk and reset registers
    DRV_RCC_RESET(RCC_CLK_LPTIM);

    // enable lptim
    drv_lptim_control(om_lptim, LPTIM_CONTROL_ENABLE, (void *)1);

    // config mode, top_en, prescaler
    register_set(&om_lptim->CTRL, MASK_5REG(LPTIM_CTRL_STOPMODE,  cfg->stop_mode,
                                            LPTIM_CTRL_REPMODE01, LPTIM_MODE_DOUBLE,
                                            LPTIM_CTRL_REPMODE23, LPTIM_MODE_DOUBLE,
                                            LPTIM_CTRL_CNTTOPEN,  cfg->top_en,
                                            LPTIM_CTRL_CNTPRESC,  cfg->presclar));
    // set top val
    drv_lptim_control(om_lptim, LPTIM_CONTROL_TOP_SET, (void *)(uint32_t)cfg->top_val);
    // set rep0,rep1,rep2,rep3 val
    drv_lptim_control(om_lptim, LPTIM_CONTROL_REP0_SET, (void *)(uint32_t)cfg->rep0_val);
    drv_lptim_control(om_lptim, LPTIM_CONTROL_REP1_SET, (void *)(uint32_t)cfg->rep1_val);
    drv_lptim_control(om_lptim, LPTIM_CONTROL_REP2_SET, (void *)(uint32_t)cfg->rep2_val);
    drv_lptim_control(om_lptim, LPTIM_CONTROL_REP3_SET, (void *)(uint32_t)cfg->rep3_val);

    // set compare val
    om_lptim->COMP0 = cfg->compare_val0;
    om_lptim->COMP1 = cfg->compare_val1;
    om_lptim->COMP2 = cfg->compare_val2;
    om_lptim->COMP3 = cfg->compare_val3;

   // enable interrupt
    // om_lptim->INTE |= LPTIM_INTE_UF_EN_MASK | LPTIM_INTE_COMP0_EN_MASK | LPTIM_INTE_COMP1_EN_MASK | LPTIM_INTE_COMP2_EN_MASK | LPTIM_INTE_COMP3_EN_MASK | LPTIM_INTE_REP0_EN_MASK | LPTIM_INTE_REP1_EN_MASK | LPTIM_INTE_REP2_EN_MASK | LPTIM_INTE_REP3_EN_MASK;

    return OM_ERROR_OK;
}

/**
 * @brief lp tim multimode config
 *
 * @param[in] om_lptim     lp timer instance
 * @param[in] cfg           multimode config
 *
 * @return status, see@om_error_t
 **/
om_error_t drv_lptim_multimode_init(OM_LPTIM_Type *om_lptim, const lptim_multimode_config_t *cfg)
{
    // open lptim clk and reset registers
    DRV_RCC_RESET(RCC_CLK_LPTIM);

    // enable lptim
    drv_lptim_control(om_lptim, LPTIM_CONTROL_ENABLE, (void *)1);

    // config mode, top_en, prescaler
    register_set(&om_lptim->CTRL, MASK_6REG(LPTIM_CTRL_STOPMODE,  cfg->stop_mode,
                                            LPTIM_CTRL_REPMODE01, cfg->rep01_mode,
                                            LPTIM_CTRL_REPMODE23, cfg->rep23_mode,
                                            LPTIM_CTRL_CNTTOPEN,  cfg->top_en,
                                            LPTIM_CTRL_BUFTOP,    cfg->buftop_en,
                                            LPTIM_CTRL_CNTPRESC,  cfg->presclar));
    // set top val
    drv_lptim_control(om_lptim, LPTIM_CONTROL_TOP_SET, (void *)(uint32_t)cfg->top_val);
    // set topbuff
    om_lptim->TOPBUFF = cfg->buftop_val;
    // set rep0,rep1,rep2,rep3 val
    drv_lptim_control(om_lptim, LPTIM_CONTROL_REP0_SET, (void *)(uint32_t)cfg->rep0_val);
    drv_lptim_control(om_lptim, LPTIM_CONTROL_REP1_SET, (void *)(uint32_t)cfg->rep1_val);
    drv_lptim_control(om_lptim, LPTIM_CONTROL_REP2_SET, (void *)(uint32_t)cfg->rep2_val);
    drv_lptim_control(om_lptim, LPTIM_CONTROL_REP3_SET, (void *)(uint32_t)cfg->rep3_val);

    // set compare val
    om_lptim->COMP0 = cfg->compare_val0;
    om_lptim->COMP1 = cfg->compare_val1;
    om_lptim->COMP2 = cfg->compare_val2;
    om_lptim->COMP3 = cfg->compare_val3;

    // enable interrupt
    // om_lptim->INTE |= LPTIM_INTE_UF_EN_MASK | LPTIM_INTE_COMP0_EN_MASK | LPTIM_INTE_COMP1_EN_MASK | LPTIM_INTE_COMP2_EN_MASK | LPTIM_INTE_COMP3_EN_MASK | LPTIM_INTE_REP0_EN_MASK | LPTIM_INTE_REP1_EN_MASK | LPTIM_INTE_REP2_EN_MASK | LPTIM_INTE_REP3_EN_MASK;

    return OM_ERROR_OK;
}

/**
 * @brief lp tim out config
 *
 * @param[in] om_lptim     lp timer instance
 * @param[in] chan_outx     output channel(0,1)
 * @param[in] cfg           out config
 *
 * @return status, see@om_error_t
 **/
om_error_t drv_lptim_outx_config(OM_LPTIM_Type *om_lptim, lptim_chan_out_t chan_outx, const lptim_out_config_t *cfg)
{
    if (chan_outx == LPTIM_CHAN_OUT0) {
        register_set(&om_lptim->CH0_CFG, MASK_2REG(LPTIM_CH0_CFG_OPOL, cfg->pol,
                                                   LPTIM_CH0_CFG_UFOA, cfg->action));
    } else if (chan_outx == LPTIM_CHAN_OUT1) {
        register_set(&om_lptim->CH1_CFG, MASK_2REG(LPTIM_CH1_CFG_OPOL, cfg->pol,
                                                   LPTIM_CH1_CFG_UFOA, cfg->action));
    } else if (chan_outx == LPTIM_CHAN_OUT2) {
        register_set(&om_lptim->CH2_CFG, MASK_2REG(LPTIM_CH2_CFG_OPOL, cfg->pol,
                                                   LPTIM_CH2_CFG_UFOA, cfg->action));
    } else if (chan_outx == LPTIM_CHAN_OUT3) {
        register_set(&om_lptim->CH3_CFG, MASK_2REG(LPTIM_CH3_CFG_OPOL, cfg->pol,
                                                   LPTIM_CH3_CFG_UFOA, cfg->action));
    } else {
        return OM_ERROR_PARAMETER;
    }

    return OM_ERROR_OK;
}

/**
 * @brief lp tim control
 *
 * @param[in] om_lptim     lp timer instance
 * @param[in] ctrl          control command
 * @param[in] argu          argument
 *
 * @return None
 **/
void *drv_lptim_control(OM_LPTIM_Type *om_lptim, lptim_control_t ctrl, void *argu)
{
    uint32_t ret;
    ret = (uint32_t)OM_ERROR_OK;

    OM_CRITICAL_BEGIN();
    switch (ctrl) {
        case  LPTIM_CONTROL_ENABLE:
            if (argu) {
                om_lptim->EN |= LPTIM_EN_MASK;
            } else {
                om_lptim->EN &= ~LPTIM_EN_MASK;
            }
            break;
        case LPTIM_CONTROL_START:
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_STOP_MASK);
            om_lptim->CMD &= ~LPTIM_CMD_STOP_MASK;
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_STOP_MASK);

            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_START_MASK);
            om_lptim->CMD |= LPTIM_CMD_START_MASK;
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_START_MASK);
            break;
        case LPTIM_CONTROL_STOP:
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_STOP_MASK);
            do {
                om_lptim->CMD |= LPTIM_CMD_STOP_MASK;
                while (om_lptim->CMD & LPTIM_CMD_STOP_MASK);
            } while (om_lptim->CMD & LPTIM_CMD_START_MASK);
            break;
        case LPTIM_CONTROL_CLEAR:
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_CLEAR_MASK);
            om_lptim->CMD |= LPTIM_CMD_CLEAR_MASK;
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_CLEAR_MASK);
            break;
        case LPTIM_CONTROL_CTO0:
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_CTO0_MASK);
            om_lptim->CMD |= LPTIM_CMD_CTO0_MASK;
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_CTO0_MASK);
            break;
        case LPTIM_CONTROL_CTO1:
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_CTO1_MASK);
            om_lptim->CMD |= LPTIM_CMD_CTO1_MASK;
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_CTO1_MASK);
            break;
        case LPTIM_CONTROL_CTO2:
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_CTO2_MASK);
            om_lptim->CMD |= LPTIM_CMD_CTO2_MASK;
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_CTO2_MASK);
            break;
        case LPTIM_CONTROL_CTO3:
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_CTO3_MASK);
            om_lptim->CMD |= LPTIM_CMD_CTO3_MASK;
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_CTO3_MASK);
            break;
        case LPTIM_CONTROL_CNT_SET:
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_CNT_BUSY_MASK);
            om_lptim->CNT = (uint32_t)argu;
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_CNT_BUSY_MASK);
            break;
        case LPTIM_CONTROL_TOP_SET:
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_TOP_BUSY_MASK);
            om_lptim->TOP = (uint32_t)argu;
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_TOP_BUSY_MASK);
            break;
        case LPTIM_CONTROL_REP0_SET:
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_REP0_BUSY_MASK);
            om_lptim->REP0 = (uint32_t)argu;
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_REP0_BUSY_MASK);
            break;
        case LPTIM_CONTROL_REP1_SET:
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_REP1_BUSY_MASK);
            om_lptim->REP1 = (uint32_t)argu;
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_REP1_BUSY_MASK);
            break;
        case LPTIM_CONTROL_REP2_SET:
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_REP2_BUSY_MASK);
            om_lptim->REP2 = (uint32_t)argu;
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_REP2_BUSY_MASK);
            break;
        case LPTIM_CONTROL_REP3_SET:
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_REP3_BUSY_MASK);
            om_lptim->REP3 = (uint32_t)argu;
            while (om_lptim->SYNCBUSY & LPTIM_SYNCBUSY_REP3_BUSY_MASK);
            break;
        case LPTIM_CONTROL_POWER_IN_SLEEP:
            if (argu) {
                OM_PMU->PSO_PM |= PMU_PSO_LPTIM_POWER_ON_MASK;
            } else {
                OM_PMU->PSO_PM &= ~PMU_PSO_LPTIM_POWER_ON_MASK;
            }
            break;
        case LPTIM_CONTROL_INT_EN:
            if (argu) {
                NVIC_ClearPendingIRQ(LPTIM_IRQn);
                NVIC_SetPriority(LPTIM_IRQn, RTE_LPTIM_IRQ_PRIORITY);
                NVIC_EnableIRQ(LPTIM_IRQn);
            } else {
                NVIC_DisableIRQ(LPTIM_IRQn);
                NVIC_ClearPendingIRQ(LPTIM_IRQn);
            }
            om_lptim->INTE = (uint32_t)argu;
            break;
        default:
            break;
    }
    OM_CRITICAL_END();

    return (void *)ret;
}

/**
 * @brief lp tim interrupt service routine
 *
 * @param[in] om_lptim     lp timer instance
 *
 * @return None
 **/
void drv_lptim_isr(OM_LPTIM_Type *om_lptim)
{
    uint32_t status;
    // get status and clear
    status = om_lptim->INTF & LPTIM_INTF_ALL_MASK;
    om_lptim->INTF = status;
    if (status) {
        drv_lptim_isr_callback(om_lptim, (drv_event_t)(status << 16), NULL, NULL);
    }
}

#endif  /* (RTE_LPTIM) */


/** @} */
