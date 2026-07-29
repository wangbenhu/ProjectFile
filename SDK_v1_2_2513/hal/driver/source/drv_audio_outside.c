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
 * @note     WM8753 as example
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
#if (RTE_AUDIO && RTE_AUDIO_USE_EXTERNAL)
#include "om_device.h"
#include "om_driver.h"


/*******************************************************************************
 * WM8753 CONFIGURATION DEFINES
 */
/// WM8753 address, GPIO5/CSB is low level, address:0x1A, otherwise is 0x1B
#define CFG_WM8753_DEV_ADDRESS              0x1A

/// Is mclk pin input 24M clock
#define CFG_WM8753_MCLK_PIN_24M             1

/// WM8753 role master or slave
#define CFG_WM8753_MST_EN                   0

/// 1: headset, 2: speaker
#define CFG_WM8753_DACOUT_SEL               1
/// MIC channel, MIC1:1, MIC2:2
#define CFG_WM8753_ADCIN_SEL                2


/*******************************************************************************
 * WM8753 REGISTERS DEFINES
 */
typedef enum {
    WM8753_REG_DAC          = 0x01,
    WM8753_REG_ADC          = 0x02,
    WM8753_REG_PCM          = 0x03,
    WM8753_REG_HIFI         = 0x04,
    WM8753_REG_IOCTL        = 0x05,
    WM8753_REG_SRATE1       = 0x06,
    WM8753_REG_SRATE2       = 0x07,
    WM8753_REG_LDAC         = 0x08,
    WM8753_REG_RDAC         = 0x09,
    WM8753_REG_BASS         = 0x0A,
    WM8753_REG_TREBLE       = 0x0B,
    WM8753_REG_ALC1         = 0x0C,
    WM8753_REG_ALC2         = 0x0D,
    WM8753_REG_ALC3         = 0x0E,
    WM8753_REG_NGATE        = 0x0F,
    WM8753_REG_LADC         = 0x10,
    WM8753_REG_RADC         = 0x11,
    WM8753_REG_ADCTL1       = 0x12,
    WM8753_REG_3D           = 0x13,
    WM8753_REG_PWR1         = 0x14,
    WM8753_REG_PWR2         = 0x15,
    WM8753_REG_PWR3         = 0x16,
    WM8753_REG_PWR4         = 0x17,
    WM8753_REG_ID           = 0x18,
    WM8753_REG_INTPOL       = 0x19,
    WM8753_REG_INTEN        = 0x1A,
    WM8753_REG_GPIO1        = 0x1B,
    WM8753_REG_GPIO2        = 0x1C,
    WM8753_REG_RESET        = 0x1F,
    WM8753_REG_RECMIX1      = 0x20,
    WM8753_REG_RECMIX2      = 0x21,
    WM8753_REG_LOUTM1       = 0x22,
    WM8753_REG_LOUTM2       = 0x23,
    WM8753_REG_ROUTM1       = 0x24,
    WM8753_REG_ROUTM2       = 0x25,
    WM8753_REG_MOUTM1       = 0x26,
    WM8753_REG_MOUTM2       = 0x27,
    WM8753_REG_LOUT1V       = 0x28,
    WM8753_REG_ROUT1V       = 0x29,
    WM8753_REG_LOUT2V       = 0x2A,
    WM8753_REG_ROUT2V       = 0x2B,
    WM8753_REG_MOUTV        = 0x2C,
    WM8753_REG_OUTCTL       = 0x2D,
    WM8753_REG_ADCIN        = 0x2E,
    WM8753_REG_INCTL1       = 0x2F,
    WM8753_REG_INCTL2       = 0x30,
    WM8753_REG_LINVOL       = 0x31,
    WM8753_REG_RINVOL       = 0x32,
    WM8753_REG_MICBIAS      = 0x33,
    WM8753_REG_CLOCK        = 0x34,
    WM8753_REG_PLL1CTL1     = 0x35,
    WM8753_REG_PLL1CTL2     = 0x36,
    WM8753_REG_PLL1CTL3     = 0x37,
    WM8753_REG_PLL1CTL4     = 0x38,
    WM8753_REG_PLL2CTL1     = 0x39,
    WM8753_REG_PLL2CTL2     = 0x3A,
    WM8753_REG_PLL2CTL3     = 0x3B,
    WM8753_REG_PLL2CTL4     = 0x3C,
    WM8753_REG_BIASCTL      = 0x3D,
    WM8753_REG_ADCTL2       = 0x3F
} WM8753_REGS;


/*******************************************************************************
 * TYPEDEFS
 */
/// Audio environment structure
typedef struct {
    audio_state_t state;
} audio_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
/// Audio registers default value
static uint16_t wm8753_regs[0x40] = {
    0x0000, 0x0008, 0x0000, 0x000a, 0x000a, 0x0033, 0x0000, 0x0007,    /* REG_0x00 - REG_0x07 */
    0x00ff, 0x00ff, 0x000f, 0x000f, 0x007b, 0x0000, 0x0032, 0x0000,    /* REG_0x08 - REG_0x0F */
    0x00c3, 0x00c3, 0x00c0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,    /* REG_0x10 - REG_0x17 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,    /* REG_0x18 - REG_0x1F */
    0x0055, 0x0005, 0x0050, 0x0055, 0x0050, 0x0055, 0x0050, 0x0055,    /* REG_0x20 - REG_0x27 */
    0x0079, 0x0079, 0x0079, 0x0079, 0x0079, 0x0000, 0x0005, 0x0000,    /* REG_0x28 - REG_0x2F */
    0x0000, 0x0097, 0x0097, 0x0000, 0x0004, 0x0000, 0x0083, 0x0024,    /* REG_0x30 - REG_0x27 */
    0x01ba, 0x0000, 0x0083, 0x0024, 0x01ba, 0x0000, 0x0000, 0x0000,    /* REG_0x38 - REG_0x3F */
};

/// Audio environment variable define
static audio_env_t audio_env = {
    .state      = AUDIO_STATE_IDLE
};


/*******************************************************************************
 * STATIC FUNCTIONS
 */
static void wm8753_i2c_init(void)
{
    const i2c_config_t i2c_config = {
        .mode  = I2C_MODE_MASTER,
        .speed = I2C_SPEED_100K
    };

    drv_i2c_init(OM_I2C0, &i2c_config);
}

static void wm8753_i2c_write(uint8_t *tx_data, uint16_t tx_num)
{
    drv_i2c_master_write(OM_I2C0, CFG_WM8753_DEV_ADDRESS, tx_data, tx_num, 0);
}

static void wm8753_set_reg(uint8_t reg_addr, uint8_t start_bit, uint8_t end_bit, uint16_t bit_val)
{
    uint8_t tx_buf[2];
    uint16_t mask = 0;
    uint16_t reg_val = 0;

    mask    = ((1 << ((end_bit - start_bit) + 1)) - 1) << start_bit;
    reg_val = wm8753_regs[reg_addr];
    reg_val = (reg_val & (~mask)) | ((bit_val << start_bit) & mask);
    wm8753_regs[reg_addr] = reg_val;

    /// Format of address and register value, address[15:9] | reg_val[8 : 0]
    tx_buf[0] = (reg_addr << 1) + ((reg_val >> 8) & 1);
    tx_buf[1] = (uint8_t)reg_val;

    /// Send data via i2c
    wm8753_i2c_write(tx_buf, sizeof(tx_buf));
}

/// @brief  USB mode, ADC Samplerate selected by SR and USB
static void wm8753_set_sample(i2s_sr_t sr)
{
    uint16_t sr_cfg = 0;

    switch (sr) {
    case I2S_SR_8K:
        sr_cfg = 0x06;
        break;

    case I2S_SR_11K:
        sr_cfg = 0x19;
        break;

    case I2S_SR_12K:
        sr_cfg = 0x08;
        break;

    case I2S_SR_16K:
        sr_cfg = 0x0a;
        break;

    case I2S_SR_22K:
        sr_cfg = 0x1b;
        break;

    case I2S_SR_24K:
        sr_cfg = 0x1c;
        break;

    case I2S_SR_32K:
        sr_cfg = 0x0c;
        break;

    case I2S_SR_44P1K:
        sr_cfg = 0x11;
        break;

    case I2S_SR_48K:
        sr_cfg = 0x00;
        break;

    default:
        break;
    }

    /// Use USB mode and samplerate control
    /// Bit0-0: USB, [1]USB mode,[0]normal mode
    /// Bit1-5: SR, sample rate control
    /// Bit8-8: SRMODE, [1] ADC sample rate selected by PSR, [0] ADC sample rate selected by SR and USB
    wm8753_set_reg(WM8753_REG_SRATE1, 0, 5, (sr_cfg << 1) | 0x1);
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
void drv_audio_outside_init(void)
{
    if(audio_env.state != AUDIO_STATE_IDLE) {
        return;
    }

    wm8753_i2c_init();

    /// Reset all registers to default state
    wm8753_set_reg(WM8753_REG_RESET, 0, 8, 0);

    /// Set DAC mute
    wm8753_set_reg(WM8753_REG_DAC, 3, 3, 1);

    /// bit6:   Verf enable, for all functions
    /// bit7-8: 01,for playback and record function
    wm8753_set_reg(WM8753_REG_PWR1, 6, 8, 3);

    /// Enable MCLK into digital
    wm8753_set_reg(WM8753_REG_PWR1, 0, 0, 0);

    /// MCLK from pin, if 24m need divide by 2, see SR table
    wm8753_set_reg(WM8753_REG_PLL1CTL1, 3, 3, CFG_WM8753_MCLK_PIN_24M);

    /// HIFI interface, set master or slave, set I2S mode
    wm8753_set_reg(WM8753_REG_HIFI, 0, 6, (CFG_WM8753_MST_EN << 6) | 2);

    /// BCLK: mclk is 12M, sclk = 12/4=3M
    wm8753_set_reg(WM8753_REG_SRATE2, 3, 5, 2);

    audio_env.state = AUDIO_STATE_INITED;
}

void drv_audio_outside_uninit(void)
{
    /// Is uninited?
    if(audio_env.state == AUDIO_STATE_INITED) {
        return;
    }

    /// If working, stop first.
    if(audio_env.state > AUDIO_STATE_INITED) {
        return;
    }

    /// Set uninit state
    audio_env.state = AUDIO_STATE_UNINITED;

    /// Disable MCLK into digital
    wm8753_set_reg(WM8753_REG_PWR1, 0, 0, 1);

    // VREF disable
    wm8753_set_reg(WM8753_REG_PWR1, 6, 8, 0);

    drv_i2c_uninit(OM_I2C0);
}

om_error_t drv_audio_outside_play_start(audio_play_config_t *config)
{
    /// Inited first
    if((audio_env.state & AUDIO_STATE_INITED) == 0) {
        return OM_ERROR_STATUS;
    }

    /// Is instanced?
    if(audio_env.state & AUDIO_STATE_PLAY) {
        return OM_ERROR_STATUS;
    }

    audio_env.state |= AUDIO_STATE_PLAY;

    /// Enable DAC left and right
    wm8753_set_reg(WM8753_REG_PWR1, 2, 3, 3);

#if CFG_WM8753_DACOUT_SEL == 1
    /// LOUT1 and ROUT1 enable
    wm8753_set_reg(WM8753_REG_PWR3, 7, 8, 3);
#elif CFG_WM8753_DACOUT_SEL == 2
    /// LOUT2 and ROUT2 enable
    wm8753_set_reg(WM8753_REG_PWR3, 5, 6, 3);
#endif

    /// Left mixer and Right mixer enable
    wm8753_set_reg(WM8753_REG_PWR4, 0, 1, 3);

    /// Left mixer control,  LD2LO=1, LM2LO=0, LM2LOVOL=0
    wm8753_set_reg(WM8753_REG_LOUTM1, 4, 8, (1 << 4) | (0 << 3) | 0);
    /// Right mixer control, RD2RO=1, RM2RO=0, RM2ROVOL=0
    wm8753_set_reg(WM8753_REG_ROUTM1, 4, 8, (1 << 4) | (0 << 3) | 0);

#if CFG_WM8753_DACOUT_SEL == 1
    /// Set left out volume,  LOUT1VOL=0X7F
    wm8753_set_reg(WM8753_REG_LOUT1V, 0, 8, 0x100 | config->volume);
    /// Set right out volume, ROUT1VOL=0X7F
    wm8753_set_reg(WM8753_REG_ROUT1V, 0, 8, 0x100 | config->volume);
#elif CFG_WM8753_DACOUT_SEL == 2
    /// Set left out volume,  LOUT2VOL=0X7F
    wm8753_set_reg(WM8753_REG_LOUT2V, 0, 8, 0x100 | config->volume);
    /// Set right out volume, ROUT1VOL=0X7F
    wm8753_set_reg(WM8753_REG_ROUT2V, 0, 8, 0x100 | config->volume);
#endif

    /// Set DAC sample rate
    wm8753_set_sample(config->sample_rate);

    /// Set Master mode, I2S format and bit width 1 << 6
    wm8753_set_reg(WM8753_REG_HIFI, 0, 6, (CFG_WM8753_MST_EN << 6) | (I2S_BW_24BIT == config->bit_width ? 8 : 0) | (2 << 0));

    /// DAC unmute
    wm8753_set_reg(WM8753_REG_DAC, 3, 3, 0);

    return OM_ERROR_OK;
}

om_error_t drv_audio_outside_play_stop(void)
{
    /// Is stopped?
    if((audio_env.state & AUDIO_STATE_PLAY) == 0) {
        return OM_ERROR_STATUS;
    }

    /// Update state
    audio_env.state &= (~AUDIO_STATE_PLAY);

    /// DAC mute
    wm8753_set_reg(WM8753_REG_DAC, 3, 3, 1);
    /// Power down DAC
    wm8753_set_reg(WM8753_REG_PWR1, 2, 3, 0);
    /// Power down output
    wm8753_set_reg(WM8753_REG_PWR3, 0, 8, 0);

    return OM_ERROR_OK;
}

om_error_t drv_audio_outside_record_start(audio_record_config_t *config)
{
    /// Is instanced?
    if(audio_env.state & AUDIO_STATE_RECORD) {
        return OM_ERROR_STATUS;
    }

    /// Inited first
    if((audio_env.state & AUDIO_STATE_INITED) == 0) {
        return OM_ERROR_STATUS;
    }

    audio_env.state |= AUDIO_STATE_RECORD;

    wm8753_set_sample(config->sample_rate);

    /// Enable Left and Right ADC ADCL=1, ADCR=1, Left PGA=1, Right PGA=1
    wm8753_set_reg(WM8753_REG_PWR2, 2, 5, 0xF);

    /// Config interface
    /// BCLK decide by MS, ADCDAT config output, hifi, LRC output
    wm8753_set_reg(WM8753_REG_IOCTL, 0, 8, 0xa8 | CFG_WM8753_MST_EN);

    /// USB mode ADCOSR set to 0 for better SNR
    wm8753_set_reg(WM8753_REG_SRATE2, 1, 1, 0);

    /// Set audio bit width
    wm8753_set_reg(WM8753_REG_HIFI,  0, 3, (config->bit_width == I2S_BW_24BIT ? 8 : 0) | (2 << 0));

    /// Left ADC volume
    wm8753_set_reg(WM8753_REG_LADC, 0, 8, 0x100 | config->volume);
    /// Right ADC volume
    wm8753_set_reg(WM8753_REG_RADC, 0, 8, 0x100 | config->volume);

#if CFG_WM8753_ADCIN_SEL == 1
    /// Microphone amplifier enable
    wm8753_set_reg(WM8753_REG_PWR2, 8, 8, 1);
    /// Preamp gain boost contorl, 0:12dB, 1:18dB, 2:24dB, 3:30dB
    wm8753_set_reg(WM8753_REG_INCTL1, 5, 6, 0);
#elif CFG_WM8753_ADCIN_SEL == 2
    /// Microphone amplifier enable
    wm8753_set_reg(WM8753_REG_PWR2, 7, 7, 1);
    /// Preamp gain boost contorl, 0:12dB, 1:18dB, 2:24dB, 3:30dB
    wm8753_set_reg(WM8753_REG_INCTL1, 7, 8, 0);
#endif

    // ADC Input select PGA
    wm8753_set_reg(WM8753_REG_ADCIN, 0, 3, 0);
    // Left Channel Input Volume Control, 0dB gain, unmute
    wm8753_set_reg(WM8753_REG_LINVOL, 0, 8, 0x117);
    // Right Channel Input Volume Control, 0dB gain, unmute
    wm8753_set_reg(WM8753_REG_RINVOL, 0, 8, 0x117);

    /// Enable mic bias enable
    wm8753_set_reg(WM8753_REG_PWR1, 5, 5, 1);
    // Microphone Bias Voltage Control,0 = 0.9 * AVDD;1 = 0.75 * AVDD
    wm8753_set_reg(WM8753_REG_MICBIAS, 0, 0, 1);

    // MIC BIAS select mic1
#if (CFG_WM8753_ADCIN_SEL == 1)
    wm8753_set_reg(WM8753_REG_MICBIAS, 6, 7, 0);
    // MIC BIAS select mic2
#elif (CFG_WM8753_ADCIN_SEL == 2)
    wm8753_set_reg(WM8753_REG_MICBIAS, 6, 7, 1);
#endif

    // Set mono mix
    wm8753_set_reg(WM8753_REG_ADCIN, 4, 5, config->channel == 1 ? 1 : 0);

    return OM_ERROR_OK;
}

om_error_t drv_audio_outside_record_stop(void)
{
    /// Is stopped?
    if((audio_env.state & AUDIO_STATE_RECORD) == 0) {
        return OM_ERROR_STATUS;
    }

    /// Disable Left and Right ADC
    wm8753_set_reg(WM8753_REG_PWR2, 2, 3, 0);

    /// Update state
    audio_env.state &= (~AUDIO_STATE_RECORD);

    return OM_ERROR_OK;
}

audio_state_t drv_audio_outside_work_state(void)
{
    return audio_env.state;
}

#endif /* RTE_AUDIO_USE_EXTERNAL */

/** @} */