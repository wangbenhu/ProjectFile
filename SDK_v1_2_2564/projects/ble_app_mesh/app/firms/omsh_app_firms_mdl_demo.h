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
 * @brief    Mesh app of firms model demo header file
 * @details  Mesh app of firms model demo header file

 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __OMSH_APP_FIRMS_MDL_DEMO_H
#define __OMSH_APP_FIRMS_MDL_DEMO_H

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * INCLUDES
 */
#include "omsh_apis.h"


/*******************************************************************************
 * FUNCTIONS
 */
/**
 ****************************************************************************************
 * @brief Initialization of Mesh Firms Model Demo Programming Interface
 *
 * @param[in] reset     True means SW reset, False means task initialization
 * @param[in] p_env     Pointer to the environment structure
 * @param[in] p_cfg     Pointer to Mesh Model Configuration Structure provided by the Application
 *
 * @return Size of environment required for this module
 ****************************************************************************************
 */
uint16_t mm_firms_demo_init(bool reset, void *p_env, void *p_cfg);

/**
 ****************************************************************************************
 * @brief Return memory size needed for environment allocation of Mesh Model Message Application
 * Program Interface
 *
 * @param[in] p_cfg     Pointer to Mesh Model Configuration Structure provided by the Application
 *
 * @return Size of environment required for this module
 ****************************************************************************************
 */
uint16_t mm_firms_demo_get_env_size(void *p_cfg);


#ifdef __cplusplus
}
#endif

#endif /* __OMSH_APP_FIRMS_MDL_DEMO_H */

/** @} */
