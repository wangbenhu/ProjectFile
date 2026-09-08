/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @version
 * Version V20240101.2.0  (V1.6 通用 BULK 数据通道)
 *  - 适配多种数据类型: 音频 (0xA1) / GNSS (0xA2) / 日志 (0xA3) / 预留 (0xA4-0xFF)
 *  - 协议帧: 状态(1) + 数据类型(1) + 传输ID(1) + 总长度(2) + CRC(2) = 7 字节
 *  - ACK 包改为 3 字节: 状态 + 数据类型 + 传输ID (原 2 字节缺类型)
 *  - 开始包 / 结束包均加 CRC 校验 (原缺失)
 *  - 按数据类型分发给对应 handler (audio/gnss/log)
 *
 * @{
 */

/*******************************************************************************
 * INCLUDES
 */
#include <string.h>
#include "omble.h"
#include "service_audio.h"
#include "common_def.h"

/*********************************************************************
 * MACROS - BULK 协议
 */
/* BULK_TYPE_* 定义见 service_audio.h */

#define BULK_CMD_START              0x01   // 开始包 (BLE→设备)
#define BULK_CMD_ACK_DATA           0x02   // 数据包 ACK (设备→BLE)
#define BULK_CMD_COMPLETE           0x03   // 结束包 / 结束 ACK (BLE→设备 / 设备→BLE)
#define BULK_CMD_NACK               0x04   // 错误 / NACK (设备→BLE)

#define BULK_MTU_SIZE               160    // 仅用于日志估算总包数 (实际包长 = 手机MTU, 160/240/512 自适应)
#define BULK_PACKETS_PER_ROUND      10     // 每轮 10 包 ACK
#define BULK_BUFFER_SIZE            2048   // 2KB 累积缓冲 (放不下时先 flush 再收, 不 NACK)

// 传输状态机
typedef enum {
    M_BULK_STATE_IDLE             = 0x00,
    M_BULK_STATE_TRANSFERRING     = 0x01,
    M_BULK_STATE_COMPLETING       = 0x02,
    M_BULK_STATE_ERROR            = 0x03,
} bulk_state_t;

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint16_t m_start_handle;

typedef struct {
    bulk_state_t    state;
    uint8_t         bulk_type;        // BULK_TYPE_AUDIO / GNSS / LOG
    uint8_t         transfer_id;      // 传输 ID (音频对应 audioID 1~5)
    uint32_t        total_length;
    uint32_t        received_bytes;
    uint16_t        total_packets;
    uint16_t        received_packets;
    uint16_t        packets_in_current_round;
    uint16_t        crc_calculated;   // 数据区 CRC 累积 (用于完整性校验)
    uint8_t         buffer[BULK_BUFFER_SIZE];
    uint16_t        buffer_offset;
    uint8_t         file_opened;
    uint8_t         is_new_transfer;
    uint8_t         length_err_nacked; // 超长/长度错误已回 04
} bulk_transfer_t;

static bulk_transfer_t m_bulk;

/*********************************************************************
 * TYPE HANDLER DISPATCH - 数据类型分发表
 */
typedef int (*bulk_open_fn) (uint8_t transfer_id, uint8_t is_new_transfer);
typedef int (*bulk_write_fn)(const uint8_t *data, uint16_t len);
typedef int (*bulk_close_fn)(void);

typedef struct {
    uint8_t         type;
    const char     *name;
    bulk_open_fn    open;
    bulk_write_fn   write;
    bulk_close_fn   close;
} bulk_handler_t;

/* ===== 音频 handler: 复用用户实现的 audio_file_* 接口 ===== */
static int bulk_audio_open(uint8_t transfer_id, uint8_t is_new_transfer) {
    return audio_file_open((uint32_t)transfer_id, BULK_TYPE_AUDIO, is_new_transfer);
}
static int bulk_audio_write(const uint8_t *data, uint16_t len) {
    return audio_file_write(data, len);
}
static int bulk_audio_close(void) {
    return audio_file_close();
}

/* ===== GNSS 星历 handler: TODO 用户实现 (存 flash 或直接解析) ===== */
static int bulk_gnss_open(uint8_t transfer_id, uint8_t is_new_transfer) {
    /* TODO: 用户实现 - GNSS 星历文件打开 */
    log_debug("[BULK][GNSS] OPEN tid=%d is_new=%d (TODO)\r\n", transfer_id, is_new_transfer);
    (void)transfer_id; (void)is_new_transfer;
    return 0;
}
static int bulk_gnss_write(const uint8_t *data, uint16_t len) {
    /* TODO: 用户实现 - 写星历数据 (lfs_file_write 或解析 + 应用) */
    (void)data; (void)len;
    return 0;
}
static int bulk_gnss_close(void) {
    /* TODO: 用户实现 - 关闭星历文件 (可触发解析/上报) */
    return 0;
}

/* ===== 日志 handler: TODO 用户实现 ===== */
static int bulk_log_open(uint8_t transfer_id, uint8_t is_new_transfer) {
    /* TODO: 用户实现 - 日志文件打开 */
    log_debug("[BULK][LOG] OPEN tid=%d is_new=%d (TODO)\r\n", transfer_id, is_new_transfer);
    (void)transfer_id; (void)is_new_transfer;
    return 0;
}
static int bulk_log_write(const uint8_t *data, uint16_t len) {
    /* TODO: 用户实现 - 写日志数据 */
    (void)data; (void)len;
    return 0;
}
static int bulk_log_close(void) {
    /* TODO: 用户实现 - 关闭日志文件 */
    return 0;
}

static const bulk_handler_t bulk_handlers[] = {
    { BULK_TYPE_AUDIO, "AUDIO", bulk_audio_open,  bulk_audio_write,  bulk_audio_close  },
    { BULK_TYPE_GNSS,  "GNSS",  bulk_gnss_open,   bulk_gnss_write,   bulk_gnss_close   },
    { BULK_TYPE_LOG,   "LOG",   bulk_log_open,    bulk_log_write,    bulk_log_close    },
};
#define BULK_HANDLER_COUNT  (sizeof(bulk_handlers) / sizeof(bulk_handlers[0]))

static const bulk_handler_t *bulk_get_handler(uint8_t type) {
    for (uint32_t i = 0; i < BULK_HANDLER_COUNT; i++) {
        if (bulk_handlers[i].type == type) {
            return &bulk_handlers[i];
        }
    }
    return NULL;
}

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void    bulk_transfer_init(void);
static void    bulk_send_ack(uint8_t cmd, uint8_t type, uint8_t transfer_id);
static void    bulk_process_start(const uint8_t *data, uint16_t len);
static void    bulk_process_complete(const uint8_t *data, uint16_t len);
static void    bulk_process_data(const uint8_t *data, uint16_t len);
static uint8_t bulk_flush_buffer(void);
static uint16_t bulk_calculate_crc(const uint8_t *data, uint32_t len, uint16_t init_crc);

/* GATT characteristic indices */
enum {
    IDX_BULK_SVC,
    IDX_BULK_CMD_CHAR,
    IDX_BULK_CMD_VAL,
    IDX_BULK_CMD_DESC,
    IDX_BULK_DATA_CHAR,
    IDX_BULK_DATA_VAL,
};

/* CRC-16/MODBUS (poly 0xA001, init 0xFFFF) */
static uint16_t bulk_calculate_crc(const uint8_t *data, uint32_t len, uint16_t init_crc) {
    uint16_t crc = init_crc;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            crc = (crc & 0x0001U) ? (uint16_t)((crc >> 1) ^ 0xA001U) : (uint16_t)(crc >> 1);
        }
    }
    return crc;
}

/* GATT characteristic declaration - 复用原有 audio service UUID */
void service_om_audio_init(void) {
    static const uint8_t serv_bulk[2] = {0xE0, 0xFF};
    static const uint8_t cmd_char[2]  = {0xE1, 0xFF};
    static const uint8_t data_char[2] = {0xE2, 0xFF};

    static const ob_gatt_item_t atts_bulk[] = {
        { ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
        { cmd_char,        OB_UUID_16BIT, OB_ATT_PROP_WRITE | OB_ATT_PROP_NTF },
        { ob_att_cccd_def, OB_UUID_16BIT, OB_ATT_PROP_READ  | OB_ATT_PROP_WRITE },
        { ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
        { data_char,       OB_UUID_16BIT, OB_ATT_PROP_WRITE_CMD },
    };

    static const ob_gatt_serv_t att_serv_bulk = {
        serv_bulk, OB_UUID_16BIT,
        sizeof(atts_bulk) / sizeof(atts_bulk[0]), atts_bulk
    };

    ob_gatts_add_service(&att_serv_bulk, &m_start_handle);
    bulk_transfer_init();
}

static void bulk_transfer_init(void) {
    memset(&m_bulk, 0, sizeof(m_bulk));
    m_bulk.state = M_BULK_STATE_IDLE;
}

/* ACK: 3 字节 (状态+数据类型+传输ID) — 修复原 2 字节缺类型 */
static void bulk_send_ack(uint8_t cmd, uint8_t type, uint8_t transfer_id) {
    uint8_t resp[3] = { cmd, type, transfer_id };
    ob_gatts_hvx_t hvx = {
        OB_HANDLE_VALUE_NTF,
        m_start_handle + IDX_BULK_CMD_VAL,
        resp,
        3,
    };
    ob_gatts_send_hvx(0, &hvx);
    log_debug("[BULK] ACK cmd=0x%02X type=0x%02X tid=%d\r\n", cmd, type, transfer_id);
}

static uint8_t bulk_flush_buffer(void) {
    if (m_bulk.buffer_offset == 0) {
        return 0;
    }
    const bulk_handler_t *h = bulk_get_handler(m_bulk.bulk_type);
    if (h == NULL || h->write == NULL) {
        log_debug("[BULK] no handler write for type 0x%02X\r\n", m_bulk.bulk_type);
        return 1;
    }
    if (h->write(m_bulk.buffer, m_bulk.buffer_offset) != 0) {
        return 1;
    }
    m_bulk.buffer_offset = 0;
    return 0;
}

/* 开始包: 7 字节 = 状态(1)+类型(1)+传输ID(1)+总长度(2)+CRC(2) — 加 CRC 校验 */
static void bulk_process_start(const uint8_t *data, uint16_t len) {
    if (len != 7) {
        log_debug("[BULK] start bad len=%d (expect 7)\r\n", len);
        if (len >= 3) bulk_send_ack(BULK_CMD_NACK, data[1], data[2]);
        return;
    }
    uint8_t state = data[0], type = data[1], tid = data[2];
    if (state != BULK_CMD_START) {
        log_debug("[BULK] start bad state=0x%02X\r\n", state);
        return;
    }
    uint32_t total_length = ((uint32_t)data[3] << 8) | (uint32_t)data[4];
    /* CRC-16/MODBUS 发送字节序: 低字节在前 (整帧重算恒为 0) */
    uint16_t crc_exp = (uint16_t)(data[5] | ((uint16_t)data[6] << 8));
    /* CRC 校验 header (5字节: 状态+类型+传输ID+总长度) */
    uint16_t crc_cal = bulk_calculate_crc(data, 5, 0xFFFFU);
    if (crc_cal != crc_exp) {
        log_debug("[BULK] start CRC err cal=0x%04X exp=0x%04X\r\n", crc_cal, crc_exp);
        bulk_send_ack(BULK_CMD_NACK, type, tid);
        return;
    }
    /* 类型分发表查找 */
    const bulk_handler_t *h = bulk_get_handler(type);
    if (h == NULL) {
        log_debug("[BULK] unsupported type 0x%02X\r\n", type);
        bulk_send_ack(BULK_CMD_NACK, type, tid);
        return;
    }
    /* 关闭进行中的旧传输(不同 ID 或同 ID 重新传输) */
    if (m_bulk.state == M_BULK_STATE_TRANSFERRING) {
        if (m_bulk.buffer_offset > 0) {
            bulk_flush_buffer();
        }
        if (m_bulk.file_opened) {
            const bulk_handler_t *oh = bulk_get_handler(m_bulk.bulk_type);
            if (oh && oh->close) oh->close();
            m_bulk.file_opened = 0;
        }
    }
    /* 初始化新传输状态 */
    m_bulk.bulk_type               = type;
    m_bulk.transfer_id             = tid;
    m_bulk.total_length            = total_length;
    m_bulk.received_bytes          = 0;
    m_bulk.buffer_offset           = 0;
    m_bulk.received_packets        = 0;
    m_bulk.packets_in_current_round = 0;
    m_bulk.crc_calculated           = 0xFFFFU;
    m_bulk.is_new_transfer         = 1;
    m_bulk.length_err_nacked       = 0;
    m_bulk.total_packets = (uint16_t)((total_length + BULK_MTU_SIZE - 1) / BULK_MTU_SIZE);
    /* open 数据通道 */
    if (h->open(tid, 1) != 0) {
        log_debug("[BULK] start open failed type=%s tid=%d\r\n", h->name, tid);
        bulk_send_ack(BULK_CMD_NACK, type, tid);
        return;
    }
    m_bulk.file_opened = 1;
    m_bulk.state = M_BULK_STATE_TRANSFERRING;
    /* 3 字节开始 ACK: 状态(0x01) + 类型 + 传输ID */
    bulk_send_ack(BULK_CMD_START, type, tid);
    log_debug("[BULK] START type=%s tid=%d total=%d packets=%d\r\n",
              h->name, tid, total_length, m_bulk.total_packets);
}

/* 结束包: 7 字节, 校验 CRC + 长度 */
static void bulk_process_complete(const uint8_t *data, uint16_t len) {
    if (len != 7) {
        log_debug("[BULK] complete bad len=%d\r\n", len);
        if (len >= 3) bulk_send_ack(BULK_CMD_NACK, data[1], data[2]);
        return;
    }
    uint8_t state = data[0], type = data[1], tid = data[2];
    if (state != BULK_CMD_COMPLETE) return;
    /* 验证当前传输匹配 */
    if (m_bulk.state != M_BULK_STATE_TRANSFERRING ||
        tid != m_bulk.transfer_id ||
        type != m_bulk.bulk_type) {
        bulk_send_ack(BULK_CMD_NACK, type, tid);
        return;
    }
    uint32_t total_length = ((uint32_t)data[3] << 8) | (uint32_t)data[4];
    /* CRC-16/MODBUS 发送字节序: 低字节在前 (整帧重算恒为 0) */
    uint16_t crc_exp = (uint16_t)(data[5] | ((uint16_t)data[6] << 8));
    uint16_t crc_cal = bulk_calculate_crc(data, 5, 0xFFFFU);
    /* 校验 header CRC 和 length 匹配 */
    if (crc_cal != crc_exp || m_bulk.received_bytes != total_length) {
        log_debug("[BULK] complete verify err: crc cal=0x%04X exp=0x%04X recv=%u total=%u\r\n",
                  crc_cal, crc_exp, m_bulk.received_bytes, total_length);
        bulk_send_ack(BULK_CMD_NACK, type, tid);
        if (m_bulk.file_opened) {
            const bulk_handler_t *oh = bulk_get_handler(type);
            if (oh && oh->close) oh->close();
            m_bulk.file_opened = 0;
        }
        bulk_transfer_init();
        return;
    }
    /* flush 残余数据 */
    if (m_bulk.buffer_offset > 0) {
        if (bulk_flush_buffer() != 0) {
            bulk_send_ack(BULK_CMD_NACK, type, tid);
            bulk_transfer_init();
            return;
        }
    }
    /* close 数据通道 */
    const bulk_handler_t *h = bulk_get_handler(type);
    if (h && h->close) h->close();
    m_bulk.file_opened = 0;
    /* ACK: 成功 */
    bulk_send_ack(BULK_CMD_COMPLETE, type, tid);
    log_debug("[BULK] COMPLETE type=%s tid=%d bytes=%u\r\n",
              h ? h->name : "?", tid, m_bulk.received_bytes);
    bulk_transfer_init();
}

/* 数据包: 长度 = 手机MTU(160/240/512...), 自适应累积; 缓冲放不下先 flush 再收; 每满 10 包或收完 ACK */
static void bulk_process_data(const uint8_t *data, uint16_t len) {
    if (m_bulk.state != M_BULK_STATE_TRANSFERRING) {
        log_debug("[BULK] data ignored: not transferring\r\n");
        return;
    }
    /* 已收满: 多余数据包 = 总长度不符, 回 04 NACK (仅首次, 防刷屏), 等结束包或新 START */
    if (m_bulk.received_bytes >= m_bulk.total_length) {
        if (!m_bulk.length_err_nacked) {
            log_debug("[BULK] data overflow: recv=%u total=%u, NACK\r\n",
                      m_bulk.received_bytes, m_bulk.total_length);
            bulk_send_ack(BULK_CMD_NACK, m_bulk.bulk_type, m_bulk.transfer_id);
            m_bulk.length_err_nacked = 1;
        }
        return;
    }
    /* 本包会导致超出总长度: 截断到剩余量 (异常长度保护) */
    if (m_bulk.received_bytes + len > m_bulk.total_length) {
        log_debug("[BULK] data truncated: len=%d remain=%u\r\n", len,
                  (uint32_t)(m_bulk.total_length - m_bulk.received_bytes));
        len = (uint16_t)(m_bulk.total_length - m_bulk.received_bytes);
    }
    /* 缓冲放不下当前包: 先 flush 已有数据腾空间, 而不是 NACK */
    if (m_bulk.buffer_offset + len > BULK_BUFFER_SIZE) {
        if (m_bulk.buffer_offset > 0) {
            if (bulk_flush_buffer() != 0) {
                bulk_send_ack(BULK_CMD_NACK, m_bulk.bulk_type, m_bulk.transfer_id);
                return;
            }
        }
        /* 单包超过整个缓冲(极端大 MTU): 直接落盘, 不进缓冲 */
        if (len > BULK_BUFFER_SIZE) {
            const bulk_handler_t *h = bulk_get_handler(m_bulk.bulk_type);
            if (h == NULL || h->write == NULL || h->write(data, len) != 0) {
                bulk_send_ack(BULK_CMD_NACK, m_bulk.bulk_type, m_bulk.transfer_id);
                return;
            }
            m_bulk.received_bytes += len;
            m_bulk.received_packets++;
            m_bulk.packets_in_current_round++;
            m_bulk.crc_calculated = bulk_calculate_crc(data, len, m_bulk.crc_calculated);
            if (m_bulk.packets_in_current_round >= BULK_PACKETS_PER_ROUND ||
                m_bulk.received_bytes >= m_bulk.total_length) {
                m_bulk.packets_in_current_round = 0;
                bulk_send_ack(BULK_CMD_ACK_DATA, m_bulk.bulk_type, m_bulk.transfer_id);
                log_debug("[BULK] DATA ACK type=0x%02X tid=%d packets=%u/%u bytes=%u/%u\r\n",
                          m_bulk.bulk_type, m_bulk.transfer_id,
                          m_bulk.received_packets, m_bulk.total_packets,
                          m_bulk.received_bytes, m_bulk.total_length);
            }
            return;
        }
    }
    /* 累积数据 */
    memcpy(&m_bulk.buffer[m_bulk.buffer_offset], data, len);
    m_bulk.buffer_offset = (uint16_t)(m_bulk.buffer_offset + len);
    m_bulk.received_bytes += len;
    m_bulk.received_packets++;
    m_bulk.packets_in_current_round++;
    /* 数据区 CRC 累积 (用于完整性校验/未来扩展) */
    m_bulk.crc_calculated = bulk_calculate_crc(data, len, m_bulk.crc_calculated);

    uint8_t should_ack = 0;
    /* 条件1: buffer 满 (已 flush 腾空, 但包数未到 10 也 ACK, 通知 APP 可继续发) */
    /* 条件2: 本轮满 10 包 */
    if (m_bulk.packets_in_current_round >= BULK_PACKETS_PER_ROUND) {
        if (m_bulk.buffer_offset > 0) {
            if (bulk_flush_buffer() != 0) {
                bulk_send_ack(BULK_CMD_NACK, m_bulk.bulk_type, m_bulk.transfer_id);
                return;
            }
        }
        should_ack = 1;
        m_bulk.packets_in_current_round = 0;
    }
    /* 条件3: 收完所有数据(等结束包 CRC 最终校验) */
    if (m_bulk.received_bytes >= m_bulk.total_length) {
        should_ack = 1;
    }
    if (should_ack) {
        bulk_send_ack(BULK_CMD_ACK_DATA, m_bulk.bulk_type, m_bulk.transfer_id);
        log_debug("[BULK] DATA ACK type=0x%02X tid=%d packets=%u/%u bytes=%u/%u\r\n",
                  m_bulk.bulk_type, m_bulk.transfer_id,
                  m_bulk.received_packets, m_bulk.total_packets,
                  m_bulk.received_bytes, m_bulk.total_length);
    }
}

/* BLE 事件分发 */
void service_om_audio_evt_cb(uint16_t evt_id, const omble_evt_t *evt) {
    if (evt_id == OB_GATTS_EVT_WRITE_REQ) {
        const uint8_t *data  = evt->gatt.write_req.data;
        uint16_t       len   = evt->gatt.write_req.len;
        switch (evt->gatt.write_req.att_hdl - m_start_handle) {
            case IDX_BULK_CMD_VAL:
                if (len > 0) {
                    switch (data[0]) {
                        case BULK_CMD_START:
                            bulk_process_start(data, len);
                            break;
                        case BULK_CMD_COMPLETE:
                            bulk_process_complete(data, len);
                            break;
                        default:
                            break;
                    }
                }
                break;
            case IDX_BULK_DATA_VAL:
                bulk_process_data(data, len);
                break;
            default:
                break;
        }
    } else if (evt_id == OB_GAP_EVT_CONNECTED) {
        bulk_transfer_init();
    } else if (evt_id == OB_GAP_EVT_DISCONNECTED) {
        if (m_bulk.file_opened) {
            const bulk_handler_t *h = bulk_get_handler(m_bulk.bulk_type);
            if (h && h->close) h->close();
            m_bulk.file_opened = 0;
        }
        bulk_transfer_init();
    }
}

void app_om_audio_transfer_init(void) {
    service_om_audio_init();
    ob_event_callback_reg(service_om_audio_evt_cb);
}

/*******************************************************************************
 * 兼容性 stub - 用户已实现的 audio_file_* (weak 符号)
 * 如果用户有自己的实现, 链接器会用用户版本; 否则用默认 stub 返回 0 (成功)。
 *******************************************************************************/
__attribute__((weak)) int audio_file_open(uint32_t audio_id, uint8_t audio_type, uint8_t is_new_transfer) {
    (void)audio_id; (void)audio_type; (void)is_new_transfer;
    return 0;
}
__attribute__((weak)) int audio_file_write(const uint8_t *data, uint16_t len) {
    (void)data; (void)len;
    return 0;
}
__attribute__((weak)) int audio_file_close(void) {
    return 0;
}

/** @} */