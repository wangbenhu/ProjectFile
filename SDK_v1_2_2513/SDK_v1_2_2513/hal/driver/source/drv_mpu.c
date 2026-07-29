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
 * @brief    MPU driver source file
 * @details  MPU driver source file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include "om_device.h"
#include "drv_mpu.h"
#include "om_common.h"


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
void drv_mpu_init(const mpu_region_t *region, uint8_t region_size)
{
    for (uint32_t i=0; i<region_size; i++) {
        drv_mpu_set_region(i, region + i);
    }
}

void drv_mpu_set_region(uint8_t region_idx, const mpu_region_t *region)
{
    OM_ASSERT((MPU->TYPE >> MPU_TYPE_DREGION_Pos) > region_idx);
    OM_ASSERT(region);
    OM_ASSERT((region->start_addr & ((2U << (uint32_t)(region->size)) - 1U)) == 0U);

    __DMB();
    MPU->RNR = region_idx;
    MPU->RBAR = ARM_MPU_RBAR(region_idx, region->start_addr);
    MPU->RASR = ARM_MPU_RASR_EX(region->xn, (region->ro ? ARM_MPU_AP_RO : ARM_MPU_AP_FULL),
                                region->attr, region->sub_region_disable, region->size);
    MPU->CTRL = MPU_CTRL_ENABLE_Msk;  /* enable MPU */
    __DSB();
    __ISB();
}


/** @} */
