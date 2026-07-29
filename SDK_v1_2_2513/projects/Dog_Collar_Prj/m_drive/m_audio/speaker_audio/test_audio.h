#ifndef TEST_ADUIO_H
#define TEST_ADUIO_H

#include <stdint.h>

#define DEFALT_16k_16bit    0
#define DEFALT_8k_8bit   	1

#define AUDIO_FORMAT_DEFALT DEFALT_8k_8bit
// 声明外部数组（在某个.c文件中定义）
extern const uint8_t audio_data[];
extern const uint32_t audio_data_size;

// 或者直接在这里定义（如果编译器支持）
// const uint8_t audio_data[] = {0x12, 0x34, 0x56, ...};
// const uint32_t audio_data_size = 1024;

#endif // AUDIO_DATA_H