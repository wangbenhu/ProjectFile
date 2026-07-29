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
#include "om_log.h"

/* Kernel includes. */
#include "cmsis_os2.h"

/*********************************************************************
 * MACROS
 */


/*********************************************************************
 * TYPEDEFS
 */


/*********************************************************************
 * CONSTANTS
 */


/*********************************************************************
 * LOCAL VARIABLES
 */
static osEventFlagsId_t xEvtEvent = NULL;

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERN FUNCTIONS
 */
extern void usbd_cdc_acm_init(void);
extern void usbd_cdc_acm_data_send(const char *pdata, uint32_t length);

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static void hardware_init(void)
{

}

static void vEvtEventHandler(void)
{
    if (xEvtEvent) {
        osEventFlagsSet(xEvtEvent, 0x0001);
    }
}

static void cmd_shell_cdc_tx(int argc, char *argv[])
{
    if (strcmp(argv[0], "tx") == 0) {
        if (argc < 1) {
            om_printf("USBD: invalid params\n");
            return;
        }

        usbd_cdc_acm_data_send(argv[1], strlen(argv[1]));
    } else {
        om_printf("USBD: invalid command\n");
    }
}

static void vEvtScheduleTask(void *argument)
{
    uint32_t uxBits;
    const shell_cmd_t shell_cmd[] = {
        { "cdc",     cmd_shell_cdc_tx,  "tx <string>" },
        { NULL,      NULL,               NULL},     /* donot deleted */
    };

    evt_init();
    hardware_init();
    shell_init(shell_cmd);
    usbd_cdc_acm_init();

    // Create event
    xEvtEvent = osEventFlagsNew(NULL);

    // set ke event callback
    evt_schedule_trigger_callback_set(vEvtEventHandler);

    while (1) {
        // schedule
        evt_schedule();

        // Wait for event
        uxBits = osEventFlagsWait(xEvtEvent, 0xFFFF, osFlagsWaitAny, osWaitForever);
        (void)uxBits;
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
    const osThreadAttr_t evtThreadAttr = {
        .name = NULL,
        .attr_bits = 0,
        .cb_mem = NULL,
        .cb_size = 0,
        .stack_mem = NULL,
        .stack_size = 1024,
        .priority = osPriorityRealtime,
        .tz_module = 0,
    };

    // Create ble Task
    osThreadNew(vEvtScheduleTask, NULL, &evtThreadAttr);
}

/** @} */

// vim: fdm=marker
