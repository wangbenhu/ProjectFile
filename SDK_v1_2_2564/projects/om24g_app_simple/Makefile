#1. Specify SDK & Project workspace directory
SDK_DIR = ../..

PROJECT_DIR = .

#2. config
-include $(PROJECT_DIR)/.config

#3. Sources/includes/defines/library
include $(PROJECT_DIR)/linker_script/linker_script.mk


PROJECT_SRCS = $(PROJECT_DIR)/main/main.c \
               $(PROJECT_DIR)/main/app_om24g.c \
               $(PROJECT_DIR)/main/system.c \

PROJECT_INCS = $(PROJECT_DIR)/config/

PROJECT_DEFS =

PROJECT_LIBS =

#3. rules.mk
include $(SDK_DIR)/tools/build/rules.mk
