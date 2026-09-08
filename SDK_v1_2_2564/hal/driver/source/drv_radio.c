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
 * @brief    RADIO driver source file
 * @details  RADIO driver source file
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
#if (RTE_RADIO)
#include <stdint.h>
#include "om_device.h"
#include "om_driver.h"


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief  drv rf init
 *******************************************************************************
 */
void drv_rf_init(void)
{
    drv_rf_tx_power_set(false, RF_TX_POWER_NORMAL);
    int16_t t;

    #if (RTE_GPADC)
    t = (int)drv_gpadc_control(GPADC_CONTROL_READ_TEMPERATURE, NULL);
    #else
    t = 20;
    #endif
    drv_calib_repair_init(t);
    drv_calib_rf(t);
}

/**
 *******************************************************************************
 * @brief  rf txrx pin enable
 *
 * @param[in] enable  enable
 * @param[in] pol  polarity, 0 or 1
 *******************************************************************************
 **/
void drv_rf_txrx_pin_enable(bool enable, int pol)
{
    DRV_RCC_ANA_CLK_ENABLE();

    REGW(&OM_DAIF->TRX_EXT_PD_CFG, MASK_2REG(DAIF_TX_EXT_PD_EN, 0, DAIF_RX_EXT_PD_EN, 0));

    if (enable) {
        // Digital issue: DAIF_TX_EXT_PD_POL REG must open follow clock
        uint32_t clk_ens_save = OM_DAIF->CLK_ENS;
        REGW1(&OM_DAIF->CLK_ENS, DAIF_PLL_CLK_REF_EN_MASK | DAIF_MAIN_FSM_CLK_EN_MASK);
        uint32_t clk_cfg_save = OM_DAIF->CLK_CFG;
        REGW1(&OM_DAIF->CLK_CFG, DAIF_XTAL32M_EN_CKO16M_DIG_MASK);
        // setup pol
        REGW(&OM_DAIF->TRX_EXT_PD_CFG, MASK_4REG(DAIF_TX_EXT_PD_POL, pol, DAIF_RX_EXT_PD_POL, pol, DAIF_TX_EXT_PD_TCFG, 0, DAIF_RX_EXT_PD_TCFG, 0));
        // restore clock
        OM_DAIF->CLK_ENS = clk_ens_save;
        OM_DAIF->CLK_CFG = clk_cfg_save;

        // Enable
        REGW1(&OM_DAIF->TRX_EXT_PD_CFG, DAIF_TX_EXT_PD_EN_MASK | DAIF_RX_EXT_PD_EN_MASK);
    }

    DRV_RCC_ANA_CLK_RESTORE();
}

/**
 *******************************************************************************
 * @brief  rf carrier enable
 *
 * @param[in] enable  enable
 * @param[in] freq  2402MHz ... 2480MHz, If set freq to 2402.123456MHZ, freq_channel = 2402, fractFreq = 0.123456
 *******************************************************************************
 **/
void drv_rf_carrier_enable(bool enable, uint32_t freq, float fract_freq)
{
    if (enable) {
        drv_pmu_ana_enable(enable, PMU_ANA_RF);
        DRV_RCC_ANA_CLK_ENABLE();
        // frequency
        REGW(&OM_DAIF->FREQ_CFG0, MASK_2REG(DAIF_FREQ_REG_ME, 1, DAIF_FREQ_REG_MO, freq));
        if (fract_freq) {
            REGW(&OM_DAIF->FREQ_CFG3, MASK_2REG(DAIF_FREQ_FRAC_REG, (uint32_t)(fract_freq * 0x3FFFFF), DAIF_FREQ_0P5_EN, 1));
        } else {
            REGW(&OM_DAIF->FREQ_CFG3, MASK_2REG(DAIF_FREQ_FRAC_REG, 0, DAIF_FREQ_0P5_EN, 0));
        }

        // SDM
        REGW(&OM_DAIF->PLL_CTRL1, MASK_2REG(DAIF_DIGI_DIN_BYPASS, 1, DAIF_DIGI_DIN_REG, 0));
//        REGW(&OM_DAIF->PLL_CTRL2, MASK_2REG(DAIF_DIN_SDM_TX_ME, 1, DAIF_DATA_SYNC_BYPASS, 1));

        // do TX
        REGW(&OM_DAIF->VCO_CTRL0, MASK_3REG(DAIF_TRX_DBG, 1, DAIF_RX_EN_MO, 0, DAIF_TX_EN_MO, 0));
        DRV_DELAY_US(100);
        REGW(&OM_DAIF->VCO_CTRL0, MASK_3REG(DAIF_TRX_DBG, 1, DAIF_RX_EN_MO, 0, DAIF_TX_EN_MO, 1));

        DRV_RCC_ANA_CLK_RESTORE();
    } else {
        DRV_RCC_ANA_CLK_ENABLE();
        REGW(&OM_DAIF->FREQ_CFG0, MASK_1REG(DAIF_FREQ_REG_ME, 0));
        REGW(&OM_DAIF->PLL_CTRL1, MASK_2REG(DAIF_DIGI_DIN_BYPASS, 0, DAIF_DIGI_DIN_REG, 0));
//        REGW(&OM_DAIF->PLL_CTRL2, MASK_2REG(DAIF_DIN_SDM_TX_ME, 0, DAIF_DATA_SYNC_BYPASS, 0));
        REGW(&OM_DAIF->VCO_CTRL0, MASK_3REG(DAIF_TRX_DBG, 1, DAIF_RX_EN_MO, 0, DAIF_TX_EN_MO, 0));
//        REGW(&OM_DAIF->PD_CFG0, MASK_4REG(DAIF_PD_LDO_PA_ME,1, DAIF_PD_LDO_PA_MO,1,DAIF_PD_PA_ME,1,DAIF_PD_PA_MO,1));
        DRV_RCC_ANA_CLK_RESTORE();

        drv_pmu_ana_enable(enable, PMU_ANA_RF);
    }
}

/**
 *******************************************************************************
 * @brief  rf single tone enable
 *
 * @param[in] enable  enable
 * @param[in] freq  freq
 * @param[in] payload  payload (0-255)
 *******************************************************************************
 */
void drv_rf_single_tone_enable(bool enable, uint32_t freq, uint8_t payload)
{
    if (enable) {
        DRV_RCC_CLOCK_ENABLE(RCC_CLK_2P4, 1U);
        DRV_RCC_CLOCK_ENABLE(RCC_CLK_PHY, 1U);
        drv_pmu_ana_enable(true, PMU_ANA_RF);
        REGW1(&OM_24G->PKTCTRL0, OM24G_PKTCTRL0_MAC_SEL_MASK);

        REGW0(&OM_PHY->TX_CTRL0, PHY_TX_CTRL0_BP_GAU_MASK);  //GFSK
        // REGW0(&OM_PHY->REG_PHY_RST_N, PHY_REG_PHY_RST_N_REG_PHY_RST_N_MASK);  //reset phy
        REGW(&OM_24G->TESTCTRL, MASK_1REG(OM24G_CONT_WAVE, 1));
        REGW(&OM_DAIF->FREQ_CFG0, MASK_2REG(DAIF_FREQ_REG_MO, freq, DAIF_FREQ_REG_ME, 1));
        REGW(&OM_24G->TESTCTRL, MASK_1REG(OM24G_TEST_PAT_EN, 1));
        REGW(&OM_24G->TESTCTRL, MASK_1REG(OM24G_TEST_PAT, payload));
        // CE High
        OM24G_CE_HIGH();
    } else {
        // CE Low
        OM24G_CE_LOW();
        // diable single tone
        REGW1(&OM_PHY->REG_PHY_RST_N, PHY_REG_PHY_RST_N_REG_PHY_RST_N_MASK);  //reset phy
        REGW(&OM_24G->TESTCTRL, MASK_1REG(OM24G_CONT_WAVE, 0));
        REGW(&OM_24G->TESTCTRL, MASK_1REG(OM24G_TEST_PAT_EN, 0));

        drv_pmu_ana_enable(false, PMU_ANA_RF);
        REGW0(&OM_24G->PKTCTRL0, OM24G_PKTCTRL0_MAC_SEL_MASK);
        DRV_RCC_CLOCK_ENABLE(RCC_CLK_2P4, 0U);
        DRV_RCC_CLOCK_ENABLE(RCC_CLK_PHY, 0U);
    }
}

/**
 *******************************************************************************
 * @brief  rf full rx enable
 *
 * @param[in] enable  enable
 * @param[in] freq  2402MHz ... 2480MHz
 *******************************************************************************
 **/
void drv_rf_full_rx_enable(bool enable, uint32_t freq)
{
    if (enable) {
        drv_pmu_ana_enable(enable, PMU_ANA_RF);
        DRV_RCC_ANA_CLK_ENABLE();
        // frequency
        REGW(&OM_DAIF->FREQ_CFG0, MASK_2REG(DAIF_FREQ_REG_ME, 1, DAIF_FREQ_REG_MO, freq));
        // Do RX
        REGW(&OM_DAIF->VCO_CTRL0, MASK_3REG(DAIF_TRX_DBG, 1, DAIF_RX_EN_MO, 0, DAIF_TX_EN_MO, 0));
        DRV_DELAY_US(100);
        REGW(&OM_DAIF->VCO_CTRL0, MASK_3REG(DAIF_TRX_DBG, 1, DAIF_RX_EN_MO, 1, DAIF_TX_EN_MO, 0));
        DRV_RCC_ANA_CLK_RESTORE();
    } else {
        DRV_RCC_ANA_CLK_ENABLE();
        REGW(&OM_DAIF->FREQ_CFG0, MASK_1REG(DAIF_FREQ_REG_ME, 0));
        REGW(&OM_DAIF->VCO_CTRL0, MASK_3REG(DAIF_TRX_DBG, 0, DAIF_RX_EN_MO, 0, DAIF_TX_EN_MO, 0));
        DRV_RCC_ANA_CLK_RESTORE();
        drv_pmu_ana_enable(enable, PMU_ANA_RF);
    }
}

/**
 *******************************************************************************
 * @brief  rf tx power set
 *
 * @param[in] power  power
 *******************************************************************************
 **/
void drv_rf_tx_power_set(bool auto_ctrl_by_ble, rf_tx_power_t power)
{
    extern pmu_dcdc_info_t pmu_dcdc_info;
    bool high_tx_power_mode = false;
    int8_t pmu_ldo_v1p2_vbat = drv_calib_repair_env.ana_ldo;
    pmu_dcdc_info.dcdc_vout = drv_calib_repair_env.dcdc_vout;
    int8_t pmu_pll_vco_vout = drv_calib_repair_env.vco_ldo;

    DRV_RCC_ANA_CLK_ENABLE();

    if (auto_ctrl_by_ble) {
        // DBG=1: MO_REG ctrl PA
        // DBG=0: SEL ctrl PA
        //   SEL=0: REG_TABLE ctrl PA
        //   SEL=1: RW ctrl PA
        REGW(&OM_DAIF->PA_CNS, MASK_2REG(DAIF_PA_DBG, 0, DAIF_PA_GAIN_IDX_REG, 1));
    } else {
        if (power >= RF_TX_POWER_2P5DBM) {
            // high tx power mode
            high_tx_power_mode = true;
            switch (power) {
                case RF_TX_POWER_9P5DBM:
                    pmu_dcdc_info.dcdc_vout += 13;
                    pmu_ldo_v1p2_vbat += 10;
                    REGW(&OM_DAIF->PA_CNS, MASK_2REG(DAIF_EN_BYPASS_PALDO, 1, DAIF_PA_GAIN_IDX_REG, 18));
                    REGW(&OM_DAIF->PA_CTRL, MASK_1REG(DAIF_TRS_ANTCAP_CT, 0xF));
                    break;

                case RF_TX_POWER_9DBM:
                    pmu_dcdc_info.dcdc_vout += 10;
                    pmu_ldo_v1p2_vbat += 8;
                    REGW(&OM_DAIF->PA_CNS, MASK_2REG(DAIF_EN_BYPASS_PALDO, 1, DAIF_PA_GAIN_IDX_REG, 18));
                    REGW(&OM_DAIF->PA_CTRL, MASK_1REG(DAIF_TRS_ANTCAP_CT, 0xF));
                    break;

                case RF_TX_POWER_8P5DBM:
                    pmu_dcdc_info.dcdc_vout += 7;
                    pmu_ldo_v1p2_vbat += 5;
                    REGW(&OM_DAIF->PA_CNS, MASK_2REG(DAIF_EN_BYPASS_PALDO, 1, DAIF_PA_GAIN_IDX_REG, 18));
                    REGW(&OM_DAIF->PA_CTRL, MASK_1REG(DAIF_TRS_ANTCAP_CT, 0xF));
                    break;

                case RF_TX_POWER_8DBM:
                    pmu_dcdc_info.dcdc_vout += 7;
                    pmu_ldo_v1p2_vbat += 5;
                    REGW(&OM_DAIF->PA_CNS, MASK_2REG(DAIF_EN_BYPASS_PALDO, 1, DAIF_PA_GAIN_IDX_REG, 16));
                    REGW(&OM_DAIF->PA_CTRL, MASK_1REG(DAIF_TRS_ANTCAP_CT, 0xF));
                    break;

                case RF_TX_POWER_7P5DBM:
                    pmu_dcdc_info.dcdc_vout += 3;
                    pmu_ldo_v1p2_vbat += 2;
                    REGW(&OM_DAIF->PA_CNS, MASK_2REG(DAIF_EN_BYPASS_PALDO, 1, DAIF_PA_GAIN_IDX_REG, 18));
                    REGW(&OM_DAIF->PA_CTRL, MASK_1REG(DAIF_TRS_ANTCAP_CT, 0xF));
                    break;

                case RF_TX_POWER_7DBM:
                    pmu_dcdc_info.dcdc_vout += 3;
                    pmu_ldo_v1p2_vbat += 2;
                    REGW(&OM_DAIF->PA_CNS, MASK_2REG(DAIF_EN_BYPASS_PALDO, 1, DAIF_PA_GAIN_IDX_REG, 16));
                    REGW(&OM_DAIF->PA_CTRL, MASK_1REG(DAIF_TRS_ANTCAP_CT, 0xF));
                    break;

                case RF_TX_POWER_6P5DBM:
                    pmu_dcdc_info.dcdc_vout += 3;
                    pmu_ldo_v1p2_vbat += 2;
                    REGW(&OM_DAIF->PA_CNS, MASK_2REG(DAIF_EN_BYPASS_PALDO, 1, DAIF_PA_GAIN_IDX_REG, 14));
                    REGW(&OM_DAIF->PA_CTRL, MASK_1REG(DAIF_TRS_ANTCAP_CT, 0xF));
                    break;

                case RF_TX_POWER_6DBM:
                    pmu_dcdc_info.dcdc_vout += 3;
                    pmu_ldo_v1p2_vbat += 2;
                    REGW(&OM_DAIF->PA_CNS, MASK_2REG(DAIF_EN_BYPASS_PALDO, 1, DAIF_PA_GAIN_IDX_REG, 13));
                    REGW(&OM_DAIF->PA_CTRL, MASK_1REG(DAIF_TRS_ANTCAP_CT, 0xF));
                    break;

                case RF_TX_POWER_5P5DBM:
                    pmu_dcdc_info.dcdc_vout += 3;
                    pmu_ldo_v1p2_vbat += 2;
                    REGW(&OM_DAIF->PA_CNS, MASK_2REG(DAIF_EN_BYPASS_PALDO, 1, DAIF_PA_GAIN_IDX_REG, 11));
                    REGW(&OM_DAIF->PA_CTRL, MASK_1REG(DAIF_TRS_ANTCAP_CT, 0xF));
                    break;

                case RF_TX_POWER_5DBM:
                    pmu_dcdc_info.dcdc_vout += 3;
                    pmu_ldo_v1p2_vbat += 2;
                    REGW(&OM_DAIF->PA_CNS, MASK_2REG(DAIF_EN_BYPASS_PALDO, 1, DAIF_PA_GAIN_IDX_REG, 10));
                    REGW(&OM_DAIF->PA_CTRL, MASK_1REG(DAIF_TRS_ANTCAP_CT, 0xF));
                    break;

                case RF_TX_POWER_4P5DBM:
                    pmu_dcdc_info.dcdc_vout += 3;
                    pmu_ldo_v1p2_vbat += 2;
                    REGW(&OM_DAIF->PA_CNS, MASK_2REG(DAIF_EN_BYPASS_PALDO, 1, DAIF_PA_GAIN_IDX_REG, 14));
                    REGW(&OM_DAIF->PA_CTRL, MASK_1REG(DAIF_TRS_ANTCAP_CT, 0x0));
                    break;

                case RF_TX_POWER_4DBM:
                    pmu_dcdc_info.dcdc_vout += 3;
                    pmu_ldo_v1p2_vbat += 2;
                    REGW(&OM_DAIF->PA_CNS, MASK_2REG(DAIF_EN_BYPASS_PALDO, 1, DAIF_PA_GAIN_IDX_REG, 10));
                    REGW(&OM_DAIF->PA_CTRL, MASK_1REG(DAIF_TRS_ANTCAP_CT, 0x0));
                    break;

                case RF_TX_POWER_3P5DBM:
                    pmu_dcdc_info.dcdc_vout += 3;
                    pmu_ldo_v1p2_vbat += 2;
                    REGW(&OM_DAIF->PA_CNS, MASK_2REG(DAIF_EN_BYPASS_PALDO, 1, DAIF_PA_GAIN_IDX_REG, 8));
                    REGW(&OM_DAIF->PA_CTRL, MASK_1REG(DAIF_TRS_ANTCAP_CT, 0x0));
                    break;

                case RF_TX_POWER_3DBM:
                    pmu_dcdc_info.dcdc_vout += 3;
                    pmu_ldo_v1p2_vbat += 2;
                    REGW(&OM_DAIF->PA_CNS, MASK_2REG(DAIF_EN_BYPASS_PALDO, 1, DAIF_PA_GAIN_IDX_REG, 6));
                    REGW(&OM_DAIF->PA_CTRL, MASK_1REG(DAIF_TRS_ANTCAP_CT, 0x0));
                    break;

                case RF_TX_POWER_2P5DBM:
                    pmu_dcdc_info.dcdc_vout += 3;
                    pmu_ldo_v1p2_vbat += 2;
                    REGW(&OM_DAIF->PA_CNS, MASK_2REG(DAIF_EN_BYPASS_PALDO, 1, DAIF_PA_GAIN_IDX_REG, 5));
                    REGW(&OM_DAIF->PA_CTRL, MASK_1REG(DAIF_TRS_ANTCAP_CT, 0x0));
                    break;

                default:
                    OM_ASSERT(0);
                    break;
            }
        } else {
            // normal tx power mode
            REGW(&OM_DAIF->PA_CNS, MASK_2REG(DAIF_EN_BYPASS_PALDO, 0, DAIF_PA_GAIN_IDX_REG, power));
            REGW(&OM_DAIF->PA_CTRL, MASK_1REG(DAIF_TRS_ANTCAP_CT, 0x0));
        }
    }

    // check temperature
    if (!high_tx_power_mode && drv_calib_repair_env.temperature > 70) {
        pmu_dcdc_info.dcdc_vout += 3;
        pmu_ldo_v1p2_vbat += 2;
    }

    if (drv_calib_repair_env.temperature > 70) {
        pmu_pll_vco_vout += 1;
        pmu_pll_vco_vout = (pmu_pll_vco_vout > 0x7) ? 0x7 : pmu_pll_vco_vout;
    }

    pmu_dcdc_info.dcdc_vout_is_set = true;
    if (drv_pmu_dcdc_is_enabled()) {
        REGW(&OM_PMU->ANA_PD_1, MASK_1REG_SSAT(PMU_ANA_PD_1_DCDC_VOUT, pmu_dcdc_info.dcdc_vout));
    } else {
        REGW(&OM_PMU->ANA_PD_1, MASK_1REG(PMU_ANA_PD_1_DCDC_VOUT, 0));
    }
    REGW(&OM_PMU->OM26B_CFG0, MASK_1REG_SSAT(PMU_OM26B_CFG0_LDO_ANA1P2_TRIM, pmu_ldo_v1p2_vbat));
    REGW(&OM_DAIF->PLL_CTRL0, MASK_1REG(DAIF_CTRL_LDO_VCO, pmu_pll_vco_vout));

    drv_calib_rf_store();

    DRV_RCC_ANA_CLK_RESTORE();
}

void drv_rf_rx_mode_set(rf_rx_mode_t mode)
{
    switch (mode) {
        case RF_RX_MODE_DEFAULT:
            REGW(&OM_DAIF->LNA_MIX_CFG, MASK_3REG(DAIF_RF_CONSTGM_CT, 0x0, DAIF_RF_LNA_VBCT, 0x3, DAIF_RF_MIX_VBCT, 0x3));
            REGW(&OM_DAIF->TIA_DCOC_CFG, MASK_2REG(DAIF_TIA_GAIN_ME, 1, DAIF_TIA_GAIN_MO, 3));
            REGW(&OM_DAIF->MIX_CFG0, MASK_1REG(DAIF_TIA_IBCT, 1));
            break;
        case RF_RX_MODE_HIGH_PERFORMANCE:
            REGW(&OM_DAIF->LNA_MIX_CFG, MASK_3REG(DAIF_RF_CONSTGM_CT, 0x3, DAIF_RF_LNA_VBCT, 0x3, DAIF_RF_MIX_VBCT, 0x3));
            REGW(&OM_DAIF->TIA_DCOC_CFG, MASK_2REG(DAIF_TIA_GAIN_ME, 1, DAIF_TIA_GAIN_MO, 3));
            REGW(&OM_DAIF->MIX_CFG0, MASK_1REG(DAIF_TIA_IBCT, 1));
            break;
        case RF_RX_MODE_LOW_POWER:
            REGW(&OM_DAIF->LNA_MIX_CFG, MASK_3REG(DAIF_RF_CONSTGM_CT, 0x2, DAIF_RF_LNA_VBCT, 0x1, DAIF_RF_MIX_VBCT, 0x3));
            REGW(&OM_DAIF->TIA_DCOC_CFG, MASK_2REG(DAIF_TIA_GAIN_ME, 1, DAIF_TIA_GAIN_MO, 3));
            REGW(&OM_DAIF->MIX_CFG0, MASK_1REG(DAIF_TIA_IBCT, 0));
            break;
        default:
            break;
    }
    drv_calib_rf_store();
}

#endif  /* RTE_RADIO */


/** @} */
