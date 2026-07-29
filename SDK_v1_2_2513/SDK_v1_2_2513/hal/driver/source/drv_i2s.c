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
 * @brief    I2S driver source file
 * @details  I2S driver source file
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
#if (RTE_I2S)
#include "om_device.h"
#include "om_driver.h"

#if (!RTE_GPDMA)
#error I2S strong dependence on GPDMA !!
#endif


/*******************************************************************************
 * DEBUG DEFINES
 */
/// Debug I2S assert
#define I2S_ASSERT_EN           (1)

#if (I2S_ASSERT_EN)
#define I2S_ASSERT(x)           OM_ASSERT(x);
#else
#define I2S_ASSERT(x)
#endif /* I2S_ASSERT_EN */


/*******************************************************************************
 * TYPEDEFS
 */
/// I2S environment structure
typedef struct {
    /// Common: I2S configuration
    i2s_config_t                config;

    /// TX: Block handled finished callback
    drv_isr_callback_t          tx_isr_cb;
    /// TX: Chains node environment pointer
    gpdma_chain_trans_t         tx_chain_env[RTE_I2S_GPDMA_LLP_CHAIN_NUM];
    /// TX: Chains next node write position
    uint8_t                     tx_chain_wpos;
    /// TX: Chains current node read position
    uint8_t                     tx_chain_rpos;
    /// TX: Occupied DMA channel index
    uint8_t                     tx_gpdma_idx;

    /// RX: Block handled finished callback
    drv_isr_callback_t          rx_isr_cb;
    /// RX: Chains node environment pointer
    gpdma_chain_trans_t         rx_chain_env[RTE_I2S_GPDMA_LLP_CHAIN_NUM];
    /// RX: Chains next node write position
    uint8_t                     rx_chain_wpos;
    /// RX: Chains current node read position
    uint8_t                     rx_chain_rpos;
    /// RX: Occupied DMA channel index
    uint8_t                     rx_gpdma_idx;
} i2s_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
/// I2S environment variable define
static i2s_env_t i2s_env = {
    .config = {
        .role                   = I2S_ROLE_INVALID,
        .dir                    = I2S_DIR_RX,
        .channel                = I2S_CHN_MONO,
        .bit_width              = I2S_BW_16BIT,
        .sample_rate            = I2S_SR_8K
    },
    .tx_isr_cb                  = NULL,
    .tx_chain_env               = {{0}},
    .tx_chain_wpos              = 0,
    .tx_chain_rpos              = 0,
    .tx_gpdma_idx               = GPDMA_NUMBER_OF_CHANNELS,
    .rx_isr_cb                  = NULL,
    .rx_chain_env               = {{0}},
    .rx_chain_wpos              = 0,
    .rx_chain_rpos              = 0,
    .rx_gpdma_idx               = GPDMA_NUMBER_OF_CHANNELS,
};

/// I2S resource variable define
static const drv_resource_t i2s_resource = {
    .cap                        = 0,    /* Not present */
    .reg                        = (void *)OM_I2S,
    .env                        = (void *)&i2s_env,
    .irq_num                    = I2S_IRQn,
    .irq_prio                   = RTE_I2S_IRQ_PRIORITY,

    .gpdma_tx = {
        .id                     = GPDMA_ID_I2S_TX,
        .prio                   = GPDMA_PRIORITY_LOW,
    },

    .gpdma_rx = {
        .id                     = GPDMA_ID_I2S_RX,
        .prio                   = GPDMA_PRIORITY_LOW,
    }
};


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static void i2s_enable_daif_16m(i2s_role_t role)
{
    if(role == I2S_ROLE_SLAVE) {
        return;
    }

#if (!RTE_I2S_USE_EXTERNAL_MCLK) && (RTE_AUDIO_USE_EXTERNAL)
    drv_pmu_ana_enable(true, PMU_ANA_I2S);

    /// Reset Audio module and Enable clock
    DRV_RCC_RESET(RCC_CLK_AUDIO);

    /// Poweron Audio IREF use for Audio 16M clock
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_PD_IREF, 0));

    // I2S_SDI and I2S_SDO connecnt to GPIO
    register_set(&OM_AUDIO->CODEC_CLK_CTRL_1, MASK_1REG(AU_CLK_I2S_CONN_CTRL, 0));

    /// LDO/BIAS/ADC/PGA control by reg
    register_set(&OM_AUDIO->CODEC_ANA_PWR_1, MASK_2REG(AU_ANA_PWR_LDO_CTRL, 1, AU_ANA_PWR_ADC_CTRL, 1));
    /// Power on LDO and BIAS
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_PD_CLK, 0));
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_PD_CORE, 0));
    DRV_DELAY_US(5);
#endif
}

static void i2s_disable_daif_16m(void)
{
#if (!RTE_I2S_USE_EXTERNAL_MCLK) && (RTE_AUDIO_USE_EXTERNAL)
    // Close Audio CLOCK 16M
    drv_pmu_ana_enable(false, PMU_ANA_I2S);

    /// Powerdown Audio IREF use for Audio 16M clock
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_PD_IREF, 1));

    /// Power off LDO and BIAS
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_PD_CLK, 1));
    register_set(&OM_AUDIO->CODEC_ANA_CTRL_1, MASK_1REG(AU_ANA_PD_CORE, 1));
    /// LDO/BIAS/ADC/PGA control by reg
    register_set(&OM_AUDIO->CODEC_ANA_PWR_1, MASK_2REG(AU_ANA_PWR_LDO_CTRL, 0, AU_ANA_PWR_ADC_CTRL, 0));

    /// Disable Audio Clock
    DRV_RCC_CLOCK_ENABLE(RCC_CLK_AUDIO, 0);
#endif
}

#if RTE_AUDIO_USE_EXTERNAL
static void i2s_set_blck_ws(i2s_sr_t sr, i2s_bw_t bit_width)
{
    uint8_t is12m, coeff, odd, high;
    uint32_t bclk;

    bclk = 2 * sr * bit_width;

    // odd   = (bclk / sr % 2) ? 1 : 0
    odd = (bclk / sr % 2) ? 1 : 0;
    // high  = round(bclk / sr / 2)
    high = bclk / sr / 2;

    // coeff = mclk / bclk
    if(sr == I2S_SR_8K || sr == I2S_SR_16K || sr == I2S_SR_32K) {
        coeff = 16000000 / bclk;
        is12m = 0;
    } else {
        coeff = 12000000 / bclk;
        is12m = 1;
    }

    /// Clock Source 48M
    if(is12m) {
        drv_pmu_syspll_power_enable(1);
    }

    /// Select I2S clock
#if !RTE_I2S_USE_EXTERNAL_MCLK
    register_set(&OM_CPM->I2S_CFG, MASK_1REG(CPM_I2S_CFG_MS_SRC_SEL, is12m));       /* I2S MCLK 12M or 16M */
#else
    register_set(&OM_CPM->I2S_CFG, MASK_1REG(CPM_I2S_CFG_MS_SRC_SEL, 3));           /* I2S MCLK external */
#endif

    /// Calculate blck and ws
    register_set(&OM_CPM->I2S_CFG, MASK_3REG(CPM_I2S_CFG_MST_DIV_COEFF,   coeff,    /* BCLK = I2S clock / coeff */
                 CPM_I2S_CFG_MST_ODD,      odd,                                     /* odd  = (BCLK/Rate % 2) ? 1 : 0 */
                 CPM_I2S_CFG_MST_HIGH_NUM, high));                                  /* high = round(BCLK / Rate / 2) */
}
#endif /* RTE_AUDIO_USE_EXTERNAL */

static void i2s_set_ws_bclk(i2s_config_t *config)
{
    // Enable I2S module
    register_set1(&OM_I2S->IER, I2S_IER_IEN_MASK);

#if (RTE_AUDIO_USE_INTERNAL)
    /// When use internal Codec, I2S can only work as master role
    I2S_ASSERT(I2S_ROLE_MASTER == config->role);

    /// When use internal Codec, I2S can only work at 8K,16K,32K samplerate
    I2S_ASSERT(I2S_SR_8K  == config->sample_rate ||
               I2S_SR_16K == config->sample_rate ||
               I2S_SR_32K == config->sample_rate);

    /// When use internal Codec, I2S can only work at RX direction
    I2S_ASSERT(I2S_DIR_RX == config->dir);

    /// When use internal Codec, I2S can only work at 16bits of bit width
    I2S_ASSERT(I2S_BW_16BIT == config->bit_width);

    /// Master clock generation enable, select internal clock
    register_set(&OM_CPM->I2S_CFG, MASK_2REG(CPM_I2S_CFG_MST_EN, 1, CPM_I2S_CFG_MS_SEL, 1));

    /// Select I2S clock 16M
    register_set(&OM_CPM->I2S_CFG, MASK_1REG(CPM_I2S_CFG_MS_SRC_SEL, 0));

    /// Calculate blck and ws, bclk = 16M/div_coeff, odd  = (BCLK/Rate % 2) ? 1 : 0, high = round(BCLK / Rate / 2)
    switch (config->sample_rate) {
    case I2S_SR_8K:
        /// BCLK = 1M, ODD = 1, HIGH_NUM = 0x3F
        register_set(&OM_CPM->I2S_CFG, MASK_3REG(CPM_I2S_CFG_MST_DIV_COEFF, 16, CPM_I2S_CFG_MST_ODD, 1, CPM_I2S_CFG_MST_HIGH_NUM,  0x3f));
        break;

    case I2S_SR_16K:
        /// BCLK = 2M, ODD = 1, HIGH_NUM = 0x3F
        register_set(&OM_CPM->I2S_CFG, MASK_3REG(CPM_I2S_CFG_MST_DIV_COEFF, 8, CPM_I2S_CFG_MST_ODD, 1, CPM_I2S_CFG_MST_HIGH_NUM,  0x3f));
        break;

    case I2S_SR_32K:
        /// BCLK = 3.2M, ODD = 0, HIGH_NUM = 0x32
        register_set(&OM_CPM->I2S_CFG, MASK_3REG(CPM_I2S_CFG_MST_DIV_COEFF, 5, CPM_I2S_CFG_MST_ODD, 0, CPM_I2S_CFG_MST_HIGH_NUM,  0x32));
        break;

    default:
        I2S_ASSERT(0);
        break;
    }

#else /* RTE_AUDIO_USE_INTERNAL */

    if(I2S_ROLE_MASTER == config->role) {

        /// Master clock generation enable, Make as master role
        register_set(&OM_CPM->I2S_CFG, MASK_2REG(CPM_I2S_CFG_MST_EN, 1, CPM_I2S_CFG_MS_SEL, 1));

        /// Set sample rate divider
        //i2s_set_sr_reg(config->sample_rate);
        i2s_set_blck_ws(config->sample_rate, config->bit_width);

    } else {
        /// Master clock generation disable, Make as slave role
        register_set(&OM_CPM->I2S_CFG, MASK_2REG(CPM_I2S_CFG_MST_EN, 0, CPM_I2S_CFG_MS_SEL, 0));
    }

#endif /* RTE_AUDIO_USE_INTERNAL */
}

static void i2s_gpdma_tx_isr_cb(void *p_resource, drv_event_t event, gpdma_chain_trans_t *next_chain)
{
    i2s_env_t *env;
    volatile uint32_t addr = 0;
    volatile gpdma_trans_width_t dma_width;

    I2S_ASSERT((uint32_t)p_resource == (uint32_t)&i2s_resource);

    /// Only this event type can handle continue
    if(!(DRV_GPDMA_EVENT_TERMINAL_COUNT_REQUEST & event)) {
        return;
    }

    env = (i2s_env_t *)i2s_resource.env;

    /// Capacity check
    if(!(env->config.dir & I2S_DIR_TX)) {
        I2S_ASSERT(0);
        return;
    }
    if(env->config.role == I2S_ROLE_INVALID) {
        return;
    }

    /// DMA transmit width
    dma_width = (env->config.bit_width > I2S_BW_16BIT) ? GPDMA_TRANS_WIDTH_4B : GPDMA_TRANS_WIDTH_2B;

    /// Obtain consumed node address
    addr = env->tx_chain_env[env->tx_chain_rpos].src_addr;
    if(0 == addr) {
        return;
    }

    /// Mark data block has transfer completed
    env->tx_chain_env[env->tx_chain_rpos].src_addr = 0;
    env->tx_chain_env[env->tx_chain_rpos].ll_ptr   = NULL;

    /// Make list work up when node number smaller than 2
#if RTE_I2S_GPDMA_LLP_CHAIN_NUM <= 2
    drv_gpdma_channel_set_ptr(env->tx_gpdma_idx, &env->tx_chain_env[env->tx_chain_rpos]);
#endif

    /// Handle callback
    if(env->tx_isr_cb) {
#if defined ( __ICCARM__ )
#pragma diag_suppress=Pa082
        env->tx_isr_cb(OM_I2S, DRV_GPDMA_EVENT_TERMINAL_COUNT_REQUEST, (void *)addr, (void *)(env->tx_chain_env[env->tx_chain_rpos].size_byte << dma_width));
#pragma diag_warning=Pa082
#else
        env->tx_isr_cb(OM_I2S, DRV_GPDMA_EVENT_TERMINAL_COUNT_REQUEST, (void *)addr, (void *)(env->tx_chain_env[env->tx_chain_rpos].size_byte << dma_width));
#endif
    }

    /// Indicate will read next node
    env->tx_chain_rpos = (env->tx_chain_rpos + 1) % RTE_I2S_GPDMA_LLP_CHAIN_NUM;

    /*
     * Example: If GDB make cpu hold, so LL_PTR and trans_size will make 0, DMA thinks only one node.
     * But traversing I2S chian list not just one. So force assign to next valid node and enable DMA.
     * Make DMA work at chain mode again.
    */
    if(NULL == next_chain) {
        uint8_t next = env->tx_chain_rpos;

        for(uint8_t i = 0; i < RTE_I2S_GPDMA_LLP_CHAIN_NUM; i++) {
            uint32_t check_addr = env->tx_chain_env[next].src_addr;
            uint32_t ll_ptr = (uint32_t)env->tx_chain_env[next].ll_ptr;

            if(check_addr) {
                if(ll_ptr) {
                    drv_gpdma_channel_set_ptr(env->tx_gpdma_idx, &env->tx_chain_env[(next + 1) % RTE_I2S_GPDMA_LLP_CHAIN_NUM]);
                }

                if(drv_gpdma_channel_is_busy(env->tx_gpdma_idx) == 0) {
                    drv_gpdma_channel_enable(env->tx_gpdma_idx, env->tx_chain_env[next].dst_addr, env->tx_chain_env[next].src_addr, env->tx_chain_env[next].size_byte << dma_width);
                }
                break;
            }

            next = (next + 1) % RTE_I2S_GPDMA_LLP_CHAIN_NUM;
        }
    }
}

static void i2s_gpdma_rx_isr_cb(void *p_resource, drv_event_t event, gpdma_chain_trans_t *next_chain)
{
    i2s_env_t *env;
    volatile uint32_t addr = 0;
    volatile gpdma_trans_width_t dma_width;

    I2S_ASSERT((uint32_t)p_resource == (uint32_t)&i2s_resource);

    /// Only this event type can handle continue
    if(!(DRV_GPDMA_EVENT_TERMINAL_COUNT_REQUEST & event)) {
        return;
    }

    env = (i2s_env_t *)i2s_resource.env;

    /// Capacity check
    if(!(env->config.dir & I2S_DIR_RX)) {
        I2S_ASSERT(0);
        return;
    }
    if(env->config.role == I2S_ROLE_INVALID) {
        return;
    }

    /// DMA transmit width
    dma_width = (env->config.bit_width > I2S_BW_16BIT) ? GPDMA_TRANS_WIDTH_4B : GPDMA_TRANS_WIDTH_2B;

    /// Obtain consumed node address
    addr = env->rx_chain_env[env->rx_chain_rpos].dst_addr;
    if(0 == addr) {
        return;
    }

    /// Mark data block has transfer completed
    env->rx_chain_env[env->rx_chain_rpos].dst_addr = 0;
    env->rx_chain_env[env->rx_chain_rpos].ll_ptr   = NULL;

    /// Make list work up when node number smaller than 2
#if RTE_I2S_GPDMA_LLP_CHAIN_NUM <= 2
    drv_gpdma_channel_set_ptr(env->rx_gpdma_idx, &env->rx_chain_env[env->rx_chain_rpos]);
#endif

    /// Handle callback
    if(env->rx_isr_cb) {
#if defined ( __ICCARM__ )
#pragma diag_suppress=Pa082
        env->rx_isr_cb(OM_I2S, DRV_GPDMA_EVENT_TERMINAL_COUNT_REQUEST, (void *)addr, (void *)(env->rx_chain_env[env->rx_chain_rpos].size_byte << dma_width));
#pragma diag_warning=Pa082
#else
        env->rx_isr_cb(OM_I2S, DRV_GPDMA_EVENT_TERMINAL_COUNT_REQUEST, (void *)addr, (void *)(env->rx_chain_env[env->rx_chain_rpos].size_byte << dma_width));
#endif
    }

    /// Indicate will read next node
    env->rx_chain_rpos = (env->rx_chain_rpos + 1) % RTE_I2S_GPDMA_LLP_CHAIN_NUM;

    /*
     * Example: If GDB make cpu hold, so LL_PTR and trans_size will make 0, DMA thinks only one node.
     * But traversing I2S chian list not just one. So force assign to next valid node and enable DMA.
     * Make DMA work at chain mode again.
    */
    if(NULL == next_chain) {
        uint8_t next = env->rx_chain_rpos;

        for(uint8_t i = 0; i < RTE_I2S_GPDMA_LLP_CHAIN_NUM; i++) {
            uint32_t check_addr = env->rx_chain_env[next].dst_addr;
            uint32_t ll_ptr = (uint32_t)env->rx_chain_env[next].ll_ptr;

            if(check_addr) {
                if(ll_ptr) {
                    drv_gpdma_channel_set_ptr(env->rx_gpdma_idx, &env->rx_chain_env[(next + 1) % RTE_I2S_GPDMA_LLP_CHAIN_NUM]);
                }

                if(drv_gpdma_channel_is_busy(env->rx_gpdma_idx) == 0) {
                    drv_gpdma_channel_enable(env->rx_gpdma_idx, env->rx_chain_env[next].dst_addr, env->rx_chain_env[next].src_addr, env->rx_chain_env[next].size_byte << dma_width);
                }
                break;
            }

            next = (next + 1) % RTE_I2S_GPDMA_LLP_CHAIN_NUM;
        }
    }
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
void drv_i2s_init(OM_I2S_Type *om_i2s, i2s_config_t *config)
{
    (void)om_i2s;
    i2s_env_t *env = (i2s_env_t *)(i2s_resource.env);

    I2S_ASSERT(config);

    /// Is inited?
    if(env->config.role != I2S_ROLE_INVALID) {
        return;
    }

    /// For global use
    memcpy(&env->config, config, sizeof(i2s_config_t));

    /// Enable audio 16M
    i2s_enable_daif_16m(config->role);

    /// Reset I2S module and Enable clock
    DRV_RCC_RESET(RCC_CLK_I2S);

    /// Set Channel mode
    register_set(&OM_I2S->IER, MASK_1REG(I2S_IER_MONO_EN, (I2S_CHN_MONO == config->channel) ? 1 : 0));

    /// Set work mode
    register_set(&OM_I2S->IER, MASK_2REG(I2S_IER_MODE, (RTE_I2S_MODE & 0x03), I2S_IER_DSP_MODE, ((RTE_I2S_MODE & 0x04) ? 1 : 0)));

    /// Set Bit width
    uint8_t i2s_bw = 0;
    switch (config->bit_width)
    {
    case I2S_BW_12BIT:
        i2s_bw = 1;
        break;
    case I2S_BW_16BIT:
        i2s_bw = 2;
        break;
    case I2S_BW_20BIT:
        i2s_bw = 3;
        break;
    case I2S_BW_24BIT:
        i2s_bw = 4;
        break;
    default:
        I2S_ASSERT(0);
        break;
    }

    /// Set I2S RX register
    if(config->dir & I2S_DIR_RX) {
        /// Bitwidth
        register_set(&OM_I2S->CH0.RCR, MASK_1REG(I2S_RCRx_WLEN, i2s_bw));

        /// Disable RX channel and RX block to flush RX FIFO
        register_set0(&OM_I2S->CH0.RER, I2S_RERx_RXCHEN_MASK);
        register_set0(&OM_I2S->IRER, I2S_IRER_RXEN_MASK);

        /// Flush RX FIFO
        register_set1(&OM_I2S->CH0.RFF, I2S_RFFx_RXCHFR_MASK);
        register_set1(&OM_I2S->RXFFR, I2S_RXFFR_RXFFR_MASK);
        register_set1(&OM_I2S->RRXDMA, I2S_RRXDMA_RRXDMA_MASK);

        /// Enable RX channel
        register_set1(&OM_I2S->IRER, I2S_IRER_RXEN_MASK);
        register_set1(&OM_I2S->CH0.RER, I2S_RERx_RXCHEN_MASK);

        /// Set RX channel FIFO depth
        register_set(&OM_I2S->CH0.RFCR, MASK_1REG(I2S_RFCRx_RXCHDT, 14));
    }

    /// Set I2S TX register
    if(config->dir & I2S_DIR_TX) {
        /// Bitwidth
        register_set(&OM_I2S->CH0.TCR, MASK_1REG(I2S_TCRx_WLEN, i2s_bw));

        /// Disable TX channel and RX block to flush RX FIFO
        register_set0(&OM_I2S->CH0.TER, I2S_TERx_TXCHEN_MASK);
        register_set0(&OM_I2S->ITER, I2S_ITER_TXEN_MASK);

        /// Flush TX FIFO
        register_set1(&OM_I2S->CH0.TFF, I2S_TFFx_TXCHFR_MASK);
        register_set1(&OM_I2S->TXFFR, I2S_TXFFR_TXFFR_MASK);
        register_set1(&OM_I2S->RTXDMA, I2S_RTXDMA_RTXDMA_MASK);

        /// Enable TX channel
        register_set1(&OM_I2S->ITER, I2S_ITER_TXEN_MASK);
        register_set1(&OM_I2S->CH0.TER, I2S_TERx_TXCHEN_MASK);

        /// Set TX channel FIFO depth
        register_set(&OM_I2S->CH0.TFCR, MASK_1REG(I2S_TFCRx_TXCHET, 14));
    }

    /// Mask all interrupt
    register_set(&OM_I2S->CH0.IMR, MASK_4REG(I2S_IMRx_RXDAM, 1, I2S_IMRx_RXFOM, 1, I2S_IMRx_TXFEM, 1, I2S_IMRx_TXFOM, 1));

    /// Disable Kernal IRQ
    NVIC_ClearPendingIRQ(i2s_resource.irq_num);
    NVIC_SetPriority(i2s_resource.irq_num, i2s_resource.irq_prio);
    NVIC_DisableIRQ(i2s_resource.irq_num);

    /// Init environment variable
    env->rx_isr_cb      = NULL;
    env->rx_gpdma_idx   = GPDMA_NUMBER_OF_CHANNELS;
    env->rx_chain_wpos  = 0;
    env->rx_chain_rpos  = 0;

    env->tx_isr_cb      = NULL;
    env->tx_gpdma_idx   = GPDMA_NUMBER_OF_CHANNELS;
    env->tx_chain_wpos  = 0;
    env->tx_chain_rpos  = 0;

    if (config->dir & I2S_DIR_RX) {
        memset(env->rx_chain_env, 0, sizeof(env->rx_chain_env));
    }

    if(config->dir & I2S_DIR_TX) {
        memset(env->tx_chain_env, 0, sizeof(env->tx_chain_env));
    }
}

void drv_i2s_uninit(OM_I2S_Type *om_i2s)
{
    (void)om_i2s;
    i2s_env_t *env = (i2s_env_t *)(i2s_resource.env);

    /// Is uninited?
    if(I2S_ROLE_INVALID == env->config.role) {
        return;
    }

    /// Invalid role mark uninit state
    env->config.role = I2S_ROLE_INVALID;

    /// Abort gpdma transmit
    if(env->config.dir & I2S_DIR_RX) {
        if (env->rx_gpdma_idx < GPDMA_NUMBER_OF_CHANNELS) {
            drv_gpdma_channel_release(env->rx_gpdma_idx);
            env->rx_gpdma_idx  = GPDMA_NUMBER_OF_CHANNELS;
        }
        env->rx_chain_wpos = 0;
        env->rx_chain_rpos = 0;
    }

    if(env->config.dir & I2S_DIR_TX) {
        if (env->tx_gpdma_idx < GPDMA_NUMBER_OF_CHANNELS) {
            drv_gpdma_channel_release(env->tx_gpdma_idx);
            env->tx_gpdma_idx  = GPDMA_NUMBER_OF_CHANNELS;
        }
        env->tx_chain_wpos = 0;
        env->tx_chain_rpos = 0;
    }

    /// Disable I2S module
    register_set0(&OM_I2S->IER, I2S_IER_IEN_MASK);

    /// Master clock generation disable
    register_set0(&OM_CPM->I2S_CFG, CPM_I2S_CFG_MST_EN_MASK);

    /// Gate I2S clock
    DRV_RCC_CLOCK_ENABLE(RCC_CLK_I2S, 0);

    /// Disable audio 16M
    i2s_disable_daif_16m();
}

void drv_i2s_tx_register_isr_callback(OM_I2S_Type *om_i2s, drv_isr_callback_t isr_cb)
{
    (void)om_i2s;
    i2s_env_t *env = (i2s_env_t *)(i2s_resource.env);
    env->tx_isr_cb = isr_cb;
}

void drv_i2s_rx_register_isr_callback(OM_I2S_Type *om_i2s, drv_isr_callback_t isr_cb)
{
    (void)om_i2s;
    i2s_env_t *env = (i2s_env_t *)(i2s_resource.env);
    env->rx_isr_cb = isr_cb;
}

om_error_t drv_i2s_write_dma(OM_I2S_Type *om_i2s, uint8_t *buffer, uint32_t size)
{
    i2s_env_t *env = (i2s_env_t *)(i2s_resource.env);
    gpdma_config_t dma_config;
    om_error_t error = OM_ERROR_OK;
    gpdma_trans_width_t dma_width;

    (void)om_i2s;

    I2S_ASSERT(buffer);
    I2S_ASSERT(OM_IS_ALIGN(buffer, 2));
    I2S_ASSERT(size);

    /// Capacity check
    if(!(env->config.dir & I2S_DIR_TX)) {
        return OM_ERROR_PERMISSION;
    }
    if(env->config.role == I2S_ROLE_INVALID) {
        return OM_ERROR_STATUS;
    }

    /// DMA transmit width
    dma_width = (env->config.bit_width > I2S_BW_16BIT) ? GPDMA_TRANS_WIDTH_4B : GPDMA_TRANS_WIDTH_2B;

    /// Create list first, only once.
    if(env->tx_gpdma_idx >= GPDMA_NUMBER_OF_CHANNELS) {

        /// Allocate gpdma channel
        env->tx_gpdma_idx = drv_gpdma_channel_allocate();

        I2S_ASSERT(env->tx_gpdma_idx < GPDMA_NUMBER_OF_CHANNELS);
        if(env->tx_gpdma_idx >= GPDMA_NUMBER_OF_CHANNELS) {
            return OM_ERROR_RESOURCES;
        }

        /// DMA channel config
        dma_config.channel_ctrl  = GPDMA_SET_CTRL(GPDMA_ADDR_CTRL_INC,
                                   GPDMA_ADDR_CTRL_FIXED,
                                   dma_width,
                                   dma_width,
                                   GPDMA_BURST_SIZE_1T,
                                   (i2s_resource.gpdma_tx.prio) ? GPDMA_PRIORITY_HIGH : GPDMA_PRIORITY_LOW);
        dma_config.src_id        = GPDMA_ID_MEM;
        dma_config.dst_id        = (gpdma_id_t)i2s_resource.gpdma_tx.id;
        dma_config.isr_cb        = i2s_gpdma_tx_isr_cb;
        dma_config.cb_param      = (void *)&i2s_resource;

        /// Fill DMA channel LL_PTR register initial value is the second node address
        dma_config.chain_trans     = &env->tx_chain_env[1];
        /// Config node trans_size and ctrl value begin at second node, so chain_trans_num is RTE_I2S_GPDMA_LLP_CHAIN_NUM - 1
        dma_config.chain_trans_num = RTE_I2S_GPDMA_LLP_CHAIN_NUM - 1;

        /// Init TX GPDMA chain
        for(uint8_t i = 0; i < RTE_I2S_GPDMA_LLP_CHAIN_NUM; i++) {
            env->tx_chain_env[i].dst_addr   = (uint32_t)(&OM_I2S->TXDMA);
            env->tx_chain_env[i].src_addr   = 0;
            env->tx_chain_env[i].size_byte  = size;
            env->tx_chain_env[i].ll_ptr     = NULL;
        }

        /// Config second to RTE_I2S_GPDMA_LLP_CHAIN_NUM - 1 node
        error = drv_gpdma_channel_config(env->tx_gpdma_idx, &dma_config);
        if(error != OM_ERROR_OK) {
            drv_gpdma_channel_release(env->tx_gpdma_idx);
            env->tx_gpdma_idx = GPDMA_NUMBER_OF_CHANNELS;
            return error;
        }

        /// Fill first node
        env->tx_chain_env[0].src_addr   = (uint32_t)buffer;
        env->tx_chain_env[0].size_byte  = env->tx_chain_env[1].size_byte;
        env->tx_chain_env[0].ctrl       = env->tx_chain_env[1].ctrl;
        env->tx_chain_env[0].ll_ptr     = &env->tx_chain_env[1];

        /// Set fill position next time
        env->tx_chain_wpos              = 1;

        /// Start transmit
        error = drv_gpdma_channel_enable(env->tx_gpdma_idx, (uint32_t)(&(OM_I2S->TXDMA)), (uint32_t)buffer, size);
        I2S_ASSERT(error == OM_ERROR_OK);
        if(error == OM_ERROR_OK) {
            /// Set I2S MCLK & BCLK & WS, Begain work !!!
            i2s_set_ws_bclk(&env->config);
        }
    } else {
        uint32_t dst_fill_addr = 0;
        uint8_t pre_node_idx = 0;

        OM_CRITICAL_BEGIN();
        dst_fill_addr = env->tx_chain_env[env->tx_chain_wpos].src_addr;
        OM_CRITICAL_END();

        /// Is full, wait consume
        if(dst_fill_addr) {
            return error;
        }

        OM_CRITICAL_BEGIN();
        /// Write new node
        env->tx_chain_env[env->tx_chain_wpos].src_addr   = (uint32_t)buffer;
        env->tx_chain_env[env->tx_chain_wpos].trans_size = (size >> dma_width);
        env->tx_chain_env[env->tx_chain_wpos].ll_ptr     = NULL;

        /// Set previous node ll_ptr
        pre_node_idx = (env->tx_chain_wpos + RTE_I2S_GPDMA_LLP_CHAIN_NUM - 1) % RTE_I2S_GPDMA_LLP_CHAIN_NUM;
        if(env->tx_chain_env[pre_node_idx].src_addr) {
            env->tx_chain_env[pre_node_idx].ll_ptr = &(env->tx_chain_env[env->tx_chain_wpos]);
        }

        /// Move index pointer to next node
        env->tx_chain_wpos = (env->tx_chain_wpos + 1) % RTE_I2S_GPDMA_LLP_CHAIN_NUM;
        OM_CRITICAL_END();

        /// Enable GPDMA channel start transmit
        if (drv_gpdma_channel_is_busy(env->tx_gpdma_idx) == 0) {
            drv_gpdma_channel_enable(env->tx_gpdma_idx, (uint32_t)(&(OM_I2S->TXDMA)), (uint32_t)buffer, (uint32_t)size);
        }
    }

    return error;
}

om_error_t drv_i2s_read_dma(OM_I2S_Type *om_i2s, uint8_t *buffer, uint32_t size)
{
    i2s_env_t *env = (i2s_env_t *)(i2s_resource.env);
    gpdma_config_t dma_config;
    om_error_t error = OM_ERROR_OK;
    gpdma_trans_width_t dma_width;

    (void)om_i2s;

    I2S_ASSERT(buffer);
    I2S_ASSERT(OM_IS_ALIGN(buffer, 2));
    I2S_ASSERT(size);

    /// Capacity check
    if(!(env->config.dir & I2S_DIR_RX)) {
        return OM_ERROR_PERMISSION;
    }
    if(env->config.role == I2S_ROLE_INVALID) {
        return OM_ERROR_STATUS;
    }

    /// DMA transmit width
    dma_width = (env->config.bit_width > I2S_BW_16BIT) ? GPDMA_TRANS_WIDTH_4B : GPDMA_TRANS_WIDTH_2B;

    /// Create list first, only once.
    if(env->rx_gpdma_idx >= GPDMA_NUMBER_OF_CHANNELS) {

        /// Allocate gpdma channel
        env->rx_gpdma_idx = drv_gpdma_channel_allocate();

        I2S_ASSERT(env->rx_gpdma_idx < GPDMA_NUMBER_OF_CHANNELS);
        if(env->rx_gpdma_idx >= GPDMA_NUMBER_OF_CHANNELS) {
            return OM_ERROR_RESOURCES;
        }

        /// DMA channel config
        dma_config.channel_ctrl             = GPDMA_SET_CTRL(GPDMA_ADDR_CTRL_FIXED,
                                              GPDMA_ADDR_CTRL_INC,
                                              dma_width,
                                              dma_width,
                                              GPDMA_BURST_SIZE_1T,
                                              (i2s_resource.gpdma_rx.prio) ? GPDMA_PRIORITY_HIGH : GPDMA_PRIORITY_LOW);
        dma_config.src_id                   = (gpdma_id_t)i2s_resource.gpdma_rx.id;
        dma_config.dst_id                   = GPDMA_ID_MEM;
        dma_config.isr_cb                   = i2s_gpdma_rx_isr_cb;
        dma_config.cb_param                 = (void *)&i2s_resource;

        /// Fill DMA channel LL_PTR register initial value is the second node address
        dma_config.chain_trans              = &env->rx_chain_env[1];
        /// Config node trans_size and ctrl value begin at second node, so chain_trans_num is RTE_I2S_GPDMA_LLP_CHAIN_NUM - 1
        dma_config.chain_trans_num          = RTE_I2S_GPDMA_LLP_CHAIN_NUM - 1;

        /// Init RX GPDMA chain
        for(uint8_t i = 0; i < RTE_I2S_GPDMA_LLP_CHAIN_NUM; i++) {
            env->rx_chain_env[i].src_addr   = (uint32_t)(&OM_I2S->RXDMA);
            env->rx_chain_env[i].dst_addr   = 0;
            env->rx_chain_env[i].size_byte  = size;
            env->rx_chain_env[i].ll_ptr     = NULL;
        }

        /// Config second to RTE_I2S_GPDMA_LLP_CHAIN_NUM - 1 node
        error = drv_gpdma_channel_config(env->rx_gpdma_idx, &dma_config);
        if(error != OM_ERROR_OK) {
            drv_gpdma_channel_release(env->rx_gpdma_idx);
            env->rx_gpdma_idx = GPDMA_NUMBER_OF_CHANNELS;
            return error;
        }

        /// Fill first node
        env->rx_chain_env[0].dst_addr   = (uint32_t)buffer;
        env->rx_chain_env[0].size_byte  = env->rx_chain_env[1].size_byte;
        env->rx_chain_env[0].ctrl       = env->rx_chain_env[1].ctrl;
        env->rx_chain_env[0].ll_ptr     = &env->rx_chain_env[1];

        /// Set fill position next time
        env->rx_chain_wpos              = 1;

        /// Start transmit
        error = drv_gpdma_channel_enable(env->rx_gpdma_idx, (uint32_t)buffer, (uint32_t)(&(OM_I2S->RXDMA)), size);
        I2S_ASSERT(error == OM_ERROR_OK);
        if(error == OM_ERROR_OK) {
            /// Set I2S MCLK & BCLK & WS, Begain work !!!
            i2s_set_ws_bclk(&env->config);
        }
    } else {
        uint32_t dst_fill_addr = 0;
        uint8_t pre_node_idx = 0;

        OM_CRITICAL_BEGIN();
        dst_fill_addr = env->rx_chain_env[env->rx_chain_wpos].dst_addr;
        OM_CRITICAL_END();

        /// Is full, wait consume
        if(dst_fill_addr) {
            return error;
        }

        OM_CRITICAL_BEGIN();
        /// Write new node
        env->rx_chain_env[env->rx_chain_wpos].dst_addr   = (uint32_t)buffer;
        env->rx_chain_env[env->rx_chain_wpos].trans_size = (size >> dma_width);
        env->rx_chain_env[env->rx_chain_wpos].ll_ptr     = NULL;

        /// Set previous node ll_ptr
        pre_node_idx = (env->rx_chain_wpos + RTE_I2S_GPDMA_LLP_CHAIN_NUM - 1) % RTE_I2S_GPDMA_LLP_CHAIN_NUM;
        if(env->rx_chain_env[pre_node_idx].dst_addr) {
            env->rx_chain_env[pre_node_idx].ll_ptr = &(env->rx_chain_env[env->rx_chain_wpos]);
        }

        /// Move index pointer to next node
        env->rx_chain_wpos = (env->rx_chain_wpos + 1) % RTE_I2S_GPDMA_LLP_CHAIN_NUM;
        OM_CRITICAL_END();

        /// Enable GPDMA channel start transmit
        if (drv_gpdma_channel_is_busy(env->rx_gpdma_idx) == 0) {
            drv_gpdma_channel_enable(env->rx_gpdma_idx, (uint32_t)buffer, (uint32_t)(&(OM_I2S->RXDMA)), (uint32_t)size);
        }
    }

    return error;
}

uint8_t drv_i2s_tx_get_chain_space(OM_I2S_Type *om_i2s)
{
    i2s_env_t *env = (i2s_env_t *)(i2s_resource.env);
    uint8_t chain_space = 0;

    (void)om_i2s;

    if(env->config.role == I2S_ROLE_INVALID) {
        return 0;
    }

    OM_CRITICAL_BEGIN();
    for(uint8_t i = 0; i < RTE_I2S_GPDMA_LLP_CHAIN_NUM; i++) {
        if((env->tx_chain_env[i].src_addr == 0)) {
            chain_space++;
        }
    }
    OM_CRITICAL_END();

    return chain_space;
}

uint8_t drv_i2s_rx_get_chain_space(OM_I2S_Type *om_i2s)
{
    i2s_env_t *env = (i2s_env_t *)(i2s_resource.env);
    uint8_t chain_space = 0;

    (void)om_i2s;

    if(env->config.role == I2S_ROLE_INVALID) {
        return 0;
    }

    OM_CRITICAL_BEGIN();
    for(uint8_t i = 0; i < RTE_I2S_GPDMA_LLP_CHAIN_NUM; i++) {
        if((env->rx_chain_env[i].dst_addr == 0)) {
            chain_space++;
        }
    }
    OM_CRITICAL_END();

    return chain_space;
}

void drv_i2s_isr(void)
{
    // Should not run here
    while(1);
}

#endif /* RTE_I2S */

/** @} */