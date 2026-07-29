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
#include <string.h>
#include "omble.h"
#include "ancs_client.h"


/*********************************************************************
 * MACROS
 */
#define HDL_CHAR_CMD_OFFSET    2
#define HDL_CHAR_NOTIFY_OFFSET 5
#define HDL_DESC_NOTIFY_OFFSET 6
#define HDL_CHAR_DATA_OFFSET   8
#define HDL_DESC_DATA_OFFSET   9
#define ANCS_HANDLE_NUM        10

/*********************************************************************
 * LOCAL VARIABLES
 */
static const uint8_t serv_ancs[]     = {0xD0, 0x00, 0x2D, 0x12, 0x1E, 0x4B, 0x0F, 0xA4, 0x99, 0x4E, 0xCE, 0xB5, 0x31, 0xF4, 0x05, 0x79};

static uint16_t m_conn_idx;
static uint16_t m_start_handle;
static ancs_data_t m_ancs_data;
static uint8_t  m_state;
static uint8_t  m_parse;
static uint32_t perform_uid;
static uint8_t perform_act;
// pending uid
#define MAX_PENDING_UID_NUM (1<<3)
#define MOD_IDX(n)          ((n)&(MAX_PENDING_UID_NUM-1))
static uint32_t pending_uid[MAX_PENDING_UID_NUM];
static uint8_t  fr, ra;

/*********************************************************************
 * TYPEDEFS
 */
enum {
    ANCS_ST_INIT,
    ANCS_ST_DISC,
    ANCS_ST_WAIT_ENC,
    ANCS_ST_EN_DESC1,
    ANCS_ST_EN_DESC2,
    ANCS_ST_WAIT_MSG,
    ANCS_ST_PARSING,
    ANCS_ST_ERROR,
};

enum {
    PARSE_ST_IDLE,
    PARSE_ST_HEADER,
    PARSE_ST_ITEMS,
    PARSE_ST_SUCCESS,
    PARSE_ST_FAIL,
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static void push_pending_uid(uint32_t uid)
{
    if (MOD_IDX(ra + 1) == fr) { // buffer full
        fr = MOD_IDX(fr + 1); // drop one
    }
    pending_uid[ra] = uid;
    ra = MOD_IDX(ra + 1);
}

static uint8_t peek_pending_uid(uint32_t *uid)
{
    if (ra == fr) { // buffer empty
        return false;
    }
    *uid = pending_uid[fr];
    return true;
}

static void pop_pending_uid(void)
{
    if (ra != fr) { // buffer not empty
        fr = MOD_IDX(fr + 1);
    }
}

#define ANCS_ENC_UINT8(p,u8)    {*(p)++ = (uint8_t)(u8);}
#define ANCS_ENC_UINT16(p,u16)  {*(p)++ = (uint8_t)(u16); *(p)++ = (uint8_t)((u16) >> 8);}
#define ANCS_ENC_UINT32(p,u32)  {*(p)++ = (uint8_t)(u32); *(p)++ = (uint8_t)((u32) >> 8); *(p)++ = (uint8_t)((u32) >> 16); *(p)++ = (uint8_t)((u32) >> 24);}
#define ANCS_DEC_UINT32(buf)    ( ((uint32_t) ((uint8_t *)(buf))[0]) | (((uint32_t)((uint8_t *)(buf))[1]) << 8) | (((uint32_t)((uint8_t *)(buf))[2]) << 16) | (((uint32_t) ((uint8_t *)(buf))[3])) << 24)

static bool ancs_check_timeout(uint8_t is_check) // false:set, true:check
{
    static uint32_t mark_time; // mark_time == 0 meas no attribute request pending
    if (is_check) {
        size_t now = omble_tick_get();
        if (omble_tick_diff_10ms(mark_time, now) > (1000)) { // get message timeout(about 10s)
            return true;
        } else {
            return false;
        }
    } else {
        mark_time = omble_tick_get();
        return false;
    }
}

static void ancs_parse_notify(ancs_notify_t *ntf, const uint8_t *value)
{
    ntf->evt_id = *value++;
    ntf->evt_flags = *value++;
    ntf->cate_id = *value++;
    ntf->cate_count = *value++;
    ntf->notify_uid = ANCS_DEC_UINT32(value);
}

static uint8_t *get_buf(ancs_data_t *data, uint8_t attr)
{
    switch (attr) {
        case ANCS_NTF_ATTR_ID_APP_IDENTIFIER: return data->app_id;
        case ANCS_NTF_ATTR_ID_TITLE:          return data->title;
        case ANCS_NTF_ATTR_ID_SUBTITLE:       return data->subtitle;
        case ANCS_NTF_ATTR_ID_MESSAGE:        return data->msg;
        case ANCS_NTF_ATTR_ID_DATE:           return data->date;
        default:                              return NULL;
    }
}

static uint8_t get_max_item_len(uint8_t attr)
{
    switch (attr) {
        case ANCS_NTF_ATTR_ID_APP_IDENTIFIER: return ANCS_APPID_CAP;
        case ANCS_NTF_ATTR_ID_TITLE:          return ANCS_TITLE_CAP;
        case ANCS_NTF_ATTR_ID_SUBTITLE:       return ANCS_SUBTITLE_CAP;
        case ANCS_NTF_ATTR_ID_MESSAGE:        return ANCS_MSG_CAP;
        case ANCS_NTF_ATTR_ID_DATE:           return ANCS_DATE_CAP;
        default:                              return 0;
    }
}

static uint8_t ancs_parse_data(const uint8_t *value, uint8_t len)
{
    static uint8_t header[3];
    static uint8_t header_len;
    static uint8_t item_recv_len;
    uint8_t i = 0; // index of value
    while (len > 0) {
        if (m_parse == PARSE_ST_IDLE) {
            memset(&m_ancs_data, 0, sizeof(ancs_data_t));
            if (len < 5 || *value != ANCS_COMMAND_ID_GET_NTF_ATTR) { // format error
                m_parse = PARSE_ST_IDLE;
                return PARSE_ST_FAIL;
            }
            i++; len--; // skip id
            m_ancs_data.notify_uid = ANCS_DEC_UINT32(&value[i]); // decode uid
            i += 4; len -= 4;
            header_len = 0;
            m_parse = PARSE_ST_HEADER;
        }
        if (m_parse == PARSE_ST_HEADER) {
            if (header_len + len  < 3) {
                memcpy(&header[header_len], &value[i], len);
                header_len += len;
                i += len;
                return m_parse; // more data needed
            } else {
                memcpy(&header[header_len], &value[i], 3 - header_len);
                i += 3 - header_len; len -= 3 - header_len;
                header_len += 3 - header_len;
                uint16_t item_len = header[1] + (header[2] << 8);
                if (item_len == 0) { // length of attribute is 0, nothing to parse
                    header_len = 0;
                } else if (header[0] != ANCS_NTF_ATTR_ID_APP_IDENTIFIER
                           && get_max_item_len(header[0]) < item_len) { // length exceed capability
                    m_parse = PARSE_ST_IDLE;
                    return PARSE_ST_FAIL;
                } else {
                    uint8_t *p = get_buf(&m_ancs_data, header[0]);
                    if (!p) { // error: attr not support
                        m_parse = PARSE_ST_IDLE;
                        return PARSE_ST_FAIL;
                    } else {
                        *(p - 1) = 0; // reset length of attribute
                        item_recv_len = 0;
                        m_parse = PARSE_ST_ITEMS;
                    }
                }
            }
        }
        if (m_parse == PARSE_ST_ITEMS) {
            uint8_t *p = get_buf(&m_ancs_data, header[0]);
            if (header_len == 0 || !p) {
                m_parse = PARSE_ST_IDLE;
                return PARSE_ST_FAIL;
            }
            uint8_t *recv_len = p - 1;
            uint16_t item_len = header[1] + (header[2] << 8);

            uint8_t max_item_len = get_max_item_len(header[0]);
            uint8_t parse_len = item_len - *recv_len;
            parse_len  = parse_len < len ? parse_len : len;
            uint8_t copy_len = parse_len < (max_item_len - *recv_len) ? parse_len : (max_item_len - *recv_len);
            memcpy(&p[*recv_len], &value[i], copy_len);
            i += parse_len; len -= parse_len;
            *recv_len += copy_len;
            item_recv_len += parse_len;
            if (item_recv_len == item_len) {
                if (m_ancs_data.msg_len == item_len) {
                    m_parse = PARSE_ST_IDLE;
                    return PARSE_ST_SUCCESS;
                } else {
                    header_len = 0;
                    m_parse = PARSE_ST_HEADER;
                }
            }
        }
    }
    return m_parse;
}

static uint8_t req_data_encode(uint8_t *buf, uint32_t uid)
{
    uint8_t *p = buf;
    ANCS_ENC_UINT8(p, ANCS_COMMAND_ID_GET_NTF_ATTR);
    ANCS_ENC_UINT32(p, uid);
    ANCS_ENC_UINT8(p, ANCS_NTF_ATTR_ID_APP_IDENTIFIER);
    ANCS_ENC_UINT16(p, ANCS_APPID_CAP);
    ANCS_ENC_UINT8(p, ANCS_NTF_ATTR_ID_TITLE);
    ANCS_ENC_UINT16(p, ANCS_TITLE_CAP);
    ANCS_ENC_UINT8(p, ANCS_NTF_ATTR_ID_SUBTITLE);
    ANCS_ENC_UINT16(p, ANCS_SUBTITLE_CAP);
    ANCS_ENC_UINT8(p, ANCS_NTF_ATTR_ID_DATE);
    ANCS_ENC_UINT16(p, ANCS_DATE_CAP);
    ANCS_ENC_UINT8(p, ANCS_NTF_ATTR_ID_MESSAGE);
    ANCS_ENC_UINT16(p, ANCS_MSG_CAP);
    return p - buf;
}

static void req_perform()
{
    if (perform_uid != 0xFFFFFFFF) {
        if (m_state != ANCS_ST_WAIT_MSG) {
            return;
        }
        uint16_t handle = m_start_handle + HDL_CHAR_CMD_OFFSET;
        uint8_t req[] = {
            ANCS_COMMAND_ID_PERFORM_NTF_ACT,
            (perform_uid >> 0) & 0xFF,  (perform_uid >> 8) & 0xFF,
            (perform_uid >> 16) & 0xFF, (perform_uid >> 24) & 0xFF,
            perform_act,
        };
        uint32_t res = ob_gattc_write(m_conn_idx, handle, OB_GATTC_WRITE_REQ, req, sizeof(req));
        if (res == OB_ERROR_NO_ERR) {
            perform_uid = 0xFFFFFFFF;
        }
    }
}

static uint8_t ancs_set_state(uint8_t new_state)
{
    uint8_t res = OB_ERROR_NO_ERR;
    if (m_state == ANCS_ST_INIT && new_state == ANCS_ST_DISC) {
        uint8_t enc_state;
        res = ob_gap_get_encrypt_state(m_conn_idx, &enc_state);
        if (res == OB_ERROR_NO_ERR && enc_state != 0) {
            res = ob_gattc_find_service_by_uuid(m_conn_idx, 0x0001, 0xFFFF, serv_ancs, sizeof(serv_ancs));
            log_debug("[ANCS]: Find service.\n");
        } else {
            return res;
        }
    } else if (m_state == ANCS_ST_DISC && new_state == ANCS_ST_WAIT_ENC) {
        m_state  = ANCS_ST_WAIT_ENC;
        return ancs_set_state(ANCS_ST_EN_DESC1);
    } else if ((m_state == ANCS_ST_WAIT_ENC && new_state == ANCS_ST_EN_DESC1) ||
               (m_state == ANCS_ST_EN_DESC1 && new_state == ANCS_ST_EN_DESC2)) {
        uint8_t enc_state;
        res = ob_gap_get_encrypt_state(m_conn_idx, &enc_state);
        if (res == OB_ERROR_NO_ERR && enc_state != 0) {
            uint16_t enable_desc = 0x0001;
            uint16_t handle = m_start_handle + (new_state == ANCS_ST_EN_DESC1 ? HDL_DESC_DATA_OFFSET : HDL_DESC_NOTIFY_OFFSET );
            log_debug("[ANCS] Write handle 0x%04X\n", handle);
            res = ob_gattc_write(m_conn_idx, handle, OB_GATTC_WRITE_REQ, (uint8_t *)&enable_desc, sizeof(enable_desc));
            if (res == OB_ERROR_NO_ERR) {
                m_state = new_state;
                if (new_state == ANCS_ST_EN_DESC1) {
                    return ancs_set_state(ANCS_ST_EN_DESC2);
                } else { // new_state == ANCS_ST_EN_DESC2
                    return ancs_set_state(ANCS_ST_WAIT_MSG);
                }
            }
        }
    } else if ((m_state == ANCS_ST_EN_DESC2 || m_state == ANCS_ST_PARSING) && new_state == ANCS_ST_WAIT_MSG) {
        m_parse = PARSE_ST_IDLE;
    } else if (m_state == ANCS_ST_WAIT_MSG && new_state == ANCS_ST_PARSING) {
        ancs_check_timeout(false);
    }
    if (res == OB_ERROR_NO_ERR) {
        m_state = new_state;
    }
    return res;
}

static void ancs_get_pending_data(void)
{
    uint32_t uid;
    if (m_state == ANCS_ST_PARSING && ancs_check_timeout(true)) {
        ancs_set_state(ANCS_ST_WAIT_MSG);
    }
    if (peek_pending_uid(&uid)) {
        if (m_state != ANCS_ST_WAIT_MSG) {
            return;
        }
        log_debug("[ANCS]: Get data UID: 0x%02X\n", uid);
        uint16_t handle = m_start_handle + HDL_CHAR_CMD_OFFSET;
        uint8_t buf[32], len = req_data_encode(buf, uid);
        uint32_t res = ob_gattc_write(m_conn_idx, handle, OB_GATTC_WRITE_REQ, buf, len);
        if (res == OB_ERROR_NO_ERR) {
            pop_pending_uid();
            ancs_set_state(ANCS_ST_PARSING);
        }
    }
}

static void ancs_event_cb(uint16_t evt_id, const omble_evt_t *evt)
{
    switch (evt_id) {
        case OB_GATT_EVT_TX_COMPLETE:
            if (1) {
                break;
            } else {
                // no break;
            }
        case OB_GATTC_EVT_WRITE_RSP:
            switch (m_state) {
                case ANCS_ST_WAIT_MSG: req_perform(); ancs_get_pending_data(); break;
                case ANCS_ST_ERROR   : break;
                case ANCS_ST_INIT    : ancs_set_state(ANCS_ST_DISC); break;
                case ANCS_ST_WAIT_ENC: ancs_set_state(ANCS_ST_EN_DESC1); break;
                case ANCS_ST_EN_DESC1: ancs_set_state(ANCS_ST_EN_DESC2); break;
            }
            break;
        case OB_GAP_EVT_CONNECTED:
            m_conn_idx = evt->gap.conn_idx;
            fr = ra = 0;
            m_state = ANCS_ST_INIT;
            ancs_set_state(ANCS_ST_DISC);
            break;
        case OB_GAP_EVT_DISCONNECTED:
            m_state = ANCS_ST_ERROR;
            break;
        case OB_GATTC_EVT_FIND_SERV_BY_UUID_RSP:
            if (m_state == ANCS_ST_INIT) {
                ancs_set_state(ANCS_ST_DISC);
            } else if (m_state != ANCS_ST_DISC) {
                break; // Other profile execute discovery
            }
            {
                const ob_gattc_evt_find_serv_by_uuid_rsp_t *p = &evt->gatt.find_serv_by_uuid_rsp;
                if ( p->status == OB_ERROR_NO_ERR && p->service_num && p->service->uuid_len == 16 &&
                        !memcmp(p->service->uuid, serv_ancs, 16)) {
                    ob_gatt_service_t *serv = p->service;
                    m_start_handle = serv->start_hdl;
                    if (serv->end_hdl - serv->start_hdl != ANCS_HANDLE_NUM - 1) {
                        ancs_set_state(ANCS_ST_ERROR);
                        log_debug("[ANCS]: Service not found\n");
                    } else {
                        log_debug("[ANCS]: Remote ANCS start handle: 0x%04X\n", m_start_handle);
                        ancs_set_state(ANCS_ST_WAIT_ENC);
                    }
                }
            }
            break;
        case OB_GAP_EVT_ENCRYPT:
            if (evt->gap.encrypt.encrypted) {
                if (m_state == ANCS_ST_INIT) {
                    ancs_set_state(ANCS_ST_DISC);
                } else {
                    ancs_set_state(ANCS_ST_EN_DESC1);
                }
            }
            break;
        case OB_GATTC_EVT_HVX_IND: {
            const ob_gattc_evt_hvx_ind_t *hvx = &evt->gatt.hvx_ind;
            if (hvx->att_hdl == m_start_handle + HDL_CHAR_NOTIFY_OFFSET && hvx->len == sizeof(ancs_notify_t)) {
                ancs_notify_t notify;
                ancs_parse_notify(&notify, hvx->data);
                ancs_notify_evt(&notify);
            } else if (hvx->att_hdl == m_start_handle + HDL_CHAR_DATA_OFFSET) {
                uint8_t state = ancs_parse_data(hvx->data, hvx->len);
                if (state == PARSE_ST_SUCCESS) {
                    ancs_data_evt(&m_ancs_data);
                    ancs_set_state(ANCS_ST_WAIT_MSG);
                    ancs_get_pending_data();
                } else if (state == PARSE_ST_FAIL) {
                    ancs_get_pending_data();
                }
            }
        }   break;
    }
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
void ancs_client_init(void)
{
    ob_event_callback_reg(ancs_event_cb);
}

void ancs_get_data(uint32_t uid)
{
    push_pending_uid(uid);
    ancs_get_pending_data();
}

void ancs_perform_act(uint32_t uid, uint8_t act)
{
    perform_uid = uid;
    perform_act = act;
    req_perform();
}

__WEAK void ancs_notify_evt(ancs_notify_t *notify)
{
    log_debug("[ANCS]: Notify-> UID:0x%02X ID:%d Flag:0x%02X cate:%d cnt:%d\n",
              notify->notify_uid, notify->evt_id, notify->evt_flags, notify->cate_id, notify->cate_count);
    if (notify->evt_flags && (notify->evt_id == ANCS_EVENT_ID_NOTIFICATION_ADDED)) {
        ancs_get_data(notify->notify_uid);
    }
}
__WEAK void ancs_data_evt(ancs_data_t *data)
{
    log_debug("[ANCS]: Data  -> UID:0x%02X ", data->notify_uid);
    log_debug("Date: %s ", data->date);
    log_debug("Msg length: %d\n", data->msg_len);
}

/** @} */
