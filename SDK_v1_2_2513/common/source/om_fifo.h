/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup FIFO FIFO
 * @ingroup  COMMON
 * @brief    fifo with unlimited size
 * @details  fifo with unlimited size
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


#ifndef __OM_FIFO_H
#define __OM_FIFO_H


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
/// fifo struct
typedef struct {
    /// fifo's buffer
    unsigned char *buffer;
    /// buffer total size
    unsigned int size;
    /// in position
    volatile unsigned int in;
    /// out position
    volatile unsigned int out;
} om_fifo_t;


/*******************************************************************************
 * EXTERN FUNCTIONS
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
extern unsigned int om_fifo_size(om_fifo_t* fifo);

/**
 *******************************************************************************
 * @brief  om nfifo avail
 *
 * @param[in] fifo  fifo
 *
 * @return valid length
 *******************************************************************************
 **/
extern unsigned int om_fifo_avail(om_fifo_t *fifo);

/**
 *******************************************************************************
 * @brief  om nfifo len
 *
 * @param[in] fifo  fifo
 *
 * @return length
 *******************************************************************************
 **/
extern unsigned int om_fifo_len(om_fifo_t* fifo);

/**
 *******************************************************************************
 * @brief whether is fifo empty
 *
 * @param[in] fifo  fifo object
 *
 * @return empty?
 *******************************************************************************
 **/
extern int om_fifo_is_empty(om_fifo_t *fifo);

/**
 * @brief  om fifo reset
 *
 * @param[in] fifo  fifo
 **/
extern void om_fifo_reset(om_fifo_t *fifo);

/**
 *******************************************************************************
 * @brief  om nfifo init
 *
 * @param[in] fifo  fifo
 * @param[in] buffer  buffer
 * @param[in] size  size
 *******************************************************************************
 **/
extern void om_fifo_init(om_fifo_t *fifo, unsigned char *buffer, unsigned int size);

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
extern unsigned int om_fifo_in_1byte(om_fifo_t *fifo, const unsigned char *from);

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
extern unsigned int om_fifo_in(om_fifo_t *fifo, const unsigned char *from, unsigned int len);

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
extern unsigned int om_fifo_out_1byte(om_fifo_t *fifo, unsigned char *to);

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
extern unsigned int om_fifo_out(om_fifo_t *fifo, unsigned char *to, unsigned int len);

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
extern unsigned int om_fifo_peek(om_fifo_t *fifo, unsigned char *to, unsigned int len);


#ifdef  __cplusplus
}
#endif

#endif  /* __OM_FIFO_H */

/** @} */
