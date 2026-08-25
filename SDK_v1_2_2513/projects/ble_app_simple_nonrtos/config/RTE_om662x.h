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
 * @brief    RTE driver
 * @details  All the RTE configuration file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

//-------- <<< Use Configuration Wizard in Context Menu >>> --------------------

#ifndef __RTE_OM662X_H
#define __RTE_OM662X_H

// <e> RTE_SYSTICK
//   <i> Enable or disable the system tick timer (SysTick) configuration.
#define RTE_SYSTICK                                     0
//   <o.0..2> RTE_SYSTICK_IRQ_PRIORITY
//     <i> Set the interrupt priority for SysTick (0 = highest, 7 = lowest).
//     <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_SYSTICK_IRQ_PRIORITY                        7
//   <q> RTE_SYSTICK_REGISTER_CALLBACK
//     <i> Enable callback registration for SysTick.
#define RTE_SYSTICK_REGISTER_CALLBACK                   0
// </e>

// <e> RTE_RCC
//   <i> Enable or disable the Reset and Clock Control (RCC) configuration.
#define RTE_RCC                                         1
// </e>

// <e> RTE_PIN
//   <i> Enable or disable the PIN configuration.
#define RTE_PIN                                         1
// </e>

// <e> RTE_ISR
//   <i> Enable or disable the Interrupt Service Routine (ISR) configuration.
#define RTE_ISR                                         1
// </e>

// <e> RTE_ICACHE
//   <i> Enable or disable the instruction cache (ICACHE) configuration.
#define RTE_ICACHE                                      1
// </e>

// <e> RTE_CALIB
//   <i> Enable or disable the calibration module configuration.
#define RTE_CALIB                                       1
//   <q> RTE_CALIB_USE_FT_KDCO_DATA
//     <i> Use KDCO calibration data in FT region to accelerate KDCO calibration.
#define RTE_CALIB_USE_FT_KDCO_DATA                      0
//   <q> RTE_CALIB_USE_FT_DF1_DATA
//     <i> Use DF1 data in FT region to compensate DF1 parameter.
#define RTE_CALIB_USE_FT_DF1_DATA                       0
// </e>

// <e> RTE_GPDMA
//   <i> Enable or disable the General Purpose DMA (GPDMA) configuration.
#define RTE_GPDMA                                       1
//   <o.0..2> RTE_GPDMA_IRQ_PRIORITY
//     <i> Set the interrupt priority for GPDMA (0 = highest, 7 = lowest).
//     <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_GPDMA_IRQ_PRIORITY                          3
// </e>

// <h> RTE_GPIO
//   <i> GPIO configuration options.
//   <q> RTE_GPIO0
//     <i> Enable GPIO0.
#define RTE_GPIO0                                       1
//   <q> RTE_GPIO1
//     <i> Enable GPIO1.
#define RTE_GPIO1                                       1
//   <o.0..2> RTE_GPIO_IRQ_PRIORITY
//     <i> Set the interrupt priority for GPIO (0 = highest, 7 = lowest).
//     <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_GPIO_IRQ_PRIORITY                           7
//   <q> RTE_GPIO_REGISTER_CALLBACK
//     <i> Enable callback registration for GPIO.
#define RTE_GPIO_REGISTER_CALLBACK                      1
// </h>

// <h> RTE_UART
//   <i> UART configuration options.
//   <e> RTE_UART0
//     <i> Enable UART0.
#define RTE_UART0                                       0
//     <o.0..2> RTE_UART0_IRQ_PRIORITY
//       <i> Set the interrupt priority for UART0 (0 = highest, 7 = lowest).
//       <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_UART0_IRQ_PRIORITY                          3
//   </e>
//   <e> RTE_UART1
//     <i> Enable UART1.
#define RTE_UART1                                       1
//     <o.0..2> RTE_UART1_IRQ_PRIORITY
//       <i> Set the interrupt priority for UART1 (0 = highest, 7 = lowest).
//       <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_UART1_IRQ_PRIORITY                          3
//   </e>
//   <e> RTE_UART2
//     <i> Enable UART2.
#define RTE_UART2                                       0
//     <o.0..2> RTE_UART2_IRQ_PRIORITY
//       <i> Set the interrupt priority for UART2 (0 = highest, 7 = lowest).
//       <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_UART2_IRQ_PRIORITY                          3
//   </e>
//   <q> RTE_UART_REGISTER_CALLBACK
//     <i> Enable callback registration for UART.
#define RTE_UART_REGISTER_CALLBACK                      1
// </h>

// <h> RTE_SPI
//   <i> SPI configuration options.
//   <e> RTE_SPI0
//     <i> Enable SPI0.
#define RTE_SPI0                                        0
//     <o.0..2> RTE_SPI0_IRQ_PRIORITY
//       <i> Set the interrupt priority for SPI0 (0 = highest, 7 = lowest).
//       <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_SPI0_IRQ_PRIORITY                           3
//   </e>
//   <e> RTE_SPI1
//     <i> Enable SPI1.
#define RTE_SPI1                                        0
//     <o.0..2> RTE_SPI1_IRQ_PRIORITY
//       <i> Set the interrupt priority for SPI1 (0 = highest, 7 = lowest).
//       <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_SPI1_IRQ_PRIORITY                           3
//   </e>
//   <q> RTE_SPI_REGISTER_CALLBACK
//     <i> Enable callback registration for SPI.
#define RTE_SPI_REGISTER_CALLBACK                       1
//   <q> RTE_SPI_CSN_MANUAL_CONTROL
//     <i> Enable manual control for SPI CSN.
#define RTE_SPI_CSN_MANUAL_CONTROL                      0
// </h>

// <h> RTE_TIM
//   <i> Timer configuration options.
//   <e> RTE_TIM0
//     <i> Enable TIM0.
#define RTE_TIM0                                        0
//     <o.0..2> RTE_TIM0_IRQ_PRIORITY
//       <i> Set the interrupt priority for TIM0 (0 = highest, 7 = lowest).
//       <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_TIM0_IRQ_PRIORITY                           3
//   </e>
//   <e> RTE_TIM1
//     <i> Enable TIM1.
#define RTE_TIM1                                        0
//     <o.0..2> RTE_TIM1_IRQ_PRIORITY
//       <i> Set the interrupt priority for TIM1 (0 = highest, 7 = lowest).
//       <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_TIM1_IRQ_PRIORITY                           3
//   </e>
//   <e> RTE_TIM2
//     <i> Enable TIM2.
#define RTE_TIM2                                        0
//     <o.0..2> RTE_TIM2_IRQ_PRIORITY
//       <i> Set the interrupt priority for TIM2 (0 = highest, 7 = lowest).
//       <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_TIM2_IRQ_PRIORITY                           3
//   </e>
//   <q> RTE_TIM_REGISTER_CALLBACK
//     <i> Enable callback registration for TIM.
#define RTE_TIM_REGISTER_CALLBACK                       1
// </h>

// <e> RTE_LPTIM
//   <i> Enable LPTIM.
#define RTE_LPTIM                                       0
//   <o.0..2> RTE_LPTIM_IRQ_PRIORITY
//     <i> Set the interrupt priority for LPTIM (0 = highest, 7 = lowest).
//     <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_LPTIM_IRQ_PRIORITY                          7
//   <q> RTE_LPTIM_REGISTER_CALLBACK
//     <i> Enable callback registration for LPTIM.
#define RTE_LPTIM_REGISTER_CALLBACK                     1
// </e>

// <e> RTE_WDT
//   <i> Enable WDT.
#define RTE_WDT                                         1
//   <o.0..2> RTE_WDT_IRQ_PRIORITY
//     <i> Set the interrupt priority for WDT (0 = highest, 7 = lowest).
//     <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_WDT_IRQ_PRIORITY                            0
//   <q> RTE_WDT_REGISTER_CALLBACK
//     <i> Enable callback registration for WDT.
#define RTE_WDT_REGISTER_CALLBACK                       1
// </e>

// <e> RTE_RNG
//   <i> Enable RNG.
#define RTE_RNG                                         0
// </e>

// <e> RTE_PMU
//   <i> Enable PMU.
#define RTE_PMU                                         1
//   <o.0..2> RTE_PMU_PIN_WAKEUP_IRQ_PRIORITY
//     <i> Set the interrupt priority for PMU pin wakeup (0 = highest, 7 = lowest).
//     <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_PMU_PIN_WAKEUP_IRQ_PRIORITY                 4
//   <q> RTE_PMU_PIN_WAKEUP_REGISTER_CALLBACK
//     <i> Enable callback registration for PMU pin wakeup.
#define RTE_PMU_PIN_WAKEUP_REGISTER_CALLBACK            1
//   <o.0..2> RTE_PMU_POF_IRQ_PRIORITY
//     <i> Set the interrupt priority for PMU power-on failure (POF) (0 = highest, 7 = lowest).
//     <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_PMU_POF_IRQ_PRIORITY                        4
//   <q> RTE_PMU_POF_REGISTER_CALLBACK
//     <i> Enable callback registration for PMU power-on failure (POF).
#define RTE_PMU_POF_REGISTER_CALLBACK                   1
// </e>

// <e> RTE_PMU_TIMER
//   <i> Enable PMU Timer.
#define RTE_PMU_TIMER                                   1
//   <o.0..2> RTE_PMU_TIMER_IRQ_PRIORITY
//     <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_PMU_TIMER_IRQ_PRIORITY                      15
//   <q> RTE_PMU_TIMER_REGISTER_CALLBACK
#define RTE_PMU_TIMER_REGISTER_CALLBACK                 1
// </e>

// <e> RTE_RTC
//   <i> Enable RTC.
#define RTE_RTC                                         1
//   <o.0..2> RTE_RTC_SECOND_IRQ_PRIORITY
//     <i> Set the interrupt priority for RTC second event (0 = highest, 7 = lowest).
//     <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_RTC_SECOND_IRQ_PRIORITY                     7
//   <o.0..2> RTE_RTC_ALARM_IRQ_PRIORITY
//     <i> Set the interrupt priority for RTC alarm event (0 = highest, 7 = lowest).
//     <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_RTC_ALARM_IRQ_PRIORITY                      7
//   <q> RTE_RTC_REGISTER_CALLBACK
//     <i> Enable callback registration for RTC.
#define RTE_RTC_REGISTER_CALLBACK                       1
// </e>

// <h> RTE_FLASH
//   <i> Flash memory configuration options.
//   <e> RTE_FLASH0
//     <i> Enable Flash0.
#define RTE_FLASH0                                      1
//     <o.0..2> RTE_FLASH0_IRQ_PRIORITY
//       <i> Set the interrupt priority for Flash0 (0 = highest, 7 = lowest).
//       <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_FLASH0_IRQ_PRIORITY                         3
//   </e>
//   <q> RTE_FLASH0_REGISTER_CALLBACK
//     <i> Enable callback registration for Flash0.
#define RTE_FLASH0_REGISTER_CALLBACK                    1

//   <q> RTE_FLASH0_XIP
#define RTE_FLASH0_XIP                                  (1 && RTE_FLASH0)
// </h>

// <h> RTE_OSPI
//   <i> OSPI Controller configuration options.
//   <e> RTE_OSPI1
//     <i> Enable or disable OSPI1 configuration.
#define RTE_OSPI1                                       0
//     <o.0..2> RTE_OSPI1_IRQ_PRIORITY
//       <i> Set the interrupt priority for OSPI1 (0 = highest, 7 = lowest).
//       <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_OSPI1_IRQ_PRIORITY                          3
//     <q> RTE_OSPI1_REGISTER_CALLBACK
//       <i> Enable callback registration for OSPI1.
#define RTE_OSPI1_REGISTER_CALLBACK                     1
//   </e>

//   <e> RTE_FLASH1
//     <i> Enable or disable Flash1 configuration.
#define RTE_FLASH1                                      (1 && RTE_OSPI1)
//     <q> RTE_FLASH1_XIP
//       <i> Enable Flash1 Execute-In-Place (XIP) mode.
#define RTE_FLASH1_XIP                                  (0 && RTE_FLASH1)
//     <q> RTE_FLASH1_REGISTER_CALLBACK
//       <i> Enable callback registration for Flash1.
#define RTE_FLASH1_REGISTER_CALLBACK                    (1 && RTE_OSPI1_REGISTER_CALLBACK)
//   </e>

//   <e> RTE_PSRAM
//     <i> Enable or disable PSRAM configuration.
#define RTE_PSRAM                                       (0 && RTE_OSPI1)
//     <q> RTE_PSRAM_REGISTER_CALLBACK
//       <i> Enable callback registration for PSRAM.
#define RTE_PSRAM_REGISTER_CALLBACK                     (1 && RTE_OSPI1_REGISTER_CALLBACK)
//   </e>
// </h>

// <e> RTE_RADIO
//   <i> Enable RADIO Control.
#define RTE_RADIO                                       1
//   <o.0..1> RTE_RADIO_MODE
//     <i> Set the RADIO mode configuration.
//     <0=>Default Mode   <1=>High Performance Mode   <2=>Low Power Mode
#define RTE_RADIO_MODE                                  0
// </e>

// <e> RTE_USB
//   <i> Enable USB.
#define RTE_USB                                         0
//   <o.0..2> RTE_USB_IRQ_PRIORITY
//     <i> Set the interrupt priority for USB (0 = highest, 7 = lowest).
//     <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_USB_IRQ_PRIORITY                            8
//   <o.0..2> RTE_USB_WK_IRQ_PRIORITY
//     <i> Set the interrupt priority for USB Wakeup (0 = highest, 7 = lowest).
//     <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_USB_WK_IRQ_PRIORITY                         9
//   <q> RTE_USB_REGISTER_CALLBACK
//     <i> Enable callback registration for USB.
#define RTE_USB_REGISTER_CALLBACK                       0
// </e>

// <e> RTE_EFUSE
#define RTE_EFUSE                                       1
//   <o.0..2> RTE_EFUSE_IRQ_PRIORITY
//     <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_EFUSE_IRQ_PRIORITY                          1
//   <q> RTE_EFUSE_REGISTER_CALLBACK
#define RTE_EFUSE_REGISTER_CALLBACK                     1
// </e>

// <e> RTE_SHA256
#define RTE_SHA256                                      0
//   <q> RTE_SHA256_USING_BIG_ENDIAN
#define RTE_SHA256_USING_BIG_ENDIAN                     1
// </e>

// <e> RTE_ECDSA
#define RTE_ECDSA                                       1
// </e>

// <e> RTE_AES
//   <i> Enable AES.
#define RTE_AES                                         1
//   <i> Just keep AES Engine to reduce ram usage.
#define RTE_AES_KEEP_RAW_ENGINE                         1
// </e>

// <h> RTE_I2C
//   <i> I2C configuration options.
//   <e> RTE_I2C0
//     <i> Enable I2C0.
#define RTE_I2C0                                        0
//     <o.0..2> RTE_I2C0_IRQ_PRIORITY
//       <i> Set the interrupt priority for I2C0 (0 = highest, 7 = lowest).
//       <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_I2C0_IRQ_PRIORITY                           3
//   </e>
//   <e> RTE_I2C1
//     <i> Enable I2C1.
#define RTE_I2C1                                        0
//     <o.0..2> RTE_I2C1_IRQ_PRIORITY
//       <i> Set the interrupt priority for I2C1 (0 = highest, 7 = lowest).
//       <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_I2C1_IRQ_PRIORITY                           3
//   </e>
//   <q> RTE_I2C_REGISTER_CALLBACK
//     <i> Enable callback registration for I2C.
#define RTE_I2C_REGISTER_CALLBACK                       1
// </h>

// <e> RTE_I2S
//   <i> Enable I2S.
#define RTE_I2S                                         0
//   <o> RTE_I2S_MODE
//     <i> Set the I2S mode.
//     <0=>I2S   <1=>Left align   <2=>Right align   <3=>DSP_A   <7=>DSP_B
#define RTE_I2S_MODE                                    0
//   <o.0..2> RTE_I2S_IRQ_PRIORITY
//     <i> Set the interrupt priority for I2S (0 = highest, 7 = lowest).
//     <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_I2S_IRQ_PRIORITY                            7
//   <q> RTE_I2S_REGISTER_CALLBACK
//     <i> Enable callback registration for I2S.
#define RTE_I2S_REGISTER_CALLBACK                       1
//   <o> RTE_I2S_GPDMA_LLP_CHAIN_NUM
//     <i> Set the GPDMA LLP chain number for I2S.
#define RTE_I2S_GPDMA_LLP_CHAIN_NUM                     2
//   <q> RTE_I2S_USE_EXTERNAL_MCLK
//     <i> Enable external MCLK usage for I2S.
#define RTE_I2S_USE_EXTERNAL_MCLK                       (0 && RTE_AUDIO_USE_EXTERNAL)
// </e>

// <e> RTE_AUDIO
//   <i> Enable Audio.
#define RTE_AUDIO                                       0
//   <o.0..2> RTE_AUDIO_IRQ_PRIORITY
//     <i> Set the interrupt priority for Audio (0 = highest, 7 = lowest).
//     <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_AUDIO_IRQ_PRIORITY                          15
//   <q> RTE_AUDIO_USE_INTERNAL
//     <i> Enable internal Audio usage.
#define RTE_AUDIO_USE_INTERNAL                          (0 && RTE_AUDIO)
//   <q> RTE_AUDIO_USE_EXTERNAL
//     <i> Enable external Audio usage.
#define RTE_AUDIO_USE_EXTERNAL                          (RTE_AUDIO && (!RTE_AUDIO_USE_INTERNAL))
//   <q> RTE_AUDIO_USE_ANA_MIC
//     <i> Enable analog microphone for Audio.
#define RTE_AUDIO_USE_ANA_MIC                           0
// </e>

// <e> RTE_QDEC
//   <i> Enable QDEC.
#define RTE_QDEC                                        0
//   <o.0..2> RTE_QDEC_IRQ_PRIORITY
//     <i> Set the interrupt priority for QDEC (0 = highest, 7 = lowest).
//     <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_QDEC_IRQ_PRIORITY                           3
//   <q> RTE_QDEC_REGISTER_CALLBACK
//     <i> Enable callback registration for QDEC.
#define RTE_QDEC_REGISTER_CALLBACK                      1
// </e>

// <e> RTE_RGB
//   <i> Enable RGB.
#define RTE_RGB                                         0
//   <o.0..2> RTE_RGB_IRQ_PRIORITY
//     <i> Set the interrupt priority for RGB (0 = highest, 7 = lowest).
//     <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_RGB_IRQ_PRIORITY                            6
//   <q> RTE_RGB_REGISTER_CALLBACK
//     <i> Enable callback registration for RGB.
#define RTE_RGB_REGISTER_CALLBACK                       1
// </e>

// <e> RTE_GPADC
//   <i> Enable GPADC.
#define RTE_GPADC                                       1
//   <o.0..2> RTE_GPADC_IRQ_PRIORITY
//     <i> Set the interrupt priority for GPADC (0 = highest, 7 = lowest).
//     <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_GPADC_IRQ_PRIORITY                          6
//   <q> RTE_GPADC_REGISTER_CALLBACK
//     <i> Enable callback registration for GPADC.
#define RTE_GPADC_REGISTER_CALLBACK                     1
// </e>

// <e> RTE_IRTX
//   <i> Enable IRTX.
#define RTE_IRTX                                        0
//   <o.0..2> RTE_IRTX_IRQ_PRIORITY
//     <i> Set the interrupt priority for IRTX (0 = highest, 7 = lowest).
//     <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_IRTX_IRQ_PRIORITY                           6
//   <q> RTE_IRTX_REGISTER_CALLBACK
//     <i> Enable callback registration for IRTX.
#define RTE_IRTX_REGISTER_CALLBACK                      1
// </e>

// <e> RTE_OM24G
//   <i> Enable OM24G.
#define RTE_OM24G                                       0
//   <o.0..2> RTE_OM24G_IRQ_PRIORITY
//     <i> Set the interrupt priority for OM24G (0 = highest, 7 = lowest).
//     <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_OM24G_IRQ_PRIORITY                          2
//   <q> RTE_OM24G_REGISTER_CALLBACK
//     <i> Enable callback registration for OM24G.
#define RTE_OM24G_REGISTER_CALLBACK                     1
//   <o.0..2> RTE_OM24G_RF_MODE
//     <i> Set the RF mode for OM25G.
//     <0=>Compatible with om66XX   <1=>Compatible with TI2640   <2=>Compatible with SILICONLAB   <3=>Compatible with NORDIC
#define RTE_OM24G_RF_MODE                               0
// </e>

// <e> RTE_LCD
//   <i> Enable LCD.
#define RTE_LCD                                         0
//   <o.0..2> RTE_LCD_IRQ_PRIORITY
//     <i> Set the interrupt priority for LCD (0 = highest, 7 = lowest).
//     <0=>0   <1=>1   <2=>2   <3=>3   <4=>4   <5=>5   <6=>6   <7=>7
#define RTE_LCD_IRQ_PRIORITY                            0
//   <q> RTE_LCD_REGISTER_CALLBACK
//     <i> Enable callback registration for LCD.
#define RTE_LCD_REGISTER_CALLBACK                       1
// </e>


#endif  /* __RTE_OM662X_H */


/** @} */
