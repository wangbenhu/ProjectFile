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
 * @brief    app_calib_dongle
 * @details  app_calib_dongle
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
#include "app_calib_dongle.h"

/*******************************************************************************
 * MACROS
 */
#if CONFIG_OM_LOG
#define log_debug(level, ...)                           do{OM_LOG(level, __VA_ARGS__); \
                                                            OM_LOG(level, "    %d@%s\n", __LINE__, __func__);  \
                                                        }while(0)
#define log_debug_array_ex(level, note, array, len)     do{OM_LOG(level, "%s: ",note); \
                                                            OM_LOG_HEXDUMP(level, array, len, 16); \
                                                        }while(0);
#else
#define log_debug(level, ...)
#define log_debug_array_ex(level, note, array, len)
#endif

#define EVT_TYPE_USART          ((evt_type_t)(EVT_TYPE_USR_FIRST+0))
#define EVT_TYPE_RF_BEGIN       ((evt_type_t)(EVT_TYPE_USR_FIRST+1))

#define CAP_TIMES               20//50
#define CAP_TIMES_LIMIT         15//45  // 90%*CAP_TIMERS
#define CAP_MAX                 64

#define VERSION_TYPE            1   //0: BLE; 1: 24G;
#define VERSION_1               2
#define VERSION_2               0
#define VERSION_3               0
#define VERSION_4               0

#define USART_RECV_HEADER_START     0xFF
#define USART_SEND_HEADER_START     0xEE

enum usart_cmd {
    USART_CMD_24G_DONGLE_RESET          = 0x10,
    USART_CMD_24G_DONGLE_CONFIG         = 0x11,
    USART_CMD_24G_DUT_CALIB_START       = 0x12,
    USART_CMD_24G_DUT_CALIB_END         = 0x13,
    USART_CMD_24G_DONGLE_GET_RESULT     = 0x14,
    USART_CMD_24G_DUT_SET_RESULT        = 0x15,
    USART_CMD_24G_VERSION               = 0xFE,
};

enum usart_fail_reaon {
    USART_FAIL_NONE,
    USART_FAIL_CRC,
    USART_FAIL_TIMEOUT,
    USART_FAIL_CONFIG,
    USART_FAIL_CALIB,
    USART_FAIL_GET_RESULT,
    USART_FAIL_CONFIRM,
    USART_FAIL_CMD,
};

/*******************************************************************************
 * TYPEDEFS
 */

typedef enum usart_cmd_status {
    USART_CMD_STATUS_SUCCESS,
    USART_CMD_STATUS_FAIL,
    USART_CMD_STATUS_TIMEOUT,
    USART_CMD_STATUS_CRC_ERR,
} usart_cmd_status_t;

typedef struct {
    uint32_t addr;
    uint8_t tx_power_idx;
    uint16_t freq;
} rf_set_t;

typedef enum state_usart {
    STATE_USART_WAIT_FOR_HEADER,
    STATE_USART_WAIT_FOR_CMD,
    STATE_USART_WAIT_FOR_LENGTH,
    STATE_USART_WAIT_FOR_DATA,
    STATE_USART_WAIT_FOR_CRC
} state_usart_t;

typedef struct {
    uint8_t header;
    uint8_t cmd;
    uint8_t length[2];
    uint8_t data[32];
    uint8_t crc[2];
    uint32_t length_idx;
    uint32_t data_idx;
    uint32_t crc_idx;
    uint8_t save[32];
    uint32_t save_idx;
} obj_cmd_t;

 /*******************************************************************************
 * CONST & VARIABLES
 */

static om_fifo_t  usart_rx_fifo;
static uint8_t usart_rx_pool[512];

static int32_t fq_offset_save[CAP_MAX][CAP_TIMES];
static uint8_t fq_save_cap = 0xFF;
static uint8_t fq_save_count;
static uint8_t best_cap;
static int32_t best_offset;

static rf_tx_power_t tx_power_buf[19] = {
    RF_TX_POWER_0DBM  ,
    RF_TX_POWER_6DBM  ,
    RF_TX_POWER_5P5DBM  ,
    RF_TX_POWER_5DBM  ,
    RF_TX_POWER_4P5DBM  ,
    RF_TX_POWER_4DBM  ,
    RF_TX_POWER_3P5DBM  ,
    RF_TX_POWER_3DBM  ,
    RF_TX_POWER_2P5DBM  ,
    RF_TX_POWER_2DBM  ,
    RF_TX_POWER_1P5DBM  ,
    RF_TX_POWER_1DBM  ,
    RF_TX_POWER_0P5DBM  ,
    RF_TX_POWER_0DBM  ,
    RF_TX_POWER_N1DBM,
    RF_TX_POWER_N5DBM,
    RF_TX_POWER_N9DBM,
    RF_TX_POWER_N18DBM,
    RF_TX_POWER_N47DBM,
};

static bool next_step_is_get_result = false;
volatile static state_usart_t recv_state = STATE_USART_WAIT_FOR_HEADER;
static uint32_t recv_length;
static uint16_t crc16_calculate;

static obj_cmd_t cmd_recv;
static rf_set_t rf_set;

/* 从PHY寄存器取出的CFO原始数据，用于计算频偏 */
static int32_t cfo_raw_data;  // 从phy里直接读出来的32bits 寄存器
static int32_t fq_offset;
/* rf DataRate 目前用的1M基准速率，即DataRate=1000*1000，公式单位K，所以最后DATA_RATE=1000 */
#define DATA_RATE                       1000//1000*1000

/* tx payload */
static uint8_t om24g_tx_payload[64];
/* rx payload */
static uint8_t om24g_rx_payload[128];
static om24g_config_t rf_config = {
    .tx_data             = om24g_tx_payload,
    .rx_data             = om24g_rx_payload,
    .packet_struct_sel   = OM24G_STRUCTURE_B,
    .preamble            = 0xaa,
    .preamble_len        = 0x05,
    .sync_word_sel       = OM24G_SYNCWORD0,
    .sync_word0          = 0x12345678,
    .sync_word1          = 0x12345678,
    .tx_addr             = 0x02,
    .rx_addr             = 0x02,
    .addr_chk            = 0x00,
    .static_len          = 64,
    .hdr_bits            = 0x08,
    .addr1_bits          = 0x00,
    .len_bits            = 0x08,
    .addr1_pos           = 0x00,
    .len_pos             = 0x00,
    .endian              = OM24G_ENDIAN_MSB,
    .freq                = 2440,
    .data_rate           = OM24G_RATE_1M,
    .ack_en              = 0,
    .dpl_en              = 1,
    .white_en            = 1,
    .white_skip_hdr      = 1,
    .white_skip_addr     = 1,
    .white_skip_crc      = 0,
    .white_sel           = 0x00,
    .white_seed          = 0x80,
    .white_obit          = 0x07,
    .crc_len             = OM24G_CRC_2BYTE,
    .crc_en              = 1,
    .crc_mode            = 0,
    .crc_poly            = 0x1021, // 0x07,
    .crc_init            = 0xFFFF, // 0xFF,
    .crc_skip_sync       = 0,
    .crc_skip_len        = 0,
    .crc_skip_addr       = 0,
    .modulation_mode     = OM24G_MODULATION_GFSK,
    .detect_mode         = OM24G_SOFT_DETECTION,
};

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

// recv_buffer
/* -------+-----+-----------+-----------+---------+-----+---------+--------+--------*/
/* header | cmd | length[0] | length[1] | data[0] | ``` | data[n] | crc[0] | crc[1] */
/* -------+-----+-----------+-----------+---------+-----+---------+--------+--------*/

// response_buffer
/* -------+-----+--------+----------+-----------+---------+-----+---------+--------+--------*/
/* header | cmd | status |length[0] | length[1] | data[0] | ``` | data[n] | crc[0] | crc[1] */
/* -------+-----+--------+----------+-----------+---------+-----+---------+--------+--------*/

static void state_machine_usart_start(uint8_t *buf, uint32_t len);

static uint16_t om_crc16_ccitt(uint16_t crcinit, const void *buf, unsigned len)
{
    register unsigned counter;
    register uint16_t crc = crcinit;
    const uint8_t *__buf = buf;

    for( counter = 0; counter < len; counter++)
        crc = (crc<<8) ^ crc16tab[((crc>>8) ^ *(uint8_t *)__buf++) & 0x00FF];

    return crc;
}

static void app_usart_cmd_response(uint8_t cmd, usart_cmd_status_t status, uint8_t *pdata, uint16_t len)
{
    // app_state_machine_idx_reinit();

    uint8_t buf[32] = {0};
    uint16_t crc_caculate = 0;

    OM_CRITICAL_BEGIN();
    buf[0] = USART_SEND_HEADER_START;
    buf[1] = cmd;
    buf[2] = status;
    if (len != 0) {
        buf[3] = len & 0xFF;
        buf[4] = (len >> 8) & 0xFF;
        memcpy(&buf[5], pdata, len);
    }
    crc_caculate =  om_crc16_ccitt(0, buf, 5+len);
    buf[5+len] = crc_caculate & 0xFF;
    buf[5+len+1] = (crc_caculate >> 8) & 0xFF;
    OM_CRITICAL_END();

    drv_uart_write(OM_UART1, buf, len+7, DRV_MAX_DELAY);
    log_debug_array_ex(OM_LOG_DEBUG, "response data", buf, len+7);
}


static void evt_callback_rf_begin(void)
{
    evt_clear(EVT_TYPE_RF_BEGIN);
    // om24g_control(OM24G_CONTROL_DUMP_RF_REGISTER, NULL);
    drv_om24g_read_int(om24g_rx_payload, 64);
}


static void fq_offset_save_init(void)
{
    for (uint8_t i=0; i<CAP_MAX; i++) {
        for (uint8_t j=0; j<CAP_TIMES; j++) {
            fq_offset_save[i][j] = INT32_MAX;
        }
    }
    fq_save_cap = 0xFF;
}


static int16_t calculate_result(uint8_t *cap)
{
    uint8_t i = 0;
    uint8_t j = 0;
    int64_t temp = 0;
    int32_t min = INT32_MAX;
    int32_t result_arr[CAP_MAX];
    for (i=0; i<CAP_MAX; i++) {
        result_arr[i] = 0;
        temp = 0;
        for (j=0; j<CAP_TIMES; j++) {
            if (fq_offset_save[i][j] == INT32_MAX) {
                break;
            }
            temp += fq_offset_save[i][j];
        }
        // log_debug("temp = %lld, j = %d", temp, j);

        if (j < CAP_TIMES_LIMIT) {
            result_arr[i] = INT32_MAX;
        } else {
            result_arr[i] = (int32_t)(temp / j);
        }
    }

    for (uint8_t k=0; k<CAP_MAX; k++) {
        if (abs(result_arr[k]) <= min) {
            *cap = k;
            min = result_arr[k];
        }
    }
    log_debug(OM_LOG_INFO, "calib_result: %d", min);
    return (int16_t)min;
}


static void app_cmd_handle(usart_cmd_status_t status, uint8_t *raw_buffer)
{
    uint8_t response_buffer[32] = {0};
    uint16_t response_date_length = 0;
    uint8_t response_date[8] = {0};
    uint16_t crc = 0;

    response_buffer[0] = USART_SEND_HEADER_START;
    response_buffer[1] = raw_buffer[1];


    if (status == USART_CMD_STATUS_TIMEOUT) {
        response_date_length = 1;
        response_date[0] = USART_FAIL_TIMEOUT;
    } else if (status == USART_CMD_STATUS_CRC_ERR) {
        response_date_length = 1;
        response_date[0] = USART_FAIL_CRC;
    } else {  // status == USART_CMD_STATUS_SUCCESS || status == USART_CMD_STATUS_FAIL
        switch (response_buffer[1]) {
            case USART_CMD_24G_DONGLE_RESET:
                drv_pmu_reset(PMU_REBOOT_FROM_SOFT_RESET_USER);
                // Never come here
                while (1);
                break;
            case USART_CMD_24G_DONGLE_CONFIG:
                // tx power or tx freq out of range
                if ((raw_buffer[8] > 0x12) || ((uint16_t)(raw_buffer[9] | (raw_buffer[10]<<8)) > 2512)
                || ((uint16_t)(raw_buffer[9] | (raw_buffer[10]<<8)) < 2358)) {
                    status = USART_CMD_STATUS_FAIL;
                }
                if (status == USART_CMD_STATUS_SUCCESS) {
                    // addr: 4bytes,
                    response_date_length = 4;
                    memcpy(response_date, raw_buffer+4, response_date_length);
                    rf_set.addr = (uint32_t)(raw_buffer[4] | (raw_buffer[5] << 8)
                        | (raw_buffer[6] << 16) | (raw_buffer[7] << 24));
                    rf_set.tx_power_idx = raw_buffer[8];
                    rf_set.freq = (uint16_t)(raw_buffer[9] | (raw_buffer[10] << 8));
                    OM_24G->SYNC_WORD0 = rf_set.addr;
                    drv_rf_tx_power_set(false, tx_power_buf[rf_set.tx_power_idx]);
                    drv_om24g_set_freq(rf_set.freq, 0);
                    evt_set(EVT_TYPE_RF_BEGIN);

                    next_step_is_get_result = true;
                } else {
                    response_date_length = 1;
                    response_date[0] = USART_FAIL_CONFIG;
                }
                break;
            case USART_CMD_24G_DONGLE_GET_RESULT:
                if (next_step_is_get_result == false) {
                    status = USART_CMD_STATUS_FAIL;
                    response_date_length = 1;
                    response_date[0] = USART_FAIL_GET_RESULT;
                } else {
                    next_step_is_get_result = false;
                    OM24G_CE_LOW();

                    best_offset = calculate_result(&best_cap);
                    log_debug(OM_LOG_INFO, "best cap: %d, best offset: %d", best_cap, best_offset);
                    if (best_offset < 10000) {
                        response_date_length = 4;
                        response_date[0] = drv_om24g_get_rssi();
                        response_date[1] = best_cap;
                        response_date[2] = best_offset & 0xff;
                        response_date[3] = (best_offset>>8) & 0xff;
                    } else {

                        response_date_length = 1;
                        response_date[0] = USART_FAIL_CALIB;
                    }
                    fq_offset_save_init();
                }
                break;
            case USART_CMD_24G_VERSION:
                response_date_length = 5;
                response_date[0] = VERSION_1;
                response_date[1] = VERSION_2;
                response_date[2] = VERSION_3;
                response_date[3] = VERSION_4;
                response_date[4] = VERSION_TYPE;
                break;
            default:
                status = USART_CMD_STATUS_FAIL;
                response_date_length = 1;
                response_date[0] = USART_FAIL_CMD;
                break;
        }
    }
    response_buffer[2] = status;
    if (response_date_length != 0) {
        response_buffer[3] = response_date_length & 0xFF;
        response_buffer[4] = (response_date_length >> 8) & 0xFF;
        memcpy(&response_buffer[5], response_date, response_date_length);
    }
    crc = om_crc16_ccitt(0, response_buffer, response_date_length+5);
    response_buffer[5+response_date_length] = crc & 0xFF;
    response_buffer[5+response_date_length+1] = (crc >> 8) & 0xFF;
    drv_uart_write(OM_UART1, response_buffer, response_date_length+7, DRV_MAX_DELAY);
    log_debug_array_ex(OM_LOG_DEBUG, "response data", response_buffer, response_date_length+7);
}


static void om24g_callback(void *om_reg, drv_event_t drv_event, void *buff, void *num)
{
    int16_t raw_data;

    switch (drv_event) {
        case DRV_EVENT_COMMON_RECEIVE_COMPLETED:
            if (fq_save_cap != *((uint8_t *)buff)) {
                fq_save_cap = *((uint8_t *)buff);
                fq_save_count = 0;
            }
            fq_offset_save[fq_save_cap][fq_save_count++] = fq_offset;
        case DRV_EVENT_COMMON_RX_OVERFLOW:   // max retry
            break;
        case DRV_EVENT_COMMON_TRANSMIT_COMPLETED:
            break;
        case DRV_EVENT_OM24G_SYNC:
            cfo_raw_data = OM_PHY->FYSNC_DET_INFO;
            raw_data = (int16_t)((cfo_raw_data & 0xFFFF0000) >> 16);
            // DataRate*cfo_est/2^14
            // fq_offset = round((DATA_RATE / 16384) * raw_data); //  16384 = pow(2, 14));
            fq_offset = (int32_t)(DATA_RATE * raw_data) / 16384; //  16384 = pow(2, 14));
            break;
        default:
            OM_ASSERT(0);
            break;
    }
}


static void usart_rx_handler(void *om_usart, drv_event_t event, void *rx_buf, void *rx_cnt)
{
    if (event == DRV_EVENT_COMMON_READ_COMPLETED) {
        om_fifo_in(&usart_rx_fifo, rx_buf, (uint32_t)rx_cnt);
        evt_set(EVT_TYPE_USART);
    }
}
static void evt_callback_usart(void)
{
    uint8_t buffer[32];
    uint32_t length;
    evt_clear(EVT_TYPE_USART);
    while (1) {
        length = om_fifo_out(&usart_rx_fifo, buffer, sizeof(buffer));
        if (length == 0) {
            break;
        }
        state_machine_usart_start(buffer, length);
    }
}


void app_calib_init(void)
{
    // UART1 init
    uart_config_t uart_cfg = {
        .baudrate       = 115200,
        .flow_control   = UART_FLOW_CONTROL_NONE,
        .data_bit       = UART_DATA_BIT_8,
        .stop_bit       = UART_STOP_BIT_1,
        .parity         = UART_PARITY_NONE,
        .half_duplex_en = 0,
        .lin_enable     = 0,
    };
    drv_uart_init(OM_UART1, &uart_cfg);
    drv_uart_register_isr_callback(OM_UART1, usart_rx_handler);
    drv_uart_read_int(OM_UART1, NULL, 0);

    om_fifo_init(&usart_rx_fifo, usart_rx_pool, sizeof(usart_rx_pool));
    recv_state = STATE_USART_WAIT_FOR_HEADER;
    evt_callback_set(EVT_TYPE_USART, evt_callback_usart);
    evt_callback_set(EVT_TYPE_RF_BEGIN, evt_callback_rf_begin);
    drv_om24g_register_event_callback(om24g_callback);
    drv_om24g_init(&rf_config);
    /* enable sync isr */
    OM_24G->INT_MASK = 0x1B;
    fq_offset_save_init();
    if (drv_pmu_reboot_reason() == PMU_REBOOT_FROM_SOFT_RESET_USER) {
        app_usart_cmd_response(USART_CMD_24G_DONGLE_RESET, USART_CMD_STATUS_SUCCESS, NULL, 0);
    }
}


static void state_machine_usart_start(uint8_t *buf, uint32_t len)
{
    uint8_t buffer[32];
    uint32_t length = len;
    int32_t idx = 0;
    memcpy(buffer, buf, length);
    log_debug_array_ex(OM_LOG_DEBUG, "recv buffer ", buffer, length);
    while (idx < length) {
        cmd_recv.save[cmd_recv.save_idx] = buffer[idx];
        switch (recv_state) {
            case STATE_USART_WAIT_FOR_HEADER:
                if (cmd_recv.save[cmd_recv.save_idx] == USART_RECV_HEADER_START) {
                    recv_state = STATE_USART_WAIT_FOR_CMD;
                    cmd_recv.header = USART_RECV_HEADER_START;
                } else {
                    recv_state = STATE_USART_WAIT_FOR_HEADER;
                    // If first byte is not USART_RECV_HEADER_START (0xFF), reset reception
                    // Note: End of cycle counter is incremented by 1
                    cmd_recv.save_idx = -1;
                }
                break;
            case STATE_USART_WAIT_FOR_CMD:
                    if (cmd_recv.save[cmd_recv.save_idx] == USART_CMD_24G_DONGLE_RESET
                    || cmd_recv.save[cmd_recv.save_idx] == USART_CMD_24G_DONGLE_CONFIG
                    || cmd_recv.save[cmd_recv.save_idx]== USART_CMD_24G_DONGLE_GET_RESULT
                    || cmd_recv.save[cmd_recv.save_idx] == USART_CMD_24G_VERSION) {
                        recv_state = STATE_USART_WAIT_FOR_LENGTH;
                        cmd_recv.length_idx = 0;
                        cmd_recv.data_idx = 0;
                        cmd_recv.crc_idx = 0;
                    } else {
                        recv_state = STATE_USART_WAIT_FOR_HEADER;
                        cmd_recv.save_idx = -1;
                        app_cmd_handle(USART_CMD_STATUS_FAIL, cmd_recv.save);
                        // app_usart_response(usart_recv_save[1], USART_CMD_STATUS_FAIL, NULL, 0);
                    }
                break;
            case STATE_USART_WAIT_FOR_LENGTH:
                cmd_recv.length[cmd_recv.length_idx++] = cmd_recv.save[cmd_recv.save_idx];
                if (cmd_recv.length_idx < 2) {
                    recv_state = STATE_USART_WAIT_FOR_LENGTH;
                } else {
                    recv_length = (uint16_t)(cmd_recv.length[0] | (cmd_recv.length[1] << 8));
                    if (recv_length == 0) {
                        recv_state = STATE_USART_WAIT_FOR_CRC;
                        crc16_calculate = om_crc16_ccitt(0, cmd_recv.save, cmd_recv.save_idx+1);
                    } else {
                        recv_state = STATE_USART_WAIT_FOR_DATA;
                    }
                }
                break;
            case STATE_USART_WAIT_FOR_DATA:
                cmd_recv.data[cmd_recv.data_idx++] = cmd_recv.save[cmd_recv.save_idx];
                if (cmd_recv.data_idx < recv_length) {
                    recv_state = STATE_USART_WAIT_FOR_DATA;
                } else {
                     recv_state = STATE_USART_WAIT_FOR_CRC;
                     crc16_calculate = om_crc16_ccitt(0, cmd_recv.save, cmd_recv.save_idx+1);
                }
                break;
            case STATE_USART_WAIT_FOR_CRC:
                cmd_recv.crc[cmd_recv.crc_idx++] = cmd_recv.save[cmd_recv.save_idx];
                cmd_recv.save_idx = -1;
                if (cmd_recv.crc_idx < 2) {
                    recv_state = STATE_USART_WAIT_FOR_CRC;
                } else {
                    recv_state = STATE_USART_WAIT_FOR_HEADER;
                    if ((uint16_t)(cmd_recv.crc[0] | (cmd_recv.crc[1] << 8)) == crc16_calculate) {
                        app_cmd_handle(USART_CMD_STATUS_SUCCESS, cmd_recv.save);
                    } else {
                        log_debug(OM_LOG_INFO, "crc error!!! rece crc: %02x", (uint16_t)(cmd_recv.crc[0] | (cmd_recv.crc[1] << 8)));
                        app_cmd_handle(USART_CMD_STATUS_CRC_ERR, cmd_recv.save);
                    }
                }
                break;

            default:
                break;
        }
        cmd_recv.save_idx++;
        idx++;
    }
}

/** @} */
