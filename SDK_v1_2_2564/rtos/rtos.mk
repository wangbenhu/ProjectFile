RTOS_DIR = $(SDK_DIR)/rtos

# NON_RTOS
ifeq ($(CONFIG_NON_RTOS), y)
RTOS_SRCS =
RTOS_INCS =
RTOS_DEFS =
else
RTOS_SRCS =
RTOS_DEFS =
RTOS_INCS = $(RTOS_DIR)/include
endif

#FreeRTOS
ifeq ($(CONFIG_FREE_RTOS), y)
include $(RTOS_DIR)/FreeRTOS/free_rtos.mk
RTOS_SRCS += $(FREE_RTOS_SRCS)
RTOS_INCS += $(FREE_RTOS_INCS)
RTOS_DEFS += $(FREE_RTOS_DEFS)
endif

#LiteOS
ifeq ($(CONFIG_LITE_RTOS), y)
include $(RTOS_DIR)/Liteos_m/liteos_m.mk
RTOS_SRCS += $(LITEOS_SRCS)
RTOS_INCS += $(LITEOS_INCS)
RTOS_DEFS += $(LITEOS_DEFS)
endif
