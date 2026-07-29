/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup OM24G OM24G
 * @ingroup  DEVICE
 * @brief    OM24G register
 * @details  OM24G register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __OM24G_REG_H
#define __OM24G_REG_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include "common_reg.h"


#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
// PKTCTRL0 (00h)
#define OM24G_PKTCTRL0_PRIM_RX_POS          0
#define OM24G_PKTCTRL0_STRUCT_SEL_POS       1
#define OM24G_PKTCTRL0_ACK_TX_POSITION_POS  2
#define OM24G_PKTCTRL0_ADDR_CHK_POS         4
#define OM24G_PKTCTRL0_RX_0_1_RVS_POS       6
#define OM24G_PKTCTRL0_TX_0_1_RVS_POS       7
#define OM24G_PKTCTRL0_TIMESTAMP_POS        8
#define OM24G_PKTCTRL0_TS_CLK_EN_POS        9
#define OM24G_PKTCTRL0_FORCE_CAL_POS        10
#define OM24G_PKTCTRL0_MAC_SEL_POS          11
#define OM24G_PKTCTRL0_CE_H_THRE_POS        12
#define OM24G_PKTCTRL0_NUM_HDR_BITS_POS     16
#define OM24G_PKTCTRL0_SPARK_EN_POS         24

#define OM24G_PKTCTRL0_PRIM_RX_MASK          0x00000001
#define OM24G_PKTCTRL0_STRUCT_SEL_MASK       0x00000002
#define OM24G_PKTCTRL0_ACK_TX_POSITION_MASK  0x0000000C
#define OM24G_PKTCTRL0_ADDR_CHK_MASK         0x00000030
#define OM24G_PKTCTRL0_RX_0_1_RVS_MASK       0x00000040
#define OM24G_PKTCTRL0_TX_0_1_RVS_MASK       0x00000080
#define OM24G_PKTCTRL0_TIMESTAMP_MASK        0x00000100
#define OM24G_PKTCTRL0_TS_CLK_EN_MASK        0x00000200
#define OM24G_PKTCTRL0_FORCE_CAL_MASK        0x00000400
#define OM24G_PKTCTRL0_MAC_SEL_MASK          0x00000800
#define OM24G_PKTCTRL0_CE_H_THRE_MASK        0x0000F000
#define OM24G_PKTCTRL0_NUM_HDR_BITS_MASK     0x003F0000
#define OM24G_PKTCTRL0_SPARK_EN_MASK         0x01000000

// FB_PKTCTRL (04h)
#define OM24G_FB_PKTCTRL_NUM_ADDR1_BITS_POS   0
#define OM24G_FB_PKTCTRL_NUM_LEN_BITS_POS     8
#define OM24G_FB_PKTCTRL_ADDR1_POS_POS        16
#define OM24G_FB_PKTCTRL_LEN_POS_POS          24
#define OM24G_FB_PKTCTRL_ADDR1_LOC_POS        29

#define OM24G_FB_PKTCTRL_NUM_ADDR1_BITS_MASK  0x0000000F
#define OM24G_FB_PKTCTRL_NUM_LEN_BITS_MASK    0x00001F00
#define OM24G_FB_PKTCTRL_ADDR1_POS_MASK       0x001F0000
#define OM24G_FB_PKTCTRL_LEN_POS_MASK         0x1F000000
#define OM24G_FB_PKTCTRL_ADDR1_LOC_MASK       0x60000000

// MAC_EN(08h)
#define OM24G_CLK_EN_POS                    0

#define OM24G_CLK_EN_MASK                   0x00000001

// DYNPD (0Ch)
#define OM24G_DPL_P0_POS                    0
#define OM24G_DPL_P1_POS                    1
#define OM24G_DPL_P2_POS                    2
#define OM24G_DPL_P3_POS                    3
#define OM24G_DPL_P4_POS                    4
#define OM24G_DPL_P5_POS                    5
#define OM24G_DPL_P6_POS                    6
#define OM24G_DPL_P7_POS                    7

#define OM24G_DPL_P0_MASK                   0x00000001
#define OM24G_DPL_P1_MASK                   0x00000002
#define OM24G_DPL_P2_MASK                   0x00000004
#define OM24G_DPL_P3_MASK                   0x00000008
#define OM24G_DPL_P4_MASK                   0x00000010
#define OM24G_DPL_P5_MASK                   0x00000020
#define OM24G_DPL_P6_MASK                   0x00000040
#define OM24G_DPL_P7_MASK                   0x00000080

//FEATURE  (10h)
#define OM24G_EN_ACK_PAY_POS                0
#define OM24G_EN_DPL_POS                    1

#define OM24G_EN_ACK_PAY_MASK               0x00000001
#define OM24G_EN_DPL_MASK                   0x00000002

//INT_MASK (14h)
#define OM24G_MASK_CRC_ERR_POS               0
#define OM24G_MASK_BCC_MATCH_POS             1
#define OM24G_MASK_SYNC0_DET_POS             2
#define OM24G_MASK_SYNC1_DET_POS             3
#define OM24G_MASK_DEV_MATCH_POS             4
#define OM24G_MASK_MAX_RT_POS                5
#define OM24G_MASK_TX_DS_POS                 6
#define OM24G_MASK_RX_DR_POS                 7

#define OM24G_MASK_CRC_ERR_MASK              0x00000001
#define OM24G_MASK_BCC_MATCH_MASK            0x00000002
#define OM24G_MASK_SYNC0_DET_MASK            0x00000004
#define OM24G_MASK_SYNC1_DET_MASK            0x00000008
#define OM24G_MASK_DEV_MATCH_MASK            0x00000010
#define OM24G_MASK_MAX_RT_MASK               0x00000020
#define OM24G_MASK_TX_DS_MASK                0x00000040
#define OM24G_MASK_RX_DR_MASK                0x00000080

//INT_ST(18h)
#define OM24G_INT_CRC_ERR_POS               0
#define OM24G_INT_BCC_MATCH_POS             1
#define OM24G_INT_SYNC0_DET_POS             2
#define OM24G_INT_SYNC1_DET_POS             3
#define OM24G_INT_RX_TM_POS                 4
#define OM24G_INT_MAX_RT_POS                5
#define OM24G_INT_TX_DS_POS                 6
#define OM24G_INT_RX_DR_POS                 7
#define OM24G_INT_MAX_LEN_POS               8
#define OM24G_INT_RX_ADDR_MISS_POS          9
#define OM24G_INT_RX_OVERLAY_POS            10
#define OM24G_INT_SYNC_DET_TM_POS           11
#define OM24G_INT_CRC_OK_POS                12
#define OM24G_INT_DEVMATCH_POS              13
#define OM24G_INT_PAYLOAD_POS               14
#define OM24G_INT_ADDRESS_POS               15
#define OM24G_INT_TIMER1_POS                16
#define OM24G_INT_TIMER0_POS                17
#define OM24G_INT_HDR_CRC_ERR_POS           19

#define OM24G_INT_CRC_ERR_MASK              0x00000001
#define OM24G_INT_BCC_MATCH_MASK            0x00000002
#define OM24G_INT_SYNC0_DET_MASK            0x00000004
#define OM24G_INT_SYNC1_DET_MASK            0x00000008
#define OM24G_INT_RX_TM_MASK                0x00000010
#define OM24G_INT_MAX_RT_MASK               0x00000020
#define OM24G_INT_TX_DS_MASK                0x00000040
#define OM24G_INT_RX_DR_MASK                0x00000080
#define OM24G_INT_MAX_LEN_MASK              0x00000100
#define OM24G_RX_ADDR_MISS_MASK             0x00000200
#define OM24G_INT_RX_OVERLAY_MASK           0x00000400
#define OM24G_INT_SYNC_DET_TM_MASK          0x00000800
#define OM24G_INT_CRC_OK_MASK               0x00001000
#define OM24G_INT_DEVMATCH_MASK             0x00002000
#define OM24G_INT_PAYLOAD_MASK              0x00004000
#define OM24G_INT_ADDRESS_MASK              0x00008000
#define OM24G_INT_TIMER1_MASK               0x00010000
#define OM24G_INT_TIMER0_MASK               0x00020000
#define OM24G_INT_HDR_MASK                  0x00040000
#define OM24G_INT_HDR_CRC_ERR_MASK          0x00080000

//EN_AA(20h)
#define OM24G_ENAA_P0_POS                   0
#define OM24G_ENAA_P1_POS                   1
#define OM24G_ENAA_P2_POS                   2
#define OM24G_ENAA_P3_POS                   3
#define OM24G_ENAA_P4_POS                   4
#define OM24G_ENAA_P5_POS                   5

#define OM24G_ENAA_P0_MASK                  0x00000001
#define OM24G_ENAA_P1_MASK                  0x00000002
#define OM24G_ENAA_P2_MASK                  0x00000004
#define OM24G_ENAA_P3_MASK                  0x00000008
#define OM24G_ENAA_P4_MASK                  0x00000010
#define OM24G_ENAA_P5_MASK                  0x00000020

//EN_RXADDR(24h)
#define OM24G_ERX_P0_POS                   0
#define OM24G_ERX_P1_POS                   1
#define OM24G_ERX_P2_POS                   2
#define OM24G_ERX_P3_POS                   3
#define OM24G_ERX_P4_POS                   4
#define OM24G_ERX_P5_POS                   5

#define OM24G_ERX_P0_MASK                  0x00000001
#define OM24G_ERX_P1_MASK                  0x00000002
#define OM24G_ERX_P2_MASK                  0x00000004
#define OM24G_ERX_P3_MASK                  0x00000008
#define OM24G_ERX_P4_MASK                  0x00000010
#define OM24G_ERX_P5_MASK                  0x00000020

// FA_PKTCTRL(28h)
#define OM24G_FA_PKTCTRL_SETUP_AW_POS      0
#define OM24G_FA_PKTCTRL_BP_RX_ADDR_POS    1
#define OM24G_FA_PKTCTRL_NO_ACK_POS        2
#define OM24G_FA_PKTCTRL_EN_DYN_ACK_POS    3
#define OM24G_FA_PKTCTRL_LEN_10B_EN_POS    4

#define OM24G_FA_PKTCTRL_SETUP_AW_MASK     0x00000001
#define OM24G_FA_PKTCTRL_BP_RX_ADDR_MASK   0x00000002
#define OM24G_FA_PKTCTRL_NO_ACK_MASK       0x00000004
#define OM24G_FA_PKTCTRL_EN_DYN_ACK_MASK   0x00000008
#define OM24G_FA_PKTCTRL_LEN_10B_EN_MASK   0x00000010

// FA_SETUP_RETR(2Ch)
#define OM24G_FA_SETUP_RETR_ARC_POS        0
#define OM24G_FA_SETUP_RETR_ARD_POS        4
#define OM24G_FA_SETUP_RETR_ARC_MASK       0x0000000F
#define OM24G_FA_SETUP_RETR_ARD_MASK       0x000000F0

// FA_OBSERVE_TX (30h)
#define OM24G_ARC_CNT_POS                 0
#define OM24G_PLOS_CNT_POS                4
#define OM24G_PLOS_CNT_CLR_POS            8
#define OM24G_ARC_CNT_MASK                0x0000000F
#define OM24G_PLOS_CNT_MASK               0x000000F0
#define OM24G_PLOS_CNT_CLR_MASK           0x00000100

// PREAMBLE (34h)
#define OM24G_PREAMBLE_POS                 0
#define OM24G_PREAMBLE_MASK                0x000000FF

// PREAMBLE_LEN (38h)
#define OM24G_PREAMBLE_LEN_POS             0
#define OM24G_PREAMBLE_LEN_MASK            0x0000000F

// PRE_GUARD (3Ch)
#define OM24G_PREGRD_CNT_POS              0
#define OM24G_PREGRD_EN_POS               4
#define OM24G_TAIL_CTL_POS                5
#define OM24G_GUARD_EN_POS                10
#define OM24G_TAIL_PATERN_POS             12
#define OM24G_PREGRD_CNT_MASK             0x0000000F
#define OM24G_PREGRD_EN_MASK              0x00000010
#define OM24G_TAIL_CTL_MASK               0x000003E0
#define OM24G_GUARD_EN_MASK               0x00000400
#define OM24G_TAIL_PATERN_MASK            0x00007000

// SYNC_WORD0 (40h)
#define OM24G_SYNC_WORD0_POS              0
#define OM24G_SYNC_WORD0_MASK             0xFFFFFFFF

// SYNC_WORD1 (44h)
#define OM24G_SYNC_WORD1_POS              0
#define OM24G_SYNC_WORD1_MASK             0xFFFFFFFF

// TX_ADDR (48h)
#define OM24G_TX_ADDR_POS                 0
#define OM24G_TX_ADDR_MASK                0x000000FF

// SYNC_WORD_SEL (4Ch)
#define OM24G_SYNC_WORD_SEL_POS           0
#define OM24G_SYNC_WORD_SEL_MASK          0x00000001

// RX_ADDR_P0 (50h)
#define OM24G_RX_ADDR_P0_POS              0
#define OM24G_RX_ADDR_P0_MASK             0x000000FF

// RX_ADDR_P1 (54h)
#define OM24G_RX_ADDR_P1_POS              0
#define OM24G_RX_ADDR_P1_MASK             0x000000FF

// RX_ADDR_P2 (58h)
#define OM24G_RX_ADDR_P2_POS              0
#define OM24G_RX_ADDR_P2_MASK             0x000000FF

// RX_ADDR_P3 (5Ch)
#define OM24G_RX_ADDR_P3_POS              0
#define OM24G_RX_ADDR_P3_MASK             0x000000FF

// RX_ADDR_P4 (60h)
#define OM24G_RX_ADDR_P4_POS              0
#define OM24G_RX_ADDR_P4_MASK             0x000000FF

// RX_ADDR_P5 (64h)
#define OM24G_RX_ADDR_P5_POS              0
#define OM24G_RX_ADDR_P5_MASK             0x000000FF

// AGC_ME_MO(68h)
#define OM24G_AGC_MO_POS                  0
#define OM24G_AGC_ME_POS                  1

#define OM24G_AGC_MO_MASK                 0x00000001
#define OM24G_AGC_ME_MASK                 0x00000002

// RF_DR(70h)
#define OM24G_RF_DR_TX_RF_DR_POS           0
#define OM24G_EN_TX_ARB_POS                2
#define OM24G_N_REP_POS                    4
#define OM24G_RF_DR_RX_RF_DR_POS           8
#define OM24G_EN_RX_ARB_POS                10
#define OM24G_N_AVR_POS                    12

#define OM24G_RF_DR_TX_RF_DR_MASK          0x00000003
#define OM24G_EN_TX_ARB_MASK               0x00000004
#define OM24G_N_REP_MASK                   0x000000F0
#define OM24G_RF_DR_RX_RF_DR_MASK          0x00000300
#define OM24G_EN_RX_ARB_MASK               0x00000400
#define OM24G_N_AVR_MASK                   0x0000F000

// RF_PD_AHEAD(74h)
#define OM24G_RF_PD_AHEAD_POS              0
#define OM24G_RF_PD_AHEAD_EN_POS           8

#define OM24G_RF_PD_AHEAD_MASK             0x0000007F
#define OM24G_RF_PD_AHEAD_EN_MASK          0x00000100

// RX_P_NO (78h)
#define OM24G_RX_P_NO_POS                 0
#define OM24G_RX_P_NO_MASK                0x00000007

// SETUP_VALUE_H (7Ch)
#define OM24G_RX_SETUP_VALUE_H_POS        0
#define OM24G_TX_SETUP_VALUE_H_POS        8

#define OM24G_RX_SETUP_VALUE_H_MASK       0x0000003F
#define OM24G_TX_SETUP_VALUE_H_MASK       0x00003F00

// CRCCFG (80h)
#define OM24G_CRC_LEN_POS                 0
#define OM24G_CRC_EN_POS                  2
#define OM24G_CRC_MODE_POS                3

#define OM24G_CRC_LEN_MASK                0x00000003
#define OM24G_CRC_EN_MASK                 0x00000004
#define OM24G_CRC_MODE_MASK               0x00000008

// CRCPOLY (84h)
#define OM24G_CRCPOLY_POS                 0
#define OM24G_CRCPOLY_MASK                0xFFFFFFFF

// CRCINIT (88h)
#define OM24G_CRCINIT_POS                 0
#define OM24G_CRCINIT_MASK                0xFFFFFFFF

// CRCSKIP (8Ch)
#define OM24G_CRC_SKIP_SYNC_POS           0
#define OM24G_CRC_SKIP_LEN_POS            1
#define OM24G_CRC_SKIP_ADDR_POS           2
#define OM24G_CRC_SKIP_SYNC_MASK          0x00000001
#define OM24G_CRC_SKIP_LEN_MASK           0x00000002
#define OM24G_CRC_SKIP_ADDR_MASK          0x00000004

// WHITECFG  (94h)
#define OM24G_WHITE_EN_POS                0
#define OM24G_WHITE_SKIP_HEADER_POS       1
#define OM24G_WHITE_SKIP_ADDR_POS         2
#define OM24G_WHITE_SKIP_CRC_POS          3
#define OM24G_WHITE_EN_MASK               0x00000001
#define OM24G_WHITE_SKIP_HEADER_MASK      0x00000002
#define OM24G_WHITE_SKIP_ADDR_MASK        0x00000004
#define OM24G_WHITE_SKIP_CRC_MASK         0x00000008

// WHITESEL (98h)
#define OM24G_WHITE_SEL_POS               0
#define OM24G_WHITE_SEL_MASK              0x00000007

// WHITESEED (9Ch)
#define OM24G_WHITE_SEED_POS              0
#define OM24G_WHITE_SEED_MASK             0x0000FFFF

// WHITEOBIT (A0h)
#define OM24G_WHITEOBIT_POS               0
#define OM24G_WHITEOBIT_MASK              0x0000000F

// DMA_CMD (A4h)
#define OM24G_DMA_CMD_POS                 0
#define OM24G_DMA_CMD_MASK                0x0000FFFF

// DMA_TX_LEN (A8h)
#define OM24G_DMA_TX_LEN_POS              0
#define OM24G_DMA_TX_LEN_MASK             0x0000FFFF

// DMA_TX_ADDR (B8h)
#define OM24G_DMA_TX_ADDR_POS             0
#define OM24G_DMA_TX_ADDR_MASK            0x0001FFFF

// DMA_RX_ADDR (C4h)
#define OM24G_DMA_RX_ADDR_POS             0
#define OM24G_DMA_RX_ADDR_MASK            0x0001FFFF

// RX_DYN_LEN (C8h)
#define OM24G_RX_DYN_LEN_POS              0
#define OM24G_RX_DYN_LEN_MASK             0x0000FFFF

// PACKET_LEN (CCh)
#define OM24G_PACKET_LEN_POS              0
#define OM24G_PACKET_LEN_MASK             0x0000FFFF

// BCC (D0h)
#define OM24G_BCC_CNT_POS                 0
#define OM24G_BCC_CNT_MASK                0x0000FFFF

// TIMESTAMP_RT (D4h)
#define OM24G_TS_VALUE_RT_POS             0
#define OM24G_TS_VALUE_RT_MASK            0xFFFFFFFF

// TIMESTAMP_TRIGER (D8h)
#define OM24G_TS_TRIGER_POS               0
#define OM24G_TS_TRIGER_MASK              0xFFFFFFFF

// TIMESTAMP_CFG (DCh)
#define OM24G_TS_CNT_EN_POS               0
#define OM24G_TS_TRIGER_EN_POS            1
#define OM24G_TS_CNT_EN_MASK              0x00000001
#define OM24G_TS_TRIGER_EN_MASK           0x00000002

// SETUP_VALUE (E0h)
#define OM24G_RX_SETUP_VALUE_POS          0
#define OM24G_TX_SETUP_VALUE_POS          8
#define OM24G_RX_TM_CNT_POS               16
#define OM24G_RX_SETUP_VALUE_MASK         0x000000FF
#define OM24G_TX_SETUP_VALUE_MASK         0x0000FF00
#define OM24G_RX_TM_CNT_MASK              0x3FFF0000


// ENDIAN  (E4h)
#define OM24G_EDIBIT_CRC_POS              0
#define OM24G_EDIBIT_PL_POS               1
#define OM24G_EDIBIT_SW_POS               3
#define OM24G_EDIBIT_ADDR_POS             4
#define OM24G_EDIBIT_HDR_POS              5
#define OM24G_EDIBYTE_CRC_POS             8
#define OM24G_EDIBYTE_SW_POS              9
#define OM24G_EDIBIT_CRC_MASK             0x00000001
#define OM24G_EDIBIT_PL_MASK              0x00000002
#define OM24G_EDIBIT_SW_MASK              0x00000008
#define OM24G_EDIBIT_ADDR_MASK            0x00000010
#define OM24G_EDIBIT_HDR_MASK             0x00000020
#define OM24G_EDIBYTE_CRC_MASK            0x00000100
#define OM24G_EDIBYTE_SW_MASK             0X00000200
// FLUSH  (E8h)
#define OM24G_TX_FIFO_FLUSH_POS           0
#define OM24G_RX_FIFO_FLUSH_POS           4
#define OM24G_ABORT_RF_POS                5
#define OM24G_TX_FIFO_FLUSH_MASK          0x00000001
#define OM24G_RX_FIFO_FLUSH_MASK          0x00000010
#define OM24G_ABORT_RF_MASK               0x00000020

// TESTCTRL  (ECh)
#define OM24G_TEST_PAT_POS                0
#define OM24G_TEST_PAT_EN_POS             8
#define OM24G_PSUDO_RND_POS               9
#define OM24G_CONT_WAVE_POS               10
#define OM24G_FORCE_CRC_ERR_POS           11
#define OM24G_TEST_PAT_MASK               0x000000FF
#define OM24G_TEST_PAT_EN_MASK            0x00000100
#define OM24G_PSUDO_RND_MASK              0x00000200
#define OM24G_CONT_WAVE_MASK              0x00000400
#define OM24G_FORCE_CRC_ERR_MASK          0x00000800

// STATE (F0h)
#define OM24G_MAIN_STATE_POS              0
#define OM24G_CRC_OK_POS                  6
#define OM24G_BUSY_POS                    7
#define OM24G_RXFIFO_FULL_POS             8
#define OM24G_RXFIFO_EMPTY_POS            9
#define OM24G_TXFIFO_FULL_POS             10
#define OM24G_TXFIFO_EMPTY_POS            11
#define OM24G_SYNC_DET_POS                12
#define OM24G_RX_EN_POS                   13
#define OM24G_TX_EN_POS                   14
#define OM24G_CLK_GATE_EN_POS             15
#define OM24G_DMA_READY_POS               16

#define OM24G_MAIN_STATE_MASK             0x0000003F
#define OM24G_CRC_OK_MASK                 0x00000040
#define OM24G_BUSY_MASK                   0x00000080
#define OM24G_RXFIFO_FULL_MASK            0x00000100
#define OM24G_RXFIFO_EMPTY_MASK           0x00000200
#define OM24G_TXFIFO_FULL_MASK            0x00000400
#define OM24G_TXFIFO_EMPTY_MASK           0x00000800
#define OM24G_SYNC_DET_MASK               0x00001000
#define OM24G_RX_EN_MASK                  0x00002000
#define OM24G_TX_EN_MASK                  0x00004000
#define OM24G_CLK_GATE_EN_MASK            0x00008000
#define OM24G_DMA_READY_MASK              0x00010000

// DBG_FREQ (F4h)
#define OM24G_FREQ_POS                    0
#define OM24G_FREQ_MASK                   0x00000FFF

// ACK_MODE (100h)
#define OM24G_ACK_MODE_POS                0
#define OM24G_FIX_SENT_ACK_POS            1
#define OM24G_ACK_JUDGE_MOMENT_POS        2
#define OM24G_CONTINUOUS_MODE_POS         3

#define OM24G_ACK_MODE_MASK               0x00000001
#define OM24G_FIX_SENT_ACK_MASK           0x00000002
#define OM24G_ACK_JUDGE_MOMENT_MASK       0x00000004
#define OM24G_CONTINUOUS_MODE_MASK        0x00000008

// RX_DONE (104h)
#define OM24G_RX_DONE_POS                0
#define OM24G_CRC_ERR_POS                1

#define OM24G_RX_DONE_MASK               0x00000001
#define OM24G_CRC_ERR_MASK               0x00000002

// DEBUG_BUS (108h)
#define OM24G_DBG_BUS_TO_REG_POS         0
#define OM24G_DBG_INDEX_POS              16
#define OM24G_DBG_BUS_TO_REG_EN_POS      18

#define OM24G_DBG_BUS_TO_REG_MASK        0x000000FF
#define OM24G_DBG_INDEX_MASK             0x00000300
#define OM24G_DBG_BUS_TO_REG_EN_MASK     0x00000400

// RX_CTRL (10Ch)
#define OM24G_ADDR_MISS_RESET_ANA_EN_POS 0
#define OM24G_MAX_LEN_RESET_ANA_EN_POS   1
#define OM24G_RX_READY_RESET_AGC_EN_POS  2
#define OM24G_LOOP_SEND_MODE_EN_POS      3
#define OM24G_RX_DONG_DIS_POS            4
#define OM24G_RX_READY_BP_POS            5
#define OM24G_CRC_WHITE_ORDER_POS        6

#define OM24G_ADDR_MISS_RESET_ANA_EN_MASK 0x00000001
#define OM24G_MAX_LEN_RESET_ANA_EN_MASK   0x00000002
#define OM24G_RX_READY_RESET_AGC_EN_MASK  0x00000004
#define OM24G_LOOP_SEND_MODE_EN_MASK      0x00000008
#define OM24G_RX_DONG_DIS_MASK            0x00000010
#define OM24G_RX_READY_BP_MASK            0x00000020
#define OM24G_CRC_WHITE_ORDER_MASK        0x00000040

// SYNC_DET_CNT (110h)
#define OM24G_CNT_THRE_POS                0
#define OM24G_SYNC_DET_CNT_EN_POS         31

#define OM24G_CNT_THRE_MASK               0x0FFFFFFF
#define OM24G_SYNC_DET_CNT_EN_MASK        0x80000000

// SPARK_HEADER_SET (114h)
#define OM24G_SPARK_HEADER_SET_POS        0

#define OM24G_SPARK_HEADER_SET_MASK       0x0001FFFF

// SPARK_CRC_SET (118h)
#define OM24G_SPARK_CRC_SET_CRC_INIT_POS  0
#define OM24G_SPARK_CRC_SET_CRC_POLY_POS  16

#define OM24G_SPARK_CRC_SET_CRC_INIT_MASK 0x00000FFF
#define OM24G_SPARK_CRC_SET_CRC_POLY_MASK 0x0FFF0000

// DBG_SPI0(124h)
#define OM24G_DBG_SPI0_ADDRESS_POS        0
#define OM24G_DBG_SPI0_DATA_POS           8
#define OM24G_DBG_SPI0_ADDRESS_MASK       0x000000FF
#define OM24G_DBG_SPI0_DATA_MASK          0x00FFFF00

// DBG_SPI1(128h)
#define OM24G_DBG_SPI1_START_POS          0
#define OM24G_DBG_SPI1_FREQ_POS           8
#define OM24G_DBG_SPI1_COMP_POS           10
#define OM24G_DBG_SPI1_START_MASK         0x00000001
#define OM24G_DBG_SPI1_FREQ_MASK          0x00000030
#define OM24G_DBG_SPI1_COMP_MASK          0x00000400

#define OM24G_CE_LOW()                                                         \
    do {                                                                       \
        OM_CRITICAL_BEGIN();                                                   \
        OM_24G->MAC_EN = 0;                                                    \
        if((OM_CPM->MAC_24G_CFG & CPM_2P4_CFG_MAC_GATE_EN_MASK) == 0) {        \
            om_error_t ret;                                                    \
            DRV_WAIT_US_UNTIL_TO(!((REGR(&OM_24G->STATE, MASK_POS(OM24G_MAIN_STATE)) == 0x02) || (REGR(&OM_24G->STATE, MASK_POS(OM24G_MAIN_STATE)) == 0x00)), 200, ret);(void) ret; \
        }                                                                      \
        OM_CRITICAL_END();                                                     \
    } while(0)

#define OM24G_CE_HIGH()                                                        \
    do {                                                                       \
        REGW1(&OM_24G->MAC_EN, OM24G_CLK_EN_MASK);                             \
    } while(0)

#define OM24G_FLUSH_TX_FIFO()                                                  \
    do {                                                                       \
        REGW1(&OM_24G->FLUSH, OM24G_TX_FIFO_FLUSH_MASK);                       \
    } while(0)

#define OM24G_FLUSH_RX_FIFO()                                                  \
    do {                                                                       \
        REGW1(&OM_24G->FLUSH, OM24G_RX_FIFO_FLUSH_MASK);                       \
    } while(0)

#define OM24G_ABORT_RF()                                                       \
    do {                                                                       \
        REGW1(&OM_24G->FLUSH, OM24G_ABORT_RF_MASK);                            \
    } while(0)

#define OM24G_CLEAR_ALL_IRQ()                                                  \
    do {                                                                       \
        REGW1(&OM_24G->INT_ST, 0XFFFFFFFF);                                    \
    } while(0)


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    __IO uint32_t PKTCTRL0;                     // offset:0x00
    __IO uint32_t FB_PKTCTRL;                   // offset:0x04
    __IO uint32_t MAC_EN;                       // offset:0x08
    __IO uint32_t DYNPD;                        // offset:0x0C
    __IO uint32_t FEATURE;                      // offset:0x10
    __IO uint32_t INT_MASK;                     // offset:0x14
    __IO uint32_t INT_ST;                       // offset:0x18
    __IO uint32_t INT_ST_RAW;                   // offset:0x1C
    __IO uint32_t EN_AA;                        // offset:0x20
    __IO uint32_t EN_RXADDR;                    // offset:0x24
    __IO uint32_t FA_PKTCTRL;                   // offset:0x28
    __IO uint32_t FA_SETUP_RETR;                // offset:0x2C
    __IO uint32_t FA_OBSERVE_TX;                // offset:0x30
    __IO uint32_t PREAMBLE;                     // offset:0x34
    __IO uint32_t PREAMBLE_LEN;                 // offset:0x38
    __IO uint32_t PRE_GUARD;                    // offset:0x3C
    __IO uint32_t SYNC_WORD0;                   // offset:0x40
    __IO uint32_t SYNC_WORD1;                   // offset:0x44
    __IO uint32_t TX_ADDR;                      // offset:0x48
    __IO uint32_t SYNC_WORD_SEL;                // offset:0x4C
    __IO uint32_t RX_ADDR_P0;                   // offset:0x50
    __IO uint32_t RX_ADDR_P1;                   // offset:0x54
    __IO uint32_t RX_ADDR_P2;                   // offset:0x58
    __IO uint32_t RX_ADDR_P3;                   // offset:0x5C
    __IO uint32_t RX_ADDR_P4;                   // offset:0x60
    __IO uint32_t RX_ADDR_P5;                   // offset:0x64
    __IO uint32_t TAIL_DATA;                    // offset:0x68
    __IO uint32_t FB_HEADER_H;                  // offset:0x6C
    __IO uint32_t RF_DR;                        // offset:0x70
    __IO uint32_t RF_PD_AHEAD;                  // offset:0x74
    __IO uint32_t RX_P_NO;                      // offset:0x78
    __IO uint32_t SETUP_VALUE_H;                // offset:0x7C
    __IO uint32_t CRCCFG;                       // offset:0x80
    __IO uint32_t CRCPOLY;                      // offset:0x84
    __IO uint32_t CRCINIT;                      // offset:0x88
    __IO uint32_t CRCSKIP;                      // offset:0x8C
    __IO uint32_t RXCRC;                        // offset:0x90
    __IO uint32_t WHITECFG;                     // offset:0x94
    __IO uint32_t WHITESEL;                     // offset:0x98
    __IO uint32_t WHITESEED;                    // offset:0x9C
    __IO uint32_t WHITEOBIT;                    // offset:0xA0
    __IO uint32_t DMA_CMD;                      // offset:0xA4
    __IO uint32_t DMA_TX_LEN;                   // offset:0xA8
    __IO uint32_t AGC_ME_MO;                    // offset:0xAC
         uint32_t RESERVED6[2];
    __IO uint32_t DMA_TX_ADDR;                  // offset:0xB8
         uint32_t RESERVED7[2];
    __IO uint32_t DMA_RX_ADDR;                  // offset:0xC4
    __IO uint32_t RX_DYN_LEN;                   // offset:0xC8
    __IO uint32_t PACKET_LEN;                   // offset:0xCC
    __IO uint32_t BCC;                          // offset:0xD0
    __IO uint32_t TIMESTAMP_RT;                 // offset:0xD4
    __I  uint32_t TIMESTAMP_TRIGER;             // offset:0xD8
    __IO uint32_t TIMESTAMP_CFG;                // offset:0xDC
    __IO uint32_t SETUP_VALUE;                  // offset:0xE0
    __IO uint32_t ENDIAN;                       // offset:0xE4
    __IO uint32_t FLUSH;                        // offset:0xE8
    __IO uint32_t TESTCTRL;                     // offset:0xEC
    __I  uint32_t STATE;                        // offset:0xF0
    __IO uint32_t FB_HEADER;                    // offset:0xF4
    __IO uint32_t PMU_CNT_32K;                  // offset:0xF8
    __IO uint32_t PMU_CNT_4M;                   // offset:0xFC
    __IO uint32_t ACK_MODE;                     // offset:0x100
    __IO uint32_t RX_DONE;                      // offset:0x104
    __IO uint32_t DEBUG_BUS;                    // offset:0x108
    __IO uint32_t RX_CTRL;                      // offset:0x10C
    __IO uint32_t SYNC_DET_CNT;                 // offset:0x110
    __IO uint32_t SPARK_HEADER_SET;             // offset:0x114
    __IO uint32_t SPARK_CRC_SET;                // offset:0x118
         uint32_t RESERVED9[1];
    __IO uint32_t FPGA_TX_INDEX;                // offset:0x120
    __IO uint32_t DBG_SPI0;                     // offset:0x124
    __IO uint32_t DBG_SPI1;                     // offset:0x128
         uint32_t RESERVED10;
    __IO uint32_t TMR0_NXT_32K_TP;              // offset:0x130
    __IO uint32_t TMR0_NXT_8M_TP;               // offset:0x134
    __IO uint32_t TMR0_TRIG_PREFETCH;           // offset:0x138
         uint32_t RESERVED11;
    __IO uint32_t TMR1_NXT_32K_TP;              // offset:0x140
    __IO uint32_t TMR1_NXT_8M_TP;               // offset:0x144
    __IO uint32_t TMR1_TRIG_PREFETCH;           // offset:0x148
         uint32_t RESERVED12;
    __IO uint32_t TMR_NATIVE_CNT_32K;           // offset:0x150
    __IO uint32_t TMR_NATIVE_CNT_8M;            // offset:0x154
    __IO uint32_t TMR_SYNC_CNT_32K_LOCK;        // offset:0x158
    __IO uint32_t TMR_SYNC_CNT_8M_LOCK;         // offset:0x15C
} OM_24G_Type;


#ifdef __cplusplus
}
#endif


#endif  /* __OM24G_REG_H */


/** @} */
