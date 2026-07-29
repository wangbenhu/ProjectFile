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
 * @brief    memory heap manager
 * @details  memory heap manager
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
#include "om_mem.h"

#if (CONFIG_MEM_NUM)

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "om_driver.h"
#include "om_common.h"


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct om_mem_heap {            /* << Memory Pool management struct >>     */
    struct om_mem_heap   *next;         /* Next Memory Block in the list           */
    uint32_t              len;          /* Length of data block                    */
} om_mem_heap_t;

typedef struct {
    uint32_t base[CONFIG_MEM_NUM];
    uint32_t end[CONFIG_MEM_NUM];
} om_mem_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
static om_mem_env_t om_mem_env;


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
void om_mem_init(void)
{
    memset((void *)(&om_mem_env), 0, sizeof(om_mem_env));
}

void om_mem_register(om_mem_type_t mem_type, void *pool, uint32_t size)
{
    om_mem_heap_t *mem;

    OM_ASSERT(mem_type < CONFIG_MEM_NUM);
    OM_ASSERT(OM_IS_ALIGN(pool, 4) && (pool != NULL));
    OM_ASSERT(OM_IS_ALIGN(size, 4) && (size != 0U));

    if ((uint32_t)pool) {
        OM_ASSERT (size > sizeof(om_mem_heap_t));

        mem = (om_mem_heap_t *)pool;
        mem->next       = (om_mem_heap_t *)((uint32_t)pool + size - sizeof(om_mem_heap_t *));
        mem->len        = 0U;
        mem->next->next = NULL;
    }

    om_mem_env.base[mem_type] = (uint32_t)pool;
    om_mem_env.end[mem_type]  = (uint32_t)pool + size;
}

void *om_mem_malloc(om_mem_type_t mem_type, uint32_t size)
{
    om_mem_heap_t *p, *p_search, *p_new;
    uint32_t hole_size;
    uint32_t pool;

    OM_ASSERT(mem_type < CONFIG_MEM_NUM);
    pool = om_mem_env.base[mem_type];
    if(!pool) {
        return NULL;
    }

    p_search = (om_mem_heap_t *)pool;
    /* Add header offset to 'size' */
    size += sizeof(om_mem_heap_t);
    /* Make sure that block is 4-byte aligned  */
    size = OM_ALIGN_CEIL(size, 4);

    OM_CRITICAL_BEGIN();
    while (1) {
        hole_size  = (uint32_t)p_search->next - (uint32_t)p_search;
        hole_size -= p_search->len;
        /* Check if hole size is big enough */
        if (hole_size >= size) {
            break;
        }
        p_search = p_search->next;
        if (p_search->next == NULL) {
            /* Failed, we are at the end of the list */
            p = NULL;
            goto _exit;
        }
    }

    if (p_search->len == 0U) {
        /* No block is allocated, set the Length of the first element */
        p_search->len = size;
        p = (om_mem_heap_t *)(((uint32_t)p_search) + sizeof(om_mem_heap_t));
    } else {
        /* Insert new list element into the memory list */
        p_new       = (om_mem_heap_t *)((uint32_t)p_search + p_search->len);
        p_new->next = p_search->next;
        p_new->len  = size;
        p_search->next = p_new;
        p = (om_mem_heap_t *)(((uint32_t)p_new) + sizeof(om_mem_heap_t));
    }

_exit:
    OM_CRITICAL_END();

    return (p);
}

void om_mem_free(om_mem_type_t mem_type, void *mem)
{
    om_mem_heap_t *p_search, *p_prev, *p_return;
    uint32_t pool;

    OM_ASSERT(mem_type < CONFIG_MEM_NUM);

    if (((uint32_t)mem <= om_mem_env.base[mem_type]) || ((uint32_t)mem >= om_mem_env.end[mem_type])) {
        return;
    }
    pool = om_mem_env.base[mem_type];
    OM_ASSERT(pool);

    OM_CRITICAL_BEGIN();
    p_return = (om_mem_heap_t *)((uint32_t)mem - sizeof(om_mem_heap_t));

    /* Set list header */
    p_prev = NULL;
    p_search = (om_mem_heap_t *)pool;
    while (p_search != p_return) {
        p_prev   = p_search;
        p_search = p_search->next;
        if (p_search == NULL) {
            /* Valid Memory block not found */
            goto _exit;
        }
    }

    if (p_prev == NULL) {
        /* First block to be released, only set length to 0 */
        p_search->len = 0U;
    } else {
        /* Discard block from chain list */
        p_prev->next = p_search->next;
    }

_exit:
    OM_CRITICAL_END();
}

void *om_mem_calloc(om_mem_type_t mem_type, uint8_t num, uint32_t size)
{
    void *mem;
    mem = om_mem_malloc(mem_type, num*size);
    if (mem != NULL) {
        memset(mem, 0, num*size);
    }

    return mem;
}

void om_mem_dump(void *print)
{
    int (*log)(const char *format, ...) = (int (*) (const char *format, ...))print;

    om_mem_heap_t *p_base = NULL, *p_end = NULL, *p_search;

    if(NULL == log) {
        return;
    }

    for(int type = 0; type < CONFIG_MEM_NUM; type++) {
        if(om_mem_env.base[type] == 0) {
            continue;
        }

        p_base = (om_mem_heap_t *)om_mem_env.base[type];
        p_end  = (om_mem_heap_t *)(om_mem_env.end[type] - sizeof(om_mem_heap_t *));

        log("Heap base: 0x%x, Heap end: 0x%x, Heap size: %d\r\n", om_mem_env.base[type], om_mem_env.end[type], om_mem_env.end[type] - om_mem_env.base[type]);

        for(p_search = p_base; (p_search != NULL) && (p_search < p_end); p_search = p_search->next) {
            if(p_search == p_base && p_search->len == 0) {
                log("[freed] Pos: 0x%x, Len: %d\n", (uint32_t)p_base, (uint32_t)(((om_mem_heap_t *)p_base)->next) - (uint32_t)p_base);
            } else {
                log("[alloc] Pos: 0x%x, Len: %d\n", (uint32_t)p_search, p_search->len);
                if(((uint32_t)p_search->next - (uint32_t)p_search) > p_search->len) {
                    log("[freed] Pos: 0x%x, Len: %d\n", (uint32_t)p_search + p_search->len, (uint32_t)p_search->next - ((uint32_t)p_search + p_search->len));
                }
            }
        }
    }
}

#endif

/** @} */
