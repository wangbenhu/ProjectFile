/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup RINGBUFF RINGBUFF
 * @ingroup  COMMON
 * @brief    template
 * @details  template, template for .h header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


#ifndef __OM_RINGBUFF_H
#define __OM_RINGBUFF_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include <stddef.h>

#ifdef  __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    uint8_t              *buf;          // pointer to ring buffer
    uint16_t              size;         // ring buffer size
    uint16_t              wptr;         // write pointer of the ring buffer
    uint16_t              rptr;         // read pointer of the ring buffer
    uint16_t              overflow_cnt; // overflow count
} ring_buff_t;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 *  @brief Initialized a ring buffer
 *
 * @param[in]   rb     Pointer to the handler of ring buffer
 * @param[in]   buf    Pointer to a ring buffer
 * @param[in]   size   Size of ring buffer
 *
 *******************************************************************************
 */
extern void om_ringbuff_init(ring_buff_t *rb, uint8_t *buf, uint16_t size);

/**
 *******************************************************************************
 *  @brief Write one byte to ring buffer
 *
 * @param[in]   rb     Pointer to the handler of ring buffer
 * @param[in]   data   Pointer to in data
 *
 * @return             Actual length into ring buffer
 *
 *******************************************************************************
 */
extern uint8_t om_ringbuff_write_byte(ring_buff_t *rb, uint8_t data);

/**
 *******************************************************************************
 * @brief Write some byte to ring buffer
 *
 * @param[in] rb       Pointer to the handler of ring buffer
 * @param[in] data     Pointer to in data
 * @param[in] length   Expected length into ring buffer
 *
 * @return             Actual length in ring buffer
 *
 *******************************************************************************
 */
extern uint16_t om_ringbuff_write(ring_buff_t *rb, const uint8_t *data, uint16_t length);

/**
 *******************************************************************************
 * @brief Read 1 byte from ring buffer
 *
 * @param[in] rb       Pointer to the handler of ring buffer
 * @param[in] data     Pointer to out data
 *
 * @return             Actual length out from ring buffer
 *
 *******************************************************************************
 */
extern uint8_t om_ringbuff_read_byte(ring_buff_t *rb, uint8_t *data);

/**
 *******************************************************************************
 * @brief Read n bytes from ring buffer
 *
 * @param[in] rb       Pointer to the handler of ring buffer
 * @param[in] data     Out data
 * @param[in] length   Expected read data length
 *
 * @return             Actual length out from ring buffer
 *
 *******************************************************************************
 */
extern uint16_t om_ringbuff_read(ring_buff_t *rb, uint8_t *data, uint16_t length);

/**
 *******************************************************************************
 * @brief Get bytes in ring buffer
 *
 * @param[in] rb       Pointer to the handler of ring buffer
 *
 * @return             Length in ring buffer
 *
 *******************************************************************************
 */
extern uint16_t om_ringbuff_get_count(ring_buff_t *rb);

/**
 *******************************************************************************
 * @brief Get overflow count in ring buffer
 *
 * @param[in] rb       Pointer to the handler of ring buffer
 *
 * @return             Overflow count
 *
 *******************************************************************************
 */
extern uint16_t om_ringbuff_read_overflow(ring_buff_t *rb);

/**
 *******************************************************************************
 * @brief Clear overflow count in ring buffer
 *
 * @param[in] rb       Pointer to the handler of ring buffer
 *
 *******************************************************************************
 */
extern void om_ringbuff_clear_overflow(ring_buff_t *rb);

#ifdef  __cplusplus
}
#endif

#endif  /* __OM_RINGBUFF_H */


/** @} */
