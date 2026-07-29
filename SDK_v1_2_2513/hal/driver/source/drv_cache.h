/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup CACHE CACHE
 * @ingroup  DRIVER
 * @brief    CACHE driver
 * @details  CACHE driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_CACHE_H
#define __DRV_CACHE_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_ICACHE)
#include <stddef.h>
#include <stdint.h>

#ifdef  __cplusplus
extern "C"
{
#endif

/**
 *******************************************************************************
 * @brief  drv icache invalidate
 *******************************************************************************
 */
__STATIC_FORCEINLINE void drv_icache_invalidate(void)
{
    OM_ICACHE->MAINT0 = ICACHE_MAINT0_INVALL_MASK;
}

/**
 *******************************************************************************
 * @brief  icache enable, not invalidate icache.
 *******************************************************************************
 */
__STATIC_FORCEINLINE void drv_icache_enable(void)
{
    OM_ICACHE->CONFIG = 0;
    OM_ICACHE->CTRL = ICACHE_CTRL_CEN_MASK;
    while((OM_ICACHE->STATUS & ICACHE_STATUS_CSTS_MASK) == 0);
}

/**
 *******************************************************************************
 * @brief  drv icache disable
 *******************************************************************************
 */
__STATIC_FORCEINLINE void drv_icache_disable(void)
{
    if (OM_ICACHE->STATUS & ICACHE_STATUS_CSTS_MASK) {
        OM_ICACHE->CTRL = 0;
        while((OM_ICACHE->STATUS & ICACHE_STATUS_CSTS_MASK) != 0);
    }
    drv_icache_invalidate();
    OM_ICACHE->CONFIG = ICACHE_CONFIG_GCLKDIS_MASK;
}


#ifdef  __cplusplus
}
#endif

#endif  /* (RTE_ICACHE) */


#endif /* __DRV_CACHE_H */


/** @} */
