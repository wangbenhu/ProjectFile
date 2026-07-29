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
 * @brief    PIN driver source file
 * @details  PIN driver source file
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
#if (RTE_PIN)
#include <stdint.h>
#include "om_device.h"
#include "om_driver.h"


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Initialize pin function
 *
 * @param[in] pin      pin
 * @param[in] func     function
 *
 *******************************************************************************
 */
void drv_pinmux_set(uint8_t pin, pinmux_t pinmux)
{
    OM_ASSERT(pin < CAP_DIG_PAD_NUM);

    uint32_t i = pin / 4;
    uint32_t p = (pin % 4) * 8;
    uint32_t lptim_group_idx = pin / 6;
    uint32_t lptim_idx_in_group = pin % 6;

    if (pinmux < PINMUX_COMMON_PERIPHERAL_CFG) {
        // clear lptim pinmux
        if (lptim_idx_in_group <= 3) {  // lptim idx:[0,3], dmic idx:[4,5] in every group
            OM_PMU->LP_TIMER_OUT_CTRL &= ~(1U << (pin - (lptim_group_idx * 2)));
        }
        // set none-lptim pinmux
        register_set(&OM_SYS->PINMUX[i], SYS_PINMUX_MASK((uint8_t)pinmux, p));
    } else if ((pinmux >= PINMUX_LPTIM_OUT0_CFG) && (pinmux <= PINMUX_LPTIM_OUT3_CFG)) {
        // clear none-lptim pinmux
        register_set(&OM_SYS->PINMUX[i], SYS_PINMUX_MASK((uint8_t)PINMUX_JTAG_MODE_CFG, p));
        // set lptim pinmux
        if (lptim_idx_in_group <= 3) {
            OM_PMU->LP_TIMER_OUT_CTRL |= (1U << (pin - (lptim_group_idx * 2)));
        }
    }

    OM_CRITICAL_BEGIN();
    switch (pin) {
        case 13:
            if (pinmux == PINMUX_RESETB_CFG) {
                OM_PMU->MISC_CTRL_1 |= PMU_MISC_CTRL_1_EN_RSTB_MASK;
            } else {
                OM_PMU->MISC_CTRL_1 &= ~PMU_MISC_CTRL_1_EN_RSTB_MASK;
            }
            break;
        case 24:
        case 25:
            if (pinmux == PINMUX_XTAL32K_CFG) {
                OM_PMU->ANA_REG |= PMU_ANA_REG_GPIO_REUSE_MASK;
            } else {
                OM_PMU->ANA_REG &= ~PMU_ANA_REG_GPIO_REUSE_MASK;
            }
            break;
        default:
            break;
    }
    OM_CRITICAL_END();
}

/**
 *******************************************************************************
 * @brief get pinmux configuration type
 *
 * @param[in] pin      Configuration number for pinmux
 *
 *******************************************************************************
 */
pinmux_t drv_pinmux_get(uint8_t pin)
{
    OM_ASSERT(pin < CAP_DIG_PAD_NUM);

    pinmux_t pinmux = PINMUX_JTAG_MODE_CFG;
    uint8_t pinmux_idx = 0;
    uint32_t lptim_group_idx;
    uint8_t lptim_out_ctrl_pin_mask;

    switch (pin) {
        case 13:
            if (OM_PMU->MISC_CTRL_1 & PMU_MISC_CTRL_1_EN_RSTB_MASK) {
                return PINMUX_RESETB_CFG;
            }
            break;
        case 24:
        case 25:
            if (OM_PMU->ANA_REG & PMU_ANA_REG_GPIO_REUSE_MASK) {
                return PINMUX_XTAL32K_CFG;
            }
            break;
        default:
            break;
    }

    lptim_group_idx = pin / 6;
    lptim_out_ctrl_pin_mask = register_get(&OM_PMU->LP_TIMER_OUT_CTRL, (1U << (pin - (lptim_group_idx * 2))), (pin - (lptim_group_idx * 2)));
    if (1 == lptim_out_ctrl_pin_mask) {
        uint8_t pinmux_lptim_idx = (pin - 2 * lptim_group_idx) % 4;
        pinmux_idx = PINMUX_LPTIM_OUT0_CFG + pinmux_lptim_idx;
    } else {
        uint32_t i, p;
        i = pin / 4;
        p = (pin % 4) * 8;
        pinmux_idx = register_get(&OM_SYS->PINMUX[i], SYS_PINMUX_MASK_POS(p));
    }
    pinmux += pinmux_idx;

    return pinmux;
}

/**
 *******************************************************************************
 * @brief Initialize pin function and electrical characteristics
 *
 * @param[in] pin_cfg      Configuration for pinmux
 * @param[in] pin_cfg_num      Configuration number for pinmux
 *
 *******************************************************************************
 */
void drv_pin_init(const pin_config_t *pin_cfg, uint32_t pin_cfg_num)
{
    uint32_t i;

    OM_ASSERT(pin_cfg);
    for (i = 0; i < pin_cfg_num; ++i) {
        /*lint -save -e613 */
        drv_pinmux_set(pin_cfg[i].dig_pad, (pinmux_t)(pin_cfg[i].func.pin_func));
        drv_pmu_pin_mode_set(pin_cfg[i].dig_pad, pin_cfg[i].mode);
        drv_pmu_pin_driven_current_set(pin_cfg[i].dig_pad, pin_cfg[i].drv);
        drv_pmu_pin_pullup_set(pin_cfg[i].dig_pad, pin_cfg[i].pullup);
        drv_pmu_pin_input_enable(pin_cfg[i].dig_pad, 1U);
        /*lint -restore */
    }
}
#endif  /* RTE_PIN */


/** @} */
