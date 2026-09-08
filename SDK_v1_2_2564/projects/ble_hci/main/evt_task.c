/**
 * @file  examples/ble_app_simple_server/src/main.c
 * @brief  simple server
 * @date Wed, Sep  5, 2018  5:19:05 PM
 * @author liqiang
 *
 * @addtogroup APP_SIMPLE_SERVER_MAIN main.c
 * @ingroup APP_SIMPLE_SERVER
 * @details simple server
 *
 * @{
 */

/*********************************************************************
 * INCLUDES
 */
#include "om_driver.h"
#include "shell.h"
#include "evt.h"
#include "pm.h"
#include "bsp.h"
#include "nvds.h"

// Controller header
#include "obc.h"

/* Kernel includes. */
#include "cmsis_os2.h"

/*********************************************************************
 * MACROS
 */
#define HCI_UART    OM_UART1

#define EVT_TYPE_HCI_H4  ((evt_type_t)(EVT_TYPE_USR_FIRST+0))

// Used for 2 wire mode instead of hci
// #define DTM_USE_2WIRE_MODE

#define EVENT_SYSTEM_RESERVE_MASK   0x00FF


/*********************************************************************
 * TYPEDEFS
 */


/*********************************************************************
 * CONSTANTS
 */


/*********************************************************************
 * LOCAL VARIABLES
 */
static osSemaphoreId_t xSemBluetooth = NULL;

static om_fifo_t hci_h4_rx_fifo;
static uint8_t hci_h4_rx_pool[512];

static const uart_config_t hci_uart_cfg = {
    .baudrate       = 115200, //115200, // 1000000
    .flow_control   = UART_FLOW_CONTROL_NONE,
    .data_bit       = UART_DATA_BIT_8,
    .stop_bit       = UART_STOP_BIT_1,
    .parity         = UART_PARITY_NONE,
    .half_duplex_en = 0,
    .lin_enable     = 0,
};

/*********************************************************************
 * GLOBAL VARIABLES
 */


/*********************************************************************
 * LOCAL FUNCTIONS
 */

static void hci_h4_transmit_handler(const uint8_t *pdata, uint32_t length)
{
    drv_uart_write(HCI_UART, pdata, length, DRV_MAX_DELAY);
}

static void hci_h4_uart_rx_handler(void *om_uart, drv_event_t event, void *rx_buf, void *rx_cnt)
{
    if (event & DRV_EVENT_COMMON_READ_COMPLETED) {
        om_fifo_in(&hci_h4_rx_fifo, rx_buf, (uint32_t)rx_cnt);
        evt_set(EVT_TYPE_HCI_H4);
    }
}

static void hci_h4_uart_rx_evt_handler(void)
{
    uint8_t buffer[64];
    uint32_t length;

    evt_clear(EVT_TYPE_HCI_H4);

    while (1) {

        length = om_fifo_out(&hci_h4_rx_fifo, buffer, sizeof(buffer));

        if (length == 0)
            break;

#ifdef DTM_USE_2WIRE_MODE
        obc_2wire_read_handler(buffer, length);
#else
        obc_hci_h4_receive_handler(buffer, length);
#endif
    }
}

static void ble_controller_init(void)
{
    obc_init();

    om_fifo_init(&hci_h4_rx_fifo, hci_h4_rx_pool, sizeof(hci_h4_rx_pool));

    evt_callback_set(EVT_TYPE_HCI_H4, hci_h4_uart_rx_evt_handler);

    obc_hci_h4_transmit_callback_set(hci_h4_transmit_handler);

#ifdef DTM_USE_2WIRE_MODE
    obc_2wire_init(hci_h4_transmit_handler);
#endif
}

static void hardware_init(void)
{
    drv_uart_init(HCI_UART, &hci_uart_cfg);
    drv_uart_register_isr_callback(HCI_UART, hci_h4_uart_rx_handler);
    drv_uart_read_int(HCI_UART, NULL, 0);
}

void cmd_shell_pta(int argc, char *argv[])
{
#if 0
    if (strcmp(argv[0], "start") == 0) {
        if (argc < 1) {
            OM_LOG_DEBUG("LLT: invalid params\n");
            return;
        }

        DRV_PIN_MUX_SET(2, PINMUX_PTA_ACTIVE_IN_CFG);
        DRV_PIN_MUX_SET(15, PINMUX_PTA_ACTIVE_OUT_CFG);
        DRV_PIN_MUX_SET(16, PINMUX_PTA_PRIORITY_CFG);
        DRV_PIN_MUX_SET(17, PINMUX_PTA_FREQ_CFG);

        obc_pta_ctrl_t ctrl;
        ctrl.grant_active_level = 0;
        ctrl.priority_txrx_include = 1;
        ctrl.priority_txrx_tx_active_level = 1;
        ctrl.priority_prio_active_keep_us = 20;
        ctrl.priority_txrx_active_keep_us = 30;
        ctrl.priority_threshold = strtoul(argv[1], NULL, 0);
        ctrl.priority_state_initiating_connection_ind_rsp = 2;
        ctrl.priority_state_connection_llcp = 15;
        ctrl.priority_state_connection_data_channel = 10;
        ctrl.priority_state_initiating_scanning = 5;
        ctrl.priority_state_active_scanning = 2;
        ctrl.priority_state_connectable_advertising = 4;
        ctrl.priority_state_non_connectable_advertising = 4;
        ctrl.priority_state_passive_scanning = 2;

        OM_LOG_DEBUG("PTA: priority_threshold=%d\n", ctrl.priority_threshold);

        obc_pta_enable(true, &ctrl);

    } else if (strcmp(argv[0], "stop") == 0) {
        obc_pta_enable(false, NULL);

    } else {
        OM_LOG_DEBUG("PTA: invalid command\n");
    }
#endif
}

/**
 *******************************************************************************
 * @brief  evt timer 0 handler
 *
 * @param[in] timer  timer
 * @param[in] param  param
 *******************************************************************************
 */

/**
 * @brief  bluetooth event handler
 **/
static void vEvtEventHandler(void)
{
    if(xSemBluetooth) {
        osSemaphoreRelease(xSemBluetooth);
    }
}

/**
 * @brief  bluetooth schedule task
 *
 * @param[in] pvParameters  pv parameters
 **/
static void vEvtScheduleTask(void *argument)
{
    hardware_init();
    drv_rf_init();
    evt_init();
    ble_controller_init();

    // Create semaphore
    xSemBluetooth = osSemaphoreNew(1, 0, NULL);

    // set ke event callback
    evt_schedule_trigger_callback_set(vEvtEventHandler);

    while (1)
    {
        // schedule
        evt_schedule();

        // Wait for semaphore
        osSemaphoreAcquire(xSemBluetooth, osWaitForever);
    }
}

/*********************************************************************
 * CONST VARIABLES
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/**
 * @brief  v start bluetooth task
 **/
void vStartEvtTask(void)
{
    const osThreadAttr_t bluetoothThreadAttr =
    {
        .name = NULL,
        .attr_bits = 0,
        .cb_mem = NULL,
        .cb_size = 0,
        .stack_mem = NULL,
        .stack_size = 2048,
        .priority = osPriorityRealtime,
        .tz_module = 0,
    };

    // Create ble Task
    osThreadNew(vEvtScheduleTask, NULL, &bluetoothThreadAttr);
}

/** @} */

// vim: fdm=marker
