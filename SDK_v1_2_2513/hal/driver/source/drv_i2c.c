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
 * @brief    I2C driver source file
 * @details  I2C driver source file
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
#if (RTE_I2C0 || RTE_I2C1)
#include <stddef.h>
#include "om_device.h"
#include "om_driver.h"


/*******************************************************************************
 * MACROS
 */
#define I2C_DISABLE                 0x0U
#define I2C_ENABLE                  0x1U

#define I2C_SPEED_STANDARD          0x1U
#define I2C_SPEED_FAST              0x2U
#define I2C_SPEED_HIGH              0x3U

#define I2C_TX_BUSY                 (1U << 0)
#define I2C_RX_BUSY                 (1U << 4)


/*******************************************************************************
 * CONST & VARIABLES
 */
/* i2c information */
#if (RTE_I2C0)
DRV_DEFINE(I2C0, i2c0);
#endif
#if (RTE_I2C1)
DRV_DEFINE(I2C1, i2c1);
#endif


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static const drv_resource_t *i2c_get_resource(OM_I2C_Type *om_i2c)
{
    static const drv_resource_t *i2c_resources[] = {
        #if (RTE_I2C0)
        &i2c0_resource,
        #endif
        #if (RTE_I2C1)
        &i2c1_resource,
        #endif
    };

    for(uint32_t i=0; i<sizeof(i2c_resources)/sizeof(i2c_resources[0]); i++) {
        if ((uint32_t)om_i2c == (uint32_t)i2c_resources[i]->reg) {
            return i2c_resources[i];
        }
    }

    OM_ASSERT(0);
    return NULL;
}

static uint32_t i2c_scl_hcnt(int ic_clk, int tsymbol, int tf, int cond, int offset)
{
    /*
     * DesignWare I2C core doesn't seem to have solid strategy to meet
     * the tHD;STA timing spec.  Configuring _HCNT based on tHIGH spec
     * will result in violation of the tHD;STA spec.
     */
    if (cond) {
        /*
         * Conditional expression:
         *
         *   IC_[FS]S_SCL_HCNT + (1+4+3) >= IC_CLK * tHIGH
         *
         * This is based on the manuals, and represents an ideal
         * configuration.  The resulting I2C bus speed will be
         * faster than any of the others.
         *
         * If your hardware is free from tHD;STA issue, try this one.
         */
        return (((ic_clk * tsymbol) + 5000) / 10000 - 8) + offset;
    } else {
        /*
         * Conditional expression:
         *
         *   IC_[FS]S_SCL_HCNT + 3 >= IC_CLK * (tHD;STA + tf)
         *
         * This is just experimental rule; the tHD;STA period turned
         * out to be proportinal to (_HCNT + 3).  With this setting,
         * we could meet both tHIGH and tHD;STA timing specs.
         *
         * If unsure, you'd better to take this alternative.
         *
         * The reason why we need to take into account "tf" here,
         * is the same as described in i2c_lld_scl_lcnt().
         */
        return ((ic_clk * (tsymbol + tf) + 5000) / 10000 - 3) + offset;
    }
}

static uint32_t i2c_scl_lcnt(int ic_clk, int tlow, int tf, int offset)
{
    /*
     * Conditional expression:
     *
     *   IC_[FS]S_SCL_LCNT + 1 >= IC_CLK * (tLOW + tf)
     *
     * DW I2C core starts counting the SCL CNTs for the LOW period
     * of the SCL clock (tLOW) as soon as it pulls the SCL line.
     * In order to meet the tLOW timing spec, we need to take into
     * account the fall time of SCL signal (tf).  Default tf value
     * should be 0.3 us, for safety.
     */
    return (((ic_clk * (tlow + tf) + 5000) / 10000) - 1) + offset;
}

static uint8_t i2c_dev_is_valid(OM_I2C_Type *om_i2c, uint32_t dev_addr)
{
    uint32_t dummy;
    dummy = om_i2c->CLR_STOP_DET;

    om_i2c->ENABLE  = I2C_DISABLE;
    om_i2c->TAR     = dev_addr;
    om_i2c->CON1    = I2C_CON1_TX_ENABLE;
    om_i2c->ENABLE  = I2C_ENABLE;
    om_i2c->DATA_CMD = 0x0U;

    // wait stop
    while (!(om_i2c->RAW_INTR_STAT & I2C_INTR_STOP_DET_MASK));

    if ((om_i2c->RAW_INTR_STAT & I2C_INTR_TX_ABRT_MASK)) {
        if ((om_i2c->TX_ABRT_SOURCE & I2C_TX_ABRT_SRC_7B_ADDR_NOACK_MASK)) {
            dummy = om_i2c->CLR_TX_ABRT;
            return 1U;
        }
    }

    dummy = om_i2c->CLR_TX_ABRT;
    (void)dummy;

    return 0U;
}

#if (RTE_GPDMA)
static void i2c_gpdma_rx_isr_cb(void *resource, drv_event_t event, gpdma_chain_trans_t *next_chain)
{
    drv_env_t   *env;
    OM_I2C_Type *om_i2c;
    uint32_t     rx_num;
    drv_event_t  drv_event;

    OM_ASSERT(resource);
    env     = (drv_env_t *)((const drv_resource_t *)resource)->env;
    om_i2c  = (OM_I2C_Type *)((const drv_resource_t *)resource)->reg;
    rx_num  = env->rx_num;
    env->rx_num = 0U;

    switch (event) {
        case DRV_GPDMA_EVENT_TERMINAL_COUNT_REQUEST:
            drv_event = DRV_EVENT_COMMON_READ_COMPLETED;
            break;
        case DRV_GPDMA_EVENT_ABORT:
            drv_event = DRV_EVENT_COMMON_ABORT;
            break;
        default:
            drv_event = DRV_EVENT_COMMON_ERROR;
            OM_ASSERT(0);
            break;
    }

    env->busy &= ~I2C_RX_BUSY;
    drv_i2c_isr_callback(om_i2c, drv_event, env->rx_buf, rx_num);
}

static void i2c_gpdma_tx_isr_cb(void *resource, drv_event_t event, gpdma_chain_trans_t *next_chain)
{
    drv_env_t   *env;
    OM_I2C_Type *om_i2c;
    uint32_t     tx_num;
    drv_event_t  drv_event;

    OM_ASSERT(resource);
    env     = (drv_env_t *)((drv_resource_t *)resource)->env;
    om_i2c  = (OM_I2C_Type *)((drv_resource_t *)resource)->reg;
    tx_num  = env->tx_num;
    env->tx_num = 0U;

    switch (event) {
        case DRV_GPDMA_EVENT_TERMINAL_COUNT_REQUEST:
            drv_event = DRV_EVENT_COMMON_GPDMA2PERIPH_COMPLETED;
            while ((!(om_i2c->STATUS & I2C_STATUS_TFE_MASK)) && (!(om_i2c->RAW_INTR_STAT & I2C_INTR_TIME_OUT_MASK)) && (!((om_i2c->RAW_INTR_STAT & I2C_INTR_TX_ABRT_MASK)&&(om_i2c->TX_ABRT_SOURCE & (I2C_TX_ABRT_SRC_7B_ADDR_NOACK_MASK | I2C_TX_ABRT_SRC_10ADDR1_NOACK_MASK | I2C_TX_ABRT_SRC_10ADDR2_NOACK_MASK | I2C_TX_ABRT_SRC_TXDATA_NOACK_MASK)))));


            if (env->rx_cnt < env->rx_num) {
                om_i2c->CON1 = (env->rx_num | I2C_CON1_RX_ENABLE | I2C_CON1_READBYTES_UPDATE);
                drv_gpdma_channel_enable(env->gpdma_rx_chan, (uint32_t)env->rx_buf, (uint32_t)&om_i2c->DATA_CMD, env->rx_num);
            }
            break;
        case DRV_GPDMA_EVENT_ABORT:
            drv_event = DRV_EVENT_COMMON_ABORT;
            break;
        default:
            drv_event = DRV_EVENT_COMMON_ERROR;
            OM_ASSERT(0);
            break;
    }

    env->busy &= ~I2C_TX_BUSY;
    if (!(env->busy & I2C_RX_BUSY)) {
        drv_i2c_isr_callback(om_i2c, drv_event, env->tx_buf, tx_num);
    }
}
#endif


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
om_error_t drv_i2c_init(OM_I2C_Type *om_i2c, const i2c_config_t *cfg)
{
    const drv_resource_t  *resource;
    drv_env_t             *env;
    uint32_t               clk;

    OM_ASSERT(cfg);
    resource = i2c_get_resource(om_i2c);
    if ((resource == NULL) || (cfg == NULL)) {
        return OM_ERROR_PARAMETER;
    }

    DRV_RCC_RESET((uint32_t)om_i2c);
    env = (drv_env_t *)resource->env;

    om_i2c->ENABLE = I2C_DISABLE;
    switch (cfg->mode) {
        case I2C_MODE_MASTER:
            om_i2c->CON = I2C_CON_MASTER_MODE_MASK | I2C_CON_SLAVE_DISABLE_MASK | I2C_CON_RESTART_EN_MASK;
            break;
        case I2C_MODE_SLAVE:
            om_i2c->CON = I2C_CON_RESTART_EN_MASK;
            break;
        case I2C_MODE_SMBUS_DEVICE:
            om_i2c->CON = I2C_CON_10BITADDR_SLAVE_MASK | I2C_CON_RESTART_EN_MASK;
            break;
        case I2C_MODE_SMBUS_HOST:
            om_i2c->CON = I2C_CON_MASTER_MODE_MASK | I2C_CON_SLAVE_DISABLE_MASK | I2C_CON_10BITADDR_MASTER_MASK | I2C_CON_RESTART_EN_MASK;
            break;
        default:
            OM_ASSERT(0);
            return OM_ERROR_PARAMETER;
    }

    clk = drv_rcc_clock_get(RCC_CLK_I2C0);
    switch (cfg->speed) {
        case I2C_SPEED_100K:
            /* set standard and fast speed deviders for high/low periods */
            /* Standard-mode @100K period=10us */
            om_i2c->SS_SCL_HCNT = i2c_scl_hcnt(clk / 1000,
                                               40, /* tHD;STA = tHIGH = 4.0 us */
                                               3,  /* tf = 0.3 us */
                                               0,  /* 0: default, 1: Ideal */
                                               0); /* No offset */
            om_i2c->SS_SCL_LCNT = i2c_scl_lcnt(clk / 1000,
                                               47, /* tLOW = 4.7 us */
                                               3,  /* tf = 0.3 us */
                                               0); /* No offset */
            /* Standard mode clock_div calculate: Tlow/Thigh = 1/1.*/
            /* Sets the Maximum Rise Time for standard mode.*/
            register_set(&om_i2c->CON, MASK_1REG(I2C_CON_SPEED, I2C_SPEED_STANDARD));
            /* set SDA hold time */
            om_i2c->SDA_HOLD = 0x10;
            break;
        case I2C_SPEED_400K:
            /* Fast-mode @400K period=2.5us */
            om_i2c->FS_SCL_HCNT = i2c_scl_hcnt(clk / 1000,
                                               6, /* tHD;STA = tHIGH = 0.6 us */
                                               3, /* tf = 0.3 us */
                                               0, /* 0: default, 1: Ideal */
                                               0); /* No offset */
            om_i2c->FS_SCL_LCNT = i2c_scl_lcnt(clk / 1000,
                                               13, /* tLOW = 1.3 us */
                                               3, /* tf = 0.3 us */
                                               0); /* No offset */
            /* Sets the Maximum Rise Time for fast mode.*/
            register_set(&om_i2c->CON, MASK_1REG(I2C_CON_SPEED, I2C_SPEED_FAST));
            /* set SDA hold time */
            om_i2c->SDA_HOLD = 0x10;
            break;
        case I2C_SPEED_1M:
            /* High-mode @1M period=1us */
            om_i2c->HS_SCL_HCNT = i2c_scl_hcnt(clk / 1000,
                                               2, /* tHD;STA = tHIGH = 0.2 us */
                                               3, /* tf = 0.3 us */
                                               0, /* 0: default, 1: Ideal */
                                               0); /* No offset */
            om_i2c->HS_SCL_LCNT = i2c_scl_lcnt(clk / 1000,
                                               2, /* tLOW = 0.2 us */
                                               3, /* tf = 0.3 us */
                                               0); /* No offset */
            /* Sets the Maximum Rise Time for high mode.*/
            register_set(&om_i2c->CON, MASK_1REG(I2C_CON_SPEED, I2C_SPEED_HIGH));
            /* set SDA hold time */
            om_i2c->SDA_HOLD = 0x1;
            break;
        case I2C_SPEED_2M:
            /* High-mode @2M period=0.5us,need to set CPU clk 64M */
            om_i2c->HS_SCL_HCNT = i2c_scl_hcnt(clk / 1000,
                                               1, /* tHD;STA = tHIGH = 0.1 us */
                                               2, /* tf = 0.2 us */
                                               0, /* 0: default, 1: Ideal */
                                               0); /* No offset */
            om_i2c->HS_SCL_LCNT = i2c_scl_lcnt(clk / 1000,
                                               1, /* tLOW = 0.1 us */
                                               1, /* tf = 0.1 us */
                                               0); /* No offset */
            /* Sets the Maximum Rise Time for high mode.*/
            register_set(&om_i2c->CON, MASK_1REG(I2C_CON_SPEED, I2C_SPEED_HIGH));
            /* set SDA hold time */
            om_i2c->SDA_HOLD = 0x1;
            break;
        case I2C_SPEED_MAX:
            /* High-mode max,speed=clk/(i2c_scl_hcnt(min6)+i2c_scl_lcnt(min8)+9),if cpu clk 64M ,then SPI max speed 2.78M */
            om_i2c->HS_SCL_HCNT = 6;
            om_i2c->HS_SCL_LCNT = 8;
            /* Sets the Maximum Rise Time for high mode.*/
            register_set(&om_i2c->CON, MASK_1REG(I2C_CON_SPEED, I2C_SPEED_HIGH));
            /* set SDA hold time */
            om_i2c->SDA_HOLD = 0x1;
            break;
        default:
            OM_ASSERT(0);
            return OM_ERROR_PARAMETER;
    }

    #if (RTE_GPDMA)
    env->gpdma_tx_chan = GPDMA_NUMBER_OF_CHANNELS;
    env->gpdma_rx_chan = GPDMA_NUMBER_OF_CHANNELS;
    #endif

    NVIC_ClearPendingIRQ(resource->irq_num);
    NVIC_SetPriority(resource->irq_num, resource->irq_prio);
    NVIC_EnableIRQ(resource->irq_num);

    // mask all the interrupt
    om_i2c->INTR_MASK = 0;
    /* tx fifo has 15 bytes or below then trigger the tx empty interrupt.*/
    om_i2c->TX_TL = 15U;
    /* rx fifo has received one byte then trigger the rx full interrupt.*/
    om_i2c->RX_TL = 0U;

    env->isr_cb = NULL;

    return OM_ERROR_OK;
}

om_error_t drv_i2c_uninit(OM_I2C_Type *om_i2c)
{
    const drv_resource_t  *resource;

    resource = i2c_get_resource(om_i2c);
    if (resource == NULL) {
        return OM_ERROR_PARAMETER;
    }

    DRV_RCC_CLOCK_ENABLE((rcc_clk_t)(size_t)resource->reg, 0U);

    NVIC_DisableIRQ(resource->irq_num);

    return OM_ERROR_OK;
}

#if (RTE_I2C_REGISTER_CALLBACK)
void drv_i2c_register_isr_callback(OM_I2C_Type *om_i2c, drv_isr_callback_t cb)
{
    const drv_resource_t  *resource;
    drv_env_t             *env;

    resource = i2c_get_resource(om_i2c);
    if (resource) {
        env = (drv_env_t *)resource->env;
        env->isr_cb = cb;
    }
}
#endif

__WEAK void drv_i2c_isr_callback(OM_I2C_Type *om_i2c, drv_event_t event, uint8_t *data, uint32_t num)
{
    #if (RTE_I2C_REGISTER_CALLBACK)
    const drv_resource_t  *resource;
    drv_env_t             *env;

    resource = i2c_get_resource(om_i2c);
    if (resource == NULL) {
        return;
    }
    env = (drv_env_t *)resource->env;

    if (env->isr_cb) {
        env->isr_cb(om_i2c, event, data, (void *)num);
    }
    #endif
}

om_error_t drv_i2c_master_write(OM_I2C_Type *om_i2c, uint16_t dev_addr, const uint8_t *tx_data, uint32_t tx_num, uint32_t timeout_ms)
{
    uint16_t len;
    uint16_t fifo_padding;
    uint16_t fifo_size;
    uint16_t offset;
    const drv_resource_t *resource;
    uint32_t irq_save;
    uint32_t tx_tl;
    uint32_t start  = drv_dwt_get_cycle();
    uint32_t target = DRV_DWT_MS_2_CYCLES_CEIL(timeout_ms);

    om_i2c->ENABLE      = I2C_DISABLE;
    om_i2c->TAR         = dev_addr;
    om_i2c->CON1        = I2C_CON1_TX_ENABLE;
    om_i2c->ENABLE      = I2C_ENABLE;

    resource = i2c_get_resource(om_i2c);
    if (resource == NULL) {
        return OM_ERROR_PARAMETER;
    }
    om_i2c->INTR_MASK = 0;

    offset = 0U;
    fifo_size = register_get(&resource->cap, MASK_POS(CAP_I2C_FIFO_LEVEL));
    tx_tl = om_i2c->TX_TL;
    OM_CRITICAL_BEGIN_EX(irq_save);
    while (tx_num) {
        fifo_padding = fifo_size - om_i2c->TXFLR;
        len = tx_num > fifo_padding ? fifo_padding : tx_num;
        for (uint32_t i = 0; i < len; i++) {
            om_i2c->DATA_CMD = tx_data[offset + i];
        }
        while (om_i2c->TXFLR > tx_tl) {
            if ((drv_dwt_get_cycle() - start) > target) {
                OM_CRITICAL_END_EX(irq_save);
                return OM_ERROR_TIMEOUT;
            }
        }
        offset += len;
        tx_num -= len;
    }
    OM_CRITICAL_END_EX(irq_save);

    while (!(om_i2c->STATUS & I2C_STATUS_TFE_MASK) || (om_i2c->STATUS & I2C_STATUS_MST_ACTIVITY_MASK)) {
        if ((drv_dwt_get_cycle() - start) > target) {
            OM_CRITICAL_END_EX(irq_save);
            return OM_ERROR_TIMEOUT;
        }
    }

    return OM_ERROR_OK;
}

om_error_t drv_i2c_master_write_int(OM_I2C_Type *om_i2c, uint16_t dev_addr, const uint8_t *tx_data, uint32_t tx_num, uint32_t timeout_ms)
{
    const drv_resource_t *resource;
    drv_env_t            *env;
    uint32_t              fifo_size;
	uint32_t main_clk;
	uint32_t timeout_count;

    resource = i2c_get_resource(om_i2c);
    if (resource == NULL) {
        return OM_ERROR_PARAMETER;
    }
    env = (drv_env_t *)resource->env;
    fifo_size = register_get(&resource->cap, MASK_POS(CAP_I2C_FIFO_LEVEL));

    env->tx_buf = (uint8_t *)tx_data;
    env->tx_num = tx_num;
    env->tx_cnt = 0U;

    om_i2c->ENABLE = I2C_DISABLE;
    om_i2c->TAR    = dev_addr;
    om_i2c->CON1   = I2C_CON1_TX_ENABLE;
    om_i2c->ENABLE = I2C_ENABLE;

    OM_CRITICAL_BEGIN();
    while (fifo_size && env->tx_cnt < env->tx_num) {
        om_i2c->DATA_CMD = env->tx_buf[env->tx_cnt];
        env->tx_cnt++;
        fifo_size--;
    }

    env->busy |= I2C_TX_BUSY;

    om_i2c->INTR_MASK = 0;
    om_i2c->INTR_MASK = I2C_INTR_TX_EMPTY_MASK | I2C_INTR_TX_ABRT_MASK | I2C_INTR_TIME_OUT_MASK;

    main_clk =  drv_rcc_clock_get(RCC_CLK_MAIN);//RCC_CLK_I2C0
    timeout_count=timeout_ms * (main_clk / 1000);//max=33554ms
    om_i2c->TIMEOUT = (timeout_count == 0) ? 0U : (I2C_TIMEOUT_EN_TIMEOUT_MASK | timeout_count);
    OM_CRITICAL_END();

    return OM_ERROR_OK;
}

#if (RTE_GPDMA)
om_error_t drv_i2c_gpdma_channel_allocate(OM_I2C_Type *om_i2c, drv_gpdma_chan_t channel)
{
    const drv_resource_t  *resource;
    drv_env_t             *env;
    gpdma_config_t         dma_cfg;
    om_error_t             error;

    OM_ASSERT(channel & DRV_GPDMA_CHAN_ALL);
    resource = i2c_get_resource(om_i2c);
    if (resource == NULL) {
        return OM_ERROR_PARAMETER;
    }

    OM_ASSERT(resource->cap & CAP_I2C_GPDMA_MASK);
    env = (drv_env_t *)resource->env;
    error = OM_ERROR_OK;

    if ((channel & DRV_GPDMA_RX_CHAN) && (env->gpdma_rx_chan >= GPDMA_NUMBER_OF_CHANNELS)) {
        env->gpdma_rx_chan = drv_gpdma_channel_allocate();
        if (env->gpdma_rx_chan >= GPDMA_NUMBER_OF_CHANNELS) {
            return OM_ERROR_RESOURCES;
        }
        dma_cfg.channel_ctrl = GPDMA_SET_CTRL(GPDMA_ADDR_CTRL_FIXED,
                                              GPDMA_ADDR_CTRL_INC,
                                              GPDMA_TRANS_WIDTH_1B,
                                              GPDMA_TRANS_WIDTH_1B,
                                              GPDMA_BURST_SIZE_1T,
                                              resource->gpdma_rx.prio ? GPDMA_PRIORITY_HIGH : GPDMA_PRIORITY_LOW);
        dma_cfg.src_id       = (gpdma_id_t)resource->gpdma_rx.id;
        dma_cfg.dst_id       = GPDMA_ID_MEM;
        dma_cfg.chain_trans  = NULL;
        dma_cfg.isr_cb       = i2c_gpdma_rx_isr_cb;
        dma_cfg.cb_param     = (void *)resource;
        dma_cfg.chain_trans_num = 0U;
        error = drv_gpdma_channel_config(env->gpdma_rx_chan, &dma_cfg);
        OM_ASSERT (error == OM_ERROR_OK);
        om_i2c->DMA_CR    |= I2C_DMA_CR_RDMAE_MASK;
        om_i2c->DMA_RDLR   = 0U;     // when rxfifo entries >= 1, assert dma_rx_req
    }
    if ((channel & DRV_GPDMA_TX_CHAN) && (env->gpdma_tx_chan >= GPDMA_NUMBER_OF_CHANNELS)) {
        env->gpdma_tx_chan = drv_gpdma_channel_allocate();
        if (env->gpdma_tx_chan >= GPDMA_NUMBER_OF_CHANNELS) {
            if (channel & DRV_GPDMA_RX_CHAN) {
                drv_gpdma_channel_release(env->gpdma_rx_chan);
                env->gpdma_rx_chan = GPDMA_NUMBER_OF_CHANNELS;
            }
            return OM_ERROR_RESOURCES;
        }
        dma_cfg.channel_ctrl = GPDMA_SET_CTRL(GPDMA_ADDR_CTRL_INC,
                                              GPDMA_ADDR_CTRL_FIXED,
                                              GPDMA_TRANS_WIDTH_1B,
                                              GPDMA_TRANS_WIDTH_1B,
                                              GPDMA_BURST_SIZE_1T,
                                              resource->gpdma_tx.prio ? GPDMA_PRIORITY_HIGH : GPDMA_PRIORITY_LOW);
        dma_cfg.src_id       = GPDMA_ID_MEM;
        dma_cfg.dst_id       = (gpdma_id_t)resource->gpdma_tx.id;
        dma_cfg.chain_trans  = NULL;
        dma_cfg.isr_cb       = i2c_gpdma_tx_isr_cb;
        dma_cfg.cb_param     = (void *)resource;
        dma_cfg.chain_trans_num = 0U;
        error = drv_gpdma_channel_config(env->gpdma_tx_chan, &dma_cfg);
        OM_ASSERT(error == OM_ERROR_OK);
        om_i2c->DMA_CR   |= I2C_DMA_CR_TDMAE_MASK;
        om_i2c->DMA_TDLR  = 10U;     // when txfifo entries <= 10, assert dma_tx_req
    }

    return error;
}

om_error_t drv_i2c_gpdma_channel_release(OM_I2C_Type *om_i2c, drv_gpdma_chan_t channel)
{
    const drv_resource_t  *resource;
    drv_env_t             *env;

    resource = i2c_get_resource(om_i2c);
    if (resource == NULL) {
        return OM_ERROR_PARAMETER;
    }
    env = (drv_env_t *)resource->env;

    OM_ASSERT(channel <= DRV_GPDMA_CHAN_ALL);

    if ((channel & DRV_GPDMA_RX_CHAN) && (env->gpdma_rx_chan < GPDMA_NUMBER_OF_CHANNELS)) {
        drv_gpdma_channel_release(env->gpdma_rx_chan);
        env->gpdma_rx_chan = GPDMA_NUMBER_OF_CHANNELS;
    }
    if ((channel & DRV_GPDMA_TX_CHAN) && (env->gpdma_tx_chan < GPDMA_NUMBER_OF_CHANNELS)) {
        drv_gpdma_channel_release(env->gpdma_tx_chan);
        env->gpdma_tx_chan = GPDMA_NUMBER_OF_CHANNELS;
    }

    return OM_ERROR_OK;
}

om_error_t drv_i2c_master_write_dma(OM_I2C_Type *om_i2c, uint16_t dev_addr, const uint8_t *tx_data, uint32_t tx_num, uint32_t timeout_ms)
{
    const drv_resource_t  *resource;
    drv_env_t             *env;
    om_error_t             error;
	uint32_t main_clk;
	uint32_t timeout_count;

    OM_ASSERT(tx_num);
    resource = i2c_get_resource(om_i2c);
    if (resource == NULL) {
        return OM_ERROR_PARAMETER;
    }
    env = (drv_env_t *)resource->env;
    if (env->gpdma_tx_chan >= GPDMA_NUMBER_OF_CHANNELS) {
        return OM_ERROR_UNSUPPORTED;
    }

    om_i2c->INTR_MASK = 0;

    main_clk =  drv_rcc_clock_get(RCC_CLK_MAIN);//RCC_CLK_I2C0
    timeout_count=timeout_ms * (main_clk / 1000);//max=33554ms
    if (timeout_count == 0) {
        om_i2c->TIMEOUT = 0;
    } else {
        om_i2c->TIMEOUT = (I2C_TIMEOUT_EN_TIMEOUT_MASK | timeout_count);
    }

    env->tx_buf     = (uint8_t *)tx_data;
    env->tx_num     = tx_num;
    om_i2c->ENABLE  = I2C_DISABLE;
    om_i2c->TAR     = dev_addr;
    om_i2c->CON1    = I2C_CON1_TX_ENABLE;

    OM_CRITICAL_BEGIN();
    error = drv_gpdma_channel_enable(env->gpdma_tx_chan, (uint32_t)&om_i2c->DATA_CMD, (uint32_t)env->tx_buf, env->tx_num);
    OM_ASSERT(error == OM_ERROR_OK);
    if (error == OM_ERROR_OK) {
        env->busy |= I2C_TX_BUSY;
    }
    OM_CRITICAL_END();
    om_i2c->ENABLE = I2C_ENABLE;

    return error;
}
#endif

om_error_t drv_i2c_master_read(OM_I2C_Type *om_i2c, uint16_t dev_addr, const uint8_t *tx_data, uint16_t tx_num, uint8_t *rx_data, uint16_t rx_num, uint32_t timeout_ms)
{
    const drv_resource_t *resource;
    uint32_t irq_save;
    uint32_t start  = drv_dwt_get_cycle();
    uint32_t target = DRV_DWT_MS_2_CYCLES_CEIL(timeout_ms);

    resource = i2c_get_resource(om_i2c);
    if (resource == NULL) {
        return OM_ERROR_PARAMETER;
    }

    om_i2c->ENABLE      = I2C_DISABLE;
    om_i2c->TAR         = dev_addr;
    om_i2c->CON1        = I2C_CON1_TX_ENABLE;
    om_i2c->ENABLE      = I2C_ENABLE;
    om_i2c->INTR_MASK = 0;

    OM_CRITICAL_BEGIN_EX(irq_save);
    for (uint32_t i = 0; i < tx_num; i++) {
        while (!(om_i2c->STATUS & I2C_STATUS_TFNF_MASK)) {
            if ((drv_dwt_get_cycle() - start) > target) {
                OM_CRITICAL_END_EX(irq_save);
                return OM_ERROR_TIMEOUT;
            }
        }
        om_i2c->DATA_CMD = tx_data[i];
    }
    while (!(om_i2c->STATUS & I2C_STATUS_TFE_MASK)) {
        if ((drv_dwt_get_cycle() - start) > target) {
            OM_CRITICAL_END_EX(irq_save);
            return OM_ERROR_TIMEOUT;
        }
    }
    om_i2c->CON1 = (rx_num | I2C_CON1_RX_ENABLE | I2C_CON1_READBYTES_UPDATE);
    for (uint32_t i = 0; i < rx_num; i++) {
        while (!(om_i2c->STATUS & I2C_STATUS_RFNE_MASK)) {
            if ((drv_dwt_get_cycle() - start) > target) {
                OM_CRITICAL_END_EX(irq_save);
                return OM_ERROR_TIMEOUT;
            }
        }
        rx_data[i] = om_i2c->DATA_CMD;
    }
    OM_CRITICAL_END_EX(irq_save);

    while ((om_i2c->STATUS & I2C_STATUS_RFNE_MASK) || (om_i2c->STATUS & I2C_STATUS_MST_ACTIVITY_MASK)) {
        if ((drv_dwt_get_cycle() - start) > target) {
            return OM_ERROR_TIMEOUT;
        }
    }

    return OM_ERROR_OK;
}

om_error_t drv_i2c_master_read_int(OM_I2C_Type *om_i2c, uint16_t dev_addr, const uint8_t *tx_data, uint16_t tx_num, uint8_t *rx_data, uint16_t rx_num, uint32_t timeout_ms)
{
    const drv_resource_t *resource;
    drv_env_t            *env;
    uint32_t              fifo_size;
    uint32_t main_clk;
    uint32_t timeout_count;

    resource = i2c_get_resource(om_i2c);
    if (resource == NULL) {
        return OM_ERROR_PARAMETER;
    }
    env = (drv_env_t *)resource->env;
    fifo_size = register_get(&resource->cap, MASK_POS(CAP_I2C_FIFO_LEVEL));

    env->tx_buf = (uint8_t *)tx_data;
    env->tx_num = tx_num;
    env->tx_cnt = 0U;
    env->rx_buf = (uint8_t *)rx_data;
    env->rx_num = rx_num;
    env->rx_cnt = 0U;

    om_i2c->ENABLE = I2C_DISABLE;
    om_i2c->TAR    = dev_addr;
    om_i2c->CON1   = I2C_CON1_TX_ENABLE;
    om_i2c->ENABLE = I2C_ENABLE;

    OM_CRITICAL_BEGIN();
    while (fifo_size && env->tx_cnt < env->tx_num) {
        om_i2c->DATA_CMD = env->tx_buf[env->tx_cnt];
        env->tx_cnt++;
        fifo_size--;
    }

    env->busy |= I2C_TX_BUSY | I2C_RX_BUSY;

    om_i2c->INTR_MASK = 0;
    om_i2c->INTR_MASK = I2C_INTR_TX_EMPTY_MASK | I2C_INTR_TX_ABRT_MASK | I2C_INTR_RX_UNDER_MASK | I2C_INTR_TIME_OUT_MASK;

    main_clk =  drv_rcc_clock_get(RCC_CLK_MAIN);//RCC_CLK_I2C0
    timeout_count=timeout_ms * (main_clk / 1000);//max=33554ms
    om_i2c->TIMEOUT =  (timeout_count == 0) ? 0U : (I2C_TIMEOUT_EN_TIMEOUT_MASK | timeout_count);
    OM_CRITICAL_END();

    return OM_ERROR_OK;
}

#if (RTE_GPDMA)
om_error_t drv_i2c_master_read_dma(OM_I2C_Type *om_i2c, uint16_t dev_addr, const uint8_t *tx_data, uint16_t tx_num, uint8_t *rx_data, uint16_t rx_num, uint32_t timeout_ms)
{
    const drv_resource_t *resource;
    drv_env_t            *env;
    om_error_t            error;
    uint32_t              main_clk;
    uint32_t              timeout_count;

    OM_ASSERT(rx_num);
    resource = i2c_get_resource(om_i2c);
    if (resource == NULL) {
        return OM_ERROR_PARAMETER;
    }
    env = (drv_env_t *)resource->env;

    om_i2c->INTR_MASK = 0;

    main_clk =  drv_rcc_clock_get(RCC_CLK_MAIN);//RCC_CLK_I2C0
    timeout_count=timeout_ms * (main_clk / 1000);//max=33554ms
    if (timeout_count == 0) {
        om_i2c->TIMEOUT = 0;
    } else {
        om_i2c->TIMEOUT = (I2C_TIMEOUT_EN_TIMEOUT_MASK | timeout_count);
    }

    if (env->gpdma_rx_chan <= GPDMA_NUMBER_OF_CHANNELS) {
        om_i2c->ENABLE          = I2C_DISABLE;
        om_i2c->TAR             = dev_addr;

        env->tx_buf  = (uint8_t *)tx_data;
        env->tx_num  = tx_num;
        env->rx_buf  = (uint8_t *)rx_data;
        env->rx_num  = rx_num;
        env->rx_cnt  = 0U;
        om_i2c->CON1 = I2C_CON1_RX_ENABLE;

        if (env->tx_num == 0U) {
            om_i2c->CON1 = (env->rx_num | I2C_CON1_RX_ENABLE | I2C_CON1_READBYTES_UPDATE);
            error = drv_gpdma_channel_enable(env->gpdma_rx_chan, (uint32_t)env->rx_buf, (uint32_t)&om_i2c->DATA_CMD, env->rx_num);
            if (error == OM_ERROR_OK) {
                env->busy |= I2C_RX_BUSY;
            }
            om_i2c->ENABLE = I2C_ENABLE;
        } else {
            if (env->gpdma_tx_chan <= GPDMA_NUMBER_OF_CHANNELS) {
                om_i2c->CON1 = I2C_CON1_TX_ENABLE;
                error = drv_gpdma_channel_enable(env->gpdma_tx_chan, (uint32_t)&om_i2c->DATA_CMD, (uint32_t)env->tx_buf, env->tx_num);
                if (error == OM_ERROR_OK) {
                    env->busy |= I2C_TX_BUSY | I2C_RX_BUSY;
                } else {
                    return error;
                }
                om_i2c->ENABLE = I2C_ENABLE;
            }
        }
    } else {
        return OM_ERROR_UNSUPPORTED;
    }

    return OM_ERROR_OK;
}
#endif

om_error_t drv_i2c_slave_write(OM_I2C_Type *om_i2c, uint16_t dev_addr, const uint8_t *tx_data, uint16_t tx_num, uint32_t timeout_ms)
{
    uint32_t dummy;
    uint16_t len;
    uint16_t fifo_padding;
    uint16_t fifo_size;
    uint16_t offset;
    const drv_resource_t *resource;
    uint32_t tx_tl;
    uint32_t irq_save;
    uint32_t start  = drv_dwt_get_cycle();
    uint32_t target = DRV_DWT_MS_2_CYCLES_CEIL(timeout_ms);

    resource = i2c_get_resource(om_i2c);
    if(resource == NULL) {
        return OM_ERROR_PARAMETER;
    }

    OM_ASSERT(tx_num);
    om_i2c->ENABLE      = 0;
    om_i2c->SAR         = dev_addr;
    om_i2c->ENABLE      = 1;

    offset = 0U;
    fifo_size = register_get(&resource->cap, MASK_POS(CAP_I2C_FIFO_LEVEL));

    while (!(om_i2c->RAW_INTR_STAT & I2C_INTR_RD_REQ_MASK)) {
        if ((drv_dwt_get_cycle() - start) > target) {
            return OM_ERROR_TIMEOUT;
        }
    }
    dummy = om_i2c->CLR_RD_REQ;
    (void)dummy;
    tx_tl = om_i2c->TX_TL;

    OM_CRITICAL_BEGIN_EX(irq_save);
    while (tx_num) {
        fifo_padding = fifo_size - om_i2c->TXFLR;
        len = tx_num > fifo_padding ? fifo_padding : tx_num;
        for (uint32_t i = 0; i < len; i++) {
            om_i2c->DATA_CMD = tx_data[offset + i];
        }
        while (om_i2c->TXFLR > tx_tl) {
            if ((drv_dwt_get_cycle() - start) > target) {
                OM_CRITICAL_END_EX(irq_save);
                return OM_ERROR_TIMEOUT;
            }
        }
        offset += len;
        tx_num -= len;
    }
    OM_CRITICAL_END_EX(irq_save);
    while (!(om_i2c->RAW_INTR_STAT & I2C_INTR_STOP_DET_MASK)) {
        if ((drv_dwt_get_cycle() - start) > target) {
            return OM_ERROR_TIMEOUT;
        }
    }

    return OM_ERROR_OK;
}


om_error_t drv_i2c_slave_write_int(OM_I2C_Type *om_i2c, uint16_t dev_addr, const uint8_t *tx_data, uint16_t tx_num)
{
    const drv_resource_t  *resource;
    drv_env_t             *env;

    OM_ASSERT(tx_num);
    resource = i2c_get_resource(om_i2c);
    if(resource == NULL) {
        return OM_ERROR_PARAMETER;
    }
    env = (drv_env_t *)(resource->env);

    env->tx_buf         = (uint8_t *)tx_data;
    env->tx_num         = tx_num;
    env->tx_cnt         = 0U;

    om_i2c->ENABLE      = I2C_DISABLE;
    om_i2c->SAR         = dev_addr;
    om_i2c->ENABLE      = I2C_ENABLE;

    env->busy |= I2C_TX_BUSY;

    // Enable Rx interrupts
    om_i2c->INTR_MASK = I2C_INTR_RD_REQ_MASK;

    return OM_ERROR_OK;
}

#if (RTE_GPDMA)
om_error_t drv_i2c_slave_write_dma(OM_I2C_Type *om_i2c, uint16_t dev_addr, const uint8_t *tx_data, uint16_t tx_num)
{
    const drv_resource_t  *resource;
    drv_env_t             *env;
    om_error_t             error;
    uint32_t               dummy;

    OM_ASSERT(tx_num);
    resource = i2c_get_resource(om_i2c);
    if(resource == NULL) {
        return OM_ERROR_PARAMETER;
    }
    env = (drv_env_t *)(resource->env);
    if (env->gpdma_tx_chan >= GPDMA_NUMBER_OF_CHANNELS) {
        return OM_ERROR_UNSUPPORTED;
    }

    om_i2c->INTR_MASK = 0;

    env->tx_buf     = (uint8_t *)tx_data;
    env->tx_num     = tx_num;
    env->tx_cnt     = 0U;
    om_i2c->ENABLE  = I2C_DISABLE;
    om_i2c->SAR     = dev_addr;
    om_i2c->ENABLE  = I2C_ENABLE;

    while (!(om_i2c->RAW_INTR_STAT & I2C_INTR_RD_REQ_MASK));
    dummy = om_i2c->CLR_RD_REQ;
    (void)dummy;

    OM_CRITICAL_BEGIN();
    error = drv_gpdma_channel_enable(env->gpdma_tx_chan, (uint32_t)&om_i2c->DATA_CMD, (uint32_t)env->tx_buf, env->tx_num);
    OM_ASSERT(error == OM_ERROR_OK);
    if (error == OM_ERROR_OK) {
        env->busy |= I2C_TX_BUSY;
    }
    OM_CRITICAL_END();

    return error;
}
#endif  /* (RTE_GPDMA) */

om_error_t drv_i2c_slave_read(OM_I2C_Type *om_i2c, uint16_t dev_addr, uint8_t *rx_data, uint16_t rx_num, uint32_t timeout_ms)
{
    const drv_resource_t   *resource;
    uint32_t start  = drv_dwt_get_cycle();
    uint32_t target = DRV_DWT_MS_2_CYCLES_CEIL(timeout_ms);

    OM_ASSERT(rx_num);
    resource = i2c_get_resource(om_i2c);
    if(resource == NULL) {
        return OM_ERROR_PARAMETER;
    }

    om_i2c->INTR_MASK = 0;

    om_i2c->ENABLE  = I2C_DISABLE;
    om_i2c->SAR     = dev_addr;
    om_i2c->ENABLE  = I2C_ENABLE;

    for (uint32_t i = 0; i < rx_num; i++) {
        while (!(om_i2c->STATUS & I2C_STATUS_RFNE_MASK)) {
            if ((drv_dwt_get_cycle() - start) > target) {
                return OM_ERROR_TIMEOUT;
            }
        }
        rx_data[i] = om_i2c->DATA_CMD;
    }
    while (!(om_i2c->RAW_INTR_STAT & I2C_INTR_STOP_DET_MASK)) {
        if ((drv_dwt_get_cycle() - start) > target) {
            return OM_ERROR_TIMEOUT;
        }
    }

    return OM_ERROR_OK;
}

om_error_t drv_i2c_slave_read_int(OM_I2C_Type *om_i2c, uint16_t dev_addr, uint8_t *rx_data, uint16_t rx_num)
{
    const drv_resource_t  *resource;
    drv_env_t             *env;
    uint32_t dummy;

    OM_ASSERT(rx_num);
    resource = i2c_get_resource(om_i2c);
    if(resource == NULL) {
        return OM_ERROR_PARAMETER;
    }
    env = (drv_env_t *)(resource->env);

    env->rx_buf = (uint8_t *)rx_data;
    env->rx_num = rx_num;
    env->rx_cnt = 0U;

    dummy = om_i2c->CLR_STOP_DET;
    (void)dummy;

    om_i2c->ENABLE  = I2C_DISABLE;
    om_i2c->SAR     = dev_addr;
    om_i2c->ENABLE  = I2C_ENABLE;

    env->busy |= I2C_RX_BUSY;

    om_i2c->INTR_MASK = I2C_INTR_RX_FULL_MASK | I2C_INTR_STOP_DET_MASK;

    return OM_ERROR_OK;
}

#if (RTE_GPDMA)
om_error_t drv_i2c_slave_read_dma(OM_I2C_Type *om_i2c, uint16_t dev_addr, uint8_t *rx_data, uint16_t rx_num)
{
    const drv_resource_t  *resource;
    drv_env_t             *env;
    om_error_t             error;

    // check input parameter
    OM_ASSERT(rx_num);
    resource = i2c_get_resource(om_i2c);
    if(resource == NULL) {
        return OM_ERROR_PARAMETER;
    }
    env = (drv_env_t *)(resource->env);

    om_i2c->INTR_MASK = 0;

    env->rx_buf = (uint8_t *)rx_data;
    env->rx_num = rx_num;
    env->rx_cnt = 0U;

    om_i2c->ENABLE  = I2C_DISABLE;
    om_i2c->SAR     = dev_addr;
    om_i2c->ENABLE  = I2C_ENABLE;

    error = drv_gpdma_channel_enable(env->gpdma_rx_chan, (uint32_t)env->rx_buf, (uint32_t)&om_i2c->DATA_CMD, env->rx_num);
    if (error == OM_ERROR_OK) {
        env->busy |= I2C_RX_BUSY;
    }

    return error;
}
#endif  /* (RTE_GPDMA) */

void *drv_i2c_control(OM_I2C_Type *om_i2c, i2c_control_t control, void *argu)
{
    const drv_resource_t    *resource;
    uint32_t                 ret;
    drv_env_t               *env;

    resource = i2c_get_resource(om_i2c);
    if (resource == NULL) {
        return (void *)OM_ERROR_PARAMETER;
    }
    env = (drv_env_t *)resource->env;
    ret = (uint32_t)OM_ERROR_OK;

    OM_CRITICAL_BEGIN();
    switch (control) {
        case I2C_CONTROL_IS_BUSY:
            ret = env->busy;
            break;
        case I2C_CONTROL_DEV_IS_VALID:
            ret = i2c_dev_is_valid(om_i2c, (uint32_t)argu);
            break;
        default:
            break;
    }
    OM_CRITICAL_END();

    return (void *)ret;
}

void drv_i2c_isr(OM_I2C_Type *om_i2c)
{
    uint16_t fifo_size;
    uint16_t fifo_padding;
    uint16_t len;
    uint32_t int_status;
    uint32_t int_status_source;
    uint32_t int_status_clr;
    drv_env_t *env;
    const drv_resource_t *resource;

    resource = i2c_get_resource(om_i2c);
    if (resource == NULL) {
        return;
    }
    env = (drv_env_t *)resource->env;

    fifo_size = register_get(&resource->cap, MASK_POS(CAP_I2C_FIFO_LEVEL));
    int_status = om_i2c->INTR_STAT;
    int_status_source = om_i2c->TX_ABRT_SOURCE;

    if (int_status & I2C_INTR_RD_REQ_MASK) {
        int_status_clr = om_i2c->CLR_RD_REQ;
        for (uint32_t i = 0; i < env->tx_num; i++) {
            while (!(om_i2c->STATUS & I2C_STATUS_TFNF_MASK));
            om_i2c->DATA_CMD = env->tx_buf[env->tx_cnt++];
        }
        om_i2c->INTR_MASK &= ~I2C_INTR_RD_REQ_MASK;

        if (env->tx_num == env->tx_cnt) {
            om_i2c->INTR_MASK &= ~I2C_INTR_RD_REQ_MASK;

            env->busy &= ~I2C_TX_BUSY;
            if (!(env->busy & I2C_RX_BUSY)) {
                drv_i2c_isr_callback(om_i2c, DRV_EVENT_COMMON_WRITE_COMPLETED, env->tx_buf, env->tx_cnt);
            }
        }
    }
    if (int_status & I2C_INTR_TIME_OUT_MASK) {
        int_status_clr = om_i2c->CLR_TIME_OUT;
        if ((env->busy & I2C_TX_BUSY) == 0x1) {
            env->busy &= ~I2C_TX_BUSY;
            env->tx_buf = NULL;
            env->tx_num = 0U;
            env->tx_cnt = 0U;
        }
        if ((env->busy & I2C_RX_BUSY) == 0x10) {
            env->busy &= ~I2C_RX_BUSY;
            env->rx_buf = NULL;
            env->rx_num = 0U;
            env->rx_cnt = 0U;
        }
        drv_i2c_isr_callback(om_i2c, DRV_EVENT_I2C_TIMEOUT, NULL, 0);
    }
    if ((int_status & I2C_INTR_TX_ABRT_MASK)&&(int_status_source & (I2C_TX_ABRT_SRC_7B_ADDR_NOACK_MASK | I2C_TX_ABRT_SRC_10ADDR1_NOACK_MASK | I2C_TX_ABRT_SRC_10ADDR2_NOACK_MASK | I2C_TX_ABRT_SRC_TXDATA_NOACK_MASK))) {
        int_status_clr = om_i2c->CLR_TX_ABRT;
        if ((env->busy & I2C_TX_BUSY) == 0x1) {
            env->busy &= ~I2C_TX_BUSY;
            env->tx_buf = NULL;
            env->tx_num = 0U;
            env->tx_cnt = 0U;
        }
        if ((env->busy & I2C_RX_BUSY) == 0x10) {
            env->busy &= ~I2C_RX_BUSY;
            env->rx_buf = NULL;
            env->rx_num = 0U;
            env->rx_cnt = 0U;
        }
        if (int_status_source & (I2C_TX_ABRT_SRC_7B_ADDR_NOACK_MASK | I2C_TX_ABRT_SRC_10ADDR1_NOACK_MASK | I2C_TX_ABRT_SRC_10ADDR2_NOACK_MASK)) {
            drv_i2c_isr_callback(om_i2c, (drv_event_t)((uint32_t)DRV_EVENT_I2C_TXADDR_NACK + (uint32_t)DRV_EVENT_I2C_RXADDR_NACK), NULL, int_status_source);
        }
        if (int_status_source & I2C_TX_ABRT_SRC_TXDATA_NOACK_MASK) {
            drv_i2c_isr_callback(om_i2c, DRV_EVENT_I2C_TXDATA_NACK, NULL, int_status_source);
        }
    }
    if (int_status & I2C_INTR_RX_UNDER_MASK) {
        int_status_clr = om_i2c->CLR_RX_UNDER;
        if ((env->busy & I2C_RX_BUSY) == 0x10) {
            env->busy &= ~I2C_RX_BUSY;
            env->rx_buf = NULL;
            env->rx_num = 0U;
            env->rx_cnt = 0U;
            drv_i2c_isr_callback(om_i2c, DRV_EVENT_I2C_RXDATA_UNDER, NULL, 0);
        }
    }

    if (int_status & I2C_INTR_TX_EMPTY_MASK) {
        if (env->tx_cnt == env->tx_num) {
            om_i2c->INTR_MASK &= ~I2C_INTR_TX_EMPTY_MASK;
            while (!(om_i2c->STATUS & I2C_STATUS_TFE_MASK));

            env->busy &= ~I2C_TX_BUSY;

            if (!(env->busy & I2C_RX_BUSY)) {
                drv_i2c_isr_callback(om_i2c, DRV_EVENT_COMMON_WRITE_COMPLETED, env->tx_buf, env->tx_cnt);
            }

            // enable receive
            if (env->rx_cnt < env->rx_num) {
                om_i2c->CON1 = (env->rx_num | I2C_CON1_RX_ENABLE | I2C_CON1_READBYTES_UPDATE);
                om_i2c->INTR_MASK |= I2C_INTR_RX_FULL_MASK;
            }
        } else {
            fifo_padding = fifo_size - om_i2c->TXFLR;
            len = env->tx_num - env->tx_cnt;
            len = len > fifo_padding ? fifo_padding : len;
            for (uint32_t i = 0; i < len; i++) {
                om_i2c->DATA_CMD = env->tx_buf[env->tx_cnt++];
            }
        }
    }

    if (int_status & I2C_INTR_RX_FULL_MASK) {
        while (om_i2c->STATUS & I2C_STATUS_RFNE_MASK && env->rx_cnt < env->rx_num) {
            env->rx_buf[env->rx_cnt++] = om_i2c->DATA_CMD;
        }
        if (env->rx_cnt == env->rx_num) {
            om_i2c->INTR_MASK &= ~I2C_INTR_RX_FULL_MASK;
            env->busy &= ~I2C_RX_BUSY;

            drv_i2c_isr_callback(om_i2c, DRV_EVENT_COMMON_READ_COMPLETED, env->rx_buf, env->rx_cnt);
        }
    }

    if (int_status & I2C_INTR_STOP_DET_MASK) {
        int_status_clr = om_i2c->CLR_STOP_DET;
        om_i2c->INTR_MASK &= ~I2C_INTR_STOP_DET_MASK;
        if ((env->busy & I2C_RX_BUSY) && (env->rx_cnt < env->rx_num)) {
            om_i2c->INTR_MASK &= ~I2C_INTR_RX_FULL_MASK;
            env->busy &= ~I2C_RX_BUSY;
            env->rx_buf = NULL;
            env->rx_num = 0U;
            env->rx_cnt = 0U;
            drv_i2c_isr_callback(om_i2c, DRV_EVENT_I2C_STOP_DET, env->rx_buf, env->rx_cnt);
        }
    }

    (void)int_status_clr;
}


#endif  /* (RTE_I2C0 || RTE_I2C1) */

/** @} */
