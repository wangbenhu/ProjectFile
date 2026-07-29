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

#ifndef __BOARD_EVB_H
#define __BOARD_EVB_H


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
#define LED_OFF_LEVEL       GPIO_LEVEL_HIGH

#define PAD_UART0_TXD       16
#define PAD_UART0_RXD       17
#define PAD_UART1_TXD       5
#define PAD_UART1_RXD       6

#define PAD_LED_0           8
#define PAD_LED_1           9
#define PAD_LED_2           10
#define PAD_LED_3           11
#define PAD_LED_4           11
#define PAD_LED_5           11

//#define PAD_RF_RXEN         13
//#define PAD_RF_TXEN         12

//#define PAD_DBGBUS_0        {5, 6, 7, 9, 11, 12, 13, 14}
//#define PAD_DBGBUS_1        {15, 16, 17, 18, 19, 20, 21, 22}

#define PAD_BUTTON_0        2   ///< UART-RTS@EVB
#define PAD_BUTTON_1        3  ///< KEY2@EVB

#ifdef CONFIG_CHERRYUSB
// Only PAD16/PAD17 can be used as USB
#define PAD_USBD_DM         16
#define PAD_USBD_DP         17
#endif

#ifdef __cplusplus
}
#endif


#endif  /*__BOARD_EVB_H */


/** @} */
