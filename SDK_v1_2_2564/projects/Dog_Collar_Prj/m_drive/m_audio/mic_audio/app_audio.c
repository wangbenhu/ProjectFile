#include "om_driver.h"


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
 * @brief    example for using audio
 * @details  example for using audio:
 * @version
 *
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


/*******************************************************************************
 * INCLUDES
 */
#include "om_driver.h"
#include "om_log.h"
#include "om_mem.h"
#include "m_codec.h"
#include "pm.h"
#include <math.h>
#include "cmsis_os2.h"
#include "app_audio.h"
/*
*	DMA互斥读取互斥
*
*
*
*
*
*/
void app_audio_close(void);
/*******************************************************************************
 * MACROS
 */
#define PAD_MIC_P            2
#define PAD_MIC_N            3
#define MUX_MIC_P            PINMUX_PAD2_INPUT_MODE_CFG
#define MUX_MIC_N            PINMUX_PAD3_INPUT_MODE_CFG

/// Audio Ring Buffer for RX Data Use, 32KB, 8K[2s]--16k[1s]--32k[0.5s]
#define LEN_RING_BUF            1200

/// I2S DMA chain number
#define RX_BUF_CHAIN_NUM        RTE_I2S_GPDMA_LLP_CHAIN_NUM
#define TX_BUF_CHAIN_NUM        RTE_I2S_GPDMA_LLP_CHAIN_NUM

/// I2S RX blcok size and number
#define RX_BUF_BLOCK_SIZE       240
#define RX_BUF_BLOCK_NUM        (LEN_RING_BUF / RX_BUF_BLOCK_SIZE)

/// I2S TX blcok size and number
#define TX_BUF_BLOCK_SIZE       240
#define TX_BUF_BLOCK_NUM        -1U

//decode fifo
#define DECODE_BUF_LEN          LEN_RING_BUF*4
/*******************************************************************************
 * TYPEDEFS
 */

/*读写互斥锁*/
osMutexId_t dma_mutex;

//osMutexId_t speaker_mic_mutex;


/*******************************************************************************
 * CONST & VARIABLES
 */
static const pin_config_t pin_cfg [] = {
    {PAD_MIC_P,    {MUX_MIC_P},    PMU_PIN_MODE_FLOAT,    PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_MIC_N,    {MUX_MIC_N},    PMU_PIN_MODE_FLOAT,    PMU_PIN_DRIVER_CURRENT_NORMAL},
};

/// RX ring buffer
// static uint8_t *p_ring_buf;
static uint8_t p_ring_buf[LEN_RING_BUF*2];
/// RX ring buffer block index
static uint32_t rx_block_idx = 0;

//decode fifo
static om_fifo_t decode_fifo;
static uint8_t decode_buffer[DECODE_BUF_LEN];

static volatile uint8_t ready_slots[RX_BUF_BLOCK_NUM];
static volatile uint8_t ready_head;
static volatile uint8_t ready_tail;
static volatile uint32_t dropped_blocks;
static bool driver_initialized;
static volatile bool driver_running;
static app_audio_rx_notify_t rx_notify;
static void *rx_notify_context;
/* Legacy timed-record test state; runtime control is owned by audio_mic_task. */
static uint8_t packet_seq;

/// log with debug
#define OM_LOG_DEBUG(format, ...)                  om_log(OM_LOG_INFO, format,  ## __VA_ARGS__)
/// log debug array
#define OM_LOG_DEBUG_ARRAY(array, len)             do{int __i; for(__i=0;__i<(len);++__i)OM_LOG_DEBUG("%02X ",((uint8_t *)(array))[__i]);}while(0)

/*******************************************************************************
 * LOCAL FUNCTIONS
 */
#define PIN_MIC_Audio_TEST      		28// adc采集周期

static const pin_config_t pin_config_test[] = 
{
	{PIN_MIC_Audio_TEST,		{PINMUX_GPIO_MODE_CFG}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
};

static const gpio_config_t gpio_config_test[] = 
{
	{OM_GPIO0, PIN_MIC_Audio_TEST,	GPIO_DIR_OUTPUT, GPIO_LEVEL_HIGH, GPIO_TRIG_NONE},
};

// 初始化配置一些调试用的IO，比如RF发送数据时拉一个IO看时间，接收时也是看是否有收到数据，还有音频更新数据的时间
void test_pin_init_audio(void)
{
	drv_pin_init(pin_config_test, sizeof(pin_config_test) / sizeof(pin_config_test[0]));

	drv_gpio_init(gpio_config_test, sizeof(gpio_config_test) / sizeof(gpio_config_test[0]));
}
extern uint32_t m_wave_data_size;

// 三频段抬升滤波器 (300Hz-4kHz)
typedef struct {
    int16_t b0, b1, b2, a1, a2;
    int32_t state[2];
} BiquadFilter;

#define M_PI 3.14159265358979323846
BiquadFilter speech_filter;

/* Speech Band Dynamic AGC: 850Hz-3.4kHz */
typedef struct {
    BiquadFilter bp;        /* bandpass filter */
    int32_t env;            /* envelope tracker (Q15) */
    int32_t gain;           /* current gain (Q15, 1.0 = 32768) */
} SpeechBandAGC;

static SpeechBandAGC speech_agc;

#define SPEECH_AGC_TARGET      16384   /* Q15: 0.5 (-6dBFS) */
#define SPEECH_AGC_MAX_GAIN    196608  /* Q15: 6.0x (+15.5dB) */
#define SPEECH_AGC_ATTACK_SHIFT  3     /* fast attack: 1/8 step */
#define SPEECH_AGC_RELEASE_SHIFT 7     /* slow release: 1/128 step */
#define SPEECH_AGC_GAIN_SHIFT    5     /* gain smoothing: 1/32 step */
#define VOLUME_BOOST            3      /* 3x overall volume boost */

/* Initialize speech band AGC: bandpass 850Hz-3400Hz, sample_rate=8000 */
void speech_band_agc_init(SpeechBandAGC *agc, int center_freq, int sample_rate) {
    float w0 = 2.0f * M_PI * (float)center_freq / (float)sample_rate;
    float Q = 0.67f;  /* wide bandwidth: 850-3400Hz */
    float alpha = sinf(w0) / (2.0f * Q);

    agc->bp.b0 = (int16_t)(alpha * 32767.0f);
    agc->bp.b1 = 0;
    agc->bp.b2 = (int16_t)(-alpha * 32767.0f);
    agc->bp.a1 = (int16_t)(-2.0f * cosf(w0) * 32767.0f);
    agc->bp.a2 = (int16_t)((1.0f - alpha) * 32767.0f);
    agc->bp.state[0] = agc->bp.state[1] = 0;

    agc->env = 0;
    agc->gain = 32768;  /* Q15: 1.0 */
}

/* Process one sample through speech band AGC */
int16_t speech_band_agc_process(SpeechBandAGC *agc, int16_t sample) {
    int32_t w, bp_out, desired_gain, result;
    int16_t abs_bp;

    /* Bandpass filter: extract 850-3400Hz speech band */
    w = (int32_t)sample
        - ((agc->bp.a1 * agc->bp.state[0]) >> 15)
        - ((agc->bp.a2 * agc->bp.state[1]) >> 15);

    bp_out = ((agc->bp.b0 * w) >> 15)
           + ((agc->bp.b1 * agc->bp.state[0]) >> 15)
           + ((agc->bp.b2 * agc->bp.state[1]) >> 15);

    agc->bp.state[1] = agc->bp.state[0];
    agc->bp.state[0] = w;

    /* Envelope detection with attack/release smoothing */
    abs_bp = (int16_t)(bp_out < 0 ? -bp_out : bp_out);

    if (abs_bp > agc->env) {
        agc->env += (abs_bp - agc->env) >> SPEECH_AGC_ATTACK_SHIFT;
    } else {
        agc->env += (abs_bp - agc->env) >> SPEECH_AGC_RELEASE_SHIFT;
    }

    /* Compute desired gain: drive speech band to target level */
    if (agc->env > 100) {
        desired_gain = ((int32_t)SPEECH_AGC_TARGET << 15) / agc->env;
        if (desired_gain > SPEECH_AGC_MAX_GAIN) {
            desired_gain = SPEECH_AGC_MAX_GAIN;
        }
        /* Smooth gain transition */
        agc->gain += (desired_gain - agc->gain) >> SPEECH_AGC_GAIN_SHIFT;
    }

    /* Apply AGC gain + volume boost to original signal */
    result = (int32_t)sample * agc->gain >> 15;
    result = result * VOLUME_BOOST;  /* overall volume boost */
    return (int16_t)__SSAT(result, 16);
}

__RAM_CODE void i2s_external_rx_loop_isr_cb(void *i2s_reg, drv_event_t event, void *addr, void *size)
{
    uint32_t offset = (uint32_t)((uint8_t *)addr - p_ring_buf);
    uint8_t slot = (uint8_t)(offset / RX_BUF_BLOCK_SIZE);
    uint8_t next = (uint8_t)((ready_head + 1U) % RX_BUF_BLOCK_NUM);

    if (!driver_running || offset >= sizeof(p_ring_buf) ||
        (offset % RX_BUF_BLOCK_SIZE) != 0U || next == ready_tail) {
        dropped_blocks++;
        return;
    }

    ready_slots[ready_head] = slot;
    ready_head = next;
    if (rx_notify != NULL) {
        rx_notify(rx_notify_context);
    }

#if 0

    drv_i2s_read_dma(OM_I2S, p_ring_buf + rx_block_idx * RX_BUF_BLOCK_SIZE, RX_BUF_BLOCK_SIZE);

    /* Speech band AGC + 16-bit to 8-bit conversion + UART send */
    {
        static uint8_t pcm8_buf[RX_BUF_BLOCK_SIZE / 2];
        const int16_t *pcm16 = (const int16_t *)(p_ring_buf + rx_block_idx * RX_BUF_BLOCK_SIZE);
        uint32_t sample_count = RX_BUF_BLOCK_SIZE / 2U;

        for (uint32_t i = 0; i < sample_count; i++) {
            int16_t processed = speech_band_agc_process(&speech_agc, pcm16[i]);
            pcm8_buf[i] = (uint8_t)(processed >> 8);
        }

        drv_uart_write(OM_UART1, pcm8_buf, sample_count, 10);
    }

	 // eee = drv_flash_write(OM_FLASH1, 0x50000+m_wave_data_size, p_ring_buf + rx_block_idx * RX_BUF_BLOCK_SIZE, RX_BUF_BLOCK_SIZE);
	drv_gpio_write(OM_GPIO0, GPIO_MASK(PIN_MIC_Audio_TEST),GPIO_LEVEL_LOW);

	m_wave_data_size+=RX_BUF_BLOCK_SIZE;
	
#if 1
	//drv_uart_write(OM_UART1,addr,len,200);
  //  uint16_t len = (size);
//	drv_uart_write_int(OM_UART1,addr,len);
   // OM_LOG_DEBUG_ARRAY(addr,len);
    rx_block_idx = ((rx_block_idx + 1) % RX_BUF_BLOCK_NUM);
	
	//	app_audio_close();
 //om_log(OM_LOG_INFO, "\r\n");
 
    return;
#endif
	

    //编码，存储
    uint8_t output[256] = {0x01};

    output[0] = 0x01;
    
    if (packet_seq%4 == 0)
            output[1] = 0x08;
        else if (packet_seq%4 == 1)
            output[1] = 0x38;
        else if (packet_seq%4 == 2)
            output[1] = 0xc8;
        else if (packet_seq%4 == 3)
            output[1] = 0xf8;
        packet_seq++;
    codec_encode(addr,(uint32_t)size,output+2,sizeof(output));

    output[59] = 0;
/*
     om_log(OM_LOG_INFO, "sbc\n");
     for(int i=0;i<RX_BUF_BLOCK_SIZE/4;i++)
     {
         om_log(OM_LOG_INFO, "%x ",output[i]);
     }
     om_log(OM_LOG_INFO, "\n");*/

    // om_fifo_in(&decode_fifo, output, 60);

   // hid_voice_send(output,60);

    rx_block_idx = ((rx_block_idx + 1) % RX_BUF_BLOCK_NUM);

#endif
}
/* Internal: audio close without mutex */
static void _audio_close_impl(void)
{
    drv_audio_record_stop();
    drv_audio_uninit();
    drv_i2s_uninit(OM_I2S);
}

//#pragma arm_section
/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/* Timer callback: stop recording after duration expires */

void app_audio_init(void)
{
	    // 创建互斥锁
    const osMutexAttr_t mutex_attr = { .name = "dma_mutex" };
    dma_mutex = osMutexNew(&mutex_attr);

	osMutexAcquire(dma_mutex, osWaitForever);   // 加锁
	
    drv_pin_init(pin_cfg, sizeof(pin_cfg) / sizeof(pin_cfg[0]));

    om_fifo_init(&decode_fifo,decode_buffer,DECODE_BUF_LEN);

    codec_init();
	
	osMutexRelease(dma_mutex);        
}
/* Internal: audio open without mutex */
static void _audio_open_impl(void)
{
    test_pin_init_audio();
    audio_record_config_t audio_rec_config = {
        .channel        = I2S_CHN_MONO,
        .bit_width      = I2S_BW_16BIT,
        .sample_rate    = I2S_SR_8K,
        .volume         = 0x95, /**< volume: 0x64 means 0dB, 0.5dB step, max 24.5dB */
        .gain_adc       = AUDIO_ADC_GAIN_6DB,
        .gain_pga       = AUDIO_PGA_GAIN_21DB,
    };

    i2s_config_t i2s_config = {
        .role           = I2S_ROLE_MASTER,
        .dir            = I2S_DIR_RX,
        .channel        = I2S_CHN_MONO,
        .bit_width      = I2S_BW_16BIT,
        .sample_rate    = I2S_SR_8K
    };

    /// Init Audio codec and start
    drv_audio_init();
    drv_audio_record_start(&audio_rec_config);

    /// Init I2S
    drv_i2s_init(OM_I2S, &i2s_config);
    drv_i2s_rx_register_isr_callback(OM_I2S, i2s_external_rx_loop_isr_cb);

    packet_seq = 0;

    /* Initialize speech band AGC: 850Hz-3400Hz, 8kHz sample rate */
    speech_band_agc_init(&speech_agc, 1700, 8000);

    /// Create chain and start
    for(rx_block_idx = 0; rx_block_idx < RX_BUF_CHAIN_NUM; rx_block_idx++) {
        drv_i2s_read_dma(OM_I2S, p_ring_buf + rx_block_idx * RX_BUF_BLOCK_SIZE, RX_BUF_BLOCK_SIZE);
    }

    om_log(OM_LOG_INFO, "app_audio_open\n");
}
void app_audio_uinit(void){};

void app_audio_open(void)
{	    
	osMutexAcquire(dma_mutex, osWaitForever);
	_audio_open_impl();
	osMutexRelease(dma_mutex);
}


void app_audio_close(void)
{
	osMutexAcquire(dma_mutex, osWaitForever);
	_audio_close_impl();
	osMutexRelease(dma_mutex);
}

void app_audio_driver_init(app_audio_rx_notify_t notify, void *context)
{
    if (!driver_initialized) {
        app_audio_init();
        driver_initialized = true;
    }
    rx_notify = notify;
    rx_notify_context = context;
}

bool app_audio_driver_start(void)
{
    if (driver_running) return true;
    ready_head = ready_tail = 0U;
    driver_running = true;
    app_audio_open();
    return true;
}

bool app_audio_driver_stop(void)
{
    if (!driver_running) return true;
    driver_running = false;
    app_audio_close();
    ready_head = ready_tail = 0U;
    return true;
}

bool app_audio_driver_get_completed(app_audio_rx_block_t *block)
{
    uint8_t slot;
    if (block == NULL || ready_tail == ready_head) return false;
    slot = ready_slots[ready_tail];
    ready_tail = (uint8_t)((ready_tail + 1U) % RX_BUF_BLOCK_NUM);
    block->data = p_ring_buf + ((uint32_t)slot * RX_BUF_BLOCK_SIZE);
    block->length = RX_BUF_BLOCK_SIZE;
    block->slot = slot;
    return true;
}

bool app_audio_driver_release(uint8_t slot)
{
    if (!driver_running || slot >= RX_BUF_BLOCK_NUM) return false;
    return drv_i2s_read_dma(OM_I2S, p_ring_buf + ((uint32_t)slot * RX_BUF_BLOCK_SIZE), RX_BUF_BLOCK_SIZE) == OM_ERROR_OK;
}

uint32_t app_audio_driver_get_drop_count(void)
{
    return dropped_blocks;
}


/** @} */