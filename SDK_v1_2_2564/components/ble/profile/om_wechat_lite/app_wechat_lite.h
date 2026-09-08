/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */
#ifndef __SERVICE_WECHAT_LITE_H__
#define __SERVICE_WECHAT_LITE_H__

#include <stdint.h>

/// Init wechat service
void app_wechat_lite_init(void);

/** @brief report measurement data to phone
	@param[in] new_measure 10 bytes: |Flag(1) | StepCount(3) | StepDistancer(3) | StepCalorie(3)|
*/
void wechat_lite_set_measurement(uint8_t* new_measure);
/** @brief report target setting to phone
	@param[in] new_target 4 Btypes: |Flag(1) | StepCount(3)|
*/
void wechat_lite_set_target(uint8_t* new_target);

#endif /* __APP_WECHAT_LITE_H__ */

/** @} */
