HAL_DIR = $(SDK_DIR)/hal

include $(HAL_DIR)/CMSIS/cmsis.mk
include $(HAL_DIR)/device/device.mk
include $(HAL_DIR)/driver/driver.mk

#hal
HAL_LIBS = $(HAL_DEV_LIBS)

HAL_SRCS = $(HAL_CMSIS_SRCS)     \
           $(HAL_DEV_SRCS)       \
           $(HAL_DRV_SRCS)

HAL_INCS = $(HAL_CMSIS_INCS)     \
           $(HAL_DEV_INCS)       \
           $(HAL_DRV_INCS)

HAL_DEFS = $(HAL_CMSIS_DEFS)     \
           $(HAL_DEV_DEFS)       \
           $(HAL_DRV_DEFS)
