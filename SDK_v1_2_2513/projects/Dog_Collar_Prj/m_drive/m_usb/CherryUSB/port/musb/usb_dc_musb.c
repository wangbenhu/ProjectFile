/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "usbd_core.h"
#include "usb_musb_reg.h"

#define HWREG(x) \
    (*((volatile uint32_t *)(x)))
#define HWREGH(x) \
    (*((volatile uint16_t *)(x)))
#define HWREGB(x) \
    (*((volatile uint8_t *)(x)))

#define USB_BASE (g_usbdev_bus[0].reg_base)

#define CONFIG_MUSB_SUSPEND_ENABLE

#if defined(CONFIG_USB_MUSB_CUSTOM)
#include "musb_custom.h"
#else
// OM_MUSB REG_ADDR diff with CherryUSB
#define MUSB_FADDR_OFFSET 0x00
#define MUSB_POWER_OFFSET 0x01
#define MUSB_TXIS_OFFSET  0x02
#define MUSB_RXIS_OFFSET  0x04
#define MUSB_TXIE_OFFSET  0x07
#define MUSB_RXIE_OFFSET  0x09
#define MUSB_IS_OFFSET    0x06
#define MUSB_IE_OFFSET    0x0B

#define MUSB_EPIDX_OFFSET 0x0E

#define MUSB_IND_TXMAP_OFFSET   0x10
#define MUSB_IND_TXCSRL_OFFSET  0x11
#define MUSB_IND_TXCSRH_OFFSET  0x12
#define MUSB_IND_RXMAP_OFFSET   0x13
#define MUSB_IND_RXCSRL_OFFSET  0x14
#define MUSB_IND_RXCSRH_OFFSET  0x15
#define MUSB_IND_RXCOUNT_OFFSET 0x16

#define MUSB_FIFO_OFFSET 0x20

#define MUSB_DEVCTL_OFFSET 0x0F

// OM_MUSB FIFO diff with CherryUSB
#define MUSB_TXFIFOCR_OFFSET  0x1C
#define MUSB_RXFIFOCR_OFFSET  0x1E

#define MUSB_FIFOCR_SZ_SHIFT  13
#define MUSB_FIFOCR_SZ_MASK   0xE000                        /*!<FIFO size, i.e MPS=2^(SZ+3)*/
#define MUSB_FIFOCR_DPB_SHIFT 12
#define MUSB_FIFOCR_DPB_MASK  0x1000                        /*!<set by CPU if double-packet buffering */
#define MUSB_FIFOCR_AD_SHIFT  0
#define MUSB_FIFOCR_AD_MASK   0x0FFF                        /*!Start address of the EP FIFO in units of 8-byte */
#define MUSB_FIFOCR_REG(addr, size, dpb)    ((addr)<<MUSB_FIFOCR_AD_SHIFT | (size)<<MUSB_FIFOCR_SZ_SHIFT | (dpb)<<MUSB_FIFOCR_DPB_SHIFT)
#define MUSB_TXFIFOCR_SZ        (HWREGH(USB_BASE + MUSB_TXFIFOCR_OFFSET)>>MUSB_FIFOCR_SZ_SHIFT)
#define MUSB_RXFIFOCR_SZ        (HWREGH(USB_BASE + MUSB_RXFIFOCR_OFFSET)>>MUSB_FIFOCR_SZ_SHIFT)

#endif

#define USB_FIFO_BASE(ep_idx) (USB_BASE + MUSB_FIFO_OFFSET + 0x4 * ep_idx)

enum test_mode_type {
    USB_TEST_J            = 1,
    USB_TEST_K            = 2,
    USB_TEST_SE0_NAK      = 3,
    USB_TEST_PACKET       = 4,
    USB_TEST_FORCE_ENABLE = 5,
};

typedef enum {
    USB_EP0_STATE_SETUP = 0x0,      /**< SETUP DATA */
    USB_EP0_STATE_IN_DATA = 0x1,    /**< IN DATA */
    USB_EP0_STATE_OUT_DATA = 0x3,   /**< OUT DATA */
    USB_EP0_STATE_IN_STATUS = 0x4,  /**< IN status */
    USB_EP0_STATE_OUT_STATUS = 0x5, /**< OUT status */
    USB_EP0_STATE_IN_ZLP = 0x6,     /**< OUT status */
    USB_EP0_STATE_STALL = 0x7,      /**< STALL status */
} ep0_state_t;

/* Endpoint state */
struct musb_ep_state {
    uint16_t ep_mps;    /* Endpoint max packet size */
    uint8_t ep_type;    /* Endpoint type */
    uint8_t ep_stalled; /* Endpoint stall flag */
    uint8_t ep_enable;  /* Endpoint enable */
#if CONFIG_USBDEV_DMACH_NUM>0
    uint8_t dma_ch;
#endif
    uint8_t *xfer_buf;
    uint32_t xfer_len;
    uint32_t actual_xfer_len;
};

/* Driver state */
struct musb_udc {
    volatile uint8_t dev_addr;
    __attribute__((aligned(32))) struct usb_setup_packet setup;
    struct musb_ep_state in_ep[CONFIG_USBDEV_EP_NUM];  /*!< IN endpoint parameters*/
    struct musb_ep_state out_ep[CONFIG_USBDEV_EP_NUM]; /*!< OUT endpoint parameters */
#if CONFIG_USBDEV_DMACH_NUM>0
    uint8_t dma_ch2ep[CONFIG_USBDEV_DMACH_NUM];
#endif
} g_musb_udc;

static volatile uint8_t usb_ep0_state = USB_EP0_STATE_SETUP;

/* get current active ep */
static uint8_t musb_get_active_ep(void)
{
    return HWREGB(USB_BASE + MUSB_EPIDX_OFFSET);
}

/* set the active ep */
static void musb_set_active_ep(uint8_t ep_index)
{
    HWREGB(USB_BASE + MUSB_EPIDX_OFFSET) = ep_index;
}

static void musb_write_packet(uint8_t ep_idx, uint8_t *buffer, uint16_t len)
{
    uint32_t *buf32;
    uint8_t *buf8;
    uint32_t count32;
    uint32_t count8;
    int i;

    if ((uint32_t)buffer & 0x03) {
        buf8 = buffer;
        for (i = 0; i < len; i++) {
            HWREGB(USB_FIFO_BASE(ep_idx)) = *buf8++;
        }
    } else {
        count32 = len >> 2;
        count8 = len & 0x03;

        buf32 = (uint32_t *)buffer;

        while (count32--) {
            HWREG(USB_FIFO_BASE(ep_idx)) = *buf32++;
        }

        buf8 = (uint8_t *)buf32;

        while (count8--) {
            HWREGB(USB_FIFO_BASE(ep_idx)) = *buf8++;
        }
    }
}

static void musb_read_packet(uint8_t ep_idx, uint8_t *buffer, uint16_t len)
{
    uint32_t *buf32;
    uint8_t *buf8;
    uint32_t count32;
    uint32_t count8;
    int i;

    USB_LOG_DBG("HW EP%d read %dbytes\r\n", ep_idx, len);

    if ((uint32_t)buffer & 0x03) {
        buf8 = buffer;
        for (i = 0; i < len; i++) {
            *buf8++ = HWREGB(USB_FIFO_BASE(ep_idx));
        }
    } else {
        count32 = len >> 2;
        count8 = len & 0x03;

        buf32 = (uint32_t *)buffer;

        while (count32--) {
            *buf32++ = HWREG(USB_FIFO_BASE(ep_idx));
        }

        buf8 = (uint8_t *)buf32;

        while (count8--) {
            *buf8++ = HWREGB(USB_FIFO_BASE(ep_idx));
        }
    }
}

static uint32_t musb_get_fifo_size(uint16_t mps, uint16_t *used)
{
    uint32_t size;

    for (uint8_t i = USB_TXFIFOSZ_SIZE_8; i <= USB_TXFIFOSZ_SIZE_2048; i++) {
        size = (8 << i);
        if (mps <= size) {
            *used = size;
            return i;
        }
    }

    *used = 0;
    return USB_TXFIFOSZ_SIZE_8;
}

static uint32_t usbd_musb_fifo_config(struct musb_fifo_cfg *cfg, uint32_t offset)
{
    uint16_t fifo_used;
    uint8_t c_size;
    uint16_t c_off;

    c_off = offset >> 3;
    c_size = musb_get_fifo_size(cfg->maxpacket, &fifo_used);

    musb_set_active_ep(cfg->ep_num);

    switch (cfg->style) {
        case FIFO_TX:
            //HWREGB(USB_BASE + MUSB_TXFIFOSZ_OFFSET) = c_size & 0x0f;
            //HWREGH(USB_BASE + MUSB_TXFIFOADD_OFFSET) = c_off;
            HWREGH(USB_BASE + MUSB_TXFIFOCR_OFFSET) = MUSB_FIFOCR_REG(c_off, c_size, 0);
            break;
        case FIFO_RX:
            //HWREGB(USB_BASE + MUSB_RXFIFOSZ_OFFSET) = c_size & 0x0f;
            //HWREGH(USB_BASE + MUSB_RXFIFOADD_OFFSET) = c_off;
            HWREGH(USB_BASE + MUSB_RXFIFOCR_OFFSET) = MUSB_FIFOCR_REG(c_off, c_size, 0);
            break;
        case FIFO_TXRX:
            //HWREGB(USB_BASE + MUSB_TXFIFOSZ_OFFSET) = c_size & 0x0f;
            //HWREGH(USB_BASE + MUSB_TXFIFOADD_OFFSET) = c_off;
            //HWREGB(USB_BASE + MUSB_RXFIFOSZ_OFFSET) = c_size & 0x0f;
            //HWREGH(USB_BASE + MUSB_RXFIFOADD_OFFSET) = c_off;
            HWREGH(USB_BASE + MUSB_TXFIFOCR_OFFSET) = MUSB_FIFOCR_REG(c_off, c_size, 0);
            HWREGH(USB_BASE + MUSB_RXFIFOCR_OFFSET) = MUSB_FIFOCR_REG(c_off, c_size, 0);
            break;

        default:
            break;
    }

    return (offset + fifo_used);
}

#if CONFIG_USBDEV_DMACH_NUM>0
static void musb_dma_ch_config(struct musb_fifo_cfg *cfg)
{
    OM_ASSERT(cfg->ep_num < CONFIG_USBDEV_EP_NUM);
    OM_ASSERT(cfg->tx_dma_ch == DMA_NONE || cfg->tx_dma_ch < CONFIG_USBDEV_DMACH_NUM);
    OM_ASSERT(cfg->rx_dma_ch == DMA_NONE || cfg->rx_dma_ch < CONFIG_USBDEV_DMACH_NUM);
    OM_ASSERT(cfg->ep_num != 0 || (cfg->rx_dma_ch == DMA_NONE && cfg->tx_dma_ch == DMA_NONE));

    g_musb_udc.in_ep[cfg->ep_num].dma_ch = cfg->tx_dma_ch;
    g_musb_udc.out_ep[cfg->ep_num].dma_ch = cfg->rx_dma_ch;

    if (cfg->rx_dma_ch != DMA_NONE) {
        g_musb_udc.dma_ch2ep[cfg->rx_dma_ch] = cfg->ep_num | USB_EP_DIR_OUT;
        OM_USB->DMA_CH[cfg->rx_dma_ch].CTRL = 0;
    }

    if (cfg->tx_dma_ch != DMA_NONE) {
        g_musb_udc.dma_ch2ep[cfg->tx_dma_ch] = cfg->ep_num | USB_EP_DIR_IN;
        OM_USB->DMA_CH[cfg->tx_dma_ch].CTRL = 0;
    }
}

static void musb_dma_in_setup(uint8_t ep)
{
    OM_ASSERT(ep < CONFIG_USBDEV_EP_NUM);

    uint8_t ch = g_musb_udc.in_ep[ep].dma_ch;

    USB_LOG_DBG("EP%d DMACH%d in\r\n", ep, ch);

    uint16_t mps8 = g_musb_udc.in_ep[ep].ep_mps >> 3;
    //uint16_t ep_type = g_musb_udc.in_ep[ep].ep_type;
    uint8_t mode = 1; // only use mode1

    OM_USB->DMA_CH[ch].ADDR  = (uint32_t)g_musb_udc.in_ep[ep].xfer_buf;
    OM_USB->DMA_CH[ch].COUNT = g_musb_udc.in_ep[ep].xfer_len;
    OM_USB->DMA_CH[ch].CTRL = (mps8 << USB_DMA_CTRL_MPS_POS) |
                              (ep << USB_DMA_CTRL_EP_POS) |
                              (1 << USB_DMA_CTRL_INTR_EN_POS) |
                              (mode << USB_DMA_CTRL_DMA_MODE_POS) |
                              (1 << USB_DMA_CTRL_DIR_POS) |
                              (1 << USB_DMA_CTRL_DMA_EN_POS);

    OM_USB->TX_IER1 |= 1 << ep;
    OM_USB->TX_CSR2 |= USB_TX_CSR2_AUTOSET | USB_TX_CSR2_DMAENAB | USB_TX_CSR2_DMAMODE;
}

static void musb_dma_out_setup(uint8_t ep)
{
    OM_ASSERT(ep < CONFIG_USBDEV_EP_NUM);

    uint8_t ch = g_musb_udc.out_ep[ep].dma_ch;

    USB_LOG_DBG("EP%d DMACH%d out\r\n", ep, ch);

    uint16_t mps8 = g_musb_udc.out_ep[ep].ep_mps >> 3;
    //uint16_t ep_type = g_musb_udc.out_ep[ep].ep_type;
    uint8_t mode = 1; // only use mode1

    OM_USB->DMA_CH[ch].ADDR  = (uint32_t)g_musb_udc.out_ep[ep].xfer_buf;
    OM_USB->DMA_CH[ch].COUNT = g_musb_udc.out_ep[ep].xfer_len;
    OM_USB->DMA_CH[ch].CTRL = (mps8 << USB_DMA_CTRL_MPS_POS) |
                              (ep << USB_DMA_CTRL_EP_POS) |
                              (1 << USB_DMA_CTRL_INTR_EN_POS) |
                              (mode << USB_DMA_CTRL_DMA_MODE_POS) |
                              (0 << USB_DMA_CTRL_DIR_POS) |
                              (1 << USB_DMA_CTRL_DMA_EN_POS);

    OM_USB->RX_IER1 |= 1 << ep;
    OM_USB->RX_CSR2 |= USB_RX_CSR2_AUTOCLEAR | USB_RX_CSR2_DMAENAB | USB_RX_CSR2_DMAMODE;

    // Must set RXPKTRDY at last. Before that, NACK to HOST
    OM_USB->RX_CSR1 &= ~USB_RX_CSR1_P_RXPKTRDY;
}

static void musb_dma_in_handler(uint8_t ep)
{
    OM_ASSERT(ep < CONFIG_USBDEV_EP_NUM);

    uint8_t ch = g_musb_udc.in_ep[ep].dma_ch;

    uint16_t left_len = g_musb_udc.in_ep[ep].xfer_len % g_musb_udc.in_ep[ep].ep_mps;
    g_musb_udc.in_ep[ep].actual_xfer_len = g_musb_udc.in_ep[ep].xfer_len - left_len;
    g_musb_udc.in_ep[ep].xfer_len = left_len;
    g_musb_udc.in_ep[ep].xfer_buf += g_musb_udc.in_ep[ep].actual_xfer_len;

    USB_LOG_DBG("DMA EP%d write %dbytes\r\n", ep, g_musb_udc.in_ep[ep].actual_xfer_len);

    OM_USB->DMA_CH[ch].CTRL = 0; /*lint !e661*/
    OM_USB->TX_CSR2 &= ~(USB_TX_CSR2_AUTOSET | USB_TX_CSR2_DMAENAB /*| USB_TX_CSR2_DMAMODE*/);

    if (g_musb_udc.in_ep[ep].xfer_len == 0) {
        // if xfer_len=0, do not send empty PKT
        OM_USB->TX_IER1 &= ~(1 << ep);
        usbd_event_ep_in_complete_handler(0, ep | 0x80, g_musb_udc.in_ep[ep].actual_xfer_len);
    } else {
        // SET TXRDY to send left PKT or empty PKT, wait fifo_in_handler complete
        OM_USB->TX_CSR1 = USB_TX_CSR1_P_TXPKTRDY;
    }
}

static void musb_dma_out_handler(uint8_t ep, uint16_t fifo_len)
{
    OM_ASSERT(ep < CONFIG_USBDEV_EP_NUM);

    uint8_t ch = g_musb_udc.out_ep[ep].dma_ch;

    OM_ASSERT(OM_USB->RX_CSR1 & USB_RX_CSR1_P_RXPKTRDY);

    //
    // FIXME
    //
    // MOVE to usbd_ep_start_read,
    // default all RXIE is opened, digital rx-irq is edge trigger, disable may lead lost irq
    // Before SET RXRDY, NACK to HOST
    // BUT: When DMA interrupt, the RXPKTRDY does not set to 1, so can not NACK to HOST
    //OM_USB->RX_CSR1 &= ~USB_RX_CSR1_P_RXPKTRDY;

    OM_USB->DMA_CH[ch].CTRL = 0; /*lint !e661*/
    OM_USB->RX_IER1 &= ~(1 << ep);
    OM_USB->RX_CSR2 &= ~(USB_RX_CSR2_AUTOCLEAR | USB_RX_CSR2_DMAENAB/* | USB_RX_CSR2_DMAMODE*/);

    uint8_t *cur_rxbuf = (uint8_t *)OM_USB->DMA_CH[ch].ADDR;

    uint16_t cmpl_len = cur_rxbuf - g_musb_udc.out_ep[ep].xfer_buf;
    if(cmpl_len > g_musb_udc.out_ep[ep].xfer_len) {
        USB_LOG_ERR("DMA rx overflow: %d>%d\r\n", cmpl_len + fifo_len, g_musb_udc.out_ep[ep].xfer_len);
        cmpl_len = g_musb_udc.out_ep[ep].xfer_len;
        return;
    }

    uint16_t left_space_len = g_musb_udc.out_ep[ep].xfer_len - cmpl_len;
    if (left_space_len < fifo_len) {
        USB_LOG_ERR("DMA rx overflow: %d>%d\r\n", cmpl_len + fifo_len, g_musb_udc.out_ep[ep].xfer_len);
        fifo_len = left_space_len;
    }

    USB_LOG_DBG("DMA EP%d read %dbytes\r\n", ep, cmpl_len);

    if (fifo_len)
        musb_read_packet(ep, cur_rxbuf, fifo_len);

    g_musb_udc.out_ep[ep].actual_xfer_len = cmpl_len + fifo_len;
    g_musb_udc.out_ep[ep].xfer_len -= g_musb_udc.out_ep[ep].actual_xfer_len;
    usbd_event_ep_out_complete_handler(0, ep, g_musb_udc.out_ep[ep].actual_xfer_len);
}
#endif

static void musb_fifo_in_setup(uint8_t ep_idx)
{
    OM_ASSERT(ep_idx < CONFIG_USBDEV_EP_NUM);

    uint8_t *data = g_musb_udc.in_ep[ep_idx].xfer_buf;
    uint32_t data_len = g_musb_udc.in_ep[ep_idx].xfer_len;

    data_len = MIN(data_len, g_musb_udc.in_ep[ep_idx].ep_mps);

    musb_write_packet(ep_idx, (uint8_t *)data, data_len);
    HWREGH(USB_BASE + MUSB_TXIE_OFFSET) |= (1 << ep_idx);

    if (ep_idx == 0x00) {
        usb_ep0_state = USB_EP0_STATE_IN_DATA;
        if (data_len < g_musb_udc.in_ep[ep_idx].ep_mps) {
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = (USB_CSRL0_TXRDY | USB_CSRL0_DATAEND);
        } else {
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_CSRL0_TXRDY;
        }
    } else {
        HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_TXCSRL1_TXRDY;
    }
}

static void musb_fifo_out_setup(uint8_t ep_idx)
{
    HWREGH(USB_BASE + MUSB_RXIE_OFFSET) |= (1 << ep_idx);

    // Must set RXPKTRDY at last. Before that, NACK to HOST
    HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) &= ~(USB_RXCSRL1_RXRDY);
}

static void musb_fifo_in_handler(uint8_t ep_idx)
{
    OM_ASSERT(ep_idx < CONFIG_USBDEV_EP_NUM);

    if (g_musb_udc.in_ep[ep_idx].xfer_len > g_musb_udc.in_ep[ep_idx].ep_mps) {
        g_musb_udc.in_ep[ep_idx].xfer_buf += g_musb_udc.in_ep[ep_idx].ep_mps;
        g_musb_udc.in_ep[ep_idx].actual_xfer_len += g_musb_udc.in_ep[ep_idx].ep_mps;
        g_musb_udc.in_ep[ep_idx].xfer_len -= g_musb_udc.in_ep[ep_idx].ep_mps;
    } else {
        g_musb_udc.in_ep[ep_idx].xfer_buf += g_musb_udc.in_ep[ep_idx].xfer_len;
        g_musb_udc.in_ep[ep_idx].actual_xfer_len += g_musb_udc.in_ep[ep_idx].xfer_len;
        g_musb_udc.in_ep[ep_idx].xfer_len = 0;
    }

    if (g_musb_udc.in_ep[ep_idx].xfer_len == 0) {
        HWREGH(USB_BASE + MUSB_TXIE_OFFSET) &= ~(1 << ep_idx);
        usbd_event_ep_in_complete_handler(0, ep_idx | 0x80, g_musb_udc.in_ep[ep_idx].actual_xfer_len);
    } else {
        uint16_t write_count = MIN(g_musb_udc.in_ep[ep_idx].xfer_len, g_musb_udc.in_ep[ep_idx].ep_mps);
        musb_write_packet(ep_idx, g_musb_udc.in_ep[ep_idx].xfer_buf, write_count);
        HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_TXCSRL1_TXRDY;
    }
}

static void musb_fifo_out_handler(uint8_t ep_idx)
{
    OM_ASSERT(ep_idx < CONFIG_USBDEV_EP_NUM);

    uint16_t read_count = HWREGH(USB_BASE + MUSB_IND_RXCOUNT_OFFSET);

    if (read_count > g_musb_udc.out_ep[ep_idx].xfer_len) {
        USB_LOG_ERR("rx overflow: %d>%d\r\n", read_count, g_musb_udc.out_ep[ep_idx].xfer_len);
    } else {
        musb_read_packet(ep_idx, g_musb_udc.out_ep[ep_idx].xfer_buf, read_count);
        // MOVE to usbd_ep_start_read,
        // default all RXIE is opened, digital rx-irq is edge trigger, disable may lead lost irq
        // Before SET RXRDY, NACK to HOST
        //HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) &= ~(USB_RXCSRL1_RXRDY);

        g_musb_udc.out_ep[ep_idx].xfer_buf += read_count;
        g_musb_udc.out_ep[ep_idx].actual_xfer_len += read_count;
        g_musb_udc.out_ep[ep_idx].xfer_len -= read_count;

        if ((read_count < g_musb_udc.out_ep[ep_idx].ep_mps) || (g_musb_udc.out_ep[ep_idx].xfer_len == 0)) {
            HWREGH(USB_BASE + MUSB_RXIE_OFFSET) &= ~(1 << ep_idx);
            g_musb_udc.out_ep[ep_idx].xfer_len = 0;
            usbd_event_ep_out_complete_handler(0, ep_idx, g_musb_udc.out_ep[ep_idx].actual_xfer_len);
        } else {
            // Clear RXRDY
            HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) &= ~(USB_RXCSRL1_RXRDY);
        }
    }
}

static void musb_fifo_flush(uint8_t ep_idx)
{
    if (ep_idx == 0) {
        OM_USB->FFR0 |= USB_FFR0_FLUSHFIFO;
        while ((OM_USB->FFR0 & USB_FFR0_FLUSHFIFO) != 0);
    } else {
        OM_USB->RX_CSR1 |= USB_RX_CSR1_P_FLUSHFIFO | USB_RX_CSR1_P_CLRDATATOG;
        while ((OM_USB->RX_CSR1 & USB_RX_CSR1_P_FLUSHFIFO) != 0);
        OM_USB->TX_CSR1 |= USB_TX_CSR1_P_FLUSHFIFO | USB_TX_CSR1_P_CLRDATATOG;
        while ((OM_USB->TX_CSR1 & USB_TX_CSR1_P_FLUSHFIFO) != 0);
    }
}

static void musb_reset_handler(void)
{
    struct musb_fifo_cfg *cfg;
    uint8_t cfg_num;

    memset(&g_musb_udc, 0, sizeof(struct musb_udc));

    usbd_event_reset_handler(0);

    HWREGH(USB_BASE + MUSB_TXIE_OFFSET) = USB_TXIE_EP0;
    HWREGH(USB_BASE + MUSB_RXIE_OFFSET) = 0;

    // Default HW-Reset RXMAP=0, change RXMAP to fifo size
    cfg_num = usbd_get_musb_fifo_cfg(&cfg);
    for (uint8_t i = 0; i < cfg_num; i++) {
        musb_set_active_ep(cfg[i].ep_num);
        HWREGH(USB_BASE + MUSB_IND_RXMAP_OFFSET) = cfg[i].maxpacket >> 3;
#if CONFIG_USBDEV_DMACH_NUM>0
        musb_dma_ch_config(&cfg[i]);
#endif
        musb_fifo_flush(i);
    }
    musb_set_active_ep(0);

#ifdef CONFIG_MUSB_SUSPEND_ENABLE
    // Enable Suspend function
    HWREGB(USB_BASE + MUSB_POWER_OFFSET) |= USB_POWER_EN_SUSPEND;
#endif

    usb_ep0_state = USB_EP0_STATE_SETUP;
}

__WEAK void usb_dc_low_level_init(void)
{
}

__WEAK void usb_dc_low_level_deinit(void)
{
}

__WEAK void usb_suspend_low_level_init(void)
{
}

__WEAK void usb_resume_low_level_init(void)
{
}

int usb_dc_init(uint8_t busid)
{
    uint16_t offset = 0;
    uint8_t cfg_num;
    struct musb_fifo_cfg *cfg;

    usb_dc_low_level_init();

#ifdef CONFIG_USB_HS
    HWREGB(USB_BASE + MUSB_POWER_OFFSET) |= USB_POWER_HSENAB;
#else
    HWREGB(USB_BASE + MUSB_POWER_OFFSET) &= ~USB_POWER_HSENAB;
#endif

    musb_set_active_ep(0);
    HWREGB(USB_BASE + MUSB_FADDR_OFFSET) = 0;

    HWREGB(USB_BASE + MUSB_DEVCTL_OFFSET) |= USB_DEVCTL_SESSION;

    cfg_num = usbd_get_musb_fifo_cfg(&cfg);

    for (uint8_t i = 0; i < cfg_num; i++) {
        offset = usbd_musb_fifo_config(&cfg[i], offset);
#if CONFIG_USBDEV_DMACH_NUM>0
        musb_dma_ch_config(&cfg[i]);
#endif
        musb_fifo_flush(i);
    }

    if (offset > usb_get_musb_ram_size()) {
        USB_LOG_ERR("offset:%d is overflow, please check your table\r\n", offset);
        while (1) {
        }
    }

    /* Enable USB interrupts */
    HWREGB(USB_BASE + MUSB_IE_OFFSET) = USB_IE_RESET | USB_IE_SUSPND | USB_IE_RESUME | USB_IE_DISCON | USB_IE_CONN;
    HWREGH(USB_BASE + MUSB_TXIE_OFFSET) = USB_TXIE_EP0;
    HWREGH(USB_BASE + MUSB_RXIE_OFFSET) = 0;

    HWREGB(USB_BASE + MUSB_POWER_OFFSET) |= USB_POWER_SOFTCONN;

#ifdef CONFIG_MUSB_SUSPEND_ENABLE
    // Enable Suspend function
    HWREGB(USB_BASE + MUSB_POWER_OFFSET) |= USB_POWER_EN_SUSPEND;
#endif

    return 0;
}

int usb_dc_deinit(uint8_t busid)
{
    return 0;
}

int usbd_set_address(uint8_t busid, const uint8_t addr)
{
    if (addr == 0) {
        HWREGB(USB_BASE + MUSB_FADDR_OFFSET) = 0;
    }

    g_musb_udc.dev_addr = addr;
    return 0;
}

int usbd_set_remote_wakeup(uint8_t busid)
{
    usb_resume_low_level_init();

    HWREGB(USB_BASE + MUSB_POWER_OFFSET) |= USB_POWER_RESUME;
    usbd_musb_delay_ms(10);
    HWREGB(USB_BASE + MUSB_POWER_OFFSET) &= ~USB_POWER_RESUME;
    return 0;
}

uint8_t usbd_get_port_speed(uint8_t busid)
{
    uint8_t speed = USB_SPEED_UNKNOWN;

    if (HWREGB(USB_BASE + MUSB_POWER_OFFSET) & USB_POWER_HSMODE)
        speed = USB_SPEED_HIGH;
    else if (HWREGB(USB_BASE + MUSB_DEVCTL_OFFSET) & USB_DEVCTL_FSDEV)
        speed = USB_SPEED_FULL;
    else if (HWREGB(USB_BASE + MUSB_DEVCTL_OFFSET) & USB_DEVCTL_LSDEV)
        speed = USB_SPEED_LOW;

    return speed;
}

int usbd_ep_open(uint8_t busid, const struct usb_endpoint_descriptor *ep)
{
    uint8_t ep_idx = USB_EP_GET_IDX(ep->bEndpointAddress);
    uint8_t old_ep_idx;
    uint32_t ui32Flags = 0;
    uint16_t ui32Register = 0;

    if (ep_idx == 0) {
        g_musb_udc.out_ep[0].ep_mps = USB_CTRL_EP_MPS;
        g_musb_udc.out_ep[0].ep_type = 0x00;
        g_musb_udc.out_ep[0].ep_enable = true;
        g_musb_udc.in_ep[0].ep_mps = USB_CTRL_EP_MPS;
        g_musb_udc.in_ep[0].ep_type = 0x00;
        g_musb_udc.in_ep[0].ep_enable = true;
        return 0;
    }

    if (ep_idx > (CONFIG_USBDEV_EP_NUM - 1)) {
        USB_LOG_ERR("Ep addr %02x overflow\r\n", ep->bEndpointAddress);
        return -1;
    }

    old_ep_idx = musb_get_active_ep();
    musb_set_active_ep(ep_idx);

    if (USB_EP_DIR_IS_OUT(ep->bEndpointAddress)) {
        g_musb_udc.out_ep[ep_idx].ep_mps = USB_GET_MAXPACKETSIZE(ep->wMaxPacketSize);
        g_musb_udc.out_ep[ep_idx].ep_type = USB_GET_ENDPOINT_TYPE(ep->bmAttributes);
        g_musb_udc.out_ep[ep_idx].ep_enable = true;

        if ((8 << MUSB_RXFIFOCR_SZ) < g_musb_udc.out_ep[ep_idx].ep_mps) {
            USB_LOG_ERR("Ep %02x fifo is overflow\r\n", ep->bEndpointAddress);
            return -2;
        }

        HWREGH(USB_BASE + MUSB_IND_RXMAP_OFFSET) = USB_GET_MAXPACKETSIZE(ep->wMaxPacketSize) >> 3;

        //
        // Allow auto clearing of RxPktRdy when packet of size max packet
        // has been unloaded from the FIFO.
        //
        if (ui32Flags & USB_EP_AUTO_CLEAR) {
            ui32Register = USB_RXCSRH1_AUTOCL;
        }
        //
        // Configure the DMA mode.
        //
        if (ui32Flags & USB_EP_DMA_MODE_1) {
            ui32Register |= USB_RXCSRH1_DMAEN | USB_RXCSRH1_DMAMOD;
        } else if (ui32Flags & USB_EP_DMA_MODE_0) {
            ui32Register |= USB_RXCSRH1_DMAEN;
        }
        //
        // If requested, disable NYET responses for high-speed bulk and
        // interrupt endpoints.
        //
        if (ui32Flags & USB_EP_DIS_NYET) {
            ui32Register |= USB_RXCSRH1_DISNYET;
        }

        //
        // Enable isochronous mode if requested.
        //
        if (USB_GET_ENDPOINT_TYPE(ep->bmAttributes) == 0x01) {
            ui32Register |= USB_RXCSRH1_ISO;
        }

        HWREGB(USB_BASE + MUSB_IND_RXCSRH_OFFSET) = ui32Register;

        // Reset the Data toggle to zero.
        if (HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) & USB_RXCSRL1_RXRDY)
            HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) = (USB_RXCSRL1_CLRDT | USB_RXCSRL1_FLUSH);
        else
            HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) = USB_RXCSRL1_CLRDT;
    } else {
        g_musb_udc.in_ep[ep_idx].ep_mps = USB_GET_MAXPACKETSIZE(ep->wMaxPacketSize);
        g_musb_udc.in_ep[ep_idx].ep_type = USB_GET_ENDPOINT_TYPE(ep->bmAttributes);
        g_musb_udc.in_ep[ep_idx].ep_enable = true;

        if ((8 << MUSB_TXFIFOCR_SZ) < g_musb_udc.in_ep[ep_idx].ep_mps) {
            USB_LOG_ERR("Ep %02x fifo is overflow\r\n", ep->bEndpointAddress);
            return -2;
        }

        HWREGH(USB_BASE + MUSB_IND_TXMAP_OFFSET) = USB_GET_MAXPACKETSIZE(ep->wMaxPacketSize) >> 3;

        //
        // Allow auto setting of TxPktRdy when max packet size has been loaded
        // into the FIFO.
        //
        if (ui32Flags & USB_EP_AUTO_SET) {
            ui32Register |= USB_TXCSRH1_AUTOSET;
        }

        //
        // Configure the DMA mode.
        //
        if (ui32Flags & USB_EP_DMA_MODE_1) {
            ui32Register |= USB_TXCSRH1_DMAEN | USB_TXCSRH1_DMAMOD;
        } else if (ui32Flags & USB_EP_DMA_MODE_0) {
            ui32Register |= USB_TXCSRH1_DMAEN;
        }

        //
        // Enable isochronous mode if requested.
        //
        if (USB_GET_ENDPOINT_TYPE(ep->bmAttributes) == 0x01) {
            ui32Register |= USB_TXCSRH1_ISO;
        }

        HWREGB(USB_BASE + MUSB_IND_TXCSRH_OFFSET) = ui32Register;

        // Reset the Data toggle to zero.
        if (HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) & USB_TXCSRL1_TXRDY)
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = (USB_TXCSRL1_CLRDT | USB_TXCSRL1_FLUSH);
        else
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_TXCSRL1_CLRDT;
    }

    musb_set_active_ep(old_ep_idx);

    return 0;
}

int usbd_ep_close(uint8_t busid, const uint8_t ep)
{
    return 0;
}

int usbd_ep_set_stall(uint8_t busid, const uint8_t ep)
{
    uint8_t ep_idx = USB_EP_GET_IDX(ep);
    uint8_t old_ep_idx;

    old_ep_idx = musb_get_active_ep();
    musb_set_active_ep(ep_idx);

    if (USB_EP_DIR_IS_OUT(ep)) {
        if (ep_idx == 0x00) {
            usb_ep0_state = USB_EP0_STATE_STALL;
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) |= (USB_CSRL0_STALL | USB_CSRL0_RXRDYC);
        } else {
            HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) |= USB_RXCSRL1_STALL;
        }
    } else {
        if (ep_idx == 0x00) {
            usb_ep0_state = USB_EP0_STATE_STALL;
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) |= (USB_CSRL0_STALL | USB_CSRL0_RXRDYC);
        } else {
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) |= USB_TXCSRL1_STALL;
        }
    }

    musb_set_active_ep(old_ep_idx);
    return 0;
}

int usbd_ep_clear_stall(uint8_t busid, const uint8_t ep)
{
    uint8_t ep_idx = USB_EP_GET_IDX(ep);
    uint8_t old_ep_idx;

    old_ep_idx = musb_get_active_ep();
    musb_set_active_ep(ep_idx);

    if (USB_EP_DIR_IS_OUT(ep)) {
        if (ep_idx == 0x00) {
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) &= ~USB_CSRL0_STALLED;
        } else {
            // Clear the stall on an OUT endpoint.
            HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) &= ~(USB_RXCSRL1_STALL | USB_RXCSRL1_STALLED);
            // Reset the data toggle.
            HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) |= USB_RXCSRL1_CLRDT;
        }
    } else {
        if (ep_idx == 0x00) {
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) &= ~USB_CSRL0_STALLED;
        } else {
            // Clear the stall on an IN endpoint.
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) &= ~(USB_TXCSRL1_STALL | USB_TXCSRL1_STALLED);
            // Reset the data toggle.
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) |= USB_TXCSRL1_CLRDT;
        }
    }

    musb_set_active_ep(old_ep_idx);
    return 0;
}

int usbd_ep_is_stalled(uint8_t busid, const uint8_t ep, uint8_t *stalled)
{
    uint8_t ep_idx = USB_EP_GET_IDX(ep);
    uint8_t old_ep_idx;

    old_ep_idx = musb_get_active_ep();
    musb_set_active_ep(ep_idx);

    if (USB_EP_DIR_IS_OUT(ep)) {
        if(HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) & USB_RXCSRL1_STALL) {
            *stalled = 1;
        } else {
            *stalled = 0;
        }
    } else {
        if(HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) & USB_TXCSRL1_STALL) {
            *stalled = 1;
        } else {
            *stalled = 0;
        }
    }
    musb_set_active_ep(old_ep_idx);
    return 0;
}

int usbd_ep_start_write(uint8_t busid, const uint8_t ep, const uint8_t *data, uint32_t data_len)
{
    uint8_t ep_idx = USB_EP_GET_IDX(ep);
    uint8_t old_ep_idx;
    int res = 0;

    USB_LOG_DBG("EP%d start write %08X@%d\r\n", ep_idx, data, data_len);

    if (!data && data_len) {
        return -1;
    }
    if (!g_musb_udc.in_ep[ep_idx].ep_enable) {
        return -2;
    }

    OM_CRITICAL_BEGIN();

    old_ep_idx = musb_get_active_ep();
    musb_set_active_ep(ep_idx);

    if (HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) & USB_TXCSRL1_TXRDY) {
        res = -3;
    } else {
        g_musb_udc.in_ep[ep_idx].xfer_buf = (uint8_t *)data;
        g_musb_udc.in_ep[ep_idx].xfer_len = data_len;
        g_musb_udc.in_ep[ep_idx].actual_xfer_len = 0;

        if (data_len == 0) {
            if (ep_idx == 0x00) {
                if (g_musb_udc.setup.wLength == 0) {
                    usb_ep0_state = USB_EP0_STATE_IN_STATUS;
                } else {
                    usb_ep0_state = USB_EP0_STATE_IN_ZLP;
                }
                HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = (USB_CSRL0_TXRDY | USB_CSRL0_DATAEND);
            } else {
                HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_TXCSRL1_TXRDY;
                HWREGH(USB_BASE + MUSB_TXIE_OFFSET) |= (1 << ep_idx);
            }
#if CONFIG_USBDEV_DMACH_NUM>0
        } else if (g_musb_udc.in_ep[ep_idx].dma_ch != DMA_NONE) {
            musb_dma_in_setup(ep_idx);
#endif
        } else {
            musb_fifo_in_setup(ep_idx);
        }
    }
    musb_set_active_ep(old_ep_idx);

    OM_CRITICAL_END();

    return res;
}

int usbd_ep_start_read(uint8_t busid, const uint8_t ep, uint8_t *data, uint32_t data_len)
{
    uint8_t ep_idx = USB_EP_GET_IDX(ep);
    uint8_t old_ep_idx;

    USB_LOG_DBG("EP%d start read %08X@%d\r\n", ep_idx, data, data_len);

    if (!data && data_len) {
        return -1;
    }
    if (!g_musb_udc.out_ep[ep_idx].ep_enable) {
        return -2;
    }

    OM_CRITICAL_BEGIN();

    old_ep_idx = musb_get_active_ep();
    musb_set_active_ep(ep_idx);

    g_musb_udc.out_ep[ep_idx].xfer_buf = data;
    g_musb_udc.out_ep[ep_idx].xfer_len = data_len;
    g_musb_udc.out_ep[ep_idx].actual_xfer_len = 0;

    if (data_len == 0) {
        if (ep_idx == 0) {
            usb_ep0_state = USB_EP0_STATE_SETUP;
        }
    } else {
        if (ep_idx == 0) {
            usb_ep0_state = USB_EP0_STATE_OUT_DATA;
#if CONFIG_USBDEV_DMACH_NUM>0
        } else if (g_musb_udc.out_ep[ep_idx].dma_ch != DMA_NONE) {
            musb_dma_out_setup(ep_idx);
#endif
        } else if ((HWREGH(USB_BASE + MUSB_RXIE_OFFSET) & (1 << ep_idx)) == 0) {
            musb_fifo_out_setup(ep_idx);
        }
    }

//#if CONFIG_USBDEV_DMACH_NUM>0
//    if (HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) & USB_RXCSRL1_RXRDY) {
//        musb_fifo_out_handler(ep_idx);
//    }
//#endif

    musb_set_active_ep(old_ep_idx);

    OM_CRITICAL_END();

    return 0;
}

static void handle_ep0(void)
{
    uint8_t ep0_status = HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET);
    uint16_t read_count;

    if (ep0_status & USB_CSRL0_STALLED) {
        HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) &= ~USB_CSRL0_STALLED;
        usb_ep0_state = USB_EP0_STATE_SETUP;
        return;
    }

    if (ep0_status & USB_CSRL0_SETEND) {
        HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_CSRL0_SETENDC;
    }

    if (g_musb_udc.dev_addr > 0) {
        HWREGB(USB_BASE + MUSB_FADDR_OFFSET) = g_musb_udc.dev_addr;
        g_musb_udc.dev_addr = 0;
    }

    switch (usb_ep0_state) {
        case USB_EP0_STATE_SETUP:
            if (ep0_status & USB_CSRL0_RXRDY) {
                read_count = HWREGH(USB_BASE + MUSB_IND_RXCOUNT_OFFSET);

                if (read_count != 8) {
                    return;
                }

                musb_read_packet(0, (uint8_t *)&g_musb_udc.setup, 8);
                if (g_musb_udc.setup.wLength) {
                    HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_CSRL0_RXRDYC;
                } else {
                    HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = (USB_CSRL0_RXRDYC | USB_CSRL0_DATAEND);
                }

                usbd_event_ep0_setup_complete_handler(0, (uint8_t *)&g_musb_udc.setup);
            }
            break;

        case USB_EP0_STATE_IN_DATA:
            if (g_musb_udc.in_ep[0].xfer_len > g_musb_udc.in_ep[0].ep_mps) {
                g_musb_udc.in_ep[0].actual_xfer_len += g_musb_udc.in_ep[0].ep_mps;
                g_musb_udc.in_ep[0].xfer_len -= g_musb_udc.in_ep[0].ep_mps;
            } else {
                g_musb_udc.in_ep[0].actual_xfer_len += g_musb_udc.in_ep[0].xfer_len;
                g_musb_udc.in_ep[0].xfer_len = 0;
            }

            usbd_event_ep_in_complete_handler(0, 0x80, g_musb_udc.in_ep[0].actual_xfer_len);

            break;
        case USB_EP0_STATE_OUT_DATA:
            if (ep0_status & USB_CSRL0_RXRDY) {
                read_count = HWREGH(USB_BASE + MUSB_IND_RXCOUNT_OFFSET);

                musb_read_packet(0, g_musb_udc.out_ep[0].xfer_buf, read_count);
                g_musb_udc.out_ep[0].xfer_buf += read_count;
                g_musb_udc.out_ep[0].actual_xfer_len += read_count;

                if (read_count < g_musb_udc.out_ep[0].ep_mps) {
                    usbd_event_ep_out_complete_handler(0, 0x00, g_musb_udc.out_ep[0].actual_xfer_len);
                    HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = (USB_CSRL0_RXRDYC | USB_CSRL0_DATAEND);
                    usb_ep0_state = USB_EP0_STATE_IN_STATUS;
                } else {
                    HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_CSRL0_RXRDYC;
                }
            }
            break;
        case USB_EP0_STATE_IN_STATUS:
        case USB_EP0_STATE_IN_ZLP:
            usb_ep0_state = USB_EP0_STATE_SETUP;
            usbd_event_ep_in_complete_handler(0, 0x80, 0);
            break;
    }
}

void USBD_IRQHandler(uint8_t busid)
{
    uint32_t is;
    uint32_t txis;
    uint32_t rxis;
    uint8_t old_ep_idx;
    uint8_t ep_idx;

    is = HWREGB(USB_BASE + MUSB_IS_OFFSET);
    txis = HWREGH(USB_BASE + MUSB_TXIS_OFFSET);
    rxis = HWREGH(USB_BASE + MUSB_RXIS_OFFSET);

    HWREGB(USB_BASE + MUSB_IS_OFFSET) = is;

    old_ep_idx = musb_get_active_ep();

    /* Receive a reset signal from the USB bus */
    if (is & USB_IS_RESET) {
        musb_reset_handler();
    }

    if (is & USB_IS_SOF) {
    }

    if (is & USB_IS_CONN) {
        usbd_event_connect_handler(0);
    }

    if (is & USB_IS_DISCON) {
        usbd_event_disconnect_handler(0);
    }

    if (is & USB_IS_RESUME) {
        // Move to USB_BUSACT_IRQHandler
        //usb_resume_low_level_init();
        //usbd_event_resume_handler(0);
    }

    if (is & USB_IS_SUSPEND) {
        usb_suspend_low_level_init();
        usbd_event_suspend_handler(0);
    }

    txis &= HWREGH(USB_BASE + MUSB_TXIE_OFFSET);
    /* Handle EP0 interrupt */
    if (txis & USB_TXIE_EP0) {
        HWREGH(USB_BASE + MUSB_TXIS_OFFSET) = USB_TXIE_EP0;
        musb_set_active_ep(0);
        handle_ep0();
        txis &= ~USB_TXIE_EP0;
    }

    ep_idx = 1;
    while (txis) {
        if (txis & (1 << ep_idx)) {
            musb_set_active_ep(ep_idx);
            HWREGH(USB_BASE + MUSB_TXIS_OFFSET) = (1 << ep_idx);
            if (HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) & USB_TXCSRL1_UNDRN) {
                HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) &= ~USB_TXCSRL1_UNDRN;
            }

#if CONFIG_USBDEV_DMACH_NUM>0
            if ((OM_USB->TX_CSR2 & USB_TX_CSR2_DMAENAB) == 0)
#endif
            {
                musb_fifo_in_handler(ep_idx);
            }

            txis &= ~(1 << ep_idx);
        }
        ep_idx++;
    }

    rxis &= HWREGH(USB_BASE + MUSB_RXIE_OFFSET);
    ep_idx = 1;
    while (rxis) {
        if (rxis & (1 << ep_idx)) {
            musb_set_active_ep(ep_idx);
            HWREGH(USB_BASE + MUSB_RXIS_OFFSET) = (1 << ep_idx);
            if (HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) & USB_RXCSRL1_RXRDY) {
#if CONFIG_USBDEV_DMACH_NUM>0
                //if (OM_USB->RX_CSR2 & USB_RX_CSR2_DMAENAB)
                if (g_musb_udc.out_ep[ep_idx].dma_ch != DMA_NONE) {
                    uint16_t fifo_len = HWREGH(USB_BASE + MUSB_IND_RXCOUNT_OFFSET);
                    musb_dma_out_handler(ep_idx, fifo_len);
                } else
#endif
                {
                    musb_fifo_out_handler(ep_idx);
                }
            }

            rxis &= ~(1 << ep_idx);
        }
        ep_idx++;
    }

    musb_set_active_ep(old_ep_idx);
}

void USBD_DMA_IRQHandler(void)
{
#if CONFIG_USBDEV_DMACH_NUM>0
    uint32_t int_dma = OM_USB->DMA_ISR;

    uint8_t ch, ep, ep_saved;
    ep_saved = OM_USB->IDX;
    for (ch = 0; ch < CONFIG_USBDEV_DMACH_NUM; ++ch) {
        if (0 == (int_dma & (1 << ch))) {
            continue;
        }

#if 1
        ep = g_musb_udc.dma_ch2ep[ch];
        if (ep != DMA_NONE) {
            OM_USB->IDX = USB_EP_GET_IDX(ep);
            if (USB_EP_DIR_IS_OUT(ep)) {
                musb_dma_out_handler(USB_EP_GET_IDX(ep), 0);
            } else {
                musb_dma_in_handler(USB_EP_GET_IDX(ep));
            }
        }
#else
        for (ep = 1; ep < CONFIG_USBDEV_EP_NUM; ++ep) {
            if (ch == g_musb_udc.out_ep[ep].dma_ch) {
                OM_USB->IDX = ep;
                musb_dma_out_handler(ep, 0);
            } else if (ch == g_musb_udc.in_ep[ep].dma_ch) {
                OM_USB->IDX = ep;
                musb_dma_in_handler(ep);
            }
        }
#endif
    } /* for ch */
    OM_USB->IDX = ep_saved;
#endif
}

#ifdef CONFIG_USBDEV_TEST_MODE
void usbd_execute_test_mode(uint8_t busid, uint8_t test_mode)
{
    uint32_t regval;

    switch (test_mode) {
        case USB_TEST_J:
            break;
        case USB_TEST_K:
            break;
        case USB_TEST_SE0_NAK:
            break;
        case USB_TEST_PACKET:
        {
            uint8_t temp[53];
            uint8_t *pp;
            uint8_t i;
            pp = temp;

            for (i = 0; i < 9; i++) /*JKJKJKJK x 9*/
                *pp++ = 0x00;
            for (i = 0; i < 8; i++) /* 8*AA */
                *pp++ = 0xAA;
            for (i = 0; i < 8; i++) /* 8*EE */
                *pp++ = 0xEE;
            *pp++ = 0xFE;
            for (i = 0; i < 11; i++) /* 11*FF */
                *pp++ = 0xFF;
            *pp++ = 0x7F; *pp++ = 0xBF; *pp++ = 0xDF; *pp++ = 0xEF;
            *pp++ = 0xF7; *pp++ = 0xFB; *pp++ = 0xFD; *pp++ = 0xFC;
            *pp++ = 0x7E; *pp++ = 0xBF; *pp++ = 0xDF; *pp++ = 0xEF;
            *pp++ = 0xF7; *pp++ = 0xFB; *pp++ = 0xFD; *pp++ = 0x7E;

        } break;

        case USB_TEST_FORCE_ENABLE:
            break;

        default:
            break;
    }
}
#endif

