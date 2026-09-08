COMMON_DIR = $(SDK_DIR)/common

COMMON_SRCS = $(COMMON_DIR)/source/om_printf.c                                 \
              $(COMMON_DIR)/source/om_mem.c                                    \
              $(COMMON_DIR)/source/om_ringbuff.c                               \
              $(COMMON_DIR)/source/om_common.c                                 \
              $(COMMON_DIR)/source/om_libc_retarget.c                          \
              $(COMMON_DIR)/source/om_fifo.c                                   \
              $(COMMON_DIR)/source/om_list.c                                   \

COMMON_INCS = $(COMMON_DIR)/include                                            \
              $(COMMON_DIR)/source

COMMON_DEFS =
