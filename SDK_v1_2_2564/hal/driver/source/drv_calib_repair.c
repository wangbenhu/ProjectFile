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
 * @brief    CALIB_REPAIR driver source file
 * @details  CALIB_REPAIR driver source file
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
#if (RTE_CALIB)
#include <stddef.h>
#include "om_driver.h"
#include "om_time.h"


/*******************************************************************************
 * MACROS
 */
/// Invalid TEMPERATURE
#define DRV_INVALID_TEMPERATURE    0xFFU
#define RC_RF_REPAIR_INTERVAL_MS   ((60*1000) * 32)
#define RC32K_REPAIR_INTERVAL_MS   ((60*1000) * 32)


/*******************************************************************************
 * CONST & VARIABLES
 */
drv_calib_repair_t drv_calib_repair_env = {
    .temperature            = 25,
    .rc_rf_repair_time      = PMU_TIMER_MAX_TICK,
    .rc_repair_temperature  = DRV_INVALID_TEMPERATURE,
    .rf_repair_temperature  = DRV_INVALID_TEMPERATURE,
    .rc32k_repair_time      = PMU_TIMER_MAX_TICK,
    .trim_vref              = 12,
    .dig_ldo                = 6,
    .pa_ldo                 = 6,
    .dcdc_vout              = 9,
    .pfd_ldo                = 1,
    .vco_ldo                = 5,
    .buff_ldo               = 4,
    .ana_ldo                = 4,
    .soft_major             = 0xFF,
    .soft_minor             = 0xFF,
    .soft_svn               = 0xFF,
    .ate_svn                = 0xFF,
    .kdco_lut_1m_2406       = 0x20,
    .kdco_lut_1m_2420       = 0x20,
    .kdco_lut_1m_2434       = 0x20,
    .kdco_lut_1m_2448       = 0x20,
    .kdco_lut_1m_2462       = 0x20,
    .kdco_lut_1m_2476       = 0x20,
    .kdco_lut_2m_2406       = 0x20,
    .kdco_lut_2m_2420       = 0x20,
    .kdco_lut_2m_2434       = 0x20,
    .kdco_lut_2m_2448       = 0x20,
    .kdco_lut_2m_2462       = 0x20,
    .kdco_lut_2m_2476       = 0x20,
    .con_bias_idac_pll      = 0x1F,
    .vco_cur                = 15,
    .kdco_df1_1m            = 250,
    .kdco_df1_2m            = 500,
};


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
/**
 * @brief  calib repair voltage temperature
 *
 * @param[in] t  temperature
 **/
static void drv_calib_repair_voltage_temperature(int16_t t)
{
    drv_pmu_register_step_set(&OM_PMU->TRIM_SET, MASK_STEP(PMU_TRIM_SET_PMU_TRIM_VREF, drv_calib_repair_env.trim_vref), true/*should_update*/, 10/*delay_us*/);
}

/**
 * @brief rf pll temperature repair
 **/
void drv_calib_repair_rf_pll_temperature_repair(bool is_calib_start)
{
    static int16_t calib_repair_env_calib_start_temperature;
    int8_t con_bias_idac_pll_delta = 0;
    uint8_t ftun = 0;
    int8_t  pll_vco_ibias_cur_delta = 0;

    if (is_calib_start) {
        calib_repair_env_calib_start_temperature = drv_calib_repair_env.rf_repair_temperature;
        if (calib_repair_env_calib_start_temperature > 40) {
            ftun = 3;
        } else if (calib_repair_env_calib_start_temperature <= 10) {
            ftun = 0;
        } else {
            ftun = 1;
        }
    } else {
        if ((calib_repair_env_calib_start_temperature > 10) && (calib_repair_env_calib_start_temperature <= 40)) {
            ftun = 1;
            if ((drv_calib_repair_env.temperature - calib_repair_env_calib_start_temperature) > 80) {
                ftun = 3;
            } else if ((drv_calib_repair_env.temperature - calib_repair_env_calib_start_temperature) > 20) {
                ftun = 3;
            } else if ((drv_calib_repair_env.temperature - calib_repair_env_calib_start_temperature) < -20) {
                ftun = 0;
            }
        } else if ((calib_repair_env_calib_start_temperature > 40) && (calib_repair_env_calib_start_temperature <= 60)) {
            ftun = 3;
            if ((drv_calib_repair_env.temperature - calib_repair_env_calib_start_temperature) > 60) {
                ftun = 3;
            } else if ((drv_calib_repair_env.temperature - calib_repair_env_calib_start_temperature) > 20) {
                ftun = 3;
            }

            if ((drv_calib_repair_env.temperature - calib_repair_env_calib_start_temperature) < -40) {
                ftun = 0;
            } else if ((drv_calib_repair_env.temperature - calib_repair_env_calib_start_temperature) < -20) {
                ftun = 0;
            }
        } else if ((calib_repair_env_calib_start_temperature > 60) && (calib_repair_env_calib_start_temperature <= 80)) {  /// 70
            ftun = 3;
            if ((drv_calib_repair_env.temperature - calib_repair_env_calib_start_temperature) < -60) {  // -0
                ftun = 0;
            } else if ((drv_calib_repair_env.temperature - calib_repair_env_calib_start_temperature) < -40) { //25
                ftun = 0;
            }
        } else if ((calib_repair_env_calib_start_temperature > 80) && (calib_repair_env_calib_start_temperature <= 100)) { ///90
            ftun = 3;
            if ((drv_calib_repair_env.temperature - calib_repair_env_calib_start_temperature) < -80) { //0
                ftun = 0;
            } else if ((drv_calib_repair_env.temperature - calib_repair_env_calib_start_temperature) < -60) { //25
                ftun = 0;
            } else if ((drv_calib_repair_env.temperature - calib_repair_env_calib_start_temperature) < -40) { //50
                ftun = 1;
            }
        } else if (calib_repair_env_calib_start_temperature > 100) { ///115
            ftun = 3;
        } else if ((calib_repair_env_calib_start_temperature > -10) && (calib_repair_env_calib_start_temperature <= 10)) { ///0
            ftun = 0;
            if ((drv_calib_repair_env.temperature - calib_repair_env_calib_start_temperature) > 106) {
                ftun = 3;
            } else if ((drv_calib_repair_env.temperature - calib_repair_env_calib_start_temperature) > 50) {
                ftun = 3;
            } else if ((drv_calib_repair_env.temperature - calib_repair_env_calib_start_temperature) > 30) {
                ftun = 3;
            }

            if ((drv_calib_repair_env.temperature - calib_repair_env_calib_start_temperature) < -30) {
                ftun = 0;
            }
        } else if ((calib_repair_env_calib_start_temperature > -30) && (calib_repair_env_calib_start_temperature <= -10)) {///-25
            ftun = 0;
            if ((drv_calib_repair_env.temperature - calib_repair_env_calib_start_temperature) > 110) {
                ftun = 3;
            } else if ((drv_calib_repair_env.temperature - calib_repair_env_calib_start_temperature) > 50) {
                ftun = 3;
            } else if ((drv_calib_repair_env.temperature - calib_repair_env_calib_start_temperature) > 30) {
                ftun = 0;
            }

            if ((drv_calib_repair_env.temperature - calib_repair_env_calib_start_temperature) < -15) {
                ftun = 0;
            }
        } else if (calib_repair_env_calib_start_temperature <= -30) { ///-40
            ftun = 0;
            if ((drv_calib_repair_env.temperature - calib_repair_env_calib_start_temperature) > 50) {
                ftun = 3;
            }
        }

        if ((drv_calib_repair_env.temperature >= -10) && (drv_calib_repair_env.temperature <= 70)) {
            con_bias_idac_pll_delta = 0;
        } else if ((drv_calib_repair_env.temperature) > 100) {
            con_bias_idac_pll_delta = 8;
        } else if ((drv_calib_repair_env.temperature) > 90) {
            con_bias_idac_pll_delta = 7;
        } else if ((drv_calib_repair_env.temperature) > 80) {
            con_bias_idac_pll_delta = 5;
        } else if ((drv_calib_repair_env.temperature) > 70) {
            con_bias_idac_pll_delta = 2;
        } else if ((drv_calib_repair_env.temperature) < -28) {
            con_bias_idac_pll_delta = 7;
        } else if ((drv_calib_repair_env.temperature) < -20) {
            con_bias_idac_pll_delta = 5;
        } else if ((drv_calib_repair_env.temperature) < -10) {
            con_bias_idac_pll_delta = 2;
        }

        if ((drv_calib_repair_env.temperature) < 0) {
            pll_vco_ibias_cur_delta = 1;
        }
    }

    int8_t use_bias_idac_pll     = drv_calib_repair_env.con_bias_idac_pll + con_bias_idac_pll_delta;
    int8_t use_pll_vco_ibias_cur = drv_calib_repair_env.vco_cur + pll_vco_ibias_cur_delta;
    if (use_bias_idac_pll < 0) {
        use_bias_idac_pll = 0;
    } else if (use_bias_idac_pll > 63) {
        use_bias_idac_pll = 63;
    }
    if (use_pll_vco_ibias_cur < 0) {
        use_pll_vco_ibias_cur = 0;
    } else if (use_pll_vco_ibias_cur > 15) {
        use_pll_vco_ibias_cur = 15;
    }

    DRV_RCC_ANA_CLK_ENABLE();
    REGW(&OM_DAIF->PLL_CTRL1, MASK_2REG(DAIF_CON_BIAS_IDAC_PLL, use_bias_idac_pll,
                                        DAIF_RDPLL_SEL_VCO_IBIAS, use_pll_vco_ibias_cur));
    REGW(&OM_DAIF->PLL_CTRL0, MASK_1REG(DAIF_REG_FTUN, ftun));
    DRV_RCC_ANA_CLK_RESTORE();
}

/**
 * @brief rf temperature repair check
 **/
static void drv_calib_repair_rc_rf_temperature(uint32_t cur_time)
{
    #if (RTE_GPADC)
    if (!drv_gpadc_control(GPADC_CONTROL_IS_BUSY, NULL)) {
        int16_t t = (int)drv_gpadc_control(GPADC_CONTROL_READ_TEMPERATURE, NULL);
        int16_t delta_t_rc = drv_calib_repair_env.rc_repair_temperature - t;
        int16_t pre_all_repair_t = drv_calib_repair_env.rf_repair_temperature;
        bool topclk_recalibed = false;

        drv_calib_repair_env.temperature = t;
        drv_calib_repair_env.rc_rf_repair_time = cur_time;

        // if delta-temperature>15C, re-calib rc
        if ((delta_t_rc > 15) || (delta_t_rc < -15)) {
            drv_calib_repair_env.rc_repair_temperature = t;

            drv_pmu_topclk_recalib();
            topclk_recalibed = true;
        }

        // re-calib all
        if (((pre_all_repair_t < 85) && (t > 90)) || ((pre_all_repair_t > 90) && (t < 85))) {
            drv_calib_repair_env.rc_repair_temperature = t;
            drv_calib_repair_env.rf_repair_temperature = t;

            if (!topclk_recalibed) {
                drv_pmu_topclk_recalib();
            }
            drv_calib_repair_voltage_temperature(t);
            drv_calib_rf(t);
        }

        // drv_calib_repair_rf_pll_temperature_repair(false);
    }
    #else
    #warning "Need GPADC RTE driver"
    #endif
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 * @brief  calib repair rc rf init
 **/
void drv_calib_repair_init(int16_t t)
{
    #if (RC_RF_REPAIR_INTERVAL_MS)

    drv_calib_repair_env.temperature = t;
    drv_calib_repair_env.rf_repair_temperature = t;
    drv_calib_repair_env.rc_repair_temperature = t;
    drv_calib_repair_env.rc32k_repair_time = drv_calib_repair_env.rc_rf_repair_time = om_time();

    // voltage repair
    drv_calib_repair_voltage_temperature(t);
    #endif
}

/**
 * @brief  calib repair rc rf temperature check
 * @return repaired
 **/
__RAM_CODES("PM")
bool drv_calib_repair_rc_rf_temperature_check(void)
{
    #if (RC_RF_REPAIR_INTERVAL_MS)
    do {
        uint32_t pre_time_tmp;

        pre_time_tmp = drv_calib_repair_env.rc_rf_repair_time;
        if (om_time_delay_past(RC_RF_REPAIR_INTERVAL_MS, &pre_time_tmp)) {
            drv_calib_repair_rc_rf_temperature(pre_time_tmp);
            return true;
        }
    } while (0);
    #endif

    return false;
}

/**
 * @brief  calib repair rc32k temperature check
 * @return repaired
 **/
__RAM_CODES("PM")
bool drv_calib_repair_rc32k_temperature_check(void)
{
    bool repaired = false;

    #if (RC32K_REPAIR_INTERVAL_MS)
    do {
        if (drv_pmu_select_32k_get() != PMU_32K_SEL_RC) {
            break;
        }
        uint32_t pre_time_tmp = drv_calib_repair_env.rc32k_repair_time;
        if (om_time_delay_past(RC32K_REPAIR_INTERVAL_MS, &pre_time_tmp)) {  // delay is pasted
            // check and re-select (will trigger calib_rc32k)
            drv_pmu_select_32k(PMU_32K_SEL_RC);
            drv_calib_repair_env.rc32k_repair_time = pre_time_tmp;
            repaired = true;
        }
    } while (0);
    #endif

    return repaired;
}

#endif  /* RTE_CALIB */

/** @} */
