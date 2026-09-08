/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @addtogroup OMBLE_GAP
 * @brief GAP
 * @details GAP related API mainly control Bluetooth advertising, scanning, connection and pairing functions
 * @version
 * Version 1.0
 *  - Initial release
 *
 */
/// @{

#ifndef __OMBLE_GAP_H__
#define __OMBLE_GAP_H__
#include <stdint.h>
#include <stdbool.h>
#include "omble_range.h"

/// OB_GAP_EVENTS
enum OB_GAP_EVENTS {
    /// Connection established event, refer to @ref ob_gap_evt_connected_t
    OB_GAP_EVT_CONNECTED                    = OB_GAP_EVTS_BASE + 0x00,
    /// Connection disconnected event, refer to @ref ob_gap_evt_disconnected_t
    OB_GAP_EVT_DISCONNECTED                 = OB_GAP_EVTS_BASE + 0x01,
    /// Advertising state changed event, refer to @ref ob_gap_evt_adv_state_changed_t
    OB_GAP_EVT_ADV_STATE_CHANGED            = OB_GAP_EVTS_BASE + 0x02,
    /// Connection Parameter update event, refer to @ref ob_gap_evt_conn_params_update_t
    OB_GAP_EVT_CONN_PARAMS_UPDATE           = OB_GAP_EVTS_BASE + 0x03,
    /// Connection parameter update request, refer to @ref ob_gap_evt_conn_params_request_t
    OB_GAP_EVT_CONN_PARAMS_REQUEST          = OB_GAP_EVTS_BASE + 0x04,
    /// Advertising report event, refer to @ref ob_gap_evt_adv_report_t
    OB_GAP_EVT_ADV_REPORT                   = OB_GAP_EVTS_BASE + 0x05,
    /// Link encryption state changed event, refer to @ref ob_gap_evt_encrypt_t
    OB_GAP_EVT_ENCRYPT                      = OB_GAP_EVTS_BASE + 0x06,
    /// Security request event, only supported by central role, refer to @ref ob_gap_evt_sec_request_t
    OB_GAP_EVT_SEC_REQUEST                  = OB_GAP_EVTS_BASE + 0x10,
    /// Pairing request event, only supported by peripheral role, refer to @ref ob_gap_evt_pairing_request_t
    OB_GAP_EVT_PAIRING_REQUEST              = OB_GAP_EVTS_BASE + 0x11,
    /// Pairing PIN code request event, refer to @ref ob_gap_evt_pin_request_t
    OB_GAP_EVT_PIN_REQUEST                  = OB_GAP_EVTS_BASE + 0x12,
    /// LTK request, only supported by peripheral role, refer to @ref ob_gap_evt_ltk_request_t
    OB_GAP_EVT_LTK_REQUEST                  = OB_GAP_EVTS_BASE + 0x13,
    /// Binding information request, refer to @ref ob_gap_evt_bond_info_request_t
    OB_GAP_EVT_BOND_INFO_REQUEST            = OB_GAP_EVTS_BASE + 0x14,
    /// Pairing completion event, refer to @ref ob_gap_evt_bonded_t
    OB_GAP_EVT_BONDED                       = OB_GAP_EVTS_BASE + 0x15,
    /// Pairing information reporting event (LTK/IRK), refer to @ref ob_gap_evt_bond_info_t
    OB_GAP_EVT_BOND_INFO                    = OB_GAP_EVTS_BASE + 0x16,
    /// Timeout event (scan), refer to @ref ob_gap_evt_timeout_t
    OB_GAP_EVT_TIMEOUT                      = OB_GAP_EVTS_BASE + 0x20,
    /// Power changed event, refer to @ref ob_gap_evt_power_changed_t
    OB_GAP_EVT_POWER_CHANGED                = OB_GAP_EVTS_BASE + 0x21,
    /// PHY changed event, refer to @ref ob_gap_evt_phy_update_t
    OB_GAP_EVT_PHY_UPDATE                   = OB_GAP_EVTS_BASE + 0x22,
    /// Data Length changed event, refer to @ref ob_gap_evt_data_len_changed_t
    OB_GAP_EVT_DATA_LEN_CHANGED             = OB_GAP_EVTS_BASE + 0x23,
    /// Device information event, refer to @ref ob_gap_evt_device_info_t
    OB_GAP_EVT_DEVICE_INFO                  = OB_GAP_EVTS_BASE + 0x24,
    /// Scan request event received, refer to @ref ob_gap_evt_scan_req_recv_t
    OB_GAP_EVT_SCAN_REQ_RECV                = OB_GAP_EVTS_BASE + 0x25,
    /// Periodic advertising synchronization event, refer to @ref ob_gap_evt_padv_sync_t
    OB_GAP_EVT_PADV_SYNC                    = OB_GAP_EVTS_BASE + 0x30,
    /// Periodic advertising report event, refer to @ref ob_gap_evt_padv_report_t
    OB_GAP_EVT_PADV_REPORT                  = OB_GAP_EVTS_BASE + 0x31,
    /// Periodic advertising subevent data request event, refer to @ref ob_gap_evt_padv_sub_data_request_t
    OB_GAP_EVT_PADV_SUB_DATA_REQUEST        = OB_GAP_EVTS_BASE + 0x32,
    /// Periodic advertising subevent response data report, refer to @ref ob_gap_evt_padv_resp_report_t
    OB_GAP_EVT_PADV_RESP_REPORT             = OB_GAP_EVTS_BASE + 0x33,
    /// HCI instruction error (for debugging), refer to @ref ob_gap_evt_hci_error_t
    OB_GAP_EVT_HCI_ERROR                    = OB_GAP_EVTS_BASE + 0xFF,
};

/// @brief advertising attribute bit
/// @details
/// If the bit OB_ADV_PROP_BIT4_LEGACY_ADV is enabled, the legacy advertising is used, the maximum advertising data length is 31 bytes, and the advertising attribute only supports the following combinations:\n
/// OB_ADV_PROP_BIT4_LEGACY_ADV | OB_ADV_PROP_BIT1_SCANNABLE       | OB_ADV_PROP_BIT0_CONNECTALBE\n
/// OB_ADV_PROP_BIT4_LEGACY_ADV | OB_ADV_PROP_BIT2_DIRECTED_ADV    | OB_ADV_PROP_BIT0_CONNECTALBE\n
/// OB_ADV_PROP_BIT4_LEGACY_ADV | OB_ADV_PROP_BIT3_HIGH_DUTY_CYCLE | OB_ADV_PROP_BIT0_CONNECTALBE\n
/// OB_ADV_PROP_BIT4_LEGACY_ADV | OB_ADV_PROP_BIT1_SCANNABLE\n
/// If bit OB_ADV_PROP_BIT4_LEGACY_ADV is not enabled, extended advertising is used, and the maximum advertising data length is 251 bytes\n
enum ob_adv_properties_bits {
    OB_ADV_PROP_BIT0_CONNECTALBE        = 1 << 0, ///< connectable
    OB_ADV_PROP_BIT1_SCANNABLE          = 1 << 1, ///< respond to scan request
    OB_ADV_PROP_BIT2_DIRECTED_ADV       = 1 << 2, ///< direct advertising
    OB_ADV_PROP_BIT3_HIGH_DUTY_CYCLE    = 1 << 3, ///< high-speed direct advertising
    OB_ADV_PROP_BIT4_LEGACY_ADV         = 1 << 4, ///< legacy advertising
    OB_ADV_PROP_BIT5_OMIT_ADV_ADDRESS   = 1 << 5, ///< anonymous advertising
    OB_ADV_PROP_BIT6_INCLUDE_TX_POWER   = 1 << 6, ///< Tx Power included
};

/// Predefined advertising attributes
enum ob_adv_properties {
    OB_ADV_PROP_LEGACY_IND                  = 0x13, ///< Legacy advertising
    OB_ADV_PROP_LEGACY_DIRECT_IND_LOW       = 0x15, ///< Low-speed direct legacy advertising, advertising interval defined by parameters
    OB_ADV_PROP_LEGACY_DIRECT_IND_HIGH      = 0x1D, ///< High-speed direct legacy advertising, advertising interval controlled by controller (less than 3.75ms)
    OB_ADV_PROP_LEGACY_SCAN_IND             = 0x12, ///< Scannable but not connectable legacy advertising
    OB_ADV_PROP_LEGACY_NONCONN_IND          = 0x10, ///< Not connectable and not scannable legacy advertising
    OB_ADV_PROP_EXT_CONN_NONSCAN            = 0x01, ///< Connectable but not scannable extended advertising
    OB_ADV_PROP_EXT_NONCONN_SCAN            = 0x02, ///< Unconnectable scannable extended advertising
    OB_ADV_PROP_EXT_NONCONN_NONSCAN         = 0x00, ///< Unconnectable non-scannable extended advertising
    OB_ADV_PROP_EXT_ANONYMOUS               = 0x20, ///< Anonymous extended advertising (ignore advertising address)
};

/// Advertising Filter Accept List settings
enum ob_adv_filter_policy {
    OB_ADV_FILTER_NONE,         ///< Disable Filter Accept List function
    OB_ADV_FILTER_SCAN,         ///< Respond to connection requests from all devices, but only respond to scan requests from devices in the Filter Accept List
    OB_ADV_FILTER_CONN,         ///< Respond to scan requests from all devices, but only respond to connection requests from devices in the Filter Accept List
    OB_ADV_FILTER_CONN_SCAN,    ///< Respond only to scan requests and connection requests from devices in the Filter Accept List
};

/// Connection Filter Accept List settings
enum ob_conn_filter_policy {
    OB_CONN_FILTER_NOT_USE,     ///< Do not enable the Filter Accept List function when creating a connection
    OB_CONN_FILTER_USE,         ///< Enable the Filter Accept List function when creating a connection, the address parameter is unused, and only connect to the addresses in the Filter Accept List
};

/// Scan Filter Accept List settings
enum ob_scan_filter_policy {
    OB_SCAN_FILTER_BASIC_UNFILTER,
    OB_SCAN_FILTER_BASIC_FILTER,
    OB_SCAN_FILTER_EXT_UNFILTER,
    OB_SCAN_FILTER_EXT_FILTER,
};

/// Advertising channel
enum ob_adv_channel {
    OB_ADV_CH_37  = 0x01,
    OB_ADV_CH_38  = 0x02,
    OB_ADV_CH_39  = 0x04,
    OB_ADV_CH_ALL = 0x07,
};

/// Advertising address type
enum ob_adv_addr_type {
    OB_ADV_ADDR_TYPE_PUBLIC,
    OB_ADV_ADDR_TYPE_RANDOM,
};

/// Advertising PHY
enum ob_adv_phy {
    OB_ADV_PHY_1M = 0x01,
    OB_ADV_PHY_2M = 0x02,
    OB_ADV_PHY_CODED = 0x03,
};

/// Binding information type
enum ob_bond_info_type {
    OB_BOND_INFO_LTK,
    OB_BOND_INFO_IRK,
};

/// Pairing IO capability type
enum ob_smp_io_capability {
    OB_SMP_IOCAP_DIS_ONLY, // DisplayOnly
    OB_SMP_IOCAP_DISP_YN,  // DisplayYesNo
    OB_SMP_IOCAP_KBD_ONLY, // KeyboardOnly
    OB_SMP_IOCAP_NONE,     // NoInputNoOutput
    OB_SMP_IOCAP_KDB_DISP, // KeyboardDisplay
};

/// Pairing pin type
enum ob_smp_pin_type {
    OB_SMP_PIN_TYPE_NONE,       ///< No PIN required, just report pairing started
    OB_SMP_PIN_TYPE_OOB_REQ,    ///< Request oob input
    OB_SMP_PIN_TYPE_DIS_YN,     ///< Display pin code and request confirmation
    OB_SMP_PIN_TYPE_PK_REQ,     ///< Request passkey input
    OB_SMP_PIN_TYPE_DIS,        ///< Display pin code
};

/// Connection role
enum ob_gap_conn_role {
    OB_GAP_ROLE_CENTRAL,            ///< central
    OB_GAP_ROLE_PHERIPHERAL,        ///< peripheral
    OB_GAP_ROLE_CENTRAL_PAWR,       ///< central connected by PAwR
    OB_GAP_ROLE_PHERIPHERAL_PAWR,   ///< peripheral connected by PAwR
};

/// Distribution key type
enum ob_smp_distribution {
    OB_SMP_DIST_BIT_ENC_KEY = 0x01, ///< EncKey
    OB_SMP_DIST_BIT_ID_KEY  = 0x02, ///< IdKey
};

/// Timeout reason
enum ob_gap_timeout_source_t {
    OB_GAP_TOUT_SCAN,               ///< Scan timeout
    OB_GAP_TOUT_CONN_PARAM_UPDATE,  ///< Connectino Parameter update timeout
};

/// Device information type
enum ob_dev_info_type {
    OB_DEV_INFO_PEER_VERSION,       ///< peer device version information
    OB_DEV_INFO_PEER_LE_FEATURE,    ///< peer device feature information
    OB_DEV_INFO_LOCAL_VERSION,      ///< local device version information
    OB_DEV_INFO_LOCAL_LE_FEATURE,   ///< local device feature information
    OB_DEV_INFO_RSSI,               ///< RSSI of the specified connection
    OB_DEV_INFO_CHANNEL_MAP,        ///< ChannelMap information of the specified connection
};

/// PHY type
enum ob_gap_phy {
    OB_GAP_PHY_1M       = 0x01,
    OB_GAP_PHY_2M       = 0x02,
    OB_GAP_PHY_CODED    = 0x03,
};

/// PHY Options
enum ob_gap_phy_ops {
    OB_GAP_PHY_NO_PREFERRED = 0x00,
    OB_GAP_PHY_PREFERS_S2   = 0x01,
    OB_GAP_PHY_PREFERS_S8   = 0x02,
};

#define OB_GAP_ADDR_LEN 6               ///< Address length
#define OB_GAP_RANDOM_LEN 8             ///< Random value length
#define OB_GAP_KEY_LEN 16               ///< Key length
#define OB_GAP_ADV_TX_POWER_NO_AVA 0x7F ///< Transmit power is not available

/// GAP address
typedef struct ob_gap_addr {
    uint8_t addr_type;              ///< See @ref ob_adv_addr_type.
    uint8_t addr[OB_GAP_ADDR_LEN];  ///< 48-bit address, LSB format.
} ob_gap_addr_t;

/// Advertising parameter
typedef struct ob_adv_param {
    uint32_t prim_intv_min;   ///< unit of 0.625ms
    uint32_t prim_intv_max;   ///< unit of 0.625ms
    uint16_t adv_properties;  ///< refer to @ref ob_adv_properties
    uint8_t own_addr_type;    ///< refer to @ref ob_adv_addr_type
    uint8_t prim_ch_map;      ///< refer to @ref ob_adv_channel
    ob_gap_addr_t *peer_addr; ///< peer address. If the Filter Accept List is enabled, this parameter is unused. Otherwise, this parameter can NOT be NULL.
    uint8_t filter_policy;    ///< refer to @ref ob_adv_filter_policy
    uint8_t prim_phy;         ///< refer to @ref ob_adv_phy @note OB_ADV_PHY_2M not support in primary phy
    uint8_t secd_phy;         ///< refer to @ref ob_adv_phy
    int8_t tx_pwr;            ///< -127 ~ +20, 0x7F: Host has no preference
    uint16_t timeout;         ///< Timeout unit: 10ms, 0 means no timeout
    uint8_t *local_addr;      ///< Local address. If local_addr == NULL, a default random address is used.
} ob_adv_param_t;

/// Advertising data/scan data
/// @note If the type is legacy advertising, the maximum value of len is 31, else the maximum value of len is 251
typedef struct ob_data {
    uint8_t *data;       ///< Data
    int len;             ///< Data length
} ob_data_t;

/// Connection parameters
typedef struct {
    uint16_t conn_intv;      ///< Connection interval (unit: 1.25 ms)
    uint16_t latency_max;    ///< Maximum latency
    uint16_t timeout;        ///< Connection timeout (unit: 10 ms)
} ob_gap_conn_params_t;

/// Connection parameters V2
typedef struct {
    uint16_t conn_intv_min;  ///< Minimum connection interval (unit: 1.25 ms)
    uint16_t conn_intv_max;  ///< Maximum connection interval (unit: 1.25 ms)
    uint16_t latency_max;    ///< Maximum latency
    uint16_t timeout;        ///< Connection timeout (unit: 10 ms)
} ob_gap_conn_parameter_t;

/// Bonding information
typedef struct {
    uint8_t type; ///< @ref ob_bond_info_type
    union {
        /// Identical Address and IRK
        struct {
            ob_gap_addr_t id_addr;
            uint8_t irk[OB_GAP_KEY_LEN];
        } id_info;
        /// EDIV, Random and LTK
        struct {
            uint16_t ediv;
            uint8_t random[OB_GAP_RANDOM_LEN];
            uint8_t ltk[OB_GAP_KEY_LEN];
        } enc_info;
    };
} ob_bond_info_t;

/// Response pin code
typedef struct {
    uint8_t type;                      ///< refer to @ref ob_smp_pin_type
    union {
        uint8_t oob[OB_GAP_KEY_LEN];   ///< OOB data
        uint32_t passkey;              ///< Key data
    };
} ob_smp_pin_t;

/// Advertising changed status
enum ob_gap_adv_state {
    OB_GAP_ADV_ST_STARTED,                  ///< Advertising is enabled
    OB_GAP_ADV_ST_STOPPED_BY_USER,          ///< User manually disables advertising
    OB_GAP_ADV_ST_STOPPED_BY_CONNECTED,     ///< Connection successful
    OB_GAP_ADV_ST_STOPPED_BY_TIMEOUT,       ///< Advertising timeout
    OB_GAP_ADV_ST_STOPPED_BY_EVENT,         ///< Max_Extended_Advertising_Events count reached
    OB_GAP_ADV_ST_STOPPED_UNEXPECTED,       ///< Advertising is disabled for unknown reasons
    OB_GAP_PADV_ST_STARTED,                 ///< Periodic advertising is enabled
    OB_GAP_PADV_ST_STOPPED,                 ///< Periodic advertising is stopped
};

/// Transmit power level mark
enum ob_gap_tx_pwr_level_flag {
    OB_GAP_TX_PWR_LEVEL_MIN = 0x01,         ///< The transmission power reaches the minimum value
    OB_GAP_TX_PWR_LEVEL_MAX = 0x02,         ///< The transmission power reaches the maximum value
};

/// Advertising data status
enum ob_gap_report_state {
    OB_GAP_REPORT_ST_COMPLETE,          ///< Complete
    OB_GAP_REPORT_ST_MORE_DATA,         ///< Incomplete, more data to come
    OB_GAP_REPORT_ST_TRUNCATED,         ///< Incomplete, data truncated, no more to come
};

/// Event structure for @ref OB_GAP_EVT_CONNECTED.
typedef struct {
    uint8_t role;                       ///< Connection role, refer to @ref ob_gap_conn_role
    uint8_t adv_idx;                    ///< Advertising index corresponding to the current connection, if role is Central, this parameter is unused
    ob_gap_addr_t peer_addr;            ///< The peer address, refer to @ref ob_gap_addr_t
    uint8_t pawr_idx;                   ///< Periodic Advertising index or periodic Advertising synchronization index corresponding to PAwR connection
    ob_gap_conn_params_t conn_params;   ///< Connection parameters, refer to @ref ob_gap_conn_params_t
} ob_gap_evt_connected_t;

/// Event structure for @ref OB_GAP_EVT_DISCONNECTED.
typedef struct {
    uint8_t reason;                     ///< Disconnection reason
} ob_gap_evt_disconnected_t;

/// Event structure for @ref OB_GAP_EVT_ADV_STATE_CHANGED.
typedef struct {
    uint8_t adv_idx;                   ///< Advertising index
    uint8_t state;                     ///< Advertising status, refer to @ref ob_gap_adv_state
} ob_gap_evt_adv_state_changed_t;

/// Event structure for @ref OB_GAP_EVT_CONN_PARAMS_UPDATE.
typedef struct {
    uint16_t conn_intv_min;     ///< Minimum connection interval (unit: 1.25 ms)
    uint16_t conn_intv_max;     ///< Maximum connection interval (unit: 1.25 ms)
    uint16_t latency_max;       ///< Maximum latency
    uint16_t timeout;           ///< Connection timeout (unit: 10 ms)
} ob_gap_evt_conn_params_update_t;

/// Event structure for @ref OB_GAP_EVT_CONN_PARAMS_REQUEST.
typedef struct {
    uint16_t conn_intv_min;     ///< Minimum connection interval (unit: 1.25 ms)
    uint16_t conn_intv_max;     ///< Maximum connection interval (unit: 1.25 ms)
    uint16_t latency_max;       ///< Maximum latency
    uint16_t timeout;           ///< Connection timeout (unit: 10 ms)
} ob_gap_evt_conn_params_request_t;

/// Advertising report type
typedef struct {
    uint8_t connectable     : 1; ///< connectable advertising flag
    uint8_t scannable       : 1; ///< scannable advertising flag
    uint8_t direct          : 1; ///< direct advertising flag
    uint8_t scan_resp       : 1; ///< scan response data flag
    uint8_t legacy_adv      : 1; ///< Legacy advertising flag
    uint8_t data_state      : 2; ///< data state, refer to @ref ob_gap_report_state
    uint8_t rsv             : 1; ///< reserved
} ob_gap_report_event_type_t;

/// Event structure for @ref OB_GAP_EVT_ADV_REPORT.
typedef struct {
    ob_gap_report_event_type_t report_type; ///< Advertising type @ref ob_gap_report_event_type_t
    ob_gap_addr_t addr;             ///< Address of advertising device
    ob_gap_addr_t
    direct_adv_addr;  ///< Direct advertising destination address. If direct in report_type is 0, this parameter is unused
    uint8_t prim_phy;               ///< Primary advertising channel phy, refer to @ref ob_adv_phy
    uint8_t secd_phy;               ///< Secondary advertising channel phy, refer to @ref ob_adv_phy
    uint8_t adv_sid;                ///< 0x00~0x0F, 0xFF means No ADI field provided
    uint16_t padv_intv;             ///< Corresponding to the interval of periodic advertising (unit: 1.25 ms). If it is 0x0000, it means no periodic advertising
    int8_t tx_pwr;                  ///< Transmit power, if the value is @ref OB_GAP_ADV_TX_POWER_NO_AVA, it means it is not available
    int8_t rssi;                    ///< Signal Strength
    const uint8_t *data;            ///< Advertising data
    uint8_t data_len;               ///< Advertising data length
} ob_gap_evt_adv_report_t;

/// Event structure for @ref OB_GAP_EVT_ENCRYPT.
typedef struct {
    bool encrypted;       ///< Current encryption status of the link
} ob_gap_evt_encrypt_t;

/// SMP Authentication requirements flags
typedef struct {
    uint8_t bond_flags  : 2; ///< Whether to bond
    uint8_t mitm        : 1; ///< Whether to support MITM
    uint8_t sc          : 1; ///< Whether to support secure pairing
    uint8_t keypress    : 1; ///< Whether to support key notification
    uint8_t rsv         : 3; ///< reserved
} ob_gap_auth_t;

/// Event structure for @ref OB_GAP_EVT_SEC_REQUEST.
typedef struct {
    ob_gap_auth_t auth; ///< Request security attributes and binding information, refer to @ref ob_gap_auth_t
} ob_gap_evt_sec_request_t;

/// Event structure for @ref OB_GAP_EVT_PAIRING_REQUEST.
typedef struct {
    uint8_t io_capability;              ///< io_capability, @ref ob_smp_io_capability
    uint8_t oob_data_flag;              ///< Whether to support oob_data
    ob_gap_auth_t authreq;              ///< refer to @ref ob_gap_auth_t
    uint8_t initiator_key_distribution; ///< initiator_key_distribution @ref ob_smp_distribution
    uint8_t responder_key_distribution; ///< responder_key_distribution @ref ob_smp_distribution
} ob_gap_evt_pairing_request_t;

/// Event structure for @ref OB_GAP_EVT_PIN_REQUEST.
typedef struct {
    uint8_t type; ///< refer to @ref ob_smp_pin_type
    uint32_t pin_code; ///< only valid if type == OB_SMP_PIN_TYPE_DIS
} ob_gap_evt_pin_request_t;

/// Event structure for @ref OB_GAP_EVT_LTK_REQUEST.
typedef struct {
    uint16_t ediv; ///< EDIV
    uint8_t random[OB_GAP_RANDOM_LEN]; ///< RANDOM
} ob_gap_evt_ltk_request_t;

/// Event structure for @ref OB_GAP_EVT_BOND_INFO_REQUEST.
typedef struct {
    uint8_t type; ///< @ref ob_bond_info_type
} ob_gap_evt_bond_info_request_t;

/// Event structure for @ref OB_GAP_EVT_BONDED.
typedef struct {
    uint32_t status;     ///< Binding Status
    ob_gap_auth_t auth;  ///< Binding success information, refer to @ref ob_gap_auth_t
} ob_gap_evt_bonded_t;

/// Event structure for @ref OB_GAP_EVT_BOND_INFO.
typedef struct {
    uint8_t type; ///< @ref ob_bond_info_type
    union {
        /// Identical Address and IRK
        struct {
            ob_gap_addr_t id_addr;
            uint8_t irk[OB_GAP_KEY_LEN];
        } id_info;
        /// EDIV, Random and LTK
        struct {
            uint16_t ediv;
            uint8_t random[OB_GAP_RANDOM_LEN];
            uint8_t ltk[OB_GAP_KEY_LEN];
        } enc_info;
    };
} ob_gap_evt_bond_info_t;

/// Event structure for @ref OB_GAP_EVT_TIMEOUT.
typedef struct {
    uint8_t source; ///< Timeout reason��refer to @ref ob_gap_timeout_source_t
} ob_gap_evt_timeout_t;

/// Event structure for @ref OB_GAP_EVT_POWER_CHANGED.
typedef struct {
    uint8_t side;               ///< Update the transmit power of this party or the other party, 0x00 local transmit power update, 0x01 other party transmit power update
    uint8_t phy;                ///< Update the PHY corresponding to the power, 0x01 1M Phy, 0x02 2M Phy, 0x03 coded Phy(S=8), 0x04 coded Phy(S=2)
    int8_t tx_pwr;              ///< Updated power, if the value is @ref OB_GAP_ADV_TX_POWER_NO_AVA(0x7F), it means unavailable
    uint8_t tx_pwr_level_flag;  ///< Whether the maximum or minimum power is reached, refer to @ref ob_gap_tx_pwr_level_flag
    int8_t tx_pwr_delta;        ///< Power change, if the value is @ref OB_GAP_ADV_TX_POWER_NO_AVA(0x7F) means unavailable
} ob_gap_evt_power_changed_t;

/// Event structure for @ref OB_GAP_EVT_PHY_UPDATE.
typedef struct {
    uint8_t status;                 ///< Update status, 0 means the update is successful, otherwise it means the update failed
    uint8_t tx_phy;                 ///< TX PHY for this connection��refer to @ref ob_gap_phy
    uint8_t rx_phy;                 ///< RX PHY for this connection��refer to @ref ob_gap_phy
} ob_gap_evt_phy_update_t;

/// Event structure for @ref OB_GAP_EVT_DATA_LEN_CHANGED.
typedef struct {
    uint16_t max_tx_octets;     ///< Maximum number of bytes sent
    uint16_t max_rx_octets;     ///< Maximum number of bytes received
    uint16_t max_tx_time_us;    ///< Maximum sending time (unit: us)
    uint16_t max_rx_time_us;    ///< Maximum receiving time (unit: us)
} ob_gap_evt_data_len_changed_t;

/// Event structure for @ref OB_GAP_EVT_DEVICE_INFO.
typedef struct {
    uint8_t type;                         ///< Information Type @ref ob_dev_info_type
    union {
        /// bluetooth version
        struct {
            uint8_t version;              ///< version
            uint8_t subversion;           ///< subversion
            uint16_t company_identifier;  ///< company_identifier
        } version;
        uint8_t features[8];              ///< device feature��refer to core spec [Vol 6] Part B, Section 4.6
        int8_t rssi;                      ///< Signal Strength
        uint8_t channel_map[5];           ///< Channel map
    };
} ob_gap_evt_device_info_t;

/// Event structure for @ref ob_gap_evt_scan_req_recv_t.
typedef struct {
    ob_gap_addr_t addr;                         ///< The address of the advertising device
    uint8_t adv_idx;                            ///< advertise index
} ob_gap_evt_scan_req_recv_t;

enum ob_padv_sync_state_type {
    OB_PADV_SYNC_STATE_SUCCESS,     ///< Synchronization is successfully established
    OB_PADV_SYNC_STATE_TIMEOUT,     ///< Synchronization timeout failure
    OB_PADV_SYNC_STATE_CANCEL,      ///< User cancels the synchronization operation
    OB_PADV_SYNC_STATE_STOPPED,     ///< User stops the synchronization operation
    OB_PADV_SYNC_STATE_LOST,        ///< Synchronization is lost
    OB_PADV_SYNC_STATE_FAILED,      ///< Synchronization failed
    OB_PADV_SYNC_TRANS_SUCCESS,     ///< Periodic advertising synchronization transmission is successful
    OB_PADV_SYNC_TRANS_FAILED,      ///< Periodic advertising synchronization transmission fails
};

/// Event structure for @ref OB_GAP_EVT_PADV_SYNC.
typedef struct {
    uint8_t state;                      ///< Periodic advertising synchronization state, @ref ob_padv_sync_state_type
    uint8_t sync_idx;                   ///< Periodic advertising synchronization index value
    uint8_t adv_sid;                    ///< Advertising sid
    uint8_t phy;                        ///< Advertising phy, @ref ob_adv_phy
    const ob_gap_addr_t *addr;          ///< Advertising address
    uint16_t service_data;              ///< Only valid when state==OB_PADV_SYNC_STATE_STRANS
    uint16_t interval;                  ///< Advertising interval, unit 1.25ms
    uint8_t adv_clock_acc;              ///< Advertising clock accuracy
    // parameters of period advertising with response
    uint8_t num_subevents;              ///< Number of subevents
    uint8_t subevent_interval;          ///< Subevent interval, unit 1.25ms
    uint8_t response_slot_delay;        ///< Unit 1.25ms (0x01~0xFE)
    uint8_t response_slot_spacing;      ///< Unit 0.125ms (0x02~0xFF)
} ob_gap_evt_padv_sync_t;

/// Event structure for @ref OB_GAP_EVT_ADV_REPORT.
typedef struct {
    uint8_t sync_idx;       ///< Periodic advertising synchronization index value
    int8_t tx_pwr;          ///< Transmit power, if the value is @ref OB_GAP_ADV_TX_POWER_NO_AVA, it means unavailable
    int8_t rssi;            ///< If the value is @ref OB_GAP_ADV_TX_POWER_NO_AVA, it means unavailable
    uint8_t data_state;     ///< Data state, refer to @ref ob_gap_report_state
    const uint8_t *data;    ///< Periodic advertising data
    uint8_t data_len;       ///< Periodic advertising length
    // parameters of period advertising with response
    uint16_t pawr_counter;  ///< The value of paEventCounter
    uint8_t pawr_subevent;  ///< The subevent number
} ob_gap_evt_padv_report_t;

/// Event structure for @ref OB_GAP_EVT_PADV_SUB_DATA_REQUEST.
typedef struct {
    uint8_t sync_idx;               ///< Periodic advertising synchronization index value
    uint8_t subevent_start;         ///< Requested start subevent
    uint8_t subevent_data_count;    ///< Requested number of subevents
} ob_gap_evt_padv_sub_data_request_t;

/// Event structure for @ref OB_GAP_EVT_PADV_RESP_REPORT.
typedef struct {
    uint8_t sync_idx;           ///< Periodic advertising synchronization index value
    uint8_t subevent;           ///< Periodic advertising subevent index
    uint8_t tx_status;          ///< Whether AUX_SYNC_SUBEVENT_IND is sent
    int8_t tx_pwr;              ///< Transmit power, if the value is @ref OB_GAP_ADV_TX_POWER_NO_AVA, it means unavailable
    int8_t rssi;                ///< Signal Strength
    uint8_t response_slot;      ///< response_slot
    uint8_t data_state;         ///< Data status, refer to @ref ob_gap_report_state
    const uint8_t *data;        ///< Periodic advertising data
    uint8_t data_len;           ///< Periodic advertising length
} ob_gap_evt_padv_resp_report_t;

/// Event structure for @ref OB_GAP_EVT_HCI_ERROR.
typedef struct {
    uint16_t hci_opcode;     ///< HCI opcode corresponding to the error
    uint8_t status;          ///< HCI status when an error occurs
} ob_gap_evt_hci_error_t;

/// GAP event structure
typedef struct {
    uint8_t conn_idx; ///< Connection index
    union {
        ob_gap_evt_connected_t              connected;             ///< Connection established event parameters
        ob_gap_evt_disconnected_t           disconnected;          ///< Connection disconnected event parameters
        ob_gap_evt_adv_state_changed_t      adv_state_changed;     ///< Advertising state changed event parameters
        ob_gap_evt_conn_params_update_t     conn_params_update;    ///< Connection Parameter update completion event parameters
        ob_gap_evt_conn_params_request_t    conn_params_request;   ///< Connection Parameters Update Request Parameters
        ob_gap_evt_adv_report_t             adv_report;            ///< Advertising reporting event parameters
        ob_gap_evt_encrypt_t                encrypt;               ///< Link encryption status changed event parameters
        ob_gap_evt_sec_request_t            sec_request;           ///< Encryption request event parameters
        ob_gap_evt_pairing_request_t        pairing_request;       ///< Pairing request event parameters
        ob_gap_evt_pin_request_t            pin_request;           ///< Pairing PIN request event parameters
        ob_gap_evt_ltk_request_t            ltk_request;           ///< LTK Request Parameters
        ob_gap_evt_bond_info_request_t      bond_info_request;     ///< Binding information request parameters
        ob_gap_evt_bonded_t                 bonded;                ///< Pairing completed event parameters
        ob_gap_evt_bond_info_t              bond_info;             ///< Pairing information reporting event (LTK/IRK) parameters
        ob_gap_evt_timeout_t                timeout;               ///< Timeout event parameters
        ob_gap_evt_power_changed_t          power_changed;         ///< Power Change Event Parameters
        ob_gap_evt_phy_update_t             phy_updated;           ///< PHY Change Event Parameters
        ob_gap_evt_data_len_changed_t       data_len_changed;      ///< Data Length Change Event Parameters
        ob_gap_evt_device_info_t            device_info;           ///< Device information parameters
        ob_gap_evt_scan_req_recv_t          scan_req_recv;         ///< Scan request event parameters
        ob_gap_evt_padv_sync_t              padv_sync;             ///< Periodic advertising synchronization event parameters
        ob_gap_evt_padv_report_t            padv_report;           ///< Periodic advertising reporting event parameters
        ob_gap_evt_padv_sub_data_request_t
        padv_sub_data_req;     ///< Periodic advertising subevent data request event parameters
        ob_gap_evt_padv_resp_report_t       padv_resp_report;      ///< Periodic advertising subevent response event parameters
        ob_gap_evt_hci_error_t              hci_error;             ///< HCI command error parameter
    };
} omble_gap_evt_t;

/**@brief Set the default address
 * @note The address must be set in the state of no advertising and connection
 * @note This address affects the device's connection, scan address and advertising address (only when local_addr in the advertising parameter is NULL)
 * @param[in] addr_type address type, refer to @ref ob_adv_addr_type.
 * @param[in] addr device address
 * @return result, refer to @ref ob_error
 */
uint32_t ob_gap_addr_set(uint8_t addr_type, const uint8_t addr[OB_GAP_ADDR_LEN]);

/**@brief Get the default address
 * @param[in]  addr_type address type, refer to @ref ob_adv_addr_type.
 * @param[out] addr address pointer
 * @return result, refer to @ref ob_error
 */
uint32_t ob_gap_addr_get(uint8_t addr_type, uint8_t addr[OB_GAP_ADDR_LEN]);

/**@brief Set Filter Accept List
 * @details Can only be called when advertising is not enabled. If fa_addrs is not NULL, will clear the current Filter Accept list and set the addresses in the fa_addrs list to the Filter Accept List.
 * @details If fa_addrs is NULL, only clear the current Filter Accept list
 * @param[in] fa_addrs filter accept address pointer group
 * @param[in] len array length
 * @return result, refer to @ref ob_error
 */
uint32_t ob_gap_accept_list_set(const ob_gap_addr_t *fa_addrs, uint8_t len);

/**@brief Enable the advertising of the specified adv_idx, or update the advertising data
 * @details In the multi-advertising scenario, the parameter adv_idx is used to distinguish different advertisings, and different advertisings can be configured with different advertising addresses\n
 * If local_addr in the parameter adv_param is NULL, the advertising uses the address set by @ref ob_gap_addr_set, otherwise the address specified by local_addr is used
 * @param[in] adv_idx advertising index
 * @param[in] adv_param advertising parameter refer to @ref ob_adv_param_t, if NULL, only the advertising data of the current advertising is updated
 * @param[in] adv_data advertising data
 * @param[in] scan_rsp_data scan data
 * @note If the advertising data need to be updated, the advertising must be enabled
 * @note The value of the parameter adv_idx must be less than the maximum number of advertising supported by the HOST. The maximum number of advertising is defined when the protocol stack is initialized, see function @ref omble_init Parameters @ref ob_stack_param
 * @return result, refer to @ref ob_error
 */
uint32_t ob_gap_adv_start(uint8_t adv_idx, ob_adv_param_t *adv_param, ob_data_t *adv_data, ob_data_t *scan_rsp_data);

/**@brief Stop the advertising of the specified index
 * @param[in] adv_idx advertising index
 * @return result, refer to @ref ob_error
 */
uint32_t ob_gap_adv_stop(uint8_t adv_idx);

/**@brief Request connection parameter update
 * @param[in]  conn_idx       Connection index
 * @param[in]  p_conn_params  Connection parameters��refer to @ref ob_gap_conn_params_t
 * @return result��refer to @ref ob_error
 * @note Update connection parameters when acting as a centrel role, and request connection parameters when acting as a peripheral role
 */
uint32_t ob_gap_conn_param_update(uint8_t conn_idx, ob_gap_conn_params_t const *p_conn_params);

/**@brief Request connection parameter update V2
 * @param[in]  conn_idx       Connection index
 * @param[in]  p_conn_params  Connection parameters��refer to @ref ob_gap_conn_parameter_t
 * @return result��refer to @ref ob_error
 * @note Update connection parameters when acting as a centrel role, and request connection parameters when acting as a peripheral role
 * @note The difference between this interface and @ref ob_gap_conn_param_update is that the requested parameters distinguish between the maximum connection interval and the minimum connection interval.
 */
uint32_t ob_gap_conn_param_request(uint8_t conn_idx, ob_gap_conn_parameter_t const *p_conn_params);

/**@brief Disconnect
 * @param[in]  conn_idx        Connection index
 * @param[in]  hci_status_code Disconnection reason
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_disconnect(uint8_t conn_idx, uint8_t hci_status_code);

/**@brief Pairing/encryption request
 * @param[in]  conn_idx       Connection index
 * @param[in]  auth           auth��refer to @ref ob_gap_auth_t
 * @note Only peripheral roles can call this function
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_security_request(uint8_t conn_idx, ob_gap_auth_t auth);

/// GAP SMP pairing parameter structure
typedef struct {
    uint8_t io_capability;              ///< io_capability, @ref ob_smp_io_capability
    uint8_t oob_data_flag;              ///< Whether OOB is supported
    ob_gap_auth_t authreq;              ///< authreq��refer to @ref ob_gap_auth_t
    uint8_t initiator_key_distribution; ///< initiator_key_distribution @ref ob_smp_distribution
    uint8_t responder_key_distribution; ///< responder_key_distribution @ref ob_smp_distribution
} ob_pairing_param_t;

/**@brief Pairing/bonding request
 * @param[in]  conn_idx       Connection index
 * @param[in]  request        Pairing request parameters��refer to @ref ob_pairing_param_t
 * @note Only the central role can call this function
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_pairing_request(uint8_t conn_idx, ob_pairing_param_t *request);

/**@brief Pairing/bonding response
 * @param[in]  conn_idx       Connection index
 * @param[in]  response       Pairing Response Parameters��refer to @ref ob_pairing_param_t
 * @note Only the peripheral role can call this function to respond to the pairing request after receiving the event @ref OB_GAP_EVT_PAIRING_REQUEST
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_pairing_response(uint8_t conn_idx, ob_pairing_param_t *response);

/**@brief Abort the pairing operation
 * @param[in]  conn_idx       Connection index
 * @param[in]  reason         Reasons for pairing failure, refer to @ref ob_smp_error
 * @note To reject pairing, interrupt the pairing operation when receiving the OB_GAP_EVT_PAIRING_REQUEST message
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_pairing_abort(uint8_t conn_idx, uint16_t reason);

/**@brief Link encryption request
 * @param[in]  conn_idx       Connection index
 * @param[in]  ediv           EDIV
 * @param[in]  rand           random value
 * @param[in]  ltk            Long Term Key
 * @note Only the central role can call this function��Used to initiate link encryption process
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_encrypt(uint8_t conn_idx, uint16_t ediv, uint8_t *rand, uint8_t *ltk);

/**@brief Link Encryption Response
 * @param[in]  conn_idx       Connection index
 * @param[in]  ltk            Long Term Key
 * @note Only the peripheral role can call this function to respond to the encryption request after receiving the message @ref OB_GAP_EVT_LTK_REQUEST
 * @note If ltk is NULL, it means LTK key missing
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_ltk_response(uint8_t conn_idx, uint8_t *ltk);

/**@brief Pairing information response
 * @param[in]  conn_idx       Connection index
 * @param[in]  bond_info      bond information��refer to @ref ob_bond_info_t
 * @note This function is used to reply to the OB_GAP_EVT_BOND_INFO_REQUEST message.
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_bond_info_response(uint8_t conn_idx, ob_bond_info_t *bond_info);

/**@brief PIN code response
 * @param[in]  conn_idx       Connection index
 * @param[in]  accept         Whether to accept the pairing request
 * @param[in]  pin_info       PIN code to respond to, refer to @ref ob_smp_pin_t
 * @note This function is used to respond to the @ref OB_GAP_EVT_PIN_REQUEST event
 * @note When pin_info::type is OB_SMP_PIN_TYPE_PK_REQ/OB_SMP_PIN_TYPE_OOB_REQ, it is used to reply the corresponding PIN code.
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_pin_response(uint8_t conn_idx, uint8_t accept, ob_smp_pin_t *pin_info);

/**@brief Get the current encryption status
 * @param[in]  conn_idx       Connection index
 * @param[out] state          true/false��Indicates whether the link is encrypted
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_get_encrypt_state(uint8_t conn_idx, uint8_t *state);

/// ob_gap_phys
typedef struct {
    uint8_t phy_1m    : 1; ///< 1M phy
    uint8_t phy_2m    : 1; ///< 2M phy
    uint8_t phy_coded : 1; ///< Coded phy
    uint8_t rsv       : 5; ///< reserved
} ob_gap_phys_t;

/**@brief phy update request
 * @param[in]  conn_idx       Connection index, if conn_idx == 0xFF, set the default PHY
 * @param[in]  tx_phys        Recommended value for link transmit PHY, refer to @ref ob_gap_phys_t
 * @param[in]  rx_phys        Recommended value for link receive PHY, refer to @ref ob_gap_phys_t
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_phy_update(uint8_t conn_idx, ob_gap_phys_t tx_phys, ob_gap_phys_t rx_phys);

/**@brief Update data length
 * @param[in]  conn_idx       Connection index
 * @param[out] tx_octets      Recommended maximum data byte length
 * @param[out] tx_time        Recommended maximum data transfer time (in microseconds)
 * @note The range of tx_octets is 0x001B ~ 0x00FB
 * @note The range of tx_time is 0x0148 ~ 0x4290
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_data_length_update(uint8_t conn_idx, uint16_t tx_octets, uint16_t tx_time);

/**@brief Set Host Channel Classification
 * @param[in]  channel_map The lower 37 bits correspond to the channel number: field bad = 0, unknown = 1
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_host_channel_map_update(const uint8_t channel_map[5]);

/// ob_scanning_param
typedef struct {
    uint8_t scan_type;   ///< is Active Scanning
    uint16_t interval;   ///< Scan interval
    uint16_t window;     ///< Scan Window
} ob_scanning_param_t;

/// ob_scan_param
typedef struct {
    uint8_t addr_type;               ///< See @ref ob_adv_addr_type
    uint8_t filter_policy;           ///< @ref ob_scan_filter_policy
    uint16_t timeout;                ///< Timeout period, in 10 milliseconds
    ob_scanning_param_t *scan_1m;    ///< 1M phy scan parameters
    ob_scanning_param_t *scan_coded; ///< Coded phy scan parameters
} ob_scan_param_t;

/**@brief Start Scanning
 * @param[in]  param       Scan parameters, refer to @ref ob_scan_param_t
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_scan_start(ob_scan_param_t *param);

/**@brief Stop Scanning
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_scan_stop(void);

/// ob_conn_phy_param
typedef struct {
    uint16_t scan_intv;      ///< Scan interval
    uint16_t scan_wind;      ///< Scan Window
    uint16_t conn_intv_min;  ///< Minimum connection interval
    uint16_t conn_intv_max;  ///< Maximum connection interval
    uint16_t latency_max;    ///< Maximum latency
    uint16_t timeout;        ///< Connection timeout
} ob_conn_phy_param_t;

/// ob_conn_param
typedef struct {
    uint8_t addr_type;               ///< ob_adv_addr_type
    uint8_t filter_policy;           ///< @ref ob_conn_filter_policy
    ob_gap_addr_t peer_addr;         ///< Connection Address
    ob_conn_phy_param_t *scan_1m;    ///< 1M phy connection parameters
    ob_conn_phy_param_t *scan_2m;    ///< 2M phy connection parameters
    ob_conn_phy_param_t *scan_coded; ///< Coded phy Scanning parameters
} ob_conn_param_t;

/**@brief Create a connection
 * @param[in]  param       Connection parameters
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_connect(ob_conn_param_t *param);

/// ob_conn_param
typedef struct {
    uint8_t adv_idx;
    uint8_t subevent;
    uint8_t addr_type;               ///< ob_adv_addr_type
    ob_gap_addr_t peer_addr;         ///< Connection Address
    uint16_t conn_intv_min;          ///< Minimum connection interval
    uint16_t conn_intv_max;          ///< Maximum connection interval
    uint16_t latency_max;            ///< Maximum latency
    uint16_t timeout;                ///< Connection timeout
} ob_conn_padv_param_t;

/**@brief Create a connection through padv handle
 * @param[in]  conn_padv_param       Connection parameters
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_padv_connect(ob_conn_padv_param_t *conn_padv_param);

/**@brief Cancel the connection
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_connect_cancel(void);

/// Periodic Advertise parameter
typedef struct ob_padv_param {
    ob_adv_param_t ext_adv_param;    ///< Extern advertising parameters
    uint16_t padv_intv_min;          ///< unit of 1.25ms
    uint16_t padv_intv_max;          ///< unit of 1.25ms
    uint16_t padv_properties;        ///< @ref ob_adv_properties_bits, only bit6 supported
    // parameters of period advertising with response
    uint8_t num_subevents;           ///< max subevent number
    uint8_t subevent_interval;       ///< subevent interval, unit of 1.25ms
    uint8_t response_slot_delay;     ///< Time between the subevent and the first response slot, unit of 1.25ms. 0 meas no response slots
    uint8_t response_slot_spacing;   ///< Time between response slots(0x02~0xFF), unit of 0.125ms. 0 meas no response slots
    uint8_t num_response_slots;      ///< Number of subevent response slots.  0 meas no response slots
} ob_padv_param_t;

/**@brief Enable periodic advertising
 * @param[in]  padv_index     Periodic advertising index
 * @param[in]  padv_param     Periodic advertising parameter. If it is NULL, only extended advertising data or periodic advertising data will be updated.
 * @param[in]  adv_data       Extended advertising data
 * @param[in]  period_data    Periodic advertising data
 * @note If padv_param == NULL, the advertising must be enabled.
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_period_adv_start(uint8_t padv_index, ob_padv_param_t *padv_param, ob_data_t *adv_data,
                                 ob_data_t *period_data);

/**@brief Disable periodic advertising
 * @param[in]  padv_index        Periodic advertising index
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_period_adv_stop(uint8_t padv_index);

typedef struct {
    uint16_t req_event_counter;
    uint8_t req_subevent;
    uint8_t resp_subevent;
    uint8_t resp_slot;
    uint8_t data_len;
    const uint8_t *data;
} ob_gap_padv_resp_data_t;

/**@brief Set PAwR response data
 * @param[in]  sync_idx       Periodic advertising synchronization index
 * @param[in]  resp_data      response data, refer to @ref ob_gap_padv_resp_data_t
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_period_adv_response_data_set(uint8_t sync_idx, const ob_gap_padv_resp_data_t *resp_data);

/// Periodic advertising synchronization parameters
typedef struct {
    const ob_gap_addr_t *addr;  ///< If NULL, use the Periodic Advertiser List, @ref ob_gap_period_adv_list_set
    uint8_t adv_sid;            ///< advertising sid
    uint8_t report_enable;      ///< Whether to report periodic advertising events after successful synchronization
    uint16_t timeout;           ///< Synchronization timeout, unit 10ms
    uint16_t skip;              ///< The maximum number of periodic advertising events that can be skipped
} ob_gap_padv_sync_t;

/**@brief Start periodic advertising synchronization
 * @param[in]  sync_param        Periodic advertising synchronization parameters
 * @note After starting periodic advertising synchronization, scan should be enabled at the same time for synchronization to succeed
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_period_adv_sync(const ob_gap_padv_sync_t *sync_param);

/**@brief Cancel periodic advertising synchronization
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_period_adv_sync_cancel(void);

/**@brief Stop periodic advertising synchronization status
 * @param[in]  sync_idx        Periodic advertising synchronization index
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_period_adv_sync_terminate(uint8_t sync_idx);

/**@brief Set the Periodic Advertiser List
 * @param[in]  fa_addrs          Array of addresses of synchronized devices
 * @param[in]  num               Number of addresses
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_period_adv_list_set(const ob_gap_addr_t *fa_addrs, uint8_t num);

/**@brief  Periodic advertising data reporting switch
 * @param[in]  sync_idx        Periodic advertising synchronization index
 * @param[in]  enable          Enabled status
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_period_adv_report_en(uint8_t sync_idx, uint8_t enable);

enum ob_gap_padv_sync_trans_type {
    OB_GAP_PAST_SYNC_TYPE_SYNC_IDX, ///< Sync transfer using synchronized periodic advertising
    OB_GAP_PAST_SYNC_TYPE_ADV_IDX,  ///< Sync transfer using periodic advertising index value
};
/**@brief  Periodic Advertising Sync Transfer
 * @param[in]  conn_idx        Connection index
 * @param[in]  service_data    A value provided by the user
 * @param[in]  type            PAST mode, refer to @ref ob_gap_padv_sync_trans_type
 * @param[in]  index           The corresponding index value(sync index / adv index)
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_padv_sync_trans(uint8_t conn_idx, uint16_t service_data, uint8_t type, uint8_t index);

enum ob_gap_padv_sync_trans_recv_mode {
    OB_GAP_PAST_RECV_MODE_NO_SYNC,          ///< Not synchronize to periodic advertising
    OB_GAP_PAST_RECV_MODE_SYNC_NO_REPORT,   ///< Synchronize to periodic advertising��Do not report PA data
    OB_GAP_PAST_RECV_MODE_SYNC_REPORT,      ///< Synchronize to periodic advertising��report PA data
};
/**@brief Set the current connection PAST parameters
 * @param[in]  conn_idx        Connection index
 * @param[in]  mode            @ref ob_gap_padv_sync_trans_recv_mode
 * @param[in]  skip            The number of periodic advertising packets that can be skipped
 * @param[in]  timeout         Synchronization timeout, unit 10ms
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_padv_set_sync_trans_param(uint8_t conn_idx, uint8_t mode, uint16_t skip, uint16_t timeout);

/// subevent data structure
typedef struct {
    uint8_t subevent;
    uint8_t response_slot_start;
    uint8_t response_slot_count;
    uint8_t data_len;
    const uint8_t *data;
} ob_gap_padv_subevent_data_t;

/**@brief Synchronize with a subset of the subevents within a PAwR train
 * @param[in]  sync_idx        Periodic advertising synchronization index
 * @param[in]  padv_prop       @ref ob_adv_properties_bits, only Tx Power(bit6) supported
 * @param[in]  subevent_bit    Subevents that need to be synchronized(bit0~bit31 represents subevent index 0~31)
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_period_adv_sync_subevent_set(uint8_t sync_idx, uint16_t padv_prop, uint32_t subevent_bit);

/**@brief Set PAwR subevent data
 * @param[in]  padv_index      Periodic advertising index
 * @param[in]  sub_data        subevent data, refer to @ref ob_gap_padv_subevent_data_t
 * @return result��refer to @ref ob_error
 */
uint32_t ob_gap_period_adv_subevent_data_set(uint8_t padv_index, const ob_gap_padv_subevent_data_t *sub_data);

/**@brief Get device information
 * @param[in]  type        Information Type��refer to @ref ob_dev_info_type
 * @param[in]  conn_idx    If type is OB_DEV_INFO_LOCAL_XXX, this parameter is unused
 * @return result��refer to @ref ob_error
 * @note The device information will be reported via the @ref OB_GAP_EVT_DEVICE_INFO event
 */
uint32_t ob_gap_get_device_info(uint8_t type, uint8_t conn_idx);

/**@brief Generate RPA based on specified IRK
 * @param[in]   irk       IRK for generating RPA
 * @param[out]  rpa       Generated RPA
 */
void ob_gap_rpa_gen(const uint8_t irk[16], uint8_t rpa[6]);

/**@brief Check whether the specified RPA and IRK match
 * @param[in]   irk       IRK corresponding to RPA
 * @param[out]  rpa       RPA that needs to be checked
 * @return Whether the irk and rpa matched
 * @note If the given RPA type is wrong, it also returns false
 */
bool ob_gap_rpa_check(const uint8_t irk[16], const uint8_t rpa[6]);

/**@brief Configure the stack to support secure connection pairing functions for peripheral devices.
 * @warning This function must be called before omble_init to take effect.
 */
void ob_smp_config_per_sc(void);

/**@brief Configure the stack to support pairing functions for central & peripheral devices.
 * @warning This function must be called before omble_init to take effect.
 */
void ob_smp_config_allroles(void);

/**@brief Configure the stack to support secure connection pairing functions for central & peripheral devices.
 * @warning This function must be called before omble_init to take effect.
 */
void ob_smp_config_allroles_sc(void);


#endif /* __OMBLE_GAP_H__ */

/// @}
