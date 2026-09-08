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
 * @brief    Mesh app of firms model header file
 * @details  Mesh app of firms model header file

 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


#ifndef __OMSH_APP_FIRMS_MDL_H
#define __OMSH_APP_FIRMS_MDL_H

#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 * INCLUDES
 */
#include "omsh_app_firms_mdl_demo.h"
// TODO: Include more header file which you register models


/*******************************************************************************
 * DEFINES
 */
/// Align value multiple of 4
#define CO_ALIGN4_HI(val)                   (((val) + 3) & ~3)


#ifdef __cplusplus
}
#endif

#endif /* __OMSH_APP_FIRMS_MDL_H */

/** @} */