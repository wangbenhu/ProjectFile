/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup MPU MPU
 * @ingroup  DRIVER
 * @brief    MPU for cortex-M4F driver
 * @details  MPU driver
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_MPU_H
#define __DRV_MPU_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include "om_device.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * TYPEDEFS
 */
typedef enum {
    MPU_REGION_SIZE_32B       = ARM_MPU_REGION_SIZE_32B,
    MPU_REGION_SIZE_64B       = ARM_MPU_REGION_SIZE_64B,
    MPU_REGION_SIZE_128B      = ARM_MPU_REGION_SIZE_128B,
    MPU_REGION_SIZE_256B      = ARM_MPU_REGION_SIZE_256B,
    MPU_REGION_SIZE_512B      = ARM_MPU_REGION_SIZE_512B,
    MPU_REGION_SIZE_1KB       = ARM_MPU_REGION_SIZE_1KB,
    MPU_REGION_SIZE_2KB       = ARM_MPU_REGION_SIZE_2KB,
    MPU_REGION_SIZE_4KB       = ARM_MPU_REGION_SIZE_4KB,
    MPU_REGION_SIZE_8KB       = ARM_MPU_REGION_SIZE_8KB,
    MPU_REGION_SIZE_16KB      = ARM_MPU_REGION_SIZE_16KB,
    MPU_REGION_SIZE_32KB      = ARM_MPU_REGION_SIZE_32KB,
    MPU_REGION_SIZE_64KB      = ARM_MPU_REGION_SIZE_64KB,
    MPU_REGION_SIZE_128KB     = ARM_MPU_REGION_SIZE_128KB,
    MPU_REGION_SIZE_256KB     = ARM_MPU_REGION_SIZE_256KB,
    MPU_REGION_SIZE_512KB     = ARM_MPU_REGION_SIZE_512KB,
    MPU_REGION_SIZE_1MB       = ARM_MPU_REGION_SIZE_1MB,
    MPU_REGION_SIZE_2MB       = ARM_MPU_REGION_SIZE_2MB,
    MPU_REGION_SIZE_4MB       = ARM_MPU_REGION_SIZE_4MB,
    MPU_REGION_SIZE_8MB       = ARM_MPU_REGION_SIZE_8MB,
    MPU_REGION_SIZE_16MB      = ARM_MPU_REGION_SIZE_16MB,
    MPU_REGION_SIZE_32MB      = ARM_MPU_REGION_SIZE_32MB,
    MPU_REGION_SIZE_64MB      = ARM_MPU_REGION_SIZE_64MB,
    MPU_REGION_SIZE_128MB     = ARM_MPU_REGION_SIZE_128MB,
    MPU_REGION_SIZE_256MB     = ARM_MPU_REGION_SIZE_256MB,
    MPU_REGION_SIZE_512MB     = ARM_MPU_REGION_SIZE_512MB,
    MPU_REGION_SIZE_1GB       = ARM_MPU_REGION_SIZE_1GB,
    MPU_REGION_SIZE_2GB       = ARM_MPU_REGION_SIZE_2GB,
    MPU_REGION_SIZE_4GB       = ARM_MPU_REGION_SIZE_4GB,
} mpu_region_size_t;

typedef enum {
    MPU_MEM_ATTR_FLASH        = ARM_MPU_ACCESS_(0, 0, 1, 0),  /* Non-shareable, write through; TEX=0, S=0, C=1, B=0 */
    MPU_MEM_ATTR_INTERNAL_RAM = ARM_MPU_ACCESS_(0, 1, 1, 0),  /* shareable, write through;     TEX=0, S=1, C=1, B=0 */
    MPU_MEM_ATTR_EXTERNAL_RAM = ARM_MPU_ACCESS_(0, 1, 1, 1),  /* shareable, write back;        TEX=0, S=1, C=1, B=1 */
    MPU_MEM_ATTR_PERIPH       = ARM_MPU_ACCESS_(0, 1, 0, 1),  /* shareable devices;            TEX=0, S=1; C=0, B=1 */
} mpu_mem_attr_t;

typedef struct {
    uint32_t          start_addr;          /*< start_addr must align to size, see@mpu_region_size_t */
    mpu_region_size_t size;                /*<  region size */
    mpu_mem_attr_t    attr;                /*< region memory attribute */
    uint8_t           sub_region_disable;  /*< sub region disable,  an MPU region divide
                                               into 8 equal sub-regions, and each of them
                                               can be enabled or disabled individually */
    uint8_t           ro : 1;              /*< read-only */
    uint8_t           xn : 1;              /*< execute never */
} mpu_region_t;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Initialize MPU by region settings
 *
 * @param region       Pointer to region, see @mpu_region_t
 * @param region_size  region size
 *******************************************************************************
 */
extern void drv_mpu_init(const mpu_region_t *region, uint8_t region_size);

/**
 *******************************************************************************
 * @brief Setting MPU region according region index
 *
 * @param region_idx        MPU region index
 * @param region            Pointer to region, see @mpu_region_t
 *******************************************************************************
 */
extern void drv_mpu_set_region(uint8_t region_idx, const mpu_region_t *region);


#ifdef __cplusplus
}
#endif


#endif  /* __DRV_MPU_H */


/** @} */
