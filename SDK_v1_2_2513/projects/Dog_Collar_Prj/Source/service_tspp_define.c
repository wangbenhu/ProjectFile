/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */
/*******************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <string.h>
#include "omble.h"
#include "service_tspp_define.h"
#include "om_fifo.h"
#include "om_log.h"
#include "cmsis_os2.h"
#include "ble_op_queue.h"
#include "common_def.h"

#define log_debug(...) om_log(OM_LOG_INFO, ##__VA_ARGS__)
/*********************************************************************
 * LOCAL VARIABLES
 */
/*** tspp fifo ***/
static uint8_t tspp_buffer[TSPP_BUFFER_SIZE];
static om_fifo_t tspp_fifo;

// Start handle
static uint16_t m_tspp_start_handle;
static uint16_t m_trans_enabled;
static uint8_t m_tspp_mtu;
static uint8_t m_tspp_send_pending;
static tspp_tx_ready_callback_t m_tspp_tx_ready_callback;
static void tspp_transmit(void);
extern bool isBleTsppCommplete;
extern void app_tspp_data_sent_ind_handler(void);
extern void ble_dataDown_handler(uint8_t *str, uint16_t len);
/*********************************************************************
 * TYPEDEFS
 */
enum {
    IDX_TSPP_SVC,
    IDX_TSPP_COMMAND_CHAR,
    IDX_TSPP_COMMAND_VAL,
    IDX_TSPP_NOTIFY_CHAR,
    IDX_TSPP_NOTIFY_VAL,
    IDX_TSPP_NOTIFY_DESC,
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void service_tspp_init(void)
{
    static const uint8_t serv_tspp[2]    = {0x01, 0xFF};
    static const uint8_t tspp_command[2] = {0x02, 0xFF};
    static const uint8_t tspp_notify[2]  = {0x03, 0xFF};
    static const ob_gatt_item_t atts_tspp[] = {
        { ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
        { tspp_command,    OB_UUID_16BIT, OB_ATT_PROP_WRITE_CMD },
        { ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
        { tspp_notify,     OB_UUID_16BIT, OB_ATT_PROP_NTF },
        { ob_att_cccd_def, OB_UUID_16BIT, OB_ATT_PROP_READ | OB_ATT_PROP_WRITE },
    };
    static const ob_gatt_serv_t att_serv_hid = {
        serv_tspp, OB_UUID_16BIT,
        sizeof(atts_tspp) / sizeof(atts_tspp[0]), atts_tspp
    };
    ob_gatts_add_service(&att_serv_hid, &m_tspp_start_handle);
    om_fifo_init(&tspp_fifo, tspp_buffer, TSPP_BUFFER_SIZE);
}


static void tspp_event_cb(uint16_t evt_id, const omble_evt_t *evt)
{
    if (evt_id == OB_GATTS_EVT_READ_REQ) {
        const ob_gatts_evt_read_req_t *req = &evt->gatt.read_req;
        if (req->att_hdl == m_tspp_start_handle + IDX_TSPP_NOTIFY_DESC) {
            ob_gatts_read_response(evt->gatt.conn_idx, OB_GATT_ERR_NO_ERROR, (uint8_t *)&m_trans_enabled, sizeof(uint16_t));
        }
    } else if (evt_id == OB_GATTS_EVT_WRITE_REQ) {
        const ob_gatts_evt_write_req_t *req = &evt->gatt.write_req;
        if (req->att_hdl == m_tspp_start_handle + IDX_TSPP_COMMAND_VAL) {
            ob_gatts_write_response(evt->gatt.conn_idx, OB_GATT_ERR_NO_ERROR);
            app_tspp_write_cmd_ind_handler(req->data, req->len);
        } else if (req->att_hdl == m_tspp_start_handle + IDX_TSPP_NOTIFY_DESC) {
            ob_gatts_write_response(evt->gatt.conn_idx, OB_GATT_ERR_NO_ERROR);
            m_trans_enabled = req->data[0];
			log_debug("m_trans_enabled = %d\r\n",m_trans_enabled);
//            app_tspp_status_ind_handler(!!m_trans_enabled);
        }
    } else if (evt_id == OB_GAP_EVT_CONNECTED) {
        m_trans_enabled = false;
        m_tspp_send_pending = false;
        m_tspp_mtu = 20;
		m_ble_op_update_conn_param();
        #if 0
        ob_gap_data_length_update(evt->gap.conn_idx, 251, 0x2000);
        ob_gap_phys_t phys = {0, 1};
        ob_gap_phy_update(evt->gap.conn_idx, phys, phys);
        #endif
    } else if (evt_id == OB_GAP_EVT_DISCONNECTED) {
        m_trans_enabled = false;
    } else if (evt_id == OB_GATT_EVT_TX_COMPLETE) {
        if (evt->gatt.tx_complete.id == m_tspp_start_handle) {
			isBleTsppCommplete = true;
            m_tspp_send_pending = false;
            tspp_transmit();
            if (m_tspp_tx_ready_callback != NULL) {
                m_tspp_tx_ready_callback();
            }
        }
    } else if (evt_id == OB_GATT_EVT_MTU_EXCHANGED) {
        m_tspp_mtu = evt->gatt.mtu_exchanged.mtu - 3;
		
    }
}

static void tspp_transmit(void)
{
    tspp_size_t data_len = om_fifo_len(&tspp_fifo);
    if (data_len) {
        if (m_tspp_send_pending) {
            return;
        }
        uint16_t send_len;
        #if TSPP_STREAM_MODE
        send_len = m_tspp_mtu < TSPP_MAX_MTU_SIZE ? m_tspp_mtu : TSPP_MAX_MTU_SIZE;
        if (send_len > data_len) {
            send_len  = data_len;
        }
        #else
        tspp_size_t len;
        om_fifo_out(&tspp_fifo, &len, sizeof(len));
        send_len = len;
        #endif
        uint8_t send_data[TSPP_MAX_MTU_SIZE];
        send_len = om_fifo_out(&tspp_fifo, send_data, send_len);
        ob_gatts_hvx_t hvx = {
            OB_HANDLE_VALUE_NTF,
            m_tspp_start_handle + IDX_TSPP_NOTIFY_VAL, // handle
            send_data,
            send_len,
            m_tspp_start_handle,
        };
		//osKernelLock();   
        ob_gatts_send_hvx(0, &hvx);
        m_tspp_send_pending = true;
		//osKernelUnlock();
    }
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
uint8_t tspp_send(uint8_t *data, tspp_size_t len)
{
    #if !TSPP_STREAM_MODE
    if (len > m_tspp_mtu || len > TSPP_MAX_MTU_SIZE) {
        return TSPP_ERR_EXCEED_MTU;
    }
    #endif
	
    if (!m_trans_enabled) {// Notify 未开启导致的发送失败统计
        return TSPP_ERR_DISABLED;
    }
    tspp_size_t avail_len = tspp_avail();
	log_debug("m_trans_enabled = %d %d %d\r\n",m_trans_enabled,avail_len,len);
    if (len > avail_len) {// TSPP FIFO 空间不足统计
        return TSPP_ERR_FULL;
    }
    #if TSPP_STREAM_MODE
    om_fifo_in(&tspp_fifo, data, len);
    #else
    om_fifo_in(&tspp_fifo, &len, sizeof(len));
    om_fifo_in(&tspp_fifo, data, len);
    #endif
    tspp_transmit();
    return TSPP_ERR_NO_ERROR;
}

void tspp_tx_ready_callback_set(tspp_tx_ready_callback_t callback)
{
    m_tspp_tx_ready_callback = callback;
}
void tspp_clear(void)
{
    om_fifo_reset(&tspp_fifo);
}

uint16_t tspp_avail(void)
{
    tspp_size_t avail_len = om_fifo_avail(&tspp_fifo);
    #if !TSPP_STREAM_MODE
    if (avail_len < sizeof(avail_len)) {
        avail_len = 0;
    } else {
        avail_len -= sizeof(avail_len);
    }
    #endif
    return avail_len;
}

/*lint -save -e661 */
#if TSPP_UPLOAD_TEST
#define TEST_SEND_SIZE (128 << 10)
static int send_cnt;
void app_tspp_data_sent_ind_handler(void)
{
    if (m_trans_enabled) {
        int i;
        uint32_t tmp[TSPP_MAX_MTU_SIZE / sizeof(uint32_t)];
//        while (send_cnt < (int)(TEST_SEND_SIZE / sizeof(uint32_t))) {
//            uint16_t send_len;
//            send_len = tspp_avail();
//            send_len = send_len < TSPP_MAX_MTU_SIZE ? send_len : TSPP_MAX_MTU_SIZE;
//            if (send_len < sizeof(uint32_t)) {
//                break;
//            }
//            for (i = 0; i < (int)(send_len / sizeof(uint32_t)); i++) {
//                tmp[i] = send_cnt++;
//                if (send_cnt == (int)(TEST_SEND_SIZE / sizeof(uint32_t))) {
//                    send_len = (i + 1) * sizeof(uint32_t);
//                    break;
//                }
//            }
////            if (tspp_send((uint8_t *)tmp, send_len) != TSPP_ERR_NO_ERROR) {
////                break;
////            }
//        }
    }
}
void app_tspp_status_ind_handler(uint8_t enabled)
{
    log_debug("Tspp status: %d\n", enabled);
    if (enabled) {
        send_cnt = 0;
    }
    app_tspp_data_sent_ind_handler();
}


#endif
/*lint -restore */
void app_tspp_write_cmd_ind_handler(const uint8_t *data, uint16_t len)
{
	ble_dataDown_handler((uint8_t *)data, len);
}

void app_tspp_init(void)
{
    service_tspp_init();
    ob_event_callback_reg(tspp_event_cb);
}

/** @} */
