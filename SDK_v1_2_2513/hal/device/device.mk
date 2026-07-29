HAL_DEV_DIR = $(HAL_DIR)/device

# device
HAL_DEV_INCS = $(HAL_DEV_DIR)/include

HAL_DEV_LIBS = $(HAL_DEV_DIR)/GCC/libcpft.a

# startup & linker_script
HAL_DEV_STARTUP_SRCS ?= $(HAL_DEV_DIR)/GCC/startup_cm4f.S                      \
                        $(HAL_DEV_DIR)/system_cm4f.c                           \

HAL_DEV_SRCS = $(HAL_DEV_STARTUP_SRCS)
