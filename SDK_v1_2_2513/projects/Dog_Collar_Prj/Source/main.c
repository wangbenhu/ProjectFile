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
#include "common_def.h"

/*******************************************************************************
 * EXTERN FUNCTIONS
 */
extern void system_init(void);
extern osThreadId_t vStartEntryTask(void);
extern osStatus_t lfs_port_sync_init(void);
/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
int main(void)
{
    // Initialize CMSIS-RTOS
    osKernelInitialize();

    system_init();
	
	if (lfs_port_sync_init() != osOK)
    {
        /* 启动失败处理 */
    }

	vStartEntryTask();
	
    // Start thread execution
    if (osKernelGetState() == osKernelReady) {
        osKernelStart();
    }

    /* Never run here */
    while(1);
}

/** @} */
