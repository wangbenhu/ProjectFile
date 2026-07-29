FREE_RTOS_DIR = $(RTOS_DIR)/FreeRTOS

FREE_RTOS_SRCS =
FREE_RTOS_INCS =

FREE_RTOS_SRCS += $(FREE_RTOS_DIR)/Source/croutine.c                           \
                  $(FREE_RTOS_DIR)/Source/event_groups.c                       \
                  $(FREE_RTOS_DIR)/Source/list.c                               \
                  $(FREE_RTOS_DIR)/Source/queue.c                              \
                  $(FREE_RTOS_DIR)/Source/stream_buffer.c                      \
                  $(FREE_RTOS_DIR)/Source/tasks.c                              \
                  $(FREE_RTOS_DIR)/Source/timers.c                             \
                  $(FREE_RTOS_DIR)/Source/Portable/MemMang/heap_5.c            \
                  $(FREE_RTOS_DIR)/Source/Portable/Common/mpu_wrappers.c       \
                  $(FREE_RTOS_DIR)/Source/Portable/GCC/port.c                  \
                  $(FREE_RTOS_DIR)/CMSIS/Source/cmsis_os2.c                    \
                  $(FREE_RTOS_DIR)/CMSIS/Source/cmsis_os1.c                    \
                  $(FREE_RTOS_DIR)/CMSIS/Source/freertos_evr.c                 \
                  $(FREE_RTOS_DIR)/CMSIS/Source/os_systick.c                   \

FREE_RTOS_INCS += $(FREE_RTOS_DIR)/Source/Portable/GCC                         \
                  $(FREE_RTOS_DIR)/Source/Include                              \
                  $(FREE_RTOS_DIR)/CMSIS/Include                               \
                  $(FREE_RTOS_DIR)/CMSIS/Include1                              \
                  $(FREE_RTOS_DIR)/CMSIS/Config/ARMCM                          \


FREE_RTOS_DEFS =
