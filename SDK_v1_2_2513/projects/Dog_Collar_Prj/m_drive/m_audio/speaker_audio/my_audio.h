#ifndef __MY_AUDIO_H__
#define __MY_AUDIO_H__

#include "om_driver.h"
#include "test_audio.h"

#define PWM_TIMER	   						OM_TIM2


#define FILE_FLASH_ADDRESS			(0x50000) //  // 语音数据存放在内部flash的地址

#if (AUDIO_FORMAT_DEFALT)



	//#define FRAME_SIZE							160//240//160
	#define FRAME_SIZE_8K       640
	#define FRAME_SIZE_16K      (FRAME_SIZE_8K * 2)
	#define SAMPLE_RATE							8000
	#define CHANNELS								1

	#define MAX_FRAME_SIZE					(2 * FRAME_SIZE_8K)
	
	#define BUFFER_BLOCK_NUM 				4 // block counter define, more than 3 block
	//#define BUFFER_BLOCK_SIZE       (FRAME_SIZE * 2) // 块的大小
	#define BUFFER_BLOCK_SIZE   (FRAME_SIZE_16K * sizeof(uint16_t))
	// 系统时钟跑96M的时候
	#define PWM_Sample_Rate					16000 // pwm输出频率
	#define PWM_Clock_Hz						16000000UL // 系统时钟跑64M的时候，pwm时钟才能跑32M
	#define DMA_REPEAT_CNT					1 // 8k采样率的时候pwm同一个占空比值要输出4次，16k采样率的时候要输出2次.

	#define PWM_Period_Cnt					(PWM_Clock_Hz / PWM_Sample_Rate) // pwm输出级数，这里是64000000/48000=1333级
	#define PWM_0_OFFSET					(PWM_Period_Cnt / 2) // pwm数据偏移量，因为mic采集出来的数据有正数和负数，pwm的占空比正数和负数各占一半



	typedef struct {
		gpdma_chain_trans_t chain[BUFFER_BLOCK_NUM];
		uint16_t buf[BUFFER_BLOCK_NUM * BUFFER_BLOCK_SIZE / 2];
	} pwm_dma_env_t;

	
	
#else
	#define FRAME_SIZE							160//240//160
	//#define SAMPLE_RATE							8000
	#define SAMPLE_RATE							16000
	#define CHANNELS								1
	#define APPLICATION							OPUS_APPLICATION_AUDIO
	#define BITRATE									32000

	#define MAX_FRAME_SIZE					(2 * FRAME_SIZE)
	#define MAX_PACKET_SIZE					(2 * (FRAME_SIZE + 256))

	#define BUFFER_BLOCK_NUM 				4 // block counter define, more than 3 block
	#define BUFFER_BLOCK_SIZE       (FRAME_SIZE * 2) // 块的大小

	// 系统时钟跑96M的时候
	#define PWM_Sample_Rate					SAMPLE_RATE // pwm输出频率
	#define PWM_Clock_Hz						(32 * 1000 * 1000) // 系统时钟跑64M的时候，pwm时钟才能跑32M
	#define DMA_REPEAT_CNT					1 // 8k采样率的时候pwm同一个占空比值要输出4次，16k采样率的时候要输出2次.

	#define PWM_Period_Cnt					(PWM_Clock_Hz / PWM_Sample_Rate) // pwm输出级数，这里是64000000/48000=1333级
	#define PWM_0_OFFSET						(PWM_Period_Cnt / 2) // pwm数据偏移量，因为mic采集出来的数据有正数和负数，pwm的占空比正数和负数各占一半

	typedef struct {
		gpdma_chain_trans_t chain[BUFFER_BLOCK_NUM];
		uint16_t buf[BUFFER_BLOCK_NUM * BUFFER_BLOCK_SIZE / 2];
	} pwm_dma_env_t;

#endif

	#pragma pack (1)
typedef struct // 44 byte
	{
														// 别名					字节数		类型			注释
		int8_t riff[4];					// ckid						4				char			"RIFF" 标志, 大写
		uint32_t fileLength;		// cksize					4				int32			文件长度。这个长度不包括"RIFF"标志 和文件长度 本身所占字节, 下面的子块大小也是这样。
		int8_t wavTag[4];				// fcc type				4				char			"WAVE" 类型块标识, 大写。
		int8_t fmt[4];					// ckid						4				char			表示"fmt" chunk的开始。此块中包括文件内部格式信息。小写, 最后一个字符是空格。
		uint32_t size;					// cksize					4       int32     文件内部格式信息数据的大小。
		uint16_t formatTag;			// FormatTag			2       int16     音频数据的编码方式。1 表示是 PCM 编码
		uint16_t channel;				// Channels				2       int16     声道数，单声道为1，双声道为2
		uint32_t sampleRate;		// SamplesPerSec	4       int32     采样率(每秒样本数), 比如 44100 等
		uint32_t bytePerSec;		// BytesPerSec		4       int32     音频数据传送速率, 单位是字节。其值为采样率×每次采样大小。播放软件利用此值可以估计缓冲区的大小。
		uint16_t blockAlign;		// BlockAlign			2       int16     每次采样的大小 = 采样精度*声道数/8(单位是字节); 这也是字节对齐的最小单位, 譬如 16bit 立体声在这里的值是 4 字节。播放软件需要 一次处理多个该值大小的字节数据，以便将其值用于缓冲区的调整。
		uint16_t bitPerSample;	// BitsPerSample	2       int16     每个声道的采样精度; 譬如 16bit 在这里的值就是16。如果有多个声道，则每个声道的采样精度大小都一样的。
	//	/****有的wav文件不一定有这3个内容*****/
	//	uint32_t fact;					// [ckid]					4       char      "fact".
	//	uint32_t fact_cd_size;	// [cksize]				4       int32     "fact" chunk data size.
	//	uint32_t fact_data_size;// [fact data]		4       int32     解压后的音频数据的大小(Bytes).
	//	/****有的wav文件不一定有这3个内容*****/
		int8_t data[4];					// ckid						4       char      表示 "data" chunk的开始。此块中包含音频数据。小写。
		uint32_t dataSize;			// cksize					4       int32     音频数据的长度
	}wave_header_t;

	typedef struct
	{
	  uint32_t ChunkID;
	  uint32_t ChunkSize;
	  uint32_t Format;
	  uint32_t SubChunk1ID;
	  uint32_t SubChunk1Size;
	  uint16_t AudioFormat;
	  uint16_t NumChannels;
	  uint32_t SampleRate;
	  uint32_t ByteRate;
	  uint16_t BlockAlign;
	  uint16_t BitPerSample;
	}wave_chunks_t;

	typedef enum {
		IFLASH_AUDIO_1 = 0x00054000U,
		IFLASH_AUDIO_2 = 0x0005B000U,
		IFLASH_AUDIO_3 = 0x00062000U,
		IFLASH_AUDIO_4 = 0x00069000U,
		IFLASH_AUDIO_5 = 0x00070000U,
	} AudioAddr_T;
	
	typedef struct
	{
	  uint32_t id;
	  uint32_t size;
	}wave_chunk2_t;
	#pragma pack ()
	
typedef enum {
    FLASH_INT = 0,  // 内部Flash
    FLASH_EXT = 1,   // 外部Flash
} FlashSel_T;

	void AudioHal_Init(void);
	void PWM_Audio_Play(void);
	void PWM_Audio_Stop(void);
	void Audio_PCM_Data_Update(uint8_t *pSrc, int16_t nBytes);
	void Audio_push_to_RingBuffer(uint8_t *audio_pcm_buffer, int nBytes);

	void PWM_audio_play_handler(void);

	uint8_t Audio_Status_Get(void);
	void test_pin_init(void);
	float calculate_playback_time(uint32_t data_size_bytes, 
								 uint32_t sample_rate, 
								 uint16_t bit_depth);
	void AudioHal_DeInit(void);
	uint8_t Audio_Play_Start(const char *filename,uint8_t audio_reset);
	uint8_t Audio_Play_Request(const char *filename, uint8_t audio_reset);
	uint8_t Audio_IFlash_Play_Request(uint8_t index, uint8_t audio_reset);
	uint8_t Audio_Play_Stop_Request(void);
	void Audio_Play_Complete_Notify(void);
	void audio_evt_callback(void);
uint8_t Audio_IFlash_Play_Start(uint8_t index,uint8_t audio_reset);
#endif
