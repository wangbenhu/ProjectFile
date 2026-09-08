PROJECT_LINKER_SCRIPT_DIR = $(PROJECT_DIR)/linker_script

# linker script select
ifeq ($(CONFIG_XIP_ROM), y)
LINKER_SCRIPT = $(PROJECT_LINKER_SCRIPT_DIR)/GCC/cm4f_rom.ld
endif

ifeq ($(CONFIG_XIP_RAM), y)
LINKER_SCRIPT = $(PROJECT_LINKER_SCRIPT_DIR)/GCC/cm4f_ram.ld
endif

ifeq ($(CONFIG_XIP_FLASH), y)
LINKER_SCRIPT = $(PROJECT_LINKER_SCRIPT_DIR)/GCC/cm4f_flash.ld
endif
