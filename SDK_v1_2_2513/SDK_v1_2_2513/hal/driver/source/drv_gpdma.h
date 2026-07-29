/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup GPDMA GPDMA
 * @ingroup  DRIVER
 * @brief    GPDMA driver
 * @details  GPDMA driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_GPDMA_H
#define __DRV_GPDMA_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_GPDMA)
#include <stdint.h>
#include "om_device.h"
#include "om_driver.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
/// Max trans bytes = max_trans_size * src_width
#define GPDMA_CHAN_MAX_TRANS_SIZE         (0x3FFFFF)
/// DMA Control Setting
#define GPDMA_SET_CTRL(src_addr_ctrl, dst_addr_ctrl, src_width, dst_width, src_burst_size, priority) \
          (((src_addr_ctrl)  << GPDMA_CHAN_CTRL_SRCADDRCTRL_POS)   |   \
           ((dst_addr_ctrl)  << GPDMA_CHAN_CTRL_DSTADDRCTRL_POS)   |   \
           ((src_width)      << GPDMA_CHAN_CTRL_SRCWIDTH_POS)      |   \
           ((dst_width)      << GPDMA_CHAN_CTRL_DSTWIDTH_POS)      |   \
           ((src_burst_size) << GPDMA_CHAN_CTRL_SRCBURSTSIZE_POS)  |   \
           ((priority)       << GPDMA_CHAN_CTRL_PRIORITY_POS))

#define GPDMA_INVALID_HANDSHAKE_SIGNAL    0xFFU           /* invalid handshake signal */

/**
 *******************************************************************************
 * @brief RAM address must be upper 0x20000000U if GPDMA used to transfer to/from
 *        RAM memory. This macro used to convert RAM address to upper 0x20000000U
 *        If address not in lowwer RAM address, the macro donnot convert. This macro
 *        can be used to GPDMA chain struct member.
 *
 * @param[in] addr   GPDMA source or dst address.
 *******************************************************************************
 */
#define GPDMA_ADDR_CONVERT(addr)                                               \
    ((((addr) < (OM_MEM_RAM_BASE + CAP_RAM_SIZE)) && ((addr) >= OM_MEM_RAM_BASE)) ? (0x20000000U + (addr & (CAP_RAM_SIZE - 1U))) : addr)


/*******************************************************************************
 * TYPEDEFS
 */
/// GPDMA Address Config
typedef enum {
    /// Address Increase
    GPDMA_ADDR_CTRL_INC    = 0x00,
    /// Address Decrease
    GPDMA_ADDR_CTRL_DEC    = 0x01,
    /// Address Fixed
    GPDMA_ADDR_CTRL_FIXED  = 0x02,
} gpdma_addr_ctrl_t;

/// GPDMA Transfer Width
typedef enum {
    /// 1 byte is transfered at a time
    GPDMA_TRANS_WIDTH_1B   = 0x00,
    /// 2 bytes are transfered at a time
    GPDMA_TRANS_WIDTH_2B   = 0x01,
    /// 4 bytes are transfered at a time
    GPDMA_TRANS_WIDTH_4B   = 0x02,
} gpdma_trans_width_t;

/// GPDMA Burst Size
typedef enum {
    /// 1-transfer per burst
    GPDMA_BURST_SIZE_1T     = 0x00,
    /// 2-transfer per burst
    GPDMA_BURST_SIZE_2T     = 0x01,
    /// 4-transfer per burst
    GPDMA_BURST_SIZE_4T     = 0x02,
    /// 8-transfer per burst
    GPDMA_BURST_SIZE_8T     = 0x03,
    /// 16-transfer per burst
    GPDMA_BURST_SIZE_16T    = 0x04,
    /// 32-transfer per burst
    GPDMA_BURST_SIZE_32T    = 0x05,
    /// 64-transfer per burst
    GPDMA_BURST_SIZE_64T    = 0x06,
    /// 128-transfer per burst
    GPDMA_BURST_SIZE_128T   = 0x07,
} gpdma_burst_size_t;

/// GPDMA Transfer Priority
typedef enum {
    /// Low Priority
    GPDMA_PRIORITY_LOW     = 0x00,
    /// High Priority
    GPDMA_PRIORITY_HIGH    = 0x01,
} gpdma_priority_t;

/// DMA chain transfer, preload the next descriptor before the interrupt is generated
typedef struct {
    /// GPDMA Control
    uint32_t              ctrl;
    /// Source Address
    uint32_t              src_addr;
    /// Destination Address
    uint32_t              dst_addr;
    union {
        /// total number of transfered bytes
        uint32_t          size_byte;
        /// transSize = sizebyte / src_width
        uint32_t          trans_size;
    };
    /// Pointer to chain transfer item
    void                 *ll_ptr;
} gpdma_chain_trans_t;

/// GPDMA Event Callback
typedef void (*gpdma_isr_callback_t)(void *cb_param, drv_event_t event, gpdma_chain_trans_t *next_chain);

/// DMA Configuration
typedef struct {
    /// Channel Ctrl
    uint32_t                   channel_ctrl;
    /// Pheripheral Source Index
    gpdma_id_t                 src_id;
    /// Pheripheral Destination Index
    gpdma_id_t                 dst_id;
    /// GPDMA isr Callback
    gpdma_isr_callback_t       isr_cb;

    /// Callback Parameter
    void                      *cb_param;
    /// Pointer to chain transfer item
    gpdma_chain_trans_t       *chain_trans;
    /// Number of chain transfer item
    uint8_t                    chain_trans_num;
} gpdma_config_t;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Allocate a new channel
 *
 * @returns:
 *          [0, GPDMA_NUMBER_OF_CHANNELS-1]:  GPDMA channel allocated
 *          GPDMA_NUMBER_OF_CHANNELS:           No GPDMA channel can be allocated
 *
 *******************************************************************************
 */
extern uint8_t drv_gpdma_channel_allocate(void);

/**
 *******************************************************************************
 * @brief Release a channel
 *
 * @param   chan_idx        Channel index [0, GPDMA_NUMBER_OF_CHANNELS-1]
 *
 *******************************************************************************
 */
extern void drv_gpdma_channel_release(uint8_t chan_idx);

/**
 *******************************************************************************
 * @brief Configure DMA channel for next transfer
 * dma interrupt is enabled by default
 *
 * @param   chan_idx        Channel index [0, GPDMA_NUMBER_OF_CHANNELS-1]
 * @param   config          Channel config
 *
 * @return: errno
 *******************************************************************************
 */
extern om_error_t drv_gpdma_channel_config(uint8_t chan_idx, const gpdma_config_t *config);

/**
 *******************************************************************************
 * @brief Enable GPDMA channel for next transfer
 *
 * @param   chan_idx        Channel index [0, GPDMA_NUMBER_OF_CHANNELS-1]
 * @param   src_addr        Source address
 * @param   dst_addr        Destination address
 * @param   size            Amount of data to transfer in byte
 *
 * @return: errno
 *******************************************************************************
 */
extern om_error_t drv_gpdma_channel_enable(uint8_t chan_idx, uint32_t dst_addr, uint32_t src_addr, uint32_t size);

/**
 *******************************************************************************
 * @brief Disable GPDMA channel
 *
 * @param   chan_idx        Channel index [0, GPDMA_NUMBER_OF_CHANNELS-1]
 *
 * @return: errno
 *******************************************************************************
 */
extern om_error_t drv_gpdma_channel_disable(uint8_t chan_idx);

/**
 *******************************************************************************
 * @brief Check if GPDMA channel is busy
 *
 * @param   chan_idx  Channel index [0, GPDMA_NUMBER_OF_CHANNELS-1]
 *
 * @return:
 *         1          Channel is busy
 *         0          Channel is idle
 *******************************************************************************
 */
extern uint8_t drv_gpdma_channel_is_busy(uint8_t chan_idx);

/**
 *******************************************************************************
 * @brief Get the number of left data
 *
 * @param   chan_idx   Channel index [0, GPDMA_NUMBER_OF_CHANNELS-1]
 *
 * @return:  left count
 *******************************************************************************
 */
extern uint32_t drv_gpdma_channel_get_left_count(uint8_t chan_idx);

/**
 *******************************************************************************
 * @brief Set GPDMA link list ptr
 *
 * @param[in] chan_idx   the channel index
 * @param[in] ptr        the link list ptr
 *******************************************************************************
 */
__STATIC_FORCEINLINE void drv_gpdma_channel_set_ptr(uint8_t chan_idx, void *ptr)
{
    OM_GPDMA_CHAN_Type *gpdma_chan;

    gpdma_chan = &(OM_GPDMA->CHAN[chan_idx]);
    gpdma_chan->LL_PTR = (uint32_t)ptr;
}

/**
 *******************************************************************************
 * @brief Allocate handshake signal by gpdma id, see, This function is WEAK
 *        symbol, users can re-write this function. The default implementation allocate
 *        handshake signal from the first avaliable handshake signal.
 *
 * @param id gpdma id
 * @return handshake signal, return GPDMA_INVALID_HANDSHAKE_SIGNAL if no handshake signal available
 *******************************************************************************
 */
extern uint8_t drv_gpdma_handshake_signal_allocate(gpdma_id_t id);

/**
 *******************************************************************************
 * @brief gpdma interrupt service routine
 *
 *******************************************************************************
 */
extern void drv_gpdma_isr(void);

#ifdef __cplusplus
}
#endif

#endif   /* RTE_GPDMA */

#endif  /* __DRV_GPDMA_H */


/** @} */
