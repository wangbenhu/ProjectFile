#1. Specify SDK & Project workspace directory
SDK_DIR = ../../
PROJECT_DIR = ./

#2. config
include $(PROJECT_DIR)/.config

#3. Sources/includes/defines/library
include $(PROJECT_DIR)/config/config.mk
include $(PROJECT_DIR)/linker_script/linker_script.mk
include $(PROJECT_DIR)/main/main.mk
include $(PROJECT_DIR)/app/app.mk

PROJECT_SRCS = $(PROJECT_CONFIG_SRCS)                                          \
               $(PROJECT_MAIN_SRCS)                                            \
               $(PROJECT_APP_SRCS)                                         	   \

PROJECT_INCS = $(PROJECT_CONFIG_INCS)                                          \
               $(PROJECT_MAIN_INCS)                                            \
               $(PROJECT_APP_INCS)                                             \

PROJECT_DEFS = $(PROJECT_CONFIG_DEFS)                                          \
               $(PROJECT_MAIN_DEFS)                                            \
               $(PROJECT_APP_DEFS)                                             \

PROJECT_LIBS =

#4. rules.mk
include $(SDK_DIR)/tools/build/rules.mk
