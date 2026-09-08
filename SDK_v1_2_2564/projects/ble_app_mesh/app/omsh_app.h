/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup MESH MESH
 * @ingroup  DOCUMENT
 * @brief
 * @details  Mesh app header file

 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __OMSH_APP_H
#define __OMSH_APP_H

#ifdef __cplusplus
extern "C"
{
#endif
/*******************************************************************************
 * INCLUDES
 */
#include "omsh_app_config.h"

// Use for mesh thread
#include "cmsis_os2.h"

// Use for evb board config
#include "bsp.h"

// Use for driver
#include "om_driver.h"

// Power manager
#include "pm.h"

// Shell and Log
#if CONFIG_SHELL
#include "shell.h"
#endif

#if CONFIG_LOG
#include "om_log.h"
#endif

// NVDS
#if CONFIG_NVDS
#include "nvds.h"
#endif

// Event timer
#include "evt_timer.h"

// BLE
#include "ob_config.h"
#include "omble.h"

// Mesh stack apis
#include "omsh_apis.h"

#if APP_MESH_FIRMS
#include "omsh_app_firms.h"
#elif APP_MESH_TMALL
#include "omsh_app_tmall.h"
#endif


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 * @brief  Start ble mesh task
 **/
void vStartBleMeshTask(void);


/**
 * @brief  Start ble mesh app
 **/
void msh_app_start(bool prov_state);


#ifdef __cplusplus
}
#endif

#endif /* __OMSH_APP_H */

/** @} */