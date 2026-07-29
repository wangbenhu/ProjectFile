/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup MEM MEM
 * @ingroup  COMMON
 * @brief    memory heap manager
 * @details  memory heap manager
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __OM_MEM_H
#define __OM_MEM_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include "autoconf.h"


#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
#ifndef CONFIG_MEM_NUM
#define CONFIG_MEM_NUM      0U
#endif

#if (CONFIG_MEM_NUM)

// Memory Region Index
#define OM_MEM(n)               (n)


/*******************************************************************************
 * TYPEDEFS
 */
typedef uint8_t om_mem_type_t;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief  Initialize memory block
 *******************************************************************************
 */
extern void om_mem_init(void);

/**
 *******************************************************************************
 * @brief  Register memory block to the memory type
 *
 * @param[in] mem_type    Memory type
 * @param[in] pool        Memory pool, need align to 4 Byte
 * @param[in] size        Size of the memory block, in bytes, need to align to 4 Byte
 *******************************************************************************
 */
extern void om_mem_register(om_mem_type_t mem_type, void *pool, uint32_t size);

/**
 *******************************************************************************
 * @brief  Allocates a block of size bytes from the memory type
 *
 * @param[in] mem_type    Memory type
 * @param[in] size        Size of the memory block, in bytes
 *
 * @return                Return a pointer to the beginning of the block
 *******************************************************************************
 */
extern void *om_mem_malloc(om_mem_type_t mem_type, uint32_t size);

/**
 *******************************************************************************
 * @brief  Deallocate memory block
 *
 * @param[in] mem_type    Memory type
 * @param[in] mem         Pointer to a memory block previously allocated with malloc/calloc
 *******************************************************************************
 */
extern void om_mem_free(om_mem_type_t mem_type, void *mem);

/**
 *******************************************************************************
 * @brief  Callocate memory block
 *
 * @param[in] mem_type    Memory type
 * @param[in] num         Number of chunk
 * @param[in] size        size of chunk
 *
 * @return                Return a pointer to the beginning of the block
 *******************************************************************************
 */
extern void *om_mem_calloc(om_mem_type_t mem_type, uint8_t num, uint32_t size);

/**
 *******************************************************************************
 * @brief  Dump memory node information
 *
 * @param[in] print       Dump output log function pointer, like printf
 *
 * @return                None
 *******************************************************************************
 */
extern void om_mem_dump(void *print);

#endif

#ifdef __cplusplus
}
#endif

#endif  /* __OM_MEM_H */


/** @} */
