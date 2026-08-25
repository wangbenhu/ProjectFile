################################################################################
#
# SDK makefile
#
################################################################################
# Include all .mk
include $(SDK_DIR)/bsp/bsp.mk
include $(SDK_DIR)/common/common.mk
include $(SDK_DIR)/components/components.mk
include $(SDK_DIR)/hal/hal.mk
include $(SDK_DIR)/rtos/rtos.mk


SDK_SRCS    = $(BSP_SRCS)                   \
              $(COMMON_SRCS)                \
              $(COMPONENTS_SRCS)            \
              $(HAL_SRCS)                   \
              $(RTOS_SRCS)                  \

SDK_INCS    = $(BSP_INCS)                   \
              $(COMMON_INCS)                \
              $(COMPONENTS_INCS)            \
              $(HAL_INCS)                   \
              $(RTOS_INCS)                  \

SDK_LIBS    = $(COMPONENTS_LIBS)            \
              $(HAL_LIBS)

SDK_DEFS    = $(BSP_DEFS)                   \
              $(COMMON_DEFS)                \
              $(COMPONENTS_DEFS)            \
              $(HAL_DEFS)                   \
              $(RTOS_DEFS)                  \
