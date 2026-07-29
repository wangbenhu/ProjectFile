/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */
#ifndef __APP_COMMON_H__
#define __APP_COMMON_H__

/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include "ob_config.h"
#include "omble.h"
#include "om_log.h"

/*********************************************************************
 * MACROS
 */
#define log_debug(...) om_log(OM_LOG_INFO, ##__VA_ARGS__)

#define APP_ADV_ADDR_TYPE       OB_ADV_ADDR_TYPE_RANDOM
#define APP_ADV_PHY             OB_ADV_PHY_1M
#define APP_ADV_FILTER          OB_ADV_FILTER_NONE
#define APP_ADV_INTV_MIN        0x80
#define APP_ADV_INTV_MAX        0xA0

struct app_device {
    uint8_t conn_idx;
    uint8_t addr_type;
    uint8_t addr[6];
    char name[32];
};
/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
void log_debug_array(const uint8_t *data, int len);
/**
 *******************************************************************************
 * @brief  app common init
 *******************************************************************************
 */
void app_common_init(void);

/**
 *******************************************************************************
 * @brief  get connection avalable number
 * @return  Check if new connection or advertising can be create
 *******************************************************************************
 */
bool app_get_conn_num_get(void);

/**
 *******************************************************************************
 * @brief  Release connection number
 *******************************************************************************
 */
void app_get_conn_num_put(void);

/**
 *******************************************************************************
 * @brief  Set busy state
 * @param[in] busy_desc  Description for busy state, NULL for no busy
 *******************************************************************************
 */
void set_shell_busy(const char *busy_desc);

/**
 *******************************************************************************
 * @brief  Init advertising module
 *******************************************************************************
 */
void app_adv_init(void);

/**
 *******************************************************************************
 * @brief  Start advertise
 * @param[in] adv_index  advertise index
 * @param[in] adv_prop   advertise prop
 *******************************************************************************
 */
uint32_t app_adv_start(int adv_index, int adv_prop);

/**
 *******************************************************************************
 * @brief  Stop advertise
 * @param[in] adv_index  advertise index
 *******************************************************************************
 */
uint32_t app_adv_stop(int adv_index);

/**
 *******************************************************************************
 * @brief  Scan init
 *******************************************************************************
 */
void app_scan_init(void);

/**
 *******************************************************************************
 * @brief  Start scan
 * @param[in] timeout  scan timeout, unit in second
 *******************************************************************************
 */
uint32_t app_scan_start(uint8_t timeout);

/**
 *******************************************************************************
 * @brief  Get scanned device infomation
 * @param[in]  index   device index
 * @param[out] address address
 * @param[out] name    name pointer
 *******************************************************************************
 */
bool app_scan_device_get(uint8_t index, uint8_t *address, const char **name);

/**
 *******************************************************************************
 * @brief  Connect init
 *******************************************************************************
 */
void app_conn_init(void);

/**
 *******************************************************************************
 * @brief  Start connection
 * @param[in]  address peer device address
 * @param[in]  timeout connection timeout, unit in second
 * @param[in]  name    device name
 *******************************************************************************
 */
uint32_t app_conn_start(uint8_t *address, uint8_t timeout, const char *name);

/**
 *******************************************************************************
 * @brief  Get connected device infomation
 * @param[out]  buffer for device info
 * @param[out]  max number for buffer
 *******************************************************************************
 */
void app_conn_device_get(struct app_device *dev_buffer, uint8_t *max_num);

/**
 *******************************************************************************
 * @brief  Disconnect
 * @param[in]  index   connection index
 *******************************************************************************
 */
void app_disconnect(uint8_t conn_idx);

/**
 *******************************************************************************
 * @brief  GATT client init
 *******************************************************************************
 */
void app_gatt_client_init(void);

/**
 *******************************************************************************
 * @brief  gatt client discover all
 * @param[in]  conn_idx   connection index
 *******************************************************************************
 */
uint32_t app_gatt_client_discover_all(uint8_t conn_idx);

/**
 *******************************************************************************
 * @brief  gatt discover all service
 * @param[in]  conn_idx   connection index
 * @param[in]  start_handle   start_handle for discover service
 * @param[in]  end_handle   end_handle for discover service
 *******************************************************************************
 */
uint32_t app_gatt_discover_service(uint8_t conn_idx, uint16_t start_handle, uint16_t end_handle);
/**
 *******************************************************************************
 * @brief  gatt discover service by uuid
 * @param[in]  conn_idx   connection index
 * @param[in]  uuid       uuid
 * @param[in]  uuid_len   length of uuid
 *******************************************************************************
 */
uint32_t app_gatt_discover_service_uuid(uint8_t conn_idx, const uint8_t *uuid, uint16_t uuid_len);
/**
 *******************************************************************************
 * @brief  gatt discover characteristic by handle
 * @param[in]  conn_idx   connection index
 * @param[in]  start_handle   start_handle for discover service
 * @param[in]  end_handle   end_handle for discover service
 *******************************************************************************
 */
uint32_t app_gatt_discover_char(uint8_t conn_idx, uint16_t start_handle, uint16_t end_handle);
/**
 *******************************************************************************
 * @brief  gatt discover descriptor by handle
 * @param[in]  conn_idx   connection index
 * @param[in]  start_handle   start_handle for discover service
 * @param[in]  end_handle   end_handle for discover service
 *******************************************************************************
 */
uint32_t app_gatt_discover_desc(uint8_t conn_idx, uint16_t start_handle, uint16_t end_handle);
/**
 *******************************************************************************
 * @brief  gatt read data by handle
 * @param[in]  conn_idx   connection index
 * @param[in]  handle ATT handle
 *******************************************************************************
 */
uint32_t app_gatt_read(uint8_t conn_idx, uint16_t handle);
/**
 *******************************************************************************
 * @brief gatt read data by uuid
 * @param[in]  conn_idx   connection index
 * @param[in]  uuid       uuid
 * @param[in]  uuid_len   length of uuid
 *******************************************************************************
 */
uint32_t app_gatt_read_uuid(uint8_t conn_idx, const uint8_t *uuid, uint16_t uuid_len);
/**
 *******************************************************************************
 * @brief  gatt write request
 * @param[in]  conn_idx   connection index
 * @param[in]  handle ATT handle
 * @param[in]  data       write data
 * @param[in]  len        length of data
 *******************************************************************************
 */
uint32_t app_gatt_write(uint8_t conn_idx, uint16_t handle,  uint8_t type, const uint8_t *data, uint16_t len);

#endif /* __APP_COMMON_H__ */
/** @} */
