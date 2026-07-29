#include "ble_op_queue.h"

#include <limits.h>
#include <string.h>

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "evt.h"
#include "om_log.h"
#include "service_tspp_define.h"

#define BLE_OP_MSG_CNT 10U
#define BLE_OP_MSG_SIZE ((uint32_t)sizeof(ble_op_msg_t))
#define log_debug(...) om_log(OM_LOG_INFO, ##__VA_ARGS__)

typedef enum {
    BLE_OP_START_ADV = 0,
    BLE_OP_STOP_ADV,
    BLE_OP_DISCONNECT,
    BLE_OP_UPDATE_CONN_PARAM,
    BLE_OP_TSPP_SEND
} ble_op_cmd_t;

typedef struct {
    uint16_t cmd_id;
    uint16_t cmd_param;
    uint8_t *data;
    uint32_t data_len;
} ble_op_msg_t;

static osMessageQueueId_t ble_op_queue;
static osMutexId_t ble_op_control_mutex;
static ble_op_stats_t ble_op_stats;
static ble_op_callbacks_t ble_op_callbacks;
static void *ble_op_callback_context;
static ble_op_msg_t ble_op_pending_tspp;
static bool ble_op_pending_tspp_valid;
/*
* @brief 锁控制控制互斥锁
* */
static void ble_op_control_lock(void)
{
    if (ble_op_control_mutex != NULL) {
        (void)osMutexAcquire(ble_op_control_mutex, osWaitForever);
    }
}
/*
* @brief 解锁控制互斥锁
* */
static void ble_op_control_unlock(void)
{
    if (ble_op_control_mutex != NULL) {
        (void)osMutexRelease(ble_op_control_mutex);
    }
}
/*
* @brief 记录BLE操作队列结果
* @param accepted true if the message was accepted, false otherwise
* */
static void ble_op_record_queue_result(bool accepted)
{
    ble_op_control_lock();
    if (accepted) {
        ble_op_stats.queued++;
    } else {
        ble_op_stats.queue_rejected++;
    }
    ble_op_control_unlock();
}
/*
* @brief 记录TSPP操作结果
* @param result TSPP error code
* */
static void ble_op_record_tspp_result(uint8_t result)
{
    ble_op_control_lock();
    switch (result) {
        case TSPP_ERR_NO_ERROR:// 成功交给 TSPP FIFO 的次数
            ble_op_stats.tspp_sent++;
            break;
        case TSPP_ERR_DISABLED:// Notify 未开启导致的发送失败统计
            ble_op_stats.tspp_disabled++;
            break;
        case TSPP_ERR_FULL:// TSPP FIFO 空间不足统计
            ble_op_stats.tspp_full++;
            break;
        default:// 其他错误码统计
            ble_op_stats.tspp_other_error++;
            break;
    }
    ble_op_control_unlock();
}
/*
* @brief 释放BLE操作消息的payload
* @param msg Pointer to the message to process
* */
static void ble_op_release_payload(ble_op_msg_t *msg)
{
    if ((msg->cmd_id == BLE_OP_TSPP_SEND) && (msg->data != NULL)) {
        vPortFree(msg->data);
        msg->data = NULL;
    }
}
/*
* @brief 调用注册的回调函数
* @param msg Pointer to the message to process
* */
static void ble_op_dispatch_registered(ble_op_msg_t *msg)
{
    ble_op_callback_t callback = NULL;
    void *context;

    ble_op_control_lock();
    switch (msg->cmd_id) {
        case BLE_OP_START_ADV:
            callback = ble_op_callbacks.start_adv;
            break;
        case BLE_OP_STOP_ADV:
            callback = ble_op_callbacks.stop_adv;
            break;
        case BLE_OP_DISCONNECT:
            callback = ble_op_callbacks.disconnect;
            break;
        case BLE_OP_UPDATE_CONN_PARAM:
            callback = ble_op_callbacks.update_conn_param;
            break;
        default:
            break;
    }
    context = ble_op_callback_context;
    ble_op_control_unlock();

    if (callback != NULL) {
        callback(msg->cmd_param, msg->data, msg->data_len, context);
    }
}
/*
* @brief 处理BLE操作消息
* @param msg Pointer to the message to process
* */
static bool ble_op_process_tspp(ble_op_msg_t *msg)
{
    uint8_t result = TSPP_ERR_DISABLED;

    if (msg->data != NULL) {
        result = tspp_send(msg->data, (tspp_size_t)msg->data_len);
    }
    ble_op_record_tspp_result(result);
    if (result == TSPP_ERR_FULL) {
        return false;
    }
    ble_op_release_payload(msg);
    return true;
}

static bool ble_op_process(ble_op_msg_t *msg)
{
    switch (msg->cmd_id) {
        case BLE_OP_START_ADV:
        case BLE_OP_STOP_ADV:
        case BLE_OP_DISCONNECT:
        case BLE_OP_UPDATE_CONN_PARAM:
            ble_op_dispatch_registered(msg);
            return true;
        case BLE_OP_TSPP_SEND:
            return ble_op_process_tspp(msg);
        default:
            return true;
    }
}

static void ble_op_on_tspp_tx_ready(void)
{
    if (ble_op_queue != NULL) {
        evt_set(EVT_TYPE_BLE_OP);
    }
}
/*
* @brief BLE操作队列回调函数
* @param msg Pointer to the message to process
* @param msg_prio Message priority
* @param timeout Timeout in milliseconds
* @return true if the message was processed successfully, false otherwise
* */
static void ble_op_cb(void)
{
    ble_op_msg_t msg = {0};
    ble_op_msg_t *active_msg = NULL;
    uint8_t msg_prio = 0;
    bool processing_pending = false;
    bool completed;

    /*
     * Clear first. A producer that runs from this point onward sets a fresh
     * event that this callback will not erase.
     */
    evt_clear(EVT_TYPE_BLE_OP);

    if (ble_op_pending_tspp_valid) {
        active_msg = &ble_op_pending_tspp;
        processing_pending = true;
    } else if (osMessageQueueGet(ble_op_queue, &msg, &msg_prio, 0) == osOK) {
        active_msg = &msg;
    }

    if (active_msg != NULL) {
        completed = ble_op_process(active_msg);
        if (!completed && !processing_pending) {
            ble_op_pending_tspp = msg;
            ble_op_pending_tspp_valid = true;
        } else if (completed && processing_pending) {
            memset(&ble_op_pending_tspp, 0, sizeof(ble_op_pending_tspp));
            ble_op_pending_tspp_valid = false;
        }
    }

    /*
     * A message that was already queued before the clear, or arrived while
     * this callback was running, must keep the scheduler armed.
     */
    if (!ble_op_pending_tspp_valid &&
        (osMessageQueueGetCount(ble_op_queue) != 0U)) {
        evt_set(EVT_TYPE_BLE_OP);
    }
}
/*
* @brief 初始化BLE操作队列
* @return true if the queue was initialized successfully, false otherwise
* */
bool ble_op_init(void)
{
    osMessageQueueId_t new_queue;
    osMutexId_t new_stats_mutex;

    if (ble_op_queue != NULL) {
        return true;
    }

    new_stats_mutex = osMutexNew(NULL);
    if (new_stats_mutex == NULL) {
        return false;
    }

    new_queue = osMessageQueueNew(BLE_OP_MSG_CNT, BLE_OP_MSG_SIZE, NULL);
    if (new_queue == NULL) {
        (void)osMutexDelete(new_stats_mutex);
        return false;
    }

    memset(&ble_op_stats, 0, sizeof(ble_op_stats));
    memset(&ble_op_callbacks, 0, sizeof(ble_op_callbacks));
    memset(&ble_op_pending_tspp, 0, sizeof(ble_op_pending_tspp));
    ble_op_pending_tspp_valid = false;
    ble_op_callback_context = NULL;
    ble_op_control_mutex = new_stats_mutex;
    evt_callback_set(EVT_TYPE_BLE_OP, ble_op_cb);
    /*
     * Publish the queue last. Concurrent submitters either see NULL and
     * reject safely, or see a fully initialized queue and callback.
     */
    ble_op_queue = new_queue;
    tspp_tx_ready_callback_set(ble_op_on_tspp_tx_ready);
    return true;
}
/*
* @brief 注册BLE操作回调函数
* @param callbacks Pointer to the callbacks structure
* @param context Pointer to the context structure
* @return true if the callbacks were registered successfully, false otherwise
* */
bool ble_op_register_callbacks(const ble_op_callbacks_t *callbacks,
                               void *context)
{
    if (ble_op_queue == NULL) {
        return false;
    }

    ble_op_control_lock();
    if (callbacks == NULL) {
        memset(&ble_op_callbacks, 0, sizeof(ble_op_callbacks));
        ble_op_callback_context = NULL;
    } else {
        ble_op_callbacks = *callbacks;
        ble_op_callback_context = context;
    }
    ble_op_control_unlock();
    return true;
}
/*
* @brief 发送BLE操作消息
* @param msg Pointer to the message to send
* @return true if the message was sent successfully, false otherwise
* */
static bool ble_op_send(const ble_op_msg_t *msg)
{
    if ((ble_op_queue == NULL) || (msg == NULL)) {
        return false;
    }

    if (osMessageQueuePut(ble_op_queue, msg, 0, 0) != osOK) {
        ble_op_record_queue_result(false);
        return false;
    }

    ble_op_record_queue_result(true);
    evt_set(EVT_TYPE_BLE_OP);
    return true;
}
/*
* @brief 发送BLE操作命令
* @param command BLE operation command
* @return true if the command was sent successfully, false otherwise
* */
static bool ble_op_send_command(ble_op_cmd_t command, uint16_t cmd_param)
{
    ble_op_msg_t msg = {0};
    msg.cmd_id = (uint16_t)command;
    msg.cmd_param = cmd_param;
    return ble_op_send(&msg);
}
/*
* @brief 开始广播
* */
bool m_ble_op_start_adv(ble_adv_mode_t mode)
{
    if ((mode != BLE_ADV_MODE_FAST) && (mode != BLE_ADV_MODE_SLOW)) {
        return false;
    }
    return ble_op_send_command(BLE_OP_START_ADV, (uint16_t)mode);
}
/*
* @brief 停止广播
* */
bool m_ble_op_stop_adv(void)
{
    return ble_op_send_command(BLE_OP_STOP_ADV, 0U);
}
/*
* @brief 断开连接
* */
bool m_ble_op_disconnect(void)
{
    return ble_op_send_command(BLE_OP_DISCONNECT, 0U);
}
/*
* @brief 更新连接参数
* */
bool m_ble_op_update_conn_param(void)
{
    return ble_op_send_command(BLE_OP_UPDATE_CONN_PARAM, 0U);
}
/*
* @brief 发送TSPP数据
* @param data Pointer to the data to send
* @param len Length of the data
* @return true if the data was sent successfully, false otherwise
* */
bool m_ble_op_tspp_send(const uint8_t *data, uint32_t len)
{
    ble_op_msg_t msg = {0};
    bool accepted;

    if ((data == NULL) || (len == 0U) || (len > TSPP_BUFFER_SIZE) ||
        (ble_op_queue == NULL)) {
        return false;
    }

    msg.data = (uint8_t *)pvPortMalloc(len);
    if (msg.data == NULL) {
        ble_op_record_queue_result(false);
        return false;
    }

    memcpy(msg.data, data, len);
    msg.cmd_id = BLE_OP_TSPP_SEND;
    msg.data_len = len;
    accepted = ble_op_send(&msg);
    if (!accepted) {
        vPortFree(msg.data);
    }
    return accepted;
}
/*
* @brief 清空BLE操作队列
* */
void ble_op_queue_clear(void)
{
    ble_op_msg_t msg;
    uint8_t msg_prio;

    if (ble_op_queue == NULL) {
        return;
    }

    evt_clear(EVT_TYPE_BLE_OP);
    if (ble_op_pending_tspp_valid) {
        ble_op_release_payload(&ble_op_pending_tspp);
        memset(&ble_op_pending_tspp, 0, sizeof(ble_op_pending_tspp));
        ble_op_pending_tspp_valid = false;
    }
    while (osMessageQueueGet(ble_op_queue, &msg, &msg_prio, 0) == osOK) {
        ble_op_release_payload(&msg);
    }

    if (osMessageQueueGetCount(ble_op_queue) != 0U) {
        evt_set(EVT_TYPE_BLE_OP);
    }
}
/*
* @brief 获取BLE操作队列统计信息
* @param stats Pointer to store the statistics
* */
void ble_op_get_stats(ble_op_stats_t *stats)
{
    if (stats == NULL) {
        return;
    }
    ble_op_control_lock();
    *stats = ble_op_stats;
    ble_op_control_unlock();
}
/*
* @brief 打印BLE操作队列统计信息
* */
void ble_op_print_stats(void)
{
    ble_op_stats_t stats;
    ble_op_get_stats(&stats);
    log_debug("queued=%u rejected=%u\r\n",
            stats.queued,
            stats.queue_rejected);

    log_debug("sent=%u disabled=%u full=%u other=%u\r\n",
            stats.tspp_sent,
            stats.tspp_disabled,
            stats.tspp_full,
            stats.tspp_other_error);
} 
