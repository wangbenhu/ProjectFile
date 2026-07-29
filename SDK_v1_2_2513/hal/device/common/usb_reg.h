/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup USB USB
 * @ingroup  DEVICE
 * @brief    USB Register for OM668x
 * @details  USB Register for OM668x

 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __USB_REG_H
#define __USB_REG_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include "common_reg.h"

#ifdef  __cplusplus
extern "C"
{
#endif
/*******************************************************************************
 * MACROS
 */

/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    __IO uint32_t CTRL;  // DMA control for channel x
    __IO uint32_t ADDR;  // Memory address for channel x
    __IO uint32_t COUNT; // Byte count(32bit) for channel x
         uint32_t RESERVED;
} OM_USB_DMA_CH_Type;

struct _OM_USB_Type {
    /* Common registers */
    __IO uint8_t      FUNCAR;       // 00 Functional address register.
    __IO uint8_t      PCR;          // 01 Power control register.
    __IO uint8_t      TX_ISR1;      // 02 Interrupts status register for Endpoint 0 and the Tx Endpoints 1-7.
    __IO uint8_t      TX_ISR2;      // 03 Interrupts status register for Tx Endpoints 8-15.
    __IO uint8_t      RX_ISR1;      // 04 Interrupts status register for Rx Endpoints 1-7.
    __IO uint8_t      RX_ISR2;      // 05 Interrupts status register for Rx Endpoints 8-15.
    __IO uint8_t      USB_ISR;      // 06 Interrupts status register for USB controller.
    __IO uint8_t      TX_IER1;      // 07 Interrupt enable register for Endpoint 0 and TX endpoints 1-7.
    __IO uint8_t      TX_IER2;      // 08 Interrupt enable register for Endpoint 0 and TX endpoints 8-15
    __IO uint8_t      RX_IER1;      // 09 Interrupt enable register for RX endpoints 1-7.
    __IO uint8_t      RX_IER2;      // 0a Interrupt enable register for RX endpoints 8-15.
    __IO uint8_t      USB_IER;      // 0b Interrupt enable register for each of the interrupts in USB_ISR.
    __IO uint8_t      FN1;          // 0c Lower 8 bits of frame number.
    __IO uint8_t      FN2;          // 0d Higher 3 bit of frame number
    __IO uint8_t      IDX;          // 0e Determines endpoint control/status registers accessed at addresses 0x10 to 0x1F.
    __IO uint8_t      MSR;          // 0f Mode select register.
    __IO uint8_t      TX_MPSR;      // 10 Maximum packet size for transactions through the currently-selected Tx endpoint.
    union {
        __IO uint8_t   CSR0;        // 11 Control and status bits for Endpoint 0
        __IO uint8_t   TX_CSR1;     // 11 Control and status bits for transfers through the currently-selected Tx endpoint
    };
    union {
        __IO uint8_t   FFR0;        // 12 FIFO flush register for Endpoint 0
        __IO uint8_t   TX_CSR2;     // 12 Control bits for transfers through the currently-selected Tx endpoint.
    };
    __IO uint8_t      RX_MPSR;      // 13 Maximum packet size for transactions through the currently-selected Tx endpoint
    __IO uint8_t      RX_CSR1;      // 14 Control and status bits for transfers through the currently-selected Rx endpoint.
    __IO uint8_t      RX_CSR2;      // 15 Control bits for transfers through the currently-selected Rx endpoint.
    union {
        __IO uint8_t   RXN0;        // 16 8 bits of the number of bytes in the received packet on Endpoint 0. INDEX selected
        __IO uint8_t   RXN1;        // 16 Lower 8 bits of the number of bytes in the received packet on selected EP.
    };
    __IO uint8_t      RXN2;         // 17 Higher 3 bits of the number of bytes in the received packet on selected EP.
    __IO uint8_t      TX_TCR;       // 18 Tx type control register. Host mode only.
    union {
        __IO uint8_t    NLR0;       // 19, NAK limit register. INDEX selected
        __IO uint8_t    TX_PICR;    // 19, Tx poling interval control register. Host mode only.
    };
    __IO uint8_t     RX_TCR;        // 1a  Rx type control register. Host mode only.
    __IO uint8_t     RX_PICR;       // 1b, Rx poling interval control register. Host mode only.
    __IO uint8_t     TX_FCR1;       // 1c Tx fifo config register 1.
    __IO uint8_t     TX_FCR2;       // 1d Tx fifo config register 2.
    __IO uint8_t     RX_FCR1;       // 1e Rx fifo config register 1.
    __IO uint8_t     RX_FCR2;       // 1f Rx fifo config register 2.
    /* FIFO port */
    __IO uint32_t    EPx_FIFO[4];   // 20 EPx fifo
         uint8_t     x30_1ff[0x200 - 0x30];
    /* DMA registers */
    __IO uint32_t     DMA_ISR;     // 200 Indicate pending interrupt status of DMA.
    OM_USB_DMA_CH_Type DMA_CH[7];
};

typedef struct _OM_USB_Type OM_USB_Type;
/* USB register file define-------------------------------------------------- */

/*******************  Bit definition for PCR register  **********************/
#define  USB_PCR_ISO_UPDATE                0x80                          /*!<set by CPU in Peripheral mode */
#define  USB_PCR_VBUSVAL                   0x40                          /*!<set when VBUS > VBUSVAL threshold */
#define  USB_PCR_VBUSSESS                  0x20                          /*!<set when VBUS > VBUSSESS threshold */
#define  USB_PCR_VBUSLO                    0x10                          /*!<set when VBUS > VBUSLO threshold */
#define  USB_PCR_RESET                     0x08                          /*!<set when Reset signaling is detected in Peripheral mode;
                                                                             !<set and clear by CPU to generate Reset signaling in Host mode */
#define  USB_PCR_RESUME                    0x04                          /*!<set and clear by CPU to generate Resume signaling in suspend mode */
#define  USB_PCR_SUSPEND_MODE              0x02                          /*!<set if Suspend mode is entered in Peripheral mode;
                                                                             !<set by CPU when Suspend mode is entered in Host mode */
#define  USB_PCR_ENABLE_SUSPEND            0x01                          /*!<set by CPU to enter into Suspend mode when Suspend signaling is recevied in Peripheral mode */

/*******************  Bit definition for INTRUSB register  ********************/
#define  USB_INTR_VBUS_ERROR                 0x80                          /*!<set when VBUS < VBUSVAL in A device */
#define  USB_INTR_SESS_REQ                   0x40                          /*!<set when Session Request signaling is deteted in A device */
#define  USB_INTR_DISCONNECT                 0x20                          /*!<set when a device disconnect is detected in Host mode;
                                                                             !<set when a session ends in Peripheral mode */
#define  USB_INTR_CONNECT                    0x10                          /*!<set when a device connect is detected in Host mode */
#define  USB_INTR_SOF                        0x08                          /*!<set when a new frame starts */
#define  USB_INTR_RESET                      0x04                          /*!<set when Reset signaling is detected in Peripheral mode */
#define  USB_INTR_BABBLE                     0x04                          /*!<set when babble (sends more data > MPS) in Host mode */
#define  USB_INTR_RESUME                     0x02                          /*!<set when Resume signaling is detected in Suspend mode */
#define  USB_INTR_SUSPEND                    0x01                          /*!<set when Suspend signaling is detected in Peripheral mode */

/*******************  Bit definition for MSR register  *********************/
#define  USB_MSR_CID                      0x80                          /*!<set when the ID pin is B device */
#define  USB_MSR_FSDEV                    0x40                          /*!<set when a full-speed device is connected in Host mode */
#define  USB_MSR_LSDEV                    0x20                          /*!<set when a low-speed device is connected in Host mode */
#define  USB_MSR_PUCON                    0x10                          /*!<set when a pull-up resistor on D+ line as a peripheral */
#define  USB_MSR_PDCON                    0x08                          /*!<set when a pull-down resistor on D+ line as a host */
#define  USB_MSR_HOST_MODE                0x04                          /*!<set when act as a Host */
#define  USB_MSR_HOST_REQ                 0x02                          /*!<set by CPU to initiate the Host Negotiation when Suspend mode is entered */
#define  USB_MSR_SESSION                  0x01                          /*!<set/clear by CPU to start/end a session in A device;
                                                                             !<set/clear when a session starts/ends in B device */

/*******************  Bit definition for CSR0 register  ***********************/
#define  USB_FFR0_FLUSHFIFO                  0x01                          /*!<set by CPU to flush EP0's FIFO, self-clearing */
#define  USB_CSR0_P_SERVICED_SETUPEND        0x80                          /*!<set by CPU to clear SetupEnd bit, self-clearing */
#define  USB_CSR0_P_SERVICED_RXPKTRDY        0x40                          /*!<set by CPU to clear RxPktRdy bit, self-clearing */
#define  USB_CSR0_P_SENDSTALL                0x20                          /*!<set by CPU to send STALL handshake, self-clearing */
#define  USB_CSR0_P_SETUPEND                 0x10                          /*!<set and raise interrupt when a control transaction ends, clear by CPU via SERVICED_SETUPEND */
#define  USB_CSR0_P_DATAEND                  0x08                          /*!<set by CPU for Tx/Rx the last data packet or Tx ZLP, self-clearing */
#define  USB_CSR0_P_SENTSTALL                0x04                          /*!<set when a STALL handshake is sent, clear by CPU */
#define  USB_CSR0_P_TXPKTRDY                 0x02                          /*!<set by CPU after loading a data packet into FIFO, self-clearing; raise interrupt when transmitted */
#define  USB_CSR0_P_RXPKTRDY                 0x01                          /*!<set and raise interrupt when a a data packet received, clear by CPU via SERVICED_RXPKTRDY */

#define  USB_FFR0_H_DIS_PING                 0x08
#define  USB_FFR0_H_WR_DATATOGGLE            0x04
#define  USB_FFR0_H_DATATOGGLE               0x02
#define  USB_CSR0_H_NAKTIMEOUT               0x80                          /*!<set when NAK response receive timeout, clear by CPU to allow EP0 exit the halted status */
#define  USB_CSR0_H_STATUSPKT                0x40                          /*!<set and TxPktRdy or ReqPkt by CPU to perform a status stage transaction, while data toggle must be 1 for DATA1 packet */
#define  USB_CSR0_H_REQPKT                   0x20                          /*!<set by CPU to perform an IN transaction, clear when RxPktRdy is set */
#define  USB_CSR0_H_ERROR                    0x10                          /*!<set and raise interrupt when 3 retries of transaction but no response, clear by CPU */
#define  USB_CSR0_H_SETUPPKT                 0x08                          /*!<set and TxPktRdy by CPU to send a SETUP token instead of OUT token */
#define  USB_CSR0_H_RXSTALL                  0x04                          /*!<set when a STALL handshake is received, clear by CPU */
#define  USB_CSR0_H_TXPKTRDY                 0x02                          /*!<USB_CSR0_P_TXPKTRDY */
#define  USB_CSR0_H_RXPKTRDY                 0x01                          /*!<USB_CSR0_P_RXPKTRDY, but clear by CPU directly */

/*******************  Bit definition for TXCSR register  **********************/
#define  USB_TX_CSR2_AUTOSET                  0x80                          /*!<set by CPU, and TxPktRdy will be automatically set when MPS data is loaded into Tx FIFO */
#define  USB_TX_CSR2_ISO                      0x40                          /*!<set by CPU in Peripheral mode to enable Tx EP in ISO in stead of Bulk or Interrupt */
#define  USB_TX_CSR2_MODE                     0x20                          /*!<set by CPU to enable the EP as Tx, clear by CPU as Rx */
#define  USB_TX_CSR2_DMAENAB                  0x10                          /*!<set by CPU to enable DMA for the Tx EP */
#define  USB_TX_CSR2_FRCDATATOG               0x08                          /*!<set by CPU to force the EP data toggle switch */
#define  USB_TX_CSR2_DMAMODE                  0x04                          /*!<set by CPU to select DMA mode 1 (no interrupt), or DMA mode 0 */
#define  USB_TX_CSR2_H_WR_DATATOGGLE          0x02
#define  USB_TX_CSR2_H_DATATOGGLE             0x01

#define  USB_TX_CSR1_P_INCOMPTX               0x80
#define  USB_TX_CSR1_P_CLRDATATOG             0x40                          /*!<set by CPU to reset EP data toggle to 0 */
#define  USB_TX_CSR1_P_SENTSTALL              0x20                          /*!<set when a STALL handshake is sent, while flush Tx FIFO and clear TxPktRdy, clear by CPU */
#define  USB_TX_CSR1_P_SENDSTALL              0x10                          /*!<set by CPU to send STALL handshake to an IN token, clear by CPU to termiate the stall condition */
#define  USB_TX_CSR1_P_FLUSHFIFO              0x08                          /*!<set by CPU to flush Tx FIFO, set twice if double buffered */
#define  USB_TX_CSR1_P_UNDERRUN               0x04                          /*!<set if an IN token is received but TxPktRdy not set, clear by CPU */
#define  USB_TX_CSR1_P_FIFONOTEMPTY           0x02                          /*!<set if at least 1 packet in Tx FIFO */
#define  USB_TX_CSR1_P_TXPKTRDY               0x01                          /*!<USB_CSR0_P_TXPKTRDY */

#define  USB_TX_CSR1_H_NAKTIMEOUT             0x80                          /*!<set when NAK response receive timeout, clear by CPU to allow bulk EP exit the halted status */
#define  USB_TX_CSR1_H_CLRDATATOG             0x40                          /*!<set by CPU to reset EP data toggle to 0 */
#define  USB_TX_CSR1_H_RXSTALL                0x20                          /*!<set when a STALL handshake is received, clear by CPU */
#define  USB_TX_CSR1_H_FLUSHFIFO              0x08                          /*!<USB_TXCSR_P_FLUSHFIFO */
#define  USB_TX_CSR1_H_ERROR                  0x04                          /*!<set and raise interrupt when 3 retries of transaction but no response, clear by CPU */
#define  USB_TX_CSR1_H_FIFONOTEMPTY           0x02                          /*!<USB_TXCSR_P_FIFONOTEMPTY */
#define  USB_TX_CSR1_H_TXPKTRDY               0x01                          /*!<USB_TXCSR_P_TXPKTRDY */

/*******************  Bit definition for RXCSR register  **********************/
#define  USB_RX_CSR2_AUTOCLEAR                0x80                          /*!<set by CPU, and RxPktRdy will be automatically cleared MPS data is unloaded from Rx FIFO */
#define  USB_RX_CSR2_AUTOREQ                  0x40                          /*!<set by CPU in Host mode, and ReqPkt will be automatically set when RxPktRdy is cleared */
#define  USB_RX_CSR2_ISO                      0x40                          /*!<set by CPU in Peripheral mode to enable Rx EP in ISO in stead of Bulk or Interrupt */
#define  USB_RX_CSR2_DMAENAB                  0x20                          /*!<set by CPU to enable DMA for the Rx EP */
#define  USB_RX_CSR2_DMAMODE                  0x10                          /*!<set by CPU to select DMA mode 1 (no interrupt if !MPS), or DMA mode 0 */
#define  USB_RX_CSR1_P_CLRDATATOG             0x80                          /*!<USB_TXCSR_P_CLRDATATOG */
#define  USB_RX_CSR1_P_SENTSTALL              0x40                          /*?<USB_TXCSR_P_SENTSTALL, but not via Tx FIFO? */
#define  USB_RX_CSR1_P_SENDSTALL              0x20                          /*?<USB_TXCSR_P_SENDSTALL, but OUT token? */
#define  USB_RX_CSR1_P_FLUSHFIFO              0x10                          /*!<set by CPU to flush Rx FIFO, set twice if double buffered */
#define  USB_RX_CSR1_P_DATAERROR              0x08                          /*!<set when RxPktRdy set and CRC or bit-stuff error in ISO, clear by CPU via RxPktRdy */
#define  USB_RX_CSR1_P_OVERRUN                0x04                          /*!<set when OUT packet not loaded into Rx FIFO in ISO, clear by CPU */
#define  USB_RX_CSR1_P_FIFOFULL               0x02                          /*!<set if Rx FIFO is full */
#define  USB_RX_CSR1_P_RXPKTRDY               0x01                          /*!<set and raise interrupt when a a data packet received, clear by CPU */

#define  USB_RX_CSR1_H_CLRDATATOG             0x80                          /*!<USB_RXCSR_P_CLRDATATOG */
#define  USB_RX_CSR1_H_RXSTALL                0x40                          /*!<USB_CSR0_H_RXSTALL */
#define  USB_RX_CSR1_H_REQPKT                 0x20                          /*!<USB_CSR0_H_REQPKT */
#define  USB_RX_CSR1_H_FLUSHFIFO              0x10                          /*!<USB_RXCSR_P_FLUSHFIFO */
#define  USB_RX_CSR1_H_NAKTIMEOUT             0x08                          /*!<USB_CSR0_H_NAKTIMEOUT, but EPn in Bulk */
#define  USB_RX_CSR1_H_DATAERROR              0x08                          /*!<USB_RXCSR_P_DATAERROR in ISO */
#define  USB_RX_CSR1_H_ERROR                  0x04                          /*!<USB_CSR0_H_ERROR, but EPn in Bulk or Interrupt */
#define  USB_RX_CSR1_H_FIFOFULL               0x02                          /*!<USB_RXCSR_P_FIFOFULL */
#define  USB_RX_CSR1_H_RXPKTRDY               0x01                          /*!<USB_RXCSR_P_RXPKTRDY */

/*******************  Bit definition for TX/RX_TCR register  ******************/
#define  USB_TYPE_PROTOCOL_SHIFT             4
#define  USB_TYPE_PROTOCOL_ISO               0x10                          /*!<Iso  in Host mode */
#define  USB_TYPE_PROTOCOL_BULK              0x20                          /*!<Bulk in Host mode */
#define  USB_TYPE_PROTOCOL_INTERRUPT         0x30                          /*!<Interrupt in Host mode */
#define  USB_TYPE_EP_SHIFT                   0
#define  USB_TYPE_EP_MASK                    0x0F                          /*!<EP number during enumeration in Host mode */
#define  USB_TYPE_PROTOCOL(type)             ((type) << USB_TYPE_PROTOCOL_SHIFT)
#define  USB_TYPE_EP(ep)                     ((ep) << USB_TYPE_EP_SHIFT)

/*******************  Bit definition for TX/RXFIFO register  ******************/
#define  USB_FIFO_SZ_SHIFT                   13
#define  USB_FIFO_SZ_MASK                    0xE000                        /*!<FIFO size, i.e MPS=2^(SZ+3)*/
#define  USB_FIFO_DPB_SHIFT                  12
#define  USB_FIFO_DPB_MASK                   0x1000                        /*!<set by CPU if double-packet buffering */
#define  USB_FIFO_AD_SHIFT                   0
#define  USB_FIFO_AD_MASK                    0x0FFF                        /*!Start address of the EP FIFO in units of 8-byte */
#define  USB_FIFO_SZ(sz)                     ((29-__CLZ((sz)-1)) << USB_FIFO_SZ_SHIFT)
#define  USB_FIFO_DPB(dpb)                   ((dpb) << USB_FIFO_DPB_SHIFT)
#define  USB_FIFO_AD(ad)                     (((ad) >> 3) << USB_FIFO_AD_SHIFT)

/*******************  Bit definition for NAKLIMITn/0 register  ****************/
#define  USB_NAKLIMIT_DISABLE                0x00                          /*!<0 or 1 disables the NAK timeout function */

/*******************  Bit definition for DMA_ISR register  *******************/
#define  USB_DMA_ISR_CH4                    0x00000008                    /*!<set when DMA channel 4 raise */
#define  USB_DMA_ISR_CH3                    0x00000004                    /*!<set when DMA channel 3 raise */
#define  USB_DMA_ISR_CH2                    0x00000002                    /*!<set when DMA channel 2 raise */
#define  USB_DMA_ISR_CH1                    0x00000001                    /*!<set when DMA channel 1 raise */

/*******************  Bit definition for DMA_CTRL1/2/3/4 register  ************/
#define  USB_DMA_CTRL_BUS_ERROR              0x00008000                    /*!<set when AHB bus error */
#define  USB_DMA_CTRL_MPS_MASK               0x00007F00                    /*!<MPS in uints of 8-byte, in DMA mode 1 */
#define  USB_DMA_CTRL_EP_MASK                0x000000F0                    /*!<EP number in this DMA channel */
#define  USB_DMA_CTRL_INTR_EN                0x00000008                    /*!<0=disable; 1=enable */
#define  USB_DMA_CTRL_DMA_MODE               0x00000004                    /*!<DMA mode 0 or 1 */
#define  USB_DMA_CTRL_DIR                    0x00000002                    /*!<0=Out EP;  1=In EP */
#define  USB_DMA_CTRL_DMA_EN                 0x00000001                    /*!<0=disable; 1=enable */
#define  USB_DMA_CTRL_BUS_POS                15
#define  USB_DMA_CTRL_MPS_POS                8
#define  USB_DMA_CTRL_EP_POS                 4
#define  USB_DMA_CTRL_INTR_EN_POS            3
#define  USB_DMA_CTRL_DMA_MODE_POS           2
#define  USB_DMA_CTRL_DIR_POS                1
#define  USB_DMA_CTRL_DMA_EN_POS             0

/*******************************************************************************
 * EXTERN VARIABLES
 */


/*******************************************************************************
 * EXTERN FUNCTIONS
 */

#ifdef __cplusplus
}
#endif


#endif  /* __USB_REG_H */


/** @} */
