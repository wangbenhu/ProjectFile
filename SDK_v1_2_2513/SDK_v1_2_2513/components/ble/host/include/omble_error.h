/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @addtogroup OMBLE_ERROR
 * @brief Error Code
 * @version
 * Version 1.0
 *  - Initial release
 *
 */
/// @{

#ifndef __OMBLE_ERR_H__
#define __OMBLE_ERR_H__

/// Error Definition
typedef enum ob_error {
    OB_ERROR_NO_ERR                  = 0x00, ///< Execution Success
    OB_ERROR_NO_CONNECTION,                  ///< No connection errors
    OB_ERROR_INVALID_STATE,                  ///< Status Error
    OB_ERROR_INSUFFICIENT_RESOURCES,         ///< Insufficient resources error
    OB_ERROR_NOT_SUPPORT,                    ///< The operation or request is not supported
    OB_ERROR_INVALID_PARAM,                  ///< Parameter error
} ob_error_t;

/// GATT Error Definition
typedef enum ob_gatt_error {
    OB_GATT_ERR_NO_ERROR                       = 0x0000,
    OB_GATT_ERR_INVALID_HANDLE                 = 0x0401,
    OB_GATT_ERR_READ_NOT_PERMITTED             = 0x0402,
    OB_GATT_ERR_WRITE_NOT_PERMITTED            = 0x0403,
    OB_GATT_ERR_INVALID_PDU                    = 0x0404,
    OB_GATT_ERR_INSUFFICIENT_AUTHENTICATION    = 0x0405,
    OB_GATT_ERR_REQUEST_NOT_SUPPORTED          = 0x0406,
    OB_GATT_ERR_INVALID_OFFSET                 = 0x0407,
    OB_GATT_ERR_INSUFFICIENT_AUTHORIZATION     = 0x0408,
    OB_GATT_ERR_PREPARE_QUEUE_FULL             = 0x0409,
    OB_GATT_ERR_ATTRIBUTE_NOT_FOUND            = 0x040A,
    OB_GATT_ERR_ATTRIBUTE_NOT_LONG             = 0x040B,
    OB_GATT_ERR_ENCRYPTION_KEY_SIZE_TOO_SHORT  = 0x040C,
    OB_GATT_ERR_INVALID_ATTRIBUTE_VALUE_LENGTH = 0x040D,
    OB_GATT_ERR_UNLIKELY_ERROR                 = 0x040E,
    OB_GATT_ERR_INSUFFICIENT_ENCRYPTION        = 0x040F,
    OB_GATT_ERR_UNSUPPORTED_GROUP_TYPE         = 0x0410,
    OB_GATT_ERR_INSUFFICIENT_RESOURCES         = 0x0411,
    OB_GATT_ERR_DATABASE_OUT_OF_SYNC           = 0x0412,
    OB_GATT_ERR_VALUE_NOT_ALLOWED              = 0x0413,
} ob_gatt_error_t;

/// SMP Error Definition
typedef enum ob_smp_error {
    OB_SMP_ERR_NO_ERR                                 = 0x0000,
    OB_SMP_LOCAL_ERR_PASSKEY_ENTRY_FAILED             = 0x0601,
    OB_SMP_LOCAL_ERR_OOB_NOT_AVAILABLE                = 0x0602,
    OB_SMP_LOCAL_ERR_AUTHENTICATION_REQUIREMENTS      = 0x0603,
    OB_SMP_LOCAL_ERR_CONFIRM_VALUE_FAILED             = 0x0604,
    OB_SMP_LOCAL_ERR_PAIRING_NOT_SUPPORTED            = 0x0605,
    OB_SMP_LOCAL_ERR_ENCRYPTION_KEY_SIZE              = 0x0606,
    OB_SMP_LOCAL_ERR_COMMAND_NOT_SUPPORTED            = 0x0607,
    OB_SMP_LOCAL_ERR_UNSPECIFIED_REASON               = 0x0608,
    OB_SMP_LOCAL_ERR_REPEATED_ATTEMPTS                = 0x0609,
    OB_SMP_LOCAL_ERR_INVALID_PARAMETERS               = 0x060A,
    OB_SMP_LOCAL_ERR_DHKEY_CHECK_FAILED               = 0x060B,
    OB_SMP_LOCAL_ERR_NUMERIC_COMPARISON_FAILED        = 0x060C,
    OB_SMP_LOCAL_ERR_BR_EDR_PAIRING_IN_PROGRESS       = 0x060D,
    OB_SMP_LOCAL_ERR_CROSS_TRANSPORT_KEY_NOT_ALLOWED  = 0x060E,
    OB_SMP_LOCAL_ERR_KEY_REJECTED                     = 0x060F,
    OB_SMP_REMOTE_ERR_PASSKEY_ENTRY_FAILED            = 0x0701,
    OB_SMP_REMOTE_ERR_OOB_NOT_AVAILABLE               = 0x0702,
    OB_SMP_REMOTE_ERR_AUTHENTICATION_REQUIREMENTS     = 0x0703,
    OB_SMP_REMOTE_ERR_CONFIRM_VALUE_FAILED            = 0x0704,
    OB_SMP_REMOTE_ERR_PAIRING_NOT_SUPPORTED           = 0x0705,
    OB_SMP_REMOTE_ERR_ENCRYPTION_KEY_SIZE             = 0x0706,
    OB_SMP_REMOTE_ERR_COMMAND_NOT_SUPPORTED           = 0x0707,
    OB_SMP_REMOTE_ERR_UNSPECIFIED_REASON              = 0x0708,
    OB_SMP_REMOTE_ERR_REPEATED_ATTEMPTS               = 0x0709,
    OB_SMP_REMOTE_ERR_INVALID_PARAMETERS              = 0x070A,
    OB_SMP_REMOTE_ERR_DHKEY_CHECK_FAILED              = 0x070B,
    OB_SMP_REMOTE_ERR_NUMERIC_COMPARISON_FAILED       = 0x070C,
    OB_SMP_REMOTE_ERR_BR_EDR_PAIRING_IN_PROGRESS      = 0x070D,
    OB_SMP_REMOTE_ERR_CROSS_TRANSPORT_KEY_NOT_ALLOWED = 0x070E,
    OB_SMP_REMOTE_ERR_KEY_REJECTED                    = 0x070F,
} ob_smp_error_t;

#endif /* __OMBLE_ERR_H__ */

/// @}
