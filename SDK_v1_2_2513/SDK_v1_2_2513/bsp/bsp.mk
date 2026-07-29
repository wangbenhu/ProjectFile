BSP_DIR = $(SDK_DIR)/bsp

BSP_INCS = $(BSP_DIR)/include
BSP_DEFS =

ifeq ($(CONFIG_BOARD_EVB), y)
BSP_SRCS = $(BSP_DIR)/board_evb/board_evb.c
else
BSP_SRCS =
endif

ifeq ($(CONFIG_GLCD_GC9C01), y)
BSP_SRCS += $(BSP_DIR)/source/glcd_gc9c01.c
endif
ifeq ($(CONFIG_GLCD_ST7789), y)
BSP_SRCS += $(BSP_DIR)/source/glcd_st7789.c
endif
ifeq ($(CONFIG_GLCD_ST7796), y)
BSP_SRCS += $(BSP_DIR)/source/glcd_st7796.c
endif
