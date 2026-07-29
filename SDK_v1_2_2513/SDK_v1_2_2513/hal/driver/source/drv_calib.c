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
 * @brief    CALIB driver source file
 * @details  CALIB driver source file
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


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    bool rc32k_is_calibed;

    // Saved calibed data
    uint8_t icon_vco;
    uint8_t trs_antcap_ct;
    uint8_t tia_ibct;
} drv_calib_t;


/*******************************************************************************
 * LOCAL VARIABLES
 */
static drv_calib_t drv_calib_env = {
    .rc32k_is_calibed = false
};


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief  calib patch
 *
 * @param[in] temperature  temperature
 *******************************************************************************
 **/
static void drv_calib_patch(bool is_calib_start)
{
    if (is_calib_start) {
        //增大PA开启后等待时间，避免PLL频率漂移
        REGW(&OM_DAIF->MAIN_ST_CFG0, MASK_1REG(DAIF_TXLDO_WAIT, 0x180));//(Auto Restore)
        //默认值，不需要修改
        //REGW(&OM_DAIF->IR_RX_CFG, MASK_1REG(DAIF_WIN_CNT_THRESHOLD, 0xC000));
        //增大模拟调制增益
        REGW(&OM_DAIF->PLL_CTRL2, MASK_1REG(DAIF_EN_KVCO2, 4U));//(Auto Restore)
        //better for TX drift
        REGW(&OM_DAIF->PLL_CTRL0, MASK_3REG(DAIF_BP_DIOX, 0,
                                            DAIF_SEL_KVCO, 3,
                                            DAIF_CTRL_LDO_BUF, 7));//(Auto Restore)
        REGW(&OM_DAIF->PLL_CTRL1, MASK_1REG(DAIF_CON_BIAS_IDAC_PLL, 48));//(Auto Restore)
        //提高RF电源电压增益，提高灵敏度，固定IFLDO和RXADC电压
        REGW(&OM_PMU->OM26B_CFG0, MASK_2REG(PMU_OM26B_CFG0_RF_LDO_TRIM, 3, PMU_OM26B_CFG0_RXADC_LDO_TRIM, 1));
        REGW(&OM_PMU->ANA_REG, MASK_1REG(PMU_ANA_REG_IF_LDO_TRIM, 2));
        //ramp的第二种模式,解决旁瓣杂散问题;
        REGW1(&OM_DAIF->TRX_SW_CFG, DAIF_PA_PATTERN_SEL_MASK);//(Auto Restore)

        drv_rf_rx_mode_set((rf_rx_mode_t)RTE_RADIO_MODE);

        // 抗干扰策略2;
        REGWA(&OM_DAIF->FILT_AGC_LUT_REG0, MASK_3REG(DAIF_FILT_AGC_LUT_0, 0x104, DAIF_FILT_AGC_LUT_1, 0x105, DAIF_FILT_AGC_LUT_2, 0x108));
        REGWA(&OM_DAIF->FILT_AGC_LUT_REG1, MASK_3REG(DAIF_FILT_AGC_LUT_3, 0x10B, DAIF_FILT_AGC_LUT_4, 0x110, DAIF_FILT_AGC_LUT_5, 0x116));
        REGWA(&OM_DAIF->FILT_AGC_LUT_REG2, MASK_3REG(DAIF_FILT_AGC_LUT_6, 0x120, DAIF_FILT_AGC_LUT_7, 0x12C, DAIF_FILT_AGC_LUT_8, 0x140));
    } else {
        // restore code
        drv_calib_rf_restore();
        //射频校准结束之后需要pkd常开，避免对信号干扰；
        REGW(&OM_DAIF->PD_CFG0, MASK_2REG(DAIF_PKDT_PD_ME, 1, DAIF_PKDT_PD_MO, 0));//(Auto Restore)
    }
}
/**
 *******************************************************************************
 * @brief  Auto frequency calibration
 *******************************************************************************
 */
static void drv_calib_rf_afc(int16_t t)
{
    // initial afc/vtrack table
    for (int i=0; i < 80; ++i) {
        REGW(&OM_DAIF->PLL_LUT_DBG, MASK_3REG(DAIF_PLL_LUT_DATA, 0x1124,
                                              DAIF_PLL_LUT_IDX, i,
                                              DAIF_PLL_LUT_WR, 1));
        // wait lut wr done
        while (OM_DAIF->PLL_LUT_DBG & DAIF_PLL_LUT_WR_MASK);
    }
    // sw power on analog blocks
    // power on pll analog
    REGW(&OM_DAIF->PD_CFG1, MASK_4REG(DAIF_RFPLL_PD_ALL_ME, 1, DAIF_RFPLL_PD_ALL_MO, 0, DAIF_RFPLL_PD_VCDET_ME, 1, DAIF_RFPLL_PD_VCDET_MO, 0));
    REGW(&OM_DAIF->PD_CFG2, MASK_2REG(DAIF_RFPLL_PD_TXDAC_MO, 0, DAIF_RFPLL_PD_TXDAC_ME, 1));

    // digital ctrl
    REGW(&OM_DAIF->VCO_CTRL0, MASK_1REG(DAIF_AFC_EN_ME, 0));

    #if 1
    //do amp cali
    REGW1(&OM_DAIF->VCO_CTRL1, DAIF_PEAKDET_START_MASK);
    // wait amp done
    while (OM_DAIF->VCO_CTRL1 & DAIF_PEAKDET_START_MASK);
    #endif
    REGW(&OM_DAIF->PD_CFG1, MASK_1REG(DAIF_RFPLL_PD_VCDET_ME, 0));


    // FIXME: Digital BUG: 2M issue
    #if (RTE_CALIB_AFC_FAST)
    REGW(&OM_DAIF->PLL_AFC_CTRL, MASK_2REG(DAIF_PLL_AFC_START_FREQ, 2402, DAIF_PLL_AFC_STOP_FREQ, 2480));
    #else
    REGW(&OM_DAIF->PLL_AFC_CTRL, MASK_2REG(DAIF_PLL_AFC_START_FREQ, 2358, DAIF_PLL_AFC_STOP_FREQ, 2512));
    #endif

    REGW1(&OM_DAIF->MIX_CFG0, DAIF_SDM_DIV_MASK);

    //do vco afc
    REGW1(&OM_DAIF->VCO_CTRL0, DAIF_AFC_START_MASK);
    // wait afc done
    while (OM_DAIF->VCO_CTRL0 & DAIF_AFC_START_MASK);

    //pd_lotx power on
    REGW(&OM_DAIF->PD_CFG1, MASK_2REG(DAIF_RFPLL_PD_LOTX_MO, 0, DAIF_RFPLL_PD_LOTX_ME, 1));

    #if (RTE_CALIB_USE_FT_KDCO_DATA)
    if (t >= -20 && t <= 70) {
        // use kdco data in FT
        REGW(&OM_DAIF->KDCO_LUT_1M_REG0, MASK_5REG(DAIF_KDCO_LUT_1M_0, drv_calib_repair_env.kdco_lut_1m_2406,
                                                DAIF_KDCO_LUT_1M_1, drv_calib_repair_env.kdco_lut_1m_2406,
                                                DAIF_KDCO_LUT_1M_2, drv_calib_repair_env.kdco_lut_1m_2406,
                                                DAIF_KDCO_LUT_1M_3, drv_calib_repair_env.kdco_lut_1m_2406,
                                                DAIF_KDCO_LUT_1M_4, drv_calib_repair_env.kdco_lut_1m_2420));
        REGW(&OM_DAIF->KDCO_LUT_1M_REG1, MASK_5REG(DAIF_KDCO_LUT_1M_5, drv_calib_repair_env.kdco_lut_1m_2434,
                                                DAIF_KDCO_LUT_1M_6, drv_calib_repair_env.kdco_lut_1m_2448,
                                                DAIF_KDCO_LUT_1M_7, drv_calib_repair_env.kdco_lut_1m_2462,
                                                DAIF_KDCO_LUT_1M_8, drv_calib_repair_env.kdco_lut_1m_2476,
                                                DAIF_KDCO_LUT_1M_9, drv_calib_repair_env.kdco_lut_1m_2476));
        REGW(&OM_DAIF->KDCO_LUT_2M_REG0, MASK_4REG(DAIF_KDCO_LUT_2M_0, drv_calib_repair_env.kdco_lut_2m_2406,
                                                DAIF_KDCO_LUT_2M_1, drv_calib_repair_env.kdco_lut_2m_2420,
                                                DAIF_KDCO_LUT_2M_2, drv_calib_repair_env.kdco_lut_2m_2434,
                                                DAIF_KDCO_LUT_2M_3, drv_calib_repair_env.kdco_lut_2m_2448));
        REGW(&OM_DAIF->KDCO_LUT_2M_REG1, MASK_3REG(DAIF_KDCO_LUT_2M_4, drv_calib_repair_env.kdco_lut_2m_2462,
                                                DAIF_KDCO_LUT_2M_5, drv_calib_repair_env.kdco_lut_2m_2476,
                                                DAIF_KDCO_LUT_1M_10, drv_calib_repair_env.kdco_lut_1m_2476));
    } else
    #endif
    {
        // do kdco
        uint32_t win_cnt;
        win_cnt = 0xD700;
        #if (RTE_CALIB_USE_FT_DF1_DATA)
        int32_t df1_delta_percent = 0;
        // Calculate the delta percentage for 1M, take the floor of the value
        // to make sure the repaired value is lower than the original
        df1_delta_percent = (int32_t)((drv_calib_repair_env.kdco_df1_1m - 250.0f) / 250.0f * 100.0f);
        win_cnt = win_cnt * (100 + df1_delta_percent) / 100;
        #endif
        REGW(&OM_DAIF->IR_RX_CFG, MASK_1REG(DAIF_WIN_CNT_THRESHOLD, win_cnt));

        //do kdco -- 1M
        REGW(&OM_DAIF->VCO_CTRL1, MASK_2REG(DAIF_BLE_1M_2M_SEL_MO, 0, DAIF_BLE_1M_2M_SEL_ME, 1));
        // try kdco 1M mode
        REGW1(&OM_DAIF->VCO_CTRL0, DAIF_KDCO_START_MASK);
        //wait kdco done
        while (OM_DAIF->VCO_CTRL0 & DAIF_KDCO_START_MASK);

        win_cnt = 0xCB00;
        #if (RTE_CALIB_USE_FT_DF1_DATA)
        // Calculate the delta percentage for 2M, take the floor of the value
        // to make sure the repaired value is lower than the original
        df1_delta_percent = (int32_t)((drv_calib_repair_env.kdco_df1_2m - 500.0f) / 500.0f * 100.0f);
        win_cnt = win_cnt * (100 + df1_delta_percent) / 100;
        #endif
        REGW(&OM_DAIF->IR_RX_CFG, MASK_1REG(DAIF_WIN_CNT_THRESHOLD, win_cnt));

        // FIXME: Digital BUG: 2M issue
        #if (RTE_CALIB_AFC_FAST)
        REGW(&OM_DAIF->PLL_AFC_CTRL, MASK_2REG(DAIF_PLL_AFC_START_FREQ, 2402, DAIF_PLL_AFC_STOP_FREQ, 2480));
        #else
        REGW(&OM_DAIF->PLL_AFC_CTRL, MASK_2REG(DAIF_PLL_AFC_START_FREQ, 2358, DAIF_PLL_AFC_STOP_FREQ, 2512));
        #endif

        //do kdco -- 2M
        REGW(&OM_DAIF->VCO_CTRL1, MASK_2REG(DAIF_BLE_1M_2M_SEL_MO, 1, DAIF_BLE_1M_2M_SEL_ME, 1));
        // try kdco 2M mode
        REGW1(&OM_DAIF->VCO_CTRL0, DAIF_KDCO_START_MASK);
        //wait kdco done
        while (OM_DAIF->VCO_CTRL0 & DAIF_KDCO_START_MASK);

        // restore 1m/2m FSM ctrl
        REGW(&OM_DAIF->VCO_CTRL1, MASK_1REG(DAIF_BLE_1M_2M_SEL_ME, 0));
    }

    //make analog blocks power controlled by trx FSM
    // change power control by FSM
    //pd_pll_all
    REGW(&OM_DAIF->PD_CFG1, MASK_2REG(DAIF_RFPLL_PD_ALL_ME, 0, DAIF_RFPLL_PD_VCDET_ME, 0));
    REGW(&OM_DAIF->PD_CFG2, MASK_1REG(DAIF_RFPLL_PD_TXDAC_ME, 0));

    //pd_lotx
    REGW(&OM_DAIF->PD_CFG1, MASK_1REG(DAIF_RFPLL_PD_LOTX_ME, 0));
}


/**
 *******************************************************************************
 * @brief  drv calib rf dcoc
 *******************************************************************************
 */
static void drv_calib_rf_dcoc(void)
{
    // do dcoc calib
    REGW1(&OM_DAIF->TIA_CR1, DAIF_TIA_START_MASK);
    // fix bug: When CE is set to high level, resetting the CPU may cause the chip to fail drv_calib_rf_dcoc () calibration.
    REGW(&OM_DAIF->VCO_CTRL0, MASK_3REG(DAIF_TRX_DBG, 1, DAIF_RX_EN_MO, 0, DAIF_TX_EN_MO, 0));
    REGW(&OM_DAIF->VCO_CTRL0, MASK_3REG(DAIF_TRX_DBG, 1, DAIF_RX_EN_MO, 1, DAIF_TX_EN_MO, 0));
    // wait dcoc done
    while (OM_DAIF->TIA_CR1 & DAIF_TIA_START_MASK);
    // fix bug: When CE is set to high level, resetting the CPU may cause the chip to fail drv_calib_rf_dcoc () calibration.
    REGW(&OM_DAIF->VCO_CTRL0, MASK_1REG(DAIF_TRX_DBG, 0));
}

/**
 *******************************************************************************
 * @brief  drv calib rf txfe
 *******************************************************************************
 */
static void drv_calib_rf_txfe(void)
{
    REGW(&OM_DAIF->VCO_CTRL0, MASK_3REG(DAIF_TRX_DBG, 1, DAIF_RX_EN_MO, 0, DAIF_TX_EN_MO, 1));
    DRV_DELAY_US(70);
    REGW(&OM_DAIF->PA_GAIN_REG_1, MASK_6REG(DAIF_PA_GAIN_1_DRV0_NCT, 16, DAIF_PA_GAIN_1_DRV0_PCT, 16, DAIF_PA_GAIN_2_DRV0_NCT, 16,
                                            DAIF_PA_GAIN_2_DRV0_PCT, 16, DAIF_PA_GAIN_1_DRIV_NUM, 0, DAIF_PA_GAIN_2_DRIV_NUM, 1));
    REGW(&OM_DAIF->PA_GAIN_REG_2, MASK_6REG(DAIF_PA_GAIN_1_DRV0_NCT, 16, DAIF_PA_GAIN_1_DRV0_PCT, 16, DAIF_PA_GAIN_2_DRV0_NCT, 16,
                                            DAIF_PA_GAIN_2_DRV0_PCT, 16, DAIF_PA_GAIN_1_DRIV_NUM, 3, DAIF_PA_GAIN_2_DRIV_NUM, 5));
    REGW(&OM_DAIF->PA_GAIN_REG_3, MASK_6REG(DAIF_PA_GAIN_1_DRV0_NCT, 16, DAIF_PA_GAIN_1_DRV0_PCT, 16, DAIF_PA_GAIN_2_DRV0_NCT, 16,
                                            DAIF_PA_GAIN_2_DRV0_PCT, 16, DAIF_PA_GAIN_1_DRIV_NUM, 11, DAIF_PA_GAIN_2_DRIV_NUM, 13));///13  15
    REGW(&OM_DAIF->PA_GAIN_REG_4, MASK_6REG(DAIF_PA_GAIN_1_DRV0_NCT, 16, DAIF_PA_GAIN_1_DRV0_PCT, 16, DAIF_PA_GAIN_2_DRV0_NCT, 16,
                                            DAIF_PA_GAIN_2_DRV0_PCT, 16, DAIF_PA_GAIN_1_DRIV_NUM, 15, DAIF_PA_GAIN_2_DRIV_NUM, 16));
    REGW(&OM_DAIF->PA_GAIN_REG_5, MASK_6REG(DAIF_PA_GAIN_1_DRV0_NCT, 16, DAIF_PA_GAIN_1_DRV0_PCT, 16, DAIF_PA_GAIN_2_DRV0_NCT, 16,
                                            DAIF_PA_GAIN_2_DRV0_PCT, 16, DAIF_PA_GAIN_1_DRIV_NUM, 19, DAIF_PA_GAIN_2_DRIV_NUM, 21));
    REGW(&OM_DAIF->PA_GAIN_REG_6, MASK_6REG(DAIF_PA_GAIN_1_DRV0_NCT, 16, DAIF_PA_GAIN_1_DRV0_PCT, 16, DAIF_PA_GAIN_2_DRV0_NCT, 16,
                                            DAIF_PA_GAIN_2_DRV0_PCT, 16, DAIF_PA_GAIN_1_DRIV_NUM, 24, DAIF_PA_GAIN_2_DRIV_NUM, 25));
    REGW(&OM_DAIF->PA_GAIN_REG_7, MASK_6REG(DAIF_PA_GAIN_1_DRV0_NCT, 16, DAIF_PA_GAIN_1_DRV0_PCT, 16, DAIF_PA_GAIN_2_DRV0_NCT, 16,
                                            DAIF_PA_GAIN_2_DRV0_PCT, 16, DAIF_PA_GAIN_1_DRIV_NUM, 28, DAIF_PA_GAIN_2_DRIV_NUM, 34));
    REGW(&OM_DAIF->PA_GAIN_REG_8, MASK_6REG(DAIF_PA_GAIN_1_DRV0_NCT, 16, DAIF_PA_GAIN_1_DRV0_PCT, 16, DAIF_PA_GAIN_2_DRV0_NCT, 16,
                                            DAIF_PA_GAIN_2_DRV0_PCT, 16, DAIF_PA_GAIN_1_DRIV_NUM, 36, DAIF_PA_GAIN_2_DRIV_NUM, 44));
    REGW(&OM_DAIF->PA_GAIN_REG_9, MASK_6REG(DAIF_PA_GAIN_1_DRV0_NCT, 16, DAIF_PA_GAIN_1_DRV0_PCT, 16, DAIF_PA_GAIN_2_DRV0_NCT, 16,
                                            DAIF_PA_GAIN_2_DRV0_PCT, 16, DAIF_PA_GAIN_1_DRIV_NUM, 55, DAIF_PA_GAIN_2_DRIV_NUM, 63));
    //防止二次校准出问题，校准前需要关掉这个DONE信号
    REGW0(&OM_DAIF->PA_GAIN_CFG, DAIF_PA_GAIN_CALI_DONE_MASK);
    //do pag calib
    REGW1(&OM_DAIF->PA_GAIN_CFG, DAIF_PA_GAIN_START_MASK);
    //wait pag done
    om_error_t ret; // Max: 2ms
    DRV_WAIT_MS_UNTIL_TO(OM_DAIF->PA_GAIN_CFG & DAIF_PA_GAIN_START_MASK, 4, ret); (void)ret;
    REGW(&OM_DAIF->VCO_CTRL0, MASK_3REG(DAIF_TRX_DBG, 0, DAIF_RX_EN_MO, 0, DAIF_TX_EN_MO, 0));
    // default is 0DBM
    ///REGW(&OM_DAIF->PA_CNS, MASK_2REG(DAIF_EN_BYPASS_PALDO,0, DAIF_PA_GAIN_IDX_REG,RF_TX_POWER_0DBM));
}

/**
 *******************************************************************************
 * @brief  calib rc32k accuracy check
 *
 * @return accuracy
 *******************************************************************************
 **/
static uint32_t drv_calib_rc32k_count_by_xtal32m(uint32_t win_32k_num)
{
    //check rc32k clock again with intr enabled
    REGWA(&OM_DAIF->CLK_CHK_CNS, MASK_5REG(DAIF_RX_RCCAL_START, 1,
                                           DAIF_CLK_CHK_INTR_EN, 1,
                                           DAIF_CLK_CHK_INTR_ST, 1,
                                           DAIF_CLK_CHK_REF_SEL, 0,
                                           DAIF_CLK_CHK_WIN_CNT_THRSH, win_32k_num)); // win_cnt: 32k period number
    // wait rc 32k check done -- 2
    while (OM_DAIF->CLK_CHK_CNS & DAIF_RX_RCCAL_START_MASK);
    DRV_DELAY_US(100);

    // got rc 32k check status
    return OM_DAIF->CLK_CHK_STATUS; // in win_cnt, how many 32MHz number
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief  calib rc32k accuracy check
 *
 * @return ppm
 *******************************************************************************
 **/
int drv_calib_rc32k_accuracy_check(uint32_t win_32k_num)
{
    // >>> 16*(1.0/32768.0)/(1.0/32000000)
    // 15625.0 (62ppm)
    // >>> 32*(1.0/32768.0)/(1.0/32000000)
    // 31250.0 (32ppm)
    // >>> 64*(1.0/32768.0)/(1.0/32000000)
    // 62500.0 (15ppm)

    int clk_num_32m;
    int clk_num_32m_std = win_32k_num * 32000000LL / 32768;

    DRV_RCC_ANA_CLK_ENABLE();

    REGW1(&OM_DAIF->CLK_ENS, DAIF_CC_CLK_EN_MASK);

    clk_num_32m = drv_calib_rc32k_count_by_xtal32m(win_32k_num);

    REGW0(&OM_DAIF->CLK_ENS, DAIF_CC_CLK_EN_MASK);

    DRV_RCC_ANA_CLK_RESTORE();

    return 1000000 * (clk_num_32m - clk_num_32m_std) / clk_num_32m_std;
}

/**
 *******************************************************************************
 * @brief  calib rc32k begin
 *******************************************************************************
 **/
void drv_calib_rc32k_full(void)
{
    // PMU reg bypass disable
    REGW(&OM_PMU->CLK_CTRL_1, MASK_1REG(PMU_CLK_CTRL_1_RC_32K_RCTUNE_SEL, 0));
    while ((OM_PMU->STATUS_READ & PMU_STATUS_READ_SEL_RC32K_TUNE_MASK) != 0);

    // un-reset
    REGW1(&OM_DAIF->RC32K_TUN, DAIF_RC_32K_TUNE_SWRSTN_MASK);

    //rc 32k tune
    // do rc 32k tune 1
    REGW1(&OM_DAIF->RC32K_TUN, DAIF_RC_32K_TUNE_START_MASK);

    // wait rc32k tune 1 done
    while (OM_DAIF->RC32K_TUN & DAIF_RC_32K_TUNE_START_MASK);

    // Save
    REGW(&OM_PMU->CLK_CTRL_2, MASK_1REG(PMU_CLK_CTRL_2_RTUNE_RC32K_REG, REGR(&OM_DAIF->RC32K_TUN_OUT, MASK_POS(DAIF_RC32K_RTUNE))));
    REGW(&OM_PMU->CLK_CTRL_2, MASK_1REG(PMU_CLK_CTRL_2_CTUNE_RC32K_REG, REGR(&OM_DAIF->RC32K_TUN_OUT, MASK_POS(DAIF_RC32K_CTUNE))));
    REGW(&OM_PMU->CLK_CTRL_1, MASK_1REG(PMU_CLK_CTRL_1_RCTUNE_RC32K_UPDATE_REG, 1));
    while (OM_PMU->STATUS_READ & PMU_STATUS_READ_RCTUNE_RC32K_UPDATE_STATUS_MASK);

    // use REG to switch it
    REGW(&OM_PMU->CLK_CTRL_1, MASK_1REG(PMU_CLK_CTRL_1_RC_32K_RCTUNE_SEL, 1));
    while ((OM_PMU->STATUS_READ & PMU_STATUS_READ_SEL_RC32K_TUNE_MASK) == 0);

    // reset
    REGW0(&OM_DAIF->RC32K_TUN, DAIF_RC_32K_TUNE_SWRSTN_MASK);

    // Must delay 1 32k to switch it
    DRV_DELAY_US(60);

    drv_calib_env.rc32k_is_calibed = true;
}

/**
 *******************************************************************************
 * @brief  drv calib rc32k fast
 *******************************************************************************
 */
void drv_calib_rc32k_fast(void)
{
    int jitter_new = 0;
    uint8_t  rc32k_rtune =0;
    uint8_t  rc32k_delta_num = 0;
    bool recalib_32k;

    jitter_new = drv_calib_rc32k_accuracy_check(256);

    do {
        const uint16_t jitter_tab[] = {
            200, 1, 300, 3, 400, 4, 500,   5,  600, 6,
            700, 7, 800, 8, 900, 9, 1000, 10,
        };

        recalib_32k = true;
        for (uint8_t i=0; i < sizeof(jitter_tab)/sizeof(jitter_tab[0]); i+=2) {
            if (abs(jitter_new) <= jitter_tab[i]) {
                rc32k_delta_num = jitter_tab[i + 1];
                recalib_32k = false;
                break;
            }
        }
    } while (0);

    while ((rc32k_delta_num > 0) || recalib_32k) {
        rc32k_rtune = REGR(&OM_PMU->CLK_CTRL_2, MASK_POS(PMU_CLK_CTRL_2_RTUNE_RC32K_REG));
        if (jitter_new >= 0) {
            if (rc32k_rtune <= 0) {
                recalib_32k = true;
            } else {
                rc32k_rtune -= ((rc32k_delta_num >= 4) ? 2 : 1);
            }
        } else {
            if (rc32k_rtune >= 255) {
                recalib_32k = true;
            } else {
                rc32k_rtune += ((rc32k_delta_num >= 4) ? 2 : 1);
            }
        }

        if (recalib_32k) {
            drv_calib_rc32k_full();
            break;
        } else {
            REGW(&OM_PMU->CLK_CTRL_2, MASK_1REG(PMU_CLK_CTRL_2_RTUNE_RC32K_REG, rc32k_rtune));
            REGW(&OM_PMU->CLK_CTRL_1, MASK_2REG(PMU_CLK_CTRL_1_RC_32K_RCTUNE_SEL, 1, PMU_CLK_CTRL_1_RCTUNE_RC32K_UPDATE_REG, 1));
        }

        jitter_new = drv_calib_rc32k_accuracy_check(128);
        if ((jitter_new <= 200) && (jitter_new >= -200)) {
            break;
        }

        rc32k_delta_num--;
        if (rc32k_delta_num == 0) {
            if ((jitter_new >= 500) || (jitter_new <= -500)) {
                drv_calib_rc32k_full();
                break;
            }
        }
    }
}

/**
 *******************************************************************************
 * @brief  calib rc32k to 62ppm
 *******************************************************************************
 **/
void drv_calib_rc32k(void)
{
    // Eanble depend clock
    drv_pmu_ana_enable(true, PMU_ANA_CALIB_RC32K);

    // do calib rc32k
    if (drv_calib_env.rc32k_is_calibed) {
        drv_calib_rc32k_fast();
    } else {
        drv_calib_rc32k_full();
    }

    // Disable depend clock
    drv_pmu_ana_enable(false, PMU_ANA_CALIB_RC32K);
}

/**
 *******************************************************************************
 * @brief calib sys rc32m
 *******************************************************************************
 **/
void drv_calib_sys_rc32m(void)
{
    // clock
    REGW1(&OM_DAIF->CLK_ENS, DAIF_RC_32M_TUNE_CLK_EN_MASK | DAIF_XTAL32M_CTRL_CLK_EN_MASK);

    // FSM
    REGW(&OM_PMU->CLK_CTRL_1, MASK_1REG(PMU_CLK_CTRL_1_CTUNE_OSC_SEL, 0));

    //rc 32m tune
    // do rc 32m tune 1
    REGW1(&OM_DAIF->RC32M_TUN, DAIF_RC_32M_TUNE_START_MASK);
    // wait rc32m tune 1 done
    while (OM_DAIF->RC32M_TUN & DAIF_RC_32M_TUNE_START_MASK);

    // save
    REGW(&OM_PMU->CLK_CTRL_1, MASK_2REG(PMU_CLK_CTRL_1_CTUNE_OSC_SEL, 1,
                                        PMU_CLK_CTRL_1_CT_OSC32M, REGR(&OM_DAIF->RC32M_TUN, MASK_POS(DAIF_OSC32M_CT))));
}

/**
 *******************************************************************************
 * @brief  drv calib sys rc
 *******************************************************************************
 */
void drv_calib_sys_rc(void)
{
    uint32_t clk_ens_save = OM_DAIF->CLK_ENS;
    uint32_t clk_cfg_save = OM_DAIF->CLK_CFG;
    // clock
    REGW1(&OM_DAIF->CLK_ENS, DAIF_MAIN_FSM_CLK_EN_MASK | DAIF_PLL_CLK_REF_EN_MASK | DAIF_PLL_VTRACK_CLK_EN_MASK);
    REGW1(&OM_DAIF->CLK_CFG, DAIF_XTAL32M_EN_CKO16M_DIG_MASK | DAIF_XTAL32M_EN_CKO16M_ANA_MASK);
    // Power
    REGW(&OM_DAIF->PD_CFG0, MASK_2REG(DAIF_RF_LDO_PD_MO, 0, DAIF_RF_LDO_PD_ME, 1));
    DRV_DELAY_US(10);

    //do pag calib
    REGW1(&OM_DAIF->RX_RCCAL_CTRL, DAIF_RX_RCCAL_START_MASK);
    // fix bug: When CE is set to high level, resetting the CPU may cause the chip to fail drv_calib_sys_rc () calibration.
    REGW(&OM_DAIF->VCO_CTRL0, MASK_3REG(DAIF_TRX_DBG, 1, DAIF_RX_EN_MO, 0, DAIF_TX_EN_MO, 0));
    REGW(&OM_DAIF->VCO_CTRL0, MASK_3REG(DAIF_TRX_DBG, 1, DAIF_RX_EN_MO, 1, DAIF_TX_EN_MO, 0));
    // wait pag done
    while (OM_DAIF->RX_RCCAL_CTRL & DAIF_RX_RCCAL_START_MASK);

    // Save
    REGW(&OM_PMU->CLK_CTRL_1, MASK_1REG(PMU_CLK_CTRL_1_CT_FDOUBLER,
                                        REGR(&OM_DAIF->TIA_DCOC_R3, MASK_POS(DAIF_TIA_CTUNE_FSM))));

                            // Power
    REGW(&OM_DAIF->PD_CFG0, MASK_2REG(DAIF_RF_LDO_PD_MO, 1, DAIF_RF_LDO_PD_ME, 0));

    // fix bug: When CE is set to high level, resetting the CPU may cause the chip to fail drv_calib_rf_dcoc () calibration.
    REGW(&OM_DAIF->VCO_CTRL0, MASK_1REG(DAIF_TRX_DBG, 0));
    // Wait idle
    DRV_DAIF_WAIT_IDLE();
    // clock
    OM_DAIF->CLK_ENS = clk_ens_save;
    OM_DAIF->CLK_CFG = clk_cfg_save;
}

/**
 *******************************************************************************
 * @brief  drv calib rf
 *******************************************************************************
 */
void drv_calib_rf(int16_t t)
{
    if (SYS_IS_FPGA()) {
        return;
    }

    drv_pmu_ana_enable(true, PMU_ANA_RF);
    // enable clock
    uint32_t clk_ens_save = OM_DAIF->CLK_ENS;
    REGW1(&OM_DAIF->CLK_ENS, DAIF_PLL_AFC_CLK_EN_MASK | DAIF_RX_AGC_CLK_EN_MASK |
          DAIF_PLL_VTRACK_CLK_EN_MASK | DAIF_PLL_LUT_CLK_EN_MASK | DAIF_MAIN_FSM_CLK_EN_MASK |
          DAIF_PLL_CLK_AFC_EN_MASK | DAIF_SDM_CLK_EN_MASK | DAIF_PLL_CLK_REF_EN_MASK | DAIF_PLL_AMP_CLK_EN_MASK);
    REGW1(&OM_DAIF->CLK_CFG, DAIF_XTAL32M_EN_CKO16M_DIG_MASK|DAIF_XTAL32M_EN_CKO16M_ANA_MASK|DAIF_XTAL32M_EN_CKO16M_PLL_MASK);

    // patch
    drv_calib_patch(true/*is_calib_start*/);

    // do calib
    drv_calib_rf_afc(t);
    drv_calib_rf_dcoc();
    drv_calib_rf_txfe();

    // store calib value
    drv_calib_rf_store();

    // patch
    drv_calib_patch(false/*is_calib_start*/);

    // restore clock
    OM_DAIF->CLK_ENS = clk_ens_save;

    drv_pmu_ana_enable(false, PMU_ANA_RF);
}

/**
 *******************************************************************************
 * @brief  drv calib rf store
 *******************************************************************************
 */
void drv_calib_rf_store(void)
{
    // save icon_vco
    OM_DAIF->DBG_REG = 0x17;
    drv_calib_env.icon_vco = (OM_DAIF->DBG_REG >> 25) & 0x7;
    OM_DAIF->DBG_REG = 0;

    drv_calib_env.tia_ibct = REGR(&OM_DAIF->MIX_CFG0, MASK_POS(DAIF_TIA_IBCT));
    drv_calib_env.trs_antcap_ct = REGR(&OM_DAIF->PA_CTRL, MASK_POS(DAIF_TRS_ANTCAP_CT));
}

/**
 *******************************************************************************
 * @brief  drv calib rf restore
 *******************************************************************************
 */
__RAM_CODES("PM")
void drv_calib_rf_restore(void)
{
    REGW(&OM_DAIF->MIX_CFG1, MASK_2REG(DAIF_ICON_VCO_ME, 0, DAIF_ICON_VCO_MO, drv_calib_env.icon_vco));//睡眠唤醒后需要恢复校准值
    // fix scan sync signal issue
    REGW(&OM_DAIF->MIX_CFG0, MASK_2REG(DAIF_PHY_ADC_WAIT, 1,
                                       DAIF_SDM_DIV, 1));
    // 高中低灵敏度配置，睡眠唤醒后需要恢复
    REGW(&OM_DAIF->MIX_CFG0, MASK_1REG(DAIF_TIA_IBCT, drv_calib_env.tia_ibct));
    // 睡眠唤醒后需要和drv_radio.c里面的drv_rf_tx_power_set设置功率处一样
    REGW(&OM_DAIF->PA_CTRL, MASK_1REG(DAIF_TRS_ANTCAP_CT, drv_calib_env.trs_antcap_ct));

    REGW(&OM_PHY->TX_CTRL1, MASK_1REG(PHY_TX_CTRL1_GF_GCMP1, 0x1));

    #if 0
    //抗干扰策略1
    //有利于抗干扰测试，agc增益只有三档在调节；会比另一种agc增益调节差0.6dB;
    REGW1(&OM_DAIF->AGC_CFG0, DAIF_FILTER_MAX6DB_MASK);
    ///抗干扰策略2;
    //REGWA(&OM_DAIF->FILT_AGC_LUT_REG0, MASK_3REG(DAIF_FILT_AGC_LUT_0, 0x104, DAIF_FILT_AGC_LUT_1, 0x105, DAIF_FILT_AGC_LUT_2, 0x108));//(Auto Restore)
    //REGWA(&OM_DAIF->FILT_AGC_LUT_REG1, MASK_3REG(DAIF_FILT_AGC_LUT_3, 0x10B, DAIF_FILT_AGC_LUT_4, 0x110, DAIF_FILT_AGC_LUT_5, 0x116));//(Auto Restore)
    //REGWA(&OM_DAIF->FILT_AGC_LUT_REG2, MASK_3REG(DAIF_FILT_AGC_LUT_6, 0x120, DAIF_FILT_AGC_LUT_7, 0x12C, DAIF_FILT_AGC_LUT_8, 0x140));//(Auto Restore)
    #endif

    // ramp的第二种模式,解决旁瓣杂散问题;
    // REGW1(&OM_DAIF->TRX_SW_CFG, DAIF_PA_PATTERN_SEL_MASK);//(Auto Restore)

    #if 0 // Fix RX gain
    REGW(&OM_DAIF->LNA_MIX_CFG, MASK_2REG(DAIF_RF_ATTEN_EN_MO, 0, DAIF_RF_ATTEN_EN_ME, 1));
    REGW(&OM_DAIF->LNA_MIX_CFG, MASK_2REG(DAIF_RF_ATTEN_GAIN_MO, 2, DAIF_RF_ATTEN_GAIN_ME, 1));
    REGW(&OM_DAIF->LNA_MIX_CFG, MASK_2REG(DAIF_RF_LNA_GAIN_MO, 5, DAIF_RF_LNA_GAIN_ME, 1));  // useful
    REGW(&OM_DAIF->TIA_LPF_CFG, MASK_2REG(DAIF_FILTER_GTUNE_MO, 0, DAIF_FILTER_GTUNE_ME, 1)); // useful //7=3.71mA
    #endif

    // FIX: prevent BLE rxen and mac_rx_win has gap
    // REGW(&OM_DAIF->MAIN_ST_CFG2, MASK_1REG(DAIF_RX_PLL_WAIT, 0x510));
}

/**
 *******************************************************************************
 * @brief  drv calib sys restore
 *******************************************************************************
 */
__RAM_CODES("PM")
void drv_calib_sys_restore(void)
{
}

#endif  /* RTE_CALIB*/

/** @} */
