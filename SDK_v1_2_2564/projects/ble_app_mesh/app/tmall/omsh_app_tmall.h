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
 * @brief    Mesh app of tmall genie header file
 * @details  Mesh app of tmall genie header file

 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __OMSH_APP_TMALL_H
#define __OMSH_APP_TMALL_H

#ifdef __cplusplus
extern "C"
{
#endif
/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 ****************************************************************************************
 * @brief Mesh tmall genie app init
 ****************************************************************************************
 */
void msh_app_tmall_init(void);

/**
 ****************************************************************************************
 * @brief Mesh tmall genie app start
 *
 * @param[in] prov_state            1: proved, 0:unprov
 ****************************************************************************************
 */
void msh_app_tmall_start(uint8_t prov_state);


#ifdef __cplusplus
}
#endif

#endif /* __OMSH_APP_TMALL_H */