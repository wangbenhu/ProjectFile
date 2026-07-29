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
 * @brief    AUDIO driver source file
 * @details  AUDIO driver source file
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
#if (RTE_AUDIO_USE_INTERNAL)
#include "om_device.h"
#include "om_driver.h"


/*******************************************************************************
 * DEBUG DEFINES
 */
/// Audio Assert
#define AUDIO_ASSERT_EN         (1)

#if (AUDIO_ASSERT_EN)
#define AUDIO_ASSERT(x)         OM_ASSERT(x)
#else
#define AUDIO_ASSERT(x)
#endif /* AUDIO_ASSERT_EN */


/*******************************************************************************
 * TYPEDEFS
 */
/// Audio bitmap
typedef struct {
    uint8_t mute_int   : 1;
    uint8_t unmute_int : 1;
} audio_bit_t;

/// Audio environment structure
typedef struct {
    volatile audio_state_t  state;
    volatile audio_bit_t    bit;
} audio_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
/// Audio environment variable define
static audio_env_t audio_env = {
    .state          = AUDIO_STATE_IDLE,
    .bit.mute_int   = 0,
    .bit.unmute_int = 0
};

/// Audio resource variable define
static const drv_resource_t audio_resource = {
    .cap        = 0,    /* Not present */
    .reg        = (void *)OM_AUDIO,
    .env        = (void *)&audio_env,
    .irq_num    = AUDIO_IRQn,
    .irq_prio   = RTE_AUDIO_IRQ_PRIORITY,
};


/*******************************************************************************
 * STATIC FUNCIONTS
 */
static void _audio_power_on(void)
{
#if 0  /* Power on by register */

    /// LDO/BIAS/ADC/PGA control by reg
    register_set(&OM_AUDIO->CODEC_ANA_PWR_1, MASK_2REG(AU_ANA_PWR_LDO_CTRL, 1, AU_ANA_PWR_ADC_CTRL, 1));

    /// Power on LDO and BIAS
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_PD_IREF, 0));
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_2, MASK_1REG(AU_ANA_PD_LDO28, 0));
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_PD_AUREF, 0));
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_EN_REF_BYP, 1));
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_PD_BIAS_GEN, 0));
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_2, MASK_1REG(AU_ANA_PD_LDO12, 0));
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_PD_CLK, 0));

    /// Delay for DC point power on correctly
    DRV_DELAY_MS(100);

    /// Power on ADC and PGA
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_PD_PGA, 0));
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_PD_BIAS, 0));
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_PD_CORE, 0));
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_RST_ADC, 0));
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_EN_REF_BYP, 0));

    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_PGA_EN, 0));

#else /* Power on by fsm */

    // Enable 32K clock
    register_set(&OM_AUDIO->CODEC_CLK_CTRL_1, MASK_1REG(AU_CLK_CTRL1_32K_EN, 1));

    // 1. Power on
    register_set(&OM_AUDIO->CODEC_ANA_PWR_1, MASK_1REG(AU_ANA_PWR_ON_EN, 1));
    // 2. Wait for power on interrupt flag
    while((OM_AUDIO->CODEC_INT_STATUS & AU_INT_STATUS_ON_MASK) == 0);
    // 3. Write 1 to clear power on interrupt flag
    register_set(&OM_AUDIO->CODEC_INT_STATUS, MASK_1REG(AU_INT_STATUS_ON, 1));

    // PGA enable
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_PGA_EN, 0));
#endif
}

static void _audio_power_off(void)
{
#if 0 /* Power off by register */

    /// Power off ADC and PGA
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_PD_PGA, 1));
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_PD_BIAS, 1));
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_PD_CORE, 1));
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_RST_ADC, 1));
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_EN_REF_BYP, 1));
    DRV_DELAY_US(20);

    /// Power off LDO and BIAS
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_PD_IREF, 1));
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_2, MASK_1REG(AU_ANA_PD_LDO28, 1));
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_PD_AUREF, 1));
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_PD_BIAS_GEN, 1));
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_2, MASK_1REG(AU_ANA_PD_LDO12, 1));
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_PD_CLK, 1));

    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_PGA_EN, 1));

#else /* Power off by fsm */

    // 1. Set codec power off
    register_set(&OM_AUDIO->CODEC_ANA_PWR_1, MASK_1REG(AU_ANA_PWR_ON_EN, 0));
    // 2. Wait for power off interrupt flag
    while((OM_AUDIO->CODEC_INT_STATUS & AU_INT_STATUS_OFF_MASK) == 0);
    // 3. Write 1 to clear power off interrupt flag
    register_set(&OM_AUDIO->CODEC_INT_STATUS, MASK_1REG(AU_INT_STATUS_OFF, 1));

    // Disable 32K clock
    register_set(&OM_AUDIO->CODEC_CLK_CTRL_1, MASK_1REG(AU_CLK_CTRL1_32K_EN, 0));

    // PGA disable
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_PGA_EN, 1));
#endif
}

static void _audio_enable_daif_16m(void)
{
    // Open Audio CLOCK 16M
    drv_pmu_ana_enable(true, PMU_ANA_AUDIO);
}

static void _audio_disable_daif_16m(void)
{
    // Close Audio CLOCK 16M
    drv_pmu_ana_enable(false, PMU_ANA_AUDIO);
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
void drv_audio_inside_init(void)
{
    audio_env_t *env = (audio_env_t *)audio_resource.env;

    /// Is inited?
    if(env->state != AUDIO_STATE_IDLE) {
        return;
    }

    // Enable audio 16M
    _audio_enable_daif_16m();

    /// Reset Audio module and Enable clock
    DRV_RCC_RESET(RCC_CLK_AUDIO);

    /// Power on sequence
    _audio_power_on();

    /// Audio power on by fsm
#if RTE_AUDIO_USE_ANA_MIC

    /// Disable DMIC
    register_set(&OM_AUDIO->ADC_CTRL, MASK_1REG(AU_CTRL_DMIC_EN, 0));

#else

    /// Enable DMIC
    register_set(&OM_AUDIO->ADC_CTRL, MASK_1REG(AU_CTRL_DMIC_EN, 1));

#endif /* RTE_AUDIO_USE_ANA_MIC */

    /// Enable DC offset
    register_set(&OM_AUDIO->ADC_CTRL, MASK_1REG(AU_CTRL_DC_EN, 1));

    /// Enable codec clock, i2s_sdi and i2s_sdo connecnt to internal codec
    register_set(&OM_AUDIO->CODEC_CLK_CTRL_1, MASK_2REG(AU_CLK_CTRL1_CLK_EN, 1, AU_CLK_I2S_CONN_CTRL, 3));

    /// Clear and Disable Audio interrupt
    NVIC_ClearPendingIRQ(AUDIO_IRQn);
    NVIC_SetPriority(AUDIO_IRQn, RTE_AUDIO_IRQ_PRIORITY);
    NVIC_EnableIRQ(AUDIO_IRQn);

    /// Set inited state
    env->state = AUDIO_STATE_INITED;
}

void drv_audio_inside_uninit(void)
{
    audio_env_t *env = (audio_env_t *)audio_resource.env;

    /// Is uninited?
    if(env->state == AUDIO_STATE_UNINITED) {
        return;
    }

    /// If working, stop first
    if(env->state > AUDIO_STATE_INITED) {
        return;
    }

    /// Set uninit state
    env->state = AUDIO_STATE_UNINITED;

    /// Disable codec clock, i2s_sdi and i2s_sdo reset to default
    register_set(&OM_AUDIO->CODEC_CLK_CTRL_1, MASK_2REG(AU_CLK_CTRL1_CLK_EN, 0, AU_CLK_I2S_CONN_CTRL, 0));

    /// Power down sequence
    _audio_power_off();

    /// Gate audio clock
    DRV_RCC_CLOCK_ENABLE(RCC_CLK_AUDIO, 0);

    // Disable audio 16M
    _audio_disable_daif_16m();
}

om_error_t drv_audio_inside_record_start(audio_record_config_t *config)
{
    audio_env_t *env = (audio_env_t *)audio_resource.env;

    AUDIO_ASSERT(config);
    AUDIO_ASSERT(config->channel == I2S_CHN_MONO);
    AUDIO_ASSERT(config->bit_width == I2S_BW_16BIT || config->bit_width == I2S_BW_24BIT);
    AUDIO_ASSERT(config->sample_rate == I2S_SR_8K || config->sample_rate == I2S_SR_16K || config->sample_rate == I2S_SR_32K);

    /// Is instanced?
    if(env->state & AUDIO_STATE_RECORD) {
        return OM_ERROR_STATUS;
    }

    /// Inited first
    if((env->state & AUDIO_STATE_INITED) == 0) {
        return OM_ERROR_STATUS;
    }

    env->state |= AUDIO_STATE_RECORD;

    /// Enable adc
    register_set(&OM_AUDIO->ADC_CTRL, MASK_1REG(AU_CTRL_EN, 1));

    /// Reset filter
    register_set(&OM_AUDIO->ADC_CTRL,MASK_1REG(AU_CTRL_SW_RESET_X, 1));
    register_set(&OM_AUDIO->CODEC_CLK_CTRL_1, MASK_1REG(AU_CLK_CTRL1_RSTN, 1));

    /// Enable mute and unmute interrupt
    env->bit.mute_int   = 0;
    env->bit.unmute_int = 0;
    register_set(&OM_AUDIO->CODEC_INT_CTRL, MASK_2REG(AU_INT_CTRL_MUTE, 0, AU_INT_CTRL_UNMUTE, 0));
    /// Wait unmute flow finish
    while((env->bit.mute_int == 0) || (env->bit.unmute_int == 0));

    /// Enable clip interrupt
    register_set(&OM_AUDIO->CODEC_INT_CTRL, MASK_1REG(AU_INT_CTRL_CLIP, 0));
    /// When output is clipping, gain decreased automatically
    register_set(&OM_AUDIO->ADC_CTRL, MASK_1REG(AU_CTRL_ANTI_CLIP, 1));
    /// 7:0dB, 6:-1.16dB, 5:-2.5dB, 4: -4.08dB, 3: -6.02dB, 2: -8.53dB, 1: -12.05dB, 0: -18.09dB
    register_set(&OM_AUDIO->ADC_THD_CTRL, MASK_1REG(AU_CLIP_THD_CTRL, 7));

    /// Set sample rete
    switch (config->sample_rate) {
    case I2S_SR_8K:
        /// 8K select 2M
        register_set(&OM_AUDIO->CODEC_CLK_CTRL_1, MASK_1REG( AU_CLK_CTRL1_DMIC_SEL, 0));
        register_set(&OM_AUDIO->ADC_CTRL, MASK_3REG(AU_CTRL_SR_4X, 1, AU_CTRL_SR_2X, 0, AU_CTRL_SR_1X, 0));
        break;

    case I2S_SR_16K:
        /// 8K select 2M
        register_set(&OM_AUDIO->CODEC_CLK_CTRL_1, MASK_1REG( AU_CLK_CTRL1_DMIC_SEL, 0));
        register_set(&OM_AUDIO->ADC_CTRL, MASK_3REG(AU_CTRL_SR_4X, 0, AU_CTRL_SR_2X, 1, AU_CTRL_SR_1X, 0));
        break;

    case I2S_SR_32K:
        /// 8K select 3.2M
        register_set(&OM_AUDIO->CODEC_CLK_CTRL_1, MASK_1REG( AU_CLK_CTRL1_DMIC_SEL, 1));
        register_set(&OM_AUDIO->ADC_CTRL, MASK_3REG(AU_CTRL_SR_4X, 0, AU_CTRL_SR_2X, 0, AU_CTRL_SR_1X, 1));
        break;

    default:
        AUDIO_ASSERT(0);
        break;
    }

#if RTE_AUDIO_USE_ANA_MIC
    // DC offset 0.65V
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_2, MASK_1REG(AU_ANA_SEL_BIAS, 0xa));
#endif /* RTE_AUDIO_USE_ANA_MIC */

    /// Set extra digist gain
    register_set(&OM_AUDIO->ADC_CTRL, MASK_1REG(AU_CTRL_CIC_SCALE, config->gain_cic));

    /// Set volume
    register_set(&OM_AUDIO->ADC_VOL_CTRL, MASK_5REG(AU_VOL_UNMUTE_RATE, 5,
                 AU_VOL_MUTE_RATE,   5,
                 AU_VOL_ADCUNMU,     0,
                 AU_VOL_ADCMU,       0,
                 AU_VOL_MUTE_BYPASS, 1));

    register_set(&OM_AUDIO->ADC_VOL_CTRL, MASK_2REG(AU_VOL_LADC, config->volume, AU_VOL_UPDATE, 1));
    while(OM_AUDIO->ADC_VOL_CTRL & AU_VOL_UPDATE_MASK);

    return OM_ERROR_OK;
}

om_error_t drv_audio_inside_record_stop(void)
{
    audio_env_t *env = (audio_env_t *)audio_resource.env;

    /// Is stopped?
    if((env->state & AUDIO_STATE_RECORD) == 0) {
        return OM_ERROR_STATUS;
    }

    /// Disable interrupts
    OM_AUDIO->CODEC_INT_CTRL = 0xFF;

    /// ADC off and clock gated
    register_set(&OM_AUDIO->ADC_CTRL, MASK_1REG(AU_CTRL_EN, 0));

    /// Update state
    env->state &= (~AUDIO_STATE_RECORD);

    return OM_ERROR_OK;
}

audio_state_t drv_audio_inside_work_state(void)
{
    audio_env_t *env = (audio_env_t *)audio_resource.env;
    return env->state;
}

void drv_audio_inside_isr(void)
{
    uint32_t status = OM_AUDIO->CODEC_INT_STATUS;
    audio_env_t *env = (audio_env_t *)audio_resource.env;

    if(status & AU_INT_STATUS_CLIP_MASK) {
        OM_AUDIO->CODEC_INT_STATUS |= AU_INT_STATUS_CLIP_MASK;
    }

    if(status & AU_INT_STATUS_MUTE_MASK) {
        OM_AUDIO->CODEC_INT_CTRL |= AU_INT_CTRL_MUTE_MASK;
        env->bit.mute_int = 1;
    }

    if(status & AU_INT_STATUS_UNMUTE_MASK) {
        OM_AUDIO->CODEC_INT_CTRL |= AU_INT_CTRL_UNMUTE_MASK;
        env->bit.unmute_int = 1;
    }
}

#endif /* RTE_AUDIO_USE_INTERNAL */

/** @} */