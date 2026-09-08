#ifndef BLE_OP_QUEUE_H
#define BLE_OP_QUEUE_H

#include <stdbool.h>
#include <stdint.h>

#include "app_adv.h"

typedef struct {
    uint32_t queued;            // 成功进入 BLE 操作队列的次数
    uint32_t queue_rejected;    // 入队失败或内存申请失败次数
    uint32_t tspp_sent;         // 成功交给 TSPP FIFO 的次数
    uint32_t tspp_disabled;     // Notify 未开启导致的发送失败次数
    uint32_t tspp_full;         // TSPP FIFO 空间不足次数
    uint32_t tspp_other_error;  // 其他 TSPP 错误次数
} ble_op_stats_t;

/**
 * Optional handler for a queued non-TSPP BLE operation.
 *
 * Handlers run from the BLE scheduler thread. The parameter and binary-data
 * fields are reserved for future command-specific submit APIs.
 */
typedef void (*ble_op_callback_t)(uint16_t cmd_param, const uint8_t *data,
                                  uint32_t data_len, void *context);

typedef struct {
    ble_op_callback_t start_adv;
    ble_op_callback_t stop_adv;
    ble_op_callback_t disconnect;
    ble_op_callback_t update_conn_param;
} ble_op_callbacks_t;

/**
 * Initialize the task-to-BLE operation queue.
 *
 * This function must be called after evt_init() and before any submit API.
 */
bool ble_op_init(void);

/**
 * Atomically install a copied callback table and its context.
 *
 * Passing NULL clears every optional handler. Registration is valid only
 * after ble_op_init() succeeds. User handlers execute without the module
 * control mutex held, so they may register a replacement or enqueue work.
 * When replacing a live registration, keep the old context valid until any
 * already queued or in-flight operation that may have snapshotted it finishes.
 */
bool ble_op_register_callbacks(const ble_op_callbacks_t *callbacks,
                               void *context);

bool m_ble_op_start_adv(ble_adv_mode_t mode);
bool m_ble_op_stop_adv(void);
bool m_ble_op_disconnect(void);
bool m_ble_op_update_conn_param(void);

/**
 * Copy a binary payload and enqueue it for processing by the BLE task.
 *
 * A true return value means only that the request was accepted by the
 * asynchronous queue. It does not mean that the payload was transmitted.
 * This API is safe for concurrent normal-task callers, but not for ISR use.
 */
bool m_ble_op_tspp_send(const uint8_t *data, uint32_t len);

/**
 * Discard queued operations and release all payload ownership.
 */
void ble_op_queue_clear(void);

/**
 * Copy diagnostic counters into caller-provided storage.
 */
void ble_op_get_stats(ble_op_stats_t *stats);

#endif
