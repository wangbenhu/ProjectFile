/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup DOC DOC
 * @ingroup  DOCUMENT
 * @brief    ble manual calibration
 * @details  ble manual calibration
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

 /*******************************************************************************
  * INCLUDES
  */
#include "om_driver.h"
#include "evt.h"
#include "ob_config.h"
#include "omble.h"
#include "nvds.h"
#include "om_log.h"
#include "manu_calib_ble_v2.h"

#define LOG_FREQ_CALIB_ENABLE    (1)

#if LOG_FREQ_CALIB_ENABLE
#define LOG_FREQ_CALIB              OM_LOG
#define LOG_ARRY_FREQ_CALIB         OM_LOG_HEXDUMP
#else
#define LOG_FREQ_CALIB(...)
#define LOG_ARRY_FREQ_CALIB(...)
#endif


#define FQ_UART_RX_EVT_ID       ((evt_type_t)(EVT_TYPE_USR_FIRST+0))
#define PIN_FQ_UART_TX                    5
#define PIN_FQ_UART_RX                    6
#define PIN_FQ_ENTRY_DET                  4

#define FQ_UART                        OM_UART1
#define FQ_UART_BAUDRATE               115200
#define UART_CMD_BUF_LEN               32
#define UART_CMD_HEADER_LEN            4
#define CMD_START                      0xFF // the start byte of comman packet
#define EVT_START                      0xEE // the start byte of event packet
#define CMD_FREQ_CALIB                 0x08
#define BLE_CONN_HANDLE_INVALID        0xFF

typedef enum {
    FQ_RF_STATE_IDLE =0,
    FQ_RF_STATE_ADV,
    FQ_RF_STATE_CONNECTED,
}fq_rf_state_t;

enum {
    STATUS_SUCCESS,
    STATUS_FAILED,
};

enum {
    RESPONSE_TYPE_DUT_INFO = 0,
    RESPONSE_TYPE_CALIB_RESULT,
};


static uint8_t m_conn_handle=BLE_CONN_HANDLE_INVALID;  // BLE_CONN_HANDLE_INVALID
static fq_rf_state_t m_fq_state=FQ_RF_STATE_IDLE;
static uint16_t m_char_local_handle;
static uint16_t m_char_peer_handle;
static ob_gap_addr_t m_peer_addr;
static uint8_t uart_rx_buf[UART_CMD_BUF_LEN];
static uint8_t uart_rx_idx, cmd_pkt_len;
static uint8_t m_xtal_32m_ctune;

// static const char CALIB_END_SUCCESS[]="success";

/// Advertise data
static uint8_t sdata[] = {
    /* Flags: BLE limited discoverable mode and BR/EDR not supported */
    0x02, 0x01, 0x06,
    /* Complete Local Name */
    11, 0x09,
    'F','r','e','q',' ','C','a','l','i','b',
};

static ob_adv_param_t adv_param;
static ob_data_t adv_data = { sdata, sizeof(sdata) };
// Start handle
static uint16_t m_start_handle;
static uint8_t scr_data[] = {
    0x02, 0x0A, 0x00,
};
static ob_data_t scan_rsp_data = { scr_data, sizeof(scr_data) };

static const uint16_t crc16tab[256] =
{
    0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
    0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
    0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
    0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
    0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
    0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
    0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
    0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
    0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
    0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
    0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
    0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
    0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
    0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
    0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
    0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
    0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
    0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
    0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
    0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
    0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
    0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
    0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
    0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
    0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
    0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
    0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
    0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
    0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
    0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
    0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
    0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
};


static uint16_t om_crc16_ccitt(uint16_t crcinit, const void *buf, unsigned len)
{
    register unsigned counter;
    register uint16_t crc = crcinit;
    const uint8_t *__buf = buf;

    for( counter = 0; counter < len; counter++)
        crc = (crc<<8) ^ crc16tab[((crc>>8) ^ *(uint8_t *)__buf++) & 0x00FF];

    return crc;
}


static void fq_uart1_rx_data_handler(void *om_usart, drv_event_t event, void *rx_buf, void *rx_cnt)
{
    if (event & DRV_EVENT_COMMON_READ_COMPLETED) {
        while (rx_cnt--) {
            if (uart_rx_idx < UART_CMD_BUF_LEN) {
                uart_rx_buf[uart_rx_idx++] = *((uint8_t *)rx_buf++);
            }
            if (uart_rx_buf[0] != CMD_START) {
                uart_rx_idx = cmd_pkt_len = 0;
            } else if (uart_rx_idx == UART_CMD_HEADER_LEN) {
                cmd_pkt_len = uart_rx_buf[3] << 8 | uart_rx_buf[2]; //data length
                cmd_pkt_len += 6;         //4 bytes header, and 2 byte checkcrc at tail
                if (cmd_pkt_len > UART_CMD_BUF_LEN)
                    cmd_pkt_len=UART_CMD_BUF_LEN;
            } else if (uart_rx_idx == cmd_pkt_len) { //a complete packet received
                LOG_ARRY_FREQ_CALIB(OM_LOG_DEBUG, uart_rx_buf, cmd_pkt_len, 16);
                //Check the packet if right or not
                if ((uint16_t)(uart_rx_buf[cmd_pkt_len - 2] | (uart_rx_buf[cmd_pkt_len - 1] << 8))
                        == om_crc16_ccitt(0, uart_rx_buf, cmd_pkt_len - 2)) {
                    evt_set(FQ_UART_RX_EVT_ID);
                }
            }
        }
    }
}


static void fq_hardware_init(void)
{

    pin_config_t fq_pin_config[] = {
        {PIN_FQ_UART_TX, {PINMUX_PAD5_UART1_TRX_CFG}, PMU_PIN_MODE_PU, PMU_PIN_DRIVER_CURRENT_NORMAL},
        {PIN_FQ_UART_RX, {PINMUX_PAD6_UART1_RX_CFG}, PMU_PIN_MODE_PU, PMU_PIN_DRIVER_CURRENT_NORMAL},
    };
    // UART1
    drv_pin_init(fq_pin_config, sizeof(fq_pin_config) / sizeof(fq_pin_config[0]));
    // Init UART
    uart_config_t uart_cfg = {
        .baudrate       = 115200,
        .flow_control   = UART_FLOW_CONTROL_NONE,
        .data_bit       = UART_DATA_BIT_8,
        .stop_bit       = UART_STOP_BIT_1,
        .parity         = UART_PARITY_NONE,
        .half_duplex_en = 0,
        .lin_enable     = 0,
    };
    drv_uart_init(FQ_UART, &uart_cfg);
    drv_uart_register_isr_callback(FQ_UART, fq_uart1_rx_data_handler);
    drv_uart_read_int(FQ_UART, NULL, 0);
}


static void fq_uart1_cmd_response(uint8_t cmd, uint8_t status, uint8_t *pdata, uint16_t len)
{
    uint8_t buf[64] = { 0 };
    if((len+7)>sizeof(buf)) {
        return;
    }

    buf[0] = EVT_START;
    buf[1] = cmd;
    buf[2] = status;
    if (len != 0) {
        buf[3] = len & 0xff;
        buf[4] = (len >> 8) & 0xff;
        memcpy(buf + 5, pdata, len);
    }
    uint16_t crc16 = om_crc16_ccitt(0, buf, len + 5);
    buf[5 + len] = (uint8_t)crc16;
    buf[6 + len] = (uint8_t)(crc16 >> 8);
    drv_uart_write(FQ_UART, (const uint8_t *)buf, len + 7, DRV_MAX_DELAY);
    OM_LOG(OM_LOG_DEBUG, "response: ");
    OM_LOG_HEXDUMP(OM_LOG_DEBUG, buf, len+7, 16);
}

static uint8_t xtal_ctune_update(uint8_t xtal_32m_ctune)
{
    nvds_tag_len_t length=1;
    uint8_t ctune;
    if (NVDS_OK != nvds_put(NVDS_TAG_XTAL32M_CTUNE, 1, &xtal_32m_ctune)) {
        return 0xFF;
    }
    if (NVDS_OK != nvds_get(NVDS_TAG_XTAL32M_CTUNE, &length, &ctune)) {
        return 0xFF;
    }
    if (length != 1) {
        return 0xFF;
    }
    return ctune;
}

static void fq_calib_end_handler(void)
{
    if (m_xtal_32m_ctune < 64) {
        if(xtal_ctune_update(m_xtal_32m_ctune) == m_xtal_32m_ctune) {
            uint8_t tx_buf[2]={ 0 };
            tx_buf[0]=RESPONSE_TYPE_CALIB_RESULT;   //frequency calibration result
            tx_buf[1]=m_xtal_32m_ctune;
            fq_uart1_cmd_response(CMD_FREQ_CALIB, STATUS_SUCCESS, tx_buf, sizeof(tx_buf));

            LOG_FREQ_CALIB(OM_LOG_DEBUG, "Calib Successful Ct=%d\n", m_xtal_32m_ctune);
        } else {
            fq_uart1_cmd_response(CMD_FREQ_CALIB, STATUS_FAILED, NULL, 0);
            LOG_FREQ_CALIB(OM_LOG_DEBUG, "Calib save fail, Ct=%d\n", m_xtal_32m_ctune);
        }
    } else {
        fq_uart1_cmd_response(CMD_FREQ_CALIB, STATUS_FAILED, NULL, 0);
        LOG_FREQ_CALIB(OM_LOG_DEBUG, "Calib ctune out of range, Ct=%d\n", m_xtal_32m_ctune);
    }
}


static void fq_adv_start(void)
{
    if (FQ_RF_STATE_IDLE != m_fq_state) {
        return;
    }

    uint32_t err_code = ob_gap_adv_start(0, &adv_param, &adv_data, &scan_rsp_data);
    if (err_code != OB_ERROR_NO_ERR) {
        LOG_FREQ_CALIB(OM_LOG_DEBUG, "adv start=%x\n", err_code);
    }
    m_fq_state=FQ_RF_STATE_ADV;
}


static void fq_on_ble_evt_cb(uint16_t evt_id, const omble_evt_t *evt)
{
    if (evt_id == OB_GAP_EVT_CONNECTED) {
        if (memcmp(&m_peer_addr, &evt->gap.connected.peer_addr, sizeof(m_peer_addr))) {
            ob_gap_disconnect(evt->gap.conn_idx, 0x13);
            LOG_FREQ_CALIB(OM_LOG_DEBUG, "invalid tester\n");
        } else {
            m_fq_state=FQ_RF_STATE_CONNECTED;
            m_conn_handle = evt->gap.conn_idx;
            const ob_gap_conn_params_t *p_params = &evt->gap.connected.conn_params;//&p_ble_evt->evt.gap_evt.params.connected.conn_params;
            LOG_FREQ_CALIB(OM_LOG_DEBUG, "conn params:%d %d %d\n", p_params->conn_intv,
                                                          p_params->latency_max,
                                                          p_params->timeout);
        }
    } else if (evt_id == OB_GAP_EVT_DISCONNECTED) {
        m_fq_state=FQ_RF_STATE_IDLE;
        m_conn_handle=BLE_CONN_HANDLE_INVALID;
        if (evt->gap.disconnected.reason == 0x15) { // BLE_HCI_REMOTE_DEV_TERMINATION_DUE_TO_POWER_OFF
            fq_calib_end_handler();
        } else {
            fq_uart1_cmd_response(CMD_FREQ_CALIB, STATUS_FAILED, NULL, 0);
            fq_adv_start();
        }
    } else if (evt_id == OB_GATTS_EVT_WRITE_REQ) {
        if (evt->gatt.write_req.att_hdl == m_char_local_handle) {
            uint16_t wr_len=evt->gatt.write_req.len;
            if (wr_len == 1) {
                if (evt->gatt.write_req.data[0] < 64) {
                    m_xtal_32m_ctune = evt->gatt.write_req.data[0];
                    if (m_xtal_32m_ctune < 64) {
                        drv_pmu_xtal32m_change_param(m_xtal_32m_ctune);
                        uint8_t buf[] = "hello";
                        ob_gattc_write(m_conn_handle, m_char_peer_handle, OB_GATTC_WRITE_CMD, buf, sizeof(buf));
                    }
                }
            } /* else if (wr_len == sizeof(CALIB_END_SUCCESS)) {
                if (memcmp(evt->gatt.write_req.data, CALIB_END_SUCCESS, sizeof(CALIB_END_SUCCESS)) == 0) {
                    fq_calib_end_handler();
                }
            } */
        }
    } else if (evt_id == OB_GAP_EVT_CONN_PARAMS_REQUEST) {
        const ob_gap_evt_conn_params_request_t *p_params = &evt->gap.conn_params_request;//&p_ble_evt->evt.gap_evt.params.connected.conn_params;
        LOG_FREQ_CALIB(OM_LOG_DEBUG, "conn params:%d %d %d %d\n", p_params->conn_intv_max,
                                                      p_params->conn_intv_min,
                                                      p_params->latency_max,
                                                      p_params->timeout);
    }
}


static void chip_uuid_get(uint8_t bd_addr[6])
{
    nvds_tag_len_t length = 6;
    nvds_get(NVDS_TAG_BD_ADDRESS, &length, bd_addr);
}


static void fq_ble_service_add(void)
{
    uint32_t   err_code;
    ob_event_callback_reg(fq_on_ble_evt_cb);

    adv_param.own_addr_type = OB_ADV_ADDR_TYPE_PUBLIC;
    adv_param.prim_phy = OB_ADV_PHY_1M;
    adv_param.secd_phy = OB_ADV_PHY_1M;
    adv_param.tx_pwr = 0;
    adv_param.filter_policy = OB_ADV_FILTER_NONE;
    adv_param.prim_ch_map = OB_ADV_CH_ALL;
    adv_param.prim_intv_min = 0x40;
    adv_param.prim_intv_max = 0x80;
    adv_param.local_addr = NULL;
    adv_param.peer_addr = NULL;
    adv_param.adv_properties = OB_ADV_PROP_LEGACY_IND;

    ob_gap_addr_t bd_addr;
    chip_uuid_get(bd_addr.addr);
    ob_gap_addr_set(OB_ADV_ADDR_TYPE_PUBLIC, &bd_addr.addr[0]);

    uint16_t gatt_handle;
    static const uint8_t serv_gap[2]  = { 0x00, 0x18 };
    static const uint8_t char_name[2] = { 0x00, 0x2a };
    static const ob_gatt_item_t atts_gap[] = {
    { ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
    { char_name,       OB_UUID_16BIT, OB_ATT_PROP_READ },
    };
    static const ob_gatt_serv_t att_serv_gap = {
    serv_gap, OB_UUID_16BIT,
    sizeof(atts_gap) / sizeof(atts_gap[0]), atts_gap
    };
    err_code = ob_gatts_add_service(&att_serv_gap, &m_start_handle);
    if (err_code != OB_ERROR_NO_ERR) {
        LOG_FREQ_CALIB(OM_LOG_DEBUG, "add server failed\n");
    }

    static const uint8_t serv_calib[2] = { 0xBF, 0x01 };
    static const uint8_t char_calib[2] = { 0x01, 0x00 };
    static const ob_gatt_item_t atts_calib[] = {
    { ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
    { char_calib,        OB_UUID_16BIT, OB_ATT_PROP_READ | OB_ATT_PROP_WRITE | OB_ATT_PROP_NTF | OB_ATT_PROP_WRITE_CMD, 0},
    { ob_att_cccd_def, OB_UUID_16BIT, OB_ATT_PROP_READ | OB_ATT_PROP_WRITE },
    };
    static const ob_gatt_serv_t att_serv_calib = {
    serv_calib, OB_UUID_16BIT,
    sizeof(atts_calib) / sizeof(atts_calib[0]), atts_calib
    };
    err_code = ob_gatts_add_service(&att_serv_calib, &gatt_handle);
    if (err_code != OB_ERROR_NO_ERR) {
        LOG_FREQ_CALIB(OM_LOG_DEBUG, "add server failed\n");
    }
    m_char_local_handle = gatt_handle + 2;
}


static void uart_cmd_flow_evt_handler(void)
{
    evt_clear(FQ_UART_RX_EVT_ID);
    uint8_t cmd_code;
    uint8_t  rsp_code;
    OM_CRITICAL_BEGIN();
    cmd_code=uart_rx_buf[1];
    if (uart_rx_idx >= cmd_pkt_len) {
        rsp_code=0;
        switch (uart_rx_buf[1]) {
            case CMD_FREQ_CALIB: {
                if (cmd_pkt_len == (9 + 4 + 2)) { // payload + header + crc
                    m_peer_addr.addr_type=uart_rx_buf[4];
                    memcpy(m_peer_addr.addr, &uart_rx_buf[5], 6);
                    m_char_peer_handle=uart_rx_buf[12] << 8 | uart_rx_buf[11];
                    // LOG_ARRY_FREQ_CALIB(OM_LOG_DEBUG, &m_peer_addr, sizeof(m_peer_addr), 16);
                    LOG_FREQ_CALIB(OM_LOG_DEBUG, "peer handle=0x%04x\n", m_char_peer_handle);
                } else {
                    rsp_code=1;
                }
                break;
            }
            default:
                rsp_code=2;
                break;
        }
        uart_rx_idx -= cmd_pkt_len;
        for (uint32_t i=0; i < uart_rx_idx; ++i) {
            uart_rx_buf[i]=uart_rx_buf[i + cmd_pkt_len];
        }
    } else {
        uart_rx_idx=0;
        rsp_code=0xff;
    }
    OM_CRITICAL_END();

    uint8_t tx_buf[11]={ 0 };
    if (rsp_code == 0) {
        switch (cmd_code) {
            case CMD_FREQ_CALIB: {
                tx_buf[0]=RESPONSE_TYPE_DUT_INFO;  //device info type response
                ob_gap_addr_t bd_addr;
                ob_gap_addr_get(OB_ADV_ADDR_TYPE_PUBLIC, (uint8_t *)&bd_addr.addr);
                LOG_ARRY_FREQ_CALIB(OM_LOG_DEBUG, &bd_addr.addr[0], 6, 16);
                tx_buf[1]=OB_ADV_ADDR_TYPE_PUBLIC;
                memcpy(&tx_buf[2], &bd_addr.addr[0], 6);
                tx_buf[8]=m_char_local_handle & 0xff;
                tx_buf[9]=(m_char_local_handle >> 8) & 0xff;
                tx_buf[10]=m_xtal_32m_ctune;
                fq_uart1_cmd_response(CMD_FREQ_CALIB, STATUS_SUCCESS, tx_buf, sizeof(tx_buf));
                fq_adv_start();
                break;
            }

            default:
                break;
        }
    } else if (rsp_code < 0xff) {
        fq_uart1_cmd_response(cmd_code, STATUS_FAILED, NULL, 0);
    }
}

void fq_calib_entry(void)
{
    struct ob_stack_param param = {
    .max_connection = OB_LE_HOST_CONNECTION_NB,
    .max_ext_adv_set = OB_LE_HOST_ADV_SET_NUM,
    .max_att_mtu = OB_LE_HOST_ATT_MTU,
    .max_gatt_serv_num = OB_LE_HOST_MAX_GATT_SERV_NUM,
    .max_gatt_write_cache = OB_LE_HOST_ATT_WRITE_CACHE_SIZE,
    .smp_sc_support = OB_LE_HOST_SC_PAIRING,
    };
    m_xtal_32m_ctune = (uint8_t)drv_pmu_xtal32m_get_param();
    fq_hardware_init();
    evt_init();
    evt_callback_set(FQ_UART_RX_EVT_ID, uart_cmd_flow_evt_handler);
    nvds_init(0);
    omble_init(&param);
    fq_ble_service_add();
}

/** @} */
