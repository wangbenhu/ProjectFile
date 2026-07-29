/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @file rwmem.c
 * @brief
 * @date Thu, Jun 13, 2019 10:05:01 AM
 * @author liqiang
 *
 * @addtogroup
 * @ingroup
 * @details
 *
 * @{
 */

/*********************************************************************
 * INCLUDES
 */
#include "autoconf.h"
#include "ob_config.h"

#include <string.h>
#include <stdio.h>
#include "stdlib.h"

#include "ob_mem.h"

/*********************************************************************
 * MACROS
 */

/// Fast heap size
#define OB_HEAP_FAST_SIZE  (   1000*OB_LE_OBSERVER \
                             + 1 * (  (16 + (CONFIG_LE_ACTIVITY_NB-1) * 56) \
                                    + (58 + (CONFIG_LE_ACTIVITY_NB-1) * 26) \
                                    + (CONFIG_LE_ACTIVITY_NB * 66) \
                                    + (CONFIG_LE_LL_HCI_CMD_PKTS_NB * 255) \
                                    + (OB_LE_OBSERVER * OB_LE_MAX_NB_ADV_REP_FRAG * 255 + CONFIG_LE_ACTIVITY_NB * 100) \
                                    + (CONFIG_LE_ACTIVITY_NB * 12) \
                                   ) )

/// Slow heap size
#define OB_HEAP_SLOW_SIZE       (330*OB_LE_CENTRAL + 270*OB_LE_OBSERVER + CONFIG_LE_ACTIVITY_NB*230)

/// Size of non-retention heap
#define OB_HEAP_NRET_SIZE    ((1980 + 336) * OB_LE_LL_SEC_CON + 12 /* one more sizeof(struct mblock_free) space */)

/// Host heap size
#define OB_HEAP_HOST_SIZE      (480 \
                             + (360 + OB_LE_HOST_ATT_MTU + OB_LE_HOST_ATT_WRITE_CACHE_SIZE) * OB_LE_HOST_CONNECTION_NB \
                             + (132 * OB_LE_HOST_CONNECTION_NB * OB_LE_HOST_SC_PAIRING) \
                             + ( 20 * OB_LE_HOST_ADV_SET_NUM) \
                             + (  4 * OB_LE_HOST_MAX_GATT_SERV_NUM) \
                             + (OB_LE_HOST_MSG_SIZE))

/// ceil(len/4) + header_size
#define OB_HEAP_LEN_ALIGN(len)  ((((len)+3)/4) + (12/4))

/*********************************************************************
 * TYPEDEFS
 */


/*********************************************************************
 * CONSTANTS
 */


/*********************************************************************
 * LOCAL VARIABLES
 */

/// Memory allocated for environment variables
static uint32_t ob_heap_slow_env[OB_HEAP_LEN_ALIGN(OB_HEAP_SLOW_SIZE)];
/// Memory allocated for messages
static uint32_t ob_heap_fast_env[OB_HEAP_LEN_ALIGN(OB_HEAP_FAST_SIZE)];
/// Non Retention memory block
static uint32_t ob_heap_nret_env[OB_HEAP_LEN_ALIGN(OB_HEAP_NRET_SIZE)];

#ifdef CONFIG_BLE_HOST
/// Memory allocated for ble host
static uint32_t ob_heap_host_env[OB_HEAP_LEN_ALIGN(OB_HEAP_HOST_SIZE)];
#endif

/*********************************************************************
 * GLOBAL VARIABLES
 */


/*********************************************************************
 * LOCAL FUNCTIONS
 */


/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/**
 *******************************************************************************
 * @brief  ob mem heap init
 *******************************************************************************
 */
void ob_mem_heap_init(void)
{
    // Initialize memory heap used by kernel.
    ob_mem_init();
    // slow alloc/free heap
    ob_mem_heap_set(OB_MEM_SLOW, (uint8_t*)ob_heap_slow_env, sizeof(ob_heap_slow_env));
    // fast alloc/free heap
    ob_mem_heap_set(OB_MEM_FAST, (uint8_t*)ob_heap_fast_env, sizeof(ob_heap_fast_env));
    // Non Retention memory block
    ob_mem_heap_set(OB_MEM_NRET, (uint8_t*)ob_heap_nret_env, sizeof(ob_heap_nret_env));

#ifdef CONFIG_BLE_HOST
    // Memory allocated for ble host
    ob_mem_heap_set(OB_MEM_HOST, (uint8_t*)ob_heap_host_env, sizeof(ob_heap_host_env));
#endif
}

uint32_t obc_co_trng_word(void)
{
 #if (RTE_RNG)
    uint32_t rand;
    drv_rng_read((uint8_t *)&rand, 4);
    return rand;
#else
    return (uint32_t)rand();
#endif
}


#if CONFIG_BLE_CONTROLLER
void BLE_IRQHandler(void)
{
    extern void obc_isr(void);
    obc_isr();
}
#endif

/** @} */
