/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup I2S I2S
 * @ingroup  DEVICE
 * @brief    I2S register
 * @details  I2S register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __I2S_REG_H
#define __I2S_REG_H


/*******************************************************************************
 * INCLUDES
 */
#include "common_reg.h"


/*******************************************************************************
 * DEFINES
 */
/** \brief I2S Enable Register Definitions */
#define I2S_IER_MONO_EN_POS                         4U                                                  /*!< I2S IER: Mono mode enable position */
#define I2S_IER_MONO_EN_MASK                        (0x01UL << I2S_IER_MONO_EN_POS)                     /*!< I2S IER: Mono mode enable mask */

#define I2S_IER_DSP_MODE_POS                        3U                                                  /*!< I2S IER: I2S DSP mode position */
#define I2S_IER_DSP_MODE_MASK                       (0x01UL << I2S_IER_DSP_MODE_POS)                    /*!< I2S IER: I2S DSP mode mask */

#define I2S_IER_MODE_POS                            1U                                                  /*!< I2S IER: I2S mode position */
#define I2S_IER_MODE_MASK                           (0x03UL << I2S_IER_MODE_POS)                        /*!< I2S IER: I2S mode mask */

#define I2S_IER_IEN_POS                             0U                                                  /*!< I2S IER: Global enable position */
#define I2S_IER_IEN_MASK                            (0x01UL << I2S_IER_IEN_POS)                         /*!< I2S IER: Global enable mask */

/** \brief I2S Receiver Block Enable Register Definitions */
#define I2S_IRER_RXEN_POS                           0U                                                  /*!< I2S IRER: Receiver enable position */
#define I2S_IRER_RXEN_MASK                          (0x01UL << I2S_IRER_RXEN_POS)                       /*!< I2S IRER: Receiver enable mask */

/** \brief I2S Transmitter Block Enable Register Definitions */
#define I2S_ITER_TXEN_POS                           0U                                                  /*!< I2S ITER: Transmitter enable position */
#define I2S_ITER_TXEN_MASK                          (0x01UL << I2S_ITER_TXEN_POS)                       /*!< I2S ITER: Transmitter enable mask */

/** \brief I2S Receiver Block FIFO Register Definitions */
#define I2S_RXFFR_RXFFR_POS                         0U                                                  /*!< I2S RXFFR: Receiver FIFO reset position */
#define I2S_RXFFR_RXFFR_MASK                        (0x01UL << I2S_RXFFR_RXFFR_POS)                     /*!< I2S RXFFR: Receiver FIFO reset mask */

/** \brief I2S Transmitter Block FIFO Register Definitions */
#define I2S_TXFFR_TXFFR_POS                         0U                                                  /*!< I2S TXFFR: Transmitter FIFO reset position */
#define I2S_TXFFR_TXFFR_MASK                        (0x01UL << I2S_TXFFR_TXFFR_POS)                     /*!< I2S TXFFR: Transmitter FIFO reset mask */

/** \brief I2S Left Reveiver Buffer Definitions */
#define I2S_LRBRx_LRBR_POS                          0U                                                  /*!< I2S LRBR: Left stereo data received serially from receiver channel input read through position */
#define I2S_LRBRx_LRBR_MASK                         (0xFFFFFFFFUL << I2S_LRBRx_LRBR_POS)                /*!< I2S LRBR: Left stereo data received serially from receiver channel input read through mask */

/** \brief I2S Right Reveiver Buffer Definitions */
#define I2S_RRBRx_RRBR_POS                          0U                                                  /*!< I2S RRBR: Right stereo data received serially from receiver channel input read through position */
#define I2S_RRBRx_RRBR_MASK                         (0xFFFFFFFFUL << I2S_RRBRx_RRBR_POS)                /*!< I2S RRBR: Right stereo data received serially from receiver channel input read through mask */

/** \brief I2S Left Transmit Holding Register Definitions */
#define I2S_LTHRx_LTHR_POS                          0U                                                  /*!< I2S LTHR: Left stereo data transmitted serially from transmit channel output write through position */
#define I2S_LTHRx_LTHR_MASK                         (0xFFFFFFFFUL << I2S_LTHRx_LTHR_POS)                /*!< I2S LTHR: Left stereo data transmitted serially from transmit channel output write through mask */

/** \brief I2S Right Transmit Holding Register Definitions */
#define I2S_RTHRx_RTHR_POS                          0U                                                  /*!< I2S RTHR: Right stereo data transmitted serially from transmit channel output write through position */
#define I2S_RTHRx_RTHR_MASK                         (0xFFFFFFFFUL << I2S_RTHRx_RTHR_POS)                /*!< I2S RTHR: Right stereo data transmitted serially from transmit channel output write through mask */

/** \brief I2S Reveiver Enable Register Definitions */
#define I2S_RERx_RXCHEN_POS                         0U                                                  /*!< I2S RER: Receive channel enable position */
#define I2S_RERx_RXCHEN_MASK                        (0x01UL << I2S_RERx_RXCHEN_POS)                     /*!< I2S RER: Receive channel enable mask */

/** \brief I2S Transmit Enable Register Definitions */
#define I2S_TERx_TXCHEN_POS                         0U                                                  /*!< I2S TER: Transmit channel enable position */
#define I2S_TERx_TXCHEN_MASK                        (0x01UL << I2S_TERx_TXCHEN_POS)                     /*!< I2S TER: Transmit channel enable mask */

/** \brief I2S Reveiver Configuration Register Definitions */
#define I2S_RCRx_WLEN_POS                           0U                                                  /*!< I2S RCR: Program desired data resolution of receiver and placed in LSB of LRBR(or RRBR) position */
#define I2S_RCRx_WLEN_MASK                          (0x07UL << I2S_RCRx_WLEN_POS)                       /*!< I2S RCR: Program desired data resolution of receiver and placed in LSB of LRBR(or RRBR) mask */

/** \brief I2S Transmit Configuration Register Definitions */
#define I2S_TCRx_WLEN_POS                           0U                                                  /*!< I2S TCR: Program desired data resolution of transmitter and placed in MSB position */
#define I2S_TCRx_WLEN_MASK                          (0x07UL << I2S_TCRx_WLEN_POS)                       /*!< I2S TCR: Program desired data resolution of transmitter and placed in MSB mask */

/** \brief I2S Interrupt Status Register Definitions */
#define I2S_ISRx_TXFO_POS                           5U                                                  /*!< I2S ISR: Status of Data overrun,attempt to write to full TX FIFO position */
#define I2S_ISRx_TXFO_MASK                          (0x01UL << I2S_ISRx_TXFO_POS)                       /*!< I2S ISR: Status of Data overrun,attempt to write to full TX FIFO mask */

#define I2S_ISRx_TXFE_POS                           4U                                                  /*!< I2S ISR: Status of transmit empty trigger,FIFO is empty position */
#define I2S_ISRx_TXFE_MASK                          (0x01UL << I2S_ISRx_TXFE_POS)                       /*!< I2S ISR: Status of transmit empty trigger,FIFO is empty mask */

#define I2S_ISRx_RXFO_POS                           1U                                                  /*!< I2S ISR: Status of Data overrun,incoming data lost due to a full position */
#define I2S_ISRx_RXFO_MASK                          (0x01UL << I2S_ISRx_RXFO_POS)                       /*!< I2S ISR: Status of Data overrun,incoming data lost due to a full mask */

#define I2S_ISRx_RXDA_POS                           0U                                                  /*!< I2S ISR: Status of receive data available position */
#define I2S_ISRx_RXDA_MASK                          (0x01UL << I2S_ISRx_RXDA_POS)                       /*!< I2S ISR: Status of receive data available mask */

/** \brief I2S Interrupt Mask Register Definitions */
#define I2S_IMRx_TXFOM_POS                          5U                                                  /*!< I2S IMR: Mask TX FIFO overrun interrupt position */
#define I2S_IMRx_TXFOM_MASK                         (0x01UL << I2S_IMRx_TXFOM_POS)                      /*!< I2S IMR: Mask TX FIFO overrun interrupt mask */

#define I2S_IMRx_TXFEM_POS                          4U                                                  /*!< I2S IMR: Mask TX FIFO empty interrupt position */
#define I2S_IMRx_TXFEM_MASK                         (0x01UL << I2S_IMRx_TXFEM_POS)                      /*!< I2S IMR: Mask TX FIFO empty interrupt mask */

#define I2S_IMRx_RXFOM_POS                          1U                                                  /*!< I2S IMR: Mask RX FIFO overrun interrupt position */
#define I2S_IMRx_RXFOM_MASK                         (0x01UL << I2S_IMRx_RXFOM_POS)                      /*!< I2S IMR: Mask RX FIFO overrun interrupt mask */

#define I2S_IMRx_RXDAM_POS                          0U                                                  /*!< I2S IMR: Mask RX FIFO data available interrupt position */
#define I2S_IMRx_RXDAM_MASK                         (0x01UL << I2S_IMRx_RXDAM_POS)                      /*!< I2S IMR: Mask RX FIFO data available interrupt mask */

/** \brief I2S Receive Overrun Register Definitions */
#define I2S_RORx_RXCHO_POS                          0U                                                  /*!< I2S ROR: Read this bit to clear RX FIFO data overrun interrupt position */
#define I2S_RORx_RXCHO_MASK                         (0x01UL << I2S_RORx_RXCHO_POS)                      /*!< I2S ROR: Read this bit to clear RX FIFO data overrun interrupt mask */

/** \brief I2S Transmit Overrun Register Definitions */
#define I2S_TORx_TXCHO_POS                          0U                                                  /*!< I2S TOR: Read this bit to clear TX FIFO data overrun interrupt position */
#define I2S_TORx_TXCHO_MASK                         (0x01UL << I2S_TORx_TXCHO_POS)                      /*!< I2S TOR: Read this bit to clear TX FIFO data overrun interrupt mask */

/** \brief I2S Receive FIFO Configuration Register Definitions */
#define I2S_RFCRx_RXCHDT_POS                        0U                                                  /*!< I2S RFCR: These bit program trigger level in RX FIFO which interrupt generated position */
#define I2S_RFCRx_RXCHDT_MASK                       (0x0FUL << I2S_RFCRx_RXCHDT_POS)                    /*!< I2S RFCR: These bit program trigger level in RX FIFO which interrupt generated mask */

/** \brief I2S Transmit FIFO Configuration Register Definitions */
#define I2S_TFCRx_TXCHET_POS                        0U                                                  /*!< I2S TFCR: Transmit channel empty trigger, these bit program TX FIFO which empty threshold reached interrupt generated position */
#define I2S_TFCRx_TXCHET_MASK                       (0x0FUL << I2S_TFCRx_TXCHET_POS)                    /*!< I2S TFCR: Transmit channel empty trigger, these bit program TX FIFO which empty threshold reached interrupt generated mask */

/** \brief I2S Receive FIFO Flush Definitions */
#define I2S_RFFx_RXCHFR_POS                         0U                                                  /*!< I2S RFF: Receive channel FIFO reset. write 1 to it flush an individual RX FIFO,self clear bit position */
#define I2S_RFFx_RXCHFR_MASK                        (0x01UL << I2S_RFFx_RXCHFR_POS)                     /*!< I2S RFF: Receive channel FIFO reset. write 1 to it flush an individual RX FIFO,self clear bit mask */

/** \brief I2S Transmit FIFO Flush Definitions */
#define I2S_TFFx_TXCHFR_POS                         0U                                                  /*!< I2S TFF: Transmit channel FIFO reset, wirte 1 to it flush TX FIFO, self clear bit position */
#define I2S_TFFx_TXCHFR_MASK                        (0x01UL << I2S_TFFx_TXCHFR_POS)                     /*!< I2S TFF: Transmit channel FIFO reset, wirte 1 to it flush TX FIFO, self clear bit mask */

/** \brief I2S Receive Block DMA Register Definitions */
#define I2S_RXDMA_RXDMA_POS                         0U                                                  /*!< I2S RXDMA: Receiver block DMA register position */
#define I2S_RXDMA_RXDMA_MASK                        (0xFFFFFFFFUL << I2S_RXDMA_RXDMA_POS)               /*!< I2S RXDMA: Receiver block DMA register mask */

/** \brief I2S Reset Receive Block DMA Register Definitions */
#define I2S_RRXDMA_RRXDMA_POS                       0U                                                  /*!< I2S RRXDMA: Reset receiver block DMA register,self-clearing bit position */
#define I2S_RRXDMA_RRXDMA_MASK                      (0x01UL << I2S_RRXDMA_RRXDMA_POS)                   /*!< I2S RRXDMA: Reset receiver block DMA register,self-clearing bit mask */

/** \brief I2S Transmit Block DMA Register Definitions */
#define I2S_TXDMA_TXDMA_POS                         0U                                                  /*!< I2S TXDMA: Transmitter block DMA register position */
#define I2S_TXDMA_TXDMA_MASK                        (0xFFFFFFFFUL << I2S_TXDMA_TXDMA_POS)               /*!< I2S TXDMA: Transmitter block DMA register mask */

/** \brief I2S Reset Transmit Block DMA Register Definitions */
#define I2S_RTXDMA_RTXDMA_POS                       0U                                                  /*!< I2S RTXDMA: Reset transmitter block DMA register,self-clearing bit position */
#define I2S_RTXDMA_RTXDMA_MASK                      (0x01UL << I2S_RTXDMA_RTXDMA_POS)                   /*!< I2S RTXDMA: Reset transmitter block DMA register,self-clearing bit mask */


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    union {
        __I uint32_t LRBR;                  /*!< Left receive buffer */
        __O uint32_t LTHR;                  /*!< Left transmit holding buffer */
    };

    union {
        __I uint32_t RRBR;                  /*!< Right receive buffer */
        __O uint32_t RTHR;                  /*!< Right transmit holding buffer */
    };

    __IO uint32_t   RER;                    /*!< Receive enable register */
    __IO uint32_t   TER;                    /*!< Transmit enable register */
    __IO uint32_t   RCR;                    /*!< Receive configuration register */
    __IO uint32_t   TCR;                    /*!< Transmit configuration register */
    __I  uint32_t   ISR;                    /*!< Interrupt status register */
    __IO uint32_t   IMR;                    /*!< Interrupt mask register */
    __I  uint32_t   ROR;                    /*!< Receive overrun register */
    __I  uint32_t   TOR;                    /*!< Transmit overrun register */
    __IO uint32_t   RFCR;                   /*!< Receive FIFO Configuration register */
    __IO uint32_t   TFCR;                   /*!< Transmit FIFO Configuration register */
    __O  uint32_t   RFF;                    /*!< Receive FIFO flush */
    __O  uint32_t   TFF;                    /*!< Transmit FIFO flush */

    __IO uint32_t   REV[2];                 /*!< Reserved */
} OM_I2S_CH_Type;

typedef struct
{
    __IO uint32_t   IER;                    /*!< offset:0x0000, AHB I2S enable register */
    __IO uint32_t   IRER;                   /*!< offset:0x0004, I2S receiver block enable register */
    __IO uint32_t   ITER;                   /*!< offset:0x0008, I2S transmitter block enable register */
    __IO uint32_t   CER;                    /*!< offset:0x000C, Clock enable register */
    __IO uint32_t   CCR;                    /*!< offset:0x0010, Clock configuration register */
    __IO uint32_t   RXFFR;                  /*!< offset:0x0014, Receiver block FIFO register */
    __IO uint32_t   TXFFR;                  /*!< offset:0x0018, Transmitter block FIFO register */

    __IO uint32_t   REV0;                   /*!< offset:0x001C, Reserved */

    OM_I2S_CH_Type  CH0;                    /*!< offset:0x0020, Channel 0 */
    OM_I2S_CH_Type  CH1;                    /*!< offset:0x0060, Channel 1 */
    OM_I2S_CH_Type  CH2;                    /*!< offset:0x00A0, Channel 2 */
    OM_I2S_CH_Type  CH3;                    /*!< offset:0x00E0, Channel 3 */

    __IO uint32_t   REV1[40];               /*!< offset:0x0120, Reserved */

    __I  uint32_t   RXDMA;                  /*!< offset:0x01C0, Received block DMA register */
    __O  uint32_t   RRXDMA;                 /*!< offset:0x01C4, Reset receiver block DMA register */
    __O  uint32_t   TXDMA;                  /*!< offset:0x01C8, Transmitter block DMA register */
    __O  uint32_t   RTXDMA;                 /*!< offset:0x01CC, Reset transmitter block DMA register */

    __IO uint32_t   REV2[8];                /*!< offset:0x01D0, Reserved */

    __I  uint32_t   I2S_COMP_PARAM_2;       /*!< offset:0x01F0, Component parameter 2 register */
    __I  uint32_t   I2S_COMP_PARAM_1;       /*!< offset:0x01F4, Component parameter 1 register */
    __I  uint32_t   I2S_COMP_VERSION;       /*!< offset:0x01F8, Component version ID */
    __I  uint32_t   I2S_COMP_TYPE;          /*!< offset:0x01FC, DesignWare component type */
} OM_I2S_Type;


#endif /* __I2S_REG_H */


/** @} */