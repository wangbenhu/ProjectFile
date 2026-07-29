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
 * @brief    example for using gpdma
 * @details  example for using gpdma: transfer between memory
 * @version
 *
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


/*******************************************************************************
 * INCLUDES
 */
#include "om_driver.h"


/*******************************************************************************
 * MACROS
 */
/// GPDMA test size
#define GPDMA_TEST_SIZE    256


/*******************************************************************************
 * TYPEDEFS
 */


/*******************************************************************************
 * CONST & VARIABLES
 */
/// Source data buffer
static uint8_t src_buf[1024];

/// Destination data buffer
static uint8_t dst_buf[1024];

/// GPDMA initial configuration
static const gpdma_config_t gpdma_cfg_demo = {
    .channel_ctrl     = GPDMA_SET_CTRL(GPDMA_ADDR_CTRL_INC,  GPDMA_ADDR_CTRL_INC,
                                       GPDMA_TRANS_WIDTH_1B, GPDMA_TRANS_WIDTH_1B,
                                       GPDMA_BURST_SIZE_1T,  GPDMA_PRIORITY_LOW),
    .src_id           = GPDMA_ID_MEM,
    .dst_id           = GPDMA_ID_MEM,
    .chain_trans      = NULL,
    .chain_trans_num  = 0U,
};

/// GPDMA List Item
static gpdma_chain_trans_t chain_trans[3];


/*******************************************************************************
 * LOCAL FUNCTIONS
 */


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief example of transfering data using gpdma, move data from src_buf to dst_buf
 *
 *******************************************************************************
 */
void example_gpdma_ram2ram(void)
{
    uint8_t         chan_id;
    gpdma_config_t  gpdma_cfg;

    chan_id = drv_gpdma_channel_allocate();
    memcpy(&gpdma_cfg, &gpdma_cfg_demo, sizeof(gpdma_config_t));
    gpdma_cfg.chain_trans      = NULL;
    gpdma_cfg.chain_trans_num  = 0;
    gpdma_cfg.isr_cb           = NULL;
    gpdma_cfg.cb_param         = "ram2ram";
    drv_gpdma_channel_config(chan_id, &gpdma_cfg);

    memset(dst_buf, 0x00, GPDMA_TEST_SIZE);
    for (uint32_t i = 0x00; i < GPDMA_TEST_SIZE; i++) {
        src_buf[i] = i % 256;
    }

    drv_gpdma_channel_enable(chan_id, (uint32_t)dst_buf, (uint32_t)src_buf, GPDMA_TEST_SIZE);

    DRV_DELAY_MS(10);

    // check
    for (uint32_t i = 0x00; i < GPDMA_TEST_SIZE; i++) {
        if (src_buf[i] != dst_buf[i]) {
            om_printf("GPDMA ram2ram fail");
            OM_ASSERT(0);
        }
    }
    om_printf("DMA ram2ram pass");

    drv_gpdma_channel_release(chan_id);
}

/**
 *******************************************************************************
 * @brief example of transfer data using gpdma list
 *        1. firstly, move 32B data from src_buf to dst_buf
 *        2. move 32B data from src_buf+32 to dst_buf+32 according to chain_trans[0]
 *        3. move 32B data from src_buf+64 to dst_buf+64 according to chain_trans[1]
 *        4. finally, the rest of data is moved from src_buf+96 to dst_buf+96 according to chain_trans[2]
 *
 *******************************************************************************
 */
void example_gpdma_ram2ram_chains(void)
{
    uint8_t              chan_id;
    gpdma_config_t         gpdma_cfg;

    chan_id = drv_gpdma_channel_allocate();
    memcpy(&gpdma_cfg, &gpdma_cfg_demo, sizeof(gpdma_config_t));
    gpdma_cfg.chain_trans      = chain_trans;
    gpdma_cfg.chain_trans_num  = 3;
    gpdma_cfg.isr_cb           = NULL;
    gpdma_cfg.cb_param         = "ram2ram_chains";

    // 2. 2nd transfer 32 Byte
    chain_trans[0].src_addr   = (uint32_t) (src_buf + 32U);
    chain_trans[0].dst_addr   = (uint32_t) (dst_buf + 32U);
    chain_trans[0].size_byte = 32U;
    chain_trans[0].ll_ptr     = (void *)(&(chain_trans[1]));

    // 3. 3rd transfer 32 Byte
    chain_trans[1].src_addr   = (uint32_t)(src_buf + 32U + 32U);
    chain_trans[1].dst_addr   = (uint32_t)(dst_buf + 32U + 32U);
    chain_trans[1].size_byte = 32U;
    chain_trans[1].ll_ptr     = (void *)(&(chain_trans[2]));

    //4. finally transfer the last
    chain_trans[2].src_addr   = (uint32_t)(src_buf + 32U + 32U + 32U);
    chain_trans[2].dst_addr   = (uint32_t)(dst_buf + 32U + 32U + 32U);
    chain_trans[2].size_byte = GPDMA_TEST_SIZE - 3 * 32U;
    chain_trans[2].ll_ptr     = NULL;
    memset(dst_buf, 0x00, GPDMA_TEST_SIZE);
    for (uint32_t i = 0x00; i < GPDMA_TEST_SIZE; i++) {
        src_buf[i] = i % 256;
    }

    // 1. firstly transfer 32Byte and start GPDMA
    drv_gpdma_channel_config(chan_id, &gpdma_cfg);
    drv_gpdma_channel_enable(chan_id, (uint32_t)dst_buf, (uint32_t)src_buf, 32U);

    DRV_DELAY_MS(100);

    // 4. check
    for (uint32_t i = 0x00; i < GPDMA_TEST_SIZE; i++) {
        if (src_buf[i] != dst_buf[i]) {
            om_printf("GPDMA ram2ram chains fail");
            OM_ASSERT(0);
        }
    }
    om_printf("GPDMA ram2ram chains pass");

    drv_gpdma_channel_release(chan_id);
}


/** @} */