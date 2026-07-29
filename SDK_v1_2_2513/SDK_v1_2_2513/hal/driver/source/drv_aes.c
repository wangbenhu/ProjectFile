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
 * @brief    AES driver source file
 * @details  AES driver source file.
 *          ECB Encrypt: implemented by hardware when keybits=128/256, otherwise implemented by software.
 *          ECB Decrypt: implemented by software
 *          CBC Encrypt: implemented by hardware when keybits=128/256, otherwise implemented by software.
 *          CBC Decrypt: implemented by software
 *          CFB Encrypt: implemented by hardware when keybits=128/256, otherwise implemented by software.
 *          CFB Decrypt: implemented by hardware when keybits=128/256, otherwise implemented by software.
 *          OFB Encrypt: implemented by hardware when keybits=128/256, otherwise implemented by software.
 *          OFB Decrypt: implemented by hardware when keybits=128/256, otherwise implemented by software.
 *          CTR Encrypt: implemented by hardware when keybits=128/256, otherwise implemented by software.
 *          CTR Decrypt: implemented by hardware when keybits=128/256, otherwise implemented by software.
 *          GCM Encrypt: implemented by hardware when keybits=128/256, otherwise implemented by software.
 *          GCM Decrypt: implemented by hardware when keybits=128/256, otherwise implemented by software.
 *          CMAC: implemented by hardware when keybits=128/256, otherwise implemented by software.
 *          LE CCM: implemented by hardware.
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
#if (RTE_AES)
#include <stddef.h>
#include "om_driver.h"


/*******************************************************************************
 * MACROS
 */
#define DRV_AES_DEFINE(AESn, aesn)                                  \
static aes_env_t aesn##_env = {                                     \
    .aes_id = (uint32_t)OM_##AESn,                                  \
    .cap    = CAP_##AESn,                                           \
    .state  = DRV_STATE_STOP,                                       \
}

#define AES_HW_CLK_ENABLE(enable)                                              \
    do {                                                                       \
        if (enable) {                                                          \
            if (OM_CPM->BLE_CFG & CPM_BLE_CFG_BLE_AHB_GATE_EN_MASK) {          \
                OM_CPM->AES_CFG = COM_AES_CFG_CLK_SEL_MASK;                    \
            } else {                                                           \
                AES_HW_CLOCK_EN_WHEN_USING_BLE_REG = 1;                        \
            }                                                                  \
        } else {                                                               \
            if (OM_CPM->BLE_CFG & CPM_BLE_CFG_BLE_AHB_GATE_EN_MASK){           \
                OM_CPM->AES_CFG = CPM_AES_CFG_CLK_EN_MASK;                     \
            } else {                                                           \
                AES_HW_CLOCK_EN_WHEN_USING_BLE_REG = 0;                        \
            }                                                                  \
        }                                                                      \
    } while (0)

#if (!RTE_AES_KEEP_RAW_ENGINE)
#define AES_RT0(idx) RT0[idx]
#define AES_RT1(idx) RT1[idx]
#define AES_RT2(idx) RT2[idx]
#define AES_RT3(idx) RT3[idx]

#define AES_FT0(idx) FT0[idx]
#define AES_FT1(idx) FT1[idx]
#define AES_FT2(idx) FT2[idx]
#define AES_FT3(idx) FT3[idx]

#define AES_FROUND(X0, X1, X2, X3, Y0, Y1, Y2, Y3)                  \
    do                                                              \
    {                                                               \
        (X0) = *RK++ ^ AES_FT0(DRV_BYTE_0(Y0)) ^                    \
               AES_FT1(DRV_BYTE_1(Y1)) ^                            \
               AES_FT2(DRV_BYTE_2(Y2)) ^                            \
               AES_FT3(DRV_BYTE_3(Y3));                             \
                                                                    \
        (X1) = *RK++ ^ AES_FT0(DRV_BYTE_0(Y1)) ^                    \
               AES_FT1(DRV_BYTE_1(Y2)) ^                            \
               AES_FT2(DRV_BYTE_2(Y3)) ^                            \
               AES_FT3(DRV_BYTE_3(Y0));                             \
                                                                    \
        (X2) = *RK++ ^ AES_FT0(DRV_BYTE_0(Y2)) ^                    \
               AES_FT1(DRV_BYTE_1(Y3)) ^                            \
               AES_FT2(DRV_BYTE_2(Y0)) ^                            \
               AES_FT3(DRV_BYTE_3(Y1));                             \
                                                                    \
        (X3) = *RK++ ^ AES_FT0(DRV_BYTE_0(Y3)) ^                    \
               AES_FT1(DRV_BYTE_1(Y0)) ^                            \
               AES_FT2(DRV_BYTE_2(Y1)) ^                            \
               AES_FT3(DRV_BYTE_3(Y2));                             \
    } while (0)

#define AES_RROUND(X0, X1, X2, X3, Y0, Y1, Y2, Y3)                  \
    do                                                              \
    {                                                               \
        (X0) = *RK++ ^ AES_RT0(DRV_BYTE_0(Y0)) ^                    \
               AES_RT1(DRV_BYTE_1(Y3)) ^                            \
               AES_RT2(DRV_BYTE_2(Y2)) ^                            \
               AES_RT3(DRV_BYTE_3(Y1));                             \
                                                                    \
        (X1) = *RK++ ^ AES_RT0(DRV_BYTE_0(Y1)) ^                    \
               AES_RT1(DRV_BYTE_1(Y0)) ^                            \
               AES_RT2(DRV_BYTE_2(Y3)) ^                            \
               AES_RT3(DRV_BYTE_3(Y2));                             \
                                                                    \
        (X2) = *RK++ ^ AES_RT0(DRV_BYTE_0(Y2)) ^                    \
               AES_RT1(DRV_BYTE_1(Y1)) ^                            \
               AES_RT2(DRV_BYTE_2(Y0)) ^                            \
               AES_RT3(DRV_BYTE_3(Y3));                             \
                                                                    \
        (X3) = *RK++ ^ AES_RT0(DRV_BYTE_0(Y3)) ^                    \
               AES_RT1(DRV_BYTE_1(Y2)) ^                            \
               AES_RT2(DRV_BYTE_2(Y1)) ^                            \
               AES_RT3(DRV_BYTE_3(Y0));                             \
    } while (0)

/*
 * Forward tables
 */
#define FT \
\
    V(A5, 63, 63, C6), V(84, 7C, 7C, F8), V(99, 77, 77, EE), V(8D, 7B, 7B, F6), \
    V(0D, F2, F2, FF), V(BD, 6B, 6B, D6), V(B1, 6F, 6F, DE), V(54, C5, C5, 91), \
    V(50, 30, 30, 60), V(03, 01, 01, 02), V(A9, 67, 67, CE), V(7D, 2B, 2B, 56), \
    V(19, FE, FE, E7), V(62, D7, D7, B5), V(E6, AB, AB, 4D), V(9A, 76, 76, EC), \
    V(45, CA, CA, 8F), V(9D, 82, 82, 1F), V(40, C9, C9, 89), V(87, 7D, 7D, FA), \
    V(15, FA, FA, EF), V(EB, 59, 59, B2), V(C9, 47, 47, 8E), V(0B, F0, F0, FB), \
    V(EC, AD, AD, 41), V(67, D4, D4, B3), V(FD, A2, A2, 5F), V(EA, AF, AF, 45), \
    V(BF, 9C, 9C, 23), V(F7, A4, A4, 53), V(96, 72, 72, E4), V(5B, C0, C0, 9B), \
    V(C2, B7, B7, 75), V(1C, FD, FD, E1), V(AE, 93, 93, 3D), V(6A, 26, 26, 4C), \
    V(5A, 36, 36, 6C), V(41, 3F, 3F, 7E), V(02, F7, F7, F5), V(4F, CC, CC, 83), \
    V(5C, 34, 34, 68), V(F4, A5, A5, 51), V(34, E5, E5, D1), V(08, F1, F1, F9), \
    V(93, 71, 71, E2), V(73, D8, D8, AB), V(53, 31, 31, 62), V(3F, 15, 15, 2A), \
    V(0C, 04, 04, 08), V(52, C7, C7, 95), V(65, 23, 23, 46), V(5E, C3, C3, 9D), \
    V(28, 18, 18, 30), V(A1, 96, 96, 37), V(0F, 05, 05, 0A), V(B5, 9A, 9A, 2F), \
    V(09, 07, 07, 0E), V(36, 12, 12, 24), V(9B, 80, 80, 1B), V(3D, E2, E2, DF), \
    V(26, EB, EB, CD), V(69, 27, 27, 4E), V(CD, B2, B2, 7F), V(9F, 75, 75, EA), \
    V(1B, 09, 09, 12), V(9E, 83, 83, 1D), V(74, 2C, 2C, 58), V(2E, 1A, 1A, 34), \
    V(2D, 1B, 1B, 36), V(B2, 6E, 6E, DC), V(EE, 5A, 5A, B4), V(FB, A0, A0, 5B), \
    V(F6, 52, 52, A4), V(4D, 3B, 3B, 76), V(61, D6, D6, B7), V(CE, B3, B3, 7D), \
    V(7B, 29, 29, 52), V(3E, E3, E3, DD), V(71, 2F, 2F, 5E), V(97, 84, 84, 13), \
    V(F5, 53, 53, A6), V(68, D1, D1, B9), V(00, 00, 00, 00), V(2C, ED, ED, C1), \
    V(60, 20, 20, 40), V(1F, FC, FC, E3), V(C8, B1, B1, 79), V(ED, 5B, 5B, B6), \
    V(BE, 6A, 6A, D4), V(46, CB, CB, 8D), V(D9, BE, BE, 67), V(4B, 39, 39, 72), \
    V(DE, 4A, 4A, 94), V(D4, 4C, 4C, 98), V(E8, 58, 58, B0), V(4A, CF, CF, 85), \
    V(6B, D0, D0, BB), V(2A, EF, EF, C5), V(E5, AA, AA, 4F), V(16, FB, FB, ED), \
    V(C5, 43, 43, 86), V(D7, 4D, 4D, 9A), V(55, 33, 33, 66), V(94, 85, 85, 11), \
    V(CF, 45, 45, 8A), V(10, F9, F9, E9), V(06, 02, 02, 04), V(81, 7F, 7F, FE), \
    V(F0, 50, 50, A0), V(44, 3C, 3C, 78), V(BA, 9F, 9F, 25), V(E3, A8, A8, 4B), \
    V(F3, 51, 51, A2), V(FE, A3, A3, 5D), V(C0, 40, 40, 80), V(8A, 8F, 8F, 05), \
    V(AD, 92, 92, 3F), V(BC, 9D, 9D, 21), V(48, 38, 38, 70), V(04, F5, F5, F1), \
    V(DF, BC, BC, 63), V(C1, B6, B6, 77), V(75, DA, DA, AF), V(63, 21, 21, 42), \
    V(30, 10, 10, 20), V(1A, FF, FF, E5), V(0E, F3, F3, FD), V(6D, D2, D2, BF), \
    V(4C, CD, CD, 81), V(14, 0C, 0C, 18), V(35, 13, 13, 26), V(2F, EC, EC, C3), \
    V(E1, 5F, 5F, BE), V(A2, 97, 97, 35), V(CC, 44, 44, 88), V(39, 17, 17, 2E), \
    V(57, C4, C4, 93), V(F2, A7, A7, 55), V(82, 7E, 7E, FC), V(47, 3D, 3D, 7A), \
    V(AC, 64, 64, C8), V(E7, 5D, 5D, BA), V(2B, 19, 19, 32), V(95, 73, 73, E6), \
    V(A0, 60, 60, C0), V(98, 81, 81, 19), V(D1, 4F, 4F, 9E), V(7F, DC, DC, A3), \
    V(66, 22, 22, 44), V(7E, 2A, 2A, 54), V(AB, 90, 90, 3B), V(83, 88, 88, 0B), \
    V(CA, 46, 46, 8C), V(29, EE, EE, C7), V(D3, B8, B8, 6B), V(3C, 14, 14, 28), \
    V(79, DE, DE, A7), V(E2, 5E, 5E, BC), V(1D, 0B, 0B, 16), V(76, DB, DB, AD), \
    V(3B, E0, E0, DB), V(56, 32, 32, 64), V(4E, 3A, 3A, 74), V(1E, 0A, 0A, 14), \
    V(DB, 49, 49, 92), V(0A, 06, 06, 0C), V(6C, 24, 24, 48), V(E4, 5C, 5C, B8), \
    V(5D, C2, C2, 9F), V(6E, D3, D3, BD), V(EF, AC, AC, 43), V(A6, 62, 62, C4), \
    V(A8, 91, 91, 39), V(A4, 95, 95, 31), V(37, E4, E4, D3), V(8B, 79, 79, F2), \
    V(32, E7, E7, D5), V(43, C8, C8, 8B), V(59, 37, 37, 6E), V(B7, 6D, 6D, DA), \
    V(8C, 8D, 8D, 01), V(64, D5, D5, B1), V(D2, 4E, 4E, 9C), V(E0, A9, A9, 49), \
    V(B4, 6C, 6C, D8), V(FA, 56, 56, AC), V(07, F4, F4, F3), V(25, EA, EA, CF), \
    V(AF, 65, 65, CA), V(8E, 7A, 7A, F4), V(E9, AE, AE, 47), V(18, 08, 08, 10), \
    V(D5, BA, BA, 6F), V(88, 78, 78, F0), V(6F, 25, 25, 4A), V(72, 2E, 2E, 5C), \
    V(24, 1C, 1C, 38), V(F1, A6, A6, 57), V(C7, B4, B4, 73), V(51, C6, C6, 97), \
    V(23, E8, E8, CB), V(7C, DD, DD, A1), V(9C, 74, 74, E8), V(21, 1F, 1F, 3E), \
    V(DD, 4B, 4B, 96), V(DC, BD, BD, 61), V(86, 8B, 8B, 0D), V(85, 8A, 8A, 0F), \
    V(90, 70, 70, E0), V(42, 3E, 3E, 7C), V(C4, B5, B5, 71), V(AA, 66, 66, CC), \
    V(D8, 48, 48, 90), V(05, 03, 03, 06), V(01, F6, F6, F7), V(12, 0E, 0E, 1C), \
    V(A3, 61, 61, C2), V(5F, 35, 35, 6A), V(F9, 57, 57, AE), V(D0, B9, B9, 69), \
    V(91, 86, 86, 17), V(58, C1, C1, 99), V(27, 1D, 1D, 3A), V(B9, 9E, 9E, 27), \
    V(38, E1, E1, D9), V(13, F8, F8, EB), V(B3, 98, 98, 2B), V(33, 11, 11, 22), \
    V(BB, 69, 69, D2), V(70, D9, D9, A9), V(89, 8E, 8E, 07), V(A7, 94, 94, 33), \
    V(B6, 9B, 9B, 2D), V(22, 1E, 1E, 3C), V(92, 87, 87, 15), V(20, E9, E9, C9), \
    V(49, CE, CE, 87), V(FF, 55, 55, AA), V(78, 28, 28, 50), V(7A, DF, DF, A5), \
    V(8F, 8C, 8C, 03), V(F8, A1, A1, 59), V(80, 89, 89, 09), V(17, 0D, 0D, 1A), \
    V(DA, BF, BF, 65), V(31, E6, E6, D7), V(C6, 42, 42, 84), V(B8, 68, 68, D0), \
    V(C3, 41, 41, 82), V(B0, 99, 99, 29), V(77, 2D, 2D, 5A), V(11, 0F, 0F, 1E), \
    V(CB, B0, B0, 7B), V(FC, 54, 54, A8), V(D6, BB, BB, 6D), V(3A, 16, 16, 2C)

#define V(a, b, c, d) 0x##a##b##c##d
static const uint32_t FT0[256] = { FT };
#undef V

#define V(a, b, c, d) 0x##b##c##d##a
static const uint32_t FT1[256] = { FT };
#undef V

#define V(a, b, c, d) 0x##c##d##a##b
static const uint32_t FT2[256] = { FT };
#undef V

#define V(a, b, c, d) 0x##d##a##b##c
static const uint32_t FT3[256] = { FT };
#undef V

#undef FT

/*
 * Reverse tables
 */
#define RT \
\
    V(50, A7, F4, 51), V(53, 65, 41, 7E), V(C3, A4, 17, 1A), V(96, 5E, 27, 3A), \
    V(CB, 6B, AB, 3B), V(F1, 45, 9D, 1F), V(AB, 58, FA, AC), V(93, 03, E3, 4B), \
    V(55, FA, 30, 20), V(F6, 6D, 76, AD), V(91, 76, CC, 88), V(25, 4C, 02, F5), \
    V(FC, D7, E5, 4F), V(D7, CB, 2A, C5), V(80, 44, 35, 26), V(8F, A3, 62, B5), \
    V(49, 5A, B1, DE), V(67, 1B, BA, 25), V(98, 0E, EA, 45), V(E1, C0, FE, 5D), \
    V(02, 75, 2F, C3), V(12, F0, 4C, 81), V(A3, 97, 46, 8D), V(C6, F9, D3, 6B), \
    V(E7, 5F, 8F, 03), V(95, 9C, 92, 15), V(EB, 7A, 6D, BF), V(DA, 59, 52, 95), \
    V(2D, 83, BE, D4), V(D3, 21, 74, 58), V(29, 69, E0, 49), V(44, C8, C9, 8E), \
    V(6A, 89, C2, 75), V(78, 79, 8E, F4), V(6B, 3E, 58, 99), V(DD, 71, B9, 27), \
    V(B6, 4F, E1, BE), V(17, AD, 88, F0), V(66, AC, 20, C9), V(B4, 3A, CE, 7D), \
    V(18, 4A, DF, 63), V(82, 31, 1A, E5), V(60, 33, 51, 97), V(45, 7F, 53, 62), \
    V(E0, 77, 64, B1), V(84, AE, 6B, BB), V(1C, A0, 81, FE), V(94, 2B, 08, F9), \
    V(58, 68, 48, 70), V(19, FD, 45, 8F), V(87, 6C, DE, 94), V(B7, F8, 7B, 52), \
    V(23, D3, 73, AB), V(E2, 02, 4B, 72), V(57, 8F, 1F, E3), V(2A, AB, 55, 66), \
    V(07, 28, EB, B2), V(03, C2, B5, 2F), V(9A, 7B, C5, 86), V(A5, 08, 37, D3), \
    V(F2, 87, 28, 30), V(B2, A5, BF, 23), V(BA, 6A, 03, 02), V(5C, 82, 16, ED), \
    V(2B, 1C, CF, 8A), V(92, B4, 79, A7), V(F0, F2, 07, F3), V(A1, E2, 69, 4E), \
    V(CD, F4, DA, 65), V(D5, BE, 05, 06), V(1F, 62, 34, D1), V(8A, FE, A6, C4), \
    V(9D, 53, 2E, 34), V(A0, 55, F3, A2), V(32, E1, 8A, 05), V(75, EB, F6, A4), \
    V(39, EC, 83, 0B), V(AA, EF, 60, 40), V(06, 9F, 71, 5E), V(51, 10, 6E, BD), \
    V(F9, 8A, 21, 3E), V(3D, 06, DD, 96), V(AE, 05, 3E, DD), V(46, BD, E6, 4D), \
    V(B5, 8D, 54, 91), V(05, 5D, C4, 71), V(6F, D4, 06, 04), V(FF, 15, 50, 60), \
    V(24, FB, 98, 19), V(97, E9, BD, D6), V(CC, 43, 40, 89), V(77, 9E, D9, 67), \
    V(BD, 42, E8, B0), V(88, 8B, 89, 07), V(38, 5B, 19, E7), V(DB, EE, C8, 79), \
    V(47, 0A, 7C, A1), V(E9, 0F, 42, 7C), V(C9, 1E, 84, F8), V(00, 00, 00, 00), \
    V(83, 86, 80, 09), V(48, ED, 2B, 32), V(AC, 70, 11, 1E), V(4E, 72, 5A, 6C), \
    V(FB, FF, 0E, FD), V(56, 38, 85, 0F), V(1E, D5, AE, 3D), V(27, 39, 2D, 36), \
    V(64, D9, 0F, 0A), V(21, A6, 5C, 68), V(D1, 54, 5B, 9B), V(3A, 2E, 36, 24), \
    V(B1, 67, 0A, 0C), V(0F, E7, 57, 93), V(D2, 96, EE, B4), V(9E, 91, 9B, 1B), \
    V(4F, C5, C0, 80), V(A2, 20, DC, 61), V(69, 4B, 77, 5A), V(16, 1A, 12, 1C), \
    V(0A, BA, 93, E2), V(E5, 2A, A0, C0), V(43, E0, 22, 3C), V(1D, 17, 1B, 12), \
    V(0B, 0D, 09, 0E), V(AD, C7, 8B, F2), V(B9, A8, B6, 2D), V(C8, A9, 1E, 14), \
    V(85, 19, F1, 57), V(4C, 07, 75, AF), V(BB, DD, 99, EE), V(FD, 60, 7F, A3), \
    V(9F, 26, 01, F7), V(BC, F5, 72, 5C), V(C5, 3B, 66, 44), V(34, 7E, FB, 5B), \
    V(76, 29, 43, 8B), V(DC, C6, 23, CB), V(68, FC, ED, B6), V(63, F1, E4, B8), \
    V(CA, DC, 31, D7), V(10, 85, 63, 42), V(40, 22, 97, 13), V(20, 11, C6, 84), \
    V(7D, 24, 4A, 85), V(F8, 3D, BB, D2), V(11, 32, F9, AE), V(6D, A1, 29, C7), \
    V(4B, 2F, 9E, 1D), V(F3, 30, B2, DC), V(EC, 52, 86, 0D), V(D0, E3, C1, 77), \
    V(6C, 16, B3, 2B), V(99, B9, 70, A9), V(FA, 48, 94, 11), V(22, 64, E9, 47), \
    V(C4, 8C, FC, A8), V(1A, 3F, F0, A0), V(D8, 2C, 7D, 56), V(EF, 90, 33, 22), \
    V(C7, 4E, 49, 87), V(C1, D1, 38, D9), V(FE, A2, CA, 8C), V(36, 0B, D4, 98), \
    V(CF, 81, F5, A6), V(28, DE, 7A, A5), V(26, 8E, B7, DA), V(A4, BF, AD, 3F), \
    V(E4, 9D, 3A, 2C), V(0D, 92, 78, 50), V(9B, CC, 5F, 6A), V(62, 46, 7E, 54), \
    V(C2, 13, 8D, F6), V(E8, B8, D8, 90), V(5E, F7, 39, 2E), V(F5, AF, C3, 82), \
    V(BE, 80, 5D, 9F), V(7C, 93, D0, 69), V(A9, 2D, D5, 6F), V(B3, 12, 25, CF), \
    V(3B, 99, AC, C8), V(A7, 7D, 18, 10), V(6E, 63, 9C, E8), V(7B, BB, 3B, DB), \
    V(09, 78, 26, CD), V(F4, 18, 59, 6E), V(01, B7, 9A, EC), V(A8, 9A, 4F, 83), \
    V(65, 6E, 95, E6), V(7E, E6, FF, AA), V(08, CF, BC, 21), V(E6, E8, 15, EF), \
    V(D9, 9B, E7, BA), V(CE, 36, 6F, 4A), V(D4, 09, 9F, EA), V(D6, 7C, B0, 29), \
    V(AF, B2, A4, 31), V(31, 23, 3F, 2A), V(30, 94, A5, C6), V(C0, 66, A2, 35), \
    V(37, BC, 4E, 74), V(A6, CA, 82, FC), V(B0, D0, 90, E0), V(15, D8, A7, 33), \
    V(4A, 98, 04, F1), V(F7, DA, EC, 41), V(0E, 50, CD, 7F), V(2F, F6, 91, 17), \
    V(8D, D6, 4D, 76), V(4D, B0, EF, 43), V(54, 4D, AA, CC), V(DF, 04, 96, E4), \
    V(E3, B5, D1, 9E), V(1B, 88, 6A, 4C), V(B8, 1F, 2C, C1), V(7F, 51, 65, 46), \
    V(04, EA, 5E, 9D), V(5D, 35, 8C, 01), V(73, 74, 87, FA), V(2E, 41, 0B, FB), \
    V(5A, 1D, 67, B3), V(52, D2, DB, 92), V(33, 56, 10, E9), V(13, 47, D6, 6D), \
    V(8C, 61, D7, 9A), V(7A, 0C, A1, 37), V(8E, 14, F8, 59), V(89, 3C, 13, EB), \
    V(EE, 27, A9, CE), V(35, C9, 61, B7), V(ED, E5, 1C, E1), V(3C, B1, 47, 7A), \
    V(59, DF, D2, 9C), V(3F, 73, F2, 55), V(79, CE, 14, 18), V(BF, 37, C7, 73), \
    V(EA, CD, F7, 53), V(5B, AA, FD, 5F), V(14, 6F, 3D, DF), V(86, DB, 44, 78), \
    V(81, F3, AF, CA), V(3E, C4, 68, B9), V(2C, 34, 24, 38), V(5F, 40, A3, C2), \
    V(72, C3, 1D, 16), V(0C, 25, E2, BC), V(8B, 49, 3C, 28), V(41, 95, 0D, FF), \
    V(71, 01, A8, 39), V(DE, B3, 0C, 08), V(9C, E4, B4, D8), V(90, C1, 56, 64), \
    V(61, 84, CB, 7B), V(70, B6, 32, D5), V(74, 5C, 6C, 48), V(42, 57, B8, D0)


#define V(a, b, c, d) 0x##a##b##c##d
static const uint32_t RT0[256] = { RT };
#undef V

#define V(a, b, c, d) 0x##b##c##d##a
static const uint32_t RT1[256] = { RT };
#undef V

#define V(a, b, c, d) 0x##c##d##a##b
static const uint32_t RT2[256] = { RT };
#undef V

#define V(a, b, c, d) 0x##d##a##b##c
static const uint32_t RT3[256] = { RT };
#undef V

#undef RT


__STATIC_INLINE uint32_t drv_get_unaligned_uint32(const void *p)
{
    uint32_t r;
    memcpy(&r, p, sizeof(r));

    return r;
}

__STATIC_INLINE void drv_put_unaligned_uint32(void *p, uint32_t x)
{
    memcpy(p, &x, sizeof(x));
}

/**
 * Get the unsigned 32 bits integer corresponding to four bytes in
 * little-endian order (LSB first).
 *
 * \param   data    Base address of the memory to get the four bytes from.
 * \param   offset  Offset from \p data of the first and least significant
 *                  byte of the four bytes to build the 32 bits unsigned
 *                  integer from.
 */
#define DRV_GET_UINT32_LE(data, offset)                                \
        (drv_get_unaligned_uint32((data) + (offset)))                  \

/**
 * Put in memory a 32 bits unsigned integer in little-endian order.
 *
 * \param   n       32 bits unsigned integer to put in memory.
 * \param   data    Base address of the memory where to put the 32
 *                  bits unsigned integer in.
 * \param   offset  Offset from \p data where to put the least significant
 *                  byte of the 32 bits unsigned integer \p n.
 */
#define DRV_PUT_UINT32_LE(n, data, offset)                             \
    drv_put_unaligned_uint32((data) + (offset), ((uint32_t) (n)));     \

/**
 * Get the unsigned 32 bits integer corresponding to four bytes in
 * big-endian order (MSB first).
 *
 * \param   data    Base address of the memory to get the four bytes from.
 * \param   offset  Offset from \p data of the first and most significant
 *                  byte of the four bytes to build the 32 bits unsigned
 *                  integer from.
 */
#define DRV_GET_UINT32_BE(data, offset)                            \
    OM_BSWAP32(drv_get_unaligned_uint32((data) + (offset)))            \

/**
 * Put in memory a 32 bits unsigned integer in big-endian order.
 *
 * \param   n       32 bits unsigned integer to put in memory.
 * \param   data    Base address of the memory where to put the 32
 *                  bits unsigned integer in.
 * \param   offset  Offset from \p data where to put the most significant
 *                  byte of the 32 bits unsigned integer \p n.
 */
#define DRV_PUT_UINT32_BE(n, data, offset)                                 \
    drv_put_unaligned_uint32((data) + (offset), OM_BSWAP32((uint32_t) (n)));   \

/** Byte Reading Macros
 *
 * Given a multi-byte integer \p x, OM_BYTE_n retrieves the n-th
 * byte from x, where byte 0 is the least significant byte.
 */
#define DRV_BYTE_0(x) ((uint8_t) ((x)         & 0xff))
#define DRV_BYTE_1(x) ((uint8_t) (((x) >>  8) & 0xff))
#define DRV_BYTE_2(x) ((uint8_t) (((x) >> 16) & 0xff))
#define DRV_BYTE_3(x) ((uint8_t) (((x) >> 24) & 0xff))
#define DRV_BYTE_4(x) ((uint8_t) (((x) >> 32) & 0xff))
#define DRV_BYTE_5(x) ((uint8_t) (((x) >> 40) & 0xff))
#define DRV_BYTE_6(x) ((uint8_t) (((x) >> 48) & 0xff))
#define DRV_BYTE_7(x) ((uint8_t) (((x) >> 56) & 0xff))


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    int nr;                     /*!< The number of rounds. */
    size_t rk_offset;           /*!< The offset in array elements to AES
                                                    round keys in the buffer. */
    uint32_t buf[68];           /*!< Unaligned data buffer. This buffer can
                                   hold 32 extra Bytes, which can be used for
                                   one of the following purposes:
                                   <ul><li>Alignment if VIA padlock is
                                   used.</li>
                                   <li>Simplifying key expansion in the 256-bit
                                   case by generating an extra round key.
                                   </li></ul> */
} aes_sw_context_t;

typedef struct {
    uint8_t iv[16];
} cbc_ctx_t;

typedef struct {
    uint32_t iv_off;
    uint8_t iv[16];
} cfb_ctx_t;

typedef struct {
    uint32_t iv_off;
    uint8_t iv[16];
} ofb_ctx_t;

typedef struct {
    uint32_t nc_off;
    uint8_t nonce_counter[16];
    uint8_t stream_block[16];
} ctr_ctx_t;

typedef struct {
    gcm_operation_t gcm_op;
    uint64_t len;               /* cipher data length processed so far */
    uint64_t add_len;           /* total add data length */
    uint64_t HL[16];            /* precalculated lo-half HTable */
    uint64_t HH[16];            /* precalculated hi-half HTable */
    uint8_t base_ectr[16];      /* first counter-mode cipher output for tag */
    uint8_t y[16];              /* the current cipher-input IV|Counter value */
    uint8_t buf[16];            /* buf working value */
} gcm_ctx_t;

typedef struct {
    cbc_ctx_t rsvd; // reserved for cbc ctx used in cmac
    uint8_t unprocessd_data[AES_BLOCK_SIZE];
    uint32_t unprocessed_len;
} cmac_ctx_t;

typedef struct {
    uint32_t        aes_id;
    uint32_t        cap;
    drv_state_t     state;
    drv_aes_mode_t      mode;
    uint32_t        keybits;
    aes_operation_t operation;
    aes_sw_context_t sw_ctx;
    union {
        cbc_ctx_t cbc_ctx;
        cfb_ctx_t cfb_ctx;
        ofb_ctx_t ofb_ctx;
        ctr_ctx_t ctr_ctx;
        gcm_ctx_t gcm_ctx;
        cmac_ctx_t cmac_ctx;
    } mode_ctx;
} aes_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
DRV_AES_DEFINE(AES0, aes0);

/*
 * Forward S-box
 */
static const unsigned char FSb[256] = {
    0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5,
    0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
    0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0,
    0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
    0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC,
    0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
    0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A,
    0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
    0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0,
    0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
    0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B,
    0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
    0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85,
    0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
    0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5,
    0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
    0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17,
    0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
    0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88,
    0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
    0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C,
    0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
    0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9,
    0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
    0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6,
    0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
    0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E,
    0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
    0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94,
    0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68,
    0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
};

/*
 * Reverse S-box
 */
static const unsigned char RSb[256] = {
    0x52, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38,
    0xBF, 0x40, 0xA3, 0x9E, 0x81, 0xF3, 0xD7, 0xFB,
    0x7C, 0xE3, 0x39, 0x82, 0x9B, 0x2F, 0xFF, 0x87,
    0x34, 0x8E, 0x43, 0x44, 0xC4, 0xDE, 0xE9, 0xCB,
    0x54, 0x7B, 0x94, 0x32, 0xA6, 0xC2, 0x23, 0x3D,
    0xEE, 0x4C, 0x95, 0x0B, 0x42, 0xFA, 0xC3, 0x4E,
    0x08, 0x2E, 0xA1, 0x66, 0x28, 0xD9, 0x24, 0xB2,
    0x76, 0x5B, 0xA2, 0x49, 0x6D, 0x8B, 0xD1, 0x25,
    0x72, 0xF8, 0xF6, 0x64, 0x86, 0x68, 0x98, 0x16,
    0xD4, 0xA4, 0x5C, 0xCC, 0x5D, 0x65, 0xB6, 0x92,
    0x6C, 0x70, 0x48, 0x50, 0xFD, 0xED, 0xB9, 0xDA,
    0x5E, 0x15, 0x46, 0x57, 0xA7, 0x8D, 0x9D, 0x84,
    0x90, 0xD8, 0xAB, 0x00, 0x8C, 0xBC, 0xD3, 0x0A,
    0xF7, 0xE4, 0x58, 0x05, 0xB8, 0xB3, 0x45, 0x06,
    0xD0, 0x2C, 0x1E, 0x8F, 0xCA, 0x3F, 0x0F, 0x02,
    0xC1, 0xAF, 0xBD, 0x03, 0x01, 0x13, 0x8A, 0x6B,
    0x3A, 0x91, 0x11, 0x41, 0x4F, 0x67, 0xDC, 0xEA,
    0x97, 0xF2, 0xCF, 0xCE, 0xF0, 0xB4, 0xE6, 0x73,
    0x96, 0xAC, 0x74, 0x22, 0xE7, 0xAD, 0x35, 0x85,
    0xE2, 0xF9, 0x37, 0xE8, 0x1C, 0x75, 0xDF, 0x6E,
    0x47, 0xF1, 0x1A, 0x71, 0x1D, 0x29, 0xC5, 0x89,
    0x6F, 0xB7, 0x62, 0x0E, 0xAA, 0x18, 0xBE, 0x1B,
    0xFC, 0x56, 0x3E, 0x4B, 0xC6, 0xD2, 0x79, 0x20,
    0x9A, 0xDB, 0xC0, 0xFE, 0x78, 0xCD, 0x5A, 0xF4,
    0x1F, 0xDD, 0xA8, 0x33, 0x88, 0x07, 0xC7, 0x31,
    0xB1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xEC, 0x5F,
    0x60, 0x51, 0x7F, 0xA9, 0x19, 0xB5, 0x4A, 0x0D,
    0x2D, 0xE5, 0x7A, 0x9F, 0x93, 0xC9, 0x9C, 0xEF,
    0xA0, 0xE0, 0x3B, 0x4D, 0xAE, 0x2A, 0xF5, 0xB0,
    0xC8, 0xEB, 0xBB, 0x3C, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2B, 0x04, 0x7E, 0xBA, 0x77, 0xD6, 0x26,
    0xE1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0C, 0x7D
};

/*
 * Round constants
 */
static const uint32_t round_constants[10] = {
    0x00000001, 0x00000002, 0x00000004, 0x00000008,
    0x00000010, 0x00000020, 0x00000040, 0x00000080,
    0x0000001B, 0x00000036
};

/*
 *  This 16-entry table of pre-computed constants is used by the
 *  GHASH multiplier to improve over a strictly table-free but
 *  significantly slower 128x128 bit multiple within GF(2^128).
 */
static const uint64_t last4[16] = {
    0x0000, 0x1c20, 0x3840, 0x2460, 0x7080, 0x6ca0, 0x48c0, 0x54e0,
    0xe100, 0xfd20, 0xd940, 0xc560, 0x9180, 0x8da0, 0xa9c0, 0xb5e0
};


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static void aes_xor_no_simd(unsigned char *r,
                            const unsigned char *a,
                            const unsigned char *b,
                            size_t n)
{
    size_t i = 0;

    for (; (i + 4) <= n; i += 4) {
        uint32_t x = drv_get_unaligned_uint32(a + i) ^ drv_get_unaligned_uint32(b + i);
        drv_put_unaligned_uint32(r + i, x);
    }

    for (; i < n; i++) {
        r[i] = a[i] ^ b[i];
    }
}

static void aes_ctr_increment_counter(uint8_t n[16])
{
    // The 32-bit version seems to perform about the same as a 64-bit version
    // on 64-bit architectures, so no need to define a 64-bit version.
    for (int i = 3;; i--) {
        uint32_t x = DRV_GET_UINT32_BE(n, i << 2);
        x += 1;
        DRV_PUT_UINT32_BE(x, n, i << 2);
        if (x != 0 || i == 0) {
            break;
        }
    }
}
/*
 * Multiplication by u in the Galois field of GF(2^n)
 *
 * As explained in NIST SP 800-38B, this can be computed:
 *
 *   If MSB(p) = 0, then p = (p << 1)
 *   If MSB(p) = 1, then p = (p << 1) ^ R_n
 *   with R_64 = 0x1B and  R_128 = 0x87
 *
 * Input and output MUST NOT point to the same buffer
 * Block size must be 8 bytes or 16 bytes - the block sizes for DES and AES.
 */
static void aes_cmac_multiply_by_u(uint8_t *output, const uint8_t *input, uint32_t blocksize)
{
    uint8_t R_n;
    uint32_t overflow = 0x00;
    uint32_t i32, new_overflow;
    int32_t i;

    OM_ASSERT(blocksize == AES_BLOCK_SIZE); // This is just for aes cmac, block must be 16 bytes

    for (i = blocksize - 4; i >= 0; i -= 4) {
        i32 = DRV_GET_UINT32_BE(&input[i], 0);
        new_overflow = i32 >> 31;
        i32 = (i32 << 1) | overflow;
        DRV_PUT_UINT32_BE(i32, &output[i], 0);
        overflow = new_overflow;
    }

    R_n = (input[0] >> 7) ? 0x87 : 0x00;
    output[blocksize - 1] ^= R_n;
}

static void aes_cmac_generate_subkeys(uint32_t om_aes, uint8_t *K1, uint8_t *K2)
{
    uint8_t L[16];

    memset(L, 0, sizeof(L));

    /* Calculate Ek(0) */
    drv_aes_encrypt(om_aes, L, L);

    /*
     * Generate K1 and K2
     */
    aes_cmac_multiply_by_u(K1, L, AES_BLOCK_SIZE);
    aes_cmac_multiply_by_u(K2, K1, AES_BLOCK_SIZE);

    // protect from side-channel attacks
    memset(L, 0, sizeof(L));
}

static void aes_cmac_pad(uint8_t padded_block[16],
                         uint32_t padded_block_len,
                         const uint8_t *last_block,
                         uint32_t last_block_len)
{
    for (uint32_t j = 0; j < padded_block_len; j++) {
        if (j < last_block_len) {
            padded_block[j] = last_block[j];
        } else if (j == last_block_len) {
            padded_block[j] = 0x80;
        } else {
            padded_block[j] = 0x00;
        }
    }
}

static aes_env_t *aes_get_env(uint32_t om_aes)
{
    if (aes0_env.aes_id == om_aes) {
        return &aes0_env;
    }

    OM_ASSERT(0);
    return NULL;
}

static void aes_gcm_mult(aes_env_t *env, const uint8_t x[16], uint8_t output[16])
{
    int i;
    uint8_t lo, hi, rem;
    uint64_t zh, zl;

    lo = (uint8_t)( x[15] & 0x0f );
    hi = (uint8_t)( x[15] >> 4 );
    zh = env->mode_ctx.gcm_ctx.HH[lo];
    zl = env->mode_ctx.gcm_ctx.HL[lo];

    for(i = 15; i >= 0; i--) {
        lo = (uint8_t) ( x[i] & 0x0f );
        hi = (uint8_t) ( x[i] >> 4 );

        if( i != 15 ) {
            rem = (uint8_t) ( zl & 0x0f );
            zl = ( zh << 60 ) | ( zl >> 4 );
            zh = ( zh >> 4 );
            zh ^= (uint64_t) last4[rem] << 48;
            zh ^= env->mode_ctx.gcm_ctx.HH[lo];
            zl ^= env->mode_ctx.gcm_ctx.HL[lo];
        }
        rem = (uint8_t) ( zl & 0x0f );
        zl = ( zh << 60 ) | ( zl >> 4 );
        zh = ( zh >> 4 );
        zh ^= (uint64_t) last4[rem] << 48;
        zh ^= env->mode_ctx.gcm_ctx.HH[hi];
        zl ^= env->mode_ctx.gcm_ctx.HL[hi];
    }

    DRV_PUT_UINT32_BE(zh >> 32, output, 0);
    DRV_PUT_UINT32_BE(zh, output, 4);
    DRV_PUT_UINT32_BE(zl >> 32, output, 8);
    DRV_PUT_UINT32_BE(zl, output, 12);
}
#endif /* (RTE_AES_KEEP_RAW_ENGINE) */

/**
 *******************************************************************************
 * @brief Set hardware AES encrypt key
 *
 * @param[in] om_aes    Pointer to HW AES
 * @param[in] key       Pointer to aes key
 * @param[in] keybits   Bits of key
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_aes_hw_setkey_enc(OM_AES_HW_Type *om_aes, const uint8_t *key, uint32_t keybits)
{
    OM_ASSERT(keybits == 128 || keybits == 256);

    OM_CRITICAL_BEGIN();
    AES_HW_CLK_ENABLE(1);

    if (keybits == 256) {
        memcpy((void *)&om_aes->KEY[0], key, keybits / 8);
        om_aes->CTRL |= AES_HW_CTRL_MODE_MASK;
    } else if (keybits == 128) {
        memcpy((void *)&om_aes->KEY[4], key, keybits / 8);
        om_aes->CTRL &= ~AES_HW_CTRL_MODE_MASK;
    }

    AES_HW_CLK_ENABLE(0);
    OM_CRITICAL_END();

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Hardware AES encrypt engine
 *
 * @param[in] om_aes    Pointer to HW AES
 * @param[in] input     Pointer to input data
 * @param[out] output   Pointer to output data
 *******************************************************************************
 */
void drv_aes_hw_encrypt(OM_AES_HW_Type *om_aes, const uint8_t input[16], uint8_t output[16])
{
    OM_CRITICAL_BEGIN();
    AES_HW_CLK_ENABLE(1);

    memcpy((void *)om_aes->DATA, input, 16);
    om_aes->CTRL |= AES_HW_CTRL_START_MASK;
    while (!(om_aes->CTRL & AES_HW_CTRL_DONE_MASK));
    om_aes->CTRL |= AES_HW_CTRL_DONE_MASK;
    memcpy(output, (void *)om_aes->DATA, 16);

    AES_HW_CLK_ENABLE(0);
    OM_CRITICAL_END();
}

#if (!RTE_AES_KEEP_RAW_ENGINE)
/**
 *******************************************************************************
 * @brief Software AES set encrypt key
 *
 * @param[in] sw_ctx    Pointer to software context
 * @param[in] key       Pointer to key
 * @param[in] keybits   Key bits
 *
 * @return om_error
 *******************************************************************************
 */
static om_error_t drv_aes_sw_setkey_enc(aes_sw_context_t *ctx, const uint8_t *key, uint32_t keybits)
{
    uint32_t *RK;

    switch (keybits) {
        case 128: ctx->nr = 10; break;
        case 192: ctx->nr = 12; break;
        case 256: ctx->nr = 14; break;
        default: return OM_ERROR_PARAMETER;
    }

    ctx->rk_offset = 0;
    RK = ctx->buf + ctx->rk_offset;

    for (unsigned int i = 0; i < (keybits >> 5); i++) {
        RK[i] = DRV_GET_UINT32_LE(key, i << 2);
    }

    switch (ctx->nr) {
        case 10:
            for (unsigned int i = 0; i < 10; i++, RK += 4) {
                RK[4]  = RK[0] ^ round_constants[i] ^
                         ((uint32_t) FSb[DRV_BYTE_1(RK[3])]) ^
                         ((uint32_t) FSb[DRV_BYTE_2(RK[3])] <<  8) ^
                         ((uint32_t) FSb[DRV_BYTE_3(RK[3])] << 16) ^
                         ((uint32_t) FSb[DRV_BYTE_0(RK[3])] << 24);

                RK[5]  = RK[1] ^ RK[4];
                RK[6]  = RK[2] ^ RK[5];
                RK[7]  = RK[3] ^ RK[6];
            }
            break;

        case 12:
            for (unsigned int i = 0; i < 8; i++, RK += 6) {
                RK[6]  = RK[0] ^ round_constants[i] ^
                         ((uint32_t) FSb[DRV_BYTE_1(RK[5])]) ^
                         ((uint32_t) FSb[DRV_BYTE_2(RK[5])] <<  8) ^
                         ((uint32_t) FSb[DRV_BYTE_3(RK[5])] << 16) ^
                         ((uint32_t) FSb[DRV_BYTE_0(RK[5])] << 24);

                RK[7]  = RK[1] ^ RK[6];
                RK[8]  = RK[2] ^ RK[7];
                RK[9]  = RK[3] ^ RK[8];
                RK[10] = RK[4] ^ RK[9];
                RK[11] = RK[5] ^ RK[10];
            }
            break;

        case 14:
            for (unsigned int i = 0; i < 7; i++, RK += 8) {
                RK[8]  = RK[0] ^ round_constants[i] ^
                         ((uint32_t) FSb[DRV_BYTE_1(RK[7])]) ^
                         ((uint32_t) FSb[DRV_BYTE_2(RK[7])] <<  8) ^
                         ((uint32_t) FSb[DRV_BYTE_3(RK[7])] << 16) ^
                         ((uint32_t) FSb[DRV_BYTE_0(RK[7])] << 24);

                RK[9]  = RK[1] ^ RK[8];
                RK[10] = RK[2] ^ RK[9];
                RK[11] = RK[3] ^ RK[10];

                RK[12] = RK[4] ^
                         ((uint32_t) FSb[DRV_BYTE_0(RK[11])]) ^
                         ((uint32_t) FSb[DRV_BYTE_1(RK[11])] <<  8) ^
                         ((uint32_t) FSb[DRV_BYTE_2(RK[11])] << 16) ^
                         ((uint32_t) FSb[DRV_BYTE_3(RK[11])] << 24);

                RK[13] = RK[5] ^ RK[12];
                RK[14] = RK[6] ^ RK[13];
                RK[15] = RK[7] ^ RK[14];
            }
            break;
    }

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Software AES set decrypt key
 *
 * @param[in] sw_ctx    Pointer to software context
 * @param[in] key       Pointer to key
 * @param[in] keybits   Key bits
 *
 * @return om_error
 *******************************************************************************
 */
static om_error_t drv_aes_sw_setkey_dec(aes_sw_context_t *ctx, const uint8_t *key, uint32_t keybits)
{
    uint32_t *SK;
    om_error_t error;
    uint32_t *RK;
    aes_sw_context_t cty;

    memset(&cty, 0, sizeof(aes_sw_context_t));

    ctx->rk_offset = 0;
    RK = ctx->buf + ctx->rk_offset;

    /* Also checks keybits */
    if ((error = drv_aes_sw_setkey_enc(&cty, key, keybits)) != 0) {
        goto exit;
    }

    ctx->nr = cty.nr;

    SK = cty.buf + cty.rk_offset + cty.nr * 4;

    *RK++ = *SK++;
    *RK++ = *SK++;
    *RK++ = *SK++;
    *RK++ = *SK++;
    SK -= 8;
    for (int i = ctx->nr - 1; i > 0; i--, SK -= 8) {
        for (int j = 0; j < 4; j++, SK++) {
            *RK++ = AES_RT0(FSb[DRV_BYTE_0(*SK)]) ^
                    AES_RT1(FSb[DRV_BYTE_1(*SK)]) ^
                    AES_RT2(FSb[DRV_BYTE_2(*SK)]) ^
                    AES_RT3(FSb[DRV_BYTE_3(*SK)]);
        }
    }

    *RK++ = *SK++;
    *RK++ = *SK++;
    *RK++ = *SK++;
    *RK++ = *SK++;

exit:
    // for security
    memset(&cty, 0, sizeof(aes_sw_context_t));

    return error;
}

/**
 *******************************************************************************
 * @brief Software AES encrypt
 *
 * @param[in] sw_ctx    Pointer to software context
 * @param[in] input     Pointer to input data
 * @param[out] output   Pointer to output data
 *******************************************************************************
 */
static void drv_aes_sw_encrypt(aes_sw_context_t *ctx, const uint8_t input[16], uint8_t output[16])
{
    int i;
    uint32_t *RK = ctx->buf + ctx->rk_offset;
    struct {
        uint32_t X[4];
        uint32_t Y[4];
    } t;

    t.X[0] = DRV_GET_UINT32_LE(input,  0); t.X[0] ^= *RK++;
    t.X[1] = DRV_GET_UINT32_LE(input,  4); t.X[1] ^= *RK++;
    t.X[2] = DRV_GET_UINT32_LE(input,  8); t.X[2] ^= *RK++;
    t.X[3] = DRV_GET_UINT32_LE(input, 12); t.X[3] ^= *RK++;

    for (i = (ctx->nr >> 1) - 1; i > 0; i--) {
        AES_FROUND(t.Y[0], t.Y[1], t.Y[2], t.Y[3], t.X[0], t.X[1], t.X[2], t.X[3]);
        AES_FROUND(t.X[0], t.X[1], t.X[2], t.X[3], t.Y[0], t.Y[1], t.Y[2], t.Y[3]);
    }

    AES_FROUND(t.Y[0], t.Y[1], t.Y[2], t.Y[3], t.X[0], t.X[1], t.X[2], t.X[3]);

    t.X[0] = *RK++ ^ \
             ((uint32_t) FSb[DRV_BYTE_0(t.Y[0])]) ^
             ((uint32_t) FSb[DRV_BYTE_1(t.Y[1])] <<  8) ^
             ((uint32_t) FSb[DRV_BYTE_2(t.Y[2])] << 16) ^
             ((uint32_t) FSb[DRV_BYTE_3(t.Y[3])] << 24);

    t.X[1] = *RK++ ^ \
             ((uint32_t) FSb[DRV_BYTE_0(t.Y[1])]) ^
             ((uint32_t) FSb[DRV_BYTE_1(t.Y[2])] <<  8) ^
             ((uint32_t) FSb[DRV_BYTE_2(t.Y[3])] << 16) ^
             ((uint32_t) FSb[DRV_BYTE_3(t.Y[0])] << 24);

    t.X[2] = *RK++ ^ \
             ((uint32_t) FSb[DRV_BYTE_0(t.Y[2])]) ^
             ((uint32_t) FSb[DRV_BYTE_1(t.Y[3])] <<  8) ^
             ((uint32_t) FSb[DRV_BYTE_2(t.Y[0])] << 16) ^
             ((uint32_t) FSb[DRV_BYTE_3(t.Y[1])] << 24);

    t.X[3] = *RK++ ^ \
             ((uint32_t) FSb[DRV_BYTE_0(t.Y[3])]) ^
             ((uint32_t) FSb[DRV_BYTE_1(t.Y[0])] <<  8) ^
             ((uint32_t) FSb[DRV_BYTE_2(t.Y[1])] << 16) ^
             ((uint32_t) FSb[DRV_BYTE_3(t.Y[2])] << 24);

    DRV_PUT_UINT32_LE(t.X[0], output,  0);
    DRV_PUT_UINT32_LE(t.X[1], output,  4);
    DRV_PUT_UINT32_LE(t.X[2], output,  8);
    DRV_PUT_UINT32_LE(t.X[3], output, 12);

    // for security
    memset(&t, 0, sizeof(t));
}

/**
 *******************************************************************************
 * @brief Software AES decrypt
 *
 * @param[in] sw_ctx    Pointer to software context
 * @param[in] input     Pointer to input data
 * @param[out] output   Pointer to output data
 *******************************************************************************
 */
static void drv_aes_sw_decrypt(aes_sw_context_t *ctx, const uint8_t input[16], uint8_t output[16])
{
    int i;
    uint32_t *RK = ctx->buf + ctx->rk_offset;
    struct {
        uint32_t X[4];
        uint32_t Y[4];
    } t;

    t.X[0] = DRV_GET_UINT32_LE(input,  0); t.X[0] ^= *RK++;
    t.X[1] = DRV_GET_UINT32_LE(input,  4); t.X[1] ^= *RK++;
    t.X[2] = DRV_GET_UINT32_LE(input,  8); t.X[2] ^= *RK++;
    t.X[3] = DRV_GET_UINT32_LE(input, 12); t.X[3] ^= *RK++;

    for (i = (ctx->nr >> 1) - 1; i > 0; i--) {
        AES_RROUND(t.Y[0], t.Y[1], t.Y[2], t.Y[3], t.X[0], t.X[1], t.X[2], t.X[3]);
        AES_RROUND(t.X[0], t.X[1], t.X[2], t.X[3], t.Y[0], t.Y[1], t.Y[2], t.Y[3]);
    }

    AES_RROUND(t.Y[0], t.Y[1], t.Y[2], t.Y[3], t.X[0], t.X[1], t.X[2], t.X[3]);

    t.X[0] = *RK++ ^ \
             ((uint32_t) RSb[DRV_BYTE_0(t.Y[0])]) ^
             ((uint32_t) RSb[DRV_BYTE_1(t.Y[3])] <<  8) ^
             ((uint32_t) RSb[DRV_BYTE_2(t.Y[2])] << 16) ^
             ((uint32_t) RSb[DRV_BYTE_3(t.Y[1])] << 24);

    t.X[1] = *RK++ ^ \
             ((uint32_t) RSb[DRV_BYTE_0(t.Y[1])]) ^
             ((uint32_t) RSb[DRV_BYTE_1(t.Y[0])] <<  8) ^
             ((uint32_t) RSb[DRV_BYTE_2(t.Y[3])] << 16) ^
             ((uint32_t) RSb[DRV_BYTE_3(t.Y[2])] << 24);

    t.X[2] = *RK++ ^ \
             ((uint32_t) RSb[DRV_BYTE_0(t.Y[2])]) ^
             ((uint32_t) RSb[DRV_BYTE_1(t.Y[1])] <<  8) ^
             ((uint32_t) RSb[DRV_BYTE_2(t.Y[0])] << 16) ^
             ((uint32_t) RSb[DRV_BYTE_3(t.Y[3])] << 24);

    t.X[3] = *RK++ ^ \
             ((uint32_t) RSb[DRV_BYTE_0(t.Y[3])]) ^
             ((uint32_t) RSb[DRV_BYTE_1(t.Y[2])] <<  8) ^
             ((uint32_t) RSb[DRV_BYTE_2(t.Y[1])] << 16) ^
             ((uint32_t) RSb[DRV_BYTE_3(t.Y[0])] << 24);

    DRV_PUT_UINT32_LE(t.X[0], output,  0);
    DRV_PUT_UINT32_LE(t.X[1], output,  4);
    DRV_PUT_UINT32_LE(t.X[2], output,  8);
    DRV_PUT_UINT32_LE(t.X[3], output, 12);

    // for security
    memset(&t, 0, sizeof(t));
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Set AES encrypt key
 *
 * @param[in] om_aes    AES id
 * @param[in] key       Pointer to aes key
 * @param[in] keybits   Bits of key
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_aes_setkey_enc(uint32_t om_aes, const uint8_t *key, uint32_t keybits)
{
    uint8_t key_r[32];
    aes_env_t *env;

    env = aes_get_env(om_aes);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    env->keybits = keybits;

    if ((keybits == 128 && (env->cap & CAP_AES_HW_SUPPORT_128BITS_MASK)) ||
        (keybits == 192 && (env->cap & CAP_AES_HW_SUPPORT_192BITS_MASK))  ||
        (keybits == 256 && (env->cap & CAP_AES_HW_SUPPORT_256BITS_MASK))) {
            for (uint8_t i = 0; i < keybits / 8; i++) {
                key_r[i] = key[keybits / 8 - 1 - i];
            }
            return drv_aes_hw_setkey_enc(OM_AES, key_r, keybits);
    }

    return drv_aes_sw_setkey_enc(&env->sw_ctx, key, keybits);
}

/**
 *******************************************************************************
 * @brief Set AES decrypt key
 *
 * @param[in] om_aes    AES id
 * @param[in] key       Pointer to aes key
 * @param[in] keybits   Bits of key
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_aes_setkey_dec(uint32_t om_aes, const uint8_t *key, uint32_t keybits)
{
    aes_env_t *env;

    env = aes_get_env(om_aes);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    return drv_aes_sw_setkey_dec(&env->sw_ctx, key, keybits);
}

/**
 *******************************************************************************
 * @brief AES encrypt engine
 *
 * @param[in] om_aes    aes id
 * @param[in] input     Pointer to input data
 * @param[out] output   Pointer to output data
 *******************************************************************************
 */
void drv_aes_encrypt(uint32_t om_aes, const uint8_t input[16], uint8_t output[16])
{
    uint8_t input_r[16];
    uint8_t output_r[16];
    aes_env_t *env;

    env = aes_get_env(om_aes);
    OM_ASSERT(env != NULL);

    if ((env->keybits == 128 && (env->cap & CAP_AES_HW_SUPPORT_128BITS_MASK)) ||
        (env->keybits == 192 && (env->cap & CAP_AES_HW_SUPPORT_192BITS_MASK)) ||
        (env->keybits == 256 && (env->cap & CAP_AES_HW_SUPPORT_256BITS_MASK))) {
            for (uint8_t i = 0; i < 16; i++) {
                input_r[i] = input[15 - i];
            }
            drv_aes_hw_encrypt(OM_AES, input_r, output_r);
            for (uint8_t i = 0; i < 16; i++) {
                output[i] = output_r[15 - i];
            }
            return;
    }

    drv_aes_sw_encrypt(&env->sw_ctx, input, output);
}

/**
 *******************************************************************************
 * @brief AES decrypt engine
 *
 * @param[in] om_aes    aes id
 * @param[in] input     Pointer to input data
 * @param[out] output   Pointer to output data
 *******************************************************************************
 */
void drv_aes_decrypt(uint32_t om_aes, const uint8_t input[16], uint8_t output[16])
{
    aes_env_t *env;

    env = aes_get_env(om_aes);
    OM_ASSERT(env != NULL);

    drv_aes_sw_decrypt(&env->sw_ctx, input, output);
}

/**
 *******************************************************************************
 * @brief Start AES using ECB mode
 *
 * @param[in] om_aes        AES ID
 * @param[in] cfg           AES ECB configuration
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_aes_ecb_start(uint32_t om_aes, const aes_ecb_config_t *cfg)
{
    aes_env_t *env;
    uint32_t keybits;

    env = aes_get_env(om_aes);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    OM_ASSERT(env->state == DRV_STATE_STOP);
    OM_ASSERT(cfg->operation == AES_OP_ENCRYPT || cfg->operation == AES_OP_DECRYPT);

    switch (cfg->keybits) {
        case AES_KEYBITS_128: keybits = 128; break;
        case AES_KEYBITS_192: keybits = 192; break;
        case AES_KEYBITS_256: keybits = 256; break;
        default: OM_ASSERT(0); return OM_ERROR_PARAMETER;
    }

    env->mode = DRV_AES_MODE_ECB;
    env->keybits = keybits;
    env->operation = cfg->operation;

    // Configure Key
    if (env->operation == AES_OP_ENCRYPT) {
        drv_aes_setkey_enc(om_aes, cfg->key, keybits);
    } else {
        drv_aes_setkey_dec(om_aes, cfg->key, keybits);
    }
    // Update state
    env->state = DRV_STATE_START;

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Continue AES calculation using ECB mode
 *
 * @param[in] om_aes    AES ID
 * @param[in] input     Pointer to input data
 * @param[out] output   Pointer to output data
 * @param[in] length    Length of input data
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_aes_ecb_crypt_continue(uint32_t om_aes, const uint8_t *input, uint8_t *output, uint32_t length)
{
    aes_env_t *env;

    env = aes_get_env(om_aes);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    OM_ASSERT((length % AES_BLOCK_SIZE == 0) && (length != 0));
    OM_ASSERT((env->state == DRV_STATE_START || env->state == DRV_STATE_CONTINUE) && env->mode == DRV_AES_MODE_ECB);

    if (env->operation == AES_OP_ENCRYPT) {
        for (uint32_t i = 0; i < length; i += 16) {
            drv_aes_encrypt(om_aes, input + i, output + i);
        }
    } else {
        for (uint32_t i = 0; i < length; i += 16) {
            drv_aes_decrypt(om_aes, input + i, output + i);
        }
    }

    env->state = DRV_STATE_CONTINUE;

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Stop AES calculation using ECB mode
 *
 * @param[in] om_aes    AES ID
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_aes_ecb_crypt_stop(uint32_t om_aes)
{
    aes_env_t *env;

    env = aes_get_env(om_aes);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    OM_ASSERT(env->state == DRV_STATE_CONTINUE && env->mode == DRV_AES_MODE_ECB);

    memset(&env->mode, 0x0, sizeof(aes_env_t) - offsetof(aes_env_t, mode));

    env->state = DRV_STATE_STOP;

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Start AES using CBC mode
 *
 * @param[in] om_aes        AES ID
 * @param[in] cfg           AES CBC configuration
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_aes_cbc_start(uint32_t om_aes, const aes_cbc_config_t *cfg)
{
    aes_env_t *env;
    uint32_t keybits;

    env = aes_get_env(om_aes);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    OM_ASSERT(env->state == DRV_STATE_STOP);
    OM_ASSERT(cfg->operation == AES_OP_ENCRYPT || cfg->operation == AES_OP_DECRYPT);

    switch (cfg->keybits) {
        case AES_KEYBITS_128: keybits = 128; break;
        case AES_KEYBITS_192: keybits = 192; break;
        case AES_KEYBITS_256: keybits = 256; break;
        default: OM_ASSERT(0); return OM_ERROR_PARAMETER;
    }

    env->mode = DRV_AES_MODE_CBC;
    env->keybits = keybits;
    env->operation = cfg->operation;

    // Configure Key
    if (env->operation == AES_OP_ENCRYPT) {
        drv_aes_setkey_enc(om_aes, cfg->key, keybits);
    } else {
        drv_aes_setkey_dec(om_aes, cfg->key, keybits);
    }
    // Configure IV
    memcpy(env->mode_ctx.cbc_ctx.iv, cfg->iv, 16);
    // Update state
    env->state = DRV_STATE_START;

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Continue AES calculation using CBC mode
 *
 * @param[in] om_aes    AES ID
 * @param[in] input     Pointer to input data
 * @param[out] output   Pointer to output data
 * @param[in] length    Length of input data
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_aes_cbc_crypt_continue(uint32_t om_aes, const uint8_t *input, uint8_t *output, uint32_t length)
{
    uint8_t temp[16], output_dummy[16], *used_output;
    uint8_t *iv;
    const uint8_t *ivp;
    aes_env_t *env;

    env = aes_get_env(om_aes);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    OM_ASSERT((length % AES_BLOCK_SIZE == 0) && (length != 0));
    OM_ASSERT((env->state == DRV_STATE_START || env->state == DRV_STATE_CONTINUE) && env->mode == DRV_AES_MODE_CBC);

    iv = env->mode_ctx.cbc_ctx.iv;
    ivp = iv;

    // Used when not need output data at this time
    used_output = (output != NULL) ? output : output_dummy;

    if (env->operation == AES_OP_DECRYPT) {
        while (length > 0) {
            memcpy(temp, input, 16);
            drv_aes_decrypt(om_aes, input, used_output);
            aes_xor_no_simd(used_output, used_output, iv, 16);

            memcpy(iv, temp, 16);

            input  += 16;
            length -= 16;
            if (output != NULL) {
                used_output += 16;
            }
        }
    } else {
        while (length > 0) {
            aes_xor_no_simd(used_output, input, ivp, 16);

            drv_aes_encrypt(om_aes, used_output, used_output);

            ivp = used_output;

            input  += 16;
            length -= 16;
            if (output != NULL) {
                used_output += 16;
            }
        }
        memcpy(iv, ivp, 16);
    }

    env->state = DRV_STATE_CONTINUE;

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Stop AES calculation using CBC mode
 *
 * @param[in] om_aes    AES ID
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_aes_cbc_crypt_stop(uint32_t om_aes)
{
    aes_env_t *env;

    env = aes_get_env(om_aes);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    OM_ASSERT(env->state == DRV_STATE_CONTINUE && env->mode == DRV_AES_MODE_CBC);

    memset(&env->mode, 0x0, sizeof(aes_env_t) - offsetof(aes_env_t, mode));

    env->state = DRV_STATE_STOP;

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Start AES using CFB mode
 *
 * @param[in] om_aes        AES ID
 * @param[in] cfg           AES CFB configuration
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_aes_cfb_start(uint32_t om_aes, const aes_cfb_config_t *cfg)
{
    aes_env_t *env;
    uint32_t keybits;

    env = aes_get_env(om_aes);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    OM_ASSERT(env->state == DRV_STATE_STOP);
    OM_ASSERT(cfg->operation == AES_OP_ENCRYPT || cfg->operation == AES_OP_DECRYPT);

    switch (cfg->keybits) {
        case AES_KEYBITS_128: keybits = 128; break;
        case AES_KEYBITS_192: keybits = 192; break;
        case AES_KEYBITS_256: keybits = 256; break;
        default: OM_ASSERT(0); return OM_ERROR_PARAMETER;
    }

    env->mode = DRV_AES_MODE_CFB;
    env->keybits = keybits;
    env->operation = cfg->operation;

    // Configure Key
    drv_aes_setkey_enc(om_aes, cfg->key, keybits);
    // Configure IV offset and IV
    env->mode_ctx.cfb_ctx.iv_off = cfg->iv_offset;
    memcpy(env->mode_ctx.cfb_ctx.iv, cfg->iv, 16);

    // Update state
    env->state = DRV_STATE_START;

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Continue AES calculation using CFB mode
 *
 * @param[in] om_aes    AES ID
 * @param[in] input     Pointer to input data
 * @param[out] output   Pointer to output data
 * @param[in] length    Length of input data
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_aes_cfb_crypt_continue(uint32_t om_aes, const uint8_t *input, uint8_t *output, uint32_t length)
{
    int c;
    size_t n;
    uint8_t *iv;
    aes_env_t *env;

    env = aes_get_env(om_aes);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    n = env->mode_ctx.cfb_ctx.iv_off;
    iv = env->mode_ctx.cfb_ctx.iv;

    OM_ASSERT(n <= 15 && length != 0);
    OM_ASSERT((env->state == DRV_STATE_START || env->state == DRV_STATE_CONTINUE) && env->mode == DRV_AES_MODE_CFB);

    if (env->operation == AES_OP_DECRYPT) {
        while (length--) {
            if (n == 0) {
                drv_aes_encrypt(om_aes, iv, iv);
            }

            c = *input++;
            *output++ = (unsigned char) (c ^ iv[n]);
            iv[n] = (unsigned char) c;

            n = (n + 1) & 0x0F;
        }
    } else {
        while (length--) {
            if (n == 0) {
                drv_aes_encrypt(om_aes, iv, iv);
            }

            iv[n] = *output++ = (unsigned char) (iv[n] ^ *input++);

            n = (n + 1) & 0x0F;
        }
    }

    env->mode_ctx.cfb_ctx.iv_off = n;

    env->state = DRV_STATE_CONTINUE;

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Stop AES calculation using CFB mode
 *
 * @param[in] om_aes    AES ID
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_aes_cfb_crypt_stop(uint32_t om_aes)
{
    aes_env_t *env;

    env = aes_get_env(om_aes);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    OM_ASSERT(env->state == DRV_STATE_CONTINUE && env->mode == DRV_AES_MODE_CFB);

    memset(&env->mode, 0x0, sizeof(aes_env_t) - offsetof(aes_env_t, mode));

    env->state = DRV_STATE_STOP;

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Start AES using OFB mode
 *
 * @param[in] om_aes        AES ID
 * @param[in] cfg           AES OFB configuration
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_aes_ofb_start(uint32_t om_aes, const aes_ofb_config_t *cfg)
{
    aes_env_t *env;
    uint32_t keybits;

    env = aes_get_env(om_aes);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    OM_ASSERT(env->state == DRV_STATE_STOP);

    switch (cfg->keybits) {
        case AES_KEYBITS_128: keybits = 128; break;
        case AES_KEYBITS_192: keybits = 192; break;
        case AES_KEYBITS_256: keybits = 256; break;
        default: OM_ASSERT(0); return OM_ERROR_PARAMETER;
    }

    env->mode = DRV_AES_MODE_OFB;
    env->keybits = keybits;

    // Configure Key
    drv_aes_setkey_enc(om_aes, cfg->key, keybits);

    env->mode_ctx.cfb_ctx.iv_off = cfg->iv_offset;
    memcpy(env->mode_ctx.cfb_ctx.iv, cfg->iv, 16);

    // Update state
    env->state = DRV_STATE_START;

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Continue AES calculation using OFB mode
 *
 * @param[in] om_aes    AES ID
 * @param[in] input     Pointer to input data
 * @param[out] output   Pointer to output data
 * @param[in] length    Length of input data
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_aes_ofb_crypt_continue(uint32_t om_aes, const uint8_t *input, uint8_t *output, uint32_t length)
{
    size_t n;
    uint8_t *iv;
    aes_env_t *env;

    env = aes_get_env(om_aes);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    n =  env->mode_ctx.ofb_ctx.iv_off;
    iv = env->mode_ctx.ofb_ctx.iv;

    OM_ASSERT(n <= 15 && length != 0);
    OM_ASSERT((env->state == DRV_STATE_START || env->state == DRV_STATE_CONTINUE) && env->mode == DRV_AES_MODE_OFB);

    while (length--) {
        if (n == 0) {
            drv_aes_encrypt(om_aes, iv, iv);
        }
        *output++ =  *input++ ^ iv[n];

        n = (n + 1) & 0x0F;
    }

    env->mode_ctx.ofb_ctx.iv_off = n;

    env->state = DRV_STATE_CONTINUE;

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Stop AES calculation using OFB mode
 *
 * @param[in] om_aes    AES ID
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_aes_ofb_crypt_stop(uint32_t om_aes)
{
    aes_env_t *env;

    env = aes_get_env(om_aes);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    OM_ASSERT(env->state == DRV_STATE_CONTINUE && env->mode == DRV_AES_MODE_OFB);

    memset(&env->mode, 0x0, sizeof(aes_env_t) - offsetof(aes_env_t, mode));

    env->state = DRV_STATE_STOP;

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Start AES using CTR mode
 *
 * @param[in] om_aes        AES ID
 * @param[in] cfg           AES CTR configuration
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_aes_ctr_start(uint32_t om_aes, const aes_ctr_config_t *cfg)
{
    aes_env_t *env;
    uint32_t keybits;

    env = aes_get_env(om_aes);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    OM_ASSERT(env->state == DRV_STATE_STOP);

    switch (cfg->keybits) {
        case AES_KEYBITS_128: keybits = 128; break;
        case AES_KEYBITS_192: keybits = 192; break;
        case AES_KEYBITS_256: keybits = 256; break;
        default: OM_ASSERT(0); return OM_ERROR_PARAMETER;
    }

    env->mode = DRV_AES_MODE_CTR;
    env->keybits = keybits;

    // Configure Key
    drv_aes_setkey_enc(om_aes, cfg->key, keybits);

    // Configure NC offset and NC
    env->mode_ctx.ctr_ctx.nc_off = cfg->nc_offset;
    memcpy(env->mode_ctx.ctr_ctx.nonce_counter, cfg->nonce_counter, 16);
    // Update state
    env->state = DRV_STATE_START;

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Continue AES calculation using CTR mode
 *
 * @param[in] om_aes    AES ID
 * @param[in] input     Pointer to input data
 * @param[out] output   Pointer to output data
 * @param[in] length    Length of input data
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_aes_ctr_crypt_continue(uint32_t om_aes, const uint8_t *input, uint8_t *output, uint32_t length)
{
    size_t offset;
    uint8_t *nonce_counter;
    uint8_t *stream_block;
    aes_env_t *env;

    env = aes_get_env(om_aes);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    offset        = env->mode_ctx.ctr_ctx.nc_off;
    nonce_counter = env->mode_ctx.ctr_ctx.nonce_counter;
    stream_block  = env->mode_ctx.ctr_ctx.stream_block;

    OM_ASSERT(offset <= 15 && length != 0);
    OM_ASSERT((env->state == DRV_STATE_START || env->state == DRV_STATE_CONTINUE) && env->mode == DRV_AES_MODE_CTR);

    for (size_t i = 0; i < length;) {
        size_t n = 16;
        if (offset == 0) {
            drv_aes_encrypt(om_aes, nonce_counter, stream_block);
            aes_ctr_increment_counter(nonce_counter);
        } else {
            n -= offset;
        }

        if (n > (length - i)) {
            n = (length - i);
        }
        aes_xor_no_simd(&output[i], &input[i], &stream_block[offset], n);
        // offset might be non-zero for the last block, but in that case, we don't use it again
        offset = 0;
        i += n;
    }

    // capture offset for future resumption
    env->mode_ctx.ctr_ctx.nc_off = (env->mode_ctx.ctr_ctx.nc_off + length) % 16;

    env->state = DRV_STATE_CONTINUE;

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Stop AES calculation using CTR mode
 *
 * @param[in] om_aes    AES ID
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_aes_ctr_crypt_stop(uint32_t om_aes)
{
    aes_env_t *env;

    env = aes_get_env(om_aes);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    OM_ASSERT(env->state == DRV_STATE_CONTINUE && env->mode == DRV_AES_MODE_CTR);

    memset(&env->mode, 0x0, sizeof(aes_env_t) - offsetof(aes_env_t, mode));

    env->state = DRV_STATE_STOP;

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Start AES calculation using GCM mode, NOTE: this is BIG-ENDIAN
 *
 * @param[in] om_aes    AES ID
 * @param[in] cfg       GCM configuration
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_aes_gcm_start(uint32_t om_aes, const aes_gcm_config_t *cfg)
{
    uint32_t keybits;
    aes_env_t *env;

    env = aes_get_env(om_aes);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    OM_ASSERT(env->state == DRV_STATE_STOP);
    OM_ASSERT(cfg->gcm_operation == GCM_OP_ENCRYPT || cfg->gcm_operation == GCM_OP_DECRYPT);

    switch (cfg->keybits) {
        case AES_KEYBITS_128: keybits = 128; break;
        case AES_KEYBITS_192: keybits = 192; break;
        case AES_KEYBITS_256: keybits = 256; break;
        default: OM_ASSERT(0); return OM_ERROR_PARAMETER;
    }

    env->mode = DRV_AES_MODE_GCM;
    env->keybits = keybits;
    env->operation = AES_OP_ENCRYPT;
    env->mode_ctx.gcm_ctx.gcm_op = cfg->gcm_operation;

    int i, j;
    uint8_t h[16];
    uint64_t hi, lo;
    uint64_t vl, vh;

    uint8_t work_buf[16];           // XOR source built from provided IV if len != 16
    const uint8_t *p;               // general purpose array pointer
    uint8_t iv_len, use_len;       // IV length and length to use
    uint64_t add_len;

    memset(h, 0x0, sizeof(h));
    // since the context might be reused under the same key
    // we zero the working buffers for this next new process
    memset(env->mode_ctx.gcm_ctx.y, 0x0, sizeof(env->mode_ctx.gcm_ctx.y));
    memset(env->mode_ctx.gcm_ctx.buf, 0x0, sizeof(env->mode_ctx.gcm_ctx.buf));
    env->mode_ctx.gcm_ctx.len = 0;
    env->mode_ctx.gcm_ctx.add_len = 0;

    // Update state
    env->state = DRV_STATE_START;
    // Configure GCM key
    drv_aes_setkey_enc(om_aes, cfg->key, keybits);
    drv_aes_encrypt(om_aes, h, h);

    hi = DRV_GET_UINT32_BE(h, 0);
    lo = DRV_GET_UINT32_BE(h, 4);
    vh = (uint64_t) hi << 32 | lo;

    hi = DRV_GET_UINT32_BE(h, 8);
    lo = DRV_GET_UINT32_BE(h, 12);
    vl = (uint64_t) hi << 32 | lo;

    env->mode_ctx.gcm_ctx.HL[8] = vl;
    env->mode_ctx.gcm_ctx.HH[8] = vh;
    env->mode_ctx.gcm_ctx.HH[0] = 0;
    env->mode_ctx.gcm_ctx.HL[0] = 0;

    for(i = 4; i > 0; i >>= 1) {
        uint32_t T = (uint32_t) ( vl & 1 ) * 0xe1000000U;
        vl  = ( vh << 63 ) | ( vl >> 1 );
        vh  = ( vh >> 1 ) ^ ( (uint64_t) T << 32);
        env->mode_ctx.gcm_ctx.HL[i] = vl;
        env->mode_ctx.gcm_ctx.HH[i] = vh;
    }
    for (i = 2; i < 16; i <<= 1 ) {
        uint64_t *HiL = env->mode_ctx.gcm_ctx.HL + i, *HiH = env->mode_ctx.gcm_ctx.HH + i;
        vh = *HiH;
        vl = *HiL;
        for( j = 1; j < i; j++ ) {
            HiH[j] = vh ^ env->mode_ctx.gcm_ctx.HH[j];
            HiL[j] = vl ^ env->mode_ctx.gcm_ctx.HL[j];
        }
    }

    // Configure IV
    iv_len = cfg->iv_len;
    if (iv_len == 12) {
        memcpy(env->mode_ctx.gcm_ctx.y, cfg->iv, iv_len);
        env->mode_ctx.gcm_ctx.y[15] = 0x01;
    } else {
        memset(work_buf, 0x00, 16);                         // clear the working buffer
        DRV_PUT_UINT32_BE(iv_len * 8, work_buf, 12);   // place the IV into buffer

        p = cfg->iv;
        while(iv_len > 0) {
            use_len = (iv_len < 16) ? iv_len : 16;
            for(i = 0; i < use_len; i++) env->mode_ctx.gcm_ctx.y[i] ^= p[i];
            aes_gcm_mult(env, env->mode_ctx.gcm_ctx.y, env->mode_ctx.gcm_ctx.y);
            iv_len -= use_len;
            p += use_len;
        }
        for(i = 0; i < 16; i++) env->mode_ctx.gcm_ctx.y[i] ^= work_buf[i];
        aes_gcm_mult(env, env->mode_ctx.gcm_ctx.y, env->mode_ctx.gcm_ctx.y);
    }
    drv_aes_encrypt(om_aes, env->mode_ctx.gcm_ctx.y, env->mode_ctx.gcm_ctx.base_ectr);

    // Configure additional data
    add_len = cfg->addition_len;
    env->mode_ctx.gcm_ctx.add_len = add_len;
    p = cfg->addition;
    while(add_len > 0) {
        use_len = (add_len < 16) ? add_len : 16;
        for( i = 0; i < use_len; i++ ) env->mode_ctx.gcm_ctx.buf[i] ^= p[i];
        aes_gcm_mult(env, env->mode_ctx.gcm_ctx.buf, env->mode_ctx.gcm_ctx.buf);
        add_len -= use_len;
        p += use_len;
    }

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Continue AES calculation using GCM mode, NOTE: this is BIG-ENDIAN
 *
 * @param[in] om_aes    AES ID
 * @param[in] input     Pointer to input data
 * @param[out] output   Pointer to output data
 * @param[in] length    Length of input data
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_aes_gcm_crypt_continue(uint32_t om_aes, const uint8_t *input, uint8_t *output, uint32_t length)
{
    uint8_t ectr[16];       // counter-mode cipher output for XORing
    size_t use_len;         // byte count to process, up to 16 bytes
    size_t i;               // local loop iterator

    aes_env_t *env;

    env = aes_get_env(om_aes);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    OM_ASSERT(length != 0);
    OM_ASSERT((env->state == DRV_STATE_START || env->state == DRV_STATE_CONTINUE) && env->mode == DRV_AES_MODE_GCM);

    env->mode_ctx.gcm_ctx.len += length; // bump the GCM context's running length count

    while (length > 0) {
        // clamp the length to process at 16 bytes
        use_len = ( length < 16 ) ? length : 16;

        // increment the context's 128-bit IV||Counter 'y' vector
        for(i = 16; i > 12; i--) if (++env->mode_ctx.gcm_ctx.y[i - 1] != 0) break;

        // encrypt the context's 'y' vector under the established key
        drv_aes_encrypt(om_aes, env->mode_ctx.gcm_ctx.y, ectr);

        // encrypt or decrypt the input to the output
        if( env->mode_ctx.gcm_ctx.gcm_op == GCM_OP_ENCRYPT ) {
             for( i = 0; i < use_len; i++ ) {
                // XOR the cipher's ouptut vector (ectr) with our input
                output[i] = (uint8_t) ( ectr[i] ^ input[i] );
                // now we mix in our data into the authentication hash.
                // if we're ENcrypting we XOR in the post-XOR (output)
                // results, but if we're DEcrypting we XOR in the input
                // data
                env->mode_ctx.gcm_ctx.buf[i] ^= output[i];
            }
        } else {
            for( i = 0; i < use_len; i++ ) {
                // but if we're DEcrypting we XOR in the input data first,
                // i.e. before saving to ouput data, otherwise if the input
                // and output buffer are the same (inplace decryption) we
                // would not get the correct auth tag

       	        env->mode_ctx.gcm_ctx.buf[i] ^= input[i];

                // XOR the cipher's ouptut vector (ectr) with our input
                output[i] = (uint8_t) ( ectr[i] ^ input[i] );
             }
        }

        // perform a GHASH operation
        aes_gcm_mult(env, env->mode_ctx.gcm_ctx.buf, env->mode_ctx.gcm_ctx.buf);

        length -= use_len;  // drop the remaining byte count to process
        input  += use_len;  // bump our input pointer forward
        output += use_len;  // bump our output pointer forward
    }

    env->state = DRV_STATE_CONTINUE;

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Stop AES calculation using GCM mode, NOTE: this is BIG-ENDIAN
 *
 * @param[in] om_aes    AES ID
 * @param[out] tag      Pointer to tag(hash)
 * @param[in] tag_len   Length of tag
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_aes_gcm_crypt_stop(uint32_t om_aes, uint8_t *tag, uint8_t tag_len)
{
    uint8_t work_buf[16];
    uint64_t orig_len, orig_add_len;
    size_t i;
    aes_env_t *env;

    env = aes_get_env(om_aes);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    OM_ASSERT(tag_len != 0);
    OM_ASSERT(env->state == DRV_STATE_CONTINUE && env->mode == DRV_AES_MODE_GCM);

    orig_len = env->mode_ctx.gcm_ctx.len * 8;
    orig_add_len = env->mode_ctx.gcm_ctx.add_len * 8;

    memcpy(tag, env->mode_ctx.gcm_ctx.base_ectr, tag_len);

    if(orig_len || orig_add_len) {
        memset(work_buf, 0x00, 16);

        DRV_PUT_UINT32_BE(( orig_add_len >> 32 ), work_buf, 0  );
        DRV_PUT_UINT32_BE(( orig_add_len       ), work_buf, 4  );
        DRV_PUT_UINT32_BE(( orig_len     >> 32 ), work_buf, 8  );
        DRV_PUT_UINT32_BE(( orig_len           ), work_buf, 12 );

        for(i = 0; i < 16; i++) env->mode_ctx.gcm_ctx.buf[i] ^= work_buf[i];
        aes_gcm_mult(env, env->mode_ctx.gcm_ctx.buf, env->mode_ctx.gcm_ctx.buf);
        for(i = 0; i < tag_len; i++) tag[i] ^= env->mode_ctx.gcm_ctx.buf[i];
    }

    memset(&env->mode, 0x0, sizeof(aes_env_t) - offsetof(aes_env_t, mode));
    env->state = DRV_STATE_STOP;

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Start AES-CCM LE encrypt
 *
 * @param[in]  cfg                      CCM configuration
 * @param[in]  plain_text               Pointer to plain text
 * @param[out] cipher_text_and_tag      Pointer to cipher text and tag
 * @param[in]  text_len                 Length of plain text
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_aes_ccm_le_encrypt(aes_ccm_le_config_t *cfg, uint8_t *plain_text, uint8_t *cipher_text_and_tag, uint32_t text_len)
{
    om_error_t err;
    uint8_t key_r[16];

    err = OM_ERROR_FAIL;
    OM_ASSERT(text_len != 0 && text_len <= 255);

    for (uint8_t i = 0; i < 16; i++) {
        key_r[i] = cfg->key[15 - i];
    }

    AES_HW_CLK_ENABLE(1);
    OM_AES->CTRL &= ~AES_HW_CTRL_MODE_MASK;
    OM_AES->CCM_KEY[0] = *(uint32_t *)(key_r);
    OM_AES->CCM_KEY[1] = *(uint32_t *)(key_r + 4);
    OM_AES->CCM_KEY[2] = *(uint32_t *)(key_r + 8);
    OM_AES->CCM_KEY[3] = *(uint32_t *)(key_r + 12);

    OM_AES->CCM_NONCE[0] = *(uint32_t *)(cfg->nonce);
    OM_AES->CCM_NONCE[1] = *(uint32_t *)(cfg->nonce + 4);
    OM_AES->CCM_NONCE[2] = *(uint32_t *)(cfg->nonce + 8);
    OM_AES->CCM_NONCE3_LEN = cfg->nonce[12] | text_len << 8;

    OM_AES->CCM_BLOCK1 = (cfg->aad << 16) | 0x0100 ;

    OM_AES->CCM_SRCADDR = (uint32_t)plain_text;
    OM_AES->CCM_DSTADDR = (uint32_t)cipher_text_and_tag;

    OM_AES->CCM_CTRL &= ~AES_HW_CCM_CTRL_MODE_SEL_MASK;
    OM_AES->CCM_CTRL |= AES_HW_CCM_CTRL_ENABLE_MASK;

    OM_AES->CCM_CTRL |= AES_HW_CCM_CTRL_START_MASK;
    while (!(OM_AES->CCM_CTRL & (AES_HW_CCM_CTRL_END_INT_STATUS_MASK)));
    OM_AES->CCM_CTRL |= AES_HW_CCM_CTRL_END_INT_STATUS_MASK;

    err = OM_ERROR_OK;
    if (OM_AES->CCM_CTRL & AES_HW_CCM_CTRL_MICERR_INT_STATUS_MASK) {
        OM_AES->CCM_CTRL |= AES_HW_CCM_CTRL_MICERR_ACK_MASK;
        err = OM_ERROR_FAIL;
    }

    if (OM_AES->CCM_CTRL & AES_HW_CCM_CTRL_ABORT_INT_STATUS_MASK) {
        OM_AES->CCM_CTRL |= AES_HW_CCM_CTRL_ABORT_INT_STATUS_MASK;
        err = OM_ERROR_FAIL;
    }

    AES_HW_CLK_ENABLE(0);

    return err;
}

/**
 *******************************************************************************
 * @brief Start AES-CCM LE decrypt
 * NOTE: Cannot identify MIC error
 *
 * @param[in] cfg                       CCM configuration
 * @param[in] cipher_text_and_tag       Pointer to cipher text and tag
 * @param[out] plain_text               Pointer to plain text
 * @param[in] text_len                  Length of plain text
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_aes_ccm_le_decrypt(aes_ccm_le_config_t *cfg, uint8_t *cipher_text_and_tag, uint8_t *plain_text, uint32_t text_len)
{
    uint8_t key_r[16];

    OM_ASSERT(text_len != 0 && text_len <= 255);

    for (uint8_t i = 0; i < 16; i++) {
        key_r[i] = cfg->key[15 - i];
    }

    AES_HW_CLK_ENABLE(1);
    OM_AES->CTRL &= ~AES_HW_CTRL_MODE_MASK;
    OM_AES->CCM_KEY[0] = *(uint32_t *)(key_r);
    OM_AES->CCM_KEY[1] = *(uint32_t *)(key_r + 4);
    OM_AES->CCM_KEY[2] = *(uint32_t *)(key_r + 8);
    OM_AES->CCM_KEY[3] = *(uint32_t *)(key_r + 12);

    OM_AES->CCM_NONCE[0] = *(uint32_t *)(cfg->nonce);
    OM_AES->CCM_NONCE[1] = *(uint32_t *)(cfg->nonce + 4);
    OM_AES->CCM_NONCE[2] = *(uint32_t *)(cfg->nonce + 8);
    OM_AES->CCM_NONCE3_LEN = cfg->nonce[12] | text_len << 8;

    OM_AES->CCM_BLOCK1 = cfg->aad << 2 | 0x0100 ;

    OM_AES->CCM_SRCADDR = (uint32_t)cipher_text_and_tag;
    OM_AES->CCM_DSTADDR = (uint32_t)plain_text;

    OM_AES->CCM_CTRL |= AES_HW_CCM_CTRL_MODE_SEL_MASK;
    OM_AES->CCM_CTRL |= AES_HW_CCM_CTRL_ENABLE_MASK;

    OM_AES->CCM_CTRL |= AES_HW_CCM_CTRL_START_MASK;
    while (!(OM_AES->CCM_CTRL & (AES_HW_CCM_CTRL_END_INT_STATUS_MASK)));
    OM_AES->CCM_CTRL |= AES_HW_CCM_CTRL_END_INT_STATUS_MASK;

    if (OM_AES->CCM_CTRL & AES_HW_CCM_CTRL_MICERR_INT_STATUS_MASK) {
        OM_AES->CCM_CTRL |= AES_HW_CCM_CTRL_MICERR_ACK_MASK;
    }

    if (OM_AES->CCM_CTRL & AES_HW_CCM_CTRL_ABORT_INT_STATUS_MASK) {
        OM_AES->CCM_CTRL |= AES_HW_CCM_CTRL_ABORT_INT_STATUS_MASK;
    }

    AES_HW_CLK_ENABLE(0);

    return OM_ERROR_OK;
}

om_error_t drv_aes_cmac_start(uint32_t om_aes, aes_cmac_config_t *cfg)
{
    uint8_t key_len;
    aes_env_t *env;

    env = aes_get_env(om_aes);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    env->mode_ctx.cmac_ctx.unprocessed_len = 0U;

    aes_cbc_config_t cbc_cfg = {
        .keybits = cfg->keybits,
        .operation = AES_OP_ENCRYPT,
    };

    key_len = (cfg->keybits == AES_KEYBITS_128) ? 16 : ((cfg->keybits == AES_KEYBITS_192) ? 24 : 32);
    memcpy(cbc_cfg.key, cfg->key, key_len);
    memset(cbc_cfg.iv, 0x0, sizeof(cbc_cfg.iv));

    drv_aes_cbc_start(om_aes, &cbc_cfg);

    return OM_ERROR_OK;
}

om_error_t drv_aes_cmac_crypt_continue(uint32_t om_aes, const uint8_t *input, uint32_t len)
{
    aes_env_t *env;
    uint32_t block_num, unprocessed_len;

    env = aes_get_env(om_aes);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    /* Is there data still to process from the last call, that's greater in
     * size than a block? */
    unprocessed_len = env->mode_ctx.cmac_ctx.unprocessed_len;
    if (unprocessed_len > 0 && len > AES_BLOCK_SIZE - unprocessed_len) {
        memcpy(&env->mode_ctx.cmac_ctx.unprocessd_data[unprocessed_len], input, AES_BLOCK_SIZE - unprocessed_len);
        drv_aes_cbc_crypt_continue(om_aes, env->mode_ctx.cmac_ctx.unprocessd_data, NULL, AES_BLOCK_SIZE);
        input += AES_BLOCK_SIZE - unprocessed_len;
        len   -= AES_BLOCK_SIZE - unprocessed_len;
        env->mode_ctx.cmac_ctx.unprocessed_len = 0U;
    }

    /* block_num is the number of blocks including any final partial block */
    block_num = (len + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE;

    /* Iterate across the input data in block sized chunks, excluding any
     * final partial or complete block */
    if (block_num > 1) {
        drv_aes_cbc_crypt_continue(om_aes, input, NULL, (block_num - 1) * AES_BLOCK_SIZE);
        input += (block_num - 1) * AES_BLOCK_SIZE;
        len   -= (block_num - 1) * AES_BLOCK_SIZE;
    }

    /* If there is data left over that wasn't aligned to a block */
    unprocessed_len = env->mode_ctx.cmac_ctx.unprocessed_len;
    if (len > 0) {
        memcpy(&env->mode_ctx.cmac_ctx.unprocessd_data[unprocessed_len], input, len);
        env->mode_ctx.cmac_ctx.unprocessed_len += len;
    }

    return OM_ERROR_OK;
}

om_error_t drv_aes_cmac_crypt_stop(uint32_t om_aes, uint8_t mac[AES_BLOCK_SIZE])
{
    aes_env_t *env;
    uint8_t K1[16], K2[16], M_last[16];
    uint32_t unprocessed_len;

    env = aes_get_env(om_aes);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    memset(K1, 0, sizeof(K1));
    memset(K2, 0, sizeof(K2));

    aes_cmac_generate_subkeys(om_aes, K1, K2);

    /* Calculate last block */
    unprocessed_len = env->mode_ctx.cmac_ctx.unprocessed_len;
    if (unprocessed_len < AES_BLOCK_SIZE) {
        aes_cmac_pad(M_last, AES_BLOCK_SIZE, env->mode_ctx.cmac_ctx.unprocessd_data, unprocessed_len);
        aes_xor_no_simd(M_last, M_last, K2, AES_BLOCK_SIZE);
    } else {
        /* Last block is complete block */
        aes_xor_no_simd(M_last, env->mode_ctx.cmac_ctx.unprocessd_data, K1, AES_BLOCK_SIZE);
    }
    drv_aes_cbc_crypt_continue(om_aes, M_last, mac, AES_BLOCK_SIZE);

    /* Wipe the generated keys on the stack, and any other transients to avoid
     * side channel leakage */
    memset(K1, 0x0, sizeof(K1));
    memset(K2, 0x0, sizeof(K2));

    env->mode_ctx.cmac_ctx.unprocessed_len = 0U;
    memset(env->mode_ctx.cmac_ctx.unprocessd_data, 0x0, AES_BLOCK_SIZE);
    drv_aes_cbc_crypt_stop(om_aes);

    return OM_ERROR_OK;
}

#endif  /* (RTE_AES_KEEP_RAW_ENGINE) */

#endif  /* (RTE_AES) */

/** @} */