PROJECT_APP_DIR = $(PROJECT_DIR)/app

PROJECT_APP_INCS = $(PROJECT_APP_DIR)/firms 							\
				   $(PROJECT_APP_DIR)/tmall								\
				   $(PROJECT_APP_DIR)


PROJECT_APP_SRCS = $(PROJECT_APP_DIR)/omsh_app.c 						\
				   $(PROJECT_APP_DIR)/omsh_app_shell.c 					\
				   $(PROJECT_APP_DIR)/omsh_app_hook.c 					\
				   $(PROJECT_APP_DIR)/firms/omsh_app_firms.c			\
				   $(PROJECT_APP_DIR)/firms/omsh_app_firms_mdl.c		\
				   $(PROJECT_APP_DIR)/firms/omsh_app_firms_mdl_demo.c	\
				   $(PROJECT_APP_DIR)/tmall/omsh_app_tmall.c

PROJECT_APP_DEFS =