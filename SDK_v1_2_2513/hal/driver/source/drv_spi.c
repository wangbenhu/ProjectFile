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
 * @brief    SPI driver source file
 * @details  SPI driver source file
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
#if (RTE_SPI0 || RTE_SPI1)
#include <stddef.h>
#include "om_device.h"
#include "om_driver.h"

/*
Note:
When the SPI operates as a slave device, if the clock of the other host does not
have an interval, since the SPI module needs at least 3 bus cycles to sample and
latch data into the FIFO, it is necessary to ensure that the bus clock of the SPI
module (i.e., the CPU clock) is at least 4 times the communication clock;
otherwise, the host needs to increase the communication clock interval.
When the bus clock of the SPI slave is twice the communication clock,
the host needs to increase the communication clock interval time equivalent to
bus cycles of the slave SPI. When it is three times, the host needs to
increase the communication clock interval time by 1 bus cycle.
*/
/*******************************************************************************
 * MACROS
 */
#define SPI_TX_DUMMY                0xFFU
#define SPI_FIFO_SIZE               32U

#if (RTE_SPI_CSN_MANUAL_CONTROL)
#define SPI_CSN_LOW(OM_SPI)
#define SPI_CSN_HIGH(OM_SPI)
#else
#define SPI_CSN_LOW(OM_SPI)    drv_spi_control(OM_SPI, SPI_CONTROL_CSN, (void *)0U)
#define SPI_CSN_HIGH(OM_SPI)   drv_spi_control(OM_SPI, SPI_CONTROL_CSN, (void *)1U)
#endif

/*
 * Used to define spi_env_t and drv_resource_t structures
 */
#define DRV_SPI_DEFINE(NAMEn, namen)                                           \
static spi_env_t namen##_env = {                                               \
    .isr_cb = NULL,                                                            \
    .tx_num = 0,                                                               \
    .tx_cnt = 0,                                                               \
    .tx_buf = NULL,                                                            \
    .rx_num = 0,                                                               \
    .rx_cnt = 0,                                                               \
    .rx_buf = NULL,                                                            \
    .state  = SPI_STATE_IDLE,                                                  \
};                                                                             \
static const drv_resource_t namen##_resource = {                               \
    .cap      = CAP_##NAMEn,                                                   \
    .reg      = OM_##NAMEn,                                                    \
    .env      = (void *)&namen##_env,                                          \
    .irq_num  = NAMEn##_IRQn,                                                  \
    .irq_prio = RTE_##NAMEn##_IRQ_PRIORITY,                                    \
    .gpdma_tx = {                                                              \
        .id   = GPDMA_ID_##NAMEn##_TX,                                         \
        .prio = 0,                                                             \
    },                                                                         \
    .gpdma_rx = {                                                              \
        .id   = GPDMA_ID_##NAMEn##_RX,                                         \
        .prio = 0,                                                             \
    },                                                                         \
}


/*******************************************************************************
 * TYPEDEFS
 */
typedef enum {
    SPI_STATE_IDLE = 0U,
    SPI_STATE_TX   = (1U << 0),
    SPI_STATE_RX   = (1U << 1),
    SPI_STATE_TRX  = (SPI_STATE_TX | SPI_STATE_RX),
} spi_state_t;

typedef struct {
    drv_isr_callback_t           isr_cb;
    uint16_t                     tx_num;
    uint16_t                     tx_cnt;
    uint8_t                     *tx_buf;
    uint16_t                     rx_num;
    uint16_t                     rx_cnt;
    uint8_t                     *rx_buf;
    spi_state_t                  state;
    #if (RTE_GPDMA)
    uint8_t                      gpdma_tx_chan;
    uint8_t                      gpdma_rx_chan;
    #endif
} spi_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
/* spi information */
#if (RTE_SPI0)
DRV_SPI_DEFINE(SPI0, spi0);
#endif
#if (RTE_SPI1)
DRV_SPI_DEFINE(SPI1, spi1);
#endif


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static const drv_resource_t *spi_get_resource(OM_SPI_Type *om_spi)
{
    static const drv_resource_t *resources[] = {
        #if (RTE_SPI0)
        &spi0_resource,
        #endif
        #if (RTE_SPI1)
        &spi1_resource,
        #endif
    };

    for(uint32_t i=0; i<sizeof(resources)/sizeof(resources[0]); i++) {
        if ((uint32_t)om_spi == (uint32_t)resources[i]->reg) {
            return resources[i];
        }
    }

    OM_ASSERT(0);
    return NULL;
}

static inline void spi_wait_tx_completed(OM_SPI_Type *om_spi)
{
    while (!(om_spi->STAT & SPI_STAT_TX_EMPTY_MASK));
}

static uint8_t spi_tx_fifo_is_full(OM_SPI_Type *om_spi)
{
    uint32_t tx_cnt;

    tx_cnt = (om_spi->STAT & SPI_STAT_TX_BYTE_CNT_MASK) >> SPI_STAT_TX_BYTE_CNT_POS;
    return (tx_cnt < (SPI_FIFO_SIZE - 1)) ? 0U : 1U;
}

static void spi_clear_rx_fifo(OM_SPI_Type *om_spi)
{
    uint32_t ctrl_reg;

    ctrl_reg = om_spi->CTRL;
    om_spi->CTRL = ctrl_reg | SPI_CTRL_RX_CLR_FIFO_MASK;
    om_spi->CTRL = ctrl_reg;
}

#if (RTE_GPDMA)
static om_error_t spi_transfer_dma(const drv_resource_t *resource, const uint8_t *tx_data, uint16_t tx_num, uint8_t *rx_data, uint16_t rx_num)
{
    spi_env_t  *env;
    om_error_t error;
    OM_SPI_Type *om_spi;

    env = (spi_env_t *)resource->env;
    om_spi = (OM_SPI_Type *)(resource->reg);
    if (env->state == SPI_STATE_RX) {
        om_spi->CTRL |= SPI_CTRL_ACTIVE_DO_ENL_MASK;
    } else {
        om_spi->CTRL &= ~SPI_CTRL_ACTIVE_DO_ENL_MASK;
    }

    if (rx_num) {
        om_spi->CTRL |= SPI_CTRL_RX_FIFO_EN_MASK;//when rx fifo enable, tx fifo not enable,and rx fifo is not full,the spi clock will be generated,so the tx fifo enable all the time
        error = drv_gpdma_channel_enable(env->gpdma_rx_chan, (uint32_t)rx_data, (uint32_t)(&om_spi->RDATA), rx_num);
        if (error != OM_ERROR_OK) {
            OM_ASSERT(0);
            return error;
        }
    } else {
        om_spi->CTRL &= ~SPI_CTRL_RX_FIFO_EN_MASK;//because the receiving fifo of the 3 wire does not clear after the master has finished sending, so the receiving fifo is turned off when sending
    }
    if (tx_num) {
        error = drv_gpdma_channel_enable(env->gpdma_tx_chan, (uint32_t)(&om_spi->WDATA), (tx_data == NULL) ? 0x20000000U : (uint32_t)tx_data, tx_num);
        if (error != OM_ERROR_OK) {
            OM_ASSERT(0);
            return error;
        }
    }

    return OM_ERROR_OK;
}

static void spi_gpdma_rx_isr_cb(void *resource, drv_event_t event, gpdma_chain_trans_t *next_chain)
{
    spi_role_t role;
    spi_env_t *env;
    OM_SPI_Type *om_spi;
    drv_event_t  drv_event;
    uint32_t ctrl_reg;
    uint8_t xfer_done;

    env = (spi_env_t *)(((const drv_resource_t *)resource)->env);
    if (env->state == SPI_STATE_IDLE) {
        return;
    }
    om_spi = (OM_SPI_Type *)(((const drv_resource_t *)resource)->reg);
    ctrl_reg = om_spi->CTRL;
    role = (ctrl_reg & SPI_CTRL_MASTER_EN_MASK) ? SPI_ROLE_MASTER : SPI_ROLE_SLAVE;
    xfer_done = 0;

    switch (event) {
        case DRV_GPDMA_EVENT_TERMINAL_COUNT_REQUEST:
            switch (env->state) {
                case SPI_STATE_TRX:
                    xfer_done = 1U;
                    drv_event = DRV_EVENT_COMMON_TRANSFER_COMPLETED;
                    break;
                case SPI_STATE_RX:
                    if ((role == SPI_ROLE_SLAVE) && env->tx_num) {
                        env->state = SPI_STATE_TX;
                        spi_transfer_dma(resource, env->tx_buf, env->tx_num, NULL, 0);
                    } else {
                        xfer_done = 1U;
                        drv_event = DRV_EVENT_COMMON_TRANSFER_COMPLETED;
                    }
                    break;
                default:
                    break;
            }
            break;
        case DRV_GPDMA_EVENT_ABORT:
            xfer_done = 1U;
            drv_event = DRV_EVENT_COMMON_ABORT;
            break;
        default:
            xfer_done = 1U;
            drv_event = DRV_EVENT_COMMON_ERROR;
            OM_ASSERT(0);
            break;
    }

    if (xfer_done) {
        env->state = SPI_STATE_IDLE;
        if (role == SPI_ROLE_MASTER) {
            SPI_CSN_HIGH(om_spi);
        }
        drv_spi_isr_callback(om_spi, drv_event, env->rx_buf, env->rx_num);
    }
}

static void spi_gpdma_tx_isr_cb(void *resource, drv_event_t event, gpdma_chain_trans_t *next_chain)
{
    spi_env_t   *env;
    OM_SPI_Type *om_spi;
    spi_role_t role;
    uint32_t   ctrl_reg;
    drv_event_t drv_event;
    uint8_t xfer_done;

    env = (spi_env_t *)(((const drv_resource_t *)resource)->env);
    if (env->state == SPI_STATE_IDLE) {
        return;
    }

    om_spi = (OM_SPI_Type *)(((const drv_resource_t *)resource)->reg);
    ctrl_reg = om_spi->CTRL;
    role = (ctrl_reg & SPI_CTRL_MASTER_EN_MASK) ? SPI_ROLE_MASTER : SPI_ROLE_SLAVE;
    xfer_done = 0;

    switch (event) {
        case DRV_GPDMA_EVENT_TERMINAL_COUNT_REQUEST:
            switch(env->state) {
                case SPI_STATE_TRX:
                    // if RX using DMA, processing in spi_gpdma_rx_isr_cb()
                    if (env->rx_num == 0U) {
                        xfer_done = 1U;
                        drv_event = DRV_EVENT_COMMON_TRANSFER_COMPLETED;
                    }
                    break;
                case SPI_STATE_TX:
                    if ((role == SPI_ROLE_MASTER) && env->rx_num) {
                        env->state = SPI_STATE_RX;
                        spi_wait_tx_completed(om_spi);
                        spi_clear_rx_fifo(om_spi);
                        spi_transfer_dma(resource, NULL, env->rx_num, env->rx_buf, env->rx_num);
                    } else {
                        xfer_done = 1U;
                        drv_event = DRV_EVENT_COMMON_TRANSFER_COMPLETED;
                    }
                    break;
                default:
                    break;
            }
            break;
        case DRV_GPDMA_EVENT_ABORT:
            drv_event = DRV_EVENT_COMMON_ABORT;
            xfer_done = 1U;
            break;
        default:
            drv_event = DRV_EVENT_COMMON_ERROR;
            xfer_done = 1U;
            OM_ASSERT(0);
            break;
    }

    if (xfer_done) {
        env->state = SPI_STATE_IDLE;
        spi_wait_tx_completed(om_spi);
        if (role == SPI_ROLE_MASTER) {
            SPI_CSN_HIGH(om_spi);
        }
        spi_clear_rx_fifo(om_spi);
        drv_spi_isr_callback(om_spi, drv_event, env->tx_buf, env->tx_num);
    }
}
#endif

static om_error_t spi_transfer(OM_SPI_Type *om_spi, const uint8_t *tx_data, uint8_t *rx_data, uint16_t trx_num, uint32_t timeout_ms)
{
    spi_wire_t   wire;
    uint16_t     rx_cnt, tx_cnt;
    uint32_t     start, target;

    OM_ASSERT(trx_num);
    tx_cnt = 0;
    rx_cnt = 0;
    wire = (om_spi->CTRL & SPI_CTRL_BIDIRECT_DATA_MASK) ? SPI_WIRE_3 : SPI_WIRE_4;
    if ((tx_data != NULL) || (wire == SPI_WIRE_4)) {
        om_spi->CTRL &= ~SPI_CTRL_ACTIVE_DO_ENL_MASK;
    } else {
        om_spi->CTRL |= SPI_CTRL_ACTIVE_DO_ENL_MASK;
    }

    if (timeout_ms < DRV_MAX_DELAY) {
        start = drv_dwt_get_cycle();
        target = DRV_DWT_MS_2_CYCLES_CEIL(timeout_ms);
    }
    while (rx_cnt < trx_num) {
        if ((tx_cnt < trx_num) && (!spi_tx_fifo_is_full(om_spi))) {
            om_spi->WDATA = tx_data ? tx_data[tx_cnt] : SPI_TX_DUMMY;
            tx_cnt++;
        }
        if (om_spi->STAT & SPI_STAT_RX_NOT_EMPTY_MASK) {
            uint8_t rdata;
            rdata = om_spi->RDATA;
            if (rx_data) {
                rx_data[rx_cnt] = rdata;
            }
            rx_cnt++;
        }
        if (timeout_ms < DRV_MAX_DELAY) {
            if ((drv_dwt_get_cycle() - start) > target) {
                return OM_ERROR_TIMEOUT;
            }
        }
    }

    return OM_ERROR_OK;
}

static om_error_t spi_transfer_int(const drv_resource_t *resource)
{
    OM_SPI_Type *om_spi;
    spi_env_t   *env;
    uint32_t     fifo_space;

    env = (spi_env_t *)(resource->env);
    om_spi = (OM_SPI_Type *)(resource->reg);
    fifo_space = SPI_FIFO_SIZE;
    OM_CRITICAL_BEGIN();
    om_spi->STAT = SPI_STAT_TX_EMPTY_INT_EN_MASK;
    switch (env->state) {
        case SPI_STATE_TX:
        case SPI_STATE_TRX:
            while ((env->tx_cnt < env->tx_num) && fifo_space) {
                om_spi->WDATA = env->tx_buf ? env->tx_buf[env->tx_cnt] : SPI_TX_DUMMY;
                env->tx_cnt++;
                fifo_space--;
            }
            break;
        case SPI_STATE_RX:
            while ((env->tx_cnt < env->rx_num) && fifo_space) {
                om_spi->WDATA = SPI_TX_DUMMY;
                env->tx_cnt++;
                fifo_space--;
            }
            break;
        default:
            break;
    }
    OM_CRITICAL_END();
    return OM_ERROR_OK;
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
om_error_t drv_spi_init(OM_SPI_Type *om_spi, spi_config_t *cfg)
{
    const drv_resource_t *resource;
    spi_env_t  *env;
    uint32_t    clk;

    OM_ASSERT(cfg);
    resource = spi_get_resource(om_spi);
    if (resource == NULL) {
        return OM_ERROR_PARAMETER;
    }
    OM_ASSERT(SPI_FIFO_SIZE == ((resource->cap & CAP_SPI_FIFO_LEVEL_MASK) >> CAP_SPI_FIFO_LEVEL_POS));
    DRV_RCC_RESET((rcc_clk_t)(size_t)resource->reg);
    clk = drv_rcc_clock_get((rcc_clk_t)(size_t)resource->reg);
    env = (spi_env_t *)resource->env;

    SPI_CSN_HIGH(om_spi);
    do {
        uint32_t ctrl_reg;

        // use cs as chip enable
        ctrl_reg = SPI_CTRL_TX_FIFO_EN_MASK | SPI_CTRL_RX_FIFO_EN_MASK;
        // config clock divider in master
        if (cfg->role == SPI_ROLE_MASTER) {
            uint16_t div;

            OM_ASSERT(cfg->freq);
            OM_ASSERT(cfg->freq <= clk / 2);
            div = clk / cfg->freq;
            register_set(&ctrl_reg, MASK_3REG(SPI_CTRL_CLK_DIVIDER, div / 2 - 1,
                                              SPI_CTRL_MASTER_EN, 1,
                                              SPI_CTRL_MASTER_CE_AT_END, 1));
            if (cfg->cs_valid == SPI_CS_HIGH) {
                ctrl_reg &= ~SPI_CTRL_MASTER_CE_AT_END_MASK;
            }
        }

        // config mode
        switch (cfg->mode) {
            case SPI_MODE_0:
                register_set(&ctrl_reg, MASK_2REG(SPI_CTRL_MODE,       0,
                                                  SPI_CTRL_INVERT_CLK, 0));
                break;
            case SPI_MODE_1:
                register_set(&ctrl_reg, MASK_2REG(SPI_CTRL_MODE,       1,
                                                  SPI_CTRL_INVERT_CLK, 0));
                break;
            case SPI_MODE_2:
                register_set(&ctrl_reg, MASK_2REG(SPI_CTRL_MODE,       0,
                                                  SPI_CTRL_INVERT_CLK, 1));
                break;
            case SPI_MODE_3:
                register_set(&ctrl_reg, MASK_2REG(SPI_CTRL_MODE,       1,
                                                  SPI_CTRL_INVERT_CLK, 1));
                break;
            default:
                OM_ASSERT(0);
                break;
        }

        // config wire 3
        if (cfg->wire == SPI_WIRE_3) {
            register_set(&ctrl_reg, MASK_2REG(SPI_CTRL_BIDIRECT_DATA,   1,
                                              SPI_CTRL_INACTIVE_DO_ENL, 1));
            if (cfg->role == SPI_ROLE_SLAVE) {
                ctrl_reg |= SPI_CTRL_ACTIVE_DO_ENL_MASK;
            }
        }

        // config first bit
        if (cfg->first_bit == SPI_MSB_FIRST) {
            ctrl_reg |= SPI_CTRL_MSB_FIRST_MASK;
        }
        om_spi->CTRL = ctrl_reg;
    } while(0);

    // disable tx empty and rx trigger interrupt
    register_set(&om_spi->STAT, MASK_2REG(SPI_STAT_TX_EMPTY_INT_EN, 0,
                                          SPI_STAT_RX_TRIG_INT_EN,  0));

    NVIC_ClearPendingIRQ(resource->irq_num);
    NVIC_SetPriority(resource->irq_num, resource->irq_prio);
    NVIC_EnableIRQ(resource->irq_num);

    env->isr_cb = NULL;
    #if (RTE_GPDMA)
    env->gpdma_tx_chan = GPDMA_NUMBER_OF_CHANNELS;
    env->gpdma_rx_chan = GPDMA_NUMBER_OF_CHANNELS;
    #endif

    return OM_ERROR_OK;
}

void drv_spi_uninit(OM_SPI_Type *om_spi)
{
    #if (RTE_SPI0)
    if ((uint32_t)om_spi == (uint32_t)(spi0_resource.reg)) {
        DRV_RCC_CLOCK_ENABLE(RCC_CLK_SPI0, 0U);
        return;
    }
    #endif
    #if (RTE_SPI1)
    if ((uint32_t)om_spi == (uint32_t)(spi1_resource.reg)) {
        DRV_RCC_CLOCK_ENABLE(RCC_CLK_SPI1, 0U);
        return;
    }
    #endif
}

#if (RTE_SPI_REGISTER_CALLBACK)
void drv_spi_register_isr_callback(OM_SPI_Type *om_spi, drv_isr_callback_t isr_cb)
{
    const drv_resource_t *resource;
    spi_env_t        *env;

    resource = spi_get_resource(om_spi);
    if(resource != NULL) {
        env = (spi_env_t *)(resource->env);
        env->isr_cb = isr_cb;
    }
}
#endif

__WEAK void drv_spi_isr_callback(OM_SPI_Type *om_spi, drv_event_t event, uint8_t *data, uint32_t num)
{
    #if (RTE_SPI_REGISTER_CALLBACK)
    const drv_resource_t *resource;

    resource = spi_get_resource(om_spi);
    if (resource) {
        spi_env_t *env;

        env = (spi_env_t *)(resource->env);
        if (env->isr_cb) {
            env->isr_cb(om_spi, event, data, (void *)num);
        }
    }
    #endif
}

om_error_t drv_spi_transfer(OM_SPI_Type *om_spi, const uint8_t *tx_data, uint16_t tx_num, uint8_t *rx_data, uint16_t rx_num, uint32_t timeout_ms)
{
    uint32_t    ctrl_reg;
    om_error_t  error;
    spi_role_t  role;
    spi_wire_t  wire;

    ctrl_reg = om_spi->CTRL;
    role = (ctrl_reg & SPI_CTRL_MASTER_EN_MASK) ? SPI_ROLE_MASTER : SPI_ROLE_SLAVE;
    wire = (ctrl_reg & SPI_CTRL_BIDIRECT_DATA_MASK) ? SPI_WIRE_3 : SPI_WIRE_4;

    if (role == SPI_ROLE_MASTER) {
        SPI_CSN_LOW(om_spi);
        timeout_ms = DRV_MAX_DELAY;
    }

    om_spi->STAT = 0;
    if (wire == SPI_WIRE_4) {
        if (tx_num == 0) {
            tx_num = rx_num;
        }
        if (rx_num == 0) {
            rx_num = tx_num;
        }
        OM_ASSERT(tx_num == rx_num);
        error = spi_transfer(om_spi, tx_data, rx_data, tx_num, timeout_ms);
        goto _exit;
    } else {
        if (role == SPI_ROLE_MASTER) {
            error = spi_transfer(om_spi, tx_data, NULL, tx_num, timeout_ms);
            if (error != OM_ERROR_OK) {
                goto _exit;
            }
            if (rx_num) {
                error = spi_transfer(om_spi, NULL, rx_data, rx_num, timeout_ms);
            }
        } else {
            error = spi_transfer(om_spi, NULL, rx_data, rx_num, timeout_ms);
            if (error != OM_ERROR_OK) {
                goto _exit;
            }
            if (tx_num) {
                error = spi_transfer(om_spi, tx_data, NULL, tx_num, timeout_ms);
            }
        }
    }

_exit:
    if (role == SPI_ROLE_MASTER) {
        SPI_CSN_HIGH(om_spi);
    }

    return error;
}

om_error_t drv_spi_transfer_int(OM_SPI_Type *om_spi, const uint8_t *tx_data, uint16_t tx_num, uint8_t *rx_data, uint16_t rx_num)
{
    const drv_resource_t *resource;
    spi_env_t            *env;
    uint32_t    ctrl_reg;
    om_error_t  error;
    spi_role_t  role;
    spi_wire_t  wire;

    resource = spi_get_resource(om_spi);
    if (resource == NULL) {
        return OM_ERROR_PARAMETER;
    }
    ctrl_reg = om_spi->CTRL;
    role = (ctrl_reg & SPI_CTRL_MASTER_EN_MASK) ? SPI_ROLE_MASTER : SPI_ROLE_SLAVE;
    wire = (ctrl_reg & SPI_CTRL_BIDIRECT_DATA_MASK) ? SPI_WIRE_3 : SPI_WIRE_4;

    env = (spi_env_t *)(resource->env);
    env->tx_buf = (uint8_t *)tx_data;
    env->tx_num = tx_num;
    env->tx_cnt = 0U;
    env->rx_buf = rx_data;
    env->rx_num = rx_num;
    env->rx_cnt = 0U;

    if (role == SPI_ROLE_MASTER) {
        SPI_CSN_LOW(om_spi);
    }

    if (wire == SPI_WIRE_4) {
        if (env->tx_num == 0) {
            env->tx_num = env->rx_num;
            OM_ASSERT(env->tx_num);
        }
        if (env->rx_num == 0) {
            env->rx_num = env->tx_num;
            OM_ASSERT(env->rx_num);
        }
        OM_ASSERT(env->tx_num == env->rx_num);
        env->state = SPI_STATE_TRX;
        om_spi->CTRL = ctrl_reg & (~SPI_CTRL_ACTIVE_DO_ENL_MASK);
        error = spi_transfer_int(resource);
    } else {
        if (role == SPI_ROLE_MASTER) {
            env->state = SPI_STATE_TX;
            ctrl_reg &= ~SPI_CTRL_RX_FIFO_EN_MASK;//because the receiving fifo of the 3 wire does not clear after the master has finished sending, so the receiving fifo is turned off when sending
            om_spi->CTRL = ctrl_reg & (~SPI_CTRL_ACTIVE_DO_ENL_MASK);
            error = spi_transfer_int(resource);
        } else {
            env->state = SPI_STATE_RX;
            om_spi->CTRL = ctrl_reg | SPI_CTRL_ACTIVE_DO_ENL_MASK;
            error = spi_transfer_int(resource);
        }
    }

    return error;
}

#if (RTE_GPDMA)
om_error_t drv_spi_gpdma_channel_allocate(OM_SPI_Type *om_spi, drv_gpdma_chan_t channel)
{
    const drv_resource_t *resource;
    gpdma_config_t dma_cfg;
    spi_env_t     *env;
    om_error_t     error;

    (void)error;
    OM_ASSERT(channel & DRV_GPDMA_CHAN_ALL);
    resource = spi_get_resource(om_spi);
    if (resource == NULL) {
        return OM_ERROR_PARAMETER;
    }
    OM_ASSERT(resource->cap & CAP_SPI_GPDMA_MASK);
    env = (spi_env_t *)(resource->env);
    if ((channel & DRV_GPDMA_RX_CHAN) && (env->gpdma_rx_chan >= GPDMA_NUMBER_OF_CHANNELS)) {
        env->gpdma_rx_chan = drv_gpdma_channel_allocate();
        if (env->gpdma_rx_chan >= GPDMA_NUMBER_OF_CHANNELS) {
            return OM_ERROR_RESOURCES;
        }
        dma_cfg.channel_ctrl  = GPDMA_SET_CTRL(GPDMA_ADDR_CTRL_FIXED, GPDMA_ADDR_CTRL_INC,
                                               GPDMA_TRANS_WIDTH_1B,  GPDMA_TRANS_WIDTH_1B,
                                               GPDMA_BURST_SIZE_1T,   (resource->gpdma_rx.prio) ? GPDMA_PRIORITY_HIGH : GPDMA_PRIORITY_LOW);
        dma_cfg.src_id        = (gpdma_id_t)resource->gpdma_rx.id;
        dma_cfg.dst_id        = GPDMA_ID_MEM;
        dma_cfg.isr_cb        = spi_gpdma_rx_isr_cb;
        dma_cfg.cb_param      = (void *)resource;
        dma_cfg.chain_trans   = NULL;
        dma_cfg.chain_trans_num = 0U;
        error = drv_gpdma_channel_config(env->gpdma_rx_chan, &dma_cfg);
        OM_ASSERT(error == OM_ERROR_OK);
    }
    if ((channel & DRV_GPDMA_TX_CHAN) && (env->gpdma_tx_chan >= GPDMA_NUMBER_OF_CHANNELS)) {
        env->gpdma_tx_chan = drv_gpdma_channel_allocate();
        if (env->gpdma_tx_chan >= GPDMA_NUMBER_OF_CHANNELS) {
            if (env->gpdma_rx_chan < GPDMA_NUMBER_OF_CHANNELS) {
                drv_gpdma_channel_release(env->gpdma_rx_chan);
                env->gpdma_rx_chan = GPDMA_NUMBER_OF_CHANNELS;
            }
            return OM_ERROR_RESOURCES;
        }
        dma_cfg.channel_ctrl  = GPDMA_SET_CTRL(GPDMA_ADDR_CTRL_INC, GPDMA_ADDR_CTRL_FIXED,
                                               GPDMA_TRANS_WIDTH_1B, GPDMA_TRANS_WIDTH_1B,
                                               GPDMA_BURST_SIZE_1T, (resource->gpdma_tx.prio) ? GPDMA_PRIORITY_HIGH : GPDMA_PRIORITY_LOW);
        dma_cfg.src_id        = GPDMA_ID_MEM;
        dma_cfg.dst_id        = (gpdma_id_t)resource->gpdma_tx.id;
        dma_cfg.isr_cb        = spi_gpdma_tx_isr_cb;
        dma_cfg.cb_param      = (void *)resource;
        dma_cfg.chain_trans   = NULL;
        dma_cfg.chain_trans_num = 0U;
        error = drv_gpdma_channel_config(env->gpdma_tx_chan, &dma_cfg);
        OM_ASSERT (error == OM_ERROR_OK);
    }

    // config SPI register
    om_spi->DMACR   = SPI_DMACR_RDMAE_MASK | SPI_DMACR_TDMAE_MASK;
    om_spi->DMATDLR = 0x01;     // dma_tx_req is asserted when data in TxFifo <= 1
    om_spi->DMARDLR = 0x00;     // dma_rx_req is asserted when data in RxFifo >= 1

    return OM_ERROR_OK;
}

om_error_t drv_spi_gpdma_channel_release(OM_SPI_Type *om_spi, drv_gpdma_chan_t channel)
{
    const drv_resource_t *resource;
    spi_env_t            *env;

    OM_ASSERT(channel & DRV_GPDMA_CHAN_ALL);
    resource = spi_get_resource(om_spi);
    if (resource == NULL) {
        return OM_ERROR_PARAMETER;
    }
    om_spi->CTRL |= SPI_CTRL_RX_FIFO_EN_MASK;//restore the SPI receiving FIFO to the enabled default state after the DMA transfer is completed, for use in polling and interrupt mode
    env = (spi_env_t *)(resource->env);
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

om_error_t drv_spi_transfer_dma(OM_SPI_Type *om_spi, const uint8_t *tx_data, uint16_t tx_num, uint8_t *rx_data, uint16_t rx_num)
{
    const drv_resource_t    *resource;
    spi_env_t               *env;
    spi_wire_t               wire;
    spi_role_t               role;
    uint32_t                 ctrl_reg;

    resource = spi_get_resource(om_spi);
    if (resource == NULL) {
        return OM_ERROR_PARAMETER;
    }

    env = (spi_env_t *)(resource->env);
    ctrl_reg = om_spi->CTRL;
    wire = (ctrl_reg & SPI_CTRL_BIDIRECT_DATA_MASK) ? SPI_WIRE_3 : SPI_WIRE_4;
    role = (ctrl_reg & SPI_CTRL_MASTER_EN_MASK) ? SPI_ROLE_MASTER : SPI_ROLE_SLAVE;
    OM_ASSERT_WHILE(role == SPI_ROLE_SLAVE, rx_data);
    OM_ASSERT_WHILE(rx_data, rx_num);

    env->tx_buf = (uint8_t *)tx_data;
    env->tx_num = tx_num;
    env->rx_buf = rx_data;
    env->rx_num = rx_num;

    if (role == SPI_ROLE_MASTER) {
        OM_ASSERT(tx_num);
        SPI_CSN_LOW(om_spi);
    }
    if (wire == SPI_WIRE_4) {
        OM_ASSERT_WHILE(tx_data && rx_data,  tx_num == rx_num);
        env->state = SPI_STATE_TRX;
        return spi_transfer_dma(resource, tx_data, tx_num, rx_data, rx_num);
    } else {
        if (role == SPI_ROLE_MASTER) {
            env->state = SPI_STATE_TX;
            return spi_transfer_dma(resource, tx_data, tx_num, NULL, 0);
        } else {
            env->state = SPI_STATE_RX;
            return spi_transfer_dma(resource, NULL, 0, rx_data, rx_num);
        }
    }
}
#endif

uint32_t drv_spi_get_write_cnt(OM_SPI_Type *om_spi)
{
    const drv_resource_t  *resource;
    resource = spi_get_resource(om_spi);
    if (resource) {
        spi_env_t *env;
        env = (spi_env_t *)(resource->env);
        #if (RTE_GPDMA)
        if ((env->gpdma_tx_chan < GPDMA_NUMBER_OF_CHANNELS) && drv_gpdma_channel_is_busy(env->gpdma_tx_chan)) {
            return (env->tx_num - drv_gpdma_channel_get_left_count(env->gpdma_tx_chan));
        }
        #endif

        return env->tx_cnt;
    }

    OM_ASSERT(0);
    return 0;
}

uint32_t drv_spi_get_read_cnt(OM_SPI_Type *om_spi)
{
    const drv_resource_t  *resource;

    resource = spi_get_resource(om_spi);
    if (resource) {
        spi_env_t *env;
        env = (spi_env_t *)(resource->env);
        #if (RTE_GPDMA)
        if ((env->gpdma_rx_chan < GPDMA_NUMBER_OF_CHANNELS) && drv_gpdma_channel_is_busy(env->gpdma_rx_chan)) {
            return (env->rx_num - drv_gpdma_channel_get_left_count(env->gpdma_rx_chan));
        }
        #endif

        return env->rx_cnt;
    }

    OM_ASSERT(0);
    return 0;
}

// TODO:
//void drv_spi_abort_read(OM_SPI_Type *om_spi)
//void drv_spi_abort_read(OM_SPI_Type *om_spi)

void drv_spi_isr(OM_SPI_Type *om_spi)
{
    const drv_resource_t  *resource;
    spi_env_t             *env;
    spi_role_t             role;
    uint32_t               ctrl_reg;
    drv_event_t            drv_event;

    resource = spi_get_resource(om_spi);
    if (resource == NULL) {
        return;
    }

    env  = (spi_env_t *)(resource->env);
    ctrl_reg = om_spi->CTRL;
    role = (ctrl_reg & SPI_CTRL_MASTER_EN_MASK) ? SPI_ROLE_MASTER : SPI_ROLE_SLAVE;
    drv_event = DRV_EVENT_COMMON_NONE;

    // clear SPI interrupt status & all int enable
    om_spi->STAT = SPI_STAT_SPI_INT_MASK;

    // process received
    if ((env->state & SPI_STATE_RX) && (env->rx_buf != NULL)) {
        while (om_spi->STAT & SPI_STAT_RX_NOT_EMPTY_MASK) {
            env->rx_buf[env->rx_cnt] = om_spi->RDATA;
            env->rx_cnt++;
        }
    } else {
        spi_clear_rx_fifo(om_spi);
    }

    switch (env->state) {
        case SPI_STATE_TX:
            if (env->tx_cnt < env->tx_num) {  // tx completed
                spi_transfer_int(resource);
                break;
            }
            if ((role == SPI_ROLE_SLAVE) || (env->rx_num == 0)) {
                om_spi->CTRL |= SPI_CTRL_RX_FIFO_EN_MASK;//restore the SPI receiving FIFO to the enabled default state after the slave tx transfer is completed, for use in polling and dma mode
                drv_event = DRV_EVENT_COMMON_TRANSFER_COMPLETED;
            } else {
                env->state = SPI_STATE_RX;
                ctrl_reg |= SPI_CTRL_RX_FIFO_EN_MASK;//when rx fifo enable, tx fifo not enable,and rx fifo is not full,the spi clock will be generated,so the tx fifo enable all the time
                om_spi->CTRL = ctrl_reg | SPI_CTRL_ACTIVE_DO_ENL_MASK;
                env->tx_cnt = 0;
                spi_transfer_int(resource);
            }
            break;
        case SPI_STATE_RX:
            if (env->rx_cnt < env->rx_num) {    // rx completed
                spi_transfer_int(resource);
                break;
            }
            if ((role == SPI_ROLE_MASTER) || (env->tx_num == 0)) {
                drv_event = DRV_EVENT_COMMON_TRANSFER_COMPLETED;
            } else {
                env->state = SPI_STATE_TX;
                env->tx_cnt = 0;
                ctrl_reg &= ~SPI_CTRL_RX_FIFO_EN_MASK;//because the receiving fifo of the 3 wire does not clear after the master has finished sending, so the receiving fifo is turned off when sending
                om_spi->CTRL = ctrl_reg & (~SPI_CTRL_ACTIVE_DO_ENL_MASK);
                spi_transfer_int(resource);
            }
            break;
        case SPI_STATE_TRX:
            if (env->rx_cnt == env->rx_num) {
                drv_event = DRV_EVENT_COMMON_TRANSFER_COMPLETED;
            } else {
                spi_transfer_int(resource);
            }
            break;
        default:
            OM_ASSERT(0);
            break;
    }

    if (drv_event == DRV_EVENT_COMMON_TRANSFER_COMPLETED) {
        if (role == SPI_ROLE_MASTER) {
            SPI_CSN_HIGH(om_spi);
        }
        env->state = SPI_STATE_IDLE;
        om_spi->CTRL &= ~SPI_CTRL_ACTIVE_DO_ENL_MASK;
        drv_spi_isr_callback(om_spi, DRV_EVENT_COMMON_TRANSFER_COMPLETED, env->rx_buf, env->rx_cnt);
    }
}


#endif  /* (RTE_SPI0 || RTE_SPI1) */

/** @} */
