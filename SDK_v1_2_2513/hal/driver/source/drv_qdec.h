/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup QDEC QDEC
 * @ingroup  DRIVER
 * @brief    QDEC driver
 * @details  QDEC driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_QDEC_H
#define __DRV_QDEC_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_QDEC)
#include <stdint.h>
#include "om_driver.h"

#ifdef  __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * TYPEDEFS
 */
/// QDEC LED polarity control
typedef enum {
    /// QDEC LED active on output pin low.
    QDEC_LED_POL_LOW  = 0U,
    /// QDEC LED active on output pin high.
    QDEC_LED_POL_HIGH = 1U,
} qdec_led_pol_t;

/// QDEC Pin select for LED signaL
typedef enum {
    /// connect LED signaL
    QDEC_PIN_SEL_LED_CONNECT    = 0U,
    /// disconnect LED signaL
    QDEC_PIN_SEL_LED_DISCONNECT = 1U,
} qdec_pin_sel_led_t;

/// QDEC report period
typedef enum {
    /// 10 samples to be accumulated before the REPORTRDY and DBLRDY events can be generated
    QDEC_REPORT_PER_10  = 0U,
    /// 40 samples to be accumulated before the REPORTRDY and DBLRDY events can be generated
    QDEC_REPORT_PER_40  = 1U,
    /// 80 samples to be accumulated before the REPORTRDY and DBLRDY events can be generated
    QDEC_REPORT_PER_80  = 2U,
    /// 120 samples to be accumulated before the REPORTRDY and DBLRDY events can be generated
    QDEC_REPORT_PER_120 = 3U,
    /// 160 samples to be accumulated before the REPORTRDY and DBLRDY events can be generated
    QDEC_REPORT_PER_160 = 4U,
    /// 200 samples to be accumulated before the REPORTRDY and DBLRDY events can be generated
    QDEC_REPORT_PER_200 = 5U,
    /// 240 samples to be accumulated before the REPORTRDY and DBLRDY events can be generated
    QDEC_REPORT_PER_240 = 6U,
    /// 280 samples to be accumulated before the REPORTRDY and DBLRDY events can be generated
    QDEC_REPORT_PER_280 = 7U,
    /// 1 samples to be accumulated before the REPORTRDY and DBLRDY events can be generated
    QDEC_REPORT_PER_1   = 8U,
} qdec_report_per_t;

/// QDEC sample period
typedef enum {
    /// sample period 32us
    QDEC_SAMPLE_PER_32US     = 0U,
    /// sample period 64us
    QDEC_SAMPLE_PER_64US     = 1U,
    /// sample period 128us
    QDEC_SAMPLE_PER_128US    = 2U,
    /// sample period 256us
    QDEC_SAMPLE_PER_256US    = 3U,
    /// sample period 512us
    QDEC_SAMPLE_PER_512US    = 4U,
    /// sample period 1024us
    QDEC_SAMPLE_PER_1024US   = 5U,
    /// sample period 2048us
    QDEC_SAMPLE_PER_2048US   = 6U,
    /// sample period 4096us
    QDEC_SAMPLE_PER_4096US   = 7U,
    /// sample period 8192us
    QDEC_SAMPLE_PER_8192US   = 8U,
    /// sample period 16284us
    QDEC_SAMPLE_PER_16284US  = 9U,
    /// sample period 32768us
    QDEC_SAMPLE_PER_32768US  = 10U,
    /// sample period 65536us
    QDEC_SAMPLE_PER_65536US  = 11U,
    /// sample period 131072us
    QDEC_SAMPLE_PER_131072US = 12U,
} qdec_sample_per_t;

/// QDEC config
typedef struct {
    /// The period the LED is switched on before sampling
    uint32_t                led_pre;
    /// LED output pin polarity
    qdec_led_pol_t          led_pol;
    /// pin select for LED signal
    qdec_pin_sel_led_t      pin_sel_led;
    /// report period
    qdec_report_per_t       report_per;
    /// sampling period
    qdec_sample_per_t       sample_per;
    /// debounce filters enable
    bool                    dbf_en;
} qdec_config_t;

/// QDEC control
typedef enum {
    QDEC_CONTROL_START          = 0U,   /*!< Start. argu with NULL, return OM_ERROR_OK */
    QDEC_CONTROL_STOP           = 1U,   /*!< Stop. argu with NULL, return OM_ERROR_OK */
    QDEC_CONTROL_SET_SAMPLEPER  = 2U,   /*!< Set sample period. argu is qdec_sample_per_t, return OM_ERROR_OK */
    QDEC_CONTROL_SET_DBFEN      = 3U,   /*!< Set debounce filters enable. argu is bool, return OM_ERROR_OK */
    QDEC_CONTROL_SET_LEDPOL     = 4U,   /*!< Set LED output pin polarity. argu is qdec_led_pol_t, return OM_ERROR_OK */
    QDEC_CONTROL_SET_LEDPRE     = 5U,   /*!< Set the period the LED is switched on before sampling. argu is uint32_t, return OM_ERROR_OK */
    QDEC_CONTROL_SET_REPORTPER  = 6U,   /*!< Set report period. argu is qdec_report_per_t, return OM_ERROR_OK */
    QDEC_CONTROL_SET_PSELLED    = 7U,   /*!< Set pin select for LED signal. argu is qdec_pin_sel_led_t, return OM_ERROR_OK */
    QDEC_CONTROL_SET_INIT_A     = 8U,   /*!< Set phase A first sample value after power on, return OM_ERROR_OK */
    QDEC_CONTROL_SET_INIT_B     = 9U,   /*!< Set phase B first sample value after power on, return OM_ERROR_OK */
    QDEC_CONTROL_GET_INIT_A     = 10U,  /*!< Get phase A first sample value after power on, return sample value A */
    QDEC_CONTROL_GET_INIT_B     = 11U,  /*!< Get phase B first sample value after power on, return sample value B */
    QDEC_CONTROL_GET_INPUT_A    = 12U,  /*!< Get phase A last sample value before power down, return sample value A */
    QDEC_CONTROL_GET_INPUT_B    = 13U,  /*!< Get phase B last sample value before power down, return sample value B */
    QDEC_CONTROL_INT_ENABLE     = 14U,  /*!< Interrupt Enable, return OM_ERROR_OK */
    QDEC_CONTROL_INT_DISABLE    = 15U,  /*!< Interrupt Disable, return OM_ERROR_OK */
} qdec_control_t;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief QDEC initialization
 *
 * @param[in] om_qdec        Pointer to QDEC
 * @param[in] qdec_cfg       Configuration for QDEC
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_qdec_init(OM_QDEC_Type *om_qdec, const qdec_config_t *qdec_cfg);

/**
 *******************************************************************************
 * @brief QDEC uninitialization
 *
 * @param[in] om_qdec        Pointer to QDEC
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_qdec_uninit(OM_QDEC_Type *om_qdec);

#if (RTE_QDEC_REGISTER_CALLBACK)
/**
 *******************************************************************************
 * @brief Register event callback for transmit/receive by interrupt & dma mode
 *
 * @param[in] om_qdec        Pointer to QDEC
 * @param[in] cb             Pointer to callback
 *******************************************************************************
 */
extern void drv_qdec_register_isr_callback(OM_QDEC_Type *om_qdec, drv_isr_callback_t cb);
#endif

/**
 *******************************************************************************
 * @brief The interrupt callback for QDEC driver. It is a weak function. User should define
 *        their own callback in user file, other than modify it in the QDEC driver.
 *
 * @param[in] om_qdec        Pointer to QDEC
 * @param[in] event          The driver QDEC event
 * @param[in] acc            The acc
 * @param[in] accdbl         The accdbl
 *******************************************************************************
 */
extern void drv_qdec_isr_callback(OM_QDEC_Type *om_qdec, drv_event_t event, int acc, int accdbl);

/**
 *******************************************************************************
 * @brief Control QDEC interface.
 *
 * @param[in] om_qdec        Pointer to QDEC
 * @param[in] control        Operation
 * @param[in] argu           Operation argument
 *
 * @return status:           Control status
 *******************************************************************************
 */
__STATIC_FORCEINLINE void *drv_qdec_control(OM_QDEC_Type *om_qdec, qdec_control_t control, void *argu)
{
    uint32_t ret;

    ret = (uint32_t)OM_ERROR_OK;

    OM_CRITICAL_BEGIN();
    switch (control) {
        case QDEC_CONTROL_START:
            // Enable QDEC
            REGW(&om_qdec->ENABLE, MASK_1REG(QDEC_ENABLE_ENABLE, 1U));
            while(!REGR(&om_qdec->ENABLE, MASK_POS(QDEC_ENABLE_ENABLE)));
            // Start the QDEC
            REGW(&om_qdec->START, MASK_1REG(QDEC_START_START, 1U));
            break;
        case QDEC_CONTROL_STOP:
            // Stop the QDEC
            REGW(&om_qdec->STOP, MASK_1REG(QDEC_STOP_STOP, 1U));
            break;
        case QDEC_CONTROL_SET_SAMPLEPER:
            REGW(&om_qdec->SAMPLEPER, MASK_1REG(QDEC_SAMPLEPER_SAMPLEPER, (uint32_t)argu));
            break;
        case QDEC_CONTROL_SET_REPORTPER:
            REGW(&om_qdec->REPORTPER, MASK_1REG(QDEC_REPORTPER_REPORTPER, (uint32_t)argu));
            break;
        case QDEC_CONTROL_SET_DBFEN:
            REGW(&om_qdec->DBFEN, MASK_1REG(QDEC_DBFEN_DBFEN, (uint32_t)argu));
            break;
        case QDEC_CONTROL_SET_LEDPOL:
            REGW(&om_qdec->LEDPOL, MASK_1REG(QDEC_LEDPOL_LEDPOL, (uint32_t)argu));
            break;
        case QDEC_CONTROL_SET_LEDPRE:
            REGW(&om_qdec->LEDPRE, MASK_1REG(QDEC_LEDPRE_LEDPRE, (uint32_t)argu));
            break;
        case QDEC_CONTROL_SET_PSELLED:
            REGW(&om_qdec->PSELLED, MASK_1REG(QDEC_PSELLED_CONNECT, (uint32_t)argu));
            break;
        case QDEC_CONTROL_SET_INIT_A:
            REGW(&om_qdec->INIT, MASK_1REG(QDEC_INIT_A, (uint32_t)argu));
            break;
        case QDEC_CONTROL_SET_INIT_B:
            REGW(&om_qdec->INIT, MASK_1REG(QDEC_INIT_B, (uint32_t)argu));
            break;
        case QDEC_CONTROL_GET_INIT_A:
            ret = REGR(&om_qdec->INIT, MASK_POS(QDEC_INIT_A));
            break;
        case QDEC_CONTROL_GET_INIT_B:
            ret = REGR(&om_qdec->INIT, MASK_POS(QDEC_INIT_B));
            break;
        case QDEC_CONTROL_GET_INPUT_A:
            ret = REGR(&om_qdec->INPUT, MASK_POS(QDEC_INPUT_A));
            break;
        case QDEC_CONTROL_GET_INPUT_B:
            ret = REGR(&om_qdec->INPUT, MASK_POS(QDEC_INPUT_B));
            break;
        case QDEC_CONTROL_INT_ENABLE:
            om_qdec->INTEN |= (uint32_t)argu;
            break;
        case QDEC_CONTROL_INT_DISABLE:
            om_qdec->INTEN &= ~((uint32_t)argu);
            break;
        default:
            break;
    }

    OM_CRITICAL_END();

    return (void *)ret;
}

/**
 *******************************************************************************
 * @brief QDEC interrupt service routine
 *
 * @param[in] om_qdec        Pointer to QDEC
 *******************************************************************************
 */
extern void drv_qdec_isr(OM_QDEC_Type *om_qdec);


#ifdef  __cplusplus
}
#endif

#endif  /* (RTE_QDEC) */

#endif /* __DRV_QDEC_H */


/** @} */
