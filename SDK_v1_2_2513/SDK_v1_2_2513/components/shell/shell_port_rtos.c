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
 * @brief    shell_port_rtos.c
 * @details  shell_port_rtos.c
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
#if (!CONFIG_NON_RTOS)
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include "shell_common.h"
#include "shell.h"
#include "om_common.h"
#include "om_driver.h"
#include "cmsis_os2.h"
#if (CONFIG_PM)
#include "pm.h"
#endif


/*******************************************************************************
 * MACROS
 */
#define SHELL_RETARGET_EVENT          (1U << 0)
#ifndef CONFIG_SHELL_UART_IDX
#define CONFIG_SHELL_UART_IDX     0
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
    osThreadId_t                   thread_id;
    const shell_retarget_handle_t *retarget_handle;
    const shell_cmd_t             *cmd;
    osMutexId_t                    mutex_id;
    ring_buff_t                    rx_rb;
    uint8_t                        rx_rb_buff[SHELL_MAX_ARGUMENTS];
} shell_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
static shell_env_t shell_env;

static void shell_uart_cb(void *om_reg, drv_event_t event, void *rx_buf, void *rx_cnt)
{
    if (event & DRV_EVENT_COMMON_READ_COMPLETED) {
        om_ringbuff_write(&(shell_env.rx_rb), (uint8_t *)rx_buf, (uint16_t)((uint32_t)rx_cnt));
        drv_uart_read_int((OM_UART_Type *)om_reg, NULL, 0U);
        osThreadFlagsSet(shell_env.thread_id, SHELL_RETARGET_EVENT);
    }
}

static void shell_uart_init(void)
{
    OM_UART_Type *om_uart;

    static const uart_config_t uart_cfg = {
        #if (CONFIG_SHELL_BAUDRATE)
        .baudrate     = CONFIG_SHELL_BAUDRATE,
        #else
        .baudrate     = 115200,
        #endif
        .flow_control = UART_FLOW_CONTROL_NONE,
        .data_bit     = UART_DATA_BIT_8,
        .stop_bit     = UART_STOP_BIT_1,
        .parity       = UART_PARITY_NONE,
    };

    om_uart = drv_uart_idx2base(CONFIG_SHELL_UART_IDX);
    drv_uart_init(om_uart, &uart_cfg);
    drv_uart_register_isr_callback(om_uart, shell_uart_cb);
    drv_uart_read_int(om_uart, NULL, 0U);
}

static void shell_uart_out(char c)
{
    OM_UART_Type *om_uart;

    om_uart = drv_uart_idx2base(CONFIG_SHELL_UART_IDX);
    OM_CRITICAL_BEGIN();
    while(1U != drv_uart_write(om_uart, (uint8_t *)&c, 1, 200));
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

/* Common static functions -------------------------------------------------- */
static void shell_thread(void *param)
{
    char line[SHELL_MAX_ARGUMENTS];
    uint32_t line_index;

    line_index = 0x00;
    memset(line, 0x00, sizeof(line));

    while (1) {
        osThreadFlagsWait(SHELL_RETARGET_EVENT, osFlagsWaitAny, osWaitForever);
        // process rx character
        while (1) {
            uint8_t rx_ch;
            uint16_t rx_len;

            OM_CRITICAL_BEGIN();
            rx_len = om_ringbuff_read(&(shell_env.rx_rb), &rx_ch, 1);
            OM_CRITICAL_END();
            if (rx_len) {
				
                if (shell_get_line(rx_ch, line, sizeof(line), (unsigned *)(&line_index)) == true) {
                    if (line_index != 0) {
                        shell_main(line, (shell_cmd_t *)param);
                        memset(line, 0x00, sizeof(line));
                        line_index = 0x00;
                    }
//				om_printf(">\r\n");
                }
            } else {
                break;
            }
        }
    }
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
void shell_init(const shell_cmd_t *cmd)
{
    static const osThreadAttr_t shell_thread_attr = {
        .name       = "shell",
        .priority   = osPriorityBelowNormal,
        #if (CONFIG_SHELL_STACK_SIZE)
        .stack_size = CONFIG_SHELL_STACK_SIZE,
        #else
        .stack_size = 1024U * 4,
        #endif
    };
    const osMutexAttr_t mutex_attr = {
        .name = "shell",
        .attr_bits  = osMutexRecursive,
        .cb_mem     = NULL,
        .cb_size    = 0,
    };
    shell_env.mutex_id = osMutexNew(&mutex_attr);
    OM_ASSERT(shell_env.mutex_id);

    shell_env.cmd = cmd;
    om_ringbuff_init(&(shell_env.rx_rb), &(shell_env.rx_rb_buff[0]), sizeof(shell_env.rx_rb_buff));
    shell_env.retarget_handle = &shell_retarget_uart_handle;

    if (shell_env.retarget_handle->init) {
        shell_env.retarget_handle->init();
    }

    // shell logo
//    om_printf("\r\n");
    #ifdef __SDK_VERSION
    om_printf("SDK Version:            %s\r\n", __SDK_VERSION);
    #endif
    #ifdef __PROJECT_VERSION
    om_printf("Project Version:        %s\r\n", __PROJECT_VERSION);
    #endif
    #if defined(__DATE__) && defined(__TIME__)
  //  om_printf("Build time:             %s-%s\r\n", __DATE__, __TIME__);
    #endif
  //  om_printf("%s>",cmd->sc_name);
    shell_env.thread_id = osThreadNew(shell_thread, (void *)cmd, &shell_thread_attr);
    OM_ASSERT(shell_env.thread_id);
    #if (CONFIG_PM)
    pm_sleep_store_restore_callback_register(shell_restore_callback);
    #endif
}

void shell_exec(const char *cmd)
{
   // om_printf("shell_exec = %s\r\n", cmd);
    shell_main((char *)cmd, shell_env.cmd);
}

int shell_vprintf(const char *fmt, va_list va)
{
    int formatted_bytes;

    osMutexAcquire(shell_env.mutex_id, osWaitForever);
    formatted_bytes = om_vprintf(fmt, va);
    osMutexRelease(shell_env.mutex_id);

    return formatted_bytes;
}

void om_putchar(char character)
{
    while (shell_env.retarget_handle == NULL);    // not using OM_ASSERT for generation recursion call in OM_ASSERT
    if (shell_env.retarget_handle->out != NULL) {
        shell_env.retarget_handle->out(character);
    }
}

const char *shell_argv_to_cmdline(int argc, char *argv[])
{
    uint32_t cmd_len = 0;

    memset(shell_env.rx_rb_buff, '\0', sizeof(shell_env.rx_rb_buff));
    for (uint32_t i=0; i < argc; i++) {
        uint32_t len = strlen(argv[i]);
        memcpy(&(shell_env.rx_rb_buff[cmd_len]), argv[i], len);
        cmd_len += len;
        shell_env.rx_rb_buff[cmd_len] = ' ';
        cmd_len += 1;
    }

    return (const char *)((cmd_len == 0) ? NULL : &(shell_env.rx_rb_buff[0]));
}

#endif  /* (!CONFIG_NON_RTOS) */

/** @} */
