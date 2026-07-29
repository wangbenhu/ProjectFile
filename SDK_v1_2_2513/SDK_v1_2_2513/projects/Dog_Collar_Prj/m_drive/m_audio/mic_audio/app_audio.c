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
#include "common_def.h"
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

osMutexId_t speaker_mic_mutex;


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


static uint8_t packet_seq;


/// log with debug
#define OM_LOG_DEBUG(format, ...)                  log_debug(format,  ## __VA_ARGS__)
/// log debug array
#define OM_LOG_DEBUG_ARRAY(array, len)             do{int __i; for(__i=0;__i<(len);++__i)OM_LOG_DEBUG("%02X ",((uint8_t *)(array))[__i]);}while(0)

/// log debug array with show more
//#define OM_LOG_DEBUG_ARRAY_EX(note, array, len)    do{OM_LOG_DEBUG("%s: ",note); OM_LOG_DEBUG_ARRAY(array,len); OM_LOG_DEBUG("[%dbytes]\n",len);}while(0)

/*******************************************************************************
 * LOCAL FUNCTIONS
 */

// IIR低通滤波器 (Butterworth 2阶)
#define IIR_ORDER 2
static float iir_b[IIR_ORDER+1] = {0.0201, 0.0402, 0.0201};
static float iir_a[IIR_ORDER+1] = {1.0000, -1.5610, 0.6414};
static float iir_x[IIR_ORDER+1] = {0};
static float iir_y[IIR_ORDER+1] = {0};

float iir_lpf(float input) {
    // 移位历史数据
    for(int i = IIR_ORDER; i > 0; i--) {
        iir_x[i] = iir_x[i-1];
        iir_y[i] = iir_y[i-1];
    }
    iir_x[0] = input;
    
    // 计算输出
    iir_y[0] = iir_b[0] * iir_x[0];
    for(int i = 1; i <= IIR_ORDER; i++) {
        iir_y[0] += iir_b[i] * iir_x[i] - iir_a[i] * iir_y[i];
    }
    
    return iir_y[0];
}

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

uint8_t  data_tmp[1024]={0};

//extern uint32_t m_wave_data_size;
extern uint32_t m_wave_data_start;


// 一阶高通滤波器（Q15定点）
#define ALPHA_Q15 32440 // Q15: 0.99 = 32440/32768

int16_t dc_remove_q15(int16_t sample) {
    static int32_t prev_in = 0, prev_out = 0;
    
    // 计算：out = alpha*prev_out + sample - prev_in
    int32_t out = ((int32_t)ALPHA_Q15 * prev_out) >> 15;
    out += sample;
    out -= prev_in;
    
    // 更新状态
    prev_in = sample;
    prev_out = out;
    
    return (int16_t)__SSAT(out, 16); // 饱和处理
}
int16_t dynamic_boost_q15(int16_t sample) {
    static const int16_t BOOST_THRESH = 1638;  // Q15: 0.05 (-26dBFS)
    static const int16_t MAX_BOOST = 32767;    // Q15: 10倍增益(20dB)
    
    int16_t abs_sample = abs(sample);
    
    if (abs_sample < BOOST_THRESH) {
        // 非线性抬升曲线：低信号增益高
        int32_t ratio = (int32_t)abs_sample * 20 >> 5; // ratio = sample/thresh
        int32_t boost = MAX_BOOST - ratio * 204;      // 增益曲线
        
        // 应用增益并饱和处理
        int32_t result = (int32_t)sample * boost >> 15;
        return __SSAT(result, 16);
    }
    return sample;
}int16_t agc_q15(int16_t sample) {
    static int32_t gain = 32768; // Q15: 1.0 = 32768
    const int16_t TARGET = 24576; // Q15: 0.75 (75%满量程)
    const int16_t AGC_RATE = 163; // Q15: 0.005
    
    // 计算幅度误差
    int16_t abs_sample = abs(sample);
    int16_t error = TARGET - abs_sample;
    
    // 更新增益：gain += AGC_RATE * error
    gain += (int32_t)AGC_RATE * error;
    
    // 增益限幅 (0.1 - 10.0)
    if (gain < 3276) gain = 3276;    // 0.1x
    if (gain > 327680) gain = 327680; // 10.0x
    
    // 应用增益
    int32_t result = (int32_t)sample * (gain >> 8) >> 7; // Q15调整
    return __SSAT(result, 16);
}
// 三频段抬升滤波器 (300Hz-4kHz)
typedef struct {
    int16_t b0, b1, b2, a1, a2;
    int32_t state[2];
} BiquadFilter;

int16_t multiband_boost(int16_t sample, BiquadFilter* filter, int16_t gain) {
    // 直接形式II实现
    int32_t w = sample - ((filter->a1 * filter->state[0]) >> 15) 
                  - ((filter->a2 * filter->state[1]) >> 15);
    
    int32_t out = ((filter->b0 * w) >> 15) 
                + ((filter->b1 * filter->state[0]) >> 15)
                + ((filter->b2 * filter->state[1]) >> 15);
    
    // 更新状态
    filter->state[1] = filter->state[0];
    filter->state[0] = w;
    
    // 应用频段增益
    return __SSAT((out * gain) >> 15, 16);
}
#define M_PI 3.14159265358979323846
// 初始化滤波器系数 (示例：1kHz带通)
void init_bandpass_filter(BiquadFilter* f, int center_freq, int sample_rate) {
    float w0 = 2 * M_PI * center_freq / sample_rate;
    float Q = 1.0;
    float alpha = sin(w0)/(2*Q);
    
    f->b0 = (int16_t)(Q * alpha * 32767);
    f->b1 = 0;
    f->b2 = (int16_t)(-Q * alpha * 32767);
    f->a1 = (int16_t)(-2 * cos(w0) * 32767);
    f->a2 = (int16_t)((1 - alpha) * 32767);
    f->state[0] = f->state[1] = 0;
}// 重点增强1-4kHz语音频段

BiquadFilter speech_filter;
/*
init_bandpass_filter(&speech_filter, 2500, 8000); // 2.5kHz中心
*/
int16_t process_for_asr(int16_t sample) {
    sample = dc_remove_q15(sample);
    sample = multiband_boost(sample, &speech_filter, 18000); // +5dB增益
    return agc_q15(sample);
}

int16_t process_mic_sample(int16_t adc_sample) {
	//BiquadFilter speech_filter;

//BiquadFilter speech_filter;
//init_bandpass_filter(&speech_filter, 2500, 8000); // 2.5kHz中心



    // Stage 1: 信号调理
  // int16_t s1 = dc_remove_q15(adc_sample);
    
    // Stage 2: 动态抬升
    //int16_t s2 = dynamic_boost_q15(adc_sample);
    
    // Stage 3: 自动增益
   // int16_t s3 = agc_q15(s1);
    
	
	//BiquadFilter speech_filter;
	/*init_bandpass_filter(&speech_filter, 2500, 8000); // 2.5kHz中心
	
	int16_t s4 = process_for_asr(s3);*/
	
    // Stage 4: 频段增强 (可选)
   // #ifdef SPEECH_ENHANCE
  //  s3 = multiband_boost(s3, &speech_filter, 18000);
   // #endif
    
   // return process_for_asr(adc_sample);
	 return ((adc_sample)+200);
}

__RAM_CODE void i2s_external_rx_loop_isr_cb(void *i2s_reg, drv_event_t event, void *addr, void *size)
{
	osKernelLock();  // 或 __disable_irq()

    drv_i2s_read_dma(OM_I2S, p_ring_buf + rx_block_idx * RX_BUF_BLOCK_SIZE, RX_BUF_BLOCK_SIZE);
	
	
	//drv_gpio_toggle(OM_GPIO0, GPIO_MASK(PIN_MIC_Audio_TEST));
//	uint8_t eee=0;
//	int16_t tmp=0,tmo1=0;
//	drv_gpio_write(OM_GPIO0, GPIO_MASK(PIN_MIC_Audio_TEST),GPIO_LEVEL_HIGH);
drv_uart_write(OM_UART1,p_ring_buf + rx_block_idx * RX_BUF_BLOCK_SIZE,RX_BUF_BLOCK_SIZE,10);

	 // eee = drv_flash_write(OM_FLASH1, 0x50000+m_wave_data_size, p_ring_buf + rx_block_idx * RX_BUF_BLOCK_SIZE, RX_BUF_BLOCK_SIZE);
	drv_gpio_write(OM_GPIO0, GPIO_MASK(PIN_MIC_Audio_TEST),GPIO_LEVEL_LOW);
	//om_printf("eee = %d\r\n",eee);
	//drv_flash_read(OM_FLASH1,  0x50000+m_wave_data_size, data_tmp, RX_BUF_BLOCK_SIZE);
	
	//drv_uart_write(OM_UART1,data_tmp,RX_BUF_BLOCK_SIZE,10);
//	m_wave_data_size+=RX_BUF_BLOCK_SIZE;
	 //  uint16_t len =(size);
	 //uint16_t hhhjjj = (size);
//	om_log(OM_LOG_INFO, "sbc = %d %d\r\n",*len,hhhjjj);
//	memset(data_tmp,0,1024);
//	
//	memcpy(data_tmp,addr,len);
//	for(int i=0;i<len;i++)
//	{
//		data_tmp[0] = (uint8_t)iir_lpf(data_tmp[0]);
//	}
	
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
	osKernelUnlock();  // 或 __enable_irq()

}
//#pragma arm_section
/*******************************************************************************
 * PUBLIC FUNCTIONS
 */

void app_audio_init(void)
{
	    // 创建互斥锁
    const osMutexAttr_t mutex_attr = { .name = "dma_mutex" };
    dma_mutex = osMutexNew(&mutex_attr);
	
	const osMutexAttr_t speaker_mic_mutex = { .name = "speaker_mic_mutex" };
    dma_mutex = osMutexNew(&speaker_mic_mutex);
	
	osMutexAcquire(dma_mutex, osWaitForever);   // 加锁
	
    drv_pin_init(pin_cfg, sizeof(pin_cfg) / sizeof(pin_cfg[0]));

    om_fifo_init(&decode_fifo,decode_buffer,DECODE_BUF_LEN);

    codec_init();
	
	osMutexRelease(dma_mutex);        
}

void app_audio_uinit(void){};

void app_audio_open(void)
{	    
	osMutexAcquire(dma_mutex, osWaitForever);   // 加锁
	test_pin_init_audio();
    //音量过大/过小，优先调整volume/gain_pga > gain_adc/gain_cic
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

    /// Create chain and start
    for(rx_block_idx = 0; rx_block_idx < RX_BUF_CHAIN_NUM; rx_block_idx++) {
        drv_i2s_read_dma(OM_I2S, p_ring_buf + rx_block_idx * RX_BUF_BLOCK_SIZE, RX_BUF_BLOCK_SIZE);
    }
    
    om_log(OM_LOG_INFO, "app_audio_open\n");
	osMutexRelease(dma_mutex);                  // 解锁

	osMutexAcquire(speaker_mic_mutex, osWaitForever);   // 加锁
	
 //   pm_sleep_prevent(PM_ID_AUDIO);
}

void app_audio_close(void)
{
	osMutexAcquire(dma_mutex, osWaitForever);   // 加锁
    /// Uninit Audio
    drv_audio_record_stop();
    drv_audio_uninit();

    /// Uninit I2S
    drv_i2s_uninit(OM_I2S);
  //  pm_sleep_allow(PM_ID_AUDIO);
	osMutexRelease(dma_mutex);                  // 解锁
	
	osMutexRelease(speaker_mic_mutex);                  // 解锁
}
static void audio_thread(void *arguments)
{
	app_audio_init();
	app_audio_open();
	while(1)
	{
		
	}
}

void vStarAudioTask(void)
{
    const osThreadAttr_t audioThreadAttr = {
        .name           = "app_audio",
        .stack_size     = 10*2048U,
        .priority       = osPriorityRealtime,
    };

    // Create om24g Task
    osThreadNew(audio_thread, NULL, &audioThreadAttr);
}


/** @} */