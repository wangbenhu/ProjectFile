/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup RET_DRIVER RET_DRIVER
 * @ingroup  DRIVER
 * @brief    RTE driver configuration file
 * @details  RTE driver configuration file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __RTE_DRIVER_H
#define __RTE_DRIVER_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_om662x.h"


/*******************************************************************************
 * MACROS
 */
/*
 * Default RTE configuration
 * RTE_XXX_IRQ_PRIORITY, range in [0, (1 << __NVIC_PRIO_BITS)), Higher priorities
 * are lower numeric values, 0 is highest priority.
 * RTE_XXX_REGISTER_CALLBACK: enable register callbacks for driver，default enabled
 */
// UART0/1/2
#ifndef RTE_UART0_IRQ_PRIORITY
#define RTE_UART0_IRQ_PRIORITY                 0
#endif
#ifndef RTE_UART1_IRQ_PRIORITY
#define RTE_UART1_IRQ_PRIORITY                 0
#endif
#ifndef RTE_UART2_IRQ_PRIORITY
#define RTE_UART2_IRQ_PRIORITY                 0
#endif
#ifndef RTE_UART_REGISTER_CALLBACK
#define RTE_UART_REGISTER_CALLBACK             1
#endif
// I2C0/1
#ifndef RTE_I2C0_IRQ_PRIORITY
#define RTE_I2C0_IRQ_PRIORITY                  0
#endif
#ifndef RTE_I2C1_IRQ_PRIORITY
#define RTE_I2C1_IRQ_PRIORITY                  0
#endif
#ifndef RTE_I2C_REGISTER_CALLBACK
#define RTE_I2C_REGISTER_CALLBACK              1
#endif
// GPIO0
#ifndef RTE_GPIO0_IRQ_PRIORITY
#define RTE_GPIO0_IRQ_PRIORITY                 0
#endif
#ifndef RTE_GPIO1_IRQ_PRIORITY
#define RTE_GPIO1_IRQ_PRIORITY                 0
#endif
#ifndef RTE_GPIO_REGISTER_CALLBACK
#define RTE_GPIO_REGISTER_CALLBACK             1
#endif
// SPI0/1
#ifndef RTE_SPI0_IRQ_PRIORITY
#define RTE_SPI0_IRQ_PRIORITY                  0
#endif
#ifndef RTE_SPI1_IRQ_PRIORITY
#define RTE_SPI1_IRQ_PRIORITY                  0
#endif
#ifndef RTE_SPI_REGISTER_CALLBACK
#define RTE_SPI_REGISTER_CALLBACK              1
#endif
#ifndef RTE_SPI_CSN_MANUAL_CONTROL
#define RTE_SPI_CSN_MANUAL_CONTROL             0
#endif
// I2S
#ifndef RTE_I2S_IRQ_PRIORITY
#define RTE_I2S_IRQ_PRIORITY                   7
#endif
#ifndef RTE_I2S_REGISTER_CALLBACK
#define RTE_I2S_REGISTER_CALLBACK              1
#endif
// FLASH
#ifndef RTE_FLASH0_IRQ_PRIORITY
#define RTE_FLASH0_IRQ_PRIORITY                0
#endif
#ifndef RTE_FLASH0_REGISTER_CALLBACK
#define RTE_FLASH0_REGISTER_CALLBACK           1
#endif
#ifndef RTE_FLASH0_XIP
#define RTE_FLASH0_XIP                         1
#endif
#ifndef RTE_FLASH1_XIP
#define RTE_FLASH1_XIP                         0
#endif
// OSPI1
#ifndef RTE_OSPI1_IRQ_PRIORITY
#define RTE_OSPI1_IRQ_PRIORITY                 1
#endif
#ifndef RTE_OSPI1_REGISTER_CALLBACK
#define RTE_OSPI1_REGISTER_CALLBACK            1
#endif
// GPDMA
#ifndef RTE_GPDMA_IRQ_PRIORITY
#define RTE_GPDMA_IRQ_PRIORITY                 1
#endif
// SYSTICK
#ifndef RTE_SYSTICK_IRQ_PRIORITY
#define RTE_SYSTICK_IRQ_PRIORITY               7  /* ((1 << __NVIC_PRIO_BITS) - 1) */
#endif
#ifndef RTE_SYSTICK_REGISTER_CALLBACK
#define RTE_SYSTICK_REGISTER_CALLBACK          1
#endif
// TIM0/1/2
#ifndef RTE_TIM0_IRQ_PRIORITY
#define RTE_TIM0_IRQ_PRIORITY                  0
#endif
#ifndef RTE_TIM1_IRQ_PRIORITY
#define RTE_TIM1_IRQ_PRIORITY                  0
#endif
#ifndef RTE_TIM2_IRQ_PRIORITY
#define RTE_TIM2_IRQ_PRIORITY                  0
#endif
#ifndef RTE_TIM_REGISTER_CALLBACK
#define RTE_TIM_REGISTER_CALLBACK              1
#endif
// LPTIM
#ifndef RTE_LPTIM_IRQ_PRIORITY
#define RTE_LPTIM_IRQ_PRIORITY                 7
#endif
#ifndef RTE_LPTIM_REGISTER_CALLBACK
#define RTE_LPTIM_REGISTER_CALLBACK            1
#endif
// PMU PIN_WAKEUP
#ifndef RTE_PMU_PIN_WAKEUP_IRQ_PRIORITY
#define RTE_PMU_PIN_WAKEUP_IRQ_PRIORITY        4
#endif
#ifndef RTE_PMU_PIN_WAKEUP_REGISTER_CALLBACK
#define RTE_PMU_PIN_WAKEUP_REGISTER_CALLBACK   1
#endif
// PMU POF
#ifndef RTE_PMU_POF_IRQ_PRIORITY
#define RTE_PMU_POF_IRQ_PRIORITY               4
#endif
#ifndef RTE_PMU_POF_REGISTER_CALLBACK
#define RTE_PMU_POF_REGISTER_CALLBACK          1
#endif
// PMU_TIMER
#ifndef RTE_PMU_TIMER_IRQ_PRIORITY
#define RTE_PMU_TIMER_IRQ_PRIORITY             7
#endif
#ifndef RTE_PMU_TIMER_REGISTER_CALLBACK
#define RTE_PMU_TIMER_REGISTER_CALLBACK        1
#endif
// RTC
#ifndef RTE_RTC_SECOND_IRQ_PRIORITY
#define RTE_RTC_SECOND_IRQ_PRIORITY            7
#endif
#ifndef RTE_RTC_ALARM_IRQ_PRIORITY
#define RTE_RTC_ALARM_IRQ_PRIORITY             7
#endif
#ifndef RTE_RTC_REGISTER_CALLBACK
#define RTE_RTC_REGISTER_CALLBACK              1
#endif
// WDT
#ifndef RTE_WDT_IRQ_PRIORITY
#define RTE_WDT_IRQ_PRIORITY                   0
#endif
#ifndef RTE_WDT_REGISTER_CALLBACK
#define RTE_WDT_REGISTER_CALLBACK              1
#endif
// QDEC
#ifndef RTE_QDEC_IRQ_PRIORITY
#define RTE_QDEC_IRQ_PRIORITY                  0
#endif
#ifndef RTE_QDEC_REGISTER_CALLBACK
#define RTE_QDEC_REGISTER_CALLBACK             1
#endif
// RGB
#ifndef RTE_RGB_IRQ_PRIORITY
#define RTE_RGB_IRQ_PRIORITY                   7
#endif
#ifndef RTE_RGB_REGISTER_CALLBACK
#define RTE_RGB_REGISTER_CALLBACK              1
#endif
// GPADC
#ifndef RTE_GPADC_IRQ_PRIORITY
#define RTE_GPADC_IRQ_PRIORITY                 0
#endif
#ifndef RTE_GPADC_REGISTER_CALLBACK
#define RTE_GPADC_REGISTER_CALLBACK            1
#endif
// IRTX
#ifndef RTE_IRTX_IRQ_PRIORITY
#define RTE_IRTX_IRQ_PRIORITY                  0
#endif
#ifndef RTE_IRTX_REGISTER_CALLBACK
#define RTE_IRTX_REGISTER_CALLBACK             1
#endif
// OM24G
#ifndef RTE_OM24G_IRQ_PRIORITY
#define RTE_OM24G_IRQ_PRIORITY                 0
#endif
#ifndef RTE_OM24G_REGISTER_CALLBACK
#define RTE_OM24G_REGISTER_CALLBACK            1
#endif
// LCD
#ifndef RTE_LCD_IRQ_PRIORITY
#define RTE_LCD_IRQ_PRIORITY                   0
#endif
#ifndef RTE_LCD_REGISTER_CALLBACK
#define RTE_LCD_REGISTER_CALLBACK              1
#endif
// AUDIO
#ifndef RTE_AUDIO_IRQ_PRIORITY
#define RTE_AUDIO_IRQ_PRIORITY                 7
#endif
#ifndef RTE_AUDIO_REGISTER_CALLBACK
#define RTE_AUDIO_REGISTER_CALLBACK            1
#endif


#endif  /* __RTE_DRIVER_H */


/** @} */
