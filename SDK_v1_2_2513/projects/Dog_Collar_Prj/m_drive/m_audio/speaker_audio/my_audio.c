#include <stdlib.h>  // standard lib functions
#include <stddef.h>  // standard definitions
#include <stdint.h>  // standard integer definition
#include <stdbool.h> // boolean definition
#include <stdio.h>
#include <string.h>

#include "evt.h"
#include "evt_timer.h"
#include "om_driver.h"
#include "om_log.h"

#include "my_audio.h"

#include "cmsis_os2.h"
#include "board_define.h"
#include "test_audio.h"
#include "lfs_port.h"
#include "math.h"
#include "common_def.h"

#define MAX_FILENAME_LEN 128

#define AUDIO_EN_LEVEL_HIGH				(1)
#define AUDIO_EN_LEVEL_LOW				(0)

#define MY_AUDIO_ENABLE_LEVEL AUDIO_EN_LEVEL_HIGH//
#define SMOOTH_SHIFT 2   // 1=强平滑，2=推荐，3=更清晰

static char current_audio_file[MAX_FILENAME_LEN];

static const pin_config_t audio_pin_config[] = {
	{AUDIO_PLAY_IO, 		{PINMUX_GPIO_MODE_CFG}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_MAX},
};
static pwm_dma_env_t pwm_dma_env;

static volatile bool g_audio_play_done = true;  // 播放完成标志

static uint8_t repet_play_flag = 0; 
	
int8_t check_wave_file(const char *filename);
uint8_t  Audio_Play_Start(const char *filename,uint8_t audio_reset);

tim_pwm_output_config_t audio_pwm_config = {
	.cnt_freq = PWM_Clock_Hz,
	.period_cnt = PWM_Period_Cnt,
	.dead_time = 0,  // 增加死区时间，减少开关噪声
	.chan = {
		{0, {TIM_PWM_POL_ACTIVE_LOW, 0}},
		{0, {TIM_PWM_POL_ACTIVE_HIGH, 0}},
		{0, {TIM_PWM_POL_ACTIVE_HIGH, 0}},
		{0, {TIM_PWM_POL_ACTIVE_HIGH, 0}}
	},
	.gpdma_cfg = {
		.en = 1,
		.tim_chan = TIM_CHAN_1,
		.chain = &pwm_dma_env.chain[0]
	},
};

// 播放时间(秒) = 总样本数 / 采样率
// 总样本数 = 数据字节数 / (位深度 / 8)
float calculate_playback_time(uint32_t data_size_bytes, 
                             uint32_t sample_rate, 
                             uint16_t bit_depth) {
    // 计算总样本数
    float total_samples = (float)data_size_bytes / (bit_depth / 8.0f);
    
    // 计算播放时间(秒)
    float playback_time_seconds = total_samples / sample_rate;
    
    return playback_time_seconds;
}
#if (AUDIO_FORMAT_DEFALT)

int err;
static uint8_t audio_status_flag=0;
unsigned char pcm_bytes[MAX_FRAME_SIZE * CHANNELS];
int frame_size;
size_t samples;
#define PWM_MID        (PWM_0_OFFSET-8)// (PWM_MAX / 2)           // 2048
#define AUDIO_PLAY_NUM_MAX 5

uint8_t buffer_block_index = 3;

__ALIGNED(4) int16_t audio_pcm_buffer[BUFFER_BLOCK_SIZE / 2]; // 这个是接收到的音频数据经过处理后得到的PWM输出缓冲区。

uint32_t m_wave_data_size = 0;
uint32_t m_wave_data_start = 0;

uint32_t Voice_frame_counter = 0;

gpdma_chain_trans_t pwm_dma_chain[BUFFER_BLOCK_NUM];

static float audio_volume = 1.0f;

static uint8_t audio_start_num = 0;

							
static int16_t prev_sample = 0;
static int16_t prev = 0;

static int16_t prev_out = PWM_MID;

int8_t check_wave_file(const char *filename);


FlashSel_T iflash_or_exflash_select = FLASH_INT;  // 直接用枚举类型声明
AudioAddr_T audio_iflash_addr_select = IFLASH_AUDIO_1;
// 获取当前音频地址枚举
AudioAddr_T get_audio_addr(void)
{
    return audio_iflash_addr_select;
}


/**
* @brief 设置停止播放标志
* */
void StopPlayFlagSet(void)
{
	audio_start_num = AUDIO_PLAY_NUM_MAX;
}
/**
 * @brief 获取重复播放标志
 * @return: uint8_t: 重复播放标志
 */
uint8_t RepetFlagGet(void)
{
	return repet_play_flag;
}/*
* @brief 设置重复播放标志
* */
void RepetFlagSet(void)
{
	repet_play_flag = 1;
}
/*
* @brief 清除重复播放标志
* */
void RepetFlagClear(void)
{
	repet_play_flag=0;
}/*
* @brief 启用音频
* */

// 通过索引设置音频地址
// 参数 index: 0-4 对应 AUDIO_1 到 AUDIO_5
// 返回值: 0-成功，-1-失败
int set_audio_addr(uint8_t index)
{
    // 根据索引设置对应的地址
    switch(index) {
        case 1:
            audio_iflash_addr_select = IFLASH_AUDIO_1;
            break;
        case 2:
            audio_iflash_addr_select = IFLASH_AUDIO_2;
            break;
        case 3:
            audio_iflash_addr_select = IFLASH_AUDIO_3;
            break;
        case 4:
            audio_iflash_addr_select = IFLASH_AUDIO_4;
            break;
        case 5:
            audio_iflash_addr_select = IFLASH_AUDIO_5;
            break;
        default:
            // 无效索引，返回错误
            return -1;
    }
    
    return 0;  // 设置成功
}
// 获取当前音频地址对应的索引
// 返回值: 0-4 对应 AUDIO_1 到 AUDIO_5，0xFF-无效
uint8_t get_audio_addr_index(void)
{
    switch(audio_iflash_addr_select) {
        case IFLASH_AUDIO_1:
            return 1;
        case IFLASH_AUDIO_2:
            return 2;
        case IFLASH_AUDIO_3:
            return 3;
        case IFLASH_AUDIO_4:
            return 4;
        case IFLASH_AUDIO_5:
            return 5;
        default:
            return 0xFF;  // 无效地址
    }
}
// 直接获取地址数值（方便用于指针操作）
uint32_t get_audio_addr_value(void)
{
    return (uint32_t)audio_iflash_addr_select;
}

// 验证地址是否有效
int is_valid_audio_addr(AudioAddr_T addr)
{
    switch(addr) {
        case IFLASH_AUDIO_1:
        case IFLASH_AUDIO_2:
        case IFLASH_AUDIO_3:
        case IFLASH_AUDIO_4:
        case IFLASH_AUDIO_5:
            return 1;  // 有效
        default:
            return 0;  // 无效
    }
}

// 获取当前Flash选择
FlashSel_T get_flash_select(void)
{
    return iflash_or_exflash_select;
}

// 设置Flash选择
void set_flash_select(FlashSel_T sel)
{
    // 简单的参数校验
    if (sel == FLASH_INT || sel == FLASH_EXT) {
        iflash_or_exflash_select = sel;
    }
    // 可选：参数错误时保持原值或设置为默认值
}


void Audio_PCM_Data_Update(uint8_t *pSrc, int16_t nBytes)
{
    int16_t *pdes = audio_pcm_buffer;
    uint8_t *psrc = pSrc;

    int samples_8k = nBytes;   // 8bit: 1 byte = 1 sample

    for (int i = 0; i < samples_8k; i++)
    {

				// 8bit PCM -> PWM
		int16_t s = (int16_t)psrc[i] - 128;

		// PCM 域音量（0.0 ~ 1.2 最多）
		//s = (int16_t)(s * audio_volume);
// 直接放大 2 倍
		s <<= 1;
		// 再映射到 PWM
		s = s * (PWM_0_OFFSET - 2) / 128;

		// ★ 关键：加极小抖动（±1 LSB）★
		//s += (rand() & 1) ? 1 : -1;

		int16_t out = PWM_0_OFFSET + s;

		// 限幅
		if (out < 0) out = 0;
		if (out > PWM_Period_Cnt) out = PWM_Period_Cnt;

		// 重复输出
		*pdes++ = out;
		*pdes++ = out;
    }

    Audio_push_to_RingBuffer(
        (uint8_t *)audio_pcm_buffer,
        samples_8k * 2 * sizeof(int16_t));

}

#define AUDIO_REFILL_THRESHOLD  (BUFFER_BLOCK_NUM / 2)

#define READ_FRAMES_PER_CALL  2  // 或 3
void audio_evt_callback(void)
{
	int i;
	// log_debug( "audio_evt_callback = %d %d %d %d\r\n",g_audio_play_done,audio_start_num,Voice_frame_counter,m_wave_data_size);
	if (g_audio_play_done)
        return;	
//	log_debug( "get_flash_select() = %d\r\n",get_flash_select());
	switch(get_flash_select())
	{
		case FLASH_INT:  // 内部Flash
			drv_flash_read(OM_FLASH0,  m_wave_data_start + Voice_frame_counter, (uint8_t *)pcm_bytes, FRAME_SIZE_8K);
			break;
		case FLASH_EXT:   // 外部Flash
			lfs_read_file_offset(current_audio_file,(uint8_t *)pcm_bytes,m_wave_data_start + Voice_frame_counter,FRAME_SIZE_8K,1);
			break;	
		default:
			break;
	}
//
//	memcpy(pcm_bytes,&audio_data[m_wave_data_start + Voice_frame_counter],FRAME_SIZE_8K);	
	
	Voice_frame_counter += FRAME_SIZE_8K;
	if (Voice_frame_counter >= m_wave_data_size)
	{
		Voice_frame_counter = 0;
        g_audio_play_done = true; // 标记播放结束
		prev_out = PWM_MID;   // ★ 防止下一次接着旧值
		audio_start_num++;
		if(audio_start_num < AUDIO_PLAY_NUM_MAX)
		{
			if(repet_play_flag)//触发标志
				audio_start_num = 0;
			char file_name[MAX_FILENAME_LEN] ={0};
//			log_debug( "get_flash_select 333333= %d\r\n",get_flash_select());
			switch(get_flash_select())
			{
				case FLASH_INT:  // 内部Flash
					Audio_IFlash_Play_Start(get_audio_addr_index(),0);
				
					break;
				case FLASH_EXT:   // 外部Flash
					memcpy(file_name,current_audio_file,strlen(current_audio_file));
					Audio_Play_Start(file_name,0);
			
				break;	
				default:
					break;				
			}
		}
		else
		{
			
			audio_start_num=0;
			audio_status_flag = 1;
			PWM_Audio_Stop();   // ★ 核心：彻底停 DMA + EV
			
//			lfs_unmount_safe();	
			return;
		}
       // log_debug( "Audio playback finished, will stop PWM\n");
        return; // 不再推送数据	
	}
	
	Audio_PCM_Data_Update(pcm_bytes, FRAME_SIZE_8K);

}

// 拷贝数据到对应的PWM播放缓冲区，分成4块，循环播放。
void Audio_push_to_RingBuffer(uint8_t *audio_pcm_buffer, int nBytes)
{
  memcpy(&pwm_dma_env.buf[buffer_block_index * BUFFER_BLOCK_SIZE / 2],audio_pcm_buffer,nBytes);

    if (++buffer_block_index >= BUFFER_BLOCK_NUM)
        buffer_block_index = 0;
}
extern void Audio_Play_DMA_Set(void);
void pwm_timer_dma_cb(void *om_reg, drv_event_t event, void *param0, void *param1)
{
	if(event == DRV_EVENT_TIM_GPDMA_COMPLETE)
	{
		if (g_audio_play_done)
		{
			Audio_Play_Complete_Notify();
			//log_debug( "PWM audio stopped safely\n");
			return;   // ? 防止多次进入
		}
	//audio_evt_callback();
		 Audio_Play_DMA_Set();
	}
}

static void AudioHalTimer_Init(void)
{
	for(uint8_t i = 0; i < BUFFER_BLOCK_NUM; i ++)
	{
		pwm_dma_env.chain[i].src_addr   = (uint32_t)&pwm_dma_env.buf[i * BUFFER_BLOCK_SIZE / 2];
		pwm_dma_env.chain[i].size_byte  = (BUFFER_BLOCK_SIZE / 2) * sizeof(uint16_t);
		pwm_dma_env.chain[i].ll_ptr     = &pwm_dma_env.chain[(i + 1) % BUFFER_BLOCK_NUM];
	}
	
	audio_pwm_config.chan[TIM_CHAN_1].en  = 1;
	audio_pwm_config.chan[TIM_CHAN_1].cfg.complementary_output_enable = true;
	audio_pwm_config.dead_time = 0;

	drv_tim_init(PWM_TIMER);
	drv_tim_gpdma_channel_allocate(PWM_TIMER);
	NVIC_SetPriority(TIM2_IRQn, 1);
	drv_tim_register_isr_callback(PWM_TIMER, pwm_timer_dma_cb);
	//drv_tim_pwm_output_start(PWM_TIMER, &audio_pwm_config);
	
	register_set(&PWM_TIMER->RCR, MASK_1REG(TIM_RCR_REP, DMA_REPEAT_CNT));
}

void AudioHal_Init(void)
{
	/* Timer初始化配置 */
	AudioHalTimer_Init();
	
}
void fill_pwm_buffer_with_silence(void)
{
    for (int i = 0; i < BUFFER_BLOCK_NUM * (BUFFER_BLOCK_SIZE / 2); i++) {
        pwm_dma_env.buf[i] = PWM_MID;
    }
}
	
void AudioHal_Drive_DeInit(void)
{
	//关闭pwm播放
	drv_tim_pwm_output_stop(PWM_TIMER, TIM_CHAN_1);

	drv_tim_gpdma_channel_release(PWM_TIMER);
	Audio_Play_SD_Disable();
	drv_tim_uninit(PWM_TIMER);
	
	drv_pin_init(audio_pin_config, sizeof(audio_pin_config) / sizeof(audio_pin_config[0]));
	
	drv_gpio_write(OM_GPIO0, GPIO_MASK(AUDIO_PLAY_IO), GPIO_LEVEL_LOW); // 8002的SD脚为低电平的时候可以输出声音，为高的时候不能输出声音
	
}
void AudioHal_DeInit(void)
{
    // 停止PWM输出
    drv_tim_pwm_output_stop(PWM_TIMER, TIM_CHAN_1);

    // 禁用GPDMA
	drv_tim_gpdma_channel_release(PWM_TIMER);

    // 清空DMA链表
    memset(&pwm_dma_env, 0, sizeof(pwm_dma_env));

    // 清除NVIC挂起中断标志，防止假触发
    NVIC_ClearPendingIRQ(TIM2_IRQn);

//    log_debug( "AudioHal_DeInit: PWM and DMA deinitialized\n");
}


uint8_t Audio_Play_Start(const char *filename,uint8_t audio_reset)
{
	   if (filename == NULL) {
        return REPORT_AUDIO_ERROR;
    }
	if(!g_audio_play_done)
	{
		return REPORT_AUDIO_ERROR;
	}
	audio_status_flag=0;
	
	
    size_t filename_len = strlen(filename);
    
    // 检查文件名长度是否合法
    if (filename_len == 0 || filename_len >= MAX_FILENAME_LEN) {
        // 文件名太长或为空，清空并返回
        current_audio_file[0] = '\0';
        return REPORT_AUDIO_ERROR;
    }   
	  if(lfs_mount_safe() < 0 )
        return REPORT_AUDIO_ERROR;
 // 如果当前正在播放（g_audio_play_done==false），先停止当前播放，避免pop/click
    if (!g_audio_play_done) {
//        // 立即停止并释放所有硬件资源（同步）
        PWM_Audio_Stop();
		osDelay(20);
//        // 给硬件一点稳态时间，等待 DMA/TIMER 真实停止
    }	
	
	set_flash_select(FLASH_EXT);//标记外部flash
	
	memset(current_audio_file,0,sizeof(current_audio_file));
	memcpy(current_audio_file,filename,filename_len);
	if(1 == check_wave_file(current_audio_file))
	{

		log_debug("Audio_Play_Start\n");
		// 关闭功放
		Audio_Play_SD_Disable();
       // drv_gpio_write(OM_GPIO0, GPIO_MASK(AUDIO_SD_IO), GPIO_LEVEL_HIGH);
        osDelay(5);
		 AudioHal_DeInit();   // ? 新增
		 prev_out = PWM_MID;      // ★ 必须
		fill_pwm_buffer_with_silence();
		// 先把 dma buffer 内存整体置为静音（按 16-bit）
		g_audio_play_done = false;
		Voice_frame_counter = 0;
		buffer_block_index = 3;
		AudioHal_Init();
		drv_tim_pwm_output_start(PWM_TIMER, &audio_pwm_config);
		osDelay(20);
		Audio_Play_SD_Enable();
		if(audio_reset)
		{
			audio_start_num = 0;
		}
		//drv_gpio_write(OM_GPIO0, GPIO_MASK(AUDIO_SD_IO), GPIO_LEVEL_LOW); 
		// 8002的SD脚为低电平的时候可以输出声音，为高的时候不能输出声音
	}
	else
	{
	}
//	log_debug( "******** = %d \r\n",drv_rcc_clock_get(RCC_CLK_TIM2));
	return REPORT_OK;
}

void PWM_Audio_Play_Init(void)
{

	RepetFlagClear();//初始化重复播放标志位
	
    buffer_block_index = 3;
    
    // 初始化DMA缓冲区为静音（中间值）
    memset(pwm_dma_env.buf, PWM_MID, sizeof(pwm_dma_env.buf));
	
	AudioHal_Init();
}
uint8_t Audio_Status_Get(void)
{
	return audio_status_flag;
}
void PWM_Audio_Stop(void)
{
	/*
	 * First drive a complete DMA block at the true zero-signal duty cycle.
	 * At 64 kHz one block lasts 20 ms; stopping after only 5 ms can cut an
	 * active waveform and cause a pop at the amplifier input.
	 */
	fill_pwm_buffer_with_silence();
	osDelay(25);
	Audio_Play_SD_Disable();
	osDelay(5);
    drv_tim_pwm_output_stop(PWM_TIMER, TIM_CHAN_1);

	 prev_out = PWM_MID;      // ★ 必须

    // 释放/关闭 DMA 通道
    drv_tim_gpdma_channel_release(PWM_TIMER);

    // 清除 NVIC 挂起，避免假中断
    NVIC_ClearPendingIRQ(TIM2_IRQn);

    // 重置一些状态
    g_audio_play_done = true;
    buffer_block_index = 3;

    log_debug("PWM_Audio_Stop\r\n");
}

int8_t check_wave_file(const char *filename)
{
	int8_t file_is_ok = 1;
	uint8_t flash_buffer[44];
	wave_chunks_t *p_chunk;
	wave_chunk2_t *p_chunk2;

	if(lfs_read_file(filename,flash_buffer,sizeof(flash_buffer),1)<=0)
	{
		log_debug( "lfs_read_file Fail = %s\r\n",filename);
		return false;
	}
	//memcpy(flash_buffer,audio_data,sizeof(flash_buffer));
	//drv_flash_read(OM_FLASH0, FILE_FLASH_ADDRESS, flash_buffer, sizeof(flash_buffer));
//		for(int i=0;i<44;i++)
//	{
//		 log_debug( "%#x ",flash_buffer[i]);
//	} log_debug( "\r\n");
	  p_chunk = (wave_chunks_t *)flash_buffer;  
		
	  if(p_chunk->ChunkID != 0x46464952)       // RIFF
		file_is_ok = -1;
		
	  if(p_chunk->Format != 0x45564157)        // WAVE
		file_is_ok = -2;
	  if(p_chunk->SubChunk1ID != 0x20746D66)   // Fmt 
		file_is_ok = -3;
	  if((p_chunk->SubChunk1Size != 16)&&
		 (p_chunk->SubChunk1Size != 18)&&
		 (p_chunk->SubChunk1Size != 40))
	  {
		file_is_ok = -4;
	  }
	//  if(p_chunk->AudioFormat != 0x0001)       // PCM
	//    file_is_ok = -5;
	//  if(p_chunk->NumChannels != 0x0001)       // mono
	//    file_is_ok = -6;
	//  if(p_chunk->SampleRate != 0x00003E80)    // 16k
	//    file_is_ok = -7; 
	//  if(p_chunk->ByteRate != 0x00003E80)      // sampleRate*numChannels*BitsPerSample/8
	//    file_is_ok = -8; 
	//  if(p_chunk->BlockAlign != 0x0001)        // NumChannels * BitsPerSample/8
	//    file_is_ok = -9;
	  if(p_chunk->BitPerSample != 0x0008)      // 16bit
		file_is_ok = -10;  

	  p_chunk2 = (wave_chunk2_t *)((uint32_t)&p_chunk->SubChunk1ID + 8 + p_chunk->SubChunk1Size);
	  
	//  if(p_chunk2->id != 0x5453494c)   // data
	//    file_is_ok = -11;  
		if(file_is_ok ==1)
		{
			m_wave_data_size = p_chunk2->size;
			//  m_wave_data_start = FILE_FLASH_ADDRESS + 28 + p_chunk->SubChunk1Size;
			m_wave_data_start = 0 + 28 + p_chunk->SubChunk1Size;
			log_debug( "wave size:0x%08X\r\n", m_wave_data_size);
			log_debug( "wave start:0x%08X\r\n", m_wave_data_start);
			//log_debug( "file is OK= %d %d %d \r\n",m_wave_data_start,p_chunk->SubChunk1Size,m_wave_data_size);
		}
		else
		{
			log_debug( "file is fail:%d\r\n", file_is_ok);
		}

	  return file_is_ok;
}
int8_t check_iflash_wave_file(void)
{
	int8_t file_is_ok = 1;
	uint8_t flash_buffer[44];
	wave_chunks_t *p_chunk;
	wave_chunk2_t *p_chunk2;
	uint32_t iflash_read_addr = get_audio_addr_value();
	
	//log_debug( "iflash_read_addr = %x\r\n",iflash_read_addr);

	drv_flash_read(OM_FLASH0,iflash_read_addr , flash_buffer, sizeof(flash_buffer));
//	for(int i=0;i<44;i++)
//	{
//		 log_debug( "%#x ",flash_buffer[i]);
//	} 
//	log_debug( "\r\n");
	  p_chunk = (wave_chunks_t *)flash_buffer;  
		
	  if(p_chunk->ChunkID != 0x46464952)       // RIFF
		file_is_ok = -1;
		
	  if(p_chunk->Format != 0x45564157)        // WAVE
		file_is_ok = -2;
	  if(p_chunk->SubChunk1ID != 0x20746D66)   // Fmt 
		file_is_ok = -3;
	  if((p_chunk->SubChunk1Size != 16)&&
		 (p_chunk->SubChunk1Size != 18)&&
		 (p_chunk->SubChunk1Size != 40))
	  {
		file_is_ok = -4;
	  }

	  if(p_chunk->BitPerSample != 0x0008)      // 8bit
		file_is_ok = -10;  

		p_chunk2 = (wave_chunk2_t *)((uint32_t)&p_chunk->SubChunk1ID + 8 + p_chunk->SubChunk1Size);
	
		if(file_is_ok ==1)
		{
			m_wave_data_size = p_chunk2->size;
			if(m_wave_data_size > 28672)
			{
				file_is_ok = -5;
			}
			else
			{
				m_wave_data_start = iflash_read_addr + 28 + p_chunk->SubChunk1Size;
				log_debug( "wave size:0x%08X\r\n", m_wave_data_size);
				log_debug( "wave start:0x%08X\r\n", m_wave_data_start);
		
			//		 m_wave_data_start = 0 + 28 + p_chunk->SubChunk1Size;
			//	log_debug( "file is OK= %d %d %d \r\n",m_wave_data_start,p_chunk->SubChunk1Size,m_wave_data_size);
			}
		}
		else
		{
			log_debug( "file is fail:%d\r\n", file_is_ok);
		}
	  return file_is_ok;
}
uint8_t Audio_IFlash_Play_Start(uint8_t index,uint8_t audio_reset)
{
	log_debug("Audio_IFlash_Play_Start = %d %d %d\r\n",index,audio_reset,g_audio_play_done);
	if(index <= 0 || index > 5)
	{
	  return REPORT_PLAY_AUDIO_INDEX_ERROR;
	}
	if(!g_audio_play_done)
	{
		return REPORT_AUDIO_ERROR;
	}
    if (!g_audio_play_done) {
//        // 立即停止并释放所有硬件资源（同步）
        PWM_Audio_Stop();
		osDelay(20);
//        // 给硬件一点稳态时间，等待 DMA/TIMER 真实停止
    }	
	set_flash_select(FLASH_INT);//标记内部flash
	if(set_audio_addr(index) < 0)
	{
		return REPORT_PLAY_AUDIO_INDEX_ERROR;
	}
	if(1 == check_iflash_wave_file())
	{
		log_debug("Audio_IFlash_Play_Start\r\n");
		// 关闭功放
		Audio_Play_SD_Disable();
       // drv_gpio_write(OM_GPIO0, GPIO_MASK(AUDIO_SD_IO), GPIO_LEVEL_HIGH);
        osDelay(5);
		 AudioHal_DeInit();   // ? 新增
		 prev_out = PWM_MID;      // ★ 必须
		fill_pwm_buffer_with_silence();
		// 先把 dma buffer 内存整体置为静音（按 16-bit）
		g_audio_play_done = false;
		Voice_frame_counter = 0;
		buffer_block_index = 3;
		AudioHal_Init();
		drv_tim_pwm_output_start(PWM_TIMER, &audio_pwm_config);
		osDelay(20);
		Audio_Play_SD_Enable();
		if(audio_reset)
		{
			audio_start_num = 0;
		}
		
		//drv_gpio_write(OM_GPIO0, GPIO_MASK(AUDIO_SD_IO), GPIO_LEVEL_LOW); 
		// 8002的SD脚为低电平的时候可以输出声音，为高的时候不能输出声音
	}
	else
	{
		
	}
	return REPORT_OK;
}
#else
int err;
static uint8_t audio_status_flag=0;


unsigned char pcm_bytes[MAX_FRAME_SIZE * CHANNELS];
int frame_size;
size_t samples;
#define PWM_BITS        12
#define PWM_MAX         ((1 << PWM_BITS) - 1)   // 4095
#define PWM_MID        2000// (PWM_MAX / 2)           // 2048
#define AUDIO_PLAY_NUM_MAX 5

uint8_t buffer_block_index = 3;

__ALIGNED(4) int16_t audio_pcm_buffer[BUFFER_BLOCK_SIZE / 2]; // 这个是接收到的音频数据经过处理后得到的PWM输出缓冲区。

uint32_t m_wave_data_size = 0;
uint32_t m_wave_data_start = 0;

uint32_t Voice_frame_counter = 0;

gpdma_chain_trans_t pwm_dma_chain[BUFFER_BLOCK_NUM];

static float audio_volume = 1.0f;

static uint8_t audio_start_num = 0;


// 方案2：固定值（简化）
#define FADE_SAMPLES 160  // 16kHz × 0.02s = 320 samples
// 音量控制（0.0 ~ 1.0）
void audio_apply_volume(int16_t* data, uint32_t length, float volume)
{
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    
    for (uint32_t i = 0; i < length; i++) {
        int32_t sample = (int32_t)(data[i] * volume);
        
        // 限制在16bit范围内
        if (sample > 32767) sample = 32767;
        if (sample < -32768) sample = -32768;
        
        data[i] = (int16_t)sample;
    }
}
			 
static int16_t prev_sample = 0;
void Audio_PCM_Data_Update(uint8_t *pSrc, int16_t nBytes)
{
	int16_t *pdes = audio_pcm_buffer;
	int16_t *psrc = (int16_t *)pSrc;
	int16_t i;
	int16_t temp;
	
	for (int i = 0; i < BUFFER_BLOCK_SIZE / 2; i++)
	{
		int32_t temp = *psrc;
		
		// 缩放处理
		temp = (temp * 1) / 32;
		
		// 抗锯齿滤波：与前一样本平均
		temp = (prev_sample + temp) / 2;
		prev_sample = temp;
		
		temp += PWM_0_OFFSET;
		
		// 范围限制
		if (temp > 32767) temp = 32767;
		else if (temp < -32768) temp = -32768;
		
		*pdes = (int16_t)temp;
		pdes++;
		psrc++;
	}

	audio_apply_volume(audio_pcm_buffer,BUFFER_BLOCK_SIZE,audio_volume);
//	apply_fade_out_fixed(audio_pcm_buffer,BUFFER_BLOCK_SIZE);
	// 处理好的数据放入对应的PWM播放数据块
	Audio_push_to_RingBuffer((uint8_t *)audio_pcm_buffer, BUFFER_BLOCK_SIZE);

}

void audio_evt_callback(void)
{
	int i;
	   if (g_audio_play_done)
        return;
//drv_flash_read(OM_FLASH0,  m_wave_data_start + Voice_frame_counter, (uint8_t *)pcm_bytes, BUFFER_BLOCK_SIZE);
//	memcpy(pcm_bytes,&audio_data[m_wave_data_start + Voice_frame_counter],BUFFER_BLOCK_SIZE);	
	lfs_read_file_offset(current_audio_file,(uint8_t *)pcm_bytes,m_wave_data_start + Voice_frame_counter,BUFFER_BLOCK_SIZE,1);
	Voice_frame_counter += BUFFER_BLOCK_SIZE;
	if (Voice_frame_counter >= m_wave_data_size)
	{
		
		Voice_frame_counter = 0;
        g_audio_play_done = true; // 标记播放结束
		audio_start_num++;
		if(audio_start_num<5)
		{
			char file_name[MAX_FILENAME_LEN] ={0};
			memcpy(file_name,current_audio_file,strlen(current_audio_file));
			Audio_Play_Start(file_name);
		}
		else
		{
			audio_start_num=0;
			audio_status_flag = 1;
		}
        log_debug( "Audio playback finished, will stop PWM\n");
        return; // 不再推送数据	
	}	
	Audio_PCM_Data_Update(pcm_bytes, BUFFER_BLOCK_SIZE);

}

// 拷贝数据到对应的PWM播放缓冲区，分成4块，循环播放。
void Audio_push_to_RingBuffer(uint8_t *audio_pcm_buffer, int nBytes)
{
	// copy pcm to pwm dma buffer
	memcpy((uint8_t *)&pwm_dma_env.buf[buffer_block_index * BUFFER_BLOCK_SIZE / 2], audio_pcm_buffer, BUFFER_BLOCK_SIZE);

	if (++ buffer_block_index >= BUFFER_BLOCK_NUM)
		buffer_block_index = 0;
}
extern void Audio_Play_DMA_Set(void);
void pwm_timer_dma_cb(void *om_reg, drv_event_t event, void *param0, void *param1)
{
	if(event == DRV_EVENT_TIM_GPDMA_COMPLETE)
	{
		if (g_audio_play_done)
		{
			Audio_Play_Complete_Notify();
			//drv_gpio_write(OM_GPIO0, GPIO_MASK(AUDIO_SD_IO), GPIO_LEVEL_HIGH); // 8002的SD脚为低电平的时候可以输出声音，为高的时候不能输出声音
			log_debug( "PWM audio stopped safely\n");
			return;   // ? 防止多次进入
		}
	
		 Audio_Play_DMA_Set();
	}
}

static void AudioHalTimer_Init(void)
{
	for(uint8_t i = 0; i < BUFFER_BLOCK_NUM; i ++)
	{
		pwm_dma_env.chain[i].src_addr   = (uint32_t)&pwm_dma_env.buf[i * BUFFER_BLOCK_SIZE / 2];
		pwm_dma_env.chain[i].size_byte  = (BUFFER_BLOCK_SIZE / 2) * sizeof(uint16_t);
		pwm_dma_env.chain[i].ll_ptr     = &pwm_dma_env.chain[(i + 1) % BUFFER_BLOCK_NUM];
	}
	
	audio_pwm_config.chan[TIM_CHAN_1].en  = 1;
	audio_pwm_config.chan[TIM_CHAN_1].cfg.complementary_output_enable = true;
	audio_pwm_config.dead_time = 3;

	drv_tim_init(PWM_TIMER);
	drv_tim_gpdma_channel_allocate(PWM_TIMER);
	NVIC_SetPriority(TIM2_IRQn, 1);
	drv_tim_register_isr_callback(PWM_TIMER, pwm_timer_dma_cb);
	//drv_tim_pwm_output_start(PWM_TIMER, &audio_pwm_config);
	
	register_set(&PWM_TIMER->RCR, MASK_1REG(TIM_RCR_REP, DMA_REPEAT_CNT));
}


void AudioHal_Init(void)
{
	/* Timer初始化配置 */
	AudioHalTimer_Init();
	
}
void fill_pwm_buffer_with_silence(void)
{
    for (int i = 0; i < BUFFER_BLOCK_NUM * (BUFFER_BLOCK_SIZE / 2); i++) {
        pwm_dma_env.buf[i] = PWM_MID;
    }
}
	
void AudioHal_Drive_DeInit(void)
{
	//关闭pwm播放
	drv_tim_pwm_output_stop(PWM_TIMER, TIM_CHAN_1);

	drv_tim_gpdma_channel_release(PWM_TIMER);
	Audio_Play_SD_Disable();
	drv_tim_uninit(PWM_TIMER);
	
	drv_pin_init(audio_pin_config, sizeof(audio_pin_config) / sizeof(audio_pin_config[0]));
	
	drv_gpio_write(OM_GPIO0, GPIO_MASK(AUDIO_PLAY_IO), GPIO_LEVEL_LOW); // 8002的SD脚为低电平的时候可以输出声音，为高的时候不能输出声音
	
}
void AudioHal_DeInit(void)
{
    // 停止PWM输出
    drv_tim_pwm_output_stop(PWM_TIMER, TIM_CHAN_1);

    // 禁用GPDMA
	drv_tim_gpdma_channel_release(PWM_TIMER);

    // 清空DMA链表
    memset(&pwm_dma_env, 0, sizeof(pwm_dma_env));

    // 清除NVIC挂起中断标志，防止假触发
    NVIC_ClearPendingIRQ(TIM2_IRQn);

    log_debug( "AudioHal_DeInit: PWM and DMA deinitialized\n");
}


uint8_t Audio_Play_Start(const char *filename)
{
	uint8_t return_data = 0;
	   if (filename == NULL) {
        return 1;
    }
	   if(!g_audio_play_done)
	   {
		   return 2;
	   }
	   
	audio_status_flag=0;
	
    size_t filename_len = strlen(filename);
    
    // 检查文件名长度是否合法
    if (filename_len == 0 || filename_len >= MAX_FILENAME_LEN) {
        // 文件名太长或为空，清空并返回
        current_audio_file[0] = '\0';
        return 3;
    }   
 // 如果当前正在播放（g_audio_play_done==false），先停止当前播放，避免pop/click
    if (!g_audio_play_done) {
//		Audio_FadeOut_then_Stop(40);
//        log_debug( "Audio_Play_Start: playback in-progress, stopping current first\n");
//        // 立即停止并释放所有硬件资源（同步）
        PWM_Audio_Stop();
		osDelay(20);
//        // 给硬件一点稳态时间，等待 DMA/TIMER 真实停止
    }	
	memset(current_audio_file,0,sizeof(current_audio_file));
	memcpy(current_audio_file,filename,filename_len);
	if(1 == check_wave_file(current_audio_file))
	{
		log_debug("Audio_Play_Start\n");
		// 关闭功放
		Audio_Play_SD_Disable();
       // drv_gpio_write(OM_GPIO0, GPIO_MASK(AUDIO_SD_IO), GPIO_LEVEL_HIGH);
        osDelay(5);
		 AudioHal_DeInit();   // ? 新增
		fill_pwm_buffer_with_silence();
		// 先把 dma buffer 内存整体置为静音（按 16-bit）
		g_audio_play_done = false;
		Voice_frame_counter = 0;
		buffer_block_index = 3;
		AudioHal_Init();
		drv_tim_pwm_output_start(PWM_TIMER, &audio_pwm_config);
		osDelay(20);
		Audio_Play_SD_Enable();
		//drv_gpio_write(OM_GPIO0, GPIO_MASK(AUDIO_SD_IO), GPIO_LEVEL_LOW); 
		// 8002的SD脚为低电平的时候可以输出声音，为高的时候不能输出声音
	}
	log_debug( "******** = %d \r\n",drv_rcc_clock_get(RCC_CLK_TIM2));
	return 0;
}

void PWM_Audio_Play_Init(void)
{

    buffer_block_index = 3;
    
    // 初始化DMA缓冲区为静音（中间值）
    memset(pwm_dma_env.buf, PWM_MID, sizeof(pwm_dma_env.buf));
	
	AudioHal_Init();
}
uint8_t Audio_Status_Get(void)
{
	return audio_status_flag;
}
void PWM_Audio_Stop(void)
{
// 关闭PWM输出（停止引脚电平变化）
	// 在任何 AudioHal_Init 或 PWM 启动前执行：
	fill_pwm_buffer_with_silence();
	osDelay(5); // 稍等DMA刷新
    drv_tim_pwm_output_stop(PWM_TIMER, TIM_CHAN_1);


    // 释放/关闭 DMA 通道
    drv_tim_gpdma_channel_release(PWM_TIMER);

    // 清除 NVIC 挂起，避免假中断
    NVIC_ClearPendingIRQ(TIM2_IRQn);

    // 重置一些状态
    g_audio_play_done = true;
    buffer_block_index = 3;

    log_debug( "PWM_Audio_Stop: stopped and resources released\n");
}

int8_t check_wave_file(const char *filename)
{
	int8_t file_is_ok = 1;
	uint8_t flash_buffer[44];
	wave_chunks_t *p_chunk;
	wave_chunk2_t *p_chunk2;

	if(lfs_read_file(filename,flash_buffer,sizeof(flash_buffer),1)<=0)
	{
		log_debug( "lfs_read_file Fail = %s\r\n",filename);
		return false;
	}
//	memcpy(flash_buffer,audio_data,sizeof(flash_buffer));
	//drv_flash_read(OM_FLASH0, FILE_FLASH_ADDRESS, flash_buffer, sizeof(flash_buffer));
//		for(int i=0;i<44;i++)
//	{
//		 log_debug( "%#x ",flash_buffer[i]);
//	} log_debug( "\r\n");
	  p_chunk = (wave_chunks_t *)flash_buffer;  
		
	  if(p_chunk->ChunkID != 0x46464952)       // RIFF
		file_is_ok = -1;
		
	  if(p_chunk->Format != 0x45564157)        // WAVE
		file_is_ok = -2;
	  if(p_chunk->SubChunk1ID != 0x20746D66)   // Fmt 
		file_is_ok = -3;
	  if((p_chunk->SubChunk1Size != 16)&&
		 (p_chunk->SubChunk1Size != 18)&&
		 (p_chunk->SubChunk1Size != 40))
	  {
		file_is_ok = -4;
	  }
	//  if(p_chunk->AudioFormat != 0x0001)       // PCM
	//    file_is_ok = -5;
	//  if(p_chunk->NumChannels != 0x0001)       // mono
	//    file_is_ok = -6;
	//  if(p_chunk->SampleRate != 0x00003E80)    // 16k
	//    file_is_ok = -7; 
	//  if(p_chunk->ByteRate != 0x00003E80)      // sampleRate*numChannels*BitsPerSample/8
	//    file_is_ok = -8; 
	//  if(p_chunk->BlockAlign != 0x0001)        // NumChannels * BitsPerSample/8
	//    file_is_ok = -9;
	  if(p_chunk->BitPerSample != 0x0010)      // 16bit
		file_is_ok = -10;  

	  p_chunk2 = (wave_chunk2_t *)((uint32_t)&p_chunk->SubChunk1ID + 8 + p_chunk->SubChunk1Size);
	  
	//  if(p_chunk2->id != 0x5453494c)   // data
	//    file_is_ok = -11;  
	  if(true == file_is_ok)
	  {
		m_wave_data_size = p_chunk2->size;
	   // m_wave_data_start = FILE_FLASH_ADDRESS + 28 + p_chunk->SubChunk1Size;
		 m_wave_data_start = 0 + 28 + p_chunk->SubChunk1Size;
		log_debug( "file is OK= %d %d %d \r\n",m_wave_data_start,p_chunk->SubChunk1Size,m_wave_data_size);
	  }
		else
		{
			log_debug( "file is fail=%d\n", file_is_ok);
		}
	  log_debug( "wave size:0x%08X\n", m_wave_data_size);
	  log_debug( "wave start:0x%08X\n", m_wave_data_start);
		
	  return file_is_ok;
}

#endif
