/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup CACHE CACHE
 * @ingroup  DEVICE
 * @brief    CACHE register
 * @details  CACHE register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __CACHE_REG_H
#define __CACHE_REG_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include "common_reg.h"


#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
#define ICACHE_CONFIG_GCLKDIS_MASK  0x00000001
#define ICACHE_CTRL_CEN_MASK        0x00000001
#define ICACHE_STATUS_CSTS_MASK     0x00000001
#define ICACHE_MAINT0_INVALL_MASK   0x00000001

/*******************************************************************************
 * TYPEDEFS
 */
typedef struct
{
    __I  uint32_t TYPE;
    __IO uint32_t CONFIG;
    __IO uint32_t CTRL;
    __IO uint32_t STATUS;
         uint32_t RESERVE0[4];
    __IO uint32_t MAINT0;
    __IO uint32_t MAINT1;
    __IO uint32_t MON_CONFIG;
    __IO uint32_t MON_EN;
    __IO uint32_t MON_CTRL;
    __IO uint32_t MON_STATUS;
} OM_ICACHE_Type;

/*******************************************************************************
 * EXTERN VARIABLES
 */


/*******************************************************************************
 * EXTERN FUNCTIONS
 */


#ifdef __cplusplus
}
#endif

#endif  /* __CACHE_REG_H */


/** @} */
