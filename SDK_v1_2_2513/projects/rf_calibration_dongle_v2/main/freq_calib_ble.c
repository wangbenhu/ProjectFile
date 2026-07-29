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
#include "freq_calib_ble.h"
#include "om_time.h"
#include "evt_timer.h"


#define FQ_UART_RX_EVT_ID       ((evt_type_t)(EVT_TYPE_USR_FIRST+0))
#define PIN_FQ_UART_TX                    5
#define PIN_FQ_UART_RX                    6
#define PIN_FQ_ENTRY_DET                  4

#define FQ_UART                        OM_UART1
#define FQ_UART_BAUDRATE               115200
#define UART_CMD_BUF_LEN               32
#define UART_CMD_HEADER_LEN            4

// #define UART_RX_HEAD_LEN                4
#define UART_RX_HEAD_START              0xFF
#define UART_RX_TIMEOUT                 100 // 312.5us * 100 = 31250us
#define UART_RX_BUF_LEN                 32

#define CMD_START                       0xFF // the start byte of comman packet
#define EVT_START                       0xEE // the start byte of event packet

// #define uart_data_send(buf, len)        do{uart_send_block(HS_UART1, buf, len);}while(0)

#define FREQ_SAMPLE_NUM                 (1 << 4)
#define FREQ_AVG_NUM                    (FREQ_SAMPLE_NUM >> 1)
#define FREQ_IDX_NUM                    5

#define REASON_FREQ_CALIB_END           0x15

#define RSSI_SAMPLE_MAX_NUMB            (8)
// #define RSSI_TIMER_ID    BUILD_TIMER_ID(LAYER_USR, 1)
static evt_timer_t RSSI_TIMER_ID;
enum
{
    STATUS_SUCCESS,
    STATUS_FAILED,
};

enum
{
    ERR_CONN_ALREADY = 0x01,
    ERR_CONN_TIMEOUT,
    ERR_DISCONN_UNKNOWN,
};

typedef enum
{
    CMD_INIT,
    CMD_CONNECT,
    CMD_RESET,
    CMD_GET_RSSI,
    CMD_CALIB_START,
    CMD_DISCONNECT,
} cmd_t;

static uint32_t time_cur, time_last;
static uint8_t uart_rx_buf[UART_RX_BUF_LEN]; //uart data receive buffer
static uint8_t data_idx, packet_len;
#define BLE_CONN_HANDLE_INVALID 0xFF
static uint16_t connection_handle = BLE_CONN_HANDLE_INVALID;
// static uint8_t local_addr[7];
static uint16_t local_value_handle;
static ob_gap_addr_t target_addr;
static uint16_t target_value_handle;
static int8_t target_freq_idx;

static uint16_t sample_num;
static int16_t freq_offset[FREQ_SAMPLE_NUM];
static uint8_t freq_idx_num;
static uint8_t freq_idx[FREQ_IDX_NUM];
static uint8_t  rssi_read_times;
static int16_t  rssi_read_acc;
static int8_t  rssi_read_out;
static uint32_t rssi_try_times;
static bool    rssi_rsp_needed;


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


static void uart_cmd_response(cmd_t cmd, uint8_t status, uint8_t *pdata, uint16_t len)
{
    uint8_t buf[64] = { 0 };
    if((len+7)>sizeof(buf))
        return;

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
    OM_LOG(OM_LOG_DEBUG, "response date: \n");
    OM_LOG_HEXDUMP(OM_LOG_INFO, buf, len + 7, 16);
    drv_uart_write(FQ_UART, buf, len + 7, DRV_MAX_DELAY);
}


static void rssi_read_timer_cb(evt_timer_t *timer, void *param)
{
    if((connection_handle != BLE_CONN_HANDLE_INVALID)&&rssi_read_times)
    {
        int8_t rssi = 0;
        rssi = drv_om24g_get_rssi();

        //   sd_ble_gap_rssi_get(connection_handle, &rssi);
        if((rssi>=-100)&&(rssi<0))
        {
            rssi_read_acc += rssi;
            rssi_read_times--;
            rssi_try_times=0;
            if(rssi_read_times==0)
            {
                OM_LOG(OM_LOG_DEBUG, "rssi_read_out cb\n");
                rssi_read_out = (rssi_read_acc/RSSI_SAMPLE_MAX_NUMB);
                evt_timer_del(&RSSI_TIMER_ID);
                if(rssi_rsp_needed)
                {
                    rssi_rsp_needed=false;
                    uart_cmd_response(CMD_GET_RSSI, STATUS_SUCCESS, (uint8_t*)&rssi_read_out, 1); // send peer rssi event
                }
            }
        }
        else
        {
                if(++rssi_try_times>=300)
                {
                rssi_read_times=0;
                rssi_try_times=0;
                if(rssi_rsp_needed)
                {
                    rssi_rsp_needed=false;
                    uart_cmd_response(CMD_GET_RSSI, STATUS_FAILED, NULL, 0);
                    OM_LOG(OM_LOG_ERROR, "read rssi timeout\n");
                }
                }
        }
    }
    else
    {
        evt_timer_del(&RSSI_TIMER_ID);
        rssi_read_times=0;
        rssi_read_out=0;
        rssi_rsp_needed=false;
        rssi_try_times=0;
        //   if(connection_handle != BLE_CONN_HANDLE_INVALID)
        //     sd_ble_gap_rssi_stop(connection_handle); // stop rssi report
    }
}


void freq_calib_connected_cb(uint16_t conn_handle)
{
    OM_LOG(OM_LOG_INFO, "Connected.\n");
    connection_handle = conn_handle;
    uart_cmd_response(CMD_CONNECT, STATUS_SUCCESS, NULL, 0); // send connected event
    // sd_ble_gap_rssi_start(connection_handle, 1, 1); // start rssi report
    rssi_read_times=RSSI_SAMPLE_MAX_NUMB;
    rssi_read_acc=0;
    rssi_read_out=0;
    rssi_try_times=0;
    rssi_rsp_needed=false;
    evt_timer_del(&RSSI_TIMER_ID);
    evt_timer_set(&RSSI_TIMER_ID, 10, EVT_TIMER_REPEAT, rssi_read_timer_cb, NULL);
}


void freq_calib_disconnected_cb(uint16_t conn_handle, uint8_t reason)
{
    OM_LOG(OM_LOG_INFO, "Disconnected! Reason: 0x%02X\n", reason);
    connection_handle = BLE_CONN_HANDLE_INVALID;
    uint8_t disconn_reason = reason;
    if (reason == 0x15)
    {
        uart_cmd_response(CMD_DISCONNECT, STATUS_SUCCESS, &disconn_reason, 1); // send disconnected event
    }
//    else
//    {
//        LOG_ERR("Unexpected disconnection!!");
//        uint8_t err = ERR_DISCONN_UNKNOWN;
//        uart_cmd_response(CMD_CONNECT, STATUS_FAILED, &err, 1); // send connection failed event
//    }
    rssi_read_times=0;
    rssi_read_out=0;
    evt_timer_del(&RSSI_TIMER_ID);
}


static void freq_calib_result_report(uint8_t state)
{
    int16_t avg=0;
    uint32_t i;

    sample_num--;
    sample_num %= FREQ_SAMPLE_NUM;
    for(i=0; i<FREQ_AVG_NUM; ++i)
    {
       avg += freq_offset[sample_num];
       if(sample_num==0)
         sample_num = (FREQ_SAMPLE_NUM-1);
       else
         sample_num--;
    }
    avg = (avg + FREQ_AVG_NUM/2) /FREQ_AVG_NUM;

    uint8_t buf[3] = {0};
    buf[0] = target_freq_idx;
    buf[1] = avg & 0xff;
    buf[2] = avg >> 8 & 0xff;
    uart_cmd_response(CMD_CALIB_START, state, buf, sizeof(buf)); // send calibration result event
    OM_LOG(OM_LOG_INFO, "freq offset= %d KHz\n", avg);
}


static int16_t freq_offset_get(void)
{
    int32_t cfo_raw_data = OM_PHY->FYSNC_DET_INFO;
    int16_t raw_data = (int16_t)((cfo_raw_data & 0xFFFF0000) >> 16);
    // DataRate*cfo_est/2^14
    // fq_offset = round((DATA_RATE / 16384) * raw_data); //  16384 = pow(2, 14));
    return (1000 * raw_data / 16384); //  16384 = pow(2, 14));
}


static void freq_calibrate(void)
{
    int16_t fq_offset = freq_offset_get();

    OM_LOG(OM_LOG_INFO, "offset= %d\n", fq_offset);

    freq_offset[sample_num % FREQ_SAMPLE_NUM] = fq_offset;
    sample_num++;

    do{
        if (sample_num > FREQ_SAMPLE_NUM*10)
        {
            OM_LOG(OM_LOG_WARN, "too many samples!\n");
            break;
        }
        else if (freq_idx_num >= FREQ_IDX_NUM)
        {
            int16_t idx_avg=0, idx_max=-100, idx_min=100;
            OM_LOG(OM_LOG_DEBUG, "freq_idx: ");
            OM_LOG_HEXDUMP(OM_LOG_DEBUG, freq_idx, FREQ_IDX_NUM, 16);
            for(int i=0; i<FREQ_IDX_NUM; i++)
            {
                idx_max = (freq_idx[i] > idx_max) ? freq_idx[i] : idx_max;
                idx_min = (freq_idx[i] < idx_min) ? freq_idx[i] : idx_min;
                idx_avg += freq_idx[i];
            }
            idx_avg = (idx_avg + FREQ_IDX_NUM/2) / FREQ_IDX_NUM;
            OM_LOG(OM_LOG_DEBUG, "idx_avg = %d, idx_max = %d, idx_min = %d\n", idx_avg, idx_max, idx_min);
            if (idx_max - idx_min == 0)
            {
                target_freq_idx = idx_avg; //this is the best value for target frequency index
                break;
            }
        }
        int16_t diff = (fq_offset > 5) ? 1 : ((fq_offset < -5) ?  -1 : 0);
        target_freq_idx += diff; // target frequency index increase 1 or -1 or 0
        target_freq_idx = (target_freq_idx < 0) ? 0 : ((target_freq_idx > 63) ? 63 : target_freq_idx); // range from 0 to 63
        freq_idx[freq_idx_num % FREQ_IDX_NUM] = target_freq_idx; // loop store 5 data of frequency index
        freq_idx_num++;
    } while(0);

    if (sample_num - freq_idx_num > FREQ_AVG_NUM)
    {
        OM_LOG(OM_LOG_INFO, "calibration end\n");
        OM_LOG(OM_LOG_DEBUG, "sample_num = %d, freq_idx_num =%d\n", sample_num, freq_idx_num);
        uint8_t result[] = "success";

        ob_gattc_write(connection_handle, target_value_handle, OB_GATTC_WRITE_CMD, result, sizeof(result));
        // ble_gattc_write_params_t write ={ BLE_GATT_OP_WRITE_CMD, 0, target_value_handle, 0, sizeof(result), result };
        // sd_ble_gattc_write(connection_handle, &write);
        freq_calib_result_report(STATUS_SUCCESS);
        sample_num = freq_idx_num = 0;
    }
    else
    {
        OM_LOG(OM_LOG_INFO, "write idx= %d\n", target_freq_idx);
        // ble_gattc_write_params_t write ={ BLE_GATT_OP_WRITE_CMD, 0, target_value_handle, 0, sizeof(uint8_t), (uint8_t*)&target_freq_idx };
        // sd_ble_gattc_write(connection_handle, &write);
        ob_gattc_write(connection_handle, target_value_handle, OB_GATTC_WRITE_CMD, (uint8_t*)&target_freq_idx, sizeof(uint8_t));
    }
}

void freq_calib_gatt_write_cb(uint16_t att_handle, const uint8_t *data, uint8_t len)
{
    OM_LOG(OM_LOG_DEBUG, "recv: ");
    OM_LOG_HEXDUMP(OM_LOG_DEBUG, (uint8_t *)data, len, 16);
    freq_calibrate(); // start frequency calibration
}

void freq_calib_connection_timeout_cb(void)
{
    OM_LOG(OM_LOG_ERROR, "Connection timeout!!\n");
    uint8_t err = ERR_CONN_TIMEOUT;
    uart_cmd_response(CMD_CONNECT, STATUS_FAILED, &err, 1); // send connection failed event
}


static void on_ble_evt(uint16_t evt_id, const omble_evt_t *evt)
{
    switch (evt_id)
    {
        case OB_GAP_EVT_CONNECTED:
            OM_LOG(OM_LOG_DEBUG, "BLE_GAP_EVT_CONNECTED\n");
            // freq_calib_connected_cb(p_ble_evt->evt.gap_evt.conn_handle);
            freq_calib_connected_cb(evt->gap.conn_idx);
            break;

        case OB_GAP_EVT_DISCONNECTED:
            OM_LOG(OM_LOG_DEBUG, "BLE_GAP_EVT_DISCONNECTED\n");
            // freq_calib_disconnected_cb(p_ble_evt->evt.gap_evt.conn_handle,
            //                            p_ble_evt->evt.gap_evt.params.disconnected.reason);
            // OM_LOG(OM_LOG_DEBUG, "REASON %x\n", evt->gap.disconnected.reason);
            freq_calib_disconnected_cb(evt->gap.conn_idx, evt->gap.disconnected.reason);

            break;

        case OB_GATTS_EVT_WRITE_REQ:
            OM_LOG(OM_LOG_DEBUG, "BLE_GATTS_EVT_WRITE\n");
            freq_calib_gatt_write_cb(evt->gatt.write_req.att_hdl, evt->gatt.write_req.data, evt->gatt.write_req.len);
            break;

        case OB_GAP_EVT_TIMEOUT:
            OM_LOG(OM_LOG_DEBUG, "BLE_GAP_EVT_TIMEOUT\n");
            // test evt->gap.timeout.source
            // if(p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_CONN)
            // if(evt->gap.timeout.source == OB_GAP_TOUT_SCAN)
            // {
                freq_calib_connection_timeout_cb();
            // }
            break;

        default:
            break;
    }
}


static void connection_start(ob_gap_addr_t addr, uint16_t scan_intv, uint16_t scan_tout_ms,
                            uint16_t conn_intv, uint16_t conn_latency, uint16_t conn_tout)
{
    // ble_gap_scan_params_t scan_param =
    // {
    //     0,                       // Active scanning not set
    //     0,                       // Selective scanning set
    //     NULL,                    // White-list not set
    //     (uint16_t)scan_intv+2,   // Scan interval
    //     (uint16_t)scan_intv,     // Scan window
    //     scan_tout_ms/1000        // Timeout
    // };
    // ble_gap_conn_params_t conn_params =
    // {
    //     conn_intv,
    //     conn_intv,
    //     conn_latency,
    //     conn_tout
    // };

    ob_conn_phy_param_t conn_phy = {
        .scan_intv = (uint16_t)scan_intv+2,
        .scan_wind = (uint16_t)scan_intv,
        .conn_intv_min = conn_intv,
        .conn_intv_max = conn_intv,
        .latency_max = conn_latency,
        .timeout = conn_tout,
    };
    ob_conn_param_t conn_param = {
        OB_ADV_ADDR_TYPE_PUBLIC,
        OB_SCAN_FILTER_BASIC_UNFILTER,
        addr,
        &conn_phy,
        &conn_phy,
        NULL,
    };
    int32_t ret = ob_gap_connect(&conn_param);
    // int32_t ret = sd_ble_gap_connect((ble_gap_addr_t*)addr, &scan_param, &conn_params);
    if (ret != OB_ERROR_NO_ERR)
    {
        OM_LOG(OM_LOG_ERROR, "Connect error!! ret = %d\n", ret);
    }
}





static void uart_cmd_flow_evt_handler(void)
{
    evt_clear(FQ_UART_RX_EVT_ID);
    // if (uart_rx_idx >= cmd_pkt_len) {
        switch (uart_rx_buf[1]) {
            case CMD_INIT: {
                OM_LOG(OM_LOG_INFO, "CMD INIT\n");
                uint8_t tx_buf[9];
                tx_buf[0] = OB_ADV_ADDR_TYPE_PUBLIC;
                ob_gap_addr_get(OB_ADV_ADDR_TYPE_PUBLIC, &tx_buf[1]);

                OM_LOG_HEXDUMP(OM_LOG_DEBUG, tx_buf, sizeof(tx_buf), 16);

                tx_buf[7] = local_value_handle & 0xFF;
                tx_buf[8] = (local_value_handle >> 8) & 0xFF;
                uart_cmd_response(CMD_INIT, STATUS_SUCCESS, tx_buf, sizeof(tx_buf));
                OM_LOG(OM_LOG_DEBUG, "UART TX LOCLA %d\n", (tx_buf[8] << 8) | (local_value_handle));
                OM_LOG(OM_LOG_INFO, "Initialize dongle successful\n");
            } break;

            case CMD_CONNECT: {
                OM_LOG(OM_LOG_INFO, "CMD CONNECT\n");
                if (connection_handle == BLE_CONN_HANDLE_INVALID) {

                    memcpy(&target_addr, uart_rx_buf+4, sizeof(target_addr));
                    target_value_handle = uart_rx_buf[12] << 8 | uart_rx_buf[11];
                    target_freq_idx = uart_rx_buf[13];
                    sample_num = freq_idx_num = 0;
                    OM_LOG(OM_LOG_INFO, "target addrss: %02X:%02X:%02X:%02X:%02X:%02X, type: %d\n",
                                    target_addr.addr[5], target_addr.addr[4], target_addr.addr[3],
                                    target_addr.addr[2], target_addr.addr[1], target_addr.addr[0],
                                    target_addr.addr_type);
                    connection_start(target_addr, 0x04, 1000, 6, 0, 100);
                } else {
                    uint8_t err = ERR_CONN_ALREADY;
                    uart_cmd_response(CMD_CONNECT, STATUS_FAILED, &err, 1); // send connection failed event
                }
            } break;

            case CMD_RESET: {
                OM_LOG(OM_LOG_INFO, "CMD_RESET\n");
                uart_cmd_response(CMD_RESET, STATUS_SUCCESS, NULL, 0);
                drv_pmu_reset(PMU_REBOOT_FROM_SOFT_RESET_USER);
            } break;


            case CMD_GET_RSSI:
            {
                OM_LOG(OM_LOG_INFO, "CMD_GET_RSSI\n");
                OM_LOG(OM_LOG_DEBUG, "connect handle %x\n", connection_handle);
                if (connection_handle != BLE_CONN_HANDLE_INVALID)
                {
                    OM_LOG(OM_LOG_DEBUG, "ressi timwes %d\n", rssi_read_times);
                    if(rssi_read_times==0)
                    {
                        rssi_rsp_needed=false;
                        OM_LOG(OM_LOG_DEBUG, "HHH\n");
                        if(rssi_read_out<0)
                        {
                        OM_LOG(OM_LOG_DEBUG, "cmd get rssi: %d\n", rssi_read_out);
                        uart_cmd_response(CMD_GET_RSSI, STATUS_SUCCESS, (uint8_t*)&rssi_read_out, 1); // send peer rssi event
                        }
                        else
                        {
                        OM_LOG(OM_LOG_ERROR, "Get RSSI failed, =%d\n", rssi_read_out);
                        uart_cmd_response(CMD_GET_RSSI, STATUS_FAILED, NULL, 0);
                        }
                    }
                    else
                    {
                        rssi_rsp_needed=true;
                    }
                }
                else
                {
                    OM_LOG(OM_LOG_ERROR, "Get RSSI failed, disconnected\n");
                    uart_cmd_response(CMD_GET_RSSI, STATUS_FAILED, NULL, 0);
                }
            } break;

            case CMD_CALIB_START:
            {
                OM_LOG(OM_LOG_INFO, "CMD_CALIB_START\n");
                if (connection_handle != BLE_CONN_HANDLE_INVALID)
                {
                    uart_cmd_response(CMD_CALIB_START, STATUS_SUCCESS, NULL, 0);
                    // sd_ble_gap_rssi_stop(connection_handle); // stop rssi report
                    if(rssi_read_times)
                    {
                        OM_LOG(OM_LOG_DEBUG, "delete RSSI_TIMER_ID\n");
                        evt_timer_del(&RSSI_TIMER_ID);
                        rssi_read_times=0;
                        rssi_read_out=0;
                    }
                    freq_calibrate(); // start frequency calibration
                }
                else
                {
                    OM_LOG(OM_LOG_ERROR, "Start calibration failed!!\n");
                    uart_cmd_response(CMD_CALIB_START, STATUS_FAILED, NULL, 0);
                }
            } break;

            case CMD_DISCONNECT:
            {
                OM_LOG(OM_LOG_INFO, "CMD_DISCONNECT\n");
                uint8_t reason = uart_rx_buf[4];
                if (connection_handle != BLE_CONN_HANDLE_INVALID)
                {
                    OM_LOG(OM_LOG_INFO, "Try to disconnect the device, reason: 0x%02X\n", reason);
                    // sd_ble_gap_disconnect(connection_handle, reason); // disconnect with peer device
                    ob_gap_disconnect(connection_handle, reason);
                }
                else
                {
                    OM_LOG(OM_LOG_ERROR, "Disconnect failed!!\n");
                    uart_cmd_response(CMD_DISCONNECT, STATUS_FAILED, NULL, 0); // send disconnect failed event
                }
            } break;

            default:
                break;
        }
}


static void uart_rx_handler(OM_UART_Type *om_usart, drv_event_t event, uint8_t *rx_buf, uint32_t rx_cnt)
{
    if (event & DRV_EVENT_COMMON_READ_COMPLETED) {
        while (rx_cnt--) {
            time_cur = om_time();
            if (time_cur - time_last > UART_RX_TIMEOUT) {
                data_idx = packet_len = 0;
            }
            time_last = time_cur;

            if (data_idx < UART_CMD_BUF_LEN) {
                uart_rx_buf[data_idx++] = *((uint8_t *)rx_buf++);
            }
            if (uart_rx_buf[0] != CMD_START) {
                data_idx = packet_len = 0;
            } else if (data_idx == UART_CMD_HEADER_LEN) {
                packet_len = uart_rx_buf[3] << 8 | uart_rx_buf[2]; //data length
                packet_len += 6;         //4 bytes header, and 2 byte checkcrc at tail
                if (packet_len > UART_CMD_BUF_LEN) {
                    packet_len=UART_CMD_BUF_LEN;
                }
            } else if (data_idx == packet_len) { //a complete packet received
                OM_LOG_HEXDUMP(OM_LOG_DEBUG, uart_rx_buf, packet_len, 16);
                //Check the packet if right or not
                if ((uint16_t)(uart_rx_buf[packet_len - 2] | (uart_rx_buf[packet_len - 1] << 8))
                        == om_crc16_ccitt(0, uart_rx_buf, packet_len - 2)) {
                    evt_set(FQ_UART_RX_EVT_ID);
                }
            }
        }
    }
}


void chip_uuid_get(uint8_t bd_addr[6])
{
    // nvds_tag_len_t length = 6;
    // nvds_get(NVDS_TAG_BD_ADDRESS, &length, bd_addr);
    drv_flash_read_uid(OM_FLASH0, bd_addr, 6);

    OM_LOG(OM_LOG_DEBUG, "SF UID: ");
    OM_LOG_HEXDUMP(OM_LOG_DEBUG, bd_addr, 6, 8);
}

// Start handle
static uint16_t m_start_handle;
static void freq_calib_init(void)
{
    // ble_uuid_t ble_uuid;
    // ble_gatts_char_md_t char_md;
    // ble_gatts_attr_t attr_char_value;
    // ble_gatts_attr_md_t attr_md;
    // uint16_t service_handle;
    // ble_gatts_char_handles_t char_handles;

    // // add primary service
    // BLE_UUID_BLE_ASSIGN(ble_uuid, 0x01BF);
    // sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &service_handle);

    // // add characteristic
    // memset(&char_md, 0, sizeof(char_md));
    // char_md.char_props.write_wo_resp = 1;
    // BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    // attr_md.vloc = BLE_GATTS_VLOC_STACK;
    // BLE_UUID_BLE_ASSIGN(ble_uuid, 0x0001);

    // memset(&attr_char_value, 0, sizeof(attr_char_value));
    // attr_char_value.p_uuid = &ble_uuid;
    // attr_char_value.p_attr_md = &attr_md;
    // attr_char_value.init_len = 256;
    // attr_char_value.max_len = 256;

    // sd_ble_gatts_characteristic_add(service_handle, &char_md, &attr_char_value, &char_handles);

    // local_value_handle = char_handles.value_handle;

    uint32_t   err_code;
    ob_event_callback_reg(on_ble_evt);
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
        OM_LOG(OM_LOG_DEBUG, "add server failed\n");
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
        OM_LOG(OM_LOG_DEBUG, "add server failed\n");
    }
    local_value_handle = gatt_handle + 2;
    OM_LOG(OM_LOG_DEBUG, "local_value_handle %d\n", local_value_handle);
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
    drv_uart_register_isr_callback(FQ_UART, (drv_isr_callback_t)uart_rx_handler);
    drv_uart_read_int(FQ_UART, NULL, 0);
}

void ble_calib_entry(void)
{
    struct ob_stack_param param = {
    .max_connection = OB_LE_HOST_CONNECTION_NB,
    .max_ext_adv_set = OB_LE_HOST_ADV_SET_NUM,
    .max_att_mtu = OB_LE_HOST_ATT_MTU,
    .max_gatt_serv_num = OB_LE_HOST_MAX_GATT_SERV_NUM,
    .max_gatt_write_cache = OB_LE_HOST_ATT_WRITE_CACHE_SIZE,
    .smp_sc_support = OB_LE_HOST_SC_PAIRING,
    };
    fq_hardware_init();
    evt_init();
    evt_timer_init();
    evt_callback_set(FQ_UART_RX_EVT_ID, uart_cmd_flow_evt_handler);
    omble_init(&param);
    // fq_ble_service_add();
    freq_calib_init();
}

/** @} */
