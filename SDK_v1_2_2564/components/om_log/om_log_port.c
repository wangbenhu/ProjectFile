/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup LOG PORT
 * @ingroup  LOG
 * @brief
 * @details
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
#if CONFIG_OM_LOG
#include "om_log.h"
#include "om_log_config.h"
#include "om_common.h"
#include "om_driver.h"
#if (!CONFIG_NON_RTOS)
#include "cmsis_os2.h"
#endif
#if (CONFIG_OM_LOG_TO_SHELL)
#include "shell.h"
#elif (CONFIG_OM_LOG_TO_RTT)
#include "SEGGER_RTT.h"
#endif

#if (CONFIG_SHELL && CONFIG_OM_LOG_TO_UART && (CONFIG_SHELL_UART_IDX == CONFIG_OM_LOG_UART_IDX))
#error "CONFIG_OM_LOG_UART_IDX and CONFIG_SHELL_UART_IDX are conflict"
#endif


/*******************************************************************************
 * CONST & VARIABLES
 */
#if (!CONFIG_NON_RTOS)
static osMutexId_t log_out_mutex;
#endif

#if (CONFIG_OM_LOG_TO_FLASH)
static uint8_t log_rb_buf[CONFIG_OM_LOG_SAVE_SIZE];
static ring_buff_t log_rb = {
    .buf = log_rb_buf,
    .size = CONFIG_OM_LOG_SAVE_SIZE,
};
#endif


/*******************************************************************************
 * PUBLIC FUNCTION
 */
// log initialize porting function
void log_out_init(void)
{
    #if (!CONFIG_NON_RTOS)
    if (log_out_mutex == NULL) {
        log_out_mutex = osMutexNew(NULL);
    }
    #endif

    #if (CONFIG_OM_LOG_TO_UART)
    OM_UART_Type *uart = drv_uart_idx2base(CONFIG_OM_LOG_UART_IDX);
    static const uart_config_t uart_cfg = {
        #if (CONFIG_OM_LOG_UART_BAUDRATE)
        .baudrate     = CONFIG_OM_LOG_UART_BAUDRATE,
        #else
        .baudrate     = 115200,
        #endif
        .flow_control = UART_FLOW_CONTROL_NONE,
        .data_bit     = UART_DATA_BIT_8,
        .stop_bit     = UART_STOP_BIT_1,
        .parity       = UART_PARITY_NONE,
    };
    drv_uart_init(uart, &uart_cfg);
    #endif /* CONFIG_OM_LOG_TO_UART */

    #if (CONFIG_OM_LOG_TO_FLASH)
    om_ringbuff_init(&log_rb, log_rb_buf, sizeof(log_rb_buf));
    #endif

    #if (CONFIG_OM_LOG_TO_RTT)
    SEGGER_RTT_Init();
    #endif
}

// log putout porting function
#if (CONFIG_OM_LOG_TO_SHELL)
void log_shell_out(const char *fmt, va_list va)
{
    shell_vprintf(fmt, va);
}
#elif (CONFIG_OM_LOG_TO_RTT)
void log_rtt_out(om_log_lvl_t level, const char *fmt, va_list va)
{
    (void)level;
    SEGGER_RTT_vprintf(0, fmt, &va);
}
#elif (CONFIG_OM_LOG_TO_UART)
void log_out(uint8_t *buf, uint32_t len)
{
    OM_UART_Type *uart = drv_uart_idx2base(CONFIG_OM_LOG_UART_IDX);

    #if (!CONFIG_NON_RTOS)
    osMutexAcquire(log_out_mutex, osWaitForever);
    #endif
    drv_uart_write(uart, buf, len, 200);
    #if (!CONFIG_NON_RTOS)
    osMutexRelease(log_out_mutex);
    #endif
}
#elif (CONFIG_OM_LOG_TO_FLASH)
void log_out(uint8_t *buf, uint32_t len)
{
    om_ringbuff_write(&log_rb, buf, len);
}
#else
void log_out(uint8_t *buf, uint32_t len)
{
    // add other buffer output codes
}
#endif

// log flush porting function
void log_out_flush(void)
{
    #if (CONFIG_OM_LOG_TO_FLASH)
    uint32_t irq_save;

    OM_CRITICAL_BEGIN_EX(irq_save);

    #if (CONFIG_OM_LOG_FLASH_INTERNAL)
    OM_FLASH_Type flash = OM_FLASH0;
    #elif (CONFIG_OM_LOG_FLASH_EXTERNAL)
    OM_FLASH_Type flash = OM_FLASH1;
    #endif

    // flash init
    flash_config_t config = {
        .clk_div = 4,
        .delay = 2,
        .read_cmd = FLASH_READ,
        .write_cmd = FLASH_PAGE_PROGRAM,
        .spi_mode = FLASH_SPI_MODE_0,
    };
    drv_flash_init(flash, &config);
    // erase flash area
    for (uint32_t i = 0; i < CONFIG_OM_LOG_SAVE_SIZE; i += 0x1000) {
        drv_flash_erase(flash, CONFIG_OM_LOG_FLASH_ADDR + i, FLASH_ERASE_4K);
    }
    // write log to flash
    if (om_ringbuff_read_overflow(&log_rb)) {
        drv_flash_write(flash, CONFIG_OM_LOG_FLASH_ADDR,  log_rb.buf + log_rb.wptr,
                log_rb.size - log_rb.wptr - 1);
        drv_flash_write(flash, CONFIG_OM_LOG_FLASH_ADDR + log_rb.size - log_rb.wptr - 1,
                log_rb.buf, log_rb.wptr + 1);
        om_ringbuff_clear_overflow(&log_rb);
    } else {
        drv_flash_write(flash, CONFIG_OM_LOG_FLASH_ADDR,  log_rb.buf, log_rb.wptr + 1);
    }
    // reset ring buffer pointer
    log_rb.wptr = 0;
    log_rb.rptr = 0;

    OM_CRITICAL_END_EX(irq_save);

    #endif /* CONFIG_OM_LOG_TO_FLASH */
}

// log get time porting function
#if CONFIG_OM_LOG_TIME
uint32_t log_get_time(void)
{
    #if (CONFIG_NON_RTOS)
    return drv_pmu_timer_cnt_get();
    #else
    return osKernelGetTickCount();
    #endif
}
#endif

// log restore porting function
#if (CONFIG_OM_LOG_TO_UART)
void log_restore()
{
    log_out_init();
}
#endif

#endif /* CONFIG_OM_LOG */

/** @} */
