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
 * @brief    GPDMA driver source file
 * @details  GPDMA driver source file
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
#include "RTE_driver.h"
#if (RTE_GPDMA)
#include <stddef.h>
#include "om_device.h"
#include "om_driver.h"


/*******************************************************************************
 * MACROS
 */
#define GPDMA_CHANNEL(ch)                 (&(OM_GPDMA->CHAN[ch]))


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    gpdma_isr_callback_t   isr_cb[GPDMA_NUMBER_OF_CHANNELS];
    void                  *cb_param[GPDMA_NUMBER_OF_CHANNELS];
    uint8_t                handshake_sel[GPDMA_NUMBER_OF_CHANNELS][2];    // 0 is src handshke, 1 is dest handshake signal
    uint16_t               handshake_using;  // bit field, 1 indicate handshake signal has allocated
    uint8_t                chan_mask;
    uint8_t                busy_mask;
} gpdma_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
static gpdma_env_t gpdma_env = {
    .isr_cb  = {NULL},
    .chan_mask = 0U,
    .busy_mask = 0U,
    .handshake_using = 0U,
};

static const drv_resource_t  gpdma_resource = {
    .cap      = GPDMA_NUMBER_OF_CHANNELS,
    .reg      = OM_GPDMA,
    .irq_num  = GPDMA_IRQn,
    .irq_prio = RTE_GPDMA_IRQ_PRIORITY,
    .env      = (void *) &gpdma_env,
};


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
__WEAK uint8_t drv_gpdma_handshake_signal_allocate(gpdma_id_t id)
{
    typedef struct {
        uint16_t  gpdma_id     : 7U;
        uint16_t  flag         : 1U;
        uint16_t  sel_hs_idx   : 4U;
        uint16_t  conflict_idx : 4U;
    } hs_conflict_tab_t;

    /* define hardware handshake interface table */
    const uint32_t handshake_inf_tab[] = {
        (1U << GPDMA_ID_UART2_TX) | (1U << GPDMA_ID_SPI1_TX)  | (GPDMA_ID_SPI1_TX << 27U),   /* 0 */
        (1U << GPDMA_ID_UART2_RX) | (1U << GPDMA_ID_SPI1_RX)  | (GPDMA_ID_SPI1_RX << 27U),   /* 1 */
        (1U << GPDMA_ID_I2C0_TX)  | (1U << GPDMA_ID_UART0_TX) | (GPDMA_ID_UART0_TX << 27U),  /* 2 */
        (1U << GPDMA_ID_I2C0_RX)  | (1U << GPDMA_ID_UART0_RX) | (GPDMA_ID_UART0_RX << 27U),  /* 3 */
        (1U << GPDMA_ID_SPI0_TX)  | (1U << GPDMA_ID_UART0_TX) | (GPDMA_ID_UART0_TX << 27U),  /* 4 */
        (1U << GPDMA_ID_SPI0_RX)  | (1U << GPDMA_ID_UART0_RX) | (GPDMA_ID_UART0_RX << 27U),  /* 5 */
        (1U << GPDMA_ID_SPI1_TX)  | (1U << GPDMA_ID_UART1_TX) | (GPDMA_ID_UART1_TX << 27U),  /* 6 */
        (1U << GPDMA_ID_SPI1_RX)  | (1U << GPDMA_ID_UART1_RX) | (GPDMA_ID_UART1_RX << 27U),  /* 7 */
        (1U << GPDMA_ID_TIM0)     | (1U << GPDMA_ID_I2C1_RX)  | (GPDMA_ID_I2C1_RX << 27U),   /* 8 */
        (1U << GPDMA_ID_TIM1)     | (1U << GPDMA_ID_SPI0_RX)  | (GPDMA_ID_SPI0_RX << 27U),   /* 9 */
        (1U << GPDMA_ID_TIM2)     | (1U << GPDMA_ID_I2C1_RX)  | (GPDMA_ID_I2C1_RX << 27U),   /* 10 */
        (1U << GPDMA_ID_I2C1_TX)  | (1U << GPDMA_ID_UART1_TX) | (GPDMA_ID_UART1_TX << 27U),  /* 11 */
        (1U << GPDMA_ID_I2S_TX)   | (1U << GPDMA_ID_UART0_TX) | (GPDMA_ID_UART0_TX << 27U),  /* 12 */
        (1U << GPDMA_ID_I2S_RX)   | (1U << GPDMA_ID_UART0_RX) | (GPDMA_ID_UART0_RX << 27U),  /* 13 */
        (1U << GPDMA_ID_GPADC)    | (1U << GPDMA_ID_UART1_RX) | (GPDMA_ID_UART1_RX << 27U),  /* 14 */
        (1U << GPDMA_ID_RGB)      | (1U << GPDMA_ID_SPI0_TX)  | (GPDMA_ID_SPI0_TX << 27U),   /* 15 */
    };
    const hs_conflict_tab_t hs_conflict_tab[] = {    // digital(yangq) guarantee no conflict with allocated handshake signal
        {.gpdma_id = GPDMA_ID_I2C1_RX,  .flag = 0, .sel_hs_idx = 8, .conflict_idx = 10},
        {.gpdma_id = GPDMA_ID_SPI0_TX,  .flag = 0, .sel_hs_idx = 4, .conflict_idx = 15},
        {.gpdma_id = GPDMA_ID_SPI0_RX,  .flag = 0, .sel_hs_idx = 5, .conflict_idx = 9},
        {.gpdma_id = GPDMA_ID_SPI1_TX,  .flag = 1, .sel_hs_idx = 0, .conflict_idx = 6},
        {.gpdma_id = GPDMA_ID_SPI1_RX,  .flag = 1, .sel_hs_idx = 1, .conflict_idx = 7},
        {.gpdma_id = GPDMA_ID_UART0_TX, .flag = 0, .sel_hs_idx = 4, .conflict_idx = 12},
        {.gpdma_id = GPDMA_ID_UART0_TX, .flag = 0, .sel_hs_idx = 2, .conflict_idx = 12},
        {.gpdma_id = GPDMA_ID_UART0_TX, .flag = 0, .sel_hs_idx = 2, .conflict_idx = 4},
        {.gpdma_id = GPDMA_ID_UART0_RX, .flag = 0, .sel_hs_idx = 5, .conflict_idx = 13},
        {.gpdma_id = GPDMA_ID_UART0_RX, .flag = 0, .sel_hs_idx = 3, .conflict_idx = 13},
        {.gpdma_id = GPDMA_ID_UART0_RX, .flag = 0, .sel_hs_idx = 3, .conflict_idx = 5},
        {.gpdma_id = GPDMA_ID_UART1_TX, .flag = 0, .sel_hs_idx = 6, .conflict_idx = 11},
        {.gpdma_id = GPDMA_ID_UART1_RX, .flag = 0, .sel_hs_idx = 7, .conflict_idx = 14},
    };

    for (uint8_t i = 0; i < sizeof(handshake_inf_tab)/sizeof(handshake_inf_tab[0]); i++) {
        if (gpdma_env.handshake_using & (1U << i)) {
            continue;
        }
        if (handshake_inf_tab[i] & (1U << (uint32_t)id)) {
            OM_CRITICAL_BEGIN();
            if ((handshake_inf_tab[i] >> 27U) == (uint32_t)id) {
                OM_GPDMA->REQ_ACK_SEL |= (1U << i);
            } else {
                OM_GPDMA->REQ_ACK_SEL &= ~(1U << i);
            }
            gpdma_env.handshake_using |= (1U << i);
            // fixed conflict for handshake
            for (uint32_t j=0; j<sizeof(hs_conflict_tab)/sizeof(hs_conflict_tab[0]); j++) {
                if ((hs_conflict_tab[j].gpdma_id == id) && (hs_conflict_tab[j].sel_hs_idx == i)) {
                    uint16_t hs_using_conflict;
                    hs_using_conflict = 1U << hs_conflict_tab[j].conflict_idx;
                    OM_ASSERT_WHILE(gpdma_env.handshake_using & hs_using_conflict, (OM_GPDMA->REQ_ACK_SEL & hs_using_conflict) == (hs_conflict_tab[j].flag ? hs_using_conflict : 0U));  // check using
                    if (hs_conflict_tab[j].flag == 0U) {
                        OM_GPDMA->REQ_ACK_SEL &= ~hs_using_conflict;
                    } else {
                        OM_GPDMA->REQ_ACK_SEL |= hs_using_conflict;
                    }
                }
            }
            OM_CRITICAL_END();
            return i;
        }
    }

    return GPDMA_INVALID_HANDSHAKE_SIGNAL;
}

uint8_t drv_gpdma_channel_allocate(void)
{
    uint8_t chan_idx;

    OM_CRITICAL_BEGIN();
    for (chan_idx = 0U; chan_idx < GPDMA_NUMBER_OF_CHANNELS; chan_idx++) {
        if (!(gpdma_env.chan_mask & (1 << chan_idx))) {
            if (gpdma_env.chan_mask == 0) {       // first allocate
                DRV_RCC_RESET(RCC_CLK_GPDMA);
                /* Reset dma core, disable all channels */
                OM_GPDMA->CTRL |= GPDMA_CTRL_RESET_MASK;
                // Clear and Enable DMA IRQ
                NVIC_SetPriority(gpdma_resource.irq_num, gpdma_resource.irq_prio);
                NVIC_ClearPendingIRQ(gpdma_resource.irq_num);
                NVIC_EnableIRQ(gpdma_resource.irq_num);
            }
            gpdma_env.chan_mask |= (1U << chan_idx);
            gpdma_env.busy_mask &= ~(1U << chan_idx);
            gpdma_env.handshake_sel[chan_idx][0] = GPDMA_INVALID_HANDSHAKE_SIGNAL;
            gpdma_env.handshake_sel[chan_idx][1] = GPDMA_INVALID_HANDSHAKE_SIGNAL;
            break;
        }
    }
    OM_CRITICAL_END();

    return chan_idx;
}

void drv_gpdma_channel_release(uint8_t chan_idx)
{
    OM_ASSERT(chan_idx < GPDMA_NUMBER_OF_CHANNELS);

    OM_CRITICAL_BEGIN();
    gpdma_env.busy_mask &= ~(1U << chan_idx);
    gpdma_env.chan_mask &= ~(1U << chan_idx);
    if (gpdma_env.handshake_sel[chan_idx][0] != GPDMA_INVALID_HANDSHAKE_SIGNAL) {
        gpdma_env.handshake_using &= ~(1U << gpdma_env.handshake_sel[chan_idx][0]);
        gpdma_env.handshake_sel[chan_idx][0] = GPDMA_INVALID_HANDSHAKE_SIGNAL;
    }
    if (gpdma_env.handshake_sel[chan_idx][1] != GPDMA_INVALID_HANDSHAKE_SIGNAL) {
        gpdma_env.handshake_using &= ~(1U << gpdma_env.handshake_sel[chan_idx][1]);
        gpdma_env.handshake_sel[chan_idx][1] = GPDMA_INVALID_HANDSHAKE_SIGNAL;
    }
    if (gpdma_env.chan_mask == 0) {
        DRV_RCC_CLOCK_ENABLE(RCC_CLK_GPDMA, 0U);
    }
    OM_CRITICAL_END();
}

om_error_t drv_gpdma_channel_config(uint8_t chan_idx, const gpdma_config_t *config)
{
    OM_GPDMA_CHAN_Type *gpdma_ch;
    uint32_t channel_ctrl = config->channel_ctrl;
    uint8_t src_width;
    uint8_t dst_width;
    uint8_t src_burst_size;
    uint8_t handshake_signal;

    (void)dst_width;
    (void)src_burst_size;
    src_width = 1 << register_get(&channel_ctrl, MASK_POS(GPDMA_CHAN_CTRL_SRCWIDTH));
    dst_width = 1 << register_get(&channel_ctrl, MASK_POS(GPDMA_CHAN_CTRL_DSTWIDTH));
    src_burst_size = 1 << register_get(&channel_ctrl, MASK_POS(GPDMA_CHAN_CTRL_SRCBURSTSIZE));

    OM_ASSERT(OM_IS_ALIGN(src_width * src_burst_size, dst_width)); // burst_bytes must be aligned to dst_width
    OM_ASSERT(chan_idx < GPDMA_NUMBER_OF_CHANNELS);

    if (gpdma_env.busy_mask & (1U << chan_idx)) {
        return OM_ERROR_BUSY;
    }

    OM_ASSERT((channel_ctrl & (GPDMA_CHAN_CTRL_SRCMODE_MASK | GPDMA_CHAN_CTRL_DSTMODE_MASK)) == 0U);
    if (config->src_id != GPDMA_ID_MEM) {
        OM_ASSERT(config->dst_id == GPDMA_ID_MEM);
        if (gpdma_env.handshake_sel[chan_idx][0] == GPDMA_INVALID_HANDSHAKE_SIGNAL) {
            handshake_signal = drv_gpdma_handshake_signal_allocate(config->src_id);
            if (handshake_signal != GPDMA_INVALID_HANDSHAKE_SIGNAL) {
                gpdma_env.handshake_sel[chan_idx][0] = handshake_signal;
            } else {
                return OM_ERROR_PARAMETER;
            }
        }
        register_set(&channel_ctrl, MASK_2REG(GPDMA_CHAN_CTRL_SRCMODE,   1,
                                              GPDMA_CHAN_CTRL_SRCREQSEL, gpdma_env.handshake_sel[chan_idx][0]));
    }
    if (config->dst_id != GPDMA_ID_MEM) {
        OM_ASSERT(config->src_id == GPDMA_ID_MEM);
        if (gpdma_env.handshake_sel[chan_idx][1] == GPDMA_INVALID_HANDSHAKE_SIGNAL) {
            handshake_signal = drv_gpdma_handshake_signal_allocate(config->dst_id);
            if (handshake_signal != GPDMA_INVALID_HANDSHAKE_SIGNAL) {
                gpdma_env.handshake_sel[chan_idx][1] = handshake_signal;
            } else {
                return OM_ERROR_PARAMETER;
            }
        }
        register_set(&channel_ctrl, MASK_2REG(GPDMA_CHAN_CTRL_DSTMODE,   1,
                                              GPDMA_CHAN_CTRL_DSTREQSEL, gpdma_env.handshake_sel[chan_idx][1]));
    }

    // Save callback pointer
    gpdma_env.isr_cb[chan_idx] = config->isr_cb;
    gpdma_env.cb_param[chan_idx] = config->cb_param;
    gpdma_ch = GPDMA_CHANNEL(chan_idx);
    gpdma_ch->CTRL = channel_ctrl;

    // Link list partital config
    if (config->chain_trans_num) {
        gpdma_ch->LL_PTR = (uint32_t)(config->chain_trans);
        for (uint8_t i = 0; i < config->chain_trans_num; i++) {
            OM_ASSERT(OM_IS_ALIGN(config->chain_trans[i].size_byte, src_width)); // total trans bytes must be aligned to src,dst width
            OM_ASSERT(OM_IS_ALIGN(config->chain_trans[i].size_byte, dst_width));
            OM_ASSERT(OM_IS_ALIGN(config->chain_trans[i].src_addr, src_width));
            OM_ASSERT(OM_IS_ALIGN(config->chain_trans[i].dst_addr, dst_width));

            config->chain_trans[i].trans_size = config->chain_trans[i].size_byte / src_width;
            config->chain_trans[i].ctrl = channel_ctrl | GPDMA_CHAN_CTRL_ENABLE_MASK;  // link list item's enable bit does not affect
        }
    } else {
        gpdma_ch->LL_PTR = 0U;
    }

    return OM_ERROR_OK;
}

om_error_t drv_gpdma_channel_enable(uint8_t chan_idx, uint32_t dst_addr, uint32_t src_addr, uint32_t total_trans_byte)
{
    OM_GPDMA_CHAN_Type *gpdma_ch;
    uint8_t src_width;
    uint8_t dst_width;
    uint8_t src_burst_size;

    (void)dst_width;
    (void)src_burst_size;
    // Check if channel is valid
    OM_ASSERT(chan_idx < GPDMA_NUMBER_OF_CHANNELS);

    // Check if channel is busy
    if (gpdma_env.busy_mask & (1U << chan_idx)) {
        return OM_ERROR_BUSY;
    }

    // Check if channel is allocated
    if (gpdma_env.chan_mask & (1U << chan_idx)) {
        OM_CRITICAL_BEGIN();
        gpdma_env.busy_mask |= (1U << chan_idx);
        OM_CRITICAL_END();
    } else {
        return OM_ERROR_RESOURCES;
    }

    gpdma_ch = GPDMA_CHANNEL(chan_idx);

    src_width      = 1 << register_get(&gpdma_ch->CTRL, MASK_POS(GPDMA_CHAN_CTRL_SRCWIDTH));
    dst_width      = 1 << register_get(&gpdma_ch->CTRL, MASK_POS(GPDMA_CHAN_CTRL_DSTWIDTH));
    src_burst_size = 1 << register_get(&gpdma_ch->CTRL, MASK_POS(GPDMA_CHAN_CTRL_SRCBURSTSIZE));

    // burst size bytes check
    OM_ASSERT(OM_IS_ALIGN(src_width * src_burst_size, dst_width));
    // sources address must be aligned to the source transfer size
    // destination address must be aligned to the destination trfansfer size.
    OM_ASSERT(OM_IS_ALIGN(src_addr, src_width));
    OM_ASSERT(OM_IS_ALIGN(dst_addr, dst_width));
    // transfer size check
    OM_ASSERT(total_trans_byte != 0U);
    OM_ASSERT(OM_IS_ALIGN(total_trans_byte, src_width));
    OM_ASSERT(OM_IS_ALIGN(total_trans_byte, dst_width));
    OM_ASSERT(total_trans_byte / src_width <= GPDMA_CHAN_MAX_TRANS_SIZE);

    gpdma_ch->SRC_ADDR   = GPDMA_ADDR_CONVERT(src_addr);
    gpdma_ch->DST_ADDR   = GPDMA_ADDR_CONVERT(dst_addr);
    gpdma_ch->TRANS_SIZE = total_trans_byte / src_width;

    gpdma_ch->CTRL |= GPDMA_CHAN_CTRL_ENABLE_MASK;

    return OM_ERROR_OK;
}

om_error_t drv_gpdma_channel_disable(uint8_t chan_idx)
{
    // Check if channel is valid
    OM_ASSERT(chan_idx < GPDMA_NUMBER_OF_CHANNELS);

    // abort current transfer
    OM_GPDMA->CHAN_ABORT          = (1U << chan_idx);

    return OM_ERROR_OK;
}

uint8_t drv_gpdma_channel_is_busy(uint8_t chan_idx)
{
    uint8_t is_busy;

    if (chan_idx < GPDMA_NUMBER_OF_CHANNELS) {
        is_busy = (gpdma_env.busy_mask & (1U << chan_idx)) ? 1U : 0U;
    } else {
        is_busy = 0;
    }

    return is_busy;
}

uint32_t drv_gpdma_channel_get_left_count(uint8_t chan_idx)
{
    OM_GPDMA_CHAN_Type *gpdma_ch;
    uint8_t  src_width;

    OM_ASSERT(chan_idx < GPDMA_NUMBER_OF_CHANNELS);

    gpdma_ch = GPDMA_CHANNEL(chan_idx);
    src_width = 1 << register_get(&gpdma_ch->CTRL, MASK_POS(GPDMA_CHAN_CTRL_SRCWIDTH));

    return (gpdma_ch->TRANS_SIZE * src_width);
}

void drv_gpdma_isr(void)
{
    uint32_t             ch;
    OM_GPDMA_CHAN_Type  *gpdma_ch;
    drv_event_t          event;
    uint32_t             int_status;

    while (OM_GPDMA->INT_STATUS) {
        int_status = OM_GPDMA->INT_STATUS;
        for (ch = 0; ch < GPDMA_NUMBER_OF_CHANNELS; ch++) {
            if (int_status & GPDMA_INT_STATUS_ALL(ch)) {
                event = DRV_EVENT_COMMON_NONE;
                gpdma_ch = GPDMA_CHANNEL(ch);
                if (int_status & GPDMA_INT_STATUS_TC_MASK(ch)) {  // Terminal count request interrupt
                    // Clear interrupt flag
                    OM_GPDMA->INT_STATUS = GPDMA_INT_STATUS_TC_MASK(ch);
                    if (!OM_GPDMA->LLP_SHADOW[ch]) {
                        gpdma_env.busy_mask &= ~(1U << ch);
                    }
                    if (OM_GPDMA->LLP_SHADOW[ch] && (gpdma_ch->CTRL & GPDMA_CHAN_CTRL_ENABLE_MASK) == 0x0) {
                        gpdma_env.busy_mask &= ~(1U << ch);
                    }
                    event |= DRV_GPDMA_EVENT_TERMINAL_COUNT_REQUEST;
                }
                if (int_status & GPDMA_INT_STATUS_ERROR_MASK(ch)) {  // Error interrupt
                    gpdma_ch->CTRL &= ~GPDMA_CHAN_CTRL_ENABLE_MASK;
                    // Clear interrupt flag
                    OM_GPDMA->INT_STATUS = GPDMA_INT_STATUS_ERROR_MASK(ch);
                    event |= DRV_GPDMA_EVENT_ERROR;
                    gpdma_env.busy_mask &= ~(1U << ch);
                }
                if (int_status & GPDMA_INT_STATUS_ABORT_MASK(ch)) {  // abort interrupt
                    gpdma_ch->CTRL &= ~GPDMA_CHAN_CTRL_ENABLE_MASK;
                    // Clear interrupt flag
                    OM_GPDMA->INT_STATUS = GPDMA_INT_STATUS_ABORT_MASK(ch);
                    event |= DRV_GPDMA_EVENT_ABORT;
                    gpdma_env.busy_mask &= ~(1U << ch);
                }

                // Signal Event
                if (gpdma_env.isr_cb[ch]) {
                    gpdma_env.isr_cb[ch](gpdma_env.cb_param[ch], event, (void *)(OM_GPDMA->LLP_SHADOW[ch]));
                }
            }
        }
    }
}

#endif /* RTE_GPDMA */

/** @} */
