#include "lfs_wav.h"
#include "cmsis_os2.h"
#include "lfs.h"


// LittleFS 句柄
extern lfs_t lfs;

// 队列消息类型
typedef struct {
    void *data;
    size_t size;
    int cmd;   // 0=写数据, 1=停止
} wav_msg_t;

// 任务和队列控制
static osThreadId_t wav_thread_id;
static osMessageQueueId_t wav_queue_id;

// WAV写入上下文
typedef struct {
    lfs_file_t file;
    char filename[64];
    int channels;
    int sample_rate;
    int bits_per_sample;
    uint32_t data_written;
    int active;
} wav_context_t;

static wav_context_t wav_ctx;

// WAV头结构（44字节）
#pragma pack(push, 1)
typedef struct {
    char riff_id[4];
    uint32_t riff_size;
    char wave_id[4];
    char fmt_id[4];
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data_id[4];
    uint32_t data_size;
} wav_header_t;
#pragma pack(pop)

// =======================================================
// 内部函数：写WAV头
// =======================================================
static void wav_write_header(lfs_t *lfs, wav_context_t *ctx) {
    wav_header_t header;
    memcpy(header.riff_id, "RIFF", 4);
    header.riff_size = 0; // 占位
    memcpy(header.wave_id, "WAVE", 4);
    memcpy(header.fmt_id, "fmt ", 4);
    header.fmt_size = 16;
    header.audio_format = 1;
    header.num_channels = ctx->channels;
    header.sample_rate = ctx->sample_rate;
    header.bits_per_sample = ctx->bits_per_sample;
    header.byte_rate = ctx->sample_rate * ctx->channels * ctx->bits_per_sample / 8;
    header.block_align = ctx->channels * ctx->bits_per_sample / 8;
    memcpy(header.data_id, "data", 4);
    header.data_size = 0;

    lfs_file_rewind(lfs, &ctx->file);
    lfs_file_write(lfs, &ctx->file, &header, sizeof(header));
}

// =======================================================
// 后台任务：处理写入请求
// =======================================================
static void wav_task(void *argument) {
    wav_msg_t msg;

    while (1) {
        if (osMessageQueueGet(wav_queue_id, &msg, NULL, osWaitForever) == osOK) {
            if (msg.cmd == 0 && wav_ctx.active) {
                // 写PCM数据
                lfs_file_write(&lfs, &wav_ctx.file, msg.data, msg.size);
                wav_ctx.data_written += msg.size;
            } else if (msg.cmd == 1 && wav_ctx.active) {
                // 停止并修复头部
                wav_header_t header;
                wav_ctx.active = 0;

                uint32_t data_size = wav_ctx.data_written;
                memcpy(header.riff_id, "RIFF", 4);
                header.riff_size = 36 + data_size;
                memcpy(header.wave_id, "WAVE", 4);
                memcpy(header.fmt_id, "fmt ", 4);
                header.fmt_size = 16;
                header.audio_format = 1;
                header.num_channels = wav_ctx.channels;
                header.sample_rate = wav_ctx.sample_rate;
                header.bits_per_sample = wav_ctx.bits_per_sample;
                header.byte_rate = wav_ctx.sample_rate * wav_ctx.channels * wav_ctx.bits_per_sample / 8;
                header.block_align = wav_ctx.channels * wav_ctx.bits_per_sample / 8;
                memcpy(header.data_id, "data", 4);
                header.data_size = data_size;

                lfs_file_rewind(&lfs, &wav_ctx.file);
                lfs_file_write(&lfs, &wav_ctx.file, &header, sizeof(header));
                lfs_file_close(&lfs, &wav_ctx.file);
            }
        }
    }
}

// =======================================================
// 公共接口：初始化WAV系统
// =======================================================
void wav_system_init(void) {
    wav_queue_id = osMessageQueueNew(8, sizeof(wav_msg_t), NULL);
    wav_thread_id = osThreadNew(wav_task, NULL, NULL);
}

// 开始录音，创建新WAV文件
int wav_start(const char *filename, int channels, int sample_rate, int bits_per_sample) {
    if (wav_ctx.active) return -1;

    memset(&wav_ctx, 0, sizeof(wav_ctx));
    strncpy(wav_ctx.filename, filename, sizeof(wav_ctx.filename)-1);
    wav_ctx.channels = channels;
    wav_ctx.sample_rate = sample_rate;
    wav_ctx.bits_per_sample = bits_per_sample;
    wav_ctx.data_written = 0;
    wav_ctx.active = 1;

    if (lfs_file_open(&lfs, &wav_ctx.file, filename, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) < 0) {
        return -2;
    }

    wav_write_header(&lfs, &wav_ctx);
    return 0;
}

// 异步写入PCM数据
int wav_write_async(void *data, size_t size) {
    if (!wav_ctx.active) return -1;

    wav_msg_t msg = { .data = data, .size = size, .cmd = 0 };
    if (osMessageQueuePut(wav_queue_id, &msg, 0, 0) != osOK) {
        return -2; // 队列满
    }
    return 0;
}

// 停止录音（后台任务会修复头部）
int wav_stop(void) {
    if (!wav_ctx.active) return -1;

    wav_msg_t msg = { .cmd = 1 };
    osMessageQueuePut(wav_queue_id, &msg, 0, 0);
    return 0;
}

// 掉电恢复修复WAV文件
int wav_repair(const char *filename, int channels, int sample_rate, int bits_per_sample) {
    lfs_file_t file;
    if (lfs_file_open(&lfs, &file, filename, LFS_O_RDWR) < 0) return -1;

    lfs_soff_t filesize = lfs_file_size(&lfs, &file);
    if (filesize < sizeof(wav_header_t)) {
        lfs_file_close(&lfs, &file);
        return -2;
    }

    uint32_t data_size = filesize - sizeof(wav_header_t);

    wav_header_t header;
    memcpy(header.riff_id, "RIFF", 4);
    header.riff_size = 36 + data_size;
    memcpy(header.wave_id, "WAVE", 4);
    memcpy(header.fmt_id, "fmt ", 4);
    header.fmt_size = 16;
    header.audio_format = 1;
    header.num_channels = channels;
    header.sample_rate = sample_rate;
    header.bits_per_sample = bits_per_sample;
    header.byte_rate = sample_rate * channels * bits_per_sample / 8;
    header.block_align = channels * bits_per_sample / 8;
    memcpy(header.data_id, "data", 4);
    header.data_size = data_size;

    lfs_file_rewind(&lfs, &file);
    lfs_file_write(&lfs, &file, &header, sizeof(header));

    lfs_file_close(&lfs, &file);
    return 0;
}