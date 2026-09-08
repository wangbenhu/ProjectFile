/*
 * Copyright (c) 2024, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "usb_config.h"
#include "stdint.h"
#include "usb_musb_reg.h"
#include "usbd_core.h"

#ifdef CONFIG_PM
#include "pm.h"
#endif

#if CONFIG_USBDEV_EP_NUM != 6
#error OM chips only support 6 endpoints
#endif

#if CONFIG_USBHOST_PIPE_NUM != 6
#error OM chips only support 6 pipes
#endif

// Only PAD16/PAD17 can be used as USB
#define USBD_IO_DM         16
#define USBD_IO_DP         17

//
// USB configuration
//   - Number of DMA channels: 7  (obsolete)
//   - Size of DMA buffer: 64 bytes  (obsolete)
//   - Total FIFO size: 1024 bytes
//   - Number of Tx endpoints: 5 + 1(EP0)
//   - Number of Rx endpoints: 5 + 1(EP0)
//   - Disable pull-up resistor during transmit
//
static struct musb_fifo_cfg musb_device_table[] = {
    { .ep_num = 0, .style = FIFO_TXRX, .maxpacket = 64, },
    { .ep_num = 1, .style = FIFO_TXRX, .maxpacket = 192, },
    { .ep_num = 2, .style = FIFO_TXRX, .maxpacket = 192, },
    { .ep_num = 3, .style = FIFO_TXRX, .maxpacket = 64, },
    { .ep_num = 4, .style = FIFO_TXRX, .maxpacket = 64, },
    { .ep_num = 5, .style = FIFO_TXRX, .maxpacket = 64, },
};

void USBD_IRQHandler(uint8_t busid);
void USBD_DMA_IRQHandler(void);

static void usbd_clock_enable(bool enable)
{
    // Enable USB 48MHz clock source
    if (enable) {
        drv_pmu_syspll_power_enable(true);
    }

    // CLock
    DRV_RCC_CLOCK_ENABLE(RCC_CLK_USB, enable);
}

static void usbd_io_pull_down_enable(bool enable)
{
    // D+ controled by IP
    // D- always PullDown
    if (enable) {
        //drv_pmu_pin_mode_set(USBD_IO_DM, PMU_PIN_MODE_PD);
        //drv_pmu_pin_mode_set(USBD_IO_DP, PMU_PIN_MODE_PD);
    } else {
        //drv_pmu_pin_mode_set(USBD_IO_DM, PMU_PIN_MODE_FLOAT);
        drv_pmu_pin_mode_set(USBD_IO_DP, PMU_PIN_MODE_FLOAT);
    }
}

uint8_t usbd_get_musb_fifo_cfg(struct musb_fifo_cfg **cfg)
{
    *cfg = musb_device_table;
    return sizeof(musb_device_table) / sizeof(musb_device_table[0]);
}

uint32_t usb_get_musb_ram_size(void)
{
    return 1024;
}

void usbd_musb_delay_ms(uint8_t ms)
{
    DRV_DELAY_MS(ms);
}

void usb_dc_low_level_init(void)
{
//    USB_LOG_INFO("Init\r\n");

    // Open USB clock
    usbd_clock_enable(true);
    // Reset
    DRV_RCC_RESET(RCC_CLK_USB);
    // Power on USB module
    REGW(&OM_PMU->PSO_PM, MASK_1REG(PMU_PSO_USB_POWER_ON, 1));
    // Wakeup mode: 0:DIP 1:DIDIF
    REGW(&OM_PMU->USB_CTRL0, MASK_1REG(PMU_USB_CTRL0_WU_CTRL, 0));
    // USB IP control pullup/pulldown R
    REGW(&OM_PMU->USB_CTRL0, MASK_1REG(PMU_USB_CTRL0_IO_CTRL, 1));
    REGW(&OM_PMU->USB_CTRL2, MASK_1REG(PMU_USB_CTRL2_DELAY_EN, 1));
    // USB diff comp enable
    REGW(&OM_PMU->USB_CTRL2, MASK_1REG(PMU_USB_CTRL2_COMP_EN, 1));
    // Open wakeup clock
    REGW(&OM_PMU->USB_CTRL1, MASK_1REG(PMU_USB_CTRL1_WU_CLK_GATE, 0));
    // Clear wakeup flag
    REGW(&OM_PMU->USB_CTRL0, MASK_1REG(PMU_USB_CTRL0_WU_CLR, 1));

    // Disable PD
    usbd_io_pull_down_enable(false);

    // USB Debug BUS 2, IO20=D+ IO21=D- IO12=irq
    //OM_SYS->MON = SYS_MON_USB(2);

    // CPM Debug BUS, IO14=48MHz
    //OM_SYS->MON = SYS_MON_CPM(0);

    // NVIC enable
    NVIC_SetPriority(USB_IRQn, RTE_USB_IRQ_PRIORITY);
    NVIC_ClearPendingIRQ(USB_IRQn);
    NVIC_EnableIRQ(USB_IRQn);
    NVIC_SetPriority(USB_BUSACT_IRQn, RTE_USB_IRQ_PRIORITY);
    NVIC_ClearPendingIRQ(USB_BUSACT_IRQn);
    NVIC_EnableIRQ(USB_BUSACT_IRQn);
    NVIC_SetPriority(USB_DMA_IRQn, RTE_USB_IRQ_PRIORITY);
    NVIC_ClearPendingIRQ(USB_DMA_IRQn);
    NVIC_EnableIRQ(USB_DMA_IRQn);

#ifdef CONFIG_PM
    pm_sleep_prevent(PM_ID_USB);
#endif
}

void usb_suspend_low_level_init(void)
{
//    USB_LOG_INFO("Suspend\r\n");

    // Reset wakeup module
    REGW(&OM_PMU->USB_CTRL1, MASK_1REG(PMU_USB_CTRL1_WU_SOFT_RST, 1));
    // Close clock
    usbd_clock_enable(false);

#ifdef CONFIG_PM
    pm_sleep_allow(PM_ID_USB);
#endif
}

void usb_resume_low_level_init(void)
{
//    USB_LOG_INFO("Resume\r\n");

    // Open USB clock
    usbd_clock_enable(true);
    // Clear wakeup flag
    REGW(&OM_PMU->USB_CTRL0, MASK_1REG(PMU_USB_CTRL0_WU_CLR, 1));

#ifdef CONFIG_PM
    pm_sleep_prevent(PM_ID_USB);
#endif
}

void usb_dc_low_level_deinit(void)
{
}

void USB_IRQHandler(void)
{
    USBD_IRQHandler(0);
}

void USB_BUSACT_IRQHandler(void)
{
    usb_resume_low_level_init();

    usbd_event_resume_handler(0);
}

void USB_DMA_IRQHandler(void)
{
    USBD_DMA_IRQHandler();
}
