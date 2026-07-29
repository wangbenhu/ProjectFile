/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup BSP BSP
 * @ingroup  DOCUMENT
 * @brief
 * @details  board driver

 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DOG_COLLAR_BOARD_DEFINE_H
#define __DOG_COLLAR_BOARD_DEFINE_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#include "om_device.h"


#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
 #define CONFIG_CHERRYUSB
#define LED_OFF_LEVEL       GPIO_LEVEL_HIGH
//low power control pad
#define PAD_CONTROL_LOW_POWER               31	//5

	/// Test tx pad for uart1
#define PAD_UART0_TXD               5	//5
	/// Test rx pad for uart1		
#define PAD_UART0_RXD               6	//6
	/// Test cts pad for uart1
//#define PAD_UART1_CTS               7
//	/// Test rts pad for uart1
//#define PAD_UART1_RTS               8
#define PAD_UART1_TXD       22	//22
#define PAD_UART1_RXD       21	//21

#define PAD_UART2_TXD       18
#define PAD_UART2_RXD       19

#define GPS_AT_UART                   OM_UART2
#define GPS_AT_UART_BAUDRATE              115200

	/// Test uart
#define CAT1_AT_UART                   OM_UART1
	/// Test uart baudrate
#define CAT1_AT_UART_BAUDRATE              115200

#define LOG_UART                     	OM_UART0
#define WIFI_AT_UART_BAUDRATE              115200



//motion, keep them, or else couldn't boot
#define PAD_MICP        2   ///< UART-RTS@EVB
#define PAD_MICN        3  ///< KEY2@EVB

/***************CAT1******************/
#define PAD_CAT1_RING       	27  //RING is low pulse to wakeup MCU
#define PAD_CAT1_RING_OFFSET 	(1<<PAD_CAT1_RING)
#define PAD_CAT1_DTR        	28 //DTR drive Low to wakeup CAT1
#define PAD_CAT1_DTR_OFFSET 	(1<<PAD_CAT1_DTR)

#define PAD_CAT1_POWERKEY     37
#define PAD_CAT1_POWER       23
#define PAD_CAT1_RESET       30

//#define PD_I2C0_SCL     4
//#define MUX_I2C0_SCL                    PINMUX_PAD4_I2C0_SCK_CFG
//#define PD_I2C0_SDA     13
//#define MUX_I2C0_SDA                    PINMUX_PAD13_I2C0_SDA_CFG

/// Test pad i2c scl
#define LED_SCL_IO                    24
/// Test pad mux i2c scl
#define LED_SCL_IO_MUX                    PINMUX_PAD24_I2C0_SCK_CFG
/// Test pad i2c sda
#define LED_SDA_IO                    25
/// Test pad mux i2c sda
#define LED_SDA_IO_MUX                    PINMUX_PAD25_I2C0_SDA_CFG


/***************GNSS******************/
#define PAD_GNSS_POWER     			36
#define PAD_GNSS_BACKUPPOWER        29
#define PAD_GNSS_REST        		13
#define PAD_GNSS_WAKEUP       		30

//charge check
#define PAD_CHAGE_CHECK       		15  ///< UART-RTS@EVB
//GPADC
#define PAD_GPADC_CH_GPIO14         14
#define MUX_GPADC_CH_GPIO14        	PINMUX_PAD14_INPUT_MODE_CFG
/// MOTOR gpio output
#define PAD_MOTOR_PAD_GPIO         	0

/// Test cs pad for ospi1
#define PAD_OSPI1_CS               8
/// Test ck pad for ospi1
#define PAD_OSPI1_CK               9
/// Test si pad for ospi1
#define PAD_OSPI1_SI               10
/// Test so pad for ospi1
#define PAD_OSPI1_SO               7
/// Test wp pad for ospi1
#define PAD_OSPI1_WP               12
/// Test hd pad for ospi1
#define PAD_OSPI1_HD               11

#ifdef CONFIG_CHERRYUSB
// Only PAD16/PAD17 can be used as USB
#define PAD_USBD_DM         16
#define PAD_USBD_DP         17
#endif

//sensor ICM42607-C
#define SPI_MISO						32
#define MUX_SPI_MISO				PINMUX_PAD32_SPI0_DI_CFG
#define SPI_MOSI						33
#define MUX_SPI_MOSI				PINMUX_PAD33_SPI0_DIO_CFG
#define SPI_SCK							35
#define MUX_SPI_SCK         PINMUX_PAD35_SPI0_SCK_CFG
#define SPI_CS							34
#define MUX_SPI_CS	        PINMUX_PAD34_SPI0_CS_CFG
#define IMU_INT1						26

#define AUDIO_SD_IO       			1 // 喇叭功放输出使能脚 低有效
#define AUDIO_PLAY_IO         		20//36 // audio PWM输出脚
#define AUDIO_PLAY_IO_MUX_CH0		PINMUX_PAD20_TIM2_OUT0_CFG//PINMUX_PAD36_TIM1_OUT0_CFG

#ifdef __cplusplus
}
#endif


#endif  /*__DOG_COLLAR_BOARD_DEFINE_H */


/** @} */
