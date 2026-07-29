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
 * @brief    QDEC driver source file
 * @details  QDEC driver source file
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
#if (RTE_QDEC)
#include <stddef.h>
#include "om_driver.h"


/*******************************************************************************
 * MACROS
 */
/*
 * Used to define qdec_env_t and drv_resource_t structures
 */
#define DRV_QDEC_DEFINE(NAMEn, namen)                                          \
static qdec_env_t namen##_env = {                                              \
    .isr_cb     = NULL,                                                        \
};                                                                             \
static const drv_resource_t namen##_resource = {                               \
    .cap      = CAP_##NAMEn,                                                   \
    .reg      = OM_##NAMEn,                                                    \
    .env      = (void *)&namen##_env,                                          \
    .irq_num  = NAMEn##_IRQn,                                                  \
    .irq_prio = RTE_##NAMEn##_IRQ_PRIORITY,                                    \
}


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    drv_isr_callback_t  isr_cb;
} qdec_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
DRV_QDEC_DEFINE(QDEC, qdec);


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static const drv_resource_t *qdec_get_resource(OM_QDEC_Type *om_qdec)
{
    return &qdec_resource;
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief QDEC initialization
 *
 * @param[in] om_qdec        Pointer to QDEC
 * @param[in] qdec_cfg       Configuration for QDEC
 *
 * @return status:
 *    - OM_ERROR_OK:         Nothing more to do
 *    - others:              No
 *******************************************************************************
 */
om_error_t drv_qdec_init(OM_QDEC_Type *om_qdec, const qdec_config_t *qdec_cfg)
{
    const drv_resource_t *resource;
    qdec_env_t           *env;
    uint32_t              clk_div = 0;

    OM_ASSERT(qdec_cfg);
    resource = qdec_get_resource(om_qdec);
    if (resource == NULL) {
        return OM_ERROR_PARAMETER;
    }
    env = (qdec_env_t *)resource->env;

    DRV_RCC_RESET(RCC_CLK_QDEC);

    // Set QDEC 1M clock
    clk_div = drv_rcc_clock_get(RCC_CLK_QDEC) / (1000 * 1000);
    REGW(&OM_CPM->QDEC_CFG, MASK_1REG(CPM_QDEC_CFG_DIV_COEFF, clk_div));

    // Debounce filters enable
    REGW(&om_qdec->DBFEN, MASK_1REG(QDEC_DBFEN_DBFEN, qdec_cfg->dbf_en));
    // The LED output pin polarity
    REGW(&om_qdec->LEDPOL, MASK_1REG(QDEC_LEDPOL_LEDPOL, qdec_cfg->led_pol));
    // The period the LED is switched on before sampling.
    REGW(&om_qdec->LEDPRE, MASK_1REG(QDEC_LEDPRE_LEDPRE, qdec_cfg->led_pre));
    // Pin select for LED signal
    REGW(&om_qdec->PSELLED, MASK_1REG(QDEC_PSELLED_CONNECT, qdec_cfg->pin_sel_led));
    // Report period
    REGW(&om_qdec->REPORTPER, MASK_1REG(QDEC_REPORTPER_REPORTPER, qdec_cfg->report_per));
    // Sampling period
    REGW(&om_qdec->SAMPLEPER, MASK_1REG(QDEC_SAMPLEPER_SAMPLEPER, qdec_cfg->sample_per));

    // Disable all interrupt
    om_qdec->INTEN = 0;

    NVIC_ClearPendingIRQ(QDEC_IRQn);
    NVIC_SetPriority(QDEC_IRQn, RTE_QDEC_IRQ_PRIORITY);
    NVIC_EnableIRQ(QDEC_IRQn);

    env->isr_cb = NULL;

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief QDEC uninitialization
 *
 * @param[in] om_qdec        Pointer to QDEC
 *
 * @return status:
 *    - OM_ERROR_OK:         Nothing more to do
 *    - others:              No
 *******************************************************************************
 */
om_error_t drv_qdec_uninit(OM_QDEC_Type *om_qdec)
{
    const drv_resource_t *resource;

    resource = qdec_get_resource(om_qdec);
    if (resource == NULL) {
        return OM_ERROR_PARAMETER;
    }

    // Disable QDEC
    REGW(&om_qdec->ENABLE, MASK_1REG(QDEC_ENABLE_ENABLE, 0U));
    while(REGR(&om_qdec->ENABLE, MASK_POS(QDEC_ENABLE_ENABLE)));

    NVIC_DisableIRQ(QDEC_IRQn);

    return OM_ERROR_OK;
}

#if (RTE_QDEC_REGISTER_CALLBACK)
/**
 *******************************************************************************
 * @brief Register event callback for transmit/receive by interrupt & dma mode
 *
 * @param[in] om_qdec        Pointer to QDEC
 * @param[in] event_cb       Pointer to callback
 *
 *******************************************************************************
 */
void drv_qdec_register_isr_callback(OM_QDEC_Type *om_qdec, drv_isr_callback_t cb)
{
    const drv_resource_t *resource;
    qdec_env_t           *env;

    resource = qdec_get_resource(om_qdec);
    if (NULL != resource) {
        env = (qdec_env_t *)(resource->env);
        env->isr_cb = cb;
    }
}
#endif /* (RTE_QDEC_REGISTER_CALLBACK) */

__WEAK void drv_qdec_isr_callback(OM_QDEC_Type *om_qdec, drv_event_t event, int acc, int accdbl)
{
    #if (RTE_QDEC_REGISTER_CALLBACK)
    const drv_resource_t *resource;
    qdec_env_t           *env;

    resource = qdec_get_resource(om_qdec);
    if (resource != NULL) {
        env = (qdec_env_t *)(resource->env);
        if (env->isr_cb != NULL) {
            env->isr_cb(om_qdec, event, (void *)acc, (void *)accdbl);
        }
    }
    #endif /* (RTE_QDEC_REGISTER_CALLBACK) */
}

/**
 *******************************************************************************
 * @brief qdec interrupt service routine
 *
 * @param[in] om_qdec        Pointer to QDEC
 *******************************************************************************
 */
void drv_qdec_isr(OM_QDEC_Type *om_qdec)
{
    drv_event_t            drv_event;
    uint8_t                int_status;
    int                    acc;
    int                    accdbl;

    // read and clear interrupt status
    int_status = om_qdec->INTST;
    om_qdec->INTCLR = int_status;

    // read and clear acc/accdbl
    acc = om_qdec->ACC;
    accdbl = om_qdec->ACCDBL;
    REGW(&om_qdec->READCLRACC, MASK_1REG(QDEC_READCLRACC_READCLRACC, 1U));

    drv_event = DRV_EVENT_COMMON_NONE;
    if (int_status & QDEC_INTST_SAMPLERDY_MASK) {
        drv_event |= DRV_EVENT_QDEC_SAMPLERDY;
    }
    if (int_status & QDEC_INTST_REPORTRDY_MASK) {
        drv_event |= DRV_EVENT_QDEC_REPORTRDY;
    }
    if (int_status & QDEC_INTST_STOPPED_MASK) {
        drv_event |= DRV_EVENT_QDEC_STOPPED;
    }
    if (int_status & QDEC_INTST_DBLRDY_MASK) {
        drv_event |= DRV_EVENT_QDEC_DBLRDY;
    }
    if (int_status & QDEC_INTST_ACCOF_MASK) {
        drv_event |= DRV_EVENT_QDEC_ACCOF;
    }
    drv_qdec_isr_callback(om_qdec, drv_event, acc, accdbl);
}

#endif  /* (RTE_QDEC) */


/** @} */
