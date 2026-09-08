ifeq ($(CONFIG_SHELL), y)
SHELL_DIR = $(COMPONENTS_DIR)/shell
COMPONENTS_INCS += $(SHELL_DIR)
COMPONENTS_SRCS += $(SHELL_DIR)/shell_common.c

ifneq ($(CONFIG_NON_RTOS), y)
COMPONENTS_SRCS += $(SHELL_DIR)/shell_port_rtos.c
else
COMPONENTS_SRCS += $(SHELL_DIR)/shell_port_nonrtos.c
endif

endif
