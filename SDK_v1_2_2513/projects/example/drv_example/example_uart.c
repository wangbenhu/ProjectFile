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
 * @brief    example for using uart
 * @details  example for using uart: blocking, interrupt, and DMA
 * @version
 *
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


/*******************************************************************************
 * INCLUDES
 */
#include "om_driver.h"


/*******************************************************************************
 * MACROS
 */
/// Test tx pad for uart1
#define PAD_UART1_TXD               5
/// Test rx pad for uart1
#define PAD_UART1_RXD               6
/// Test cts pad for uart1
#define PAD_UART1_CTS               7
/// Test rts pad for uart1
#define PAD_UART1_RTS               8
/// Test tx pad for uart2
#define PAD_UART2_TXD               18
/// Test rx pad for uart2
#define PAD_UART2_RXD               19
/// Test uart
#define TEST_UART                   OM_UART1
/// Test uart_lin
#define TEST_UART_LIN               OM_UART2
/// Test uart baudrate
#define UART_BAUDRATE              115200
/// Wait flag defination
#define SEND_INT_IS_DONE            (1 << 0)
#define SEND_DMA_IS_DONE            (1 << 1)
#define READ_INT_IS_DONE            (1 << 2)
#define READ_DMA_IS_DONE            (1 << 3)


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    volatile uint8_t st;
} uart_gpdma_int_wait_flag_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
/// Test uart pin configuration
static const pin_config_t pin_config[] = {
    {PAD_UART1_TXD, {PINMUX_PAD5_UART1_TRX_CFG}, PMU_PIN_MODE_PU, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_UART1_RXD, {PINMUX_PAD6_UART1_RX_CFG}, PMU_PIN_MODE_PU, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_UART1_RTS, {PINMUX_PAD8_UART1_RTS_CFG}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_UART1_CTS, {PINMUX_PAD7_UART1_CTS_CFG}, PMU_PIN_MODE_PU, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_UART2_TXD, {PINMUX_PAD18_UART2_TRX_CFG}, PMU_PIN_MODE_PU, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_UART2_RXD, {PINMUX_PAD19_UART2_RX_CFG}, PMU_PIN_MODE_PU, PMU_PIN_DRIVER_CURRENT_NORMAL},
};
/// Buffer to store the received data
static uint32_t rec_data[4];
/// Buffer that stores the data to be sent
static uint32_t send_data[4];
/// DMA,INT wait flag
static uart_gpdma_int_wait_flag_t wait_flag;
static bool is_break = false;


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief callback for uart interrupt
 *
 * @param[in] om_uart    Pointer to uart
 * @param[in] event       uart event
 *                        - DRV_EVENT_COMMON_WRITE_COMPLETED
 *                        - DRV_EVENT_COMMON_DMA2PERIPH_COMPLETED
 *                        - DRV_EVENT_COMMON_READ_COMPLETED
 *                        - DRV_EVENT_UART_RX_TIMEOUT
 *                        - DRV_EVENT_COMMON_RX_OVERFLOW
 *                        - DRV_EVENT_UART_RX_PARITY_ERROR
 *                        - DRV_EVENT_UART_RX_BREAK
 *                        - DRV_EVENT_UART_RX_FRAME_ERROR
 * @param[in] rxbuf      Pointer to receive buffer
 * @param[in] rx_cnt     The number of received data
 *
 *******************************************************************************
 */
static void test_uart_cb(void *om_uart, drv_event_t event, void *rxbuf, void *rx_cnt)
{
    // Send what you receive
    if (event == DRV_EVENT_COMMON_READ_COMPLETED) {
        drv_uart_write(TEST_UART, (uint8_t *)rxbuf, (uint32_t)rx_cnt, 10);
        wait_flag.st |= READ_INT_IS_DONE;
        wait_flag.st |= READ_DMA_IS_DONE;
    } else if (event == DRV_EVENT_COMMON_WRITE_COMPLETED) {
        wait_flag.st |= SEND_INT_IS_DONE;
    } else if (event == DRV_EVENT_COMMON_GPDMA2PERIPH_COMPLETED) {
        wait_flag.st |= SEND_DMA_IS_DONE;
    }
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief example of sending and receiving in blocking mode
 *        1. send string "hello"
 *        2. block until 5 characters are received from user
 *        3. send the received data
 *******************************************************************************
 */
void example_uart_block(void)
{
    drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));

    uart_config_t uart_cfg = {
        .baudrate       = UART_BAUDRATE,
        .flow_control   = UART_FLOW_CONTROL_NONE,
        .data_bit       = UART_DATA_BIT_8,
        .stop_bit       = UART_STOP_BIT_1,
        .parity         = UART_PARITY_NONE,
    };

    drv_uart_init(TEST_UART, &uart_cfg);

    memcpy(send_data, "hello\r\n", 7);
    drv_uart_write(TEST_UART, (uint8_t *)send_data, 7, 10);
    if (drv_uart_read(TEST_UART, (uint8_t *)rec_data, 5, 3000) != 5U) {
        drv_uart_write(TEST_UART, (uint8_t *)"Read Timeout\n", 13, 10);
    }
    drv_uart_write(TEST_UART, (uint8_t *)rec_data, 5, 10);
}

/**
 *******************************************************************************
 * @brief example of sending and receiving in interrupt mode
 *        1. send string "hello"
 *        2. send what chip receives
 *
 *******************************************************************************
 */
void example_uart_int(void)
{
    drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));

    uart_config_t uart_cfg = {
        .baudrate       = UART_BAUDRATE,
        .flow_control   = UART_FLOW_CONTROL_NONE,
        .data_bit       = UART_DATA_BIT_8,
        .stop_bit       = UART_STOP_BIT_1,
        .parity         = UART_PARITY_NONE,
    };

    drv_uart_init(TEST_UART, &uart_cfg);
    drv_uart_register_isr_callback(TEST_UART, test_uart_cb);

    memcpy(send_data, "hello\r\n", 7);
    drv_uart_write_int(TEST_UART, (uint8_t *)send_data, 7);
    while (!(wait_flag.st & SEND_INT_IS_DONE));
    wait_flag.st &= ~SEND_INT_IS_DONE;

    drv_uart_read_int(TEST_UART, NULL, 0);
}

/**
 *******************************************************************************
 * @brief example of sending and receiving in dma mode
 *        1. send string "hello"
 *        2. when the user passes 5 characters, the chip will send those 5
 *           characters back
 *******************************************************************************
 */
void example_uart_dma(void)
{
    drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));

    uart_config_t uart_cfg = {
        .baudrate       = UART_BAUDRATE,
        .flow_control   = UART_FLOW_CONTROL_NONE,
        .data_bit       = UART_DATA_BIT_8,
        .stop_bit       = UART_STOP_BIT_1,
        .parity         = UART_PARITY_NONE,
    };

    drv_uart_init(TEST_UART, &uart_cfg);
    drv_uart_register_isr_callback(TEST_UART, test_uart_cb);

    drv_uart_gpdma_channel_allocate(TEST_UART, DRV_GPDMA_CHAN_ALL);

    memcpy(send_data, "hello\r\n", 7);
    drv_uart_write_dma(TEST_UART, (uint8_t *)send_data, 7);
    while (!(wait_flag.st & SEND_DMA_IS_DONE));
    wait_flag.st &= ~SEND_DMA_IS_DONE;

    drv_uart_read_dma(TEST_UART, (uint8_t *)rec_data, 5);
    while (!(wait_flag.st & READ_DMA_IS_DONE));
    wait_flag.st &= ~READ_DMA_IS_DONE;

    drv_uart_gpdma_channel_release(TEST_UART, DRV_GPDMA_CHAN_ALL);
}

/**
 *******************************************************************************
 * @brief example of sending and receiving in interrupt mode with flow control
 *        1. send string "hello"
 *        2. send what chip receives
 *
 *******************************************************************************
 */
void example_uart_flow_control(void)
{
    drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));

    uart_config_t uart_cfg = {
        .baudrate       = UART_BAUDRATE,
        .flow_control   = UART_FLOW_CONTROL_RTS_CTS,
        .data_bit       = UART_DATA_BIT_8,
        .stop_bit       = UART_STOP_BIT_1,
        .parity         = UART_PARITY_NONE,
    };

    drv_uart_init(TEST_UART, &uart_cfg);
    drv_uart_register_isr_callback(TEST_UART, test_uart_cb);

    memcpy(send_data, "hello\r\n", 7);
    drv_uart_write_int(TEST_UART, (uint8_t *)send_data, 7);
    while (!(wait_flag.st & SEND_INT_IS_DONE));
    wait_flag.st &= ~SEND_INT_IS_DONE;

    drv_uart_read_int(TEST_UART, NULL, 0);
}

/**
 *******************************************************************************
 * @brief example of sending in LIN mode
 *
 *******************************************************************************
 */
static uint8_t uart_lin_id_calcu(uint8_t id5_0)
{
    uint8_t id6_odd_check  =  ((id5_0>>0)^(id5_0>>1)^(id5_0>>2)^(id5_0>>4)) & 0x1;
    uint8_t id7_even_check = !(((id5_0>>1)^(id5_0>>3)^(id5_0>>4)^(id5_0>>5)) & 0x1);
    id5_0 |= (id6_odd_check<<6);
    id5_0 |= (id7_even_check<<7);
    return id5_0;
}

static uint8_t uart_lin_get_check_sum(uint8_t *data, uint8_t num)
{
    uint8_t checksum = 0;
    // if (modesel)
    for (uint32_t i=0; i<num; i++) {
        checksum += data[i];
    }
    return ~checksum;
}

void example_uart_lin_tx(void)
{
    drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));

    OM_UART_Type *om_uart;
    uint16_t write_num;
    uint8_t test_data[11];
    // 1) header
    uint8_t synch_field = 0x55;
    // protected identifier filed
    uint8_t id5_0 = 0x22;
    uint8_t id = uart_lin_id_calcu(id5_0);
    // 2) response filed
    // data filed
    uint8_t lin_data[8] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7};
    // check sum
    uint8_t check_sum = uart_lin_get_check_sum(lin_data, sizeof(lin_data)/sizeof(lin_data[0]));
    // send data
    test_data[0] = synch_field;
    test_data[1] = id;
    for (uint8_t i=0; i<sizeof(lin_data); i++) {
        test_data[2+i] = lin_data[i];
    }
    test_data[10] = check_sum;

    om_error_t error;

    om_uart = TEST_UART_LIN;
    const uart_config_t uart_cfg = {
        .baudrate     = 115200,
        .flow_control = UART_FLOW_CONTROL_NONE,
        .data_bit     = UART_DATA_BIT_8,
        .stop_bit     = UART_STOP_BIT_1,
        .parity       = UART_PARITY_NONE,
        .lin_enable   = 1,
    };

    error = drv_uart_init(om_uart, &uart_cfg);
    OM_ASSERT(OM_ERROR_OK == error);
    DRV_DELAY_MS(100);

    drv_uart_lin_send_break(om_uart);
    write_num = drv_uart_write(om_uart, test_data, sizeof(test_data), 100);
    OM_ASSERT(sizeof(test_data) == write_num);
}

/**
 *******************************************************************************
 * @brief example of receiving in LIN mode
 *
 *******************************************************************************
 */
static void uart_lin_rx_cb(OM_UART_Type *om_uart, drv_event_t event, uint8_t *data, uint32_t num)
{
    if (DRV_EVENT_UART_LIN_BREAK_DETECT == event) {
        is_break = true;
    }
}

void example_uart_lin_rx(void)
{
    drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));

    OM_UART_Type *om_uart;
    uint8_t test_data[12];
    uint8_t target_data[12];
    om_error_t error;
    om_uart = TEST_UART_LIN;
    uart_config_t uart_cfg = {
        .baudrate     = 115200,
        .flow_control = UART_FLOW_CONTROL_NONE,
        .data_bit     = UART_DATA_BIT_8,
        .stop_bit     = UART_STOP_BIT_1,
        .parity       = UART_PARITY_NONE,
        .lin_enable   = 1,
    };

    uint8_t id5_0 = 0x22;
    target_data[0] = 0; // break
    target_data[1] = 0x55;
    target_data[2] = uart_lin_id_calcu(id5_0);
    for (uint16_t i=0; i<=8; i++) {
        target_data[i+3] = i & 0xFF;
    }
    target_data[11] =uart_lin_get_check_sum(&target_data[3], 8);

    error = drv_uart_init(om_uart, &uart_cfg);
    OM_ASSERT(OM_ERROR_OK == error);
    drv_uart_register_isr_callback(om_uart, (drv_isr_callback_t)uart_lin_rx_cb);

    error = drv_uart_read_int(om_uart, test_data, sizeof(test_data));
    OM_ASSERT(OM_ERROR_OK == error);
    // wait rx completed for 20s
    for(uint32_t i=0; i<2000; i++) {
        uint32_t count;

        DRV_DELAY_MS(10);
        count = drv_uart_get_read_count(om_uart);
        if (count == sizeof(test_data)) {
            break;
        }
    }

    uart_cfg.lin_enable = 0;
    error = drv_uart_init(om_uart, &uart_cfg);
    OM_ASSERT(OM_ERROR_OK == error);

    OM_ASSERT(true == is_break);
    // recv 1st data is break, discard it
    for (uint32_t i=0; i<sizeof(target_data); i++) {
        OM_ASSERT(target_data[i] == test_data[i]);
    }
}

/**
 *******************************************************************************
 * @brief example of half-duplex sending in blocking mode
 *        1. configure uart in half-duplex mode
 *        2. send data
 *******************************************************************************
 */
void example_uart_half_duplex_tx(void)
{
    drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));

    OM_UART_Type *om_uart = TEST_UART;
    uint8_t test_data[256];
    uint16_t write_num;

    const uart_config_t uart_cfg = {
        .baudrate       = UART_BAUDRATE,
        .flow_control   = UART_FLOW_CONTROL_NONE,
        .data_bit       = UART_DATA_BIT_8,
        .stop_bit       = UART_STOP_BIT_1,
        .parity         = UART_PARITY_NONE,
        .half_duplex_en = 1,
        .lin_enable     = 0,
    };

    drv_uart_init(om_uart, &uart_cfg);
    DRV_DELAY_MS(100);

    for (uint32_t i = 0; i < sizeof(test_data); i++) {
        test_data[i] = i & 0xFF;
    }

    write_num = drv_uart_write(om_uart, test_data, sizeof(test_data), 1000);
    OM_ASSERT(sizeof(test_data) == write_num);
}

/**
 *******************************************************************************
 * @brief example of half-duplex receiving in interrupt mode
 *        1. configure uart in half-duplex mode
 *        2. enter rx mode and wait for external device to send data
 *******************************************************************************
 */
void example_uart_half_duplex_rx(void)
{
    drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));

    OM_UART_Type *om_uart = TEST_UART;
    uint8_t test_data[256];
    uint8_t target_data[256];
    om_error_t error;

    const uart_config_t uart_cfg = {
        .baudrate       = UART_BAUDRATE,
        .flow_control   = UART_FLOW_CONTROL_NONE,
        .data_bit       = UART_DATA_BIT_8,
        .stop_bit       = UART_STOP_BIT_1,
        .parity         = UART_PARITY_NONE,
        .half_duplex_en = 1,
        .lin_enable     = 0,
    };

    error = drv_uart_init(om_uart, &uart_cfg);
    OM_ASSERT(OM_ERROR_OK == error);
    DRV_DELAY_MS(100);

    for (uint32_t i = 0; i < sizeof(test_data); i++) {
        test_data[i] = i & 0xFF;
    }

    for (uint16_t i = 0; i < sizeof(target_data); i++) {
        target_data[i] = i & 0xFF;
    }
    memset(test_data, 0, sizeof(test_data));

    drv_uart_register_isr_callback(om_uart, test_uart_cb);

    error = drv_uart_read_int(om_uart, test_data, sizeof(test_data));
    OM_ASSERT(OM_ERROR_OK == error);

    // wait rx completed for 20s
    for (uint32_t i = 0; i < 2000; i++) {
        uint32_t count;

        DRV_DELAY_MS(10);
        count = drv_uart_get_read_count(om_uart);
        if (count == sizeof(test_data)) {
            break;
        }
    }

    // check rx data
    for (uint32_t i = 0; i < sizeof(test_data); i++) {
        OM_ASSERT(target_data[i] == test_data[i]);
    }
}

/** @} */