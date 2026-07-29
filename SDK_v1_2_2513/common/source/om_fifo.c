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
 * @brief    fifo with unlimited size
 * @details  fifo with unlimited size
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
#include <string.h>
#include "om_fifo.h"


/**
 *******************************************************************************
 * @brief  om fifo min
 *
 * @param[in] a  a
 * @param[in] b  b
 *
 * @return min
 *******************************************************************************
 **/
static unsigned int om_fifo_min(unsigned int a, unsigned int b)
{
    return (a < b) ? a : b;
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief  om nfifo size
 *
 * @param[in] fifo  fifo
 *
 * @return size
 *******************************************************************************
 **/
unsigned int om_fifo_size(om_fifo_t* fifo)
{
    return fifo->size;
}

/**
 *******************************************************************************
 * @brief  om nfifo avail
 *
 * @param[in] fifo  fifo
 *
 * @return valid length
 *******************************************************************************
 **/
unsigned int om_fifo_avail(om_fifo_t *fifo)
{
    return fifo->size - om_fifo_len(fifo) - 1U;
}

/**
 *******************************************************************************
 * @brief  om nfifo len
 *
 * @param[in] fifo  fifo
 *
 * @return length
 *******************************************************************************
 **/
unsigned int om_fifo_len(om_fifo_t* fifo)
{
    unsigned int in;
    unsigned int out;

    in = fifo->in;
    out = fifo->out;
    return (in + fifo->size - out) % fifo->size;
}

/**
 *******************************************************************************
 * @brief whether is fifo empty
 *
 * @param[in] fifo  fifo object
 *
 * @return empty?
 *******************************************************************************
 **/
int om_fifo_is_empty(om_fifo_t *fifo)
{
    unsigned int in;
    unsigned int out;

    in = fifo->in;
    out = fifo->out;
    return in == out;
}

/**
 * @brief  om fifo reset
 *
 * @param[in] fifo  fifo
 **/
void om_fifo_reset(om_fifo_t *fifo)
{
    fifo->in = fifo->out = 0;
}

/**
 *******************************************************************************
 * @brief  om nfifo init
 *
 * @param[in] fifo  fifo
 * @param[in] buffer  buffer
 * @param[in] size  size
 *******************************************************************************
 **/
void om_fifo_init(om_fifo_t *fifo, unsigned char *buffer, unsigned int size)
{
    fifo->buffer = buffer;
    fifo->size = size;

    om_fifo_reset(fifo);
}

/**
 *******************************************************************************
 * @brief  om nfifo in 1byte
 *
 * @param[in] fifo  fifo
 * @param[in] from  from
 *
 * @return in bytes
 *******************************************************************************
 **/
unsigned int om_fifo_in_1byte(om_fifo_t *fifo, const unsigned char *from)
{
    unsigned char *to = fifo->buffer + fifo->in;
    unsigned int in = (fifo->in + 1) % fifo->size;

    if (in != fifo->out) {
        *to = *from;
        fifo->in = in;
        return 1;
    }

    return 0;
}

/**
 *******************************************************************************
 * @brief  om nfifo in
 *
 * @param[in] fifo  fifo
 * @param[in] from  from
 * @param[in] len  len
 *
 * @return in bytes
 *******************************************************************************
 **/
unsigned int om_fifo_in(om_fifo_t *fifo, const unsigned char *from, unsigned int len)
{
    unsigned char *to = fifo->buffer + fifo->in;

    len = om_fifo_min(len, om_fifo_avail(fifo));
    if (fifo->in + len >= fifo->size) {
        unsigned int temp1, temp2;
        temp1 = fifo->size - fifo->in;
        memcpy(to, from, temp1);
        from += temp1;
        temp2 = len - temp1;
        to = fifo->buffer;
        memcpy(to, from, temp2);
        fifo->in = temp2;
    } else {
        memcpy(to, from, len);
        fifo->in += len;
    }

    return len;
}

/**
 *******************************************************************************
 * @brief  om nfifo out 1byte
 *
 * @param[in] fifo  fifo
 * @param[in] to  to
 *
 * @return  out bytes
 *******************************************************************************
 **/
unsigned int om_fifo_out_1byte(om_fifo_t *fifo, unsigned char *to)
{
    unsigned int in;
    unsigned int out;

    in = fifo->in;
    out = fifo->out;
    if (in != out) {
        *to = *(fifo->buffer + fifo->out);
        fifo->out = (fifo->out + 1) % fifo->size;
        return 1;
    }

    return 0;
}

/**
 *******************************************************************************
 * @brief  om nfifo out
 *
 * @param[in] fifo  fifo
 * @param[in] to  to
 * @param[in] len  len
 *
 * @return out bytes
 *******************************************************************************
 **/
unsigned int om_fifo_out(om_fifo_t *fifo, unsigned char *to, unsigned int len)
{
    unsigned int actual_len;

    actual_len = om_fifo_peek(fifo, to, len);
    fifo->out = (fifo->out + actual_len) % fifo->size;

    return actual_len;
}

/**
 *******************************************************************************
 * @brief  peek data from fifo
 *
 * @param[in] fifo  fifo
 * @param[in] to  to
 * @param[in] len  len
 *
 * @return peeked length
 *******************************************************************************
 **/
unsigned int om_fifo_peek(om_fifo_t *fifo, unsigned char *to, unsigned int len)
{
    unsigned char *from = fifo->buffer + fifo->out;

    len = om_fifo_min(len, om_fifo_len(fifo));
    if (fifo->out + len >= fifo->size) {
        unsigned int temp1, temp2;
        temp1 = fifo->size - fifo->out;
        memcpy(to, from, temp1);
        to += temp1;
        temp2 = len - temp1;
        from = fifo->buffer;
        memcpy(to, from, temp2);
    } else {
        memcpy(to, from, len);
    }

    return len;
}

/**
 * @}
 */
