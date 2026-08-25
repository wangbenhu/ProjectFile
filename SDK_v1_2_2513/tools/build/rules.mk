################################################################################
#
# rules makefile
#
################################################################################
ifndef SDK_DIR
$(error SDK_DIR Not defined in project)
endif

export SDK_DIR
include $(SDK_DIR)/SDK.mk

LINKFLAGS = -L $(SDK_DIR)/hal/device/GCC
LINKFLAGS += $(LIB_DIRS)

DEVICE_FLAGS = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS      += $(DEVICE_FLAGS) -std=gnu99 -fomit-frame-pointer -Wall -Wformat=0  \
               -Wstrict-aliasing=0 -ffunction-sections -fdata-sections -g3
CXXFLAGS     = $(DEVICE_FLAGS) $(CFLAGS)
ASFLAGS      = $(DEVICE_FLAGS) -nostartfiles
ARFLAGS      = -rc
LINKFLAGS   += $(DEVICE_FLAGS) -nostartfiles -static --specs=nano.specs -lc -lnosys -lm -Wl,--gc-sections

# toolchain
# COMPILER_PATH = /mnt/data/opt/gcc-arm-none-eabi-10.3-2021.10/bin/
CC       = $(COMPILER_PATH)arm-none-eabi-gcc
AS       = $(COMPILER_PATH)arm-none-eabi-gcc -x assembler-with-cpp
LD       = $(COMPILER_PATH)arm-none-eabi-ld
AR       = $(COMPILER_PATH)arm-none-eabi-ar
CXX	     = $(COMPILER_PATH)arm-none-eabi-g++
OBJCP    = $(COMPILER_PATH)arm-none-eabi-objcopy
OBJDUMP  = $(COMPILER_PATH)arm-none-eabi-objdump
OBJSIZE  = $(COMPILER_PATH)arm-none-eabi-size

GCC_VERSION = $(shell $(CC) --version | grep -oP '\d+\.\d+' | head -1)
ifeq ($(shell test $(GCC_VERSION) \> 12.2; echo $$?), 0)
    LDFLAGS += -Wl,--no-warn-rwx-segments    # GCC 12.2 and higher
endif

ifeq ($(CONFIG_LIB_GENERATE), y)
SOURCES   = $(PROJECT_SRCS)
INCLUDES  = $(PROJECT_INCS) $(SDK_INCS)
LIBRARIES = $(PROJECT_LIBS)
DEFINES   = $(PROJECT_DEFS) $(SDK_DEFS)
else
SOURCES   =  $(subst $(SDK_DIR)/,, $(SDK_SRCS)) $(PROJECT_SRCS)
INCLUDES  = $(PROJECT_INCS) $(SDK_INCS)
LIBRARIES = $(PROJECT_LIBS) $(SDK_LIBS)
DEFINES   = $(PROJECT_DEFS) $(SDK_DEFS)
endif

# .config Defines
KCONFIG_CONFIG := $(PROJECT_DIR)/.config
export KCONFIG_CONFIG

supported_devices := $(shell cat $(SDK_DIR)/tools/build/supported_devices.txt)
device ?= $(firstword $(supported_devices))
is_supported:=0
$(if $(filter $(device) , $(supported_devices)), $(eval is_supported=1))
ifeq ($(is_supported), 0)
$(error the $(device) device not in $(SDK_DIR)/tools/build/supported_devices.txt, please selected in [$(supported_devices)])
endif

DEVICE := $(shell echo $(device) | tr '[:lower:]' '[:upper:]')
# defines in compiler
DEFINES += $(DEVICE)=1

SUBVERSION_EXISTS := $(shell command -v svnversion 2>/dev/null)
ifneq ($(SUBVERSION_EXISTS), )
	DEFINES += '__SDK_VERSION="$(shell svnversion -n $(SDK_DIR))"'
	DEFINES += '__PROJECT_VERSION="$(shell svnversion -n $(PROJECT_DIR))"'
endif

VPATH = $(PROJECT_DIR):$(SDK_DIR)

# optimization level
ifeq ($(CONFIG_TOOL_CHAIN_OPTIMIZATION_LEVEL), 0)
OPTIMIZE = 0
else ifeq ($(CONFIG_TOOL_CHAIN_OPTIMIZATION_LEVEL), 1)
OPTIMIZE = 1
else ifeq ($(CONFIG_TOOL_CHAIN_OPTIMIZATION_LEVEL), 2)
OPTIMIZE = 2
else ifeq ($(CONFIG_TOOL_CHAIN_OPTIMIZATION_LEVEL), 3)
OPTIMIZE = 3
else ifeq ($(CONFIG_TOOL_CHAIN_OPTIMIZATION_LEVEL), 4)
OPTIMIZE = s
else ifeq ($(CONFIG_TOOL_CHAIN_OPTIMIZATION_LEVEL), 5)
OPTIMIZE = f
else ifeq ($(CONFIG_TOOL_CHAIN_OPTIMIZATION_LEVEL), 6)
OPTIMIZE = g
else
OPTIMIZE = s
endif

################################################################################
# verbose mode
ifeq ($(V), 1)
TRACE_CC  =
TRACE_LD  =
TRACE_AS  =
TRACE_AR  =
Q=
else
TRACE_CC  = @echo "	CC    $<"
TRACE_AS  = @echo "	AS    $<"
TRACE_LD  = @echo "Linking $@ ..."
TRACE_AR  = @echo "Archiving $@ ..."
Q=@
endif

# Target
PROJECT                 ?= $(notdir $(CURDIR))
TARGET                  ?= $(PROJECT)
TARGET_ELF              := $(TARGET).elf
TARGET_BIN              := $(TARGET).bin
TARGET_HEX              := $(TARGET).hex
TARGET_MAP              := $(TARGET).map
TARGET_DIS              := $(TARGET).dis
TARGET_LIB              := $(TARGET).a
OBJCPFLAGS_ELF_TO_BIN    = -Obinary
OBJCPFLAGS_ELF_TO_HEX    = -Oihex
OBJCPFLAGS_HEX_TO_BIN    = -Obinary -Iihex

DEFS     = $(patsubst %, -D%, $(DEFINES))
INCS     = $(patsubst %, -I%, $(INCLUDES))
LIB_DIRS = $(patsubst %, -L%, $(dir $(LIBRARIES)))
LIBS     = $(patsubst lib%.a, -l%, $(notdir $(LIBRARIES)))
SCRIPT   = $(patsubst %, -T%, $(LINKER_SCRIPT))
#LDDIR    = $(dir $(SCRIPT))
#SCRIPT_DIRS = $(patsubst %,-L%,$(LDDIR))

# FLAGS
CFLAGS += -O$(OPTIMIZE) $(DEFS) $(INCS)
CXXFLAGS = $(CFLAGS)
ASFLAGS += $(DEFS) $(INCS)
LDFLAGS += $(CFLAGS) $(LINKFLAGS) $(SCRIPT) $(LIB_DIR) $(LIBS) -Wl,-Map=$(TARGET_MAP)

BUILDDIR = $(CURDIR)/.build

# build obj files directory
FILES_C_OBJ = $(patsubst %,$(BUILDDIR)/%,$(filter %.o, $(SOURCES:%.c=%.o)))
FILES_S_OBJ = $(patsubst %,$(BUILDDIR)/%,$(filter %.o, $(SOURCES:%.S=%.o)))
FILES_S_OBJ += $(patsubst %,$(BUILDDIR)/%,$(filter %.o, $(SOURCES:%.s=%.o)))
FILES_C_DEPEND = $(patsubst %,$(BUILDDIR)/%,$(filter %.d, $(SOURCES:%.c=%.d)))
FILES_S_DEPEND = $(patsubst %,$(BUILDDIR)/%,$(filter %.d, $(SOURCES:%.S=%.d)))
FILES_S_DEPEND += $(patsubst %,$(BUILDDIR)/%,$(filter %.d, $(SOURCES:%.s=%.d)))

# Target
ifeq ($(CONFIG_LIB_GENERATE), y)
TARGET_OUT = $(TARGET_LIB)
else
TARGET_OUT = $(TARGET_HEX) $(TARGET_BIN)
endif

# .PHONY
.PHONY: all clean distclean list document help

# Target
all: $(TARGET_OUT)


# bin
$(TARGET_BIN) : $(TARGET_HEX)
	$(Q)$(OBJCP) $(OBJCPFLAGS_HEX_TO_BIN) $< $@
	@echo -e "\nEach object size:" >> $(TARGET_MAP)
	@find $(BUILDDIR) -name '*.o' | xargs $(OBJSIZE) -B -d >> $(TARGET_MAP)
	@echo ------------------- Build Done for $(DEVICE) ------------------------

# hex
$(TARGET_HEX) : $(TARGET_ELF)
	$(Q)$(OBJCP) $(OBJCPFLAGS_ELF_TO_HEX) $< $@

# elf
$(TARGET_ELF) : $(FILES_C_OBJ) $(FILES_S_OBJ)
	@touch $(SDK_DIR)/components/shell/shell_port_*.c
	$(TRACE_LD)
	$(Q)$(CC) $+ $(LDFLAGS) -o $@
	$(Q)$(OBJSIZE) -B -d $@
	$(Q)$(OBJDUMP) -d $@ > $(TARGET_DIS)
	@ln -sf -T $@ a.elf

# library
$(TARGET_LIB) : $(FILES_C_OBJ) $(FILES_S_OBJ)
	$(TRACE_AR)
	$(Q)rm -f $@
	$(Q)$(AR) -crs $@ $^
	@echo "Build done"

# c -> o
$(BUILDDIR)/%.o : %.c gen_autoconf
$(BUILDDIR)/%.o : %.c $(BUILDDIR)/%.d
	$(TRACE_CC)
	@mkdir -p $(@D) >/dev/null
	$(Q)$(CC) -MT $@ -MMD -MP -MF $(BUILDDIR)/$*.Td $(CFLAGS) -c $< -o $@
	@mv -f $(BUILDDIR)/$*.Td $(BUILDDIR)/$*.d

# S -> o
$(BUILDDIR)/%.o : %.S gen_autoconf
$(BUILDDIR)/%.o : %.S $(BUILDDIR)/%.d
	$(TRACE_AS)
	@mkdir -p $(@D) >/dev/null
	$(Q)$(AS) -MT $@ -MMD -MP -MF $(BUILDDIR)/$*.Td $(ASFLAGS) -c $< -o $@
	@mv -f $(BUILDDIR)/$*.Td $(BUILDDIR)/$*.d

# s -> o
$(BUILDDIR)/%.o : %.s gen_autoconf
$(BUILDDIR)/%.o : %.s $(BUILDDIR)/%.d
	$(TRACE_AS)
	@mkdir -p $(@D) >/dev/null
	$(Q)$(AS) -MT $@ -MMD -MP -MF $(BUILDDIR)/$*.Td $(ASFLAGS) -c $< -o $@
	@mv -f $(BUILDDIR)/$*.Td $(BUILDDIR)/$*.d

gen_autoconf:
	$(Q)genconfig $(PROJECT_DIR)/Kconfig --header-path $(PROJECT_DIR)/config/autoconf.h

# d
$(BUILDDIR)/%.d: ;
.PRECIOUS: $(BUILDDIR)/%.d

# Include dependent (*.d)
-include $(FILES_C_DEPEND)
-include $(FILES_S_DEPEND)

# list some info
list:
	@echo SOURCES: -------------------------------------------------------------
	@for f in $(PROJECT_SRCS); do echo $$f; done
	@for f in $(SDK_SRCS); do echo $$f; done
	@echo LINKER SCRIPT: -------------------------------------------------------
	@echo $(LINKER_SCRIPT)
	@echo INCLUDES: ------------------------------------------------------------
	@for f in $(INCLUDES); do echo $$f; done
	@echo LIBRARIES: -----------------------------------------------------------
	@for f in $(LIBRARIES); do echo $$f; done
	@echo DEFINES: -------------------------------------------------------------
	@for f in $(DEFINES); do echo $$f; done

# Clean
clean:
	$(Q)rm -rf $(TARGET_ELF) $(TARGET_BIN) $(TARGET_HEX) $(TARGET_MAP) $(TARGET_DIS) a.elf $(BUILDDIR)

distclean:
	$(Q)rm -rf .build out *.elf *.bin *.map *.hex *.dis


# KConfig
menuconfig:
	$(Q)menuconfig $(PROJECT_DIR)/Kconfig
	$(Q)genconfig $(PROJECT_DIR)/Kconfig --header-path $(PROJECT_DIR)/config/autoconf.h

listnewconfig:
	$(Q)listnewconfig $(PROJECT_DIR)/Kconfig

# Doxygen document
document:
	@doxygen docs/doxygen/doxyfile

help:
	@echo "Summary of Makefile building targets:"
	@echo "    all (default)     - Application and all libraries"
	@echo "    help              - Show this help info"
	@echo "    clean             - Just the application"
	@echo "    distclean         - Clean all .build/* out/* *.elf *.bin *.map *.hex *.dis"
	@echo "    document          - Generate documentation"
	@echo "    menuconfig        - Prompt menuconfig for secure/nonsecure project"
	@echo "    listnewconfig     - List new config"
	@echo "    list              - List sources/includes/libraries/define"
	@echo "    device            - Specify target device, device=xxx"
