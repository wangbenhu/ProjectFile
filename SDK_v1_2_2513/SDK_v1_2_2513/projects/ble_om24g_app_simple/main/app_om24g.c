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
 * @brief    om24g app
 * @details  om24g app
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
#include "cmsis_os2.h"
#include "bsp.h"
#include "om_driver.h"
#include "shell.h"
#include "evt.h"
#include "pm.h"
#include "bsp.h"
#include "nvds.h"
#include "om_log.h"

#include "obc.h"
#include "omble.h"

/*********************************************************************
 * MACROS
 */
#define TX_ROLE 1
#define OM24G_ACK_MODE 0
#define ENABLE_SLEEP_MODE 0
/*********************************************************************
 * LOCAL VARIABLES
 */

static volatile uint16_t error_count = 0;
static volatile uint16_t right_count = 0;
static volatile uint16_t tx_count = 0;
static volatile uint16_t max_rty_count = 0;
static volatile uint16_t time_out_count = 0;
static volatile  bool time_out_flag = false;
static volatile  bool max_try_flag = false;
static volatile  bool om24g_tx_flag = false;
static volatile  bool om24g_rx_flag = false;
static volatile uint32_t timeout = 0;

uint8_t om24g_tx_payload[166];
uint8_t om24g_rx_payload[166*2];

/**
 *******************************************************************************
 * @brief 24G configuration parameter
 *
 * Package structure B、NO ACK mode、Dynamic length mode、Compatible with 6626、nordic、TI、siliconlab chip
 * 4 bytes sync word : 0xEDD47656
 * 1bytes header:      packet lenth
 * 0~32 bytes payload: {1,2,......32}
 * 2 bytes crc: seed = 0xFFFF crc_poly = 0x1021
 *
 * data whitening ranges include header field, payload field, and crc field.
 * the scope of CRC includes sync_word, header, payload.
 *
 * @param[in] static_len  The maximum packet length is 32 bytes
 * @param[in] freq        2480MHZ
 *******************************************************************************
 */
/*Package structure B*/
om24g_config_t om24g_config_b = {
    .tx_data             = om24g_tx_payload,
    .rx_data             = om24g_rx_payload,
    .packet_struct_sel   = OM24G_STRUCTURE_B,
    .preamble            = 0xaa,
    .preamble_len        = 0x04,
    .sync_word_sel       = OM24G_SYNCWORD0,
    .sync_word0          = 0xEDD47656,
    .sync_word1          = 0xEDD47656,
    .tx_addr             = 0x02,
    .rx_addr             = 0x02,
    .addr_chk            = 0x00,
    .static_len          = 64,
    .hdr_bits            = 0x08,
    .addr1_bits          = 0x00,
    .len_bits            = 0x08,
    .addr1_pos           = 0x00,
    .addr1_loc           = 0x00,
    .len_pos             = 0x00,
    .endian              = OM24G_ENDIAN_MSB,
    .freq                = 2478,
    .data_rate           = OM24G_RATE_1M,
    .ack_en              = 0,
    .dpl_en              = 1,
    .white_en            = 0,
    .white_skip_hdr      = 1,
    .white_skip_addr     = 1,
    .white_skip_crc      = 0,
    .white_sel           = 0x00,
    .white_seed          = 0x80,
    .white_obit          = 0x07,
    .crc_len             = OM24G_CRC_2BYTE,
    .crc_en              = 1,
    .crc_mode            = 0,
    .crc_poly            = 0x1021, // 0x07,
    .crc_init            = 0xFFFF, // 0xFF,
    .crc_skip_sync       = 0,
    .crc_skip_len        = 0,
    .crc_skip_addr       = 0,
    .modulation_mode     = OM24G_MODULATION_GFSK,
    .detect_mode         = OM24G_SOFT_DETECTION,
};

/*********************************************************************
 * EXTERN FUNCTIONS
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */
/// log debug array with show more
#define OM_LOG_DEBUG_ARRAY_EX(note, array, len)    do{OM_LOG(OM_LOG_DEBUG, "%s: \r\n",note); OM_LOG_HEXDUMP(OM_LOG_DEBUG, array, len, 16); OM_LOG(OM_LOG_DEBUG, "[%dbytes]\n", len);}while(0)

static void om24g_callback(void *om_reg, drv_event_t drv_event, void *buff, void *num)
{
    uint16_t payload_lenth = 0;
    bool error_flag = false;

    switch (drv_event) {
        case DRV_EVENT_COMMON_RECEIVE_COMPLETED:
            payload_lenth = (uint32_t)num;
            for (int i = 0x00; i < (payload_lenth - 1); i++) {
                if (*((uint8_t *)buff + i) != *((uint8_t *)buff + i + 1)) {
                    error_flag = true;
                }
            }
            REGW(&OM_24G->RX_DONE, MASK_1REG(OM24G_RX_DONE, 1));
            if (error_flag) {
                error_count++;
                error_flag = false;
                OM_LOG_DEBUG_ARRAY_EX("err Pkt", buff, payload_lenth);
            } else {
                right_count++;
                OM_LOG_DEBUG_ARRAY_EX("Pkt", buff, payload_lenth);
                //OM_LOG(OM_LOG_DEBUG, "pkt:%d \r\n", *((uint8_t *)buff + payload_lenth - 1));
            }
            //OM_LOG(OM_LOG_DEBUG, "si: %d\r\n", drv_om24g_get_rssi());
            //osDelay(500);
            //DRV_DELAY_MS(500);
            OM_LOG(OM_LOG_DEBUG, "E:%d ", error_count);
            OM_LOG(OM_LOG_DEBUG, "R:%d \r\n", right_count);

            #if ENABLE_SLEEP_MODE
            OM24G_CE_LOW();
            pm_sleep_allow(PM_ID_24G);
            drv_om24g_control(OM24G_CONTROL_CLK_DISABLE, NULL);
            drv_pmu_timer_trig_set(PMU_TIMER_TRIG_VAL0, (drv_pmu_timer_cnt_get() + PMU_TIMER_MS2TICK(500)));
            #endif

            break;
        case DRV_EVENT_OM24G_MAX_RT:   // max retry
            break;
        case DRV_EVENT_OM24G_RX_OVERLAY:
            OM_LOG(OM_LOG_DEBUG, "overlay\r\n");
            break;
        case DRV_EVENT_OM24G_INT_TIMER0:
            //OM_LOG(OM_LOG_DEBUG, "om24g int timer\r\n");
            drv_gpio_toggle(OM_GPIO0, GPIO_MASK(7));
            break;
        case DRV_EVENT_OM24G_INT_TIMER1:
            //OM_LOG(OM_LOG_DEBUG, "om24g int timer\r\n");
            drv_gpio_toggle(OM_GPIO0, GPIO_MASK(9));
            break;
        case DRV_EVENT_COMMON_TRANSMIT_COMPLETED:
            //pm_sleep_allow(PM_ID_24G);
            //drv_om24g_control(OM24G_CONTROL_CLK_DISABLE, NULL);
            //drv_pmu_timer_trig_set(PMU_TIMER_TRIG_VAL0, (drv_pmu_timer_cnt_get() + PMU_TIMER_MS2TICK(500)));
            tx_count++;
            drv_gpio_toggle(OM_GPIO0, GPIO_MASK(8));

            #if 0
            drv_om24g_write_int(om24g_tx_payload, 32);
            // OM_24G->TMR0_NXT_32K_TP = OM_24G->TMR_NATIVE_CNT_32K + 16384; //16384;
            // OM_24G->TMR0_NXT_8M_TP = OM_24G->TMR_NATIVE_CNT_8M + 0;
            OM_24G->TMR1_NXT_32K_TP = OM_24G->TMR_NATIVE_CNT_32K + 16384; //16384;
            OM_24G->TMR1_NXT_8M_TP = OM_24G->TMR_NATIVE_CNT_8M + 0;
            #endif

            OM_LOG(OM_LOG_DEBUG, "tx_cnt: %d\r\n", tx_count);
            if(tx_count == 100) {
                tx_count = 0;
                drv_pmu_timer_control(PMU_TIMER_TRIG_VAL0, PMU_TIMER_CONTROL_ENABLE, (void *)0);
                drv_om24g_control(OM24G_CONTROL_CLK_DISABLE, NULL);
            }
            break;
        default:
            OM_ASSERT(0);
            break;
    }
}

static void app_24g_ble_bb_frame_ongoing_handler(bool is_ongoing)
{
    static bool low_rate_enable = false;
    if (is_ongoing)
    {
        // BLE is running, close 2.4g function.
        if (REGR(&OM_24G->RF_DR, MASK_POS(OM24G_EN_TX_ARB)) == true) {
            low_rate_enable = true;
            REGW(&OM_24G->RF_DR, MASK_2REG(OM24G_EN_RX_ARB, false, OM24G_EN_TX_ARB, false));
            REGW(&OM_PHY->DET_MODE, MASK_1REG(PHY_DET_MODE_DET_MODE, 0x00)); // OM24G_SOFT_DETECTION
        }
        if (SYS_IS_FPGA()) {
            OM_24G->FPGA_TX_INDEX = 0x02;
        }
        REGW(&OM_PHY->H_RX_CTRL, MASK_2REG(PHY_H_RX_CTRL_FXP, 0x40, PHY_H_RX_CTRL_RVS_FXP, 0x80)); // RX modulation index = 0.5
        drv_om24g_control(OM24G_CONTROL_CLK_DISABLE, NULL);
        //OM_LOG(OM_LOG_DEBUG, "BLE----- \r\n");
    }
    else
    {
        // BLE is stoped, resume 2.4g function.
        if (low_rate_enable) {
            DRV_RCC_CLOCK_ENABLE(RCC_CLK_2P4, 1U);
            REGW(&OM_24G->RF_DR, MASK_2REG(OM24G_EN_RX_ARB, true, OM24G_EN_TX_ARB, true));
            REGW(&OM_PHY->DET_MODE, MASK_1REG(PHY_DET_MODE_DET_MODE, 0x03)); // OM24G_DPLL_DETECTION
        }
        drv_om24g_control(OM24G_CONTROL_CLK_ENABLE, NULL);
        #if (RTE_OM24G_RF_MODE == 3) // Compatible with NORDIC
        REGW(&OM_PHY->H_RX_CTRL, MASK_2REG(PHY_H_RX_CTRL_FXP, 0x29, PHY_H_RX_CTRL_RVS_FXP, 0xC8)); // RX modulation index = 0.32
        #endif
        drv_om24g_read_int(om24g_rx_payload, 166);
        //OM_LOG(OM_LOG_DEBUG, "*****************OM24G \r\n");
    }
}

/*Packet structure B receives packets in interrupt mode and can communicate with function om24g_write_int_structure_b(void). */
static void om24g_read_int_structure_b(void)
{
    drv_om24g_init(&om24g_config_b);
    //drv_om24g_set_rf_parameters(OM24G_RATE_250K, 2404, 0.055281); //2404.055281MHZ  0    231865.264143
    drv_om24g_register_event_callback(om24g_callback);
    drv_om24g_dump_rf_register();
    drv_om24g_read_int(om24g_rx_payload, 166);
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */

void app_24g_init(void)
{
    obc_bb_frame_ongoing_callback_register(app_24g_ble_bb_frame_ongoing_handler);
    OM_ASSERT_WHILE(true, (sizeof(om24g_rx_payload) >= (2*om24g_config_b.static_len)));
    om24g_read_int_structure_b();

}


/** @} */