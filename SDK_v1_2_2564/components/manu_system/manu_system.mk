MANU_SYSTEM_DIR = $(COMPONENTS_DIR)/manu_system

ifeq ($(CONFIG_FAULT_HANDLE), y)
COMPONENTS_INCS += $(MANU_SYSTEM_DIR)/fault_handle
COMPONENTS_SRCS += $(MANU_SYSTEM_DIR)/fault_handle/fault_handle.c
endif

ifeq ($(CONFIG_MANU_CALIB), y)
include $(MANU_SYSTEM_DIR)/manu_calib/manu_calib.mk
COMPONENTS_INCS += $(MANU_CALIB_INCS)
COMPONENTS_SRCS += $(MANU_CALIB_SRCS)
endif

ifeq ($(CONFIG_MANU_CALIB_BLE_V2), y)
include $(MANU_SYSTEM_DIR)/manu_calib_ble_v2/manu_calib_ble_v2.mk
COMPONENTS_INCS += $(MANU_CALIB_BLE_V2_INCS)
COMPONENTS_SRCS += $(MANU_CALIB_BLE_V2_SRCS)
endif