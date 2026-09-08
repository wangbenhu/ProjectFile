/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @file     obc.h
 * @brief    obc
 * @date     15 December 2021
 * @author   OnMicro SW Team
 *
 * @defgroup obc obc
 * @ingroup  OBC
 * @brief    obc Driver
 * @details  obc Driver

 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __OBC_H__
#define __OBC_H__

#ifdef __cplusplus
extern "C"
{ /*}*/
#endif

/*********************************************************************
 * INCLUDES
 */
#include "obc_hci_h4.h"
#include "obc_pta.h"


/*********************************************************************
 * MACROS
 */


/*********************************************************************
 * TYPEDEFS
 */


/*********************************************************************
 * EXTERN VARIABLES
 */


/*********************************************************************
 * EXTERN FUNCTIONS
 */

/**
 *******************************************************************************
 * @brief  obc init
 *******************************************************************************
 */
void obc_init(void);

/**
 *******************************************************************************
 * @brief  obc isr
 *******************************************************************************
 */
void obc_isr(void);

/**
 *******************************************************************************
 * @brief  obc bb frame ongoing callback register
 *
 * @param[in] cb  cb
 * @param[in] is_ongoing  is ongoing
 *******************************************************************************
 */
void obc_bb_frame_ongoing_callback_register(void (*cb)(bool is_ongoing));

/**
 *******************************************************************************
 * @brief  obc bb advertising filter callback register.
 *         The filter use 6 bytes adva and adi, if return false,
 *         the extended advertising scan would be abort
 *
 * @param[in] cb  cb
 *******************************************************************************
 */
void obc_bb_adv_filter_callback_register(bool (*cb)(uint8_t *adva, uint16_t adi));

/**
 *******************************************************************************
 * @brief  obc bb advertising DID callback register.
 *         Use advertising data to get DID
 *
 * @param[in] cb  cb
 * @return 12bits DID
 *******************************************************************************
 */
void obc_bb_adv_did_callback_register(uint16_t (*cb)(uint8_t *data, uint16_t len));


#ifdef __cplusplus
/*{*/ }
#endif

#endif

/** @} */

