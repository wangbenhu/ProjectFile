/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup UART UART
 * @ingroup  DEVICE
 * @brief    UART register
 * @details  UART register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __UART_REG_H
#define __UART_REG_H


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
 * TYPEDEFS
 */
typedef struct {
    union {
        __I  uint32_t RBR;
        __O  uint32_t THR;
        __IO uint32_t DLL;
    };                                              // offset: 0x00
    union {
        __IO uint32_t DLH;
        __IO uint32_t IER;
    };                                              // offset: 0x04
    union {
        __I  uint32_t IIR;
        __O  uint32_t FCR;
    };                                              // offset: 0x08
    __IO uint32_t LCR;                              // offset: 0x0C
    __IO uint32_t MCR;                              // offset: 0x10
    __I  uint32_t LSR;                              // offset: 0x14
    __I  uint32_t MSR;                              // offset: 0x18
    __IO uint32_t SCR;                              // offset: 0x1C
    __IO uint32_t LPDLL;                            // offset: 0x20
    __IO uint32_t LPDLH;                            // offset: 0x24
         uint8_t Reserved28_2c[0x2c - 0x28 + 4];
    union {
        __I  uint32_t SRBR[16];
        __O  uint32_t STHR[16];
    };                                              // offset: 0x30
    __IO uint32_t FIFOAR;                           // offset: 0x70
    __I  uint32_t TFR;                              // offset: 0x74
    __O  uint32_t RFW;                              // offset: 0x78
    __I  uint32_t USR;                              // offset: 0x7C
    __I  uint32_t TFL;                              // offset: 0x80
    __I  uint32_t RFL;                              // offset: 0x84
    __O  uint32_t SRR;                              // offset: 0x88
    __IO uint32_t SRTS;                             // offset: 0x8C
    __IO uint32_t SBCR;                             // offset: 0x90
    __IO uint32_t SDMAM;                            // offset: 0x94
    __IO uint32_t SFE;                              // offset: 0x98
    __IO uint32_t SRT;                              // offset: 0x9C
    __IO uint32_t STET;                             // offset: 0xA0
    __IO uint32_t HTX;                              // offset: 0xA4
    __O  uint32_t DMASA;                            // offset: 0xA8
         uint8_t Reservedac_f0[0xf0 - 0xac + 4];
    __I uint32_t CPR;                               // offset: 0xF4
    __I uint32_t UCV;                               // offset: 0xF8
    __I uint32_t CTR;                               // offset: 0xFC
} OM_UART_Type;


/*******************************************************************************
 * MACROS
 */
/*
 * definitions for the FIFO Control Register
 */
#define UART_FCR_FIFO_EN                            (1U << 0)       /* Fifo enable */
#define UART_FCR_RST_RCVR                           (1U << 1)       /* Reset the RCVR FIFO */
#define UART_FCR_RST_XMIT                           (1U << 2)       /* Reset the XMIT FIFO */
#define UART_FCR_DMA_SELECT                         (1U << 3)       /* For DMA applications */
#define UART_FCR_TRIGGER_TRANS_FIFO_MASK            (3U << 4)       /* Mask for the XMIT FIFO trigger range */
#define UART_FCR_TRIGGER_TRANS_FIFO_0B              (0U << 4)       /* Mask for trigger set at 0B */
#define UART_FCR_TRIGGER_TRANS_FIFO_2B              (1U << 4)       /* Mask for trigger set at 2B */
#define UART_FCR_TRIGGER_TRANS_FIFO_4B              (2U << 4)       /* Mask for trigger set at FIFO 1/4 */
#define UART_FCR_TRIGGER_TRANS_FIFO_8B              (3U << 4)       /* Mask for trigger set at FIFO 1/2 */
#define UART_FCR_TRIGGER_REC_FIFO_MASK              (3U << 6)       /* Mask for the RCVR FIFO trigger range */
#define UART_FCR_TRIGGER_REC_FIFO_1B                (0U << 6)       /* mask for trigger set at 1B */
#define UART_FCR_TRIGGER_REC_FIFO_4B                (1U << 6)       /* mask for trigger set at FIFO 1/4 */
#define UART_FCR_TRIGGER_REC_FIFO_8B                (2U << 6)       /* mask for trigger set at FIFO 1/2 */
#define UART_FCR_TRIGGER_REC_FIFO_14B               (3U << 6)       /* mask for trigger set at FIFO - 2B */

/*
 * definitions for the Modem Control Register
 */
#define UART_MCR_DTR                                (1U << 0)        /* Data terminal ready */
#define UART_MCR_RTS                                (1U << 1)        /* Request to send */
#define UART_MCR_OUT1                               (1U << 2)        /* Out 1 */
#define UART_MCR_OUT2                               (1U << 3)        /* Out 2 */
#define UART_MCR_LOOP                               (1U << 4)        /* Enable loopback test mode */
#define UART_MCR_AFCE                               (1U << 5)        /* Auto Flow Control Enable */
#define UART_MCR_SIRE                               (1U << 6)        /* SIR mode enable */

/*
 * definitions for the Line Control Register
 *
 * Note: if the word length is 5 bits (UART_LCR_WLEN5), then setting
 * UART_LCR_STOP will select 1.5 stop bits, not 2 stop bits.
 */
#define UART_LCR_DLS_MASK                           (3U << 0)       /* data length select mask */
#define UART_LCR_DLS_5                              (0U << 0)       /* 5 bit data length */
#define UART_LCR_DLS_6                              (1U << 0)       /* 6 bit data length */
#define UART_LCR_DLS_7                              (2U << 0)       /* 7 bit data length */
#define UART_LCR_DLS_8                              (3U << 0)       /* 8 bit data length */
#define UART_LCR_STOP_MASK                          (1U << 2)       /* mask for Number of stop bits */
#define UART_LCR_STOP_1B                            (0U << 2)       /* 1 stop bits */
#define UART_LCR_STOP_2B                            (1U << 2)       /* 1.5 stop bits when DLS is zero, else 2 stop bits */
#define UART_LCR_PEN_MASK                           (1U << 3)       /* mask for parity enable */
#define UART_LCR_PEN                                (1U << 3)
#define UART_LCR_PAR_SEL_MASK                       (1U << 4)       /* mask for parity select */
#define UART_LCR_EPS                                (1U << 4)       /* Even parity select */
#define UART_LCR_DATA8EN                            (1U << 5)       /* UART 9bit data transfer mode, only valid in LIN mode */
#define UART_LCR_SBRK_MASK                          (1U << 6)       /* mask for set break */
#define UART_LCR_SBRK                               (1U << 6)       /* set break */
#define UART_LCR_DLAB_MASK                          (1U << 7)       /* mask for divisor latch access bit */
#define UART_LCR_DLAB                               (1U << 7)       /* divisor latch access bit */
#define UART_LCR_LIN_EN                             (1U << 8)       /* LIN mode enable, only valid in LIN mode */
#define UART_LCR_RX_EN                              (1U << 9)       /* UART Rx enable */
#define UART_LCR_HALF_DUPLEX_EN                     (1U << 10)      /* UART half-duplex */
#define UART_LCR_HALF_DUPLEX_TX_EN                  (1U << 11)      /* UART half-duplex Tx Enable */
#define UART_LCR_TIMEOUT_SW_CLEAR                   (1U << 12)      /* clear timeout state */

/* defaults for LCR */
#define UART_LCR_8N1                                (UART_LCR_DLS_8 | UART_LCR_STOP_1B)     /* data_8_bit + stop_1bit */

/*
 * definitions for the Line Status Register
 */
#define UART_LSR_DR                                 (1U << 0)        /* Data ready */
#define UART_LSR_OE                                 (1U << 1)        /* Overrun */
#define UART_LSR_PE                                 (1U << 2)        /* Parity error */
#define UART_LSR_FE                                 (1U << 3)        /* Framing error */
#define UART_LSR_BI                                 (1U << 4)        /* Break */
#define UART_LSR_THRE                               (1U << 5)        /* Xmit holding register empty */
#define UART_LSR_TEMT                               (1U << 6)        /* transmitter shift register and fifo is empty */
#define UART_LSR_RFE                                (1U << 7)        /* Receive FIFO Error */
#define UART_LSR_LINE_STATUS                        (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE | UART_LSR_BI)

/*
 * definitions for the Modem Status Register
 */
#define UART_MSR_DCTS                               (1U << 0)       /* Delta Clear To Send */
#define UART_MSR_DDSR                               (1U << 1)       /* Delta Data Set Ready */
#define UART_MSR_TERI                               (1U << 2)       /* Trailing Edge Ring Indicator */
#define UART_MSR_DDCD                               (1U << 3)       /* Delta Data Carrier Detect */
#define UART_MSR_CTS                                (1U << 4)       /* Clear To Send */
#define UART_MSR_DSR                                (1U << 5)       /* Data Set Ready */
#define UART_MSR_RI                                 (1U << 6)       /* Ring Indicator */
#define UART_MSR_DCD                                (1U << 7)       /* Data Carrier Detect */

/*
 * definitions for the Interrupt Identification Register
 */
#define UART_IIR_INT_ID_MASK                        (0x1FU << 0)     /* mask for interrupt ID */
#define UART_IIR_MSI                                (0x00U << 0)     /* modem status interrupt */
#define UART_IIR_NO_INT                             (0x01U << 0)     /* no interrupts pending */
#define UART_IIR_THREI                              (0x02U << 0)     /* transmitter holding register empty interrupt */
#define UART_IIR_RDI                                (0x04U << 0)     /* receive data available interrupt */
#define UART_IIR_RLSI                               (0x06U << 0)     /* receive line status interrupt */
#define UART_IIR_BDI                                (0x07U << 0)     /* busy detect interrupt */
#define UART_IIR_CTI                                (0x0CU << 0)     /* character timeout interrupt */
#define UART_IIR_LBDI                               (0x10U << 0)     /* LIN break detect interupt status, only valid in LIN mode*/
#define UART_IIR_FE                                 (0x03U << 6)     /* fifo enable */

/*
 * definitions for the Interrupt Enable Register
 */
#define UART_IER_RDI                                (1U << 0)       /* Enable receiver data interrupt */
#define UART_IER_THREI                              (1U << 1)       /* Enable Transmitter Holding Register Empty Interrupt */
#define UART_IER_RLSI                               (1U << 2)       /* Enable Receiver Line Status Interrupt */
#define UART_IER_MSI                                (1U << 3)       /* Enable Modem Status Interrupt */
#define UART_IER_LBDCF                              (1U << 4)       /* LIN break detect interupt clear, only valid in LIN mode */
#define UART_IER_LBDI                               (1U << 5)       /* Enable Lin break detect Interrupt, only valid in LIN mode */
#define UART_IER_NEI                                (1U << 6)       /* LIN noise error interrupt enable,  only valid in LIN mode */
#define UART_IER_P_THREI                            (1U << 7)       /* Enable Programmable THRE Interrupt */

/* SCR register */
#define UART_SCR_LOOPBACK_EN                        (1U << 1)       /* loop back mode enable */
#define UART_SCR_NE                                 (1U << 0)       /* Noise detect enable, noise error in LIN mode, only valid in LINE mode */

/* USR register */
#define UART_USR_BUSY                               (1U << 0)       /* UART busy */
#define UART_USR_TFNF                               (1U << 1)       /* Transmit FIFO not FULL */
#define UART_USR_TFE                                (1U << 2)       /* Transmit FIFO empty */
#define UART_USR_RFNE                               (1U << 3)       /* Receive FIFO not empty */
#define UART_USR_RFF                                (1U << 4)       /* Receive FIFO full */
#define UART_USR_LIN_BREAK                          (1U << 6)       /* LIN break status, only valid in LIN mode */


#ifdef __cplusplus
}
#endif


#endif  /* __UART_REG_H */


/** @} */
