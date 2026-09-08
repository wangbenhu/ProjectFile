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
 * @brief    shell task
 * @details  shell task
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
#include "autoconf.h"
#if (CONFIG_NON_RTOS)
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include "shell_common.h"
#include "shell.h"
#include "om_printf.h"
#include "om_compiler.h"
#include "om_driver.h"
#if (CONFIG_EVT)
#include "evt.h"
#endif
#include "om_ringbuff.h"
#if (CONFIG_PM)
#include "pm.h"
#endif


/*******************************************************************************
 * TYPEDEFS
 */
typedef void (*shell_retarget_init_t)(void);
typedef void (*shell_retarget_out_t)(char c);
typedef struct {
    shell_retarget_init_t    init;
    shell_retarget_out_t     out;
} shell_retarget_handle_t;

typedef struct {
    const shell_retarget_handle_t *retarget_handle;
    ring_buff_t                    rx_rb;
    uint8_t                        rx_rb_buff[20];
    char                           shell_line[SHELL_MAX_ARGUMENTS];
    uint32_t                       shell_line_index;
    const shell_cmd_t             *shell_line_cmd;
} shell_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
static shell_env_t shell_env;


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
/* 1. shell retarget to UARTx ---------------------------------------------- */
#ifndef CONFIG_SHELL_UART_IDX
#define CONFIG_SHELL_UART_IDX     1
#endif
#ifndef CONFIG_SHELL_BAUDRATE
#define CONFIG_SHELL_BAUDRATE     115200
#endif

#if (CONFIG_SHELL_UART_IDX == 0 && RTE_UART0)
#define SHELL_UART   OM_UART0
#endif
#if (CONFIG_SHELL_UART_IDX == 1 && RTE_UART1)
#define SHELL_UART   OM_UART1
#endif
#if (CONFIG_SHELL_UART_IDX == 2 && RTE_UART2)
#define SHELL_UART   OM_UART2
#endif

static void shell_uart_cb(void *om_reg, drv_event_t event, void *rx_buf, void *rx_cnt)
{
    (void)om_reg;
    if (event & DRV_EVENT_COMMON_READ_COMPLETED) {
        om_ringbuff_write(&(shell_env.rx_rb), (uint8_t *)rx_buf, (uint16_t)((uint32_t)rx_cnt));
        drv_uart_read_int(SHELL_UART, NULL, 0U);
        #if (CONFIG_EVT)
        evt_set(EVT_TYPE_SHELL);
        #endif
    }
}

static void shell_uart_init(void)
{
    static const uart_config_t uart_cfg = {
        .baudrate     = CONFIG_SHELL_BAUDRATE,
        .flow_control = UART_FLOW_CONTROL_NONE,
        .data_bit     = UART_DATA_BIT_8,
        .stop_bit     = UART_STOP_BIT_1,
        .parity       = UART_PARITY_NONE,
    };

    drv_uart_init(SHELL_UART, &uart_cfg);
    drv_uart_register_isr_callback(SHELL_UART, shell_uart_cb);
    drv_uart_read_int(SHELL_UART, NULL, 0U);
}

static void shell_uart_out(char c)
{
    OM_CRITICAL_BEGIN();
    while(1U != drv_uart_write(SHELL_UART, (uint8_t *)&c, 1, 200));
    OM_CRITICAL_END();
}

static const shell_retarget_handle_t shell_retarget_uart_handle = {
    .init = shell_uart_init,
    .out  = shell_uart_out,
};

#if (CONFIG_PM)
static void shell_restore_callback(pm_sleep_state_t sleep_state, pm_status_t power_status)
{
    if (sleep_state == PM_SLEEP_RESTORE_HSI) {
        shell_restore();
    }
}
#endif

void shell_restore(void)
{
    shell_uart_init();
}

#if (CONFIG_EVT)
static void shell_evt_handler(void)
{
    uint8_t rx_ch;
    uint16_t rx_len;

    evt_clear(EVT_TYPE_SHELL);

    #if (RTE_PM)
    pm_sleep_prevent(PM_ID_SHELL);
    #endif  /* (RTE_PM) */

    //process rx character
    while (1) {
        OM_CRITICAL_BEGIN();
        rx_len = om_ringbuff_read(&(shell_env.rx_rb), &rx_ch, 1);
        OM_CRITICAL_END();
        if (rx_len) {
            if (shell_get_line(rx_ch, shell_env.shell_line, sizeof(shell_env.shell_line), (unsigned *)(&shell_env.shell_line_index)) == true) {
                if (shell_env.shell_line_index != 0) {
                    shell_main(shell_env.shell_line, shell_env.shell_line_cmd);
                    memset(shell_env.shell_line, 0x00, sizeof(shell_env.shell_line));
                    shell_env.shell_line_index = 0x00;
                }
                om_printf(">");
            }
        } else {
            break;
        }
    }

    #if (RTE_PM)
    if (shell_env.shell_line_index == 0) {
        pm_sleep_allow(PM_ID_SHELL);
    }
    #endif  /* (RTE_PM) */
}
#endif /* (CONFIG_EVT) */


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
void shell_init(const shell_cmd_t *cmd)
{
    memset(shell_env.shell_line, 0x00, sizeof(shell_env.shell_line));
    shell_env.shell_line_cmd = cmd;
    #if (CONFIG_EVT)
    evt_callback_set(EVT_TYPE_SHELL, shell_evt_handler);
    #endif
    om_ringbuff_init(&(shell_env.rx_rb), &(shell_env.rx_rb_buff[0]), sizeof(shell_env.rx_rb_buff));
    shell_env.retarget_handle = &shell_retarget_uart_handle;

    OM_ASSERT(shell_env.retarget_handle);
    if (shell_env.retarget_handle->init) {
        shell_env.retarget_handle->init();
    }

    // shell logo
    om_printf("\r\n");
    #ifdef __SDK_VERSION
    om_printf("SDK Version:            %s\r\n", __SDK_VERSION);
    #endif
    #ifdef __PROJECT_VERSION
    om_printf("Project Version:        %s\r\n", __PROJECT_VERSION);
    #endif
    #if defined(__DATE__) && defined(__TIME__)
    om_printf("Build time:             %s-%s\r\n", __DATE__, __TIME__);
    #endif
    om_printf(">");

    #if (CONFIG_PM)
    pm_sleep_store_restore_callback_register(shell_restore_callback);
    #endif
}

void shell_exec(const char *cmd)
{
    om_printf("%s\r\n", cmd);
    shell_main((char *)cmd, shell_env.shell_line_cmd);
}

int shell_vprintf(const char *fmt, va_list va)
{
    return om_vprintf(fmt, va);
}

void om_putchar(char character)
{
    if (shell_env.retarget_handle->out != NULL) {
        shell_env.retarget_handle->out(character);
    }
}


#endif  /* (CONFIG_NON_RTOS) */

/** @} */
