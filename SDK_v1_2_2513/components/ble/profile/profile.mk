BLE_PROFILE_DIR = $(BLE_DIR)/profile

BLE_PROFILE_DEFS =
BLE_PROFILE_LIBS =

BLE_PROFILE_SRCS = $(BLE_PROFILE_DIR)/om_ancs/ancs_client.c                    \
                   $(BLE_PROFILE_DIR)/om_bms/om_bms.c                          \
                   $(BLE_PROFILE_DIR)/om_bms/service_om_bms.c                  \
                   $(BLE_PROFILE_DIR)/om_cgms/om_cgms.c                        \
                   $(BLE_PROFILE_DIR)/om_cgms/service_om_cgms.c                \
                   $(BLE_PROFILE_DIR)/om_dfu/om_dfu_nvds.c                     \
                   $(BLE_PROFILE_DIR)/om_dfu/om_dfu.c                          \
                   $(BLE_PROFILE_DIR)/om_dfu/om_dfu_ver_check.c                \
                   $(BLE_PROFILE_DIR)/om_dfu/service_om_dfu.c                  \
                   $(BLE_PROFILE_DIR)/om_gap_dis_batt/service_common.c         \
                   $(BLE_PROFILE_DIR)/om_hid_media/app_hid_media.c             \
                   $(BLE_PROFILE_DIR)/om_tspp/service_tspp.c                   \
                   $(BLE_PROFILE_DIR)/om_wechat_lite/app_wechat_lite.c         \

BLE_PROFILE_INCS = $(BLE_PROFILE_DIR)/om_ancs                                  \
                   $(BLE_PROFILE_DIR)/om_bms                                   \
                   $(BLE_PROFILE_DIR)/om_cgms                                  \
                   $(BLE_PROFILE_DIR)/om_dfu                                   \
                   $(BLE_PROFILE_DIR)/om_gap_dis_batt                          \
                   $(BLE_PROFILE_DIR)/om_hid_media                             \
                   $(BLE_PROFILE_DIR)/om_tspp                                  \
                   $(BLE_PROFILE_DIR)/om_wechat_lite                           \
