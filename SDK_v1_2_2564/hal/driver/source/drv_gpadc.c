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
 * @brief    GPADC driver source file
 * @details  GPADC driver source file
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
#if (RTE_GPADC)
#include <stddef.h>
#include "om_device.h"
#include "om_driver.h"


/*******************************************************************************
 * MACROS
 */
#ifdef RTE_GPADC_CALIB_EN
#define GPADC_OUT_IDEAL_DIFF(ideal_2, ideal_1)      ((ideal_2) - (ideal_1))

#define GPADC_OUT_UPPER_BOUND(ideal)                ((ideal) * 13 / 10)
#define GPADC_OUT_LOW_BOUND(ideal)                  ((ideal) * 6 / 10)

#define GPADC_CALIB_COMMON_NUM                      (2)
#endif

#define GPADC_CALIB_VBG_NUM                         (2)

#define GPADC_CH_MAX                                (12)

#ifdef CONFIG_GPADC_LOG
#include "om_log.h"
#define OM_GPADC_LOG_DEBUG(format, ...)               om_log(OM_LOG_INFO, format,  ## __VA_ARGS__)
#else
#define OM_GPADC_LOG_DEBUG(format, ...)
#endif

/*******************************************************************************
 * TYPEDEFS
 */
#ifdef RTE_GPADC_CALIB_EN
typedef struct {
    uint16_t gainErr_set;
    uint16_t vos_set;
    uint16_t vosTemp_set;
    uint16_t gainErr_vbat_set;

    uint16_t vbg_code_trim_1;
    uint16_t vbg_code_trim_3;

    uint16_t gainErr_set_gain_1_3;
    uint16_t vos_set_gain_1_3;
    float    vosTemp_cal_gain_1_3;

    float    gainErr_cal;
    float    vos_cal;
    float    vosTemp_cal;
    float    gainErr_vbat_cal;

    float    gpadcOut_1;
    float    gpadcOut_2;
} drv_gpadc_calib_param_t;
#endif

typedef struct {
    bool      use_efuse;
    bool      use_flash_ex_5;
    bool      use_flash_ex_6;
    bool      use_flash_ex_7;
    bool      register_store;
    uint16_t  busy;
    uint16_t  rx_cnt;
    uint16_t  rx_num;
    int16_t  *rx_buf;
    float     calib_temper;
    uint32_t  gpadc_cfg0;
    uint32_t  gpadc_cfg1;
    uint32_t  gpadc_cfg2;
    uint32_t  cali_cfg;
    uint32_t  gpadc_seln;
    uint32_t  pd_cfg1;
    drv_isr_callback_t              isr_cb;
    drv_gpadc_calib_t               cp_calib_value;
    drv_gpadc_flash_calib_t         ft_calib_value;
    drv_gpadc_flash_calib_ex_1_t    ft_calib_ex_1_value;
    drv_gpadc_flash_calib_ex_2_t    ft_calib_ex_2_value;
    drv_gpadc_flash_calib_ex_3_t    ft_calib_ex_3_value;
    drv_gpadc_flash_calib_ex_4_t    ft_calib_ex_4_value;
    drv_gpadc_calib_t               cp_calib_ex_1_value;
    drv_gpadc_flash_calib_ex_5_t    ft_calib_ex_5_value;
    drv_gpadc_flash_calib_ex_6_t    ft_calib_ex_6_value;
    drv_gpadc_flash_calib_ex_7_t    ft_calib_ex_7_value;
    drv_gpadc_mode_t                mode;
    drv_gpadc_gain_t                gain;
    #if (RTE_GPDMA)
    uint8_t                         gpdma_chan;
    #endif
} drv_gpadc_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
#ifdef RTE_GPADC_CALIB_EN
static const float GPADC_SINGLE_OUT_1_IDEAL[GPADC_GAIN_MAX] = {
    248.24, 218.45, 655.36
};
static const float GPADC_SINGLE_OUT_2_IDEAL[GPADC_GAIN_MAX] = {
    3723.64, 3276.8, 3276.8
};
static const float GPADC_DIFF_OUT_1_IDEAL[GPADC_GAIN_MAX] = {
    0, 737.28, 409.6,
};
static const float GPADC_DIFF_OUT_2_IDEAL[GPADC_GAIN_MAX] = {
    0, 3358.72, 3686.4,
};
static drv_gpadc_calib_param_t gpadc_para;
#endif

static drv_gpadc_env_t gpadc_env = {
#if (RTE_GPDMA)
    .gpdma_chan     = GPDMA_NUMBER_OF_CHANNELS,
#endif
    .calib_temper = 25,
    .busy         = 0,
    .rx_cnt       = 0,
    .rx_num       = 0,
    .rx_buf       = NULL,
    .mode         = GPADC_MODE_SINGLE,
    .isr_cb       = NULL,
};



/*******************************************************************************
 * LOCAL FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief set GPADC calibration parameters to register
 *
 * @param[in] gain  gpadc gain
 *
 * @return none
 *******************************************************************************
 **/
static void drv_gpadc_set_calib2reg(drv_gpadc_gain_t gain)
{
    if (gpadc_env.use_efuse) {
        REGW(&OM_GPADC->CALI_CFG, MASK_1REG(GPADC_GAIN_ERR, gpadc_env.cp_calib_value.gain_error & 0x3FFF));
        REGW(&OM_GPADC->DCALI_CFG, MASK_1REG(GPADC_GAIN_ERR_DIFF, gpadc_env.cp_calib_ex_1_value.gain_error & 0x3FFF));
        if (gpadc_env.cp_calib_value.vos >> 14 & 0x1) {
            REGW(&OM_GPADC->CALI_CFG, MASK_1REG(GPADC_VOS, gpadc_env.cp_calib_value.vos | (0x1 << 15)));
        } else {
            REGW(&OM_GPADC->CALI_CFG, MASK_1REG(GPADC_VOS, gpadc_env.cp_calib_value.vos));
        }
        if (gpadc_env.cp_calib_ex_1_value.vos >> 14 & 0x1) {
            REGW(&OM_GPADC->DCALI_CFG, MASK_1REG(GPADC_VOS_DIFF, gpadc_env.cp_calib_ex_1_value.vos | (0x1 << 15)));
        } else {
            REGW(&OM_GPADC->DCALI_CFG, MASK_1REG(GPADC_VOS_DIFF, gpadc_env.cp_calib_ex_1_value.vos));
        }
        if (gpadc_env.cp_calib_value.vos_temp >> 9 & 0x1) {
            REGW(&OM_GPADC->TEMP_CFG, MASK_1REG(GPADC_VOS_TEMP, (gpadc_env.cp_calib_value.vos_temp << 2) | (0x3 << 12)));
        } else {
            REGW(&OM_GPADC->TEMP_CFG, MASK_1REG(GPADC_VOS_TEMP, gpadc_env.cp_calib_value.vos_temp << 2));
        }
    } else {
        REGW(&OM_GPADC->CALI_CFG, MASK_2REG(GPADC_GAIN_ERR, gpadc_env.ft_calib_value.data[gain].gain_error & 0x3FFF,
                                            GPADC_VOS, gpadc_env.ft_calib_value.data[gain].vos));
        REGW(&OM_GPADC->TEMP_CFG, MASK_1REG(GPADC_VOS_TEMP, gpadc_env.ft_calib_value.data[gain].vos_temp & 0x3FFF));

        REGW(&OM_GPADC->DCALI_CFG, MASK_2REG(GPADC_GAIN_ERR_DIFF, gpadc_env.ft_calib_ex_4_value.data_diff[gain-1].gain_error & 0x3FFF,
                                             GPADC_VOS_DIFF, gpadc_env.ft_calib_ex_4_value.data_diff[gain-1].vos));
    }
}

/**
 *******************************************************************************
 * @brief set GPADC temperature compensation
 *
 * @param[in] argu    Operation argument
 *
 * @return temperature
 *******************************************************************************
 **/
static int16_t drv_gpadc_temperature_compen(drv_gpadc_temperature_compen_t *argu)
{
    float ret;

    int16_t ret_vbat = argu->vbat_data;
    int16_t ret_temp = argu->temp_data;

    float step_temp = gpadc_env.ft_calib_ex_3_value.temperature_vbat_3p3v - gpadc_env.ft_calib_ex_3_value.temperature_vbat_2p3v;
    ret = (3300 - ret_vbat) * (step_temp / 10.0f) / 1000.0f + ret_temp / 10.0f;

    return (int16_t)(ret * 10);
}

/**
 *******************************************************************************
 * @brief get channel_p data
 *
 * @param[in] channel_p      gpadc channels
 * @param[in] data           Pointer where data to receive from gpadc.when reading the temperature,
 *                           for example,read 265 for 26.5 degrees Celsius.when the voltage is read,
 *                           give an example,2900 indicates 2900 millivolts.
 * @param[in] num            Number of data from gpadc channel_p
 *
 * @return raw data
 *******************************************************************************
 **/
static void drv_gpadc_get_channel_data(uint16_t channel_p, int16_t *data, uint16_t num)
{
    int i = 0;
    int ch_count = 0;
    uint8_t ch[GPADC_CH_MAX];
    for (uint8_t j = 0; j < GPADC_CH_MAX; j++) {
        if ((channel_p >> j) & 0x1) {
            ch[ch_count] = j;
            ch_count++;
        }
    }
    /* clear interrupt */
    OM_GPADC->INTR = GPADC_INTR_ALL_MASK;

    /* power-on GPADC */
    REGW(&OM_GPADC->ADC_CFG0, MASK_1REG(GPADC_PD, 0));

    if (ch_count == 1) {
        /* discard first data */
        while ((OM_GPADC->INTR & channel_p) != channel_p);
        OM_GPADC->INTR |= channel_p;

        while (i < num) {
            /* check interrupt from ch0 to ch9 and clear*/
            while ((OM_GPADC->INTR & channel_p) != channel_p);
            OM_GPADC->INTR |= channel_p;

            data[i%num] = REGR(((uint32_t *)&OM_GPADC->CH_0_DATA) + ch[0], MASK_POS(GPADC_DATA_LOW));
            i++;
        }
    } else {
        /* discard first date */
        for (uint8_t j = 0; j < ch_count; j++) {
            /* check interrupt from ch0 to ch9 and clear*/
            while ((OM_GPADC->INTR & (1 << ch[j])) != (1 << ch[j]));
            OM_GPADC->INTR |= 1 << ch[j];
        }

        /* get channel data */
        while (i < num) {
            for (uint8_t j = 0; j < ch_count; j++) {
                /* check interrupt from ch0 to ch9 and clear*/
                while ((OM_GPADC->INTR & (1 << ch[j])) != (1 << ch[j]));
                OM_GPADC->INTR |= 1 << ch[j];

                data[i%num] = REGR(((uint32_t *)&OM_GPADC->CH_0_DATA) + ch[j], MASK_POS(GPADC_DATA_LOW));
                i++;
            }
        }
    }

    /* power-down ADC */
    REGW(&OM_GPADC->ADC_CFG0, MASK_1REG(GPADC_PD, 1));
}

/**
 *******************************************************************************
 * @brief convert channel_p data
 *
 * @param[in] channel_p gpadc channels P
 * @param[in] ret_raw   gpadc raw data
 *
 * @return voltage/temperature
 *******************************************************************************
 **/
static int16_t drv_gpadc_convert_channel_data(drv_gpadc_channel_p_t channel_p, int16_t ret_raw)
{
    int16_t ret = 0;
    float k = 0;
    switch(channel_p) {
        case GPADC_CH_P_TEMPERATURE:
            ret = (int16_t)(ret_raw * 10 / 64.0f + 0.5f);
            break;
        case GPADC_CH_P_VBAT:
                k = gpadc_env.ft_calib_ex_1_value.gain_error_vbat / 32768.0f;
                ret = (int16_t)(ret_raw * 3 * 1000 * k / 2048.0f + 0.5f);
            break;
        default:
            if (GPADC_MODE_SINGLE == gpadc_env.mode) {
                if ((ret_raw >> 15) & 0x1) {
                    ret = 0;
                } else {
                    if (GPADC_GAIN_1_EXTERNAL_REF == gpadc_env.gain) {
                        /* fix digital bug : ret = ret / 1.25 * 3.3 */
                        ret = (int16_t)(ret_raw * 132000 / 102400.0f + 0.5f);
                    } else {
                        ret = (int16_t)(ret_raw * 1000 / 2048.0f + 0.5f);
                    }
                }
            } else {
                ret = (int16_t)(ret_raw * 1000 / 2048.0f);
            }
            break;
    }

    return ret;
}

/**
 *******************************************************************************
 * @brief gpadc register config
 *
 * @param[in] config    gpadc config
 *
 * @return none
 *******************************************************************************
 **/
static void drv_gpadc_config(const drv_gpadc_config_t* config)
{
    /* ADC_CFG0 config */
    REGW(&OM_GPADC->ADC_CFG0, MASK_10REG(GPADC_MODE_SEL, config->mode,
                                         GPADC_EN_SCALE, (GPADC_GAIN_1_3_INTERNAL_REF == config->gain)? 1 : 0,
                                         GPADC_SEL_VREF, (GPADC_GAIN_1_EXTERNAL_REF == config->gain)? 1 : 0,
                                         GPADC_PD_VBAT_DET, ((GPADC_CH_P_VBAT & config->channel_p) == GPADC_CH_P_VBAT)? 0 : 1,
                                         GPADC_PMU_TS_ICTRL, (GPADC_CH_P_TEMPERATURE == config->channel_p)? 3 : 0,
                                         GPADC_VCTRL_LDO, 1,
                                         GPADC_CTRL_VREF, 2,
                                         GPADC_INPUT_SELN_ME, (GPADC_MODE_DIFF == config->mode)? 1 : 0,
                                         GPADC_INPUT_SELN_MO, config->channel_n,
                                         GPADC_CMREG_EN, (GPADC_MODE_DIFF == config->mode)? 1 : 0));

    /* ADC_CFG1 config */
    REGW(&OM_GPADC->ADC_CFG1, MASK_4REG(GPADC_SMP_TIME, config->sampling_cycles,
                                        GPADC_AVG_BYPASS, 1,
                                        GPADC_SUM_NUM, config->sum_num,
                                        GPADC_VIN_OUTPUT_MODE, 1));

    /* ADC_CFG2 config */
    REGW(&OM_GPADC->ADC_CFG2, MASK_4REG(GPADC_SEQ_VECT, config->channel_p,
                                        GPADC_SCANDIR, 1,
                                        GPADC_SEQ_LIFE, 0,
                                        GPADC_SW_TRIGGER_TRUE, 1));

    /* ADC_SELN config */
    REGW(&OM_GPADC->ADC_SELN, MASK_3REG(GPADC_SEQ_VECT_N, 1 << config->channel_n,
                                        GPADC_SELN_SDIR, 1,
                                        GPADC_SEQ_LIFE_N, 0));

    /* set AUTO_COMPEN, default 0 */
    REGW(&OM_GPADC->CALI_CFG, MASK_1REG(GPADC_AUTO_COMPEN, 1));

    /* set the interval of interruption */
    REGW(&OM_GPADC->DLY_CFG, MASK_3REG(GPADC_CFG_CHG_DLY, 0x50,
                                       GPADC_PD_DLY_MAX, 0x50,
                                       GPADC_ORB_DLY, 0xE));

    /* set pd temp sensor */
    REGW(&OM_DAIF->PD_CFG1, MASK_1REG(DAIF_PMU_PD_TS, ((GPADC_CH_P_TEMPERATURE == config->channel_p) || ((1<<3u) == config->channel_p))? 0 : 1));
}

#if (RTE_GPDMA)
/**
 *******************************************************************************
 * @brief dma callback
 *
 * @return none
 *******************************************************************************
 **/
static void gpadc_dma_event_cb(void *resource, drv_event_t event, gpdma_chain_trans_t *next_chain)
{
    drv_event_t drv_event = DRV_EVENT_COMMON_NONE;

    switch (event) {
        case DRV_GPDMA_EVENT_TERMINAL_COUNT_REQUEST: {
            drv_event = DRV_EVENT_COMMON_READ_COMPLETED;

            /* disable GPADC DMA */
            REGW0(&OM_GPADC->ADC_CFG1, GPADC_DMA_EN_MASK);

            /* power-down GPADC */
            REGW(&OM_GPADC->ADC_CFG0, MASK_1REG(GPADC_PD, 1));

            int i = 0;
            while (i < gpadc_env.rx_num) {
                for (uint8_t j = 0; j < GPADC_CH_MAX; j++) {
                    if ((gpadc_env.busy >> j) & 0x1) {
                        gpadc_env.rx_buf[i%gpadc_env.rx_num] = drv_gpadc_convert_channel_data((drv_gpadc_channel_p_t)(0x1 << j), gpadc_env.rx_buf[i%gpadc_env.rx_num]);
                        i++;
                    }
                }
            }
        }   break;
        case DRV_GPDMA_EVENT_ABORT:
            drv_event = DRV_EVENT_COMMON_ABORT;
            break;
        default:
            drv_event = DRV_EVENT_COMMON_ERROR;
            break;
    }

    gpadc_env.busy = 0;

    if (gpadc_env.isr_cb) {
        gpadc_env.isr_cb(OM_GPADC, drv_event, (void *)(gpadc_env.rx_buf), (void *)((uint32_t)gpadc_env.rx_num));
    }
}
#endif

/**
 *******************************************************************************
 * @brief set new GPADC calibration parameters
 *
 * @return none
 *******************************************************************************
 **/
static void drv_gpadc_set_new_calibrate_param(void)
{
    uint16_t vbg_code_trim_1[GPADC_CALIB_VBG_NUM];
    uint16_t vbg_code_trim_3[GPADC_CALIB_VBG_NUM];
    uint32_t vbg_code_trim_1_new = 0;
    uint32_t vbg_code_trim_3_new = 0;
    uint32_t vbg_code_trim_1_old = 0;
    uint32_t vbg_code_trim_3_old = 0;
    uint16_t gain_error = 0;
    uint16_t gain_error_new = 0;
    float k = 0;
    float delta = 0;

    for (uint8_t mode = GPADC_MODE_SINGLE; mode <= GPADC_MODE_DIFF; mode++) {
        for (uint8_t gain = GPADC_GAIN_1_3_INTERNAL_REF; gain < GPADC_GAIN_MAX; gain++) {
            if (mode == GPADC_MODE_SINGLE) {
                gain_error = gpadc_env.ft_calib_value.data[gain].gain_error;
                if (gain == GPADC_GAIN_1_3_INTERNAL_REF) {
                    vbg_code_trim_1_old = gpadc_env.ft_calib_ex_2_value.vbg_code_trim_1_vbat_3p3v;
                    vbg_code_trim_3_old = gpadc_env.ft_calib_ex_2_value.vbg_code_trim_3_vbat_3p3v;
                } else {
                    if (gpadc_env.use_flash_ex_6 == true) {
                        vbg_code_trim_1_old = gpadc_env.ft_calib_ex_6_value.vbg_code_trim_1_vbat_3p3v_gain_1;
                        vbg_code_trim_3_old = gpadc_env.ft_calib_ex_6_value.vbg_code_trim_3_vbat_3p3v_gain_1;
                    } else {
                        break;
                    }
                }
            } else {
                gain_error = gpadc_env.ft_calib_ex_4_value.data_diff[gain-1].gain_error;
                if (gain == GPADC_GAIN_1_3_INTERNAL_REF) {
                    if (gpadc_env.use_flash_ex_5 == true) {
                        vbg_code_trim_1_old = gpadc_env.ft_calib_ex_5_value.vbg_code_trim_1_vbat_3p3v_diff;
                        vbg_code_trim_3_old = gpadc_env.ft_calib_ex_5_value.vbg_code_trim_3_vbat_3p3v_diff;
                    } else {
                        break;
                    }
                } else {
                    if (gpadc_env.use_flash_ex_7 == true) {
                        vbg_code_trim_1_old = gpadc_env.ft_calib_ex_7_value.vbg_code_trim_1_vbat_3p3v_gain_1_diff;
                        vbg_code_trim_3_old = gpadc_env.ft_calib_ex_7_value.vbg_code_trim_3_vbat_3p3v_gain_1_diff;
                    } else {
                        break;
                    }
                }
            }

            drv_gpadc_config_t config;
            config.channel_p = 1<<2U;
            config.channel_n = GPADC_CH_N_AVSS;
            config.mode = (drv_gpadc_mode_t)mode;
            config.gain = (drv_gpadc_gain_t)gain;
            config.sum_num = GPADC_SUM_NUM_256;
            config.sampling_cycles = GPADC_SAMPLING_CYCLES_128;

            drv_gpadc_control(GPADC_CONTROL_ENABLE_CLOCK, NULL);
            drv_gpadc_config(&config);
            REGW(&OM_GPADC->CALI_CFG, MASK_1REG(GPADC_AUTO_COMPEN, 0));

            uint8_t pmu_trim_store = REGR(&OM_GPADC->ADC_CFG0, MASK_POS(GPADC_PMU_TRIM_CTRL));
            REGW(&OM_GPADC->ADC_CFG0, MASK_1REG(GPADC_PMU_TRIM_CTRL, 1));
            drv_gpadc_get_channel_data(1<<2U, (int16_t *)(&vbg_code_trim_1[0]), GPADC_CALIB_VBG_NUM);
            REGW(&OM_GPADC->ADC_CFG0, MASK_1REG(GPADC_PMU_TRIM_CTRL, 3));
            drv_gpadc_get_channel_data(1<<2U, (int16_t *)(&vbg_code_trim_3[0]), GPADC_CALIB_VBG_NUM);

            for (int i = 0; i < GPADC_CALIB_VBG_NUM; i++) {
                vbg_code_trim_1_new += vbg_code_trim_1[i];
                vbg_code_trim_3_new += vbg_code_trim_3[i];
            }

            vbg_code_trim_1_new = (uint32_t)(vbg_code_trim_1_new / GPADC_CALIB_VBG_NUM);
            vbg_code_trim_3_new = (uint32_t)(vbg_code_trim_3_new / GPADC_CALIB_VBG_NUM);

            k = (float)(vbg_code_trim_3_new - vbg_code_trim_1_new) / (vbg_code_trim_3_old - vbg_code_trim_1_old);
            vbg_code_trim_1_new = 0;
            vbg_code_trim_3_new = 0;

            delta = 8192.0f / gain_error * (k - 1);

            if (delta > 0.002f || delta < -0.002f) {
                gain_error_new = (uint16_t)(gain_error / k);
            } else {
                gain_error_new = gain_error;
            }

            REGW(&OM_GPADC->ADC_CFG0, MASK_1REG(GPADC_PMU_TRIM_CTRL, pmu_trim_store));

            drv_gpadc_control(GPADC_CONTROL_DISABLE_CLOCK, NULL);

            if (mode == GPADC_MODE_SINGLE) {
                gpadc_env.ft_calib_value.data[gain].gain_error = gain_error_new;
            } else {
                gpadc_env.ft_calib_ex_4_value.data_diff[gain-1].gain_error = gain_error_new;
            }
        }
    }
}

/**
 *******************************************************************************
 * @brief set GPADC calibration parameters
 *
 * @param[in] config  calibration parameters
 *
 * @return none
 *******************************************************************************
 **/
static void drv_gpadc_set_calibrate_param(drv_gpadc_cpft_calib_t *config, bool use_power_on_param)
{
    if (config->flash != NULL || config->flash_ex_4 != NULL) {
        gpadc_env.use_efuse = false;
        if (config->flash != NULL) {
            memcpy(&gpadc_env.ft_calib_value, config->flash, sizeof(drv_gpadc_flash_calib_t));
            memcpy(&gpadc_env.ft_calib_ex_1_value, config->flash_ex_1, sizeof(drv_gpadc_flash_calib_ex_1_t));
            memcpy(&gpadc_env.ft_calib_ex_2_value, config->flash_ex_2, sizeof(drv_gpadc_flash_calib_ex_2_t));
            memcpy(&gpadc_env.ft_calib_ex_3_value, config->flash_ex_3, sizeof(drv_gpadc_flash_calib_ex_3_t));
        }
        if (config->flash_ex_4 != NULL) {
            memcpy(&gpadc_env.ft_calib_ex_4_value, config->flash_ex_4, sizeof(drv_gpadc_flash_calib_ex_4_t));
        }
        if (config->flash_ex_5 != NULL) {
            gpadc_env.use_flash_ex_5 = true;
            memcpy(&gpadc_env.ft_calib_ex_5_value, config->flash_ex_5, sizeof(drv_gpadc_flash_calib_ex_5_t));
        } else {
            gpadc_env.use_flash_ex_5 = false;
        }
        if (config->flash_ex_6 != NULL) {
            gpadc_env.use_flash_ex_6 = true;
            memcpy(&gpadc_env.ft_calib_ex_6_value, config->flash_ex_6, sizeof(drv_gpadc_flash_calib_ex_6_t));
        } else {
            gpadc_env.use_flash_ex_6 = false;
        }
        if (config->flash_ex_7 != NULL) {
            gpadc_env.use_flash_ex_7 = true;
            memcpy(&gpadc_env.ft_calib_ex_7_value, config->flash_ex_7, sizeof(drv_gpadc_flash_calib_ex_7_t));
        } else {
            gpadc_env.use_flash_ex_7 = false;
        }
        if (use_power_on_param) {
            drv_gpadc_set_new_calibrate_param();
        }
    } else if (config->efuse != NULL || config->efuse_ex_1 != NULL) {
        gpadc_env.use_efuse = true;
        if (config->efuse != NULL) {
            memcpy(&gpadc_env.cp_calib_value, config->efuse, sizeof(drv_gpadc_calib_t));
        }
        if (config->efuse_ex_1 != NULL) {
            memcpy(&gpadc_env.cp_calib_ex_1_value, config->efuse_ex_1, sizeof(drv_gpadc_calib_t));
        }
    }
}

#ifdef RTE_GPADC_CALIB_EN
/**
 *******************************************************************************
 * @brief calibrate common function
 *
 * @param[in] channel_p         gpadc channel P
 * @param[in] channel_n         gpadc channel N
 * @param[in] gain              gpadc gain
 * @param[in] compen            If set to 0, the gpadc will do nothing but transparent transmission
 * @param[in] mode              gpadc mode
 *
 * @return gpadc_out
 *******************************************************************************
 **/
static uint16_t drv_gpadc_calib_common(drv_gpadc_channel_p_t channel_p, drv_gpadc_channel_n_t channel_n, drv_gpadc_gain_t gain, bool compen, drv_gpadc_mode_t mode)
{
    uint16_t ret_raw[GPADC_CALIB_VBG_NUM];
    uint32_t sum = 0;

    drv_gpadc_config_t config;
    config.channel_p = channel_p;
    config.channel_n = channel_n;
    config.mode = mode;
    config.gain = gain;
    config.sum_num = GPADC_SUM_NUM_256;
    config.sampling_cycles = GPADC_SAMPLING_CYCLES_128;

    drv_gpadc_config(&config);

    REGW(&OM_GPADC->CALI_CFG, MASK_1REG(GPADC_AUTO_COMPEN, compen));

    if (1<<2 == channel_p) {
        drv_gpadc_get_channel_data(channel_p, (int16_t *)(&ret_raw[0]), GPADC_CALIB_VBG_NUM);
        for (int i = 0; i < GPADC_CALIB_VBG_NUM; i++) {
            sum += ret_raw[i];
        }

        return (uint16_t)(sum / GPADC_CALIB_VBG_NUM);
    } else {
        drv_gpadc_get_channel_data(channel_p, (int16_t *)(&ret_raw[0]), GPADC_CALIB_COMMON_NUM);
        for (int i = 0; i < GPADC_CALIB_COMMON_NUM; i++) {
            sum += ret_raw[i];
        }

        return (uint16_t)(sum / GPADC_CALIB_COMMON_NUM);
    }
}

/**
 *******************************************************************************
 * @brief calibrate vosTemp
 *
 * @param[in] gain              gpadc gain
 * @param[in] gainErr_set       gainErr
 * @param[in] vos_set           vos
 *
 * @return vosTemp
 *******************************************************************************
 **/
static float drv_gpadc_calib_vos_temp(drv_gpadc_gain_t gain, uint16_t gainErr_set, uint16_t vos_set)
{
    float vosTemp = 0;

    /* start gpadc */
    uint16_t ret_raw = drv_gpadc_calib_common(GPADC_CH_P_TEMPERATURE, GPADC_CH_N_AVSS, gain, 0, GPADC_MODE_SINGLE);

    float v_gain_error = 8192.0f / gainErr_set;
    float v_vos = (int16_t)vos_set / 16.0f;
    float v_gain = (GPADC_GAIN_1_3_INTERNAL_REF == gain) ? 3 : 1;

    float vt = ((ret_raw / 16.0f) - v_vos) * 1.25f / 4096.0f / v_gain_error * v_gain;

    OM_GPADC_LOG_DEBUG("code = %f, vt[%d]=%f\r\n", ret_raw/16.0f, gain, vt);

    vosTemp = (vt * 1000) / 2.959f - (gpadc_env.calib_temper + 273);

    return vosTemp;
}

/**
 *******************************************************************************
 * @brief calibrate gainErr_vbat
 *
 * @param[in] gain              gpadc gain
 * @param[in] gainErr_set       gainErr
 * @param[in] vos_set           vos
 *
 * @return gainErr_vbat
 *******************************************************************************
 **/
static float drv_gpadc_calib_vbat(drv_gpadc_gain_t gain, uint16_t gainErr_set, uint16_t vos_set)
{
    uint16_t vbat_raw;
    float gainError_vbat;

    /* config GPADC_GAIN_ERR and GPADC_VOS */
    REGW(&OM_GPADC->CALI_CFG, MASK_2REG(GPADC_GAIN_ERR, (gainErr_set & 0x3FFF), GPADC_VOS, vos_set));

    /* start gpadc */
    vbat_raw = drv_gpadc_calib_common(GPADC_CH_P_VBAT, GPADC_CH_N_AVSS, gain, 1, GPADC_MODE_SINGLE);

    /* calculate gain error vbat*/
    gainError_vbat = 1.1f / (vbat_raw / 2048.0f);

    return gainError_vbat;
}

/**
 *******************************************************************************
 * @brief calibrate power on param
 *
 * @param[in] gain              gpadc gain
 * @param[in] pmu_trim          gpadc pmu trim
 * @param[in] mode              gpadc mode
 *
 * @return vbg_code
 *******************************************************************************
 **/
static uint16_t drv_gpadc_calib_power_on_param(drv_gpadc_gain_t gain, uint8_t pmu_trim, drv_gpadc_mode_t mode)
{
    uint16_t vbg_raw;

    /* store PMU_TRIM_CTRL */
    uint8_t pmu_trim_store = REGR(&OM_GPADC->ADC_CFG0, MASK_POS(GPADC_PMU_TRIM_CTRL));

    /* config PMU_TRIM_CTRL */
    REGW(&OM_GPADC->ADC_CFG0, MASK_1REG(GPADC_PMU_TRIM_CTRL, pmu_trim));

    /* start gpadc */
    vbg_raw = drv_gpadc_calib_common(1<<2U, GPADC_CH_N_AVSS, gain, 0, mode);

    /* restore PMU_TRIM_CTRL */
    REGW(&OM_GPADC->ADC_CFG0, MASK_1REG(GPADC_PMU_TRIM_CTRL, pmu_trim_store));

    return vbg_raw;
}
#endif

/**
 *******************************************************************************
 * @brief store gpadc register
 *
 * @return none
 *******************************************************************************
 **/
static void drv_gpadc_register_store(void)
{
    /* store ADC_CFG0 config */
    gpadc_env.gpadc_cfg0 = OM_GPADC->ADC_CFG0;

    /* store ADC_CFG1 config */
    gpadc_env.gpadc_cfg1 = OM_GPADC->ADC_CFG1;

    /* store ADC_CFG2 config */
    gpadc_env.gpadc_cfg2 = OM_GPADC->ADC_CFG2;

    /* store AUTO_COMPEN config */
    gpadc_env.cali_cfg = OM_GPADC->CALI_CFG;

    /* store ADC_SELN config */
    gpadc_env.gpadc_seln = OM_GPADC->ADC_SELN;

    /* store PD_CFG1 config */
    gpadc_env.pd_cfg1 = OM_DAIF->PD_CFG1;

}

/**
 *******************************************************************************
 * @brief restore gpadc register
 *
 * @return none
 *******************************************************************************
 **/
static void drv_gpadc_register_restore(void)
{
    /* restore ADC_CFG0 config */
    OM_GPADC->ADC_CFG0 = gpadc_env.gpadc_cfg0;

    /* restore ADC_CFG1 config */
    OM_GPADC->ADC_CFG1 = gpadc_env.gpadc_cfg1;

    /* restore ADC_CFG2 config */
    OM_GPADC->ADC_CFG2 = gpadc_env.gpadc_cfg2;

    /* restore AUTO_COMPEN config */
    OM_GPADC->CALI_CFG = gpadc_env.cali_cfg;

    /* restore ADC_SELN config */
    OM_GPADC->ADC_SELN = gpadc_env.gpadc_seln;

    /* restore PD_CFG1 config */
    OM_DAIF->PD_CFG1 = gpadc_env.pd_cfg1;
}

/**
 *******************************************************************************
 * @brief gpadc temperature read
 *
 * @return temperature
 *******************************************************************************
 **/
static int drv_gpadc_temperature_read(void)
{
    int16_t ret = 0;

    if (0 == REGR(&OM_GPADC->ADC_CFG0, MASK_POS(GPADC_RST))) {
        gpadc_env.register_store = 1;
        drv_gpadc_register_store();
    }

    drv_gpadc_config_t config;
    config.channel_p = GPADC_CH_P_TEMPERATURE;
    config.channel_n = GPADC_CH_N_AVSS;
    config.mode = GPADC_MODE_SINGLE;
    config.gain = GPADC_GAIN_1_3_INTERNAL_REF;
    config.sum_num = GPADC_SUM_NUM_2;
    config.sampling_cycles = GPADC_SAMPLING_CYCLES_128;

    drv_gpadc_init(&config);

    drv_gpadc_read(GPADC_CH_P_TEMPERATURE, &ret, 1);

    if (1 == gpadc_env.register_store) {
        gpadc_env.register_store = 0;
        drv_gpadc_register_restore();
    } else {
        drv_gpadc_control(GPADC_CONTROL_DISABLE_CLOCK, NULL);
    }

    return (ret / 10);
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief GPADC interrupt service routine
 *
 * @return None
 *******************************************************************************
 **/
void drv_gpadc_isr(void)
{
    uint32_t intr_mask = OM_GPADC->INTR_MSK;
    OM_GPADC->INTR_MSK = 0;

    uint32_t src = OM_GPADC->INTR & GPADC_INTR_ALL_MASK;
    OM_GPADC->INTR = src;

    if (gpadc_env.rx_cnt < gpadc_env.rx_num) {
        for (uint8_t i = 0; i < GPADC_CH_MAX; i++) {
            if ((src & (1 << i)) && (gpadc_env.busy & (1 << i))) {
                gpadc_env.rx_buf[gpadc_env.rx_cnt] = REGR(((uint32_t *)&OM_GPADC->CH_0_DATA) + i, MASK_POS(GPADC_DATA_LOW));
                gpadc_env.rx_cnt++;

                if (gpadc_env.rx_cnt == gpadc_env.rx_num) {
                    OM_GPADC->INTR_MSK &= ~gpadc_env.busy;
                    NVIC_DisableIRQ(GPADC_IRQn);

                    /* power-down GPADC */
                    REGW(&OM_GPADC->ADC_CFG0, MASK_1REG(GPADC_PD, 1));

                    int k = 0;
                    while (k < gpadc_env.rx_num) {
                        for (uint8_t j = 0; j < GPADC_CH_MAX; j++) {
                            if ((gpadc_env.busy >> j) & 0x1) {
                                gpadc_env.rx_buf[k%gpadc_env.rx_num] = drv_gpadc_convert_channel_data((drv_gpadc_channel_p_t)(0x1 << j), gpadc_env.rx_buf[k%gpadc_env.rx_num]);
                                k++;
                            }
                        }
                    }

                    gpadc_env.busy = 0;
                    if (gpadc_env.isr_cb) {
                        gpadc_env.isr_cb(OM_GPADC, DRV_EVENT_COMMON_READ_COMPLETED, (void *)gpadc_env.rx_buf, (void *)((uint32_t)gpadc_env.rx_num));
                    }
                } else {
                    OM_GPADC->INTR_MSK = intr_mask;
                }
            }
    }
}


}

/**
 *******************************************************************************
 * @brief GPADC initialization
 *
 * @param[in] config         Configuration for GPADC
 *
 * @return status:
 *    - OM_ERROR_OK:         init done
 *    - others:              No
 *******************************************************************************
 */
om_error_t drv_gpadc_init(const drv_gpadc_config_t *config)
{
    if (((GPADC_CH_P_TEMPERATURE & config->channel_p) && (GPADC_CH_P_VBAT & config->channel_p)) ||
        ((GPADC_CH_P_TEMPERATURE & config->channel_p) && (config->channel_p > GPADC_CH_P_GPIO2))) {
        return OM_ERROR_PARAMETER;
    }

    if (GPADC_MODE_DIFF == config->mode && (GPADC_CH_P_TEMPERATURE == config->channel_p || GPADC_CH_P_VBAT == config->channel_p)) {
        return OM_ERROR_PARAMETER;
    }

    drv_gpadc_control(GPADC_CONTROL_ENABLE_CLOCK, NULL);

    drv_gpadc_config(config);

    drv_gpadc_set_calib2reg(config->gain);

    gpadc_env.mode = config->mode;
    gpadc_env.gain = config->gain;
    gpadc_env.busy = 0;

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Register isr callback for transmit/receive by interrupt & gpdma mode
 *
 * @param[in] cb       Pointer to callback
 *
 * @return None
 *******************************************************************************
 */
void drv_gpadc_register_isr_callback(drv_isr_callback_t cb)
{
    gpadc_env.isr_cb = cb;
}

/**
 *******************************************************************************
 * @brief Prepare receive number of data by block mode
 *
 * @param[in] channel_p      gpadc channels P
 * @param[in] data           Pointer where data to receive from gpadc.when reading the temperature,
 *                           for example,read 265 for 26.5 degrees Celsius.when the voltage is read,
 *                           give an example,2900 indicates 2900 millivolts.
 * @param[in] num            Number of data from gpadc channel_p
 *
 * @return status:
 *    - OM_ERROR_OK:         receive done
 *    - others:              No
 *******************************************************************************
 */
om_error_t drv_gpadc_read(uint16_t channel_p, int16_t *data, uint16_t num)
{
    if (gpadc_env.busy) {
        return OM_ERROR_BUSY;
    }

    gpadc_env.busy = channel_p;

    drv_gpadc_get_channel_data(channel_p, data, num);

    if (REGR(&OM_GPADC->CALI_CFG, MASK_POS(GPADC_AUTO_COMPEN)) == 1) {
        int i = 0;
        while (i < num) {
            for (uint8_t j = 0; j < GPADC_CH_MAX; j++) {
                if ((channel_p >> j) & 0x1) {
                    data[i%num] = drv_gpadc_convert_channel_data((drv_gpadc_channel_p_t)(0x1 << j), data[i%num]);
                    i++;
                }
            }
        }
    }

    gpadc_env.busy = 0;

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Prepare receive number of data by interrupt mode,
 *
 * @param[in] channel_p      gpadc channels P
 * @param[in] data           Pointer where data to receive from gpadc
 * @param[in] num            Number of data from gpadc receiver
 *
 * @return status:
 *    - OM_ERROR_OK:         Begin to receive
 *    - others:              No
 *******************************************************************************
 */
om_error_t drv_gpadc_read_int(uint16_t channel_p, int16_t *data, uint16_t num)
{
    if (gpadc_env.busy) {
        return OM_ERROR_BUSY;
    }

    /* clear interrupt */
    OM_GPADC->INTR = GPADC_INTR_ALL_MASK;

    /* set INTR_MSK */
    OM_GPADC->INTR_MSK = channel_p;

    gpadc_env.busy   = channel_p;
    gpadc_env.rx_cnt = 0;
    gpadc_env.rx_buf = data;
    gpadc_env.rx_num = num;

    NVIC_ClearPendingIRQ(GPADC_IRQn);
    NVIC_SetPriority(GPADC_IRQn, RTE_GPADC_IRQ_PRIORITY);
    NVIC_EnableIRQ(GPADC_IRQn);

    /* power-on GPADC */
    REGW(&OM_GPADC->ADC_CFG0, MASK_1REG(GPADC_PD, 0));

    return OM_ERROR_OK;
}

#if (RTE_GPDMA)
/**
 *******************************************************************************
 * @brief Allocate gpdma channel for gpadc
 *
 * @return status:
 *    - OM_ERROR_OK:         Allocate ok
 *    - others:              No
 *******************************************************************************
 */
om_error_t drv_gpadc_gpdma_channel_allocate(void)
{
    if (gpadc_env.gpdma_chan >= GPDMA_NUMBER_OF_CHANNELS) {
        gpadc_env.gpdma_chan = drv_gpdma_channel_allocate();
        if (gpadc_env.gpdma_chan >= GPDMA_NUMBER_OF_CHANNELS) {
            return OM_ERROR_RESOURCES;
        }
    }

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Release gpdma channel for gpadc
 *
 * @return status:
 *    - OM_ERROR_OK:         Release ok
 *    - others:              No
 *******************************************************************************
 */
om_error_t drv_gpadc_gpdma_channel_release(void)
{
    drv_gpdma_channel_release(gpadc_env.gpdma_chan);
    gpadc_env.gpdma_chan = GPDMA_NUMBER_OF_CHANNELS;

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Prepare receive number of data by GPDMA mode
 *
 * @param[in] channel_p      gpadc channels P
 * @param[in] data           Pointer where data to receive from gpadc
 * @param[in] num            Number of data from gpadc receiver
 *
 * @return status:
 *    - OM_ERROR_OK:         Begin to receive
 *    - others:              No
 *******************************************************************************
 */
om_error_t drv_gpadc_read_dma(uint16_t channel_p, int16_t *data, uint16_t num)
{
    if (gpadc_env.busy) {
        return OM_ERROR_BUSY;
    }

    gpdma_config_t    dma_config;
    om_error_t      error;

    gpadc_env.busy = channel_p;
    gpadc_env.rx_cnt = 0;
    gpadc_env.rx_buf = data;
    gpadc_env.rx_num = num;

    /* GPADC DMA config */
    dma_config.channel_ctrl  = GPDMA_SET_CTRL(GPDMA_ADDR_CTRL_FIXED, GPDMA_ADDR_CTRL_INC,
                                              GPDMA_TRANS_WIDTH_2B,  GPDMA_TRANS_WIDTH_2B,
                                              GPDMA_BURST_SIZE_1T,   GPDMA_PRIORITY_LOW);
    dma_config.src_id        = GPDMA_ID_GPADC;
    dma_config.dst_id        = GPDMA_ID_MEM;
    dma_config.isr_cb        = gpadc_dma_event_cb;
    dma_config.cb_param      = NULL;
    dma_config.chain_trans   = NULL;
    dma_config.chain_trans_num = 0;

    error = drv_gpdma_channel_config(gpadc_env.gpdma_chan, &dma_config);
    if (error) {
        return error;
    }

    /* Enable GPADC DMA */
    REGW1(&OM_GPADC->ADC_CFG1, GPADC_DMA_EN_MASK);

    /* power-on GPADC */
    REGW(&OM_GPADC->ADC_CFG0, MASK_1REG(GPADC_PD, 0));

    error = drv_gpdma_channel_enable(gpadc_env.gpdma_chan, (uint32_t)data, (uint32_t)&OM_GPADC->CH_DMA_DATA, 2 * num);
    if (error) {
        return error;
    }

    return OM_ERROR_OK;
}
#endif  /* (RTE_GPDMA) */

/**
 *******************************************************************************
 * @brief Control GPADC interface.
 *
 * @param[in] control        Operation
 * @param[in] argu           Operation argument
 *
 * @return status:           Control status
 *******************************************************************************
 */
void *drv_gpadc_control(drv_gpadc_control_t control, void *argu)
{
    uint32_t ret = (uint32_t)OM_ERROR_OK;
    OM_CRITICAL_BEGIN();
    switch (control) {
        case GPADC_CONTROL_ENABLE_CLOCK:
            drv_pmu_ana_enable(true, PMU_ANA_GPADC);
            REGW(&OM_GPADC->ADC_CFG0, MASK_1REG(GPADC_RST, 0));
            break;
        case GPADC_CONTROL_DISABLE_CLOCK:
            REGW(&OM_GPADC->ADC_CFG0, MASK_1REG(GPADC_RST, 1));
            drv_pmu_ana_enable(false, PMU_ANA_GPADC);
            break;
        case GPADC_CONTROL_SET_PARAM:
            if (argu == NULL) {
                ret = (uint32_t)OM_ERROR_PARAMETER;
                break;
            }
            drv_gpadc_set_calibrate_param((drv_gpadc_cpft_calib_t *)argu, true);
            break;
        case GPADC_CONTROL_IS_BUSY:
            if (gpadc_env.busy)
                ret = true;
            else
                ret = false;
            break;
        case GPADC_CONTROL_READ_TEMPERATURE:
            ret = drv_gpadc_temperature_read();
            break;
        case GPADC_CONTROL_SET_CALIB_TEMPER:
            gpadc_env.calib_temper = *(float *)argu;
            break;
        case GPADC_CONTROL_TEMPERATURE_COMPEN:
            ret = drv_gpadc_temperature_compen((drv_gpadc_temperature_compen_t *)argu);
            break;
        case GPADC_CONTROL_SET_PARAM_WITHOUT_POWER_ON_PARAM:
            if (argu == NULL) {
                ret = (uint32_t)OM_ERROR_PARAMETER;
                break;
            }
            drv_gpadc_set_calibrate_param((drv_gpadc_cpft_calib_t *)argu, false);
            break;
        case GPADC_CONTROL_CHANNEL_CFG:
            if ((uint32_t)argu == GPADC_CH_P_TEMPERATURE) {
                REGW(&OM_DAIF->PD_CFG1, MASK_1REG(DAIF_PMU_PD_TS, 0));
                REGW(&OM_GPADC->ADC_CFG0, MASK_1REG(GPADC_PMU_TS_ICTRL, 3));
            } else if ((uint32_t)argu == GPADC_CH_P_VBAT) {
                REGW(&OM_GPADC->ADC_CFG0, MASK_1REG(GPADC_PD_VBAT_DET, 0));
            }
            REGW(&OM_GPADC->ADC_CFG2, MASK_1REG(GPADC_SEQ_VECT, (uint32_t)argu));
            break;
        case GPADC_CONTROL_GET_GAIN_ERROR:
            ret = REGR(&OM_GPADC->CALI_CFG, MASK_POS(GPADC_GAIN_ERR));
            break;
        case GPADC_CONTROL_GET_VOS:
            ret = REGR(&OM_GPADC->CALI_CFG, MASK_POS(GPADC_VOS));
            break;
        case GPADC_CONTROL_SET_AUTO_COMPEN:
            REGW(&OM_GPADC->CALI_CFG, MASK_1REG(GPADC_AUTO_COMPEN, (uint32_t)argu));
            break;
        default:
            break;
    }
    OM_CRITICAL_END();
    return (void *)ret;
}

#ifdef RTE_GPADC_CALIB_EN
/**
 *******************************************************************************
 * @brief Calibrate GPADC and initialize GPADC compensation parameters.
 *
 * @param[in] gain                  gpadc gain
 * @param[in] calib_sel             gpadc select calibration phase
 * @param[out] pvalue               Pointer to @p drv_gpadc_calib_t object where the
 *                                  calibration parameters are to be returned.
 * @param[out] pvalue_vbat          Pointer to @p drv_gpadc_flash_calib_ex_1_t object where the
 *                                  calibration extra parameters are to be returned.
 * @param[out] pvalue_power_on_3p3v Pointer to @p drv_gpadc_flash_calib_ex_2_t object where the
 *                                  calibration extra parameters are to be returned.
 * @param[out] pvalue_power_on_2p3v Pointer to @p drv_gpadc_flash_calib_ex_3_t object where the
 *                                  calibration extra parameters are to be returned.
 *
 * @return status:
 *    - OM_ERROR_OK:                Calibrate done
 *    - others:                     No
 *******************************************************************************
 */
om_error_t drv_gpadc_calibrate(drv_gpadc_calib_t *pvalue, drv_gpadc_flash_calib_ex_1_t *pvalue_vbat,
                               drv_gpadc_flash_calib_ex_2_t *pvalue_power_on_3p3v, drv_gpadc_flash_calib_ex_3_t *pvalue_power_on_2p3v,
                               drv_gpadc_gain_t gain, drv_gpadc_calib_sel_t calib_sel)
{
    if (gpadc_env.busy) {
        return OM_ERROR_BUSY;
    }

    om_error_t error = OM_ERROR_OK;
    uint16_t ret_raw = 0;

    drv_gpadc_control(GPADC_CONTROL_ENABLE_CLOCK, NULL);
    gpadc_env.busy = GPADC_CH_P_GPIO2 | GPADC_CH_P_GPIO3;

    if (GPADC_CALIB_FT_SINGLE == calib_sel) {
        do {
            if (pvalue != NULL) {
                /* get gpadc_out_real_1[15:0] */
                ret_raw = drv_gpadc_calib_common(GPADC_CH_P_GPIO2, GPADC_CH_N_AVSS, gain, 0, GPADC_MODE_SINGLE);
                gpadc_para.gpadcOut_1 = ret_raw / 16.0f;
                OM_GPADC_LOG_DEBUG("gpadc_out_real_1[%d]=%f\r\n", gain, gpadc_para.gpadcOut_1);
                if ((gpadc_para.gpadcOut_1 > GPADC_OUT_UPPER_BOUND(GPADC_SINGLE_OUT_1_IDEAL[gain])) ||
                    (gpadc_para.gpadcOut_1 < GPADC_OUT_LOW_BOUND(GPADC_SINGLE_OUT_1_IDEAL[gain]))) {
                    error = OM_ERROR_OUT_OF_RANGE;
                    break;
                }

                /* get gpadc_out_real_2[15:0] */
                ret_raw = drv_gpadc_calib_common(GPADC_CH_P_GPIO3, GPADC_CH_N_AVSS, gain, 0, GPADC_MODE_SINGLE);
                gpadc_para.gpadcOut_2 = ret_raw / 16.0f;
                OM_GPADC_LOG_DEBUG("gpadc_out_real_2[%d]=%f\r\n", gain, gpadc_para.gpadcOut_2);
                if ((gpadc_para.gpadcOut_2 > GPADC_OUT_UPPER_BOUND(GPADC_SINGLE_OUT_2_IDEAL[gain])) ||
                    (gpadc_para.gpadcOut_2 < GPADC_OUT_LOW_BOUND(GPADC_SINGLE_OUT_2_IDEAL[gain]))) {
                    error = OM_ERROR_OUT_OF_RANGE;
                    break;
                }

                /* calib gain err[13:0] */
                gpadc_para.gainErr_cal = (gpadc_para.gpadcOut_2 - gpadc_para.gpadcOut_1) /
                                            GPADC_OUT_IDEAL_DIFF(GPADC_SINGLE_OUT_2_IDEAL[gain], GPADC_SINGLE_OUT_1_IDEAL[gain]);
                gpadc_para.gainErr_set = (uint16_t)(8192.0f / gpadc_para.gainErr_cal);
                OM_GPADC_LOG_DEBUG("gainErr_cal[%d] = %f, gainErr_set[%d] = %x\r\n", gain, gpadc_para.gainErr_cal, gain, gpadc_para.gainErr_set);

                /* calib vos[15:0] */
                gpadc_para.vos_cal = gpadc_para.gpadcOut_2 - gpadc_para.gainErr_cal * GPADC_SINGLE_OUT_2_IDEAL[gain];
                if (gpadc_para.vos_cal < 0) {
                    gpadc_para.vos_set = ~(uint16_t)(0 - (gpadc_para.vos_cal * 16)) + 1;
                } else {
                    gpadc_para.vos_set = (uint16_t)(gpadc_para.vos_cal * 16);
                }
                OM_GPADC_LOG_DEBUG("vos_cal[%d] = %f, vos_set[%d]: %x\r\n", gain, gpadc_para.vos_cal, gain, gpadc_para.vos_set);

                /* calib vos_temp[13:0] */
                gpadc_para.vosTemp_cal = drv_gpadc_calib_vos_temp(gain, gpadc_para.gainErr_set, gpadc_para.vos_set);
                if (gpadc_para.vosTemp_cal < 0) {
                    gpadc_para.vosTemp_set = ~(uint16_t)(0 - gpadc_para.vosTemp_cal * 64) + 1;
                } else {
                    gpadc_para.vosTemp_set = (uint16_t)(gpadc_para.vosTemp_cal * 64);
                }
                OM_GPADC_LOG_DEBUG("vosTemp_cal[%d] = %f, vosTemp_set[%d] = %x\r\n", gain, gpadc_para.vosTemp_cal, gain, gpadc_para.vosTemp_set);

                pvalue->gain_error = gpadc_para.gainErr_set;
                pvalue->vos = gpadc_para.vos_set;
                pvalue->vos_temp = gpadc_para.vosTemp_set;

                if (gain == GPADC_GAIN_1_3_INTERNAL_REF) {
                    gpadc_para.vosTemp_cal_gain_1_3 = gpadc_para.vosTemp_cal;
                    gpadc_para.gainErr_set_gain_1_3 = gpadc_para.gainErr_set;
                    gpadc_para.vos_set_gain_1_3 = gpadc_para.vos_set;
                }
            }

            /* calib vbat */
            if (pvalue_vbat != NULL) {
                gpadc_para.gainErr_vbat_cal = drv_gpadc_calib_vbat(gain, gpadc_para.gainErr_set, gpadc_para.vos_set);
                if (gpadc_para.gainErr_vbat_cal < 0) {
                    gpadc_para.gainErr_vbat_set = ~(uint16_t)(0 - gpadc_para.gainErr_vbat_cal * 32768) + 1;
                } else {
                    gpadc_para.gainErr_vbat_set = (uint16_t)(gpadc_para.gainErr_vbat_cal * 32768);
                }
                OM_GPADC_LOG_DEBUG("gainErr_vbat_cal[%d] = %f, gainErr_vbat_set[%d] = %x\r\n", gain, gpadc_para.gainErr_vbat_cal, gain, gpadc_para.gainErr_vbat_set);

                pvalue_vbat->gain_error_vbat = gpadc_para.gainErr_vbat_set;
            }

            /* calib power on */
            if (pvalue_power_on_3p3v != NULL) {
                gpadc_para.vbg_code_trim_1 = drv_gpadc_calib_power_on_param(gain, 1, GPADC_MODE_SINGLE);
                gpadc_para.vbg_code_trim_3 = drv_gpadc_calib_power_on_param(gain, 3, GPADC_MODE_SINGLE);
                OM_GPADC_LOG_DEBUG("vbg_code_trim_1_vbat_3p3v = %d, vbg_code_trim_3_vbat_3p3v = %d\r\n", gpadc_para.vbg_code_trim_1, gpadc_para.vbg_code_trim_3);

                pvalue_power_on_3p3v->vbg_code_trim_1_vbat_3p3v = gpadc_para.vbg_code_trim_1;
                pvalue_power_on_3p3v->vbg_code_trim_3_vbat_3p3v = gpadc_para.vbg_code_trim_3;
            }

            if (pvalue_power_on_2p3v != NULL) {
                gpadc_para.vbg_code_trim_1 = drv_gpadc_calib_power_on_param(gain, 1, GPADC_MODE_SINGLE);
                gpadc_para.vbg_code_trim_3 = drv_gpadc_calib_power_on_param(gain, 3, GPADC_MODE_SINGLE);
                OM_GPADC_LOG_DEBUG("vbg_code_trim_1_vbat_2p3v = %d, vbg_code_trim_3_vbat_2p3v = %d\r\n", gpadc_para.vbg_code_trim_1, gpadc_para.vbg_code_trim_3);

                pvalue_power_on_2p3v->vbg_code_trim_1_vbat_2p3v = gpadc_para.vbg_code_trim_1;
                pvalue_power_on_2p3v->vbg_code_trim_3_vbat_2p3v = gpadc_para.vbg_code_trim_3;

                pvalue_power_on_2p3v->temperature_vbat_3p3v = (int16_t)((gpadc_para.vosTemp_cal_gain_1_3 + gpadc_env.calib_temper) * 10);
                pvalue_power_on_2p3v->temperature_vbat_2p3v = (int16_t)((drv_gpadc_calib_vos_temp(gain, gpadc_para.gainErr_set_gain_1_3, gpadc_para.vos_set_gain_1_3) + gpadc_env.calib_temper) * 10);
            }
        } while(0);
    } else if (GPADC_CALIB_FT_DIFF_1 == calib_sel) {
        do {
            /* get gpadc_out_real_1[15:0] */
            ret_raw = drv_gpadc_calib_common(GPADC_CH_P_GPIO2, GPADC_CH_N_GPIO3, gain, 0, GPADC_MODE_DIFF);
            gpadc_para.gpadcOut_1 = ret_raw / 16.0f;
            OM_GPADC_LOG_DEBUG("gpadc_out_real_1[%d]=%f\r\n", gain, gpadc_para.gpadcOut_1);
            if ((gpadc_para.gpadcOut_1 > GPADC_OUT_UPPER_BOUND(GPADC_DIFF_OUT_1_IDEAL[gain])) ||
                (gpadc_para.gpadcOut_1 < GPADC_OUT_LOW_BOUND(GPADC_DIFF_OUT_1_IDEAL[gain]))) {
                error = OM_ERROR_OUT_OF_RANGE;
                break;
            }
        } while(0);
    } else if (GPADC_CALIB_FT_DIFF_2 == calib_sel) {
        do {
            /* get gpadc_out_real_2[15:0] */
            ret_raw = drv_gpadc_calib_common(GPADC_CH_P_GPIO2, GPADC_CH_N_GPIO3, gain, 0, GPADC_MODE_DIFF);
            gpadc_para.gpadcOut_2 = ret_raw / 16.0f;
            OM_GPADC_LOG_DEBUG("gpadc_out_real_2[%d]=%f\r\n", gain, gpadc_para.gpadcOut_2);
            if ((gpadc_para.gpadcOut_2 > GPADC_OUT_UPPER_BOUND(GPADC_DIFF_OUT_2_IDEAL[gain])) ||
                (gpadc_para.gpadcOut_2 < GPADC_OUT_LOW_BOUND(GPADC_DIFF_OUT_2_IDEAL[gain]))) {
                error = OM_ERROR_OUT_OF_RANGE;
                break;
            }

            /* calib gain err diff[13:0] */
            gpadc_para.gainErr_cal = (gpadc_para.gpadcOut_2 - gpadc_para.gpadcOut_1) /
                                        GPADC_OUT_IDEAL_DIFF(GPADC_DIFF_OUT_2_IDEAL[gain], GPADC_DIFF_OUT_1_IDEAL[gain]);
            gpadc_para.gainErr_set = (uint16_t)(8192.0f / gpadc_para.gainErr_cal);
            OM_GPADC_LOG_DEBUG("gainErr_diff_cal[%d] = %f, gainErr_diff_set[%d] = %x\r\n", gain, gpadc_para.gainErr_cal, gain, gpadc_para.gainErr_set);

            /* calib vos diff[15:0] */
            gpadc_para.vos_cal = (2 * gpadc_para.gpadcOut_2 - 4096) - gpadc_para.gainErr_cal * (2 * GPADC_DIFF_OUT_2_IDEAL[gain] - 4096);
            if (gpadc_para.vos_cal < 0) {
                gpadc_para.vos_set = ~(uint16_t)(0 - (gpadc_para.vos_cal * 16)) + 1;
            } else {
                gpadc_para.vos_set = (uint16_t)(gpadc_para.vos_cal * 16);
            }
            OM_GPADC_LOG_DEBUG("vos_diff_cal[%d] = %f, vos_diff_set[%d]: %x\r\n", gain, gpadc_para.vos_cal, gain, gpadc_para.vos_set);

            if (pvalue != NULL) {
                pvalue->gain_error = gpadc_para.gainErr_set;
                pvalue->vos = gpadc_para.vos_set;
            }

            /* calib power on diff */
            if (pvalue_power_on_3p3v != NULL) {
                gpadc_para.vbg_code_trim_1 = drv_gpadc_calib_power_on_param(gain, 1, GPADC_MODE_DIFF);
                gpadc_para.vbg_code_trim_3 = drv_gpadc_calib_power_on_param(gain, 3, GPADC_MODE_DIFF);
                OM_GPADC_LOG_DEBUG("vbg_code_trim_1_vbat_3p3v = %d, vbg_code_trim_3_vbat_3p3v = %d\r\n", gpadc_para.vbg_code_trim_1, gpadc_para.vbg_code_trim_3);

                pvalue_power_on_3p3v->vbg_code_trim_1_vbat_3p3v = gpadc_para.vbg_code_trim_1;
                pvalue_power_on_3p3v->vbg_code_trim_3_vbat_3p3v = gpadc_para.vbg_code_trim_3;
            }
        } while(0);
    } else if (GPADC_CALIB_CP_1 == calib_sel) {
        do {
            /* get gpadc_out_real_1[15:0] */
            ret_raw = drv_gpadc_calib_common(GPADC_CH_P_GPIO3, GPADC_CH_N_AVSS, gain, 0, GPADC_MODE_SINGLE);
            gpadc_para.gpadcOut_1 = ret_raw / 16.0f;
            OM_GPADC_LOG_DEBUG("gpadc_out_real_1[%d]=%f\r\n", gain, gpadc_para.gpadcOut_1);
            if ((gpadc_para.gpadcOut_1 > GPADC_OUT_UPPER_BOUND(GPADC_SINGLE_OUT_1_IDEAL[gain])) ||
                (gpadc_para.gpadcOut_1 < GPADC_OUT_LOW_BOUND(GPADC_SINGLE_OUT_1_IDEAL[gain]))) {
                error = OM_ERROR_OUT_OF_RANGE;
                break;
            }
        } while(0);
    } else if (GPADC_CALIB_CP_2 == calib_sel) {
        do {
            /* get gpadc_out_real_2[15:0] */
            ret_raw = drv_gpadc_calib_common(GPADC_CH_P_GPIO3, GPADC_CH_N_AVSS, gain, 0, GPADC_MODE_SINGLE);
            gpadc_para.gpadcOut_2 = ret_raw / 16.0f;
            OM_GPADC_LOG_DEBUG("gpadc_out_real_2[%d]=%f\r\n", gain, gpadc_para.gpadcOut_2);
            if ((gpadc_para.gpadcOut_2 > GPADC_OUT_UPPER_BOUND(GPADC_SINGLE_OUT_2_IDEAL[gain])) ||
                (gpadc_para.gpadcOut_2 < GPADC_OUT_LOW_BOUND(GPADC_SINGLE_OUT_2_IDEAL[gain]))) {
                error = OM_ERROR_OUT_OF_RANGE;
                break;
            }

            /* calib gain err[13:0] */
            gpadc_para.gainErr_cal = (gpadc_para.gpadcOut_2 - gpadc_para.gpadcOut_1) /
                                        GPADC_OUT_IDEAL_DIFF(GPADC_SINGLE_OUT_2_IDEAL[gain], GPADC_SINGLE_OUT_1_IDEAL[gain]);
            gpadc_para.gainErr_set = (uint16_t)(8192.0f / gpadc_para.gainErr_cal);
            OM_GPADC_LOG_DEBUG("gainErr_cal[%d] = %f, gainErr_set[%d] = %x\r\n", gain, gpadc_para.gainErr_cal, gain, gpadc_para.gainErr_set);

            /* calib vos[15:0] */
            gpadc_para.vos_cal = gpadc_para.gpadcOut_2 - gpadc_para.gainErr_cal * GPADC_SINGLE_OUT_2_IDEAL[gain];
            if (gpadc_para.vos_cal < 0) {
                gpadc_para.vos_set = ~(uint16_t)(0 - (gpadc_para.vos_cal * 16)) + 1;
            } else {
                gpadc_para.vos_set = (uint16_t)(gpadc_para.vos_cal * 16);
            }
            OM_GPADC_LOG_DEBUG("vos_cal[%d] = %f, vos_set[%d]: %x\r\n", gain, gpadc_para.vos_cal, gain, gpadc_para.vos_set);

            /* calib vos_temp[13:0] */
            gpadc_para.vosTemp_cal = drv_gpadc_calib_vos_temp(gain, gpadc_para.gainErr_set, gpadc_para.vos_set);
            if (gpadc_para.vosTemp_cal < 0) {
                gpadc_para.vosTemp_set = ~(uint16_t)(0 - gpadc_para.vosTemp_cal * 64) + 1;
            } else {
                gpadc_para.vosTemp_set = (uint16_t)(gpadc_para.vosTemp_cal * 64);
            }
            OM_GPADC_LOG_DEBUG("vosTemp_cal[%d] = %f, vosTemp_set[%d] = %x\r\n", gain, gpadc_para.vosTemp_cal, gain, gpadc_para.vosTemp_set);

            if (pvalue != NULL) {
                pvalue->gain_error = gpadc_para.gainErr_set;
                pvalue->vos = gpadc_para.vos_set;
                pvalue->vos_temp = gpadc_para.vosTemp_set;
            }

        } while(0);
    } else {
        error = OM_ERROR_PARAMETER;
    }

    drv_gpadc_control(GPADC_CONTROL_DISABLE_CLOCK, NULL);
    gpadc_env.busy = 0;
    return error;
}
#endif

#else

// default drv_gpadc_control() function for CPFT load when RTE_GPADC=0
void *drv_gpadc_control(int control, void *argu)
{
    return 0;
}

#endif /* RTE_GPADC */

/** @} */
