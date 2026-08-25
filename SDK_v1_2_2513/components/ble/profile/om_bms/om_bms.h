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
#ifndef __OM_BMS_H__
#define __OM_BMS_H__

/*******************************************************************************
 * INCLUDES
 */
#include <stdbool.h>
#include <stdint.h>

enum om_mbs_error {
    BMS_ERR_NO_ERR              = 0x00,
    BMS_ERR_OPCODE_NOT_SUPPORT  = 0x80,
    BMS_ERR_OPER_FAILED         = 0x81,
};

/// Bond Management Op Codes and Operands
enum bms_opcode {
    BMS_OPCODE_RESERVED,
    /// Delete bond of requesting device (BR/EDR and LE)
    BMS_OPCODE_BR_EDR_LE_DEL_REQ_DEV,
    /// Delete bond of requesting device (BR/EDR transport only)
    BMS_OPCODE_BR_EDR_DEL_REQ_DEV,
    /// Delete bond of requesting device (LE transport only)
    BMS_OPCODE_LE_DEL_REQ_DEV,
    /// Delete all bonds on server (BR/EDR and LE)
    BMS_OPCODE_BR_EDR_LE_DEL_ALL,
    /// Delete all bonds on server (BR/EDR transport only)
    BMS_OPCODE_BR_EDR_DEL_ALL,
    /// Delete all bonds on server (LE transport only)
    BMS_OPCODE_LE_DEL_ALL,
    /// Delete all but the active bond on server (BR/EDR and LE)
    BMS_OPCODE_BR_EDR_LE_DEL_EXP_REQ_DEV,
    /// Delete all but the active bond on server (BR/EDR transport only)
    BMS_OPCODE_BR_EDR_DEL_EXP_REQ_DEV,
    /// Delete all but the active bond on server (LE transport only)
    BMS_OPCODE_LE_DEL_EXP_REQ_DEV,
};

/// Bond Management Service Feature Bit
enum bms_feature_bit {
    /// Delete bond of requesting device (BR/EDR and LE)
    BMS_FEATURE_BIT_BR_EDR_LE_DEL_REQ_DEV           = 0x00000001,
    /// Delete bond of requesting device (BR/EDR and LE) with authorization code
    BMS_FEATURE_BIT_BR_EDR_LE_DEL_REQ_DEV_AUTH      = 0x00000002,
    /// Delete bond of requesting device (BR/EDR transport only)
    BMS_FEATURE_BIT_BR_EDR_DEL_REQ_DEV              = 0x00000004,
    /// Delete bond of requesting device (BR/EDR transport only) with authorization code
    BMS_FEATURE_BIT_BR_EDR_DEL_REQ_DEV_AUTH         = 0x00000008,
    /// Delete bond of requesting device (LE transport only)
    BMS_FEATURE_BIT_LE_DEL_REQ_DEV                  = 0x00000010,
    /// Delete bond of requesting device (LE transport only) with authorization code
    BMS_FEATURE_BIT_LE_DEL_REQ_DEV_AUTH             = 0x00000020,
    /// Delete all bonds on server (BR/EDR and LE)
    BMS_FEATURE_BIT_BR_EDR_LE_DEL_ALL               = 0x00000040,
    /// Delete all bonds on server (BR/EDR and LE) with authorization code
    BMS_FEATURE_BIT_BR_EDR_LE_DEL_ALL_AUTH          = 0x00000080,
    /// Delete all bonds on server (BR/EDR transport only)
    BMS_FEATURE_BIT_BR_EDR_DEL_ALL                  = 0x00000100,
    /// Delete all bonds on server (BR/EDR transport only) with authorization code
    BMS_FEATURE_BIT_BR_EDR_DEL_ALL_AUTH             = 0x00000200,
    /// Delete all bonds on server (LE transport only)
    BMS_FEATURE_BIT_LE_DEL_ALL                      = 0x00000400,
    /// Delete all bonds on server (LE transport only) with authorization code
    BMS_FEATURE_BIT_LE_DEL_ALL_AUTH                 = 0x00000800,
    /// Delete bond of all except the requesting device on the server (BR/EDR and LE)
    BMS_FEATURE_BIT_BR_EDR_LE_DEL_EXP_REQ_DEV       = 0x00001000,
    /// Delete bond of all except the requesting device on the server (BR/EDR and LE) with authorization code
    BMS_FEATURE_BIT_BR_EDR_LE_DEL_EXP_REQ_DEV_AUTH  = 0x00002000,
    /// Delete bond of all except the requesting device on the server (BR/EDR transport only)
    BMS_FEATURE_BIT_BR_EDR_DEL_EXP_REQ_DEV          = 0x00004000,
    /// Delete bond of all except the requesting device on the server (BR/EDR transport only) with authorization code
    BMS_FEATURE_BIT_BR_EDR_DEL_EXP_REQ_DEV_AUTH     = 0x00008000,
    /// Delete bond of all except the requesting device on the server (LE transport only)
    BMS_FEATURE_BIT_LE_DEL_EXP_REQ_DEV              = 0x00010000,
    /// Delete bond of all except the requesting device on the server (LE transport only) with authorization code
    BMS_FEATURE_BIT_LE_DEL_EXP_REQ_DEV_AUTH         = 0x00020000,
};

/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief callback type for bms process
 * @param[in] opcode        opcode, @ref enum bms_opcode;
 * @param[in] auth_code     Authorization code, NULL for non-auth operation
 * @param[in] auth_code_len Length of authorization code, 0 for non-auth operation
 * @return    operation status, @ref enum om_mbs_error
 *******************************************************************************
 */
typedef uint32_t (*bms_control_handler)(uint8_t opcode, const uint8_t *auth_code, int auth_code_len, void *user_data);

/**
 *******************************************************************************
 * @brief Init BMS(Bond Management Service) module
 * @param[in] feature_bits module feature supported, @ref enum bms_feature_bit
 * @param[in] callback     callback for bms process, @ref bms_control_handler
 * @param[in] user_data    user data
 *******************************************************************************
 */
void om_bms_init(uint32_t feature_bits, bms_control_handler callback, void *user_data);

// The following functions are provided for BMS GATT service calls
/** BMS ops control uuid(0x2AA4) write */
uint32_t om_bms_write_ops_ctrl_adapt(const uint8_t *data, int len); // return status, @ref enum om_mbs_error
//
/** BMS feature uuid(0x2AA5) read */
void om_bms_read_feature_adapt(uint8_t resp_data[3]);

#endif /* __OM_BMS_H__ */

/** @} */
