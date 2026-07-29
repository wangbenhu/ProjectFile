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
 * @brief    OM24G driver source file
 * @details  OM24G driver source file
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
#if (RTE_OM24G)
#include <stddef.h>
#include "om_device.h"
#include "om_driver.h"


/*********************************************************************
 * MACROS
 */
#define PLL_CONTIMUE_OPEN 0
#define HARDWARE_TIMER_ENABLE 0

/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    drv_isr_callback_t           event_cb;            /**< event callback                      */
    uint16_t                     tx_cnt;              /**< Count of data sent*/
    uint16_t                     max_rx_num;          /**< num of data sent*/
    const uint8_t               *tx_buf;              /**< Pointer to out data buffer, system to peripheral direction */
    uint8_t                     *rx_buf;              /**< Pointer to in data buffer            */
    bool                         deviation_below_250K;/**< Flag with frequency deviation less than 250K */
    uint8_t                      rx_count;            /**< rx_count is used for alternate counting of ping pong buffers, not the number of received packets. */
    struct {
        uint32_t MIX_CFG0;
        uint32_t PD_CFG1;
        uint32_t MAIN_ST_CFG1;
    } reg_store;
} om24g_env_t;

/*******************************************************************************
 * CONST & VARIABLES
 */

/* om24g information */
#if (RTE_OM24G)
static om24g_env_t om24g_env = {
    .event_cb      = NULL,
    .tx_cnt        = 0U,
    .tx_buf        = NULL,
    .rx_buf        = NULL,
    .deviation_below_250K = false,
    .rx_count      = 0,
};

#endif  /* RTE_OM24G */

/*******************************************************************************
 * LOCAL FUNCTIONS
 */
__RAM_CODE
static void drv_om24g_ce_high_pulse(void)
{
    #if (HARDWARE_TIMER_ENABLE == 0)
    //CE HIGH
    OM_24G->MAC_EN = 1;
    DRV_DELAY_US(1);
    OM_24G->DMA_CMD = 0xA0;
    DRV_DELAY_US(3);
    // CE LOW
    OM_24G->MAC_EN = 0;
    #endif
}

__RAM_CODE
static uint16_t drv_om24g_read_rx_payload_width(void)
{
    uint16_t Width;
    uint8_t en_dpl;
    en_dpl = REGR(&OM_24G->FEATURE, OM24G_EN_DPL_MASK, OM24G_EN_DPL_POS);
    if (en_dpl) {
        Width = REGR(&OM_24G->RX_DYN_LEN, OM24G_RX_DYN_LEN_MASK, OM24G_RX_DYN_LEN_POS);
    } else {
        Width = REGR(&OM_24G->PACKET_LEN, OM24G_PACKET_LEN_MASK, OM24G_PACKET_LEN_POS);
    }
    return Width;
}

void drv_om24g_dump_rf_register(void)
{
    #if 0
    OM_LOG(OM_LOG_DEBUG, "PKTCTRL0: %08x\r\n", OM_24G->PKTCTRL0);
    OM_LOG(OM_LOG_DEBUG, "FB_PKTCTRL: %08x\r\n", OM_24G->FB_PKTCTRL);
    OM_LOG(OM_LOG_DEBUG, "DYNPD: %02x\r\n", OM_24G->DYNPD);
    OM_LOG(OM_LOG_DEBUG, "FEATURE: %02x\r\n", OM_24G->FEATURE);
    OM_LOG(OM_LOG_DEBUG, "INT_ST: %02x\r\n", OM_24G->INT_ST);
    OM_LOG(OM_LOG_DEBUG, "EN_AA: %02x\r\n", OM_24G->EN_AA);
    OM_LOG(OM_LOG_DEBUG, "EN_RXADDR: %02x\r\n", OM_24G->EN_RXADDR);
    OM_LOG(OM_LOG_DEBUG, "FA_PKTCTRL: %02x\r\n", OM_24G->FA_PKTCTRL);
    OM_LOG(OM_LOG_DEBUG, "SETUP_RETR: %02x\r\n", OM_24G->FA_SETUP_RETR);
    OM_LOG(OM_LOG_DEBUG, "FA_OBSERVE_TX: %02x\r\n", OM_24G->FA_OBSERVE_TX);
    OM_LOG(OM_LOG_DEBUG, "PREAMBLE: %02x\r\n", OM_24G->PREAMBLE);
    OM_LOG(OM_LOG_DEBUG, "PREAMBLE_LEN: %02x\r\n", OM_24G->PREAMBLE_LEN);
    OM_LOG(OM_LOG_DEBUG, "PRE_GUARD: %02x\r\n", OM_24G->PRE_GUARD);
    OM_LOG(OM_LOG_DEBUG, "OM_24G->TAIL_DATA: %08x\r\n", OM_24G->TAIL_DATA);
    OM_LOG(OM_LOG_DEBUG, "SYNC_WORD0: %08x\r\n", OM_24G->SYNC_WORD0);
    OM_LOG(OM_LOG_DEBUG, "SYNC_WORD1: %08x\r\n", OM_24G->SYNC_WORD1);
    OM_LOG(OM_LOG_DEBUG, "TX_ADDR: %02x\r\n", OM_24G->TX_ADDR);
    OM_LOG(OM_LOG_DEBUG, "SYNC_WORD_SEL: %02x\r\n", OM_24G->SYNC_WORD_SEL);
    OM_LOG(OM_LOG_DEBUG, "RX_ADDR_P0: %02x\r\n", OM_24G->RX_ADDR_P0);
    OM_LOG(OM_LOG_DEBUG, "RX_ADDR_P1: %02x\r\n", OM_24G->RX_ADDR_P1);
    OM_LOG(OM_LOG_DEBUG, "RX_ADDR_P2: %02x\r\n", OM_24G->RX_ADDR_P2);
    OM_LOG(OM_LOG_DEBUG, "RX_ADDR_P3: %02x\r\n", OM_24G->RX_ADDR_P3);
    OM_LOG(OM_LOG_DEBUG, "RX_ADDR_P4: %02x\r\n", OM_24G->RX_ADDR_P4);
    OM_LOG(OM_LOG_DEBUG, "RX_ADDR_P5: %02x\r\n", OM_24G->RX_ADDR_P5);
    OM_LOG(OM_LOG_DEBUG, "RF_DR: %02x\r\n", OM_24G->RF_DR);
    OM_LOG(OM_LOG_DEBUG, "RX_P_NO: %02x\r\n", OM_24G->RX_P_NO);
    OM_LOG(OM_LOG_DEBUG, "CRCCFG: %08x\r\n", OM_24G->CRCCFG);
    OM_LOG(OM_LOG_DEBUG, "CRCPOLY: %08x\r\n", OM_24G->CRCPOLY);
    OM_LOG(OM_LOG_DEBUG, "CRCINIT: %08x\r\n", OM_24G->CRCINIT);
    OM_LOG(OM_LOG_DEBUG, "CRCSKIP: %02x\r\n", OM_24G->CRCSKIP);
    OM_LOG(OM_LOG_DEBUG, "WHITECFG: %02x\r\n", OM_24G->WHITECFG);
    OM_LOG(OM_LOG_DEBUG, "WHITESEL: %02x\r\n", OM_24G->WHITESEL);
    OM_LOG(OM_LOG_DEBUG, "WHITESEED: %02x\r\n", OM_24G->WHITESEED);
    OM_LOG(OM_LOG_DEBUG, "WHITEOBIT: %02x\r\n", OM_24G->WHITEOBIT);
    OM_LOG(OM_LOG_DEBUG, "DMA_CMD: %02x\r\n", OM_24G->DMA_CMD);
    OM_LOG(OM_LOG_DEBUG, "DMA_TX_LEN: %02x\r\n", OM_24G->DMA_TX_LEN);
    OM_LOG(OM_LOG_DEBUG, "RX_DYN_LEN: %02x\r\n", OM_24G->RX_DYN_LEN);
    OM_LOG(OM_LOG_DEBUG, "DMA_TX_ADDR: %02x\r\n", OM_24G->DMA_TX_ADDR);
    OM_LOG(OM_LOG_DEBUG, "DMA_RX_ADDR: %08x\r\n", OM_24G->DMA_RX_ADDR);
    OM_LOG(OM_LOG_DEBUG, "PACKET_LEN: %02x\r\n", OM_24G->PACKET_LEN);
    OM_LOG(OM_LOG_DEBUG, "BCC: %02x\r\n", OM_24G->BCC);
    OM_LOG(OM_LOG_DEBUG, "TIMESTAMP_RT: %02x\r\n", OM_24G->TIMESTAMP_RT);
    OM_LOG(OM_LOG_DEBUG, "TIMESTAMP_TRIGER: %02x\r\n", OM_24G->TIMESTAMP_TRIGER);
    OM_LOG(OM_LOG_DEBUG, "TIMESTAMP_CFG: %02x\r\n", OM_24G->TIMESTAMP_CFG);
    OM_LOG(OM_LOG_DEBUG, "SETUP_VALUE: %02x\r\n", OM_24G->SETUP_VALUE);
    OM_LOG(OM_LOG_DEBUG, "ENDIAN: %02x\r\n", OM_24G->ENDIAN);
    OM_LOG(OM_LOG_DEBUG, "FLUSH: %02x\r\n", OM_24G->FLUSH);
    OM_LOG(OM_LOG_DEBUG, "TESTCTRL: %02x\r\n", OM_24G->TESTCTRL);
    OM_LOG(OM_LOG_DEBUG, "STATE: %02x\r\n", OM_24G->STATE);
    OM_LOG(OM_LOG_DEBUG, "TAIL_DATA: %02x\r\n", OM_24G->TAIL_DATA);
    OM_LOG(OM_LOG_DEBUG, "ACK_MODE: %02x\r\n", OM_24G->ACK_MODE);
    OM_LOG(OM_LOG_DEBUG, "FREQ: %02d\r\n", REGR(&OM_DAIF->FREQ_CFG0, DAIF_FREQ_REG_MO_MASK, DAIF_FREQ_REG_MO_POS));
    OM_LOG(OM_LOG_DEBUG, "FRACT_FREQ: %x\r\n", OM_DAIF->FREQ_CFG3);
    OM_LOG(OM_LOG_DEBUG, "OM_PHY->REG_PHY_RST_N: %02x\r\n", OM_PHY->REG_PHY_RST_N);
    OM_LOG(OM_LOG_DEBUG, "OM_PHY->TX_CTRL0: %02x\r\n", OM_PHY->TX_CTRL0);
    OM_LOG(OM_LOG_DEBUG, "OM_PHY->DET_MODE: %02x\r\n", OM_PHY->DET_MODE);
    OM_LOG(OM_LOG_DEBUG, "OM_PHY->H_RX_CTRL: %02x\r\n", OM_PHY->H_RX_CTRL);
    OM_LOG(OM_LOG_DEBUG, "OM_PHY->FD_CFO_CMP: %02x\r\n", OM_PHY->FD_CFO_CMP);
    OM_LOG(OM_LOG_DEBUG, "OM_PHY->STR_CTRL: %02x\r\n", OM_PHY->STR_CTRL);
    OM_LOG(OM_LOG_DEBUG, "OM_PHY->RX_GFSK_SYNC_CTRL: %08x\r\n", OM_PHY->RX_GFSK_SYNC_CTRL);
    #endif
}

static void drv_om24g_set_deviation(float deviation)
{
    if(deviation < 250) {
        om24g_env.deviation_below_250K = true;
        REGW0(&OM_DAIF->MIX_CFG0, DAIF_FREQ_DEVIA_BYPASS_TX_MASK); //freq devia bypass=0d
        REGW(&OM_DAIF->MIX_CFG0, MASK_1REG(DAIF_FREQ_DEVIA_COEFF_TX, (uint8_t)(deviation*255/250.0f)));
        REGW0(&OM_DAIF->PLL_CTRL1, DAIF_FREQ_DEVIA_BYPASS_3RD_MASK); //freq devia bypass=0d
        REGW(&OM_DAIF->PLL_CTRL1, MASK_1REG(DAIF_FREQ_DEVIA_COEFF_3RD, (uint8_t)(deviation*255/250.0f)));
    } else {
        om24g_env.deviation_below_250K = false;
        REGW1(&OM_DAIF->MIX_CFG0, DAIF_FREQ_DEVIA_BYPASS_TX_MASK); //freq devia bypass=0d
        REGW1(&OM_DAIF->PLL_CTRL1, DAIF_FREQ_DEVIA_BYPASS_3RD_MASK); //freq devia bypass=0d
    }
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
void drv_om24g_register_event_callback(drv_isr_callback_t event_cb)
{
    om24g_env.event_cb = event_cb;
}

__RAM_CODE
void drv_om24g_switch_role(om24g_role_t role)
{
    OM24G_CE_LOW();
    om_error_t ret; // Max: 150us
    DRV_WAIT_US_UNTIL_TO(!((REGR(&OM_24G->STATE, MASK_POS(OM24G_MAIN_STATE)) == 0x02) || (REGR(&OM_24G->STATE, MASK_POS(OM24G_MAIN_STATE)) == 0x00)), 150, ret);(void) ret;
    DRV_DELAY_US(1);
    if (role == OM24G_ROLE_PTX) {
        REGW0(&OM_24G->PKTCTRL0, OM24G_PKTCTRL0_PRIM_RX_MASK);
    } else {
        REGW1(&OM_24G->PKTCTRL0, OM24G_PKTCTRL0_PRIM_RX_MASK);
    }
    DRV_DELAY_US(1);
}

__RAM_CODE
static void drv_om24g_write_tx_payload(uint16_t dma_tx_num, const uint8_t *payload)
{
    OM_24G->DMA_TX_ADDR = (uint32_t)payload;
    OM_24G->DMA_TX_LEN = dma_tx_num;
}

__RAM_CODE
bool drv_om24g_write(const uint8_t *tx_payload, uint16_t tx_num)
{
    uint8_t status = 0x00;

    #if (RTE_OM24G_RF_MODE == 2) // Compatible with SILICONLAB
    REGW(&OM_PHY->TX_CTRL0, MASK_2REG(PHY_TX_CTRL0_EN_INTERP, 0x1, PHY_TX_CTRL0_BDR_PPM_TX, 0x220));
    #endif
    drv_om24g_switch_role(OM24G_ROLE_PTX);
    drv_om24g_write_tx_payload(tx_num, tx_payload);
    drv_om24g_ce_high_pulse();
    while (1) {
        status = OM_24G->INT_ST;
        if ((OM24G_INT_TX_DS_MASK & status) || (OM24G_INT_MAX_RT_MASK & status)) {
            OM_24G->INT_ST = status;
            if (OM24G_INT_TX_DS_MASK & status) {
                status = 0;
                REGW(&OM_24G->RX_DONE, MASK_1REG(OM24G_RX_DONE, 1));
                return true;
            } else {
                status = 0;
                return false;
            }
        }
    }
}

__RAM_CODE
void drv_om24g_write_int(const uint8_t *tx_payload, uint16_t tx_num)
{
    #if (RTE_OM24G_RF_MODE == 2) // Compatible with SILICONLAB
    REGW(&OM_PHY->TX_CTRL0, MASK_2REG(PHY_TX_CTRL0_EN_INTERP, 0x1, PHY_TX_CTRL0_BDR_PPM_TX, 0x220));
    #endif
    drv_om24g_switch_role(OM24G_ROLE_PTX);
    drv_om24g_write_tx_payload(tx_num, tx_payload);
    drv_om24g_ce_high_pulse();
}

__RAM_CODE
uint16_t drv_om24g_read(uint8_t *rx_payload, uint32_t timeout_ms)
{
    uint16_t rx_cnt = 0;
    bool rx_right_flag = false;
    om_error_t error;

    OM_24G->DMA_RX_ADDR = (uint32_t)rx_payload;
    drv_om24g_switch_role(OM24G_ROLE_PRX);
    OM24G_CE_HIGH();

    while (1) {
        DRV_WAIT_MS_UNTIL_TO(!( OM_24G->INT_ST & (OM24G_INT_RX_DR_MASK | OM24G_INT_CRC_ERR_MASK)), timeout_ms, error);
        if (error != OM_ERROR_OK) {
            OM24G_CE_LOW();
            return 0;
        }
        if (OM24G_INT_RX_DR_MASK & OM_24G->INT_ST) {
            OM_24G->INT_ST = OM24G_INT_RX_DR_MASK;
            if(!REGR(&OM_24G->FEATURE, MASK_POS(OM24G_EN_ACK_PAY))) {
                OM_24G->MAC_EN = 0;
            }
            if((!(OM_24G->STATE & OM24G_SYNC_DET_MASK)) && (!(OM24G_INT_CRC_ERR_MASK & OM_24G->INT_ST))) {
                rx_cnt = drv_om24g_read_rx_payload_width();
                rx_right_flag = true;
            } else {
                rx_right_flag = false;
                OM24G_CE_HIGH();
            }
        }
        REGW(&OM_24G->RX_DONE, MASK_1REG(OM24G_RX_DONE, 1));
        if(rx_right_flag) {
            break;
        }
        if (OM24G_INT_CRC_ERR_MASK & OM_24G->INT_ST) {
            OM_24G->INT_ST = OM24G_INT_CRC_ERR_MASK;
        }
    }
    return rx_cnt;
}

/**
 *******************************************************************************
 * @brief Receive data from receive FIFO by interrupt mode. The received data and length can be obtained from the callback function.

 * @param[in] rx_payload      Data read from receive buffer.
 * @param[in] max_rx_num      The maximum number of receives for this application.

 *******************************************************************************
 */
__RAM_CODE
void drv_om24g_read_int(uint8_t *rx_payload, uint16_t max_rx_num)
{
    om24g_env.rx_buf = rx_payload;
    if(OM_24G->EN_AA) {
        om24g_env.max_rx_num = 0; // ACK mode use single buff
    } else {
        om24g_env.max_rx_num = max_rx_num;  // NO_ACK mode use double buff
    }

    drv_om24g_switch_role(OM24G_ROLE_PRX);
    OM24G_CE_HIGH();
}

uint16_t drv_om24g_read_ack(void)
{
    uint16_t payload_lenth = 0;

    payload_lenth = drv_om24g_read_rx_payload_width();
    OM24G_CLEAR_ALL_IRQ();

    return payload_lenth;
}

void drv_om24g_set_addr(uint8_t pipenum, uint8_t addr)
{
    switch (pipenum) {
        case OM24G_PIPE_TX:
            REGW(&OM_24G->TX_ADDR, MASK_1REG(OM24G_TX_ADDR, addr));
            break;
        case OM24G_PIPE0:
            REGW(&OM_24G->RX_ADDR_P0, MASK_1REG(OM24G_RX_ADDR_P0, addr));
            break;
        case OM24G_PIPE1:
            REGW(&OM_24G->RX_ADDR_P1, MASK_1REG(OM24G_RX_ADDR_P1, addr));
            break;
        case OM24G_PIPE2:
            REGW(&OM_24G->RX_ADDR_P2, MASK_1REG(OM24G_RX_ADDR_P2, addr));
            break;
        case OM24G_PIPE3:
            REGW(&OM_24G->RX_ADDR_P3, MASK_1REG(OM24G_RX_ADDR_P3, addr));
            break;
        case OM24G_PIPE4:
            REGW(&OM_24G->RX_ADDR_P4, MASK_1REG(OM24G_RX_ADDR_P4, addr));
            break;
        case OM24G_PIPE5:
            REGW(&OM_24G->RX_ADDR_P5, MASK_1REG(OM24G_RX_ADDR_P5, addr));
            break;
        default:
            OM_ASSERT(0);
            break;
    }

}

/*Please use function drv_om24g_set_rf_parameters to set the rate, drv_om24g_set_rate() did not configure the frequency offset.*/
void drv_om24g_set_rate(uint8_t data_rate, bool en_arb, uint8_t n_avr, uint8_t n_rep)
{

    #if (RTE_OM24G_RF_MODE == 2) // Compatible with SILICONLAB
    REGW(&OM_24G->RF_DR, MASK_4REG(OM24G_EN_RX_ARB, true, OM24G_EN_TX_ARB, false, OM24G_N_AVR, 2, OM24G_N_REP, 2));
    REGW(&OM_24G->RF_DR, MASK_2REG(OM24G_RF_DR_TX_RF_DR, OM24G_RATE_250K, OM24G_RF_DR_RX_RF_DR, OM24G_RATE_500K));
    #else
    REGW(&OM_24G->RF_DR, MASK_4REG(OM24G_EN_RX_ARB, en_arb, OM24G_EN_TX_ARB, en_arb, OM24G_N_AVR, n_avr, OM24G_N_REP, n_rep));
    REGW(&OM_24G->RF_DR, MASK_2REG(OM24G_RF_DR_TX_RF_DR, data_rate, OM24G_RF_DR_RX_RF_DR, data_rate));
    #endif
    if (((data_rate == OM24G_RATE_1M) || (data_rate == OM24G_RATE_2M)) && !en_arb) {
        REGW1(&OM_DAIF->MIX_CFG0, DAIF_FREQ_DEVIA_BYPASS_TX_MASK); //freq devia bypass=0d
        REGW1(&OM_DAIF->PLL_CTRL1, DAIF_FREQ_DEVIA_BYPASS_3RD_MASK); //freq devia bypass=0d
    } else {
        REGW0(&OM_DAIF->MIX_CFG0, DAIF_FREQ_DEVIA_BYPASS_TX_MASK); //freq devia bypass=0d
        REGW0(&OM_DAIF->PLL_CTRL1, DAIF_FREQ_DEVIA_BYPASS_3RD_MASK); //freq devia bypass=0d
    }
}

/*The speed only allows the use of speeds listed in enumeration om24g_rate_t in drv_24g.h. Please do not configure them arbitrarily.
For special requirements, please contact the developer.*/
void drv_om24g_set_rf_parameters(om24g_rate_t data_rate, uint16_t frequency, float fract_freq)
{
    drv_om24g_set_freq(frequency, fract_freq);
    switch (data_rate) {
        case OM24G_RATE_25K:
            drv_om24g_set_rate(OM24G_RATE_250K, true, 10, 10);
            if (SYS_IS_FPGA()) {
                OM_24G->FPGA_TX_INDEX = 0x02;
            } else {
                drv_om24g_set_deviation(62.5);
            }
            REGW(&OM_PHY->H_RX_CTRL, MASK_2REG(PHY_H_RX_CTRL_FXP, 0x40, PHY_H_RX_CTRL_RVS_FXP, 0x80)); // RX modulation index = 0.5
            REGW(&OM_PHY->DET_MODE, MASK_1REG(PHY_DET_MODE_DET_MODE, 0x00)); // OM24G_HARD_DETECTION
            break;
        case OM24G_RATE_50K:
            drv_om24g_set_rate(OM24G_RATE_500K, true, 10, 10);
            if (SYS_IS_FPGA()) {
                OM_24G->FPGA_TX_INDEX = 0x02;
            } else {
                drv_om24g_set_deviation(125);
            }
            REGW(&OM_PHY->H_RX_CTRL, MASK_2REG(PHY_H_RX_CTRL_FXP, 0x40, PHY_H_RX_CTRL_RVS_FXP, 0x80)); // RX modulation index = 0.5
            REGW(&OM_PHY->DET_MODE, MASK_1REG(PHY_DET_MODE_DET_MODE, 0x00)); // OM24G_HARD_DETECTION
            break;
        case OM24G_RATE_100K:
            drv_om24g_set_rate(OM24G_RATE_500K, true, 5, 5);
            if (SYS_IS_FPGA()) {
                OM_24G->FPGA_TX_INDEX = 0x03;
            } else {
                drv_om24g_set_deviation(125);
            }
            REGW(&OM_PHY->H_RX_CTRL, MASK_2REG(PHY_H_RX_CTRL_FXP, 0x60, PHY_H_RX_CTRL_RVS_FXP, 0x55)); //RX modulation index = 0.75
            REGW(&OM_PHY->DET_MODE, MASK_1REG(PHY_DET_MODE_DET_MODE, 0x00)); // OM24G_HARD_DETECTION
            break;
        case OM24G_RATE_125K:
            drv_om24g_set_rate(OM24G_RATE_500K, true, 4, 4); //  500K/4 = 125k
            if (SYS_IS_FPGA()) {
                OM_24G->FPGA_TX_INDEX = 0x03;
            } else {
                drv_om24g_set_deviation(125);
            }
            REGW(&OM_PHY->H_RX_CTRL, MASK_2REG(PHY_H_RX_CTRL_FXP, 0x60, PHY_H_RX_CTRL_RVS_FXP, 0x55)); //RX modulation index = 0.75
            REGW(&OM_PHY->DET_MODE, MASK_1REG(PHY_DET_MODE_DET_MODE, 0x00)); // OM24G_HARD_DETECTION
            break;
        case OM24G_RATE_250K:
            drv_om24g_set_rate(OM24G_RATE_250K, false, 0, 0);
            if (SYS_IS_FPGA()) {
                OM_24G->FPGA_TX_INDEX = 0x03;
            } else {
                drv_om24g_set_deviation(125);
            }
            REGW(&OM_PHY->H_RX_CTRL, MASK_2REG(PHY_H_RX_CTRL_FXP, 0x40, PHY_H_RX_CTRL_RVS_FXP, 0x80)); // RX modulation index = 0.5
            // REGW(&OM_PHY->H_RX_CTRL, MASK_2REG(PHY_H_RX_CTRL_FXP, 0x60, PHY_H_RX_CTRL_RVS_FXP, 0x55)); //RX modulation index = 0.75
            REGW(&OM_PHY->DET_MODE, MASK_1REG(PHY_DET_MODE_DET_MODE, 0x00)); // OM24G_HARD_DETECTION
            break;
        case OM24G_RATE_500K:
            drv_om24g_set_rate(data_rate, 0, 0, 0);
            if (SYS_IS_FPGA()) {
                OM_24G->FPGA_TX_INDEX = 0x03;
            } else {
                drv_om24g_set_deviation(125);
            }
            REGW(&OM_PHY->H_RX_CTRL, MASK_2REG(PHY_H_RX_CTRL_FXP, 0x40, PHY_H_RX_CTRL_RVS_FXP, 0x80)); // RX modulation index = 0.5
            REGW(&OM_PHY->DET_MODE, MASK_1REG(PHY_DET_MODE_DET_MODE, 0x00)); // OM24G_HARD_DETECTION
            break;
        default:
            drv_om24g_set_rate(data_rate, 0, 0, 0);
            #if (RTE_OM24G_RF_MODE == 3) // Compatible with NORDIC
                if (SYS_IS_FPGA()) {
                    OM_24G->FPGA_TX_INDEX = 0x01;
                }
                REGW(&OM_PHY->H_RX_CTRL, MASK_2REG(PHY_H_RX_CTRL_FXP, 0x29, PHY_H_RX_CTRL_RVS_FXP, 0xC8)); // RX modulation index = 0.32
            #else
                if (SYS_IS_FPGA()) {
                    OM_24G->FPGA_TX_INDEX = 0x02;
                }
                REGW(&OM_PHY->H_RX_CTRL, MASK_2REG(PHY_H_RX_CTRL_FXP, 0x40, PHY_H_RX_CTRL_RVS_FXP, 0x80)); // RX modulation index = 0.5
                //REGW(&OM_PHY->H_RX_CTRL, MASK_2REG(PHY_H_RX_CTRL_FXP, 0x29, PHY_H_RX_CTRL_RVS_FXP, 0xC8)); // RX modulation index = 0.32
                //REGW(&OM_PHY->H_RX_CTRL, MASK_2REG(PHY_H_RX_CTRL_FXP, 0x60, PHY_H_RX_CTRL_RVS_FXP, 0x55)); //RX modulation index = 0.75
                //REGW(&OM_PHY->H_RX_CTRL, MASK_2REG(PHY_H_RX_CTRL_FXP, 0x80, PHY_H_RX_CTRL_RVS_FXP, 0x40)); //RX modulation index = 1.0
                //REGW(&OM_PHY->DET_MODE, MASK_1REG(PHY_DET_MODE_DET_MODE, 0x01)); // OM24G_SOFT_DETECTION
            #endif
            drv_om24g_set_deviation(250);
            break;
    }
}

void drv_om24g_detection_mode(uint8_t detc_mode)
{
    switch (detc_mode) {
        case OM24G_SOFT_DETECTION:  // 0.5 0.32  support 1M,2M 500K,250K, not support LOW RATE.  highest accruacy, highest latency. Soft demodulation does not support phy mode 1 or above.
            REGW(&OM_PHY->DET_MODE, MASK_1REG(PHY_DET_MODE_DET_MODE, 0x01));
            break;
        case OM24G_HARD_DETECTION: //   suport all modulation index 1M,500K,250K ,LOW RATE,  2M not suport, middle accruacy, middle latency.
            REGW(&OM_PHY->DET_MODE, MASK_1REG(PHY_DET_MODE_DET_MODE, 0x00));
            break;
        case OM24G_DPLL_DETECTION:     // support all modulation index and rate, lowest accruacy, lowest latency.
            REGW(&OM_PHY->DET_MODE, MASK_1REG(PHY_DET_MODE_DET_MODE, 0x03));
            break;
        default:
            OM_ASSERT(0);
            break;
    }
    #if (RTE_OM24G_RF_MODE == 2) // Compatible with SILICONLAB
    REGW(&OM_PHY->FD_CFO_CMP, MASK_1REG(PHY_FD_CFO_CMP_FD_CFO_CMP, 0));
    REGW(&OM_PHY->TX_CTRL0, MASK_2REG(PHY_TX_CTRL0_TX_GFSK_MODE, 0x04, PHY_TX_CTRL0_COEF_GFSK, 0x0D)); // phy mode4
    REGW(&OM_PHY->TX_CTRL0, MASK_2REG(PHY_TX_CTRL0_EN_INTERP, 0x01, PHY_TX_CTRL0_BDR_PPM_TX, 0x00220)); // 0x00100 0x00520
    REGW(&OM_PHY->STR_CTRL, MASK_1REG(PHY_STR_CTRL_BDR_PPM_RX, 0x0088));
    //OM_PHY->STR_CTRL = 0x880011;
    REGW(&OM_PHY->RX_GFSK_SYNC_CTRL, MASK_2REG(PHY_RX_GFSK_SYNC_CTRL_XCORR_TH_32, 0x30, PHY_RX_GFSK_SYNC_CTRL_SBE_MAX_TH_32, 1));
    #else
    REGW(&OM_PHY->FD_CFO_CMP, MASK_1REG(PHY_FD_CFO_CMP_FD_CFO_CMP, 1));
    REGW(&OM_PHY->TX_CTRL0, MASK_1REG(PHY_TX_CTRL0_TX_GFSK_MODE, 0x00)); // phy mode0: GFSK MODE
    #endif
}

int8_t drv_om24g_get_rssi(void)
{
    int rssi = (int8_t)OM_PHY->SIG_DBM_EST_O;
    rssi = (rssi*100 - 1100) / 105;
    if(rssi > -25)
        rssi -= 1;

    return rssi;
}

void drv_om24g_set_freq(uint16_t freq, float fract_freq)
{
    uint32_t fract_value;

    REGW(&OM_DAIF->FREQ_CFG0, MASK_1REG(DAIF_FREQ_REG_MO, freq));
    if (fract_freq) {
        fract_value = (uint32_t)(fract_freq * 0x3FFFFF);
        REGW(&OM_DAIF->FREQ_CFG3, MASK_2REG(DAIF_FREQ_FRAC_REG, fract_value, DAIF_FREQ_0P5_EN, 1));
    } else {
        REGW(&OM_DAIF->FREQ_CFG3, MASK_2REG(DAIF_FREQ_FRAC_REG, 0, DAIF_FREQ_0P5_EN, 0));
    }
}

void drv_om24g_pll_continue_open(bool enable)
{
    if (enable) {
        REGW(&OM_DAIF->PD_CFG1, MASK_2REG(DAIF_RFPLL_PD_ALL_ME, 1, DAIF_RFPLL_PD_ALL_MO, 0));
        REGW(&OM_DAIF->MAIN_ST_CFG2, MASK_1REG(DAIF_RX_PLL_WAIT, 0xA0));  // PLL calib = 10us
        OM_DAIF->MAIN_ST_CFG0 = 0;
        REGW(&OM_DAIF->MAIN_ST_CFG1, MASK_1REG(DAIF_RXLDO_WAIT, 0x190));  // LDO calib = 25us
        DRV_DELAY_US(80);
        REGW(&OM_24G->SETUP_VALUE, MASK_3REG(OM24G_RX_SETUP_VALUE, 0x28, OM24G_TX_SETUP_VALUE, 0x1A, OM24G_RX_TM_CNT, 0xFF));
    } else {
        REGW(&OM_DAIF->PD_CFG1, MASK_2REG(DAIF_RFPLL_PD_ALL_ME, 0, DAIF_RFPLL_PD_ALL_MO, 1));
        REGW(&OM_DAIF->MAIN_ST_CFG2, MASK_1REG(DAIF_RX_PLL_WAIT, 0x320));
        OM_DAIF->MAIN_ST_CFG0 = 0x1300180;
        REGW(&OM_DAIF->MAIN_ST_CFG1, MASK_1REG(DAIF_RXLDO_WAIT, 0x1E0));
        REGW(&OM_24G->SETUP_VALUE, MASK_3REG(OM24G_RX_SETUP_VALUE, 0x62, OM24G_TX_SETUP_VALUE, 0x5B, OM24G_RX_TM_CNT, 0xFF));
    }
}

// If the deviation is dynamically switched, it is prohibited to repeatedly call drv_om24g_init.
void drv_om24g_init(const om24g_config_t *cfg)
{
    // RF clock open
    drv_om24g_control(OM24G_CONTROL_CLK_ENABLE, NULL);
    #if (RTE_OM24G_RF_MODE == 2) // Compatible with SILICONLAB
    REGW(&OM_24G->PKTCTRL0, MASK_3REG(OM24G_PKTCTRL0_STRUCT_SEL, cfg->packet_struct_sel, OM24G_PKTCTRL0_RX_0_1_RVS, 1, OM24G_PKTCTRL0_TX_0_1_RVS, 1));
    #else
    REGW(&OM_24G->PKTCTRL0, MASK_3REG(OM24G_PKTCTRL0_STRUCT_SEL, cfg->packet_struct_sel, OM24G_PKTCTRL0_RX_0_1_RVS, 0, OM24G_PKTCTRL0_TX_0_1_RVS, 0));
    #endif
    if(cfg->packet_struct_sel == OM24G_STRUCTURE_B) {
        REGW(&OM_24G->RX_CTRL, MASK_1REG(OM24G_CRC_WHITE_ORDER, 1));
    } else {
        #if (RTE_OM24G_RF_MODE == 3) // Compatible with NORDIC
        REGW(&OM_24G->RX_CTRL, MASK_1REG(OM24G_CRC_WHITE_ORDER, 1));
        #else
        REGW(&OM_24G->RX_CTRL, MASK_1REG(OM24G_CRC_WHITE_ORDER, 0));
        #endif
    }
    REGW(&OM_24G->PKTCTRL0, MASK_1REG(OM24G_PKTCTRL0_CE_H_THRE, 1));
    REGW(&OM_24G->PRE_GUARD, MASK_5REG(OM24G_PREGRD_CNT, 0x07, OM24G_PREGRD_EN, 0x00, OM24G_TAIL_CTL, 0x10, OM24G_GUARD_EN, 0x00, OM24G_TAIL_PATERN, 0X02));
    //OM_24G->TAIL_DATA = 0xFF00FF00;
    OM_24G->PREAMBLE = cfg->preamble;
    OM_24G->PREAMBLE_LEN = cfg->preamble_len;
    OM_24G->SYNC_WORD_SEL = cfg->sync_word_sel;
    OM_24G->SYNC_WORD0 = cfg->sync_word0;
    OM_24G->SYNC_WORD1 = cfg->sync_word1;
    OM_24G->TX_ADDR = cfg->tx_addr;
    OM_24G->RX_ADDR_P0 = cfg->rx_addr;
    REGW(&OM_24G->PKTCTRL0, MASK_2REG(OM24G_PKTCTRL0_NUM_HDR_BITS, cfg->hdr_bits, OM24G_PKTCTRL0_ADDR_CHK, cfg->addr_chk));
    OM_24G->PACKET_LEN = cfg->static_len;
    REGW(&OM_24G->FB_PKTCTRL, MASK_5REG(OM24G_FB_PKTCTRL_NUM_ADDR1_BITS, cfg->addr1_bits, OM24G_FB_PKTCTRL_NUM_LEN_BITS, cfg->len_bits, OM24G_FB_PKTCTRL_ADDR1_POS, cfg->addr1_pos, OM24G_FB_PKTCTRL_LEN_POS, cfg->len_pos, OM24G_FB_PKTCTRL_ADDR1_LOC, cfg->addr1_loc));
    REGW(&OM_24G->EN_AA, 0xFF, (cfg->ack_en ? 0xFF : 0));
    REGW(&OM_24G->FEATURE, MASK_2REG(OM24G_EN_DPL, (cfg->dpl_en ? 1 : 0), OM24G_EN_ACK_PAY, (cfg->ack_en ? 1 : 0)));
    REGW(&OM_24G->DYNPD, 0xFF, (cfg->dpl_en ? 0xFF : 0));

    REGW(&OM_24G->WHITECFG, MASK_4REG(OM24G_WHITE_EN, cfg->white_en, OM24G_WHITE_SKIP_HEADER, cfg->white_skip_hdr, OM24G_WHITE_SKIP_ADDR, cfg->white_skip_addr, OM24G_WHITE_SKIP_CRC, cfg->white_skip_crc));
    OM_24G->WHITESEL = cfg->white_sel;
    OM_24G->WHITESEED = cfg->white_seed;
    OM_24G->WHITEOBIT = cfg->white_obit;

    REGW(&OM_24G->CRCCFG, MASK_3REG(OM24G_CRC_EN, cfg->crc_en, OM24G_CRC_LEN, cfg->crc_len, OM24G_CRC_MODE, cfg->crc_mode));
    if (cfg->endian == OM24G_ENDIAN_MSB) {
        #if (RTE_OM24G_RF_MODE == 2) // Compatible with SILICONLAB
        REGW(&OM_24G->ENDIAN, MASK_7REG(OM24G_EDIBIT_CRC, OM24G_ENDIAN_MSB, OM24G_EDIBIT_PL, OM24G_ENDIAN_MSB, OM24G_EDIBYTE_SW, OM24G_ENDIAN_LSB, OM24G_EDIBIT_SW, OM24G_ENDIAN_MSB, OM24G_EDIBIT_ADDR, OM24G_ENDIAN_MSB, OM24G_EDIBIT_HDR, OM24G_ENDIAN_MSB, OM24G_EDIBYTE_CRC, OM24G_ENDIAN_MSB));
        #else
        REGW(&OM_24G->ENDIAN, MASK_7REG(OM24G_EDIBIT_CRC, OM24G_ENDIAN_MSB, OM24G_EDIBIT_PL, OM24G_ENDIAN_MSB, OM24G_EDIBYTE_SW, OM24G_ENDIAN_MSB, OM24G_EDIBIT_SW, OM24G_ENDIAN_MSB, OM24G_EDIBIT_ADDR, OM24G_ENDIAN_MSB, OM24G_EDIBIT_HDR, OM24G_ENDIAN_MSB, OM24G_EDIBYTE_CRC, OM24G_ENDIAN_MSB));
        #endif
    } else {
        REGW(&OM_24G->ENDIAN, MASK_7REG(OM24G_EDIBIT_CRC, OM24G_ENDIAN_MSB, OM24G_EDIBIT_PL, OM24G_ENDIAN_LSB, OM24G_EDIBYTE_SW, OM24G_ENDIAN_LSB, OM24G_EDIBIT_SW, OM24G_ENDIAN_LSB, OM24G_EDIBIT_ADDR, OM24G_ENDIAN_LSB, OM24G_EDIBIT_HDR, OM24G_ENDIAN_LSB, OM24G_EDIBYTE_CRC, OM24G_ENDIAN_MSB));
        //REGW(&OM_24G->ENDIAN, MASK_7REG(OM24G_EDIBIT_CRC, OM24G_ENDIAN_MSB, OM24G_EDIBIT_PL, OM24G_ENDIAN_LSB, OM24G_EDIBYTE_SW, OM24G_ENDIAN_LSB, OM24G_EDIBIT_SW, OM24G_ENDIAN_LSB, OM24G_EDIBIT_ADDR, OM24G_ENDIAN_MSB, OM24G_EDIBIT_HDR, OM24G_ENDIAN_LSB, OM24G_EDIBYTE_CRC, OM24G_ENDIAN_MSB));
    }
    OM_24G->CRCPOLY = cfg->crc_poly;
    OM_24G->CRCINIT = cfg->crc_init;
    REGW(&OM_24G->CRCSKIP, MASK_3REG(OM24G_CRC_SKIP_SYNC, cfg->crc_skip_sync, OM24G_CRC_SKIP_LEN, cfg->crc_skip_len, OM24G_CRC_SKIP_ADDR, cfg->crc_skip_addr));

    REGW(&OM_24G->FA_SETUP_RETR, MASK_2REG(OM24G_FA_SETUP_RETR_ARC, 0x03, OM24G_FA_SETUP_RETR_ARD, 0x00));
    OM_24G->DMA_RX_ADDR = (uint32_t)cfg->rx_data;
    OM_24G->DMA_TX_ADDR = (uint32_t)cfg->tx_data;
    om24g_env.rx_buf = cfg->rx_data;
    om24g_env.rx_count = 0;
    REGW(&OM_DAIF->MAIN_ST_CFG1, MASK_1REG(DAIF_TX_WAIT, 0));
#if PLL_CONTIMUE_OPEN
    REGW(&OM_DAIF->MAIN_ST_CFG2, MASK_1REG(DAIF_RX_PLL_WAIT, 0xA0));  // PLL calib = 10us
    #if 0 //Single send or single receive mode
    OM_DAIF->MAIN_ST_CFG0 = 0;
    REGW(&OM_DAIF->MAIN_ST_CFG1, MASK_1REG(DAIF_RXLDO_WAIT, 0x190));  // LDO calib = 25us
    DRV_DELAY_US(80);
    REGW(&OM_24G->SETUP_VALUE, MASK_3REG(OM24G_RX_SETUP_VALUE, 0x28, OM24G_TX_SETUP_VALUE, 0x1A, OM24G_RX_TM_CNT, 0xFF));
    #else //Receive and transmit conversion in ACK mode
    REGW(&OM_DAIF->MAIN_ST_CFG0, MASK_2REG(DAIF_TXLDO_WAIT, 0x140, DAIF_TX_PLL_WAIT, 0xA0));
    DRV_DELAY_US(80);
    REGW(&OM_24G->SETUP_VALUE, MASK_3REG(OM24G_RX_SETUP_VALUE, 0x3A, OM24G_TX_SETUP_VALUE, 0x33, OM24G_RX_TM_CNT, 0xFF));
    #endif
#else
    #if (RTE_OM24G_RF_MODE == 3) // Compatible with NORDIC
    //Communicate with NORDIC: 1M RX:6A, TX:63  250k RX:78, TX:63, nordic_ack_timeout:300. 2M: RX:6A, TX:6F
    //REGW(&OM_24G->SETUP_VALUE, MASK_3REG(OM24G_RX_SETUP_VALUE, 0x6A, OM24G_TX_SETUP_VALUE, 0x68, OM24G_RX_TM_CNT, 0xFF)); // 1M
    //REGW(&OM_24G->SETUP_VALUE, MASK_3REG(OM24G_RX_SETUP_VALUE, 0x6A, OM24G_TX_SETUP_VALUE, 0x74, OM24G_RX_TM_CNT, 0xFF)); // 2M
    REGW(&OM_24G->SETUP_VALUE, MASK_3REG(OM24G_RX_SETUP_VALUE, 0x78, OM24G_TX_SETUP_VALUE, 0x68, OM24G_RX_TM_CNT, 0xFF)); // 250K
    #else
    REGW(&OM_24G->SETUP_VALUE, MASK_3REG(OM24G_RX_SETUP_VALUE, 0x62, OM24G_TX_SETUP_VALUE, 0x5B, OM24G_RX_TM_CNT, 0xFF)); //0x3F00
    // REGW(&OM_24G->SETUP_VALUE_H, MASK_2REG(OM24G_RX_SETUP_VALUE_H, 1, OM24G_TX_SETUP_VALUE_H, 1));
    #endif
#endif
    //GFSK OR FSK
    if (cfg -> modulation_mode) {
        REGW1(&OM_PHY->TX_CTRL0, PHY_TX_CTRL0_BP_GAU_MASK); //FSK
    } else {
        REGW0(&OM_PHY->TX_CTRL0, PHY_TX_CTRL0_BP_GAU_MASK);
    }
    drv_om24g_detection_mode(cfg->detect_mode);
    //sync word: symble-bit-error criterion
    // REGW0(&OM_PHY->RX_GFSK_SYNC_CTRL, PHY_RX_GFSK_SYNC_CTRL_SBE_MAX_TH_32_MASK);
    //REGW(&OM_PHY->RX_GFSK_SYNC_CTRL, MASK_1REG(PHY_RX_GFSK_SYNC_CTRL_SBE_MAX_TH_32, 1));
    // 1 - enable double address synchronization
    REGW(&OM_PHY->RX_GFSK_SYNC_CTRL, MASK_1REG(PHY_RX_GFSK_SYNC_CTRL_EN_2ND_ADDR, 0));
    //0 - All packages are forced to ACK, ignoring NO_ ACK bit.
    REGW(&OM_24G->FA_PKTCTRL, MASK_1REG(OM24G_FA_PKTCTRL_EN_DYN_ACK, 1));
    //REGW(&OM_24G->FA_PKTCTRL, MASK_1REG(OM24G_FA_PKTCTRL_LEN_10B_EN, 1));

    //REGW(&OM_24G->RX_CTRL, MASK_3REG(OM24G_ADDR_MISS_RESET_ANA_EN, 0, OM24G_MAX_LEN_RESET_ANA_EN, 0, OM24G_LOOP_SEND_MODE_EN, 0));
    // Read RSSI background noise
    //OM_PHY->RSSI_CAP_MODE = 1;
    REGW(&OM_24G->RX_CTRL, MASK_1REG(OM24G_RX_DONG_DIS, 0));
    NVIC_ClearPendingIRQ(OM24G_RF_IRQn);
    NVIC_SetPriority(OM24G_RF_IRQn, RTE_OM24G_IRQ_PRIORITY);
    NVIC_EnableIRQ(OM24G_RF_IRQn);
    NVIC_ClearPendingIRQ(OM24G_TIM_WAKEUP_IRQn);
    NVIC_SetPriority(OM24G_TIM_WAKEUP_IRQn, RTE_OM24G_IRQ_PRIORITY);
    NVIC_EnableIRQ(OM24G_TIM_WAKEUP_IRQn);
    REGW(&OM_24G->RF_PD_AHEAD, MASK_2REG(OM24G_RF_PD_AHEAD_EN, 1, OM24G_RF_PD_AHEAD, 1));
    //REGW(&OM_24G->ACK_MODE, MASK_1REG(OM24G_CONTINUOUS_MODE, 0));
    REGW(&OM_24G->ACK_MODE, MASK_1REG(OM24G_ACK_MODE, 0));
    drv_om24g_set_rf_parameters(cfg->data_rate, cfg->freq, 0);
    if (SYS_IS_FPGA()) {
        drv_om24g_rf_init_seq();
    }
}

__RAM_CODE
void drv_om24g_store(void)
{
    DRV_RCC_ANA_CLK_ENABLE_NOIRQ();
    om24g_env.reg_store.MIX_CFG0 = OM_DAIF->MIX_CFG0;
    om24g_env.reg_store.PD_CFG1 = OM_DAIF->PD_CFG1;
    om24g_env.reg_store.MAIN_ST_CFG1 = OM_DAIF->MAIN_ST_CFG1;
    DRV_RCC_ANA_CLK_RESTORE_NOIRQ();
    #if HARDWARE_TIMER_ENABLE
    REGW1(&OM_PMU->BLE_CLK_SWITCH, PMU_BLE_CLK_SWITCH_BLE_CLK_SWITCH_MASK);
    #endif
}

__RAM_CODE
void drv_om24g_restore(void)
{
    DRV_RCC_ANA_CLK_ENABLE_NOIRQ();
    OM_DAIF->MIX_CFG0 = om24g_env.reg_store.MIX_CFG0;
    OM_DAIF->PD_CFG1 = om24g_env.reg_store.PD_CFG1;
    OM_DAIF->MAIN_ST_CFG1 = om24g_env.reg_store.MAIN_ST_CFG1;
    DRV_RCC_ANA_CLK_RESTORE_NOIRQ();
    #if HARDWARE_TIMER_ENABLE
    REGW0(&OM_PMU->BLE_CLK_SWITCH, PMU_BLE_CLK_SWITCH_BLE_CLK_SWITCH_MASK);
    while(!(OM_PMU->BLE_RECOVER_FLAG & PMU_BLE_RECOVER_FLAG_RECOVER_DONE_FLAG_MASK));
    OM_PMU->BLE_RECOVER_FLAG = 1;
    drv_om24g_control(OM24G_CONTROL_CLK_ENABLE, NULL);
    #endif
}

__RAM_CODE
void drv_om24g_isr(void)
{
    uint32_t status = 0;
    uint16_t rx_cnt = 0;
    bool rx_right_flag = false;

    status = OM_24G->INT_ST;
    // OM_LOG(OM_LOG_DEBUG, "INT_ST: %02x\r\n", OM_24G->INT_ST_RAW);
    OM_24G->INT_ST = OM_24G->INT_ST_RAW;
    if((OM24G_INT_RX_DR_MASK & status) && (!(OM24G_INT_CRC_ERR_MASK & status)) && (!(OM24G_INT_RX_OVERLAY_MASK & status)) && (!(OM24G_INT_MAX_LEN_MASK & status)) && (!(OM24G_RX_ADDR_MISS_MASK & status))) {
        OM_CRITICAL_BEGIN();
        if((!(OM_24G->STATE & OM24G_SYNC_DET_MASK)) && (!(OM24G_INT_CRC_ERR_MASK & OM_24G->INT_ST)) && (!(OM24G_INT_RX_OVERLAY_MASK & OM_24G->INT_ST)) && (!(OM24G_INT_MAX_LEN_MASK & OM_24G->INT_ST)) && (!(OM24G_RX_ADDR_MISS_MASK & OM_24G->INT_ST))) {
            om24g_env.rx_count = (om24g_env.rx_count + 1) % 2;
            OM_24G->DMA_RX_ADDR = (uint32_t)(om24g_env.rx_buf + om24g_env.max_rx_num * om24g_env.rx_count);
            rx_cnt = drv_om24g_read_rx_payload_width();
            rx_right_flag = true;
            REGW(&OM_24G->RX_DONE, MASK_1REG(OM24G_RX_DONE, 1));
        } else {
            OM_24G->INT_ST = OM_24G->INT_ST_RAW;
            rx_right_flag = false;
            REGW(&OM_24G->RX_DONE, MASK_1REG(OM24G_RX_DONE, 1));
        }
        OM_CRITICAL_END();
        if (om24g_env.event_cb && rx_right_flag) {
            om24g_env.event_cb(OM_24G, DRV_EVENT_COMMON_RECEIVE_COMPLETED, (void *)(om24g_env.rx_buf + om24g_env.max_rx_num * (om24g_env.rx_count ? 0 : 1)), (void *)((uint32_t)rx_cnt));
        }
    } else {
        REGW(&OM_24G->RX_DONE, MASK_1REG(OM24G_RX_DONE, 1));
    }
    if ((OM24G_INT_TX_DS_MASK & status)) {
        if (om24g_env.event_cb) {
            om24g_env.event_cb(OM_24G, DRV_EVENT_COMMON_TRANSMIT_COMPLETED, NULL, NULL);
        }
    }
    if (OM24G_INT_MAX_RT_MASK & status) {
        if (om24g_env.event_cb) {
            om24g_env.event_cb(OM_24G, DRV_EVENT_OM24G_MAX_RT, NULL, NULL);
        }
    }
    if (OM24G_INT_RX_TM_MASK & status) {
        if (om24g_env.event_cb) {
            om24g_env.event_cb(OM_24G, DRV_EVENT_OM24G_RX_TM, NULL, NULL);
        }
    }
    if (OM24G_INT_RX_OVERLAY_MASK & status) {
        if (om24g_env.event_cb) {
            om24g_env.event_cb(OM_24G, DRV_EVENT_OM24G_RX_OVERLAY, NULL, NULL);
        }
    }
    if (OM24G_INT_TIMER0_MASK & status) {
        // TIMER0
        // OM_24G->TMR0_NXT_32K_TP = last_32k + 16384; // 512ms
        // OM_24G->TMR0_NXT_8M_TP = last_8M + 0;
        // last_32k = last_32k + 16384;
        // last_8M = last_8M + 0;
        // OM_PMU->TMR_2G4_TIM_WAKEUP_NATIVE = last_32k - 100;
        // OM_PMU->TMR_2G4_TIM_WAKEUP_SKIP = 0x20;
        if (om24g_env.event_cb) {
            om24g_env.event_cb(OM_24G, DRV_EVENT_OM24G_INT_TIMER0, NULL, NULL);
        }
    }
    if (OM24G_INT_TIMER1_MASK & status) {
        if (om24g_env.event_cb) {
            om24g_env.event_cb(OM_24G, DRV_EVENT_OM24G_INT_TIMER1, NULL, NULL);
        }
    }
    if (OM24G_INT_CRC_ERR_MASK & status) {
    }
    if (OM24G_INT_SYNC0_DET_MASK & status) {
        if (om24g_env.event_cb) {
            om24g_env.event_cb(OM_24G, DRV_EVENT_OM24G_SYNC, NULL, NULL);
        }
    }
}

void drv_om24g_tim_wakeup_isr(void)
{
    //OM_LOG(OM_LOG_DEBUG, "tim wakeup \r\n");
    REGW(&OM_PMU->OSC_INT_CTRL, MASK_1REG(PMU_OSC_INI_CTRL_2G4_WAKEUP_INT_CLR, 1));
    //drv_gpio_toggle(OM_GPIO0, GPIO_MASK(12));
}

/* drv_om24g_control() contains drv_pmu_ana_enable(), so turning on or turning off the clock only requires calling drv_om24g_control(). */
void *drv_om24g_control(om24g_control_t control, void *argu)
{
    uint32_t ret;

    ret = (uint32_t)OM_ERROR_OK;

    OM_CRITICAL_BEGIN();
    switch (control) {
        case OM24G_CONTROL_CLK_DISABLE:
            {
                DRV_RCC_ANA_CLK_ENABLE_NOIRQ();
                OM24G_CE_LOW();
                OM24G_FLUSH_TX_FIFO();
                OM24G_FLUSH_RX_FIFO();
                OM24G_ABORT_RF();
                DRV_DELAY_US(2);
                DRV_DAIF_WAIT_IDLE();
                #if PLL_CONTIMUE_OPEN
                REGW(&OM_DAIF->PD_CFG1, MASK_2REG(DAIF_RFPLL_PD_ALL_ME, 0, DAIF_RFPLL_PD_ALL_MO, 1));
                #endif
                OM_24G->INT_MASK = 0xFFFFFFFF;
                REGW0(&OM_24G->PKTCTRL0, OM24G_PKTCTRL0_MAC_SEL_MASK); // switch phy between 2.4G and ble.
                REGW(&OM_DAIF->FREQ_CFG0, MASK_1REG(DAIF_FREQ_REG_ME, 0));
                REGW1(&OM_DAIF->MIX_CFG0, DAIF_FREQ_DEVIA_BYPASS_TX_MASK); //freq devia bypass=0d
                REGW1(&OM_DAIF->PLL_CTRL1, DAIF_FREQ_DEVIA_BYPASS_3RD_MASK); //freq devia bypass=0d
                DRV_RCC_ANA_CLK_RESTORE_NOIRQ();
                drv_pmu_ana_enable(false, PMU_ANA_RF_24G);
            }
            break;
        case OM24G_CONTROL_CLK_ENABLE:
            {
                drv_pmu_ana_enable(true, PMU_ANA_RF_24G);
                DRV_RCC_CLOCK_ENABLE(RCC_CLK_DAIF, 1U);
                DRV_RCC_CLOCK_ENABLE(RCC_CLK_2P4, 1U);
                REGW1(&OM_24G->PKTCTRL0, OM24G_PKTCTRL0_MAC_SEL_MASK);
                OM24G_CE_LOW();
                OM_24G->INT_MASK = 0xFFFFFFFF;
                REGW(&OM_DAIF->FREQ_CFG0, MASK_1REG(DAIF_FREQ_REG_ME, 1));
                if(om24g_env.deviation_below_250K) {
                    REGW0(&OM_DAIF->MIX_CFG0, DAIF_FREQ_DEVIA_BYPASS_TX_MASK); //freq devia bypass=0d
                    REGW0(&OM_DAIF->PLL_CTRL1, DAIF_FREQ_DEVIA_BYPASS_3RD_MASK); //freq devia bypass=0d
                }
                // Block unnecessary interrupts
                OM_24G->INT_MASK = 0xFFFCF81E;
                #if PLL_CONTIMUE_OPEN
                REGW(&OM_DAIF->PD_CFG1, MASK_2REG(DAIF_RFPLL_PD_ALL_ME, 1, DAIF_RFPLL_PD_ALL_MO, 0));
                #endif
                REGW(&OM_24G->RX_DONE, MASK_1REG(OM24G_RX_DONE, 1));
            }
            break;
        case OM24G_CONTROL_RESET:
            {
                DRV_RCC_RESET(RCC_CLK_2P4);
                DRV_RCC_RESET(RCC_CLK_PHY);
                REGW1(&OM_24G->PKTCTRL0, OM24G_PKTCTRL0_MAC_SEL_MASK);
            }
            break;
        default:
            break;
    }
    OM_CRITICAL_END();

    return (void *)ret;
}

// true: idle  false: tx busy
bool drv_om24g_tx_idle(void)
{
    bool tx_status = false;

    REGW(&OM_DAIF->DBG_REG, MASK_2REG(DAIF_DBG_EN, 1, DAIF_DBG_IDX, 3));
    tx_status = ((REGR(&OM_DAIF->DBG_REG, MASK_POS(DAIF_DBG_DATA)) & 0x20) || (REGR(&OM_24G->STATE, MASK_POS(OM24G_MAIN_STATE)) == 0x03)) ? false : true;
    REGW(&OM_DAIF->DBG_REG, MASK_1REG(DAIF_DBG_EN, 0));

    return tx_status;
}

bool drv_om24g_rx_idle(void)
{
    bool rx_status = false;

    if(!(OM_24G->STATE & OM24G_SYNC_DET_MASK)) {
        rx_status = true;
    }

    return rx_status;
}

/*****************************************************************************************
 * @brief Ripple specific write access
 *
 * @param[in] addr    register address
 * @param[in] value   value to write
 *
 * @return uint32_t value
 ****************************************************************************************/
static void drv_om24g_rf_reg_wr(uint32_t addr, uint32_t value)
{
    REGW(&OM_24G->DBG_SPI0, MASK_2REG(OM24G_DBG_SPI0_ADDRESS, addr, OM24G_DBG_SPI0_DATA, value));
    REGW(&OM_24G->DBG_SPI1, MASK_1REG(OM24G_DBG_SPI1_START, 1));
    DRV_DELAY_US(10);
    // Waiting SW driven SPI Access completion
    while(REGR(&OM_24G->DBG_SPI1, MASK_POS(OM24G_DBG_SPI1_COMP)) == 0);
}

/***************************************************************************************
 * @brief Static Ripple radio Calibration function.
 ***************************************************************************************
 */
static void drv_om24g_rf_calib(void)
{
    //Automatic VCO subband selection
    // 1. set D8=0(Addr:0x05) to enable the automatic VCO sub-band selection;
    drv_om24g_rf_reg_wr(0x05, 0x1824);
    // 2. Enable the PLL and VCO if required.
    // 3. set D7=1(Addr:0x05) to start the FSM
    drv_om24g_rf_reg_wr(0x05,(0x1824 | (1<<7)));
    // 4. The VCO sub-band selection and PLL setting time takes less than 300us
    DRV_DELAY_US(300);
    // 5. Reset the FSM
    drv_om24g_rf_reg_wr(0x05,0x1824);
}

void drv_om24g_rf_init_seq(void)
{
    /****************************
     * MAX2829 Initialise
     ****************************/
    drv_om24g_rf_reg_wr(0x00, 0x1140);                    /*Register 0*/
    drv_om24g_rf_reg_wr(0x01, 0x00CA);                    /*Register 1*/
    drv_om24g_rf_reg_wr(0x02, 0x1007);                    /*standby */
    drv_om24g_rf_reg_wr(0x03, 0x30A2);                    /*Intger-Divider Ratio*/
    drv_om24g_rf_reg_wr(0x04, 0x1DDD);                    /*Fractional-Divider Ratio*/
    drv_om24g_rf_reg_wr(0x05, 0x1824);                    /*Band select and PLL*/
    drv_om24g_rf_reg_wr(0x06, 0x1C00);                    /*calibration*/
    drv_om24g_rf_reg_wr(0x07, 0x002A);                    /*lowpass filter*/
    drv_om24g_rf_reg_wr(0x08, 0x1C25);                    /*Rx control/RSSI*/
    drv_om24g_rf_reg_wr(0x09, 0x0603);                    /*Tx linearity/baseband gain*/
    drv_om24g_rf_reg_wr(0x0A, 0x03C0);                    /*PA bias DAC*/
    drv_om24g_rf_reg_wr(0x0B, 0x004B);                    /*Rx Gain, LNA gain=15dB, VGA=22dB*/
//    drv_om24g_rf_reg_wr(0x0B, 0x0006);                    /*Rx Gain, for Shielding Box*/
//    drv_om24g_rf_reg_wr(0x0B, 0x007F);                    /*Rx Gain, LNA gain=30dB, VGA=64dB, input loss -22dB*/
    drv_om24g_rf_reg_wr(0x0C, 0x003F);                    /*Tx VGA Gain = 0dB*/

//    HS_PHY->EN_SDET = 1;
//    *(volatile uint32_t *)(HS_PHY_BASE + 0xC4) = 0;

    //calibration procedure
    drv_om24g_rf_calib();
}

#endif /*RTE_OM24G*/

/*-------------------------------------------End Of File---------------------------------------------*/

/** @} */
