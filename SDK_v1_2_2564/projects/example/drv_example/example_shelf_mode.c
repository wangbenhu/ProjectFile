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
 * @brief    example and precautions for using shelf mode
 * @details  example and precautions for using shelf mode
 * @version
 *
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


/*******************************************************************************
 * INCLUDES
 */
#include "om_driver.h"


/*******************************************************************************
 * MACROS
 */


/*******************************************************************************
 * TYPEDEFS
 */


/*******************************************************************************
 * CONST & VARIABLES
 */


/*******************************************************************************
 * LOCAL FUNCTIONS
 */


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief example of using shelf mode
 *
 * @note 1. The shipping mode is enabled by calling the interface drv_pmu_shelf_mode_enable(true);.
 * @note    GPIO10 is the fixed IO for the shipping mode.
 * @note    Pulling this IO low will cause the chip to enter the shutdown mode, with most circuit
 * @note    modules turned off. At this point, the current is the lowest.
 * @note    Pulling this IO high will reset the chip and put it in a normal working state.
 * @note    When the chip is not in the shipping mode enabled state, the chip always remains in a
 * @note    normal working state, and GPIO10 does not function and will not enter the shutdown mode.
 * @note 2. In the shutdown mode of the shipping mode, the IO does not remain. It is in an input floating state.
 * @note 3. In the shutdown mode, the current is around 300 nA.
 * @note 4. For normal use, GPIO10 should be set as an input and then pulled low or high.
 * @note    However, if it is set as an output and controlled by software to output low or high,
 * @note    the hardware will detect the high or low level and also take effect, thus entering or exiting the shutdown mode.
 *******************************************************************************
 */
void example_shelf_mode(void)
{
    drv_pmu_shelf_mode_enable(true);
}

/** @} */