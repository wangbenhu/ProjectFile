/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @version
 * Version V20210119.1.0
 *  - Initial release
 *
 * @{
 */
 
#ifndef __SERVICE_OM_DFU_H__
#define __SERVICE_OM_DFU_H__

/*******************************************************************************
 * INCLUDES
 */
#include "om_dfu.h"

/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Init om_dfu service
 *******************************************************************************
 */
void app_om_dfu_init(void);

/**
 *******************************************************************************
 * @brief Callback for indicating dfu start
 * @param[in] status       Status
 * @param[in] p            Pointer of data
 *******************************************************************************
 */
extern void app_om_dfu_update_start_ind_handler(uint8_t status, void *p);
/**
 *******************************************************************************
 * @brief Callback for indicating dfu in progress
 * @param[in] status       Status
 * @param[in] p            Pointer of data
 *******************************************************************************
 */
extern void app_om_dfu_update_prog_ind_handler(uint8_t status, void *p);
/**
 *******************************************************************************
 * @brief Callback for indicating dfu end
 * @param[in] status       Status
 * @param[in] p            Pointer of data
 *******************************************************************************
 */
extern void app_om_dfu_update_end_ind_handler(uint8_t status, void *p);
/**
 *******************************************************************************
 * @brief callback for Image information verification 
 * @param[in] img_type       Type of image
 * @param[in] img_size       Size of image
 * @param[in] img_version    Version of image
 * @return error code, DFU_SUCCESS for success
 *******************************************************************************
 */
extern uint8_t app_om_dfu_user_check_ind_handler(uint8_t img_type, uint32_t img_size, uint32_t img_version);

#endif /* __SERVICE_OM_DFU_H__ */

/** @} */
