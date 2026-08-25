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
 * @brief    example for using i2s
 * @details  example for using i2s:
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
#define PAD_I2S_EXT_CLK         11
#define PAD_I2S_SDI             12
#define PAD_I2S_SDO             13
#define PAD_I2S_WS              14
#define PAD_I2S_SCLK            15
#define MUX_I2S_EXT_CLK         PINMUX_PAD11_AUDIO_EXT_CLK_CFG
#define MUX_I2S_SDI             PINMUX_PAD12_I2S_SDI_CFG
#define MUX_I2S_SDO             PINMUX_PAD13_I2S_SDO_CFG
#define MUX_I2S_WS              PINMUX_PAD14_I2S_WS_CFG
#define MUX_I2S_SCLK            PINMUX_PAD15_I2S_SCLK_CFG

#define RTX_BUF_TOTAL_SIZE      1024

#define RTX_BUF_BLOCK_NUM       sizeof(uint32_t)
#define RTX_BUF_BLOCK_SIZE      (RTX_BUF_TOTAL_SIZE / RTX_BUF_BLOCK_NUM)

#define RTX_BUF_CHAIN_NUM       RTE_I2S_GPDMA_LLP_CHAIN_NUM
#define RTX_BUF_CHAIN_SIZE      (RTX_BUF_TOTAL_SIZE / RTX_BUF_CHAIN_NUM)

/*******************************************************************************
 * TYPEDEFS
 */


/*******************************************************************************
 * CONST & VARIABLES
 */
static uint32_t *p_i2s_tx_buf = NULL;
static uint32_t *p_i2s_rx_buf = NULL;

static const pin_config_t pin_cfg [] = {
    {PAD_I2S_EXT_CLK, {MUX_I2S_EXT_CLK}, PMU_PIN_MODE_PP,    PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_I2S_SDI,     {MUX_I2S_SDI},     PMU_PIN_MODE_FLOAT, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_I2S_SDO,     {MUX_I2S_SDO},     PMU_PIN_MODE_PP,    PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_I2S_WS,      {MUX_I2S_WS},      PMU_PIN_MODE_PP,    PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_I2S_SCLK,    {MUX_I2S_SCLK},    PMU_PIN_MODE_PP,    PMU_PIN_DRIVER_CURRENT_NORMAL},
};

/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static void i2s_tx_isr_cb(void *i2s_reg, drv_event_t event, void *addr, void *size)
{
    drv_i2s_write_dma(OM_I2S, (uint8_t *)addr, (uint32_t)size);
}

static void i2s_rx_isr_cb(void *i2s_reg, drv_event_t event, void *addr, void *size)
{
    drv_i2s_read_dma(OM_I2S, (uint8_t *)addr, (uint32_t)size);
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief example of using i2s
 *
 *******************************************************************************
 */
void example_i2s(void)
{
    p_i2s_tx_buf = om_mem_malloc(0, RTX_BUF_TOTAL_SIZE);
    OM_ASSERT(p_i2s_tx_buf);
    p_i2s_rx_buf = om_mem_malloc(0, RTX_BUF_TOTAL_SIZE);
    OM_ASSERT(p_i2s_rx_buf);

    /// Set TX buffer initial value
    for(int i = 0; i < RTX_BUF_TOTAL_SIZE; i++) {
        ((uint8_t *)p_i2s_tx_buf)[i] = i;
    }

    drv_pin_init(pin_cfg, sizeof(pin_cfg) / sizeof(pin_cfg[0]));

    /// Init I2S and i2s_config
    i2s_config_t i2s_config = {
        .role           = I2S_ROLE_MASTER,
        .dir            = I2S_DIR_RX_TX,
        .channel        = I2S_CHN_MONO,
        .bit_width      = I2S_BW_16BIT,
        .sample_rate    = I2S_SR_8K,
    };
    drv_i2s_init(OM_I2S, &i2s_config);
    drv_i2s_tx_register_isr_callback(OM_I2S, i2s_tx_isr_cb);
    drv_i2s_rx_register_isr_callback(OM_I2S, i2s_rx_isr_cb);

    /// Clear RX buffer
    memset(p_i2s_rx_buf, 0, RTX_BUF_TOTAL_SIZE);

    /// Create TX and RX DMA list, start transmission
    for(int i = 0; i < RTX_BUF_CHAIN_NUM; i++) {
        drv_i2s_write_dma(OM_I2S, (uint8_t *)p_i2s_tx_buf + i * RTX_BUF_CHAIN_SIZE, RTX_BUF_CHAIN_SIZE);
    }

    for(int i = 0; i < RTX_BUF_CHAIN_NUM; i++) {
        drv_i2s_read_dma (OM_I2S, (uint8_t *)p_i2s_rx_buf + i * RTX_BUF_CHAIN_SIZE, RTX_BUF_CHAIN_SIZE);
    }

    /// Sleep at least 80ms for receive complete
    drv_dwt_delay_ms(80);

    /// Uninit I2S
    drv_i2s_uninit(OM_I2S);

    /// Free TX and RX buffer
    om_mem_free(0, p_i2s_tx_buf);
    om_mem_free(0, p_i2s_rx_buf);
}



/** @} */