/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

#ifndef _OB_MEM_H_
#define _OB_MEM_H_

#include "ob_config.h"     // IP configuration
#include <stdint.h>          // standard integer
#include <stdbool.h>         // standard includes

/*
 * TYPE
 ****************************************************************************************
 */

/// memory heaps types.
enum OB_MEM_HEAP
{
    /// Environment variables
    OB_MEM_SLOW,
    /// Messages
    OB_MEM_FAST,
    /// Non Retention
    OB_MEM_NRET,

    /// Host
    OB_MEM_HOST,

    /// MAX
    OB_MEM_BLOCK_MAX,
};

/**
 *******************************************************************************
 * @brief  ob mem init
 *******************************************************************************
 */
void ob_mem_init(void);

/**
 *******************************************************************************
 * @brief  ob mem heap set
 *
 * @param[in] type  type
 * @param[in] heap  heap
 * @param[in] heap_size  heap size
 *******************************************************************************
 */
void ob_mem_heap_set(uint8_t type, uint8_t* heap, uint16_t heap_size);

/**
 *******************************************************************************
 * @brief  ob mem malloc
 *
 * @param[in] size  size
 * @param[in] type  type
 *******************************************************************************
 */
void *ob_mem_malloc(uint32_t size, uint8_t type);

/**
 *******************************************************************************
 * @brief  ob mem malloc check
 *
 * @param[in] size  size
 * @param[in] type  type
 *
 * @return
 *******************************************************************************
 */
bool ob_mem_malloc_check(uint32_t size, uint8_t type);

/**
 *******************************************************************************
 * @brief  ob mem free
 *
 * @param[in] mem_ptr  mem ptr
 *******************************************************************************
 */
void ob_mem_free(void *mem_ptr);

/**
 *******************************************************************************
 * @brief  ob mem is empty
 *
 * @param[in] type  type
 *
 * @return
 *******************************************************************************
 */
bool ob_mem_is_empty(uint8_t type);

/**
 ****************************************************************************************
 * @brief Check if current pointer is free or not
 *
 * @param[in] mem_ptr pointer to a memory block
 *
 * @return true if already free, false else.
 ****************************************************************************************
 */
bool ob_mem_is_freed(void* mem_ptr);

/**
 ****************************************************************************************
 * @brief Retrieve memory usage of selected heap.
 *
 * @param[in] type Type of memory heap block
 *
 * @return current memory usage of current heap.
 ****************************************************************************************
 */
uint16_t ob_mem_get_cur_usage(uint8_t type);


/**
 ****************************************************************************************
 * @brief Retrieve max memory usage of all heap.
 * This command also resets max measured value.
 *
 * @return max memory usage of all heap.
 ****************************************************************************************
 */
uint32_t ob_mem_get_max_usage(void);

#endif // _OB_MEM_H_

