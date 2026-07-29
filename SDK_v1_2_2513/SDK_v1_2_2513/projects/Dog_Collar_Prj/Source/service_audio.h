#ifndef SERVICE_OM_AUDIO_TRANSFER_H
#define SERVICE_OM_AUDIO_TRANSFER_H

#include <stdint.h>

/* ============ V1.6: 通用 BULK 数据通道 - 数据类型定义 ============ */
#define BULK_TYPE_AUDIO         0xA1   // 音频文件
#define BULK_TYPE_GNSS          0xA2   // GNSS 星历数据
#define BULK_TYPE_LOG           0xA3   // 日志数据
#define BULK_TYPE_RESERVED_MIN  0xA4   // 0xA4~0xFF 预留

/* 传输状态(内部分状态机, 枚举定义见 service_audio.c: bulk_state_t) */

/* 公共 API (函数名保留 service_audio_* 以兼容现有注册) */
void app_om_audio_transfer_init(void);

/* 音频文件接口(weak 实现见 service_audio.c, 用户可覆盖实现具体行为) */
int audio_file_open(uint32_t audio_id, uint8_t audio_type, uint8_t is_new_transfer);
int audio_file_write(const uint8_t *data, uint16_t len);
int audio_file_close(void);

#endif /* SERVICE_OM_AUDIO_TRANSFER_H */