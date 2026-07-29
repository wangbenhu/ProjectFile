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
 * @brief    example for using wdt
 * @details  example for using wdt: configure wdt and feed wdt
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
/// Decide whether to feed the dog
#define FEED_DOG        1


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
 * @brief example of configuring and feeding wdt
 *
 *******************************************************************************
 */
void example_wdt(void)
{
    // The system is reset if the dog is not fed within 8s
    drv_wdt_init(8 * 1000);

    #if (FEED_DOG)
    // Feed the dog every 5 seconds
    while (1) {
        drv_wdt_keep_alive();
        DRV_DELAY_MS(5 * 1000);
    }
    #endif

}


/** @} */