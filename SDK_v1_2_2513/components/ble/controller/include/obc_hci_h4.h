/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @file     obc_hci_h4.h
 * @brief    obc_hci_h4
 * @date     15 December 2021
 * @author   OnMicro SW Team
 *
 * @defgroup obc_hci_h4 obc_hci_h4
 * @ingroup  OBC
 * @brief    obc_hci_h4 Driver
 * @details  obc_hci_h4 Driver

 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef H4TL_H_
#define H4TL_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>       // standard integer definition
#include <stdbool.h>      // standard boolean definition

/*
 * DEFINES
 ****************************************************************************************
 */

/**
 *******************************************************************************
 * @brief  obc hci h4 transmit callback  type
 *
 * @param[in] pdata  pdata
 * @param[in] length  length
 *******************************************************************************
 */
typedef void (*obc_hci_h4_transmit_callback_t)(const uint8_t *pdata, uint32_t length);

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 *******************************************************************************
 * @brief  obc hci h4 receive handler
 *
 * @param[in] pdata  pdata
 * @param[in] length  length
 *******************************************************************************
 */
void obc_hci_h4_receive_handler(const uint8_t *pdata, uint32_t length);

/**
 *******************************************************************************
 * @brief  obc hci h4 transmit callback register
 *
 * @param[in] cb  cb
 *******************************************************************************
 */
void obc_hci_h4_transmit_callback_set(obc_hci_h4_transmit_callback_t cb);

#endif
/// @} H4TL
