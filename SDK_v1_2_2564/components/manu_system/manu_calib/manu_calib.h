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
 * @brief    manual calibration
 * @details  manual calibration
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __MANU_CALIB_H__
#define __MANU_CALIB_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <math.h>

#include "om_driver.h"
#include "nvds.h"
#include "evt.h"
#include "om_log.h"



void manu_calib_init(void);

#ifdef __cplusplus
}
#endif



#endif    /* __MANU_CALIB_H__ */

/** @} */
