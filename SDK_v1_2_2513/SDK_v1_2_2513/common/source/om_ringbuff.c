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
 * @brief    ring buffer
 * @details  ring buffer
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
#include <string.h>
#include <stdint.h>
#include "om_ringbuff.h"


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
void om_ringbuff_init(ring_buff_t *rb, uint8_t *buf, uint16_t size)
{
    if (rb) {
        rb->buf          = buf;
        rb->size         = size;
        rb->wptr         = 0U;
        rb->rptr         = 0U;
        rb->overflow_cnt = 0U;
    }
}

uint8_t om_ringbuff_write_byte(ring_buff_t *rb, uint8_t data)
{
    uint8_t len;

    if (((rb->wptr + 1U) % rb->size) != rb->rptr) {
            rb->buf[rb->wptr] = data;
            rb->wptr = (rb->wptr + 1U) % rb->size;
            len = 1U;
    } else {
            rb->overflow_cnt++;
            len = 0U;
    }

    return len;
}

uint16_t om_ringbuff_write(ring_buff_t *rb, const uint8_t *data, uint16_t length)
{
    uint32_t space;
    space = rb->size - 1 - om_ringbuff_get_count(rb);
    if (length > space) {
        length = space;
    }
    if ((rb->wptr + length) > rb->size) {
        uint32_t temp1, temp2;
        temp1 = rb->size - rb->wptr;
        memcpy((void *)(rb->buf + rb->wptr), data, temp1);

        temp2 = length - temp1;
        memcpy(rb->buf, (void *)(data + temp1), temp2);
        rb->wptr = temp2;
    } else {
        memcpy(rb->buf + rb->wptr, data, length);
        rb->wptr += length;
    }

    return length;
}

uint8_t om_ringbuff_read_byte(ring_buff_t *rb, uint8_t *data)
{
    uint8_t len;

    if (rb->rptr != rb->wptr) {
        *data = rb->buf[rb->rptr];
        rb->rptr = (rb->rptr + 1U) % rb->size;
        len = 1U;
    } else {
        len = 0U;
    }

    return len;
}

uint16_t om_ringbuff_read(ring_buff_t *rb, uint8_t *data, uint16_t length)
{
    if (length > om_ringbuff_get_count(rb)) {
        length = om_ringbuff_get_count(rb);
    }

    if ((rb->rptr + length) >= rb->size) {
        uint32_t temp1, temp2;
        temp1 = rb->size - rb->rptr;
        memcpy(data, (void *)(rb->buf + rb->rptr), temp1);
        temp2 = length - temp1;
        memcpy((void *)(data + temp1), rb->buf, temp2);
        rb->rptr = temp2;
    } else {
        memcpy(data, (void *)(rb->buf + rb->rptr), length);
        rb->rptr += length;
    }

    return length;
}

uint16_t om_ringbuff_get_count(ring_buff_t *rb)
{
    return (((uint32_t)(rb->wptr + rb->size - rb->rptr)) % rb->size);
}

uint16_t om_ringbuff_read_overflow(ring_buff_t *rb)
{
    return rb->overflow_cnt;
}

void om_ringbuff_clear_overflow(ring_buff_t *rb)
{
    rb->overflow_cnt = 0;
}


/** @} */
