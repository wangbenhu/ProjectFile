#ifndef __LFS_WAV_H__
#define __LFS_WAV_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
/*===========================================================
 * 版本信息
 *===========================================================*/
#define DRIVER_LFS_WAV_VERSION    "1.0.0"
// =======================================================
// 公共接口：初始化WAV系统
// =======================================================
void wav_system_init(void);
// 开始录音，创建新WAV文件
int wav_start(const char *filename, int channels, int sample_rate, int bits_per_sample);
// 异步写入PCM数据
int wav_write_async(void *data, size_t size);
// 停止录音（后台任务会修复头部）
int wav_stop(void);


//example
/*
// 初始化
wav_system_init();

// 开始录音
wav_start("rec.wav", 1, 16000, 16);

// 在录音任务里不断投递PCM数据
while (recording) {
    int16_t pcm_buf[256];
    // 采集麦克风数据到 pcm_buf ...
    wav_write_async(pcm_buf, sizeof(pcm_buf));
}

// 停止录音
wav_stop();

// 如果意外掉电 → 重启时调用修复
wav_repair("rec.wav", 1, 16000, 16);
*/


#ifdef __cplusplus
}
#endif

#endif /* __DRIVER_XXX_H__ */
