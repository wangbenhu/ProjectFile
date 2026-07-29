
BLE_HOST_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

BLE_HOST_SRCS =

BLE_HOST_INCS = $(BLE_HOST_DIR)include

BLE_HOST_LIBS =

BLE_HOST_DEFS =

ifeq ($(CONFIG_BLE_HOST), y)

BLE_HOST_LIBS += $(BLE_HOST_DIR)lib/GCC/libblehost_full.a
BLE_HOST_DEFS += \
	CONFIG_BLE_HOST=1

endif


