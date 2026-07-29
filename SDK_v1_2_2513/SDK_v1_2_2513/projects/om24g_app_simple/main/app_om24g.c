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
#include "om_log.h"


/*******************************************************************************
 * MACROS
 */
#define TX_ROLE 0
#define OM24G_ACK_MODE 0
#define ENABLE_SLEEP_MODE 0


/*******************************************************************************
 * CONST & VARIABLES
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
static osSemaphoreId_t xSemOm24g = NULL;

uint8_t om24g_tx_payload[166];
uint8_t om24g_rx_payload[166*2];

// Compatible with TI2640
#if (RTE_OM24G_RF_MODE == 1)

#if 1 // dynamic length mode
om24g_config_t om24g_config_b = {
    .tx_data             = om24g_tx_payload,
    .rx_data             = om24g_rx_payload,
    .packet_struct_sel   = OM24G_STRUCTURE_B,
    .preamble            = 0xaa,
    .preamble_len        = 0x01,
    .sync_word_sel       = OM24G_SYNCWORD0,
    .sync_word0          = 0x52567853,//0x52564656,//0xA1357BDA,
    .sync_word1          = 0x52567853,
    .tx_addr             = 0xaa,
    .rx_addr             = 0xaa,
    .addr_chk            = 0x00,
    .hdr_bits            = 0x08,
    .static_len          = 0x90,
    .addr1_bits          = 0x00,
    .len_bits            = 0x08,
    .addr1_pos           = 0x00,
    .len_pos             = 0x00,
    .endian              = OM24G_ENDIAN_LSB,
    .freq                = 2471,
    .data_rate           = OM24G_RATE_2M,
    .ack_en              = 0,
    .dpl_en              = 1,
    .white_en            = 0,
    .white_skip_hdr      = 0,
    .white_skip_addr     = 0,
    .white_skip_crc      = 0,
    .white_sel           = 0x06,
    .white_seed          = 0x1ff,
    .white_obit          = 0x07,
    .crc_len             = OM24G_CRC_2BYTE,
    .crc_en              = 1,
    .crc_mode            = 0,
    .crc_poly            = 0x1021, //0x07,
    .crc_init            = 0x1D0F, //0xFF,
    .crc_skip_sync       = 0,
    .crc_skip_len        = 0,
    .crc_skip_addr       = 0,
    .modulation_mode     = OM24G_MODULATION_GFSK,
    .detect_mode         = OM24G_SOFT_DETECTION, //1/2M: OM24G_SOFT_DETECTION, 100/500K: OM24G_HARD_DETECTION,
};

#else // Fixed length mode

om24g_config_t om24g_config_b = {
    .tx_data             = om24g_tx_payload,
    .rx_data             = om24g_rx_payload,
    .packet_struct_sel   = OM24G_STRUCTURE_B,
    .preamble            = 0xAA,
    .preamble_len        = 0x01,
    .sync_word_sel       = OM24G_SYNCWORD0,
    .sync_word0          = 0x52567853, //0xA1357BDA, //0x52567853,
    .sync_word1          = 0x52567853,
    .tx_addr             = 0xaa,
    .rx_addr             = 0xaa,
    .addr_chk            = 0x00,
    .hdr_bits            = 0x00,
    .static_len          = 0x20,
    .addr1_bits          = 0x00,
    .len_bits            = 0x00,
    .addr1_pos           = 0x00,
    .len_pos             = 0x00,
    .endian              = OM24G_ENDIAN_LSB,
    .freq                = 2432,
    .data_rate           = OM24G_RATE_1M,
    .ack_en              = 0,
    .dpl_en              = 0,
    .white_en            = 1,
    .white_skip_hdr      = 1,
    .white_skip_addr     = 0,
    .white_skip_crc      = 0,
    .white_sel           = 0x06,
    .white_seed          = 0x1FF,
    .white_obit          = 0x07,
    .crc_len             = OM24G_CRC_2BYTE,
    .crc_en              = 1,
    .crc_mode            = 0,
    .crc_poly            = 0x1021, //0x07,
    .crc_init            = 0x1D0F, //0xFF,
    .crc_skip_sync       = 0,
    .crc_skip_len        = 1,
    .crc_skip_addr       = 1,
    .modulation_mode     = OM24G_MODULATION_GFSK,
    .detect_mode         = OM24G_SOFT_DETECTION, //OM24G_DPLL_DETECTION, //
};
#endif
om24g_config_t om24g_config_noack = {
    .tx_data             = om24g_tx_payload,
    .rx_data             = om24g_rx_payload,
    .packet_struct_sel   = OM24G_STRUCTURE_B,
    .preamble            = 0xaa,
    .preamble_len        = 0x01,
    .sync_word_sel       = OM24G_SYNCWORD0,
    .sync_word0          = 0x52567853,//0x52564656,//0xA1357BDA,
    .sync_word1          = 0x52567853,
    .tx_addr             = 0xaa,
    .rx_addr             = 0xaa,
    .addr_chk            = 0x00,
    .hdr_bits            = 0x08,
    .static_len          = 0x90,
    .addr1_bits          = 0x00,
    .len_bits            = 0x08,
    .addr1_pos           = 0x00,
    .len_pos             = 0x00,
    .endian              = OM24G_ENDIAN_LSB,
    .freq                = 2471,
    .data_rate           = OM24G_RATE_2M,
    .ack_en              = 0,
    .dpl_en              = 1,
    .white_en            = 0,
    .white_skip_hdr      = 0,
    .white_skip_addr     = 0,
    .white_skip_crc      = 0,
    .white_sel           = 0x06,
    .white_seed          = 0x1ff,
    .white_obit          = 0x07,
    .crc_len             = OM24G_CRC_2BYTE,
    .crc_en              = 1,
    .crc_mode            = 0,
    .crc_poly            = 0x1021, //0x07,
    .crc_init            = 0x1D0F, //0xFF,
    .crc_skip_sync       = 0,
    .crc_skip_len        = 0,
    .crc_skip_addr       = 0,
    .modulation_mode     = OM24G_MODULATION_GFSK,
    .detect_mode         = OM24G_SOFT_DETECTION, //1/2M: OM24G_SOFT_DETECTION, 100/500K: OM24G_HARD_DETECTION,
};
om24g_config_t om24g_config = {
    .tx_data             = om24g_tx_payload,
    .rx_data             = om24g_rx_payload,
    .packet_struct_sel   = OM24G_STRUCTURE_B,
    .preamble            = 0xaa,
    .preamble_len        = 0x01,
    .sync_word_sel       = OM24G_SYNCWORD0,
    .sync_word0          = 0x52567853,//0x52564656,//0xA1357BDA,
    .sync_word1          = 0x52567853,
    .tx_addr             = 0xaa,
    .rx_addr             = 0xaa,
    .addr_chk            = 0x00,
    .hdr_bits            = 0x08,
    .static_len          = 0x90,
    .addr1_bits          = 0x00,
    .len_bits            = 0x08,
    .addr1_pos           = 0x00,
    .len_pos             = 0x00,
    .endian              = OM24G_ENDIAN_LSB,
    .freq                = 2471,
    .data_rate           = OM24G_RATE_2M,
    .ack_en              = 0,
    .dpl_en              = 1,
    .white_en            = 0,
    .white_skip_hdr      = 0,
    .white_skip_addr     = 0,
    .white_skip_crc      = 0,
    .white_sel           = 0x06,
    .white_seed          = 0x1ff,
    .white_obit          = 0x07,
    .crc_len             = OM24G_CRC_2BYTE,
    .crc_en              = 1,
    .crc_mode            = 0,
    .crc_poly            = 0x1021, //0x07,
    .crc_init            = 0x1D0F, //0xFF,
    .crc_skip_sync       = 0,
    .crc_skip_len        = 0,
    .crc_skip_addr       = 0,
    .modulation_mode     = OM24G_MODULATION_GFSK,
    .detect_mode         = OM24G_SOFT_DETECTION, //1/2M: OM24G_SOFT_DETECTION, 100/500K: OM24G_HARD_DETECTION,
};

// Compatible with SILICONLAB
#elif (RTE_OM24G_RF_MODE == 2)
om24g_config_t om24g_config_b = {
    .tx_data             = om24g_tx_payload,
    .rx_data             = om24g_rx_payload,
    .packet_struct_sel   = OM24G_STRUCTURE_B,
    .preamble            = 0xAA,
    .preamble_len        = 0x04,
    .sync_word_sel       = OM24G_SYNCWORD0,
    .sync_word0          = 0x91D391D3,
    .sync_word1          = 0x52567853,
    .tx_addr             = 0x00,
    .rx_addr             = 0x00,
    .addr_chk            = 0x00,
    .hdr_bits            = 0x08,
    .static_len          = 0x100,
    .addr1_bits          = 0x00,
    .len_bits            = 0x08,
    .addr1_pos           = 0x00,
    .len_pos             = 0x00,
    .endian              = OM24G_ENDIAN_MSB,
    .freq                = 2432,
    .data_rate           = OM24G_RATE_250K,
    .ack_en              = 0,
    .dpl_en              = 1,
    .white_en            = 0,
    .white_skip_hdr      = 0,
    .white_skip_addr     = 1,
    .white_skip_crc      = 0,
    .white_sel           = 0x06,
    .white_seed          = 0x80,
    .white_obit          = 0x07,
    .crc_len             = OM24G_CRC_2BYTE,
    .crc_en              = 1,
    .crc_mode            = 0,
    .crc_poly            = 0x8005, //0x07,
    .crc_init            = 0xFFFF, //0xFF,
    .crc_skip_sync       = 1,
    .crc_skip_len        = 0,
    .crc_skip_addr       = 0,
    .modulation_mode     = OM24G_MODULATION_GFSK,
    .detect_mode         = OM24G_HARD_DETECTION,
};
om24g_config_t om24g_config_noack = {
    .tx_data             = om24g_tx_payload,
    .rx_data             = om24g_rx_payload,
    .packet_struct_sel   = OM24G_STRUCTURE_B,
    .preamble            = 0xAA,
    .preamble_len        = 0x04,
    .sync_word_sel       = OM24G_SYNCWORD0,
    .sync_word0          = 0x91D391D3,
    .sync_word1          = 0x52567853,
    .tx_addr             = 0x00,
    .rx_addr             = 0x00,
    .addr_chk            = 0x00,
    .hdr_bits            = 0x08,
    .static_len          = 0x100,
    .addr1_bits          = 0x00,
    .len_bits            = 0x08,
    .addr1_pos           = 0x00,
    .len_pos             = 0x00,
    .endian              = OM24G_ENDIAN_MSB,
    .freq                = 2432,
    .data_rate           = OM24G_RATE_250K,
    .ack_en              = 0,
    .dpl_en              = 1,
    .white_en            = 0,
    .white_skip_hdr      = 0,
    .white_skip_addr     = 1,
    .white_skip_crc      = 0,
    .white_sel           = 0x06,
    .white_seed          = 0x80,
    .white_obit          = 0x07,
    .crc_len             = OM24G_CRC_2BYTE,
    .crc_en              = 1,
    .crc_mode            = 0,
    .crc_poly            = 0x8005, //0x07,
    .crc_init            = 0xFFFF, //0xFF,
    .crc_skip_sync       = 1,
    .crc_skip_len        = 0,
    .crc_skip_addr       = 0,
    .modulation_mode     = OM24G_MODULATION_GFSK,
    .detect_mode         = OM24G_HARD_DETECTION,
};
om24g_config_t om24g_config = {
    .tx_data             = om24g_tx_payload,
    .rx_data             = om24g_rx_payload,
    .packet_struct_sel   = OM24G_STRUCTURE_B,
    .preamble            = 0xAA,
    .preamble_len        = 0x04,
    .sync_word_sel       = OM24G_SYNCWORD0,
    .sync_word0          = 0x91D391D3,
    .sync_word1          = 0x52567853,
    .tx_addr             = 0x00,
    .rx_addr             = 0x00,
    .addr_chk            = 0x00,
    .hdr_bits            = 0x08,
    .static_len          = 0x100,
    .addr1_bits          = 0x00,
    .len_bits            = 0x08,
    .addr1_pos           = 0x00,
    .len_pos             = 0x00,
    .endian              = OM24G_ENDIAN_MSB,
    .freq                = 2432,
    .data_rate           = OM24G_RATE_250K,
    .ack_en              = 0,
    .dpl_en              = 1,
    .white_en            = 0,
    .white_skip_hdr      = 0,
    .white_skip_addr     = 1,
    .white_skip_crc      = 0,
    .white_sel           = 0x06,
    .white_seed          = 0x80,
    .white_obit          = 0x07,
    .crc_len             = OM24G_CRC_2BYTE,
    .crc_en              = 1,
    .crc_mode            = 0,
    .crc_poly            = 0x8005, //0x07,
    .crc_init            = 0xFFFF, //0xFF,
    .crc_skip_sync       = 1,
    .crc_skip_len        = 0,
    .crc_skip_addr       = 0,
    .modulation_mode     = OM24G_MODULATION_GFSK,
    .detect_mode         = OM24G_HARD_DETECTION,
};

// Compatible with NORDIC
#elif (RTE_OM24G_RF_MODE == 3)
om24g_config_t om24g_config = {
    .tx_data             = om24g_tx_payload,
    .rx_data             = om24g_rx_payload,
    .packet_struct_sel   = OM24G_STRUCTURE_A,
    .preamble            = 0xAA,
    .preamble_len        = 0x01,
    .sync_word_sel       = OM24G_SYNCWORD0,
    .sync_word0          = 0x45E712E7,// 0xedd47662,
    .sync_word1          = 0x12345678,
    .tx_addr             = 0xAB,
    .rx_addr             = 0xAB,
    .addr_chk            = 0x01,
    .static_len          = 0x40,
    .hdr_bits            = 0x09,
    .addr1_bits          = 0x08,
    .addr1_loc           = 0x02,
    .len_bits            = 0x06,
    .addr1_pos           = 0x00,
    .len_pos             = 0x03,
    .endian              = OM24G_ENDIAN_MSB,
    .freq                = 2402,
    .data_rate           = OM24G_RATE_2M,
    .ack_en              = 1,
    .dpl_en              = 1,
    .white_en            = 1,
    .white_skip_hdr      = 0,
    .white_skip_addr     = 1,
    .white_skip_crc      = 0,
    .white_sel           = 0x02,
    .white_seed          = 0x08,
    .white_obit          = 0x02,
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

// 6621C A帧 no ack
om24g_config_t om24g_config_noack = {
    .tx_data             = om24g_tx_payload,
    .rx_data             = om24g_rx_payload,
    .packet_struct_sel   = OM24G_STRUCTURE_A,
    .preamble            = 0xAA,
    .preamble_len        = 0x01,
    .sync_word_sel       = OM24G_SYNCWORD0,
    .sync_word0          = 0x45E712E7,
    .sync_word1          = 0xedd47662,
    .tx_addr             = 0xAB,
    .rx_addr             = 0xAB,
    .addr_chk            = 0x01,
    .static_len          = 0x20,
    .hdr_bits            = 0x08,
    .addr1_bits          = 0x00,
    .len_bits            = 0x08,
    .addr1_pos           = 0x00,
    .len_pos             = 0x00,
    .endian              = OM24G_ENDIAN_MSB,
    .freq                = 2402,// 0x21,
    .data_rate           = OM24G_RATE_1M,
    .ack_en              = 0,
    .dpl_en              = 0,
    .white_en            = 0,
    .white_skip_hdr      = 1,
    .white_skip_addr     = 1,
    .white_skip_crc      = 0,
    .white_sel           = 0x02,
    .white_seed          = 0x08,//80,
    .white_obit          = 0x02,//07,
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
om24g_config_t om24g_config_b = {
    .tx_data             = om24g_tx_payload,
    .rx_data             = om24g_rx_payload,
    .packet_struct_sel   = OM24G_STRUCTURE_A,
    .preamble            = 0xAA,
    .preamble_len        = 0x01,
    .sync_word_sel       = OM24G_SYNCWORD0,
    .sync_word0          = 0x45E712E7,// 0xedd47662,
    .sync_word1          = 0x12345678,
    .tx_addr             = 0xAB,
    .rx_addr             = 0xAB,
    .addr_chk            = 0x01,
    .static_len          = 0x40,
    .hdr_bits            = 0x09,
    .addr1_bits          = 0x08,
    .addr1_loc           = 0x02,
    .len_bits            = 0x06,
    .addr1_pos           = 0x00,
    .len_pos             = 0x03,
    .endian              = OM24G_ENDIAN_MSB,
    .freq                = 2402,
    .data_rate           = OM24G_RATE_250K,
    .ack_en              = 1,
    .dpl_en              = 1,
    .white_en            = 1,
    .white_skip_hdr      = 0,
    .white_skip_addr     = 1,
    .white_skip_crc      = 0,
    .white_sel           = 0x02,
    .white_seed          = 0x08,
    .white_obit          = 0x02,
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

#else // OM24G_STRUCTURE_A: Compatible with 662x series
om24g_config_t om24g_config = {
    .tx_data             = om24g_tx_payload,
    .rx_data             = om24g_rx_payload,
    .packet_struct_sel   = OM24G_STRUCTURE_A,
    .preamble            = 0xaa,
    .preamble_len        = 0x04,
    .sync_word_sel       = OM24G_SYNCWORD0,
    .sync_word0          = 0x9658AD2B,// 0xedd47662,
    .sync_word1          = 0x12345678,
    .tx_addr             = 0x36,
    .rx_addr             = 0x36,
    .addr_chk            = 0x01,
    .static_len          = 0x80,
    .hdr_bits            = 0x08,
    .addr1_bits          = 0x00,
    .len_bits            = 0x08,
    .addr1_pos           = 0x00,
    .len_pos             = 0x00,
    .endian              = OM24G_ENDIAN_MSB,
    .freq                = 2480,//0x21,
    .data_rate           = OM24G_RATE_2M,
    .ack_en              = 1,
    .dpl_en              = 1,
    .white_en            = 1,
    .white_skip_hdr      = 1,
    .white_skip_addr     = 1,
    .white_skip_crc      = 1,
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
    .detect_mode         = OM24G_SOFT_DETECTION, //OM24G_HARD_DETECTION, //
};

// 6621C A帧 no ack
om24g_config_t om24g_config_noack = {
    .tx_data             = om24g_tx_payload,
    .rx_data             = om24g_rx_payload,
    .packet_struct_sel   = OM24G_STRUCTURE_A,
    .preamble            = 0xaa,
    .preamble_len        = 0x04,
    .sync_word_sel       = OM24G_SYNCWORD0,
    .sync_word0          = 0x9658AD2B,
    .sync_word1          = 0xedd47662,
    .tx_addr             = 0x36,
    .rx_addr             = 0x36,
    .addr_chk            = 0x01,
    .static_len          = 0x20,
    .hdr_bits            = 0x00,
    .addr1_bits          = 0x00,
    .len_bits            = 0x00,
    .addr1_pos           = 0x00,
    .len_pos             = 0x00,
    .endian              = OM24G_ENDIAN_MSB,
    .freq                = 2442,// 0x21,
    .data_rate           = OM24G_RATE_1M,
    .ack_en              = 0,
    .dpl_en              = 0,
    .white_en            = 1,
    .white_skip_hdr      = 1,
    .white_skip_addr     = 1,
    .white_skip_crc      = 1,
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
    .white_en            = 1,
    .white_skip_hdr      = 0,
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

#endif

om24g_config_t om24g_config_ble = {
    .tx_data             = om24g_tx_payload,
    .rx_data             = om24g_rx_payload,
    .packet_struct_sel   = OM24G_STRUCTURE_B,
    .preamble            = 0xaa,
    .preamble_len        = 0x01,
    .sync_word_sel       = OM24G_SYNCWORD0,
    .sync_word0          = 0x8E89BED6,
    .sync_word1          = 0x12345678,
    .tx_addr             = 0x02,
    .rx_addr             = 0x02,
    .addr_chk            = 0x00,
    .static_len          = 0x25,
    .hdr_bits            = 0x10,
    .addr1_bits          = 0x08,
    .len_bits            = 0x08,
    .addr1_pos           = 0x00,
    .addr1_loc           = 0x00,
    .len_pos             = 0x08,
    .endian              = OM24G_ENDIAN_LSB,
    .freq                = 2426,
    .data_rate           = OM24G_RATE_1M,
    .ack_en              = 0,
    .dpl_en              = 1,
    .white_en            = 1,
    .white_skip_hdr      = 0,
    .white_skip_addr     = 0,
    .white_skip_crc      = 0,
    .white_sel           = 0x02,
    .white_seed          = 0x1B,
    .white_obit          = 0x02,
    .crc_len             = OM24G_CRC_3BYTE,
    .crc_en              = 1,
    .crc_mode            = 0,
    .crc_poly            = 0x065B, // 0x07,
    .crc_init            = 0x555555, // 0xFF,
    .crc_skip_sync       = 1,
    .crc_skip_len        = 0,
    .crc_skip_addr       = 0,
    .modulation_mode     = OM24G_MODULATION_GFSK,
    .detect_mode         = OM24G_SOFT_DETECTION,
};

/// log debug array with show more
#define OM_LOG_DEBUG_ARRAY_EX(note, array, len)    do{OM_LOG(OM_LOG_DEBUG, "%s: \r\n",note); OM_LOG_HEXDUMP(OM_LOG_DEBUG, array, len, 16); OM_LOG(OM_LOG_DEBUG, "[%dbytes]\n", len);}while(0)
#if TX_ROLE
static void om24g_write_it(void);
#endif
static void timer1_callback_timestamp(void *om_reg, drv_event_t drv_event, void *param0, void *param1);

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
            // OM24G_CE_LOW();
            // pm_sleep_allow(PM_ID_24G);
            // drv_om24g_control(OM24G_CONTROL_CLK_DISABLE, NULL);
            // drv_pmu_timer_trig_set(PMU_TIMER_TRIG_VAL0, (drv_pmu_timer_cnt_get() + PMU_TIMER_MS2TICK(500)));
            #endif
            osSemaphoreRelease(xSemOm24g);
            break;
        case DRV_EVENT_OM24G_MAX_RT:   // max retry
            break;
        case DRV_EVENT_OM24G_RX_OVERLAY:
            OM_LOG(OM_LOG_DEBUG, "overlay\r\n");
            break;
        case DRV_EVENT_OM24G_INT_TIMER0:
            //OM_LOG(OM_LOG_DEBUG, "om24g int timer\r\n");
            pm_sleep_prevent(PM_ID_24G);
            drv_gpio_toggle(OM_GPIO0, GPIO_MASK(7));
            break;
        case DRV_EVENT_OM24G_INT_TIMER1:
            pm_sleep_prevent(PM_ID_24G);
            break;
        case DRV_EVENT_COMMON_TRANSMIT_COMPLETED:
            drv_om24g_control(OM24G_CONTROL_CLK_DISABLE, NULL);
            drv_pmu_timer_trig_set(PMU_TIMER_TRIG_VAL0, (drv_pmu_timer_cnt_get() + PMU_TIMER_MS2TICK(200)));
            tx_count++;
            drv_gpio_toggle(OM_GPIO0, GPIO_MASK(8));
            // OM_LOG(OM_LOG_DEBUG, "tx_cnt: %d\r\n", tx_count);
            pm_sleep_allow(PM_ID_24G);
            if(tx_count == 100) {
                tx_count = 0;
                drv_pmu_timer_control(PMU_TIMER_TRIG_VAL0, PMU_TIMER_CONTROL_ENABLE, (void *)0);
                drv_om24g_control(OM24G_CONTROL_CLK_DISABLE, NULL);
            }
            osSemaphoreRelease(xSemOm24g);
            break;
        default:
            break;
    }
}

static void om24g_callback_ack_mode(void *om_reg, drv_event_t drv_event, void *buff, void *num)
{
    uint16_t payload_lenth = 0;
    bool error_flag = false;

    switch (drv_event) {
        case DRV_EVENT_COMMON_RECEIVE_COMPLETED:
            om24g_rx_flag = true;
            //drv_gpio_toggle(OM_GPIO, GPIO_MASK(8));
            if(TX_ROLE) {
                // pm_sleep_allow(PM_ID_24G);
            } else {
                static uint8_t ack_num = 1;
                for (uint8_t i = 0; i < 32; i++) {
                    om24g_tx_payload[i] = i+1;
                }
                ack_num++;
                drv_om24g_write_ack(ack_num);
                //drv_om24g_write_ack(32);
                //OM_LOG_DEBUG_ARRAY_EX("ack_pkt", om24g_tx_payload, ack_num);
                if(ack_num > 31) {
                    ack_num = 0;
                }
                #if ENABLE_SLEEP_MODE
                OM24G_CE_LOW();
                pm_sleep_allow(PM_ID_24G);
                drv_om24g_control(OM24G_CONTROL_CLK_DISABLE, NULL);
                drv_pmu_timer_control(PMU_TIMER_TRIG_VAL0, PMU_TIMER_CONTROL_SET_TIMER_INCR, (void *)(drv_pmu_timer_cnt_get() + PMU_TIMER_MS2TICK(900))); // PMU_TIMER_US2TICK
                #endif
            }
            payload_lenth = (uint32_t)num;
            for (int i = 0; i < payload_lenth; i++) {
                if ((1 + i) != *((uint8_t *)buff + i)) {
                    error_flag = true;
                }
            }
            if (error_flag) {
                error_count++;
                error_flag = false;
                OM_LOG_DEBUG_ARRAY_EX("err Pkt", buff, payload_lenth);
            } else {
                right_count++;
                OM_LOG_DEBUG_ARRAY_EX("Pkt", buff, payload_lenth);
            }
            OM_LOG(OM_LOG_DEBUG, "E:%d ", error_count);
            OM_LOG(OM_LOG_DEBUG, "R:%d \r\n", right_count);
            OM_LOG(OM_LOG_DEBUG, "MAX_RT:%d\r\n", max_rty_count);
            break;
        case DRV_EVENT_OM24G_MAX_RT:   // max retry
            max_rty_count++;
            OM_LOG(OM_LOG_DEBUG, "Max_Retry\r\n");
            // pm_sleep_allow(PM_ID_24G);
            break;
        case DRV_EVENT_OM24G_RX_TM:
            OM_LOG(OM_LOG_DEBUG, "rx timeout\r\n");
            break;
        case DRV_EVENT_COMMON_TRANSMIT_COMPLETED:
            #if TX_ROLE
            tx_count++;
            OM_LOG(OM_LOG_DEBUG, "tx_cnt: %d\r\n", tx_count);
            // if(tx_count == 500) {
            //     tx_count = 0;
            //     drv_pmu_timer_control(PMU_TIMER_TRIG_VAL0, PMU_TIMER_CONTROL_DISABLE, NULL);
            // }
            #endif
            break;
        default:
            break;
    }
}

static void om24g_callback_rx_to_tx(void *om_reg, drv_event_t drv_event, void *buff, void *num)
{
    switch (drv_event) {
        case DRV_EVENT_COMMON_RECEIVE_COMPLETED:
            om24g_rx_flag = true;
            right_count++;
            OM_LOG_DEBUG_ARRAY_EX("Pkt", buff, (uint32_t)num);
            OM_LOG(OM_LOG_DEBUG, "R:%d \r\n", right_count);
            OM24G_CE_LOW();
            #if !TX_ROLE
            drv_om24g_write_int(om24g_tx_payload, 32);
            #endif
            break;
        case DRV_EVENT_COMMON_RX_OVERFLOW:   // max retry
            break;
        case DRV_EVENT_COMMON_TRANSMIT_COMPLETED:
            #if TX_ROLE
            drv_pmu_timer_control(PMU_TIMER_TRIG_VAL0, PMU_TIMER_CONTROL_SET_TIMER_VAL, (void *)(drv_pmu_timer_cnt_get() + PMU_TIMER_MS2TICK(500)));
            #endif
            drv_om24g_read_int(om24g_rx_payload, 166);
            tx_count++;
            if(tx_count == 100) {
                tx_count = 0;
                right_count = 0;
                drv_pmu_timer_control(PMU_TIMER_TRIG_VAL0, PMU_TIMER_CONTROL_ENABLE, 0);
            }
            //OM_LOG(OM_LOG_DEBUG, "tx_cnt: %d\r\n", tx_count);
            break;
        default:
            break;
    }
}

static void om24g_callback_timestamp(void *om_reg, drv_event_t drv_event, void *buff, void *num)
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
            if (error_flag) {
                error_count++;
                error_flag = false;
                OM_LOG_DEBUG_ARRAY_EX("err Pkt", buff, payload_lenth);
            } else {
                right_count++;
                OM_LOG_DEBUG_ARRAY_EX("Pkt", buff, payload_lenth);
            }
            OM_LOG(OM_LOG_DEBUG, "E:%d ", error_count);
            OM_LOG(OM_LOG_DEBUG, "R:%d \r\n", right_count);

            #if ENABLE_SLEEP_MODE
            // OM24G_CE_LOW();
            // pm_sleep_allow(PM_ID_24G);
            // drv_om24g_control(OM24G_CONTROL_CLK_DISABLE, NULL);
            // drv_pmu_timer_trig_set(PMU_TIMER_TRIG_VAL0, (drv_pmu_timer_cnt_get() + PMU_TIMER_MS2TICK(500)));
            #endif

            break;
        case DRV_EVENT_OM24G_MAX_RT:   // max retry
            break;
        case DRV_EVENT_OM24G_RX_OVERLAY:
            OM_LOG(OM_LOG_DEBUG, "overlay\r\n");
            break;
        case DRV_EVENT_COMMON_TRANSMIT_COMPLETED:
            // drv_om24g_control(OM24G_CONTROL_CLK_DISABLE, NULL);
            // drv_pmu_timer_trig_set(PMU_TIMER_TRIG_VAL0, (drv_pmu_timer_cnt_get() + PMU_TIMER_MS2TICK(200)));
            tx_count++;
            drv_gpio_toggle(OM_GPIO0, GPIO_MASK(8));
            OM_LOG(OM_LOG_DEBUG, "tx_cnt: %d\r\n", tx_count);
            // pm_sleep_allow(PM_ID_24G);
            if(tx_count == 100) {
                tx_count = 0;
                drv_pmu_timer_control(PMU_TIMER_TRIG_VAL0, PMU_TIMER_CONTROL_ENABLE, (void *)0);
                drv_om24g_control(OM24G_CONTROL_CLK_DISABLE, NULL);
            }
            break;
        default:
            break;
    }
}

#if TX_ROLE
static void om24g_write_it(void)
{
    static uint8_t j = 0;

    j++;
    if(j == 250) {
        j = 0;
    }
    for (uint16_t i = 0; i < 33; i++) {
            om24g_tx_payload[i] = j;
    }
    //om24g_control(OM24G_CONTROL_DUMP_RF_REGISTER, NULL);
    drv_om24g_write_int(om24g_tx_payload, 32);
}

static void om24g_write_it_ack_mode(void)
{
    static uint16_t j = 1;
    static uint16_t tx_cnt = 0;

    for (uint16_t i = 0; i < 100; i++) {
        om24g_tx_payload[i] = i+1;
    }
    while(1) {
        drv_om24g_write_int(om24g_tx_payload, j);
        j++;
        if (j > 32) {
            j = 1;
        }
        //OM_LOG(OM_LOG_DEBUG, "tx_cnt: %d\r\n", tx_count);
        osDelay(200);
        tx_cnt = tx_count;
        if((tx_cnt + max_rty_count) == 501) {
            tx_count = 0;
            tx_cnt = 0;
            max_rty_count = 0;
            error_count = 0;
            right_count = 0;
            break;
        }
    }
    drv_om24g_control(OM24G_CONTROL_CLK_DISABLE, NULL);
}

static void om24g_tx_to_rx_poll_mode(void)
{
    uint16_t total_count = 300;
    static uint8_t j = 0;
    bool om24g_state = false;
    uint16_t payload_len = 0;

    drv_om24g_init(&om24g_config_b);
    NVIC_DisableIRQ(OM24G_RF_IRQn);
    drv_om24g_dump_rf_register();
    while (total_count--) {
        j++;
        if(j == 250) {
            j = 0;
        }
        for (uint8_t i = 0; i < 32; i++) {
            om24g_tx_payload[i] = j;
        }
        om24g_state = drv_om24g_write(om24g_tx_payload, 32);
        if (om24g_state == true) {
            tx_count++;
            //OM_LOG(OM_LOG_DEBUG, "tx_cnt: %d\r\n", tx_count);
        }

        payload_len = drv_om24g_read(om24g_rx_payload, 4000);
        if(!payload_len) {
            OM_LOG(OM_LOG_DEBUG, "rx timeout\r\n");
        } else {
            right_count++;
        }
        OM_LOG_DEBUG_ARRAY_EX("Pkt", om24g_rx_payload, payload_len);
        DRV_DELAY_MS(100);
    }
    OM_LOG(OM_LOG_DEBUG, "R: %d\r\n", right_count);
    right_count = 0;
    while(1);
}

static void om24g_tx_to_rx_int_mode(void)
{
    for (uint8_t i = 0; i < 32; i++) {
        om24g_tx_payload[i] = i+1;
    }
    drv_om24g_init(&om24g_config_b);
    drv_om24g_register_event_callback(om24g_callback_rx_to_tx);
    drv_om24g_dump_rf_register();
    om24g_write_it();
}

/*Packet structure B transmits packets in interrupt mode and can communicate with function om24g_read_int_structure_b(void). */
static void om24g_write_int_structure_b(void)
{
    drv_om24g_init(&om24g_config_b);
    //drv_om24g_set_rf_parameters(OM24G_RATE_500K, 2471, 0.5);
    drv_om24g_register_event_callback(om24g_callback);
    drv_om24g_dump_rf_register();
    om24g_write_it();
}

static void om24g_structure_b_static_len_tx(void)
{
    om24g_config_t om24g_config_static = om24g_config_b;
    om24g_config_static.static_len          = 0x20;
    om24g_config_static.hdr_bits            = 0x00;
    om24g_config_static.dpl_en              = 0x00;

    drv_om24g_init(&om24g_config_static);
    drv_om24g_register_event_callback(om24g_callback);
    drv_om24g_dump_rf_register();
    om24g_write_it();
}

static void om24g_write_txaddr_check_b(void)
{
    om24g_config_t om24g_config_addr = om24g_config_b;
    om24g_config_addr.data_rate           = OM24G_RATE_500K;
    om24g_config_addr.dpl_en              = 1;
    om24g_config_addr.tx_addr             = 0xbb;
    om24g_config_addr.rx_addr             = 0xbb;
    om24g_config_addr.addr_chk            = 1;
    om24g_config_addr.hdr_bits            = 0x10;
    om24g_config_addr.addr1_bits          = 0x08;
    om24g_config_addr.len_bits            = 0x08;
    om24g_config_addr.addr1_pos           = 0x08;
    om24g_config_addr.len_pos             = 0x00;
    om24g_config_addr.endian              = OM24G_ENDIAN_MSB;
    om24g_config_addr.crc_len             = OM24G_CRC_3BYTE;
    om24g_config_addr.crc_poly            = 0x065B;
    om24g_config_addr.crc_init            = 0x555555;
    om24g_config_addr.crc_skip_sync       = 1;
    om24g_config_addr.crc_skip_len        = 0;
    om24g_config_addr.crc_skip_addr       = 0;
    om24g_config_addr.modulation_mode     = OM24G_MODULATION_FSK;

    drv_om24g_init(&om24g_config_addr);
    drv_om24g_register_event_callback(om24g_callback);
    drv_om24g_dump_rf_register();
    om24g_write_it();
}

static void om24g_ble_mode_tx(void)
{
    drv_om24g_init(&om24g_config_ble);
    drv_om24g_register_event_callback(om24g_callback);
    drv_om24g_dump_rf_register();
    om24g_write_it();
}

static void om24g_write_structure_a(void)
{
    uint16_t total_count = 1000;
    static uint8_t j = 0;
    bool om24g_state = false;

    drv_om24g_init(&om24g_config_noack);
    NVIC_DisableIRQ(OM24G_RF_IRQn);
    drv_om24g_dump_rf_register();
    while (total_count--) {
        j++;
        if(j == 250) {
            j = 0;
        }
        for (uint8_t i = 0; i < 32; i++) {
            om24g_tx_payload[i] = j;
        }
        om24g_state = drv_om24g_write(om24g_tx_payload, 32);
        if (om24g_state == true) {
            tx_count++;
            OM_LOG(OM_LOG_DEBUG, "tx_cnt: %d\r\n", tx_count);
        }
        DRV_DELAY_MS(50);
    }
    tx_count = 0;
    while(1);
}

/*Packet structure A transmits packets in interrupt mode and can communicate with function om24g_read_int_structure_a(void). */
static void om24g_write_int_structure_a(void)
{
    drv_om24g_init(&om24g_config_noack);
    drv_om24g_register_event_callback(om24g_callback);
    drv_om24g_dump_rf_register();
    om24g_write_it();
}

/*Packet structure A transmits packets in polling ACK mode and can communicate with function om24g_read_ack_mode_structure_a(void). */
static void om24g_write_ack_mode_structure_a(void)
{
    uint8_t j = 0;
    uint16_t payload_len = 0;
    uint16_t total_count = 500;
    bool om24g_state = false;
    bool error_flag = false;

    for (uint8_t i = 0; i < 32; i++) {
        om24g_tx_payload[i] = i + 1;
    }
    drv_om24g_init(&om24g_config);
    NVIC_DisableIRQ(OM24G_RF_IRQn);
    drv_om24g_dump_rf_register();
    while (total_count--) {
        j++;
        if (j > 32) {
            j = 1;
        }
        om24g_state = drv_om24g_write(om24g_tx_payload, j);
        if (om24g_state) {
            payload_len = drv_om24g_read_ack();
            for (int i = 0; i < payload_len; i++) {
                if ((i + 1) != om24g_rx_payload[i]) {
                    error_flag = true;
                }
            }
            if (error_flag) {
                error_count++;
                error_flag = false;
                OM_LOG_DEBUG_ARRAY_EX("err Pkt", om24g_rx_payload, payload_len);
            } else {
                right_count++;
                OM_LOG_DEBUG_ARRAY_EX("Pkt", om24g_rx_payload, payload_len);
            }
        } else {
            max_rty_count++;
            OM_LOG(OM_LOG_DEBUG, "Max_Retry\r\n");
        }
        drv_dwt_delay_ms(200);
    }
    OM_LOG(OM_LOG_DEBUG, "E:%d ", error_count);
    OM_LOG(OM_LOG_DEBUG, "R:%d \r\n", right_count);
    OM_LOG(OM_LOG_DEBUG, "MAX_RT:%d\r\n", max_rty_count);
    max_rty_count = 0;
    right_count = 0;
    while(1);
}

/*Packet structure A transmits packets in interrupt ACK mode and can communicate with function om24g_read_ack_mode_structure_a(void). */
static void om24g_write_int_ack_mode_structure_a(void)
{
    drv_om24g_init(&om24g_config);
    drv_om24g_register_event_callback(om24g_callback_ack_mode);
    drv_om24g_dump_rf_register();
    om24g_write_it_ack_mode();
}

static void om24g_hardware_timer_callback(void *om_reg, drv_event_t drv_event, void *buff, void *num)
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
            // OM24G_CE_LOW();
            // pm_sleep_allow(PM_ID_24G);
            // drv_om24g_control(OM24G_CONTROL_CLK_DISABLE, NULL);
            // drv_pmu_timer_trig_set(PMU_TIMER_TRIG_VAL0, (drv_pmu_timer_cnt_get() + PMU_TIMER_MS2TICK(500)));
            #endif

            break;
        case DRV_EVENT_OM24G_MAX_RT:   // max retry
            break;
        case DRV_EVENT_OM24G_RX_OVERLAY:
            OM_LOG(OM_LOG_DEBUG, "overlay\r\n");
            break;
        case DRV_EVENT_OM24G_INT_TIMER0:
            REGW(&OM_PMU->TMR_2G4_TRIG_CTL, MASK_1REG(PMU_TMR_2G4_TRIG_CTL_TMR_2G4_NATIVE_INT_EN, 1));
            drv_gpio_toggle(OM_GPIO0, GPIO_MASK(9));
            break;
        case DRV_EVENT_OM24G_INT_TIMER1:
            //OM_LOG(OM_LOG_DEBUG, "om24g int timer\r\n");
            // TIMER0
            // OM_24G->TMR0_NXT_32K_TP = OM_24G->TMR_NATIVE_CNT_32K + 16384; // 512ms
            // OM_24G->TMR0_NXT_8M_TP = OM_24G->TMR_NATIVE_CNT_8M + 0;
            // OM_PMU->TMR_2G4_TIM_WAKEUP_NATIVE = OM_24G->TMR0_NXT_32K_TP - 100;
            // OM_PMU->TMR_2G4_TIM_WAKEUP_SKIP = 0x20;

            // TIMER1. Placed in function drv_om24g_isr for more accurate timing.
            OM_24G->TMR1_NXT_32K_TP = OM_24G->TMR_NATIVE_CNT_32K + 16384; // 16384 = 512ms
            OM_24G->TMR1_NXT_8M_TP = OM_24G->TMR_NATIVE_CNT_8M + 0;
            OM_PMU->TMR_2G4_TIM_WAKEUP_NATIVE = OM_24G->TMR1_NXT_32K_TP - 100;
            OM_PMU->TMR_2G4_TIM_WAKEUP_SKIP = 0x20;
            REGW(&OM_PMU->TMR_2G4_TRIG_CTL, MASK_1REG(PMU_TMR_2G4_TRIG_CTL_TMR_2G4_NATIVE_INT_EN, 1));
            drv_gpio_toggle(OM_GPIO0, GPIO_MASK(9));
            break;
        case DRV_EVENT_COMMON_TRANSMIT_COMPLETED:
            // drv_om24g_control(OM24G_CONTROL_CLK_DISABLE, NULL);
            tx_count++;
            drv_gpio_toggle(OM_GPIO0, GPIO_MASK(8));
            om24g_write_it();
            om24g_tx_flag = true;
            // OM_LOG(OM_LOG_DEBUG, "OM_PMU->TMR_2G4_TIM_WAKEUP_NATIVE: %x \r\n", OM_PMU->TMR_2G4_TIM_WAKEUP_NATIVE);
            // OM_LOG(OM_LOG_DEBUG, "OM_PMU->TMR_2G4_TIM_WAKEUP_SKIP: %x \r\n", OM_PMU->TMR_2G4_TIM_WAKEUP_SKIP);
            // OM_LOG(OM_LOG_DEBUG, "OM_PMU->MISC_CTRL: %x \r\n", OM_PMU->MISC_CTRL);
            // OM_LOG(OM_LOG_DEBUG, "OM_PMU->TMR_2G4_TRIG_CTL: %x \r\n", OM_PMU->TMR_2G4_TRIG_CTL);
            // OM_LOG(OM_LOG_DEBUG, "OM_PMU->OSC_INT_CTRL: %x \r\n", OM_PMU->OSC_INT_CTRL);

            // OM_LOG(OM_LOG_DEBUG, "tx_cnt: %d\r\n", tx_count);
            if(tx_count == 500) {
                tx_count = 0;
                REGW(&OM_PMU->TMR_2G4_TRIG_CTL, MASK_4REG(PMU_TMR_2G4_TRIG_CTL_TMR_2G4_NATIVE_INT_EN, 0, PMU_TMR_2G4_TRIG_CTL_TMR1_2G4_TRIG_FUNC_EN, 0, PMU_TMR_2G4_TRIG_CTL_TMR1_2G4_TRIG_INT_EN, 0, PMU_TMR_2G4_TRIG_CTL_TMR1_2G4_TRIG_MODE, 0));
                REGW(&OM_PMU->OSC_INT_CTRL, MASK_3REG(PMU_OSC_INI_CTRL_2G4_WAKEUP_INT_CLR, 1, PMU_OSC_INI_CTRL_2G4_WAKEUP_INT_MASK_PMU, 1, PMU_OSC_INI_CTRL_2G4_WAKEUP_INT_MASK_CPU, 1));
                drv_om24g_control(OM24G_CONTROL_CLK_DISABLE, NULL);
            }
            pm_sleep_allow(PM_ID_24G);
            break;
        default:
            break;
    }
}

#if 0
static void om24g_hardware_timer_no_sleep_tx(void)
{
    drv_om24g_init(&om24g_config_b);
    drv_om24g_register_event_callback(om24g_hardware_timer_callback);
    drv_om24g_dump_rf_register();
    REGW(&OM_PMU->MISC_CTRL, MASK_1REG(PMU_MISC_CTRL_BB_1_RESET, 1));
    REGW(&OM_PMU->MISC_CTRL, MASK_1REG(PMU_MISC_CTRL_BB_1_RESET, 0));

    // TIMER0 START
    // REGW(&OM_PMU->TMR_2G4_TRIG_CTL, MASK_3REG(PMU_TMR_2G4_TRIG_CTL_TMR0_2G4_TRIG_FUNC_EN, 1, PMU_TMR_2G4_TRIG_CTL_TMR0_2G4_TRIG_INT_EN, 1, PMU_TMR_2G4_TRIG_CTL_TMR0_2G4_TRIG_MODE, 0));
    // OM_24G->TMR0_NXT_32K_TP = OM_24G->TMR_NATIVE_CNT_32K + 16384;
    // OM_24G->TMR0_NXT_8M_TP = OM_24G->TMR_NATIVE_CNT_8M + 0;
    // OM_24G->TMR0_TRIG_PREFETCH = 0;

    // TIMER1 START
    REGW(&OM_PMU->TMR_2G4_TRIG_CTL, MASK_3REG(PMU_TMR_2G4_TRIG_CTL_TMR1_2G4_TRIG_FUNC_EN, 1, PMU_TMR_2G4_TRIG_CTL_TMR1_2G4_TRIG_INT_EN, 1, PMU_TMR_2G4_TRIG_CTL_TMR1_2G4_TRIG_MODE, 0));
    OM_24G->TMR1_NXT_32K_TP = OM_24G->TMR_NATIVE_CNT_32K + 16384;
    OM_24G->TMR1_NXT_8M_TP = OM_24G->TMR_NATIVE_CNT_8M + 0;
    OM_24G->TMR1_TRIG_PREFETCH = 0;

    drv_om24g_write_int(om24g_tx_payload, 32);
}
#endif

pm_status_t om24g_sleep_checker_callback(void)
{
    if(om24g_tx_flag) {
        pm_sleep_allow(PM_ID_24G);
        om24g_tx_flag = false;
        return PM_STATUS_SLEEP;
    } else {
        pm_sleep_prevent(PM_ID_24G);
        drv_gpio_toggle(OM_GPIO0, GPIO_MASK(7));
        return PM_STATUS_IDLE;
    }
}

static void om24g_hardware_timer_sleep_tx(void)
{
    drv_om24g_init(&om24g_config_b);
    drv_om24g_register_event_callback(om24g_hardware_timer_callback);
    pm_sleep_checker_callback_register(PM_CHECKER_PRIORITY_NORMAL, om24g_sleep_checker_callback);
    // drv_om24g_dump_rf_register();
    // cpm 2g4 timer clock gate enable
    REGW(&OM_CPM->MAC_24G_CFG, MASK_1REG(CPM_2P4_CFG_TIMER_GATE_EN, 0));
    // timer reset
    REGW(&OM_PMU->MISC_CTRL, MASK_1REG(PMU_MISC_CTRL_BB_1_RESET, 1));
    // timer reset start counting, Clear pmu gpio interrupt
    REGW(&OM_PMU->MISC_CTRL, MASK_2REG(PMU_MISC_CTRL_BB_1_RESET, 0, PMU_MISC_CTRL_CLR_PMU_INT, 1));

    // TIMER0 START
    // REGW(&OM_PMU->TMR_2G4_TRIG_CTL, MASK_4REG(PMU_TMR_2G4_TRIG_CTL_TMR_2G4_NATIVE_INT_EN, 1, PMU_TMR_2G4_TRIG_CTL_TMR0_2G4_TRIG_FUNC_EN, 1, PMU_TMR_2G4_TRIG_CTL_TMR0_2G4_TRIG_INT_EN, 1, PMU_TMR_2G4_TRIG_CTL_TMR0_2G4_TRIG_MODE, 0));
    // // mac2g4 mac sleep interrupt clear register, 2g4_mac pmu wakupint mask, 2g4_mac cpu wakupint mask
    // REGW(&OM_PMU->OSC_INT_CTRL, MASK_3REG(PMU_OSC_INI_CTRL_2G4_WAKEUP_INT_CLR, 1, PMU_OSC_INI_CTRL_2G4_WAKEUP_INT_MASK_PMU, 0, PMU_OSC_INI_CTRL_2G4_WAKEUP_INT_MASK_CPU, 0));
    // REGW(&OM_PMU->TMR_2G4_TRIG_CTL, MASK_1REG(PMU_TMR_2G4_TRIG_CTL_TMR_2G4_NATIVE_INT_EN, 1));
    // OM_24G->TMR0_NXT_32K_TP = OM_24G->TMR_NATIVE_CNT_32K + 10;
    // OM_24G->TMR0_NXT_8M_TP = OM_24G->TMR_NATIVE_CNT_8M + 0;
    // last_32k = OM_24G->TMR0_NXT_32K_TP;
    // last_8M = OM_24G->TMR0_NXT_8M_TP;
    // OM_24G->TMR0_TRIG_PREFETCH = 0;

    // TIMER1 START
    REGW(&OM_PMU->TMR_2G4_TRIG_CTL, MASK_4REG(PMU_TMR_2G4_TRIG_CTL_TMR_2G4_NATIVE_INT_EN, 1, PMU_TMR_2G4_TRIG_CTL_TMR1_2G4_TRIG_FUNC_EN, 1, PMU_TMR_2G4_TRIG_CTL_TMR1_2G4_TRIG_INT_EN, 1, PMU_TMR_2G4_TRIG_CTL_TMR1_2G4_TRIG_MODE, 0));
    // mac2g4 mac sleep interrupt clear register, 2g4_mac pmu wakupint mask, 2g4_mac cpu wakupint mask
    REGW(&OM_PMU->OSC_INT_CTRL, MASK_3REG(PMU_OSC_INI_CTRL_2G4_WAKEUP_INT_CLR, 1, PMU_OSC_INI_CTRL_2G4_WAKEUP_INT_MASK_PMU, 0, PMU_OSC_INI_CTRL_2G4_WAKEUP_INT_MASK_CPU, 0));
    REGW(&OM_PMU->TMR_2G4_TRIG_CTL, MASK_1REG(PMU_TMR_2G4_TRIG_CTL_TMR_2G4_NATIVE_INT_EN, 1));
    OM_24G->TMR1_NXT_32K_TP = OM_24G->TMR_NATIVE_CNT_32K + 10;
    OM_24G->TMR1_NXT_8M_TP = OM_24G->TMR_NATIVE_CNT_8M + 0;
    OM_24G->TMR1_TRIG_PREFETCH = 0;
    om24g_write_it();
}

/**
 *******************************************************************************
 * @brief TX sensitivity test
 *
 * Package structure A、NO_ACK mode、Fixed length mode、Compatible with 6621C/D/E/F，nordic series
 * 5 bytes sync word : (sync_word0 + rx/tx_addr) 0x9843AF0B46
 * 32 bytes payload:   {1,2,......32}
 *
 * whitening and crc disabled.
 *******************************************************************************
 */
static void om24g_tx_sensitivity_test(void)
{
    bool om24g_state = false;

    om24g_config_t om24g_config_tty = om24g_config_noack;

    om24g_config_tty.sync_word0 = 0x9843AF0B;
    om24g_config_tty.tx_addr = 0x46;
    om24g_config_tty.addr_chk = 1;
    om24g_config_tty.white_en = 0;
    om24g_config_tty.crc_en = 0;
    om24g_config_tty.static_len  = 0x20;
    om24g_config_tty.freq = 2402;

    drv_om24g_init(&om24g_config_tty);
    REGW(&OM_24G->PRE_GUARD, MASK_1REG(OM24G_GUARD_EN, 0x01));

    for (uint8_t i = 0; i < 32; i++) {
        om24g_tx_payload[i] = i + 1;
    }
    NVIC_DisableIRQ(OM24G_RF_IRQn);
    drv_om24g_dump_rf_register();
    while (1) {
        for (uint8_t i = 0; i < 32; i++) {
            om24g_tx_payload[i] = i+1;
        }
        om24g_state = drv_om24g_write(om24g_tx_payload, 32);
        if (om24g_state == true) {
            tx_count++;
            OM_LOG(OM_LOG_DEBUG, "tx_cnt: %d\r\n", tx_count);
        }
        DRV_DELAY_MS(500);
    }
}

static void om24g_write_int_timestamp(void)
{
    drv_om24g_init(&om24g_config_b);
    REGW(&OM_24G->PKTCTRL0, MASK_2REG(OM24G_PKTCTRL0_TS_CLK_EN, 1, OM24G_PKTCTRL0_TIMESTAMP, 1));
    drv_om24g_register_event_callback(om24g_callback_timestamp);
    drv_pmu_timer_register_isr_callback(PMU_TIMER_TRIG_VAL0, timer1_callback_timestamp);
    drv_om24g_dump_rf_register();
    drv_pmu_timer_trig_set(PMU_TIMER_TRIG_VAL0, (drv_pmu_timer_cnt_get() + PMU_TIMER_MS2TICK(300)));
    om24g_write_it();
}

static void om24g_tx_test(om24g_transfer_t tx_case)
{
    switch(tx_case) {
        case OM24G_SENSITIVITY:
            om24g_tx_sensitivity_test();
            break;
        case OM24G_POLL_STRUCTURE_A:
            /*Polling mode transmission packet*/
            om24g_write_structure_a();
            break;
        case OM24G_INT_STRUCTURE_A:
            /*Interrupt mode transmission packet, Package structure A*/
            om24g_write_int_structure_a();
            break;
        case OM24G_INT_STRUCTURE_B:
            /*Interrupt mode transmission packet, Package structure B*/
            om24g_write_int_structure_b();
            break;
        case OM24G_ACK_MODE_STRUCTURE_A:
            /*ACK Polling mode transmission packet*/
            om24g_write_ack_mode_structure_a();
            break;
        case OM24G_INT_ACK_MODE_STRUCTURE_A:
            /*ACK Interrupt mode transmission packet*/
            om24g_write_int_ack_mode_structure_a();
            //drv_pmu_timer_trig_set(PMU_TIMER_TRIG_VAL0, (drv_pmu_timer_cnt_get() + PMU_TIMER_MS2TICK(500)));
            break;
        case OM24G_TX_RX_SWITCH_POLL:
            om24g_tx_to_rx_poll_mode();
            break;
        case OM24G_TX_RX_SWITCH_INT:
            om24g_tx_to_rx_int_mode();
            break;
        case OM24G_RX_ADDR_CHECK:
            om24g_write_txaddr_check_b();
            break;
        case OM24G_STATIC_LENTH:
            om24g_structure_b_static_len_tx();
            break;
        case OM24G_BLE_MODE:
            om24g_ble_mode_tx();
            break;
        case OM24G_HARDWARE_TIMER_TX:
            om24g_hardware_timer_sleep_tx();
            // om24g_hardware_timer_no_sleep_tx();
            break;
        case OM24G_TIMESTAMP:
            om24g_write_int_timestamp();
            break;
        default:
            break;
    }
}

#else

static uint8_t bitcount(uint8_t n)
{
    uint8_t count = 0;
    while (n) {
        count++;
        n &= (n - 1);
    }
    return count;
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

/*Packet structure A receives packets in polling mode and can communicate with function om24g_write_structure_a(void). */
static void om24g_read_structure_a(void)
{
    uint16_t payload_len = 0;
    uint16_t error_count = 0;
    uint16_t right_count = 0;
    bool error_flag = false;

    drv_om24g_init(&om24g_config_noack);
    NVIC_DisableIRQ(OM24G_RF_IRQn);
    drv_om24g_dump_rf_register();
    while (1) {
        payload_len = drv_om24g_read(om24g_rx_payload, 8000);
        OM_LOG(OM_LOG_DEBUG, "len:%d\r\n", payload_len);
        if(!payload_len) {
            OM_LOG(OM_LOG_DEBUG, "rx timeout\r\n");
            break;
        }
        for (int i = 0x00; i < (payload_len - 1); i++) {
            if (om24g_rx_payload[i] != om24g_rx_payload[i+1]) {
                error_flag = true;
            }
        }
        if (error_flag) {
            error_count++;
            error_flag = false;
            OM_LOG_DEBUG_ARRAY_EX("err Pkt", om24g_rx_payload, payload_len);
        } else {
            right_count++;
            OM_LOG_DEBUG_ARRAY_EX("Pkt", om24g_rx_payload, payload_len);
        }
        OM_LOG(OM_LOG_DEBUG, "E:%d  R:%d \r\n", error_count, right_count);
    }
    error_count = 0;
    right_count = 0;
}

/*Packet structure A receives packets in interrupt mode and can communicate with function om24g_write_int_structure_a(void). */
static void om24g_read_int_structure_a(void)
{
    drv_om24g_init(&om24g_config_noack);
    drv_om24g_register_event_callback(om24g_callback);
    drv_om24g_dump_rf_register();
    drv_om24g_read_int(om24g_rx_payload, 166);
}

static void om24g_rx_to_tx_poll_mode(void)
{
    static uint8_t j = 0;
    bool om24g_state = false;
    uint16_t payload_len = 0;

    drv_om24g_init(&om24g_config_b);
    NVIC_DisableIRQ(OM24G_RF_IRQn);
    drv_om24g_dump_rf_register();
    while (1) {
        j++;
        if(j == 250) {
            j = 0;
        }
        for (uint8_t i = 0; i < 32; i++) {
            om24g_tx_payload[i] = j;
        }
        payload_len = drv_om24g_read(om24g_rx_payload, DRV_MAX_DELAY);
        if(!payload_len) {
            OM_LOG(OM_LOG_DEBUG, "rx timeout\r\n");
        }
        OM_LOG_DEBUG_ARRAY_EX("Pkt", om24g_rx_payload, payload_len);
        om24g_state = drv_om24g_write(om24g_tx_payload, 32);
        if (om24g_state == true) {
            tx_count++;
            OM_LOG(OM_LOG_DEBUG, "tx_cnt: %d\r\n", tx_count);
        }
    }
}

static void om24g_rx_to_tx_int_mode(void)
{
    for (uint8_t i = 0; i < 32; i++) {
        om24g_tx_payload[i] = i;
    }
    drv_om24g_init(&om24g_config_b);
    drv_om24g_register_event_callback(om24g_callback_rx_to_tx);
    drv_om24g_dump_rf_register();
    drv_om24g_read_int(om24g_rx_payload, 166);
}

static void om24g_structure_b_static_len_rx(void)
{
    om24g_config_t om24g_config_static = om24g_config_b;
    om24g_config_static.static_len          = 0x20;
    om24g_config_static.hdr_bits            = 0x00;
    om24g_config_static.dpl_en              = 0x00;

    drv_om24g_init(&om24g_config_static);
    drv_om24g_register_event_callback(om24g_callback);
    drv_om24g_dump_rf_register();
    drv_om24g_read_int(om24g_rx_payload, 166);
}

static void om24g_read_rxaddr_check_b(void)
{
    om24g_config_t om24g_config_addr = om24g_config_b;
    om24g_config_addr.data_rate           = OM24G_RATE_500K;
    om24g_config_addr.dpl_en              = 1;
    om24g_config_addr.tx_addr             = 0xbb;
    om24g_config_addr.rx_addr             = 0xbb;
    om24g_config_addr.addr_chk            = 1;
    om24g_config_addr.hdr_bits            = 0x10;
    om24g_config_addr.addr1_bits          = 0x08;
    om24g_config_addr.len_bits            = 0x08;
    om24g_config_addr.addr1_pos           = 0x08;
    om24g_config_addr.len_pos             = 0x00;
    om24g_config_addr.endian              = OM24G_ENDIAN_LSB;
    om24g_config_addr.crc_len             = OM24G_CRC_3BYTE;
    om24g_config_addr.crc_poly            = 0x065B;
    om24g_config_addr.crc_init            = 0x555555;
    om24g_config_addr.crc_skip_sync       = 1;
    om24g_config_addr.crc_skip_len        = 0;
    om24g_config_addr.crc_skip_addr       = 0;
    om24g_config_addr.modulation_mode     = OM24G_MODULATION_FSK;

    drv_om24g_init(&om24g_config_addr);
    drv_om24g_register_event_callback(om24g_callback);
    drv_om24g_dump_rf_register();
    drv_om24g_read_int(om24g_rx_payload, 166);
}

static void om24g_ble_mode_rx(void)
{
    drv_om24g_init(&om24g_config_ble);
    drv_om24g_register_event_callback(om24g_callback);
    drv_om24g_dump_rf_register();
    drv_om24g_read_int(om24g_rx_payload, 166);
}

/*Packet structure A receives packets in polling ACK mode and can communicate with function om24g_write_ack_mode_structure_a(void). */
static void om24g_read_ack_mode_structure_a(void)
{
    uint8_t j = 0;
    uint16_t payload_len = 0;
    bool error_flag = false;

    for (uint8_t i = 0; i < 32; i++) {
        om24g_tx_payload[i] = i + 1;
    }
    drv_om24g_init(&om24g_config);
    NVIC_DisableIRQ(OM24G_RF_IRQn);
    drv_om24g_dump_rf_register();
    while (1) {
        j++;
        payload_len = drv_om24g_read(om24g_rx_payload, 8000);
        if(!payload_len) {
            OM_LOG(OM_LOG_DEBUG, "rx timeout\r\n");
            break;
        }
        if (j > 32) {
            j = 1;
        }
        drv_om24g_write_ack(j);
        OM_LOG_DEBUG_ARRAY_EX("ack_pkt", om24g_tx_payload, j);
        for (int i = 0; i < payload_len; i++) {
            if ((i + 1) != om24g_rx_payload[i]) {
                error_flag = true;
            }
        }
        if (error_flag) {
            error_count++;
            error_flag = false;
            OM_LOG_DEBUG_ARRAY_EX("err Pkt", om24g_rx_payload, payload_len);
        } else {
            right_count++;
            OM_LOG_DEBUG_ARRAY_EX("Pkt", om24g_rx_payload, payload_len);
        }
    }
    drv_om24g_control(OM24G_CONTROL_CLK_DISABLE, NULL);
    OM_LOG(OM_LOG_DEBUG, "E:%d ", error_count);
    OM_LOG(OM_LOG_DEBUG, "R:%d \r\n", right_count);
    error_count = 0;
    right_count = 0;
}

/*Packet structure A receives packets in interrupt ACK mode and can communicate with function om24g_write_ack_mode_structure_a(void). */
static void om24g_read_int_ack_mode_structure_a(void)
{
    drv_om24g_init(&om24g_config);
    drv_om24g_register_event_callback(om24g_callback_ack_mode);
    drv_om24g_dump_rf_register();
    drv_om24g_read_int(om24g_rx_payload, 166);
}

static void om24g_hardware_timer_sleep_rx(void)
{
    drv_om24g_init(&om24g_config_b);
    drv_om24g_register_event_callback(om24g_callback);
    drv_om24g_dump_rf_register();
    drv_om24g_read_int(om24g_rx_payload, 166);
}

/**
 *******************************************************************************
 * @brief Receiving sensitivity test
 *
 * Package structure A、NO_ACK mode、Fixed length mode、Compatible with 6621C/D/E/F，nordic series
 * 5 bytes sync word : (sync_word0 + rx/tx_addr) 0x9843AF0B46
 * 32 bytes payload:   {1,2,......32}
 *
 * whitening and crc disabled.
 *******************************************************************************
 */
static void om24g_rx_sensitivity_test(void)
{
    uint32_t error_bit = 0;
    uint16_t error_count = 0;
    uint16_t right_count = 0;
    bool error_flag = false;
    uint16_t payload_len = 0;
    //uint64_t timeout = 0;
    om24g_config_t om24g_config_rty = om24g_config_noack;

    om24g_config_rty.sync_word0 = 0x9843AF0B;
    om24g_config_rty.rx_addr = 0x46;
    om24g_config_rty.addr_chk = 1;
    om24g_config_rty.white_en = 0;
    om24g_config_rty.crc_en = 0;
    om24g_config_rty.static_len  = 0x20;
    om24g_config_rty.freq = 2402;

    drv_om24g_init(&om24g_config_rty);
    REGW(&OM_24G->PRE_GUARD, MASK_1REG(OM24G_GUARD_EN, 0x01));
    for (uint8_t i = 0; i < 32; i++) {
        om24g_tx_payload[i] = i + 1;
    }
    NVIC_DisableIRQ(OM24G_RF_IRQn);
    drv_om24g_dump_rf_register();
    while (1) {
        payload_len = drv_om24g_read(om24g_rx_payload, DRV_MAX_DELAY);
        if (payload_len) {
            for (uint8_t i = 0; i < payload_len; i++) {
                if ((i + 1) != om24g_rx_payload[i]) {
                    error_flag = true;
                    error_bit += bitcount((i+1) ^ om24g_rx_payload[i]);
                }
            }
            if (error_flag) {
                error_flag = false;
                error_count++;
                OM_LOG_DEBUG_ARRAY_EX("err Pkt", om24g_rx_payload, payload_len);
            } else {
                right_count++;
                //OM_LOG_DEBUG_ARRAY_EX("Pkt", om24g_rx_payload, payload_len);
                OM_LOG(OM_LOG_DEBUG, "\nT:%d  E:%d  R:%d  BitError:%d\r\n", (error_count + right_count), error_count, right_count, error_bit);

            }
            //timeout = 0;
            memset(om24g_rx_payload, 0, sizeof(om24g_rx_payload));
            payload_len = 0;
        } else {
            OM_LOG(OM_LOG_DEBUG, "rx timeout\r\n");
            break;
        }
//        timeout++;
//        if (timeout > 9000000) {
//            break;
//        }
    }
    OM24G_CE_LOW();
    OM_LOG(OM_LOG_DEBUG, "\nT:%d  E:%d  R:%d  BitError:%d\r\n", (error_count + right_count), error_count, right_count, error_bit);
}

static void om24g_read_int_timestamp(void)
{
    drv_om24g_init(&om24g_config_b);
    REGW(&OM_24G->PKTCTRL0, MASK_2REG(OM24G_PKTCTRL0_TS_CLK_EN, 1, OM24G_PKTCTRL0_TIMESTAMP, 1));
    REGW(&OM_24G->TIMESTAMP_CFG, MASK_1REG(OM24G_TS_CNT_EN, 1));
    drv_om24g_register_event_callback(om24g_callback_timestamp);
    drv_pmu_timer_register_isr_callback(PMU_TIMER_TRIG_VAL0, timer1_callback_timestamp);
    drv_om24g_dump_rf_register();
    drv_om24g_read_int(om24g_rx_payload, 166);
}

static void om24g_rx_test(om24g_transfer_t rx_case)
{
    switch(rx_case) {
        case OM24G_SENSITIVITY:
            om24g_rx_sensitivity_test();
            break;
        case OM24G_POLL_STRUCTURE_A:
            /*Polling mode receiving packet*/
            om24g_read_structure_a();
            break;
        case OM24G_INT_STRUCTURE_A:
            /*Interrupt mode receiving packet,  Package structure A*/
            om24g_read_int_structure_a();
            break;
        case OM24G_INT_STRUCTURE_B:
            /*Interrupt mode transmission packet, Package structure B*/
            om24g_read_int_structure_b();
            break;
        case OM24G_ACK_MODE_STRUCTURE_A:
            /*ACK Polling mode receiving packet*/
            om24g_read_ack_mode_structure_a();
            break;
        case OM24G_INT_ACK_MODE_STRUCTURE_A:
            /*ACK Interrupt mode receiving packet*/
            om24g_read_int_ack_mode_structure_a();
            break;
        case OM24G_TX_RX_SWITCH_POLL:
            om24g_rx_to_tx_poll_mode();
            break;
        case OM24G_TX_RX_SWITCH_INT:
            om24g_rx_to_tx_int_mode();
            break;
        case OM24G_RX_ADDR_CHECK:
            om24g_read_rxaddr_check_b();
            break;
        case OM24G_STATIC_LENTH:
            om24g_structure_b_static_len_rx();
            break;
        case OM24G_BLE_MODE:
            om24g_ble_mode_rx();
            break;
        case OM24G_HARDWARE_TIMER_TX:
            om24g_hardware_timer_sleep_rx();
            break;
        case OM24G_TIMESTAMP:
            om24g_read_int_timestamp();
            break;
        default:
            break;
    }
}

#endif

static void timer1_callback(void *om_reg, drv_event_t drv_event, void *param0, void *param1)
{
    OM24G_CE_LOW();
    //drv_gpio_toggle(OM_GPIO0, GPIO_MASK(8));
    drv_om24g_control(OM24G_CONTROL_CLK_ENABLE, NULL);
    // drv_om24g_dump_rf_register();
    pm_sleep_prevent(PM_ID_24G);
    #if TX_ROLE
    #if OM24G_ACK_MODE
        om24g_write_it_ack_mode();
        //drv_pmu_timer_trig_set(PMU_TIMER_TRIG_VAL0, (drv_pmu_timer_cnt_get() + 5000));
    #else
        om24g_write_it();
    #endif

    #else

    #if ENABLE_SLEEP_MODE
    #if OM24G_ACK_MODE
        drv_om24g_register_event_callback(om24g_callback_ack_mode);
    #else
        drv_om24g_register_event_callback(om24g_callback);
    #endif
        drv_om24g_read_int(om24g_rx_payload, 166);
    #endif

    #endif
    //OM_LOG(OM_LOG_DEBUG, "tim \r\n");
}

static void timer1_callback_timestamp(void *om_reg, drv_event_t drv_event, void *param0, void *param1)
{
    drv_pmu_timer_trig_set(PMU_TIMER_TRIG_VAL0, (drv_pmu_timer_cnt_get() + PMU_TIMER_MS2TICK(300)));
    // OM_LOG(OM_LOG_DEBUG, "tim \r\n");
    OM24G_CE_LOW();
    drv_om24g_control(OM24G_CONTROL_CLK_ENABLE, NULL);
    // drv_om24g_dump_rf_register();
    pm_sleep_prevent(PM_ID_24G);
    #if TX_ROLE
    #if OM24G_ACK_MODE
        om24g_write_it_ack_mode();
        //drv_pmu_timer_trig_set(PMU_TIMER_TRIG_VAL0, (drv_pmu_timer_cnt_get() + 5000));
    #else
        om24g_write_it();
    #endif

    #else

    #if ENABLE_SLEEP_MODE
    #if OM24G_ACK_MODE
        drv_om24g_register_event_callback(om24g_callback_ack_mode);
    #else
        drv_om24g_register_event_callback(om24g_callback);
    #endif
        drv_om24g_read_int(om24g_rx_payload, 166);
    #endif

    #endif
}

static void vEvtEventHandler(void)
{
    if (xSemOm24g) {
        osSemaphoreRelease(xSemOm24g);
    }
}

static void vEvtScheduleTask(void *arguments)
{
    drv_rf_init();
    drv_pmu_timer_init();
    OM_LOG(OM_LOG_DEBUG, "start run om24g \r\n");
    pm_sleep_prevent(PM_ID_24G);
    drv_pmu_timer_register_isr_callback(PMU_TIMER_TRIG_VAL0, timer1_callback);
    #if TX_ROLE
    om24g_tx_test(OM24G_INT_STRUCTURE_B);
    #else
    om24g_rx_test(OM24G_INT_STRUCTURE_B);
    #endif

    // while(1) {
    //     OM_LOG(OM_LOG_DEBUG, "thread \r\n");
    //     #if TX_ROLE
    //     drv_pmu_timer_trig_set(PMU_TIMER_TRIG_VAL0, (drv_pmu_timer_cnt_get() + PMU_TIMER_MS2TICK(500)));
    //     //drv_pmu_timer_trig_set(PMU_TIMER_TRIG_VAL0, (drv_pmu_timer_cnt_get() + 10000));
    //     #endif
    //     osDelay(1000);
    //     drv_gpio_toggle(OM_GPIO0, GPIO_MASK(7));
    // }

    // Evt and evt timer initialization
    evt_init();
   // Create semaphore
    xSemOm24g = osSemaphoreNew(1, 0, NULL);

    // Set evt callback
    evt_schedule_trigger_callback_set(vEvtEventHandler);

    while(1) {
        // schedule for handle evt
        evt_schedule();
        // Wait for semaphore
        osSemaphoreAcquire(xSemOm24g, osWaitForever);
    }
}


/**
 * @brief  Start ble mesh task
 **/
void vStartOm24gTask(void)
{
    const osThreadAttr_t Om24gThreadAttr = {
        .name           = "om24g",
        .attr_bits = 0,
        .cb_mem = NULL,
        .cb_size = 0,
        .stack_mem = NULL,
        .stack_size = 2048,
        .priority = osPriorityRealtime,
        .tz_module = 0,
    };

    // Create om24g Task
    osThreadNew(vEvtScheduleTask, NULL, &Om24gThreadAttr);
}
