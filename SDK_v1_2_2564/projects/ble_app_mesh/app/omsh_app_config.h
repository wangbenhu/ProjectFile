/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup MESH MESH
 * @ingroup  DOCUMENT
 * @brief
 * @details  Mesh app header file

 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __OMSH_APP_CONFIG_H
#define __OMSH_APP_CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 * DEFINES FOR CONFIG MESH FEATURE
 */
#define CONFIG_APP_MESH_FIRMS       (1)
#define CONFIG_APP_MESH_TMALL       (0)
#define CONFIG_BLE_MESH_LOG_EN      (1)
#define CONFIG_BLE_MESH_RELAY       (1)
#define CONFIG_BLE_MESH_GATT_PROXY  (1)
#define CONFIG_BLE_MESH_LPN         (0)
#define CONFIG_BLE_MESH_FRIEND      (0)


/*******************************************************************************
 * DEFINES FOR APP USE
 */
#if CONFIG_APP_MESH_FIRMS
#define APP_MESH_FIRMS          (1)
#elif CONFIG_APP_MESH_TMALL
#define APP_MESH_TMALL          (1)
#endif

#if CONFIG_BLE_MESH_LOG_EN
#define APP_MESH_LOG            (1)
#else
#define APP_MESH_LOG            (0)
#endif

#if CONFIG_BLE_MESH_RELAY
#define APP_FEAT_RELAY          (1)
#else
#define APP_FEAT_RELAY          (0)
#endif

#if CONFIG_BLE_MESH_GATT_PROXY
#define APP_FEAT_PROXY          (1)
#else
#define APP_FEAT_PROXY          (0)
#endif

#if CONFIG_BLE_MESH_LPN
#define APP_FEAT_LPN            (1)
#else
#define APP_FEAT_LPN            (0)
#endif

#if CONFIG_BLE_MESH_FRIEND
#define APP_FEAT_FRND           (1)
#else
#define APP_FEAT_FRND           (0)
#endif


#ifdef __cplusplus
}
#endif

#endif /* __OMSH_APP_CONFIG_H */