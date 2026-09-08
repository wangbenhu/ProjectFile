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
 * @brief    interrupt service routine
 * @details  interrupt service routine
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
#include "RTE_driver.h"
#if (RTE_ISR)
#include <stdint.h>
#include <stddef.h>
#include "om_driver.h"


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
#if 0
__WEAK void HardFault_Handler(void)
{
    while(1);
}

__WEAK void MemManage_Handler(void)
{
    while(1);
}

__WEAK void BusFault_Handler(void)
{
    while(1);
}

__WEAK void UsageFault_Handler(void)
{
    while(1);
}

__WEAK void SVC_Handler(void)
{
}

__WEAK void DebugMon_Handler(void)
{
}

__WEAK void PendSV_Handler(void)
{
}

__WEAK void SysTick_Handler(void)
{
}
#endif

#if (RTE_WDT)
__WEAK void fault_wdt_callback(uint32_t exc_return_code, uint32_t sp)
{
    while(1);
}

void NMI_Handler_C(uint32_t exc_return_code, uint32_t sp)
{
    drv_wdt_isr();
    fault_wdt_callback(exc_return_code, sp);
}

void WDT_IRQHandler(void)
{
    drv_wdt_isr();
}
#endif

#if (RTE_GPDMA)
void GPDMA_IRQHandler(void)
{
    drv_gpdma_isr();
}
#endif

#if (RTE_PMU)
void PMU_PIN_WAKEUP_IRQHandler(void)
{
    drv_pmu_pin_wakeup_isr();
}

void PMU_POF_IRQHandler(void)
{
    drv_pmu_pof_isr();
}
#endif

#if (RTE_PMU_TIMER)
void PMU_TIMER_IRQHandler(void)
{
    drv_pmu_timer_isr();
}
#endif

#if (RTE_OM24G)
void OM24G_RF_IRQHandler(void)
{
    drv_om24g_isr();
}

void OM24G_TIM_WAKEUP_IRQHandler(void)
{
    drv_om24g_tim_wakeup_isr();
}
#endif

#if (RTE_UART0)
void UART0_IRQHandler(void)
{
    drv_uart_isr(OM_UART0);
}
#endif

#if (RTE_UART1)
void UART1_IRQHandler(void)
{
    drv_uart_isr(OM_UART1);
}
#endif

#if (RTE_UART2)
void UART2_IRQHandler(void)
{
    drv_uart_isr(OM_UART2);
}
#endif

#if (RTE_EFUSE)
void EFUSE_IRQHandler(void)
{
    drv_efuse_isr();
}
#endif

#if (RTE_GPIO0) || (RTE_GPIO1)
void GPIO_IRQHandler(void)
{
    drv_gpio_isr();
}
#endif

#if (RTE_GPADC)
void GPADC_IRQHandler(void)
{
    drv_gpadc_isr();
}
#endif

#if (RTE_I2C0)
void I2C0_IRQHandler(void)
{
    drv_i2c_isr(OM_I2C0);
}
#endif

#if (RTE_I2C1)
void I2C1_IRQHandler(void)
{
    drv_i2c_isr(OM_I2C1);
}
#endif

#if (RTE_FLASH0)
void SF0_IRQHandler(void)
{
    drv_iflash_isr(OM_SF0);
}
#endif

#if (RTE_SPI0)
void SPI0_IRQHandler(void)
{
    drv_spi_isr(OM_SPI0);
}
#endif

#if (RTE_SPI1)
void SPI1_IRQHandler(void)
{
    drv_spi_isr(OM_SPI1);
}
#endif

#if (RTE_TIM0)
void TIM0_IRQHandler(void)
{
    drv_tim_isr(OM_TIM0);
}
#endif

#if (RTE_TIM1)
void TIM1_IRQHandler(void)
{
    drv_tim_isr(OM_TIM1);
}
#endif

#if (RTE_TIM2)
void TIM2_IRQHandler(void)
{
    drv_tim_isr(OM_TIM2);
}
#endif

#if (RTE_RTC)
void RTC_SECOND_IRQHandler(void)
{
    drv_rtc_second_isr();
}

void RTC_ALARM_IRQHandler(void)
{
    drv_rtc_alarm_isr();
}
#endif

#if (RTE_LPTIM)
void LPTIM_IRQHandler(void)
{
    drv_lptim_isr(OM_LPTIM);
}
#endif

#if (RTE_I2S)
void I2S_IRQHandler(void)
{
    drv_i2s_isr();
}
#endif

#if (RTE_RGB)
void RGB_IRQHandler(void)
{
    drv_rgb_isr();
}
#endif

#if (RTE_IRTX)
void IRTX_IRQHandler(void)
{
    drv_irtx_isr();
}
#endif

#if (RTE_QDEC)
void QDEC_IRQHandler(void)
{
    drv_qdec_isr(OM_QDEC);
}
#endif

#if (RTE_OSPI1)
void OSPI1_IRQHandler(void)
{
    drv_ospi_isr(OM_OSPI1);
}
#endif

#if (RTE_LCD)
void LCD_IRQHandler(void)
{
    drv_lcd_isr();
}
#endif

#if (RTE_AUDIO)
void AUDIO_IRQHandler(void)
{
    drv_audio_isr();
}
#endif


#endif  /* (RTE_ISR) */

/** @} */
