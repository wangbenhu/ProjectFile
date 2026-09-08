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
 * @brief    system
 * @details  system
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
#include "bsp.h"
#include "om_log.h"
#include "om_driver.h"
#include "nvds.h"
#if (CONFIG_PM)
#include "pm.h"
#endif
#include "om_log.h"

#if (CONFIG_SHELL)
#include "shell.h"
#endif
#if (CONFIG_FAULT_HANDLE)
#include "fault_handle.h"
#endif

#include "lfs_port.h"

#include "shell_uart_port.h"
#include "m_motor.h"
//#include "app_cdc_acm.h"
#include "cmsis_os2.h"
#include "common_def.h"

#include "imu_ex.h"
#include "board_define.h"
#include "pedometer_ex.h"

//#define log_debug(...) om_log(OM_LOG_INFO, ##__VA_ARGS__)

static pmu_reboot_reason_t m_system_reboot_reson = PMU_REBOOT_FROM_POWER_ON;




/*******************************************************************************
 * Extern FUNCTIONS
 */
extern void pm_gpadc_int(void);
	
extern void motion_int_num_add(void);
void Entry_Control_Stop_Power_Charge_FlagSet(void);
extern osThreadId_t vStartBLEScheduleTask(void);
extern void peripheral_other_init(void);
extern void SensorGetFifoEvent(void);
/*******************************************************************************
 * LOCAL FUNCTIONS
 */
 /*
* @brief    set reboot reason
* @details  set reboot reason
* @version
* Version 1.0
*  - Initial release
*/
static void m_system_set_reboot_reason(void)
{
    m_system_reboot_reson=drv_pmu_reboot_reason();
}/*
* @brief    get reboot reason
* @details  get reboot reason
* @version
* Version 1.0
*  - Initial release
*/
pmu_reboot_reason_t m_system_get_reboot_reason(void)
{
    return m_system_reboot_reson;
}
/*
* @brief    lom power mode enable
* @details  lom power mode enable
* @version
* Version 1.0
*  - Initial release
*/
 void low_power_mode_enable(void)
{
	// output high level
    drv_gpio_write(OM_GPIO0, GPIO_MASK(PAD_CONTROL_LOW_POWER), GPIO_LEVEL_LOW);
}
/*
* @brief    lom power mode disable
* @details  lom power mode disable
* @version
* Version 1.0
*  - Initial release
*/
void lom_power_mode_disable(void)
{
	// output high level
    drv_gpio_write(OM_GPIO0, GPIO_MASK(PAD_CONTROL_LOW_POWER), GPIO_LEVEL_HIGH);
}

uint8_t get_gpio_group_idx(OM_GPIO_Type *om_gpio)
{
    uint8_t idx = 0xff;
    if (om_gpio == OM_GPIO0) {
        idx = 0;
    } else if (om_gpio == OM_GPIO1) {
        idx = 1;
    } 
    return idx;
} 
/*
* @brief    DogCollar GPIO EXTI Callback
* @details  DogCollar GPIO EXTI Callback
* @version
* Version 1.0
*  - Initial release
*/
static void DogCollar_GPIO_EXTI_Callbac(void *om_gpio, drv_event_t event, void *int_status, void *gpio_data)
{
 //   log_debug("gpio trigger happens, int_status:%x, gpio_data:%x %d\r\n", (uint32_t)int_status, (uint32_t)gpio_data,event);
  uint8_t gpio_group_idx = get_gpio_group_idx(om_gpio);
	uint32_t int_mask = (uint32_t)int_status;
	if(gpio_group_idx == 0)
	{
		if(GPIO_MASK(PAD_CHAGE_CHECK) & (uint32_t)int_status)
		{
			if(drv_gpio_read(OM_GPIO0,GPIO_MASK(PAD_CHAGE_CHECK)))
			{

			}
			else
			{
				Entry_Control_Stop_Power_Charge_FlagSet();
			}
		}
		if((int_mask & BITMASK(IMU_INT1)))
		{
			set_int1_status();  // ISR 内只置标志
			SensorGetFifoEvent();//唤醒传感器任务
		}
	
	}
    //log_debug( "gpio_group_idx = %d, int_status:%x, gpio_data:%x %d\r\n",gpio_group_idx, (uint32_t)int_status, (uint32_t)gpio_data,event);
}

void example_pin_wakeup_isr_handler(void *om_reg, drv_event_t event, void *param0, void *param1)
{
	log_debug("example_pin_wakeup_isr_handler\r\n");
}
void SystemInitPeripherals(void)
{
    const flash_config_t config = {
        .clk_div = 0,
        .delay = FLASH_DELAY_AUTO,
        .read_cmd = FLASH_FAST_READ_QIO,
        .write_cmd = FLASH_PAGE_PROGRAM,
        .spi_mode = FLASH_SPI_MODE_0,
    };
    board_init();
	DRV_DELAY_MS(500);//ÖØÆôÉÏµçÔ¼Îª500ms
    drv_wdt_init(0);
    drv_flash_init(OM_FLASH0, &config);

	m_system_set_reboot_reason();

#if (SHELL_UART_TEST)
    shell_uart_init();
#else
    OM_LOG_INIT();
#endif // (SHELL_UART_TEST)

    low_power_mode_enable();
	
	pm_gpadc_int();
	
   drv_pmu_wakeup_pin_set(PAD_CHAGE_CHECK, PMU_PIN_WAKEUP_HIGH_LEVEL);
   drv_gpio_register_isr_callback(OM_GPIO0, DogCollar_GPIO_EXTI_Callbac);
   drv_pmu_wakeup_pin_register_callback(example_pin_wakeup_isr_handler);
}
#if (CONFIG_PM)
void pm_sleep_callback(pm_sleep_state_t sleep_state, pm_status_t power_status)
{
    switch (sleep_state) {
        case PM_SLEEP_ENTRY:
			log_debug("+\r\n");
            break;
        case PM_SLEEP_RESTORE_HSI:
            break;
        case PM_SLEEP_RESTORE_HSE:
            break;
        case PM_SLEEP_LEAVE_BOTTOM_HALF:
			log_debug("-\r\n");
            break;
        default:
            break;
    }
}
#endif

#if (RTE_PMU_POF_REGISTER_CALLBACK)
static void pmu_pof_isr_callback(void *om_pmu, drv_event_t event, void *voltage, void *mode)
{
//    OM_LOG(OM_LOG_WARN, "PMU POF event occured, voltage: [%d], mode: [%d]",
//        (uint32_t)voltage, (uint32_t)mode);
}
#endif

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */

void system_init(void)
{
	SystemInitPeripherals();
    nvds_init(0);
    // pmu pof enable
    #if (RTE_PMU_POF_REGISTER_CALLBACK)
    drv_pmu_pof_register_callback(pmu_pof_isr_callback);
    #endif
    drv_pmu_pof_enable(true, PMU_POF_VOLTAGE_2P5V, PMU_POF_INT_NEG_EDGE);

    #if (CONFIG_PM)
    pm_init();
    pm_sleep_enable(false); /*motion*/
	pm_sleep_ultra_sleep_mode_enable(true);
    pm_sleep_notify_user_callback_register(pm_sleep_callback);
    #endif
	
}

/** @} */
