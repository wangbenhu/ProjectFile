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
 * @brief    main entry
 * @details  main entry
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
#include "cmsis_os2.h"


/*******************************************************************************
 * Extern FUNCTIONS
 */
extern void system_init(void);

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
int main(void)
{
    // Initialize CMSIS-RTOS
    osKernelInitialize();

    system_init();

    // Start thread execution
    if (osKernelGetState() == osKernelReady) {
        osKernelStart();
    }

    /* Never run here */
    while(1);
}

/** @} */

