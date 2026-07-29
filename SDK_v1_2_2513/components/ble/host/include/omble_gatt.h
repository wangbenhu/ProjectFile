/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @addtogroup OMBLE_GATT
 * @brief GATT
 * @details GATT related interfaces mainly control Bluetooth GATT client and GATT server communications
 * @version
 * Version 1.0
 *  - Initial release
 *
 */
/// @{

#ifndef __OMBLE_GATT_H__
#define __OMBLE_GATT_H__
#include <stdint.h>
#include "omble_range.h"

/// UUID 16bit Length value
#define OB_UUID_16BIT    2
/// UUID 128bit Length value
#define OB_UUID_128BIT  16

// GATT

/// OB_GATT_EVENTS
enum OB_GATT_EVENTS {
    /// Notification or Write Command sending completion event, refer to @ref ob_gatt_evt_tx_complete_t
    OB_GATT_EVT_TX_COMPLETE              = OB_GATT_EVTS_BASE + 0,
    /// MTU exchange completion event, refer to @ref ob_gatt_evt_mtu_exchanged_t
    OB_GATT_EVT_MTU_EXCHANGED            = OB_GATT_EVTS_BASE + 1,
    /// Request timeout event, refer to @ref ob_gatt_evt_timeout_t
    OB_GATT_EVT_TIMEOUT                  = OB_GATT_EVTS_BASE + 2,
    /// Indication response event, refer to @ref ob_gatts_evt_indicate_cfm_t
    OB_GATTS_EVT_INDICATE_CFM            = OB_GATT_EVTS_BASE + 3,
    /// Read request event, see @ref ob_gatts_evt_read_req_t
    OB_GATTS_EVT_READ_REQ                = OB_GATT_EVTS_BASE + 4,
    /// Write request event, see @ref ob_gatts_evt_write_req_t
    OB_GATTS_EVT_WRITE_REQ               = OB_GATT_EVTS_BASE + 5,
    /// Service discovery completion event, refer to @ref ob_gattc_evt_find_serv_rsp_t
    OB_GATTC_EVT_FIND_SERV_RSP           = OB_GATT_EVTS_BASE + 6,
    /// Service discovery by UUID completion event, refer to @ref ob_gattc_evt_find_serv_by_uuid_rsp_t
    OB_GATTC_EVT_FIND_SERV_BY_UUID_RSP   = OB_GATT_EVTS_BASE + 7,
    /// Characteristic discovery completion event, refer to @ref ob_gattc_evt_find_char_rsp_t
    OB_GATTC_EVT_FIND_CHAR_RSP           = OB_GATT_EVTS_BASE + 8,
    /// Descriptor of the discovery completion event, see @ref ob_gattc_evt_find_desc_rsp_t
    OB_GATTC_EVT_FIND_DESC_RSP           = OB_GATT_EVTS_BASE + 9,
    /// Read completion event, refer to @ref ob_gattc_evt_read_rsp_t
    OB_GATTC_EVT_READ_RSP                = OB_GATT_EVTS_BASE + 10,
    /// Read by UUID completion event, refer to @ref ob_gattc_evt_read_by_uuid_rsp_t
    OB_GATTC_EVT_READ_BY_UUID_RSP        = OB_GATT_EVTS_BASE + 11,
    /// Write completion event, refer to @ref ob_gattc_evt_write_rsp_t
    OB_GATTC_EVT_WRITE_RSP               = OB_GATT_EVTS_BASE + 12,
    /// Notification or indication receiving event, refer to @ref ob_gattc_evt_hvx_ind_t
    OB_GATTC_EVT_HVX_IND                 = OB_GATT_EVTS_BASE + 13,
};

/// att property
enum ob_att_prop {
    OB_ATT_PROP_IND       = 1 << 5,
    OB_ATT_PROP_NTF       = 1 << 4,
    OB_ATT_PROP_WRITE     = 1 << 3,
    OB_ATT_PROP_WRITE_CMD = 1 << 2,
    OB_ATT_PROP_READ      = 1 << 1,
};

/// att attribute structure
typedef struct {
    const uint8_t *uuid;    ///< ATT UUID
    uint8_t uuid_len;       ///< ATT UUID length
    uint8_t att_prop;       ///< ATT property, see @ref ob_att_prop
    uint8_t att_perm_read;  ///< ATT read permission. When it is not 0, it means that the GATT client can perform read operations only after the link is encrypted.
    uint8_t att_perm_write; ///< ATT write permission. When it is not 0, it means that the GATT client can perform write operations only after the link is encrypted.
} ob_gatt_item_t;

/// gatt service structure
typedef struct {
    const uint8_t *uuid;        ///< Service UUID
    uint16_t uuid_len;          ///< Service UUID length
    uint16_t att_num;           ///< Number of att
    const ob_gatt_item_t
    *item; ///< ATT attribute array, used to define the characters and descriptors contained in the GATT service
} ob_gatt_serv_t;

/// OB_GATT_HVX_TYPE
enum ob_gatt_hvx_type {
    OB_HANDLE_VALUE_NTF,  ///< Notification Type
    OB_HANDLE_VALUE_IND,  ///< Indication Type
};

/// GATT Write Type
enum ob_gattc_write_type {
    OB_GATTC_WRITE_REQ,  ///< Write request
    OB_GATTC_WRITE_CMD,  ///< Write no response
};

/// GATT handle value notify/indicate structure
typedef struct {
    uint8_t type;           ///< OB_HANDLE_VALUE_NTF or OB_HANDLE_VALUE_IND, ref @ref ob_gatt_hvx_type
    uint16_t att_hdl;       ///< att handle
    const uint8_t *data;    ///< Data, the length should not be greater than the current MTU value
    int len;                ///< Data length
    uint16_t id;            ///< Corresponds to the id value in the OB_GATT_EVT_TX_COMPLETE event, used to send the completed message
} ob_gatts_hvx_t;

/// Event structure for @ref OB_GATT_EVT_TX_COMPLETE.
typedef struct {
    uint16_t id;            ///< The data corresponding to the id when the Notification or Write Command message has been sent.
} ob_gatt_evt_tx_complete_t;

/// Event structure for @ref OB_GATT_EVT_MTU_EXCHANGED.
typedef struct {
    uint16_t mtu;           ///< Exchanged MTU value
} ob_gatt_evt_mtu_exchanged_t;

/// Event structure for @ref OB_GATT_EVT_TIMEOUT.
typedef struct {
    uint8_t att_opcode;     ///< Timeout opcode
} ob_gatt_evt_timeout_t;

/// Event structure for @ref OB_GATTS_EVT_INDICATE_CFM.
typedef struct {
    uint16_t att_hdl;       ///< att handle
} ob_gatts_evt_indicate_cfm_t;

/// Event structure for @ref OB_GATTS_EVT_READ_REQ.
typedef struct {
    uint16_t att_hdl;       ///< att handle
    uint16_t offset;        ///< data offset
} ob_gatts_evt_read_req_t;

/// Event structure for @ref OB_GATTS_EVT_WRITE_REQ.
typedef struct {
    uint16_t att_hdl;       ///< att handle
    uint8_t type;           ///< att write type, @ref ob_gattc_write_type
    const uint8_t *data;    ///< data
    uint16_t len;           ///< data length
} ob_gatts_evt_write_req_t;

/// GATT Service Structure
typedef struct {
    uint16_t start_hdl;     ///< start handle
    uint16_t end_hdl;       ///< end handle
    const uint8_t *uuid;    ///< service UUID
    uint8_t uuid_len;       ///< length of UUID
} ob_gatt_service_t;

/// Event structure for @ref OB_GATTC_EVT_FIND_SERV_RSP.
typedef struct {
    uint32_t status;                ///< status
    ob_gatt_service_t *service;     ///< gatt service array
    uint8_t service_num;            ///< gatt service number
} ob_gattc_evt_find_serv_rsp_t;

/// Event structure for @ref OB_GATTC_EVT_FIND_SERV_BY_UUID_RSP.
typedef struct {
    uint32_t status;                ///< status
    ob_gatt_service_t *service;     ///< gatt service array
    uint8_t service_num;            ///< gatt service number
} ob_gattc_evt_find_serv_by_uuid_rsp_t;

/// GATT Characteristic Structure
typedef struct {
    uint16_t value_hdl;     ///< value handle
    uint8_t properties;     ///< properties
    const uint8_t *uuid;    ///< char UUID
    uint8_t uuid_len;       ///< length of UUID
} ob_gatt_characteristic_t;

/// Event structure for @ref OB_GATTC_EVT_FIND_CHAR_RSP.
typedef struct {
    uint32_t status;                          ///< status
    ob_gatt_characteristic_t *characteristic; ///< gatt char array
    uint8_t char_num;                         ///< gatt char number
} ob_gattc_evt_find_char_rsp_t;

/// Gatt Descriptor Structure
typedef struct {
    uint16_t att_hdl;       ///< att handle
    const uint8_t *uuid;    ///< char UUID
    uint8_t uuid_len;       ///< length of UUID
} ob_gatt_descriptor_t;

/// Event structure for @ref OB_GATTC_EVT_FIND_DESC_RSP.
typedef struct {
    uint32_t status;                    ///< status
    ob_gatt_descriptor_t *descriptor;   ///< gatt descriptor array
    uint8_t desc_num;                   ///< gatt descriptor number
} ob_gattc_evt_find_desc_rsp_t;

/// Event structure for @ref OB_GATTC_EVT_READ_RSP.
typedef struct {
    uint32_t status;        ///< status
    uint16_t att_hdl;       ///< att handle
    uint16_t offset;        ///< read offset
    const uint8_t *data;    ///< data
    uint16_t len;           ///< data length
} ob_gattc_evt_read_rsp_t;

/// Event structure for @ref OB_GATTC_EVT_READ_BY_UUID_RSP.
typedef struct {
    uint32_t status;        ///< status
    uint16_t att_hdl;       ///< att handle
    const uint8_t *data;    ///< data
    uint16_t len;           ///< data length
} ob_gattc_evt_read_by_uuid_rsp_t;

/// Event structure for @ref OB_GATTC_EVT_WRITE_RSP.
typedef struct {
    uint32_t status;        ///< status
    uint16_t att_hdl;       ///< att handle
} ob_gattc_evt_write_rsp_t;

/// Event structure for @ref OB_GATTC_EVT_HVX_IND.
typedef struct {
    uint8_t type;           ///< @ref ob_gatt_hvx_type
    uint16_t att_hdl;       ///< att handle
    const uint8_t *data;    ///< data
    uint16_t len;           ///< data length
} ob_gattc_evt_hvx_ind_t;

/// GATT Event structure
typedef struct {
    uint8_t conn_idx; ///< Connection index
    union {
        ob_gatt_evt_tx_complete_t
        tx_complete;            ///< Notify or Write Cmd sending completion event parameters
        ob_gatt_evt_mtu_exchanged_t           mtu_exchanged;          ///< MTU exchanged completion event parameters
        ob_gatt_evt_timeout_t                 timeout;                ///< Request timeout event parameters
        ob_gatts_evt_indicate_cfm_t           indicate_cfm;           ///< Indication confirm event parameters
        ob_gatts_evt_read_req_t               read_req;               ///< Read request event parameters
        ob_gatts_evt_write_req_t              write_req;              ///< Write request event parameters
        ob_gattc_evt_find_serv_rsp_t          find_serv_rsp;          ///< Service discovery completion event parameters
        ob_gattc_evt_find_serv_by_uuid_rsp_t  find_serv_by_uuid_rsp;  ///< Service discovery by UUID completion event parameters
        ob_gattc_evt_find_char_rsp_t          find_char_rsp;          ///< Characteristic discovery completion event parameters
        ob_gattc_evt_find_desc_rsp_t          find_desc_rsp;          ///< Description discovery completion event parameters
        ob_gattc_evt_read_rsp_t               read_rsp;               ///< Read completion event parameters
        ob_gattc_evt_read_by_uuid_rsp_t       read_by_uuid_rsp;       ///< Read by UUID completion event parameters
        ob_gattc_evt_write_rsp_t              write_rsp;              ///< Write response event parameters
        ob_gattc_evt_hvx_ind_t                hvx_ind;                ///< Notification or indication receiving event parameters
    };
} omble_gatt_evt_t;

/// ATT Primary Service definition
extern const uint8_t ob_att_serv_def[2];
/// ATT Primary Service definition
extern const uint8_t ob_att_secs_def[2];
/// ATT Include Service definition
extern const uint8_t ob_att_incl_def[2];
/// ATT Characteristic definition
extern const uint8_t ob_att_char_def[2];
/// ATT Characteristic User Description Descriptor definition
extern const uint8_t ob_att_cudd_def[2];
/// ATT Client Characteristic Configuration Descriptor definition
extern const uint8_t ob_att_cccd_def[2];
/// ATT Report Reference Descriptor definition
extern const uint8_t ob_att_rrd_def[2];

/**@brief Add GATT service
 * @param[in]  att_serv      service content, refer to @ref ob_gatt_serv_t
 * @param[out] start_handle  After the service is successfully added, the service start handle value will be written to start_handle
 * @note The total number of services added should be less than the parameter max_gatt_serv_num of @ref omble_init
 * @note The start handle value of the newly added service increases from 1
 * @warning The variable pointed to by the parameter att_serv must be a global or static variable
 * @return result, refer to @ref ob_error
 */
uint32_t ob_gatts_add_service(const ob_gatt_serv_t *att_serv, uint16_t *start_handle);

/**@brief Set service visibility
 * @details When the service is set to invisible, the GATT client will automatically skip the hidden service when searching for services, but the handle value it occupies will be retained
 * @param[in]   serv_hdl   service start handle
 * @param[in]   visible    service visible
 * @note parameter serv_hdl must be the start_handle value returned by @ref ob_gatts_add_service
 * @return result, refer to @ref ob_error
 */
uint32_t ob_gatts_set_service_visibility(uint16_t serv_hdl, uint8_t visible);

/**@brief Send notification or indication, can only be called when acting as a GATT server
 * @details After sending notification, when the data is sent, you will receive the @ref OB_GATT_EVT_TX_COMPLETE event, but before receiving this event, you can call ob_gatts_send_hvx continuously to cache the notifications to be sent to increase the communication rate\n
 * @details After sending the indication, the peer device will respond to the @ref OB_GATTS_EVT_INDICATE_CFM event
 * @param[in] conn_idx  connection index
 * @param[in] hvx data  parameter
 * @note When calling ob_gatts_send_hvx to send notifications continuously, please note that the unsent data needs to occupy memory for caching, so the amount of cached data should be controlled according to the @ref OB_GATT_EVT_TX_COMPLETE event to prevent memory exhaustion
 * @return result, refer to @ref ob_error
 */
uint32_t ob_gatts_send_hvx(uint8_t conn_idx, const ob_gatts_hvx_t *hvx);

/**@brief Response to read request
 * @details Used to respond to the attribute requested by @ref OB_GATTS_EVT_READ_REQ. The requested attribute handle value refers to @ref ob_gatts_evt_read_req_t
 * @param[in] conn_idx   connection index
 * @param[in] att_state  Read status, refer to @ref ob_gatt_error_t. If it is not success, the data parameter is useless
 * @param[in] data       Data
 * @param[in] len        Length. If it exceeds the transmittable length, the data content will be automatically truncated
 * @note The transmittable length of the data is the current MTU-1. If the data length exceeds, it will be automatically truncated\n
 * @note If the attribute data content read exceeds MTU-1, the GATT Client will initiate a read operation again, and the offset in the read parameter @ref ob_gatts_evt_read_req_t is the offset of the data to be read
 * @note If the read operation needs to return an error, set att_state to the response error code. For GATT error codes, refer to @ref ob_gatt_error_t
 * @return result, refer to @ref ob_error
 */
uint32_t ob_gatts_read_response(uint8_t conn_idx, uint16_t att_state, const uint8_t *data,
                                uint16_t len);

/**@brief Response to write request
 * @details This interface is used to respond to @ref OB_GATTS_EVT_WRITE_REQ request
 * @param[in] conn_idx   connection index
 * @param[in] att_state  write status, refer to @ref ob_gatt_error_t
 * @return result, refer to @ref ob_error
 */
uint32_t ob_gatts_write_response(uint8_t conn_idx, uint16_t att_state);

/**@brief Set the gatt server to pause responding to client requests
 * @details After setting the pause state, the GATT server will temporarily cache the client's GATT request and not report it to the application layer until the user cancels the pause state
 * @param[in] conn_idx     connection index
 * @param[in] pause_state  pending state: 1 or 0
 * @return result, refer to @ref ob_error
 */
uint32_t ob_gatts_set_pause(uint8_t conn_idx, uint8_t pause_state);

/**@brief Request MTU exchange
 * @details The requested MTU value is the parameter max_att_mtu of @ref omble_init
 * @param[in] conn_idx connection index
 * @note After the MTU parameter is successfully exchanged, if the MTU value changes, the upper layer will be notified through the event @ref OB_GATT_EVT_MTU_EXCHANGED
 * @return result, refer to @ref ob_error
 */
uint32_t ob_gattc_mtu_req(uint8_t conn_idx);

/**@brief Service search - by handle range
 * @details As a GATT Client, search for GATT Server services between start_handle and end_handle. After peer responds, it will notify the upper layer through the event @ref OB_GATTC_EVT_FIND_SERV_RSP
 * @param[in] conn_idx      connection index
 * @param[in] start_handle  handle range
 * @param[in] end_handle    handle range
 * @return result, refer to @ref ob_error
 */
uint32_t ob_gattc_find_service_by_handle(uint8_t conn_idx, uint16_t start_handle,
                                         uint16_t end_handle);

/**@brief Service search - by UUID
 * @details As a GATT Client, search for the GATT Server service with the specified uuid between start_handle and end_handle. After peer responds, it will notify the upper layer through the event @ref OB_GATTC_EVT_FIND_SERV_BY_UUID_RSP
 * @param[in] conn_idx      connection index
 * @param[in] start_handle  handle range
 * @param[in] end_handle    handle range
 * @param[in] uuid          UUID
 * @param[in] uuid_len      UUID length
 * @return result, refer to @ref ob_error
 */
uint32_t ob_gattc_find_service_by_uuid(uint8_t conn_idx, uint16_t start_handle,
                                       uint16_t end_handle, const uint8_t *uuid, uint8_t uuid_len);

/**@brief Characteristic search
 * @details As a GATT Client, search for Characteristic between start_handle and end_handle. After peer responds, it will notify the upper layer through the event @ref OB_GATTC_EVT_FIND_CHAR_RSP
 * @param[in] conn_idx      connection index
 * @param[in] start_handle  handle range
 * @param[in] end_handle    handle range
 * @return result, refer to @ref ob_error
 */
uint32_t ob_gattc_find_characteristic(uint8_t conn_idx,
                                      uint16_t start_handle, uint16_t end_handle);

/**@brief Description search
 * @details As a GATT Client, search for Descriptor between start_handle and end_handle. After peer responds, it will notify the upper layer through the event @ref OB_GATTC_EVT_FIND_DESC_RSP
 * @param[in] conn_idx      connection index
 * @param[in] start_handle  handle range
 * @param[in] end_handle    handle range
 * @return result, refer to @ref ob_error
 */
uint32_t ob_gattc_find_descriptor(uint8_t conn_idx, uint16_t start_handle, uint16_t end_handle);

/**@brief Read request - by handle range
 * @details As a GATT Client, read the data corresponding to att_hdl. After peer responds, it will notify the upper layer through the event @ref OB_GATTC_EVT_READ_RSP
 * @param[in] conn_idx   connection index
 * @param[in] att_hdl    att handle
 * @param[in] offset     data offset
 * @return result, refer to @ref ob_error
 */
uint32_t ob_gattc_read(uint8_t conn_idx, uint16_t att_hdl, uint16_t offset);

/**@brief Read request - by UUID
 * @details As a GATT Client, read the data of the specified UUID between start_handle and end_handle. After peer responds, it will notify the upper layer through the event @ref OB_GATTC_EVT_READ_BY_UUID_RSP
 * @param[in] conn_idx      connection index
 * @param[in] start_handle  handle range
 * @param[in] end_handle    handle range
 * @param[in] uuid          UUID
 * @param[in] uuid_len      UUID length
 * @return result, refer to @ref ob_error
 */
uint32_t ob_gattc_read_by_uuid(uint8_t conn_idx, uint16_t start_handle, uint16_t end_handle, const uint8_t *uuid,
                               uint8_t uuid_len);

/**@brief Write request
 * @details As a GATT Client, write data to att_hdl. After peer responds, it will notify the upper layer through the event @ref OB_GATTC_EVT_WRITE_RSP \n
 * @details If the parameter type type is OB_GATTC_WRITE_CMD, no response event @ref OB_GATTC_EVT_WRITE_RSP \n
 * @details If the parameter type type is OB_GATTC_WRITE_CMD, after the data is sent to the peer device, the upper layer will be notified through the @ref OB_GATT_EVT_TX_COMPLETE event. You can call ob_gattc_write continuously to cache the data to be written to increase the communication rate
 * @param[in] conn_idx  connection index
 * @param[in] att_hdl   att handle
 * @param[in] type      write type, refer to @ref ob_gattc_write_type
 * @param[in] data      data
 * @param[in] len       data length, automatically truncated if it exceeds the current MTU-3
 * @note When calling ob_gattc_write to send data continuously, please note that the unsent data needs to occupy memory for buffering, so the amount of buffered data should be controlled according to the @ref OB_GATT_EVT_TX_COMPLETE event to prevent memory exhaustion
 * @return result, refer to @ref ob_error
 */
uint32_t ob_gattc_write(uint8_t conn_idx, uint16_t att_hdl, uint8_t type, const uint8_t *data, int len);

/**@brief indicate response
 * @details This interface is used to respond to @ref OB_GATTC_EVT_HVX_IND request (only when type == OB_HANDLE_VALUE_IND in @ref ob_gattc_evt_hvx_ind_t parameter)
 * @param[in] conn_idx  connection index
 * @return result, refer to @ref ob_error
 */
uint32_t ob_gattc_indicate_cfm(uint8_t conn_idx);

#endif /* __OMBLE_GATT_H__ */

/// @}
