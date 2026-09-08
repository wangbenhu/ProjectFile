#
# Copyright (c) 2024, sakumisu
#
# SPDX-License-Identifier: Apache-2.0
#

# set(CONFIG_CHERRYUSB_DEVICE 1)
# set(CONFIG_CHERRYUSB_DEVICE_CDC 1)
# set(CONFIG_CHERRYUSB_DEVICE_HID 1)
# set(CONFIG_CHERRYUSB_DEVICE_MSC 1)
# set(CONFIG_CHERRYUSB_DEVICE_DCD "dwc2_st")

# set(CONFIG_CHERRYUSB_HOST 1)
# set(CONFIG_CHERRYUSB_HOST_CDC_ACM 1)
# set(CONFIG_CHERRYUSB_HOST_CDC_ECM 1)
# set(CONFIG_CHERRYUSB_HOST_CDC_NCM 1)
# set(CONFIG_CHERRYUSB_HOST_HID 1)
# set(CONFIG_CHERRYUSB_HOST_MSC 1)
# set(CONFIG_CHERRYUSB_HOST_VIDEO 1)
# set(CONFIG_CHERRYUSB_HOST_AUDIO 1)
# set(CONFIG_CHERRYUSB_HOST_CDC_RNDIS 1)
# set(CONFIG_CHERRYUSB_HOST_BLUETOOTH 1)
# set(CONFIG_CHERRYUSB_HOST_ASIX 1)
# set(CONFIG_CHERRYUSB_HOST_RTL8152 1)
# set(CONFIG_CHERRYUSB_OSAL "freertos")
# set(CONFIG_CHERRYUSB_HOST_HCD "ehci_xxx")

CHERRYUSB_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

CHERRYUSB_SRCS =

CHERRYUSB_INCS = \
	$(CHERRYUSB_DIR) \
	$(CHERRYUSB_DIR)common \
	$(CHERRYUSB_DIR)core \
	$(CHERRYUSB_DIR)class/hub \
	$(CHERRYUSB_DIR)class/cdc \
	$(CHERRYUSB_DIR)class/hid \
	$(CHERRYUSB_DIR)class/msc \
	$(CHERRYUSB_DIR)class/audio \
	$(CHERRYUSB_DIR)class/video \
	$(CHERRYUSB_DIR)class/wireless \
	$(CHERRYUSB_DIR)class/midi \
	$(CHERRYUSB_DIR)class/adb \
	$(CHERRYUSB_DIR)class/vendor/net \
	$(CHERRYUSB_DIR)class/vendor/serial \
	$(CHERRYUSB_DIR)class/vendor/wifi \

ifeq ($(CONFIG_CHERRYUSB_DEVICE),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)core/usbd_core.c
    ifeq ($(CONFIG_CHERRYUSB_DEVICE_CDC_ACM),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/cdc/usbd_cdc_acm.c
    endif
    ifeq ($(CONFIG_CHERRYUSB_DEVICE_HID),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/hid/usbd_hid.c
    endif
    ifeq ($(CONFIG_CHERRYUSB_DEVICE_MSC),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/msc/usbd_msc.c
    endif
    ifeq ($(CONFIG_CHERRYUSB_DEVICE_AUDIO),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/audio/usbd_audio.c
    endif
    ifeq ($(CONFIG_CHERRYUSB_DEVICE_VIDEO),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/video/usbd_video.c
    endif
    ifeq ($(CONFIG_CHERRYUSB_DEVICE_CDC_ECM),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/cdc/usbd_cdc_ecm.c
    endif
    ifeq ($(CONFIG_CHERRYUSB_DEVICE_CDC_NCM),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/cdc/usbd_cdc_ncm.c
    endif
    ifeq ($(CONFIG_CHERRYUSB_DEVICE_CDC_RNDIS),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/wireless/usbd_rndis.c
    endif
    ifeq ($(CONFIG_CHERRYUSB_DEVICE_DFU),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/dfu/usbd_dfu.c
    endif
    ifeq ($(CONFIG_CHERRYUSB_DEVICE_ADB),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/adb/usbd_adb.c
    endif

    ifeq ($(CONFIG_CHERRYUSB_DEVICE_DWC2_OM),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)port/dwc2/usb_dc_dwc2.c
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)port/dwc2/usb_glue_om.c
    endif
    ifeq ($(CONFIG_CHERRYUSB_DEVICE_MUSB_OM),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)port/musb/usb_dc_musb.c
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)port/musb/usb_glue_om.c
    endif

endif

ifeq ($(CONFIG_CHERRYUSB_HOST),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)core/usbh_core.c
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/hub/usbh_hub.c

    ifeq ($(CONFIG_CHERRYUSB_HOST_CDC_ACM),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/cdc/usbh_cdc_acm.c
    endif
    ifeq ($(CONFIG_CHERRYUSB_HOST_CDC_ECM),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/cdc/usbh_cdc_ecm.c
    endif
    ifeq ($(CONFIG_CHERRYUSB_HOST_CDC_RNDIS),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/wireless/usbh_rndis.c
    endif
    ifeq ($(CONFIG_CHERRYUSB_HOST_CDC_NCM),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/cdc/usbh_cdc_ncm.c
    endif
    ifeq ($(CONFIG_CHERRYUSB_HOST_HID),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/hid/usbh_hid.c
    endif
    ifeq ($(CONFIG_CHERRYUSB_HOST_MSC),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/msc/usbh_msc.c

    ifeq ($(CONFIG_CHERRYUSB_HOST_MSC_FATFS),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)third_party/fatfs-0.14/source/port/fatfs_usbh.c
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)third_party/fatfs-0.14/source/diskio.c
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)third_party/fatfs-0.14/source/ff.c
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)third_party/fatfs-0.14/source/ffsystem.c
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)third_party/fatfs-0.14/source/ffunicode.c
    CHERRYUSB_INCS += $(CHERRYUSB_DIR)third_party/fatfs-0.14/source
    endif
    endif
    ifeq ($(CONFIG_CHERRYUSB_HOST_VIDEO),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/video/usbh_video.c
    endif
    ifeq ($(CONFIG_CHERRYUSB_HOST_AUDIO),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/audio/usbh_audio.c
    endif
    ifeq ($(CONFIG_CHERRYUSB_HOST_ASIX),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/vendor/net/usbh_asix.c
    endif
    ifeq ($(CONFIG_CHERRYUSB_HOST_RTL8152),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/vendor/net/usbh_rtl8152.c
    endif
    ifeq ($(CONFIG_CHERRYUSB_HOST_CH34X),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/vendor/serial/usbh_ch34x.c
    endif
    ifeq ($(CONFIG_CHERRYUSB_HOST_CP210X),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/vendor/serial/usbh_cp210x.c
    endif
    ifeq ($(CONFIG_CHERRYUSB_HOST_FTDI),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/vendor/serial/usbh_ftdi.c
    endif
    ifeq ($(CONFIG_CHERRYUSB_HOST_PL2303),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/vendor/serial/usbh_pl2303.c
    endif
    ifeq ($(CONFIG_CHERRYUSB_HOST_BL616),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)class/vendor/wifi/usbh_bl616.c
    endif

    ifeq ($(CONFIG_CHERRYUSB_HOST_DWC2_OM),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)port/dwc2/usb_hc_dwc2.c
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)port/dwc2/usb_glue_om.c
    endif
    ifeq ($(CONFIG_CHERRYUSB_HOST_MUSB_OM),y)
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)port/musb/usb_hc_musb.c
    CHERRYUSB_SRCS += $(CHERRYUSB_DIR)port/musb/usb_glue_om.c
    endif
endif

ifeq ($(CONFIG_CHERRYUSB_OSAL_FREERTOS),y)
CHERRYUSB_SRCS += $(CHERRYUSB_DIR)osal/usb_osal_freertos.c
endif

ifeq ($(CONFIG_CHERRYRB),y)
CHERRYUSB_SRCS += $(CHERRYUSB_DIR)third_party/cherryrb/chry_ringbuffer.c
CHERRYUSB_INCS += $(CHERRYUSB_DIR)third_party/cherryrb
endif

ifeq ($(CONFIG_CHERRYMP),y)
CHERRYUSB_SRCS += $(CHERRYUSB_DIR)third_party/cherrymp/chry_mempool.c
CHERRYUSB_SRCS += $(CHERRYUSB_DIR)third_party/cherrymp/usbh_uvc_queue.c
CHERRYUSB_INCS += $(CHERRYUSB_DIR)third_party/cherrymp
endif

ifeq ($(CONFIG_CHERRYUSB), y)
COMPONENTS_SRCS += $(CHERRYUSB_SRCS)
COMPONENTS_INCS += $(CHERRYUSB_INCS)
endif
