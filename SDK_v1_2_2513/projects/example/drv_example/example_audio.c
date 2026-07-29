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
 * @brief    example for using audio
 * @details  example for using audio:
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

#define PAD_I2C0_SCL            20
#define PAD_I2C0_SDA            21
#define MUX_I2C0_SCL            PINMUX_PAD20_I2C0_SCK_CFG
#define MUX_I2C0_SDA            PINMUX_PAD21_I2C0_SDA_CFG

/// Audio Ring Buffer for RX Data Use, 32KB, 8K[2s]--16k[1s]--32k[0.5s]
#define LEN_RING_BUF            32 * 1024

/// I2S DMA chain number
#define RX_BUF_CHAIN_NUM        RTE_I2S_GPDMA_LLP_CHAIN_NUM
#define TX_BUF_CHAIN_NUM        RTE_I2S_GPDMA_LLP_CHAIN_NUM

/// I2S RX blcok size and number
#define RX_BUF_BLOCK_SIZE       512
#define RX_BUF_BLOCK_NUM        (LEN_RING_BUF / RX_BUF_BLOCK_SIZE)

/// I2S TX blcok size and number
#define TX_BUF_BLOCK_SIZE       512
#define TX_BUF_BLOCK_NUM        -1U

/*******************************************************************************
 * TYPEDEFS
 */


/*******************************************************************************
 * CONST & VARIABLES
 */
static const pin_config_t pin_cfg [] = {
    {PAD_I2S_EXT_CLK, {MUX_I2S_EXT_CLK}, PMU_PIN_MODE_PP,    PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_I2S_SDI,     {MUX_I2S_SDI},     PMU_PIN_MODE_FLOAT, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_I2S_SDO,     {MUX_I2S_SDO},     PMU_PIN_MODE_PP,    PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_I2S_WS,      {MUX_I2S_WS},      PMU_PIN_MODE_PP,    PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_I2S_SCLK,    {MUX_I2S_SCLK},    PMU_PIN_MODE_PP,    PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_I2C0_SCL,    {MUX_I2C0_SCL},    PMU_PIN_MODE_OD,    PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_I2C0_SDA,    {MUX_I2C0_SDA},    PMU_PIN_MODE_OD,    PMU_PIN_DRIVER_CURRENT_NORMAL},
};

/// RX ring buffer
static uint8_t *p_ring_buf = NULL;
/// RX ring buffer block index
static uint32_t rx_block_idx = 0;
/// TX ring buffer block index
static uint32_t tx_block_idx = 0;

/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static void i2s_external_tx_loop_isr_cb(void *i2s_reg, drv_event_t event, void *addr, void *size)
{
    drv_i2s_write_dma(OM_I2S, p_ring_buf + tx_block_idx * TX_BUF_BLOCK_SIZE, TX_BUF_BLOCK_SIZE);
    tx_block_idx = (tx_block_idx + 1) % RX_BUF_BLOCK_NUM;
}

static void i2s_external_rx_loop_isr_cb(void *i2s_reg, drv_event_t event, void *addr, void *size)
{
    drv_i2s_read_dma(OM_I2S, p_ring_buf + rx_block_idx * RX_BUF_BLOCK_SIZE, RX_BUF_BLOCK_SIZE);
    rx_block_idx = (rx_block_idx + 1) % RX_BUF_BLOCK_NUM;
}
/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief example of using audio
 *
 *******************************************************************************
 */
void example_audio(void)
{
    drv_pin_init(pin_cfg, sizeof(pin_cfg) / sizeof(pin_cfg[0]));

    /// Allocate RX fifo buffer
    p_ring_buf = om_mem_malloc(0, LEN_RING_BUF);
    OM_ASSERT(p_ring_buf);

    audio_play_config_t audio_play_config = {
        .channel        = I2S_CHN_MONO,
        .bit_width      = I2S_BW_16BIT,
        .sample_rate    = I2S_SR_8K,
        .volume         = 0x7F /* MAX(6dB), 1dB/step */
    };

    audio_record_config_t audio_rec_config = {
        .channel        = I2S_CHN_MONO,
        .bit_width      = I2S_BW_16BIT,
        .sample_rate    = I2S_SR_8K,
        .volume         = 0xC3 /* 0dB, Max 30dB(0xFF), 0.5dB/step */
    };

    i2s_config_t i2s_config = {
        .role           = I2S_ROLE_SLAVE,
        .dir            = I2S_DIR_RX_TX,
        .channel        = I2S_CHN_MONO,
        .bit_width      = I2S_BW_16BIT,
        .sample_rate    = I2S_SR_8K
    };

    /// Init Audio codec and start
    drv_audio_init();
    drv_audio_record_start(&audio_rec_config);
    drv_audio_play_start(&audio_play_config);

    /// Init I2S
    drv_i2s_init(OM_I2S, &i2s_config);
    drv_i2s_rx_register_isr_callback(OM_I2S, i2s_external_rx_loop_isr_cb);
    drv_i2s_tx_register_isr_callback(OM_I2S, i2s_external_tx_loop_isr_cb);

    /// Create chain and start
    for(rx_block_idx = 0; rx_block_idx < RX_BUF_CHAIN_NUM; rx_block_idx++) {
        drv_i2s_read_dma(OM_I2S, p_ring_buf + rx_block_idx * RX_BUF_BLOCK_SIZE, RX_BUF_BLOCK_SIZE);
    }

    /// FIXME: Delay 200ms for rx first and play bebind
    drv_dwt_delay_ms(200);

    /// Create chain and start
    for(tx_block_idx = 0; tx_block_idx < TX_BUF_CHAIN_NUM; tx_block_idx++) {
        drv_i2s_write_dma(OM_I2S, p_ring_buf + tx_block_idx * TX_BUF_BLOCK_SIZE, TX_BUF_BLOCK_SIZE);
    }

    drv_dwt_delay_ms(10000);

    /// Uninit Audio
    drv_audio_record_stop();
    drv_audio_play_stop();
    drv_audio_uninit();

    /// Uninit I2S
    drv_i2s_uninit(OM_I2S);
    om_mem_free(0, p_ring_buf);
}



/** @} */