/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @addtogroup OMBLE_MISC
 * @brief MISC
 * @version
 * Version 1.0
 *  - Initial release
 *
 */
/// @{

#ifndef __OMBLE_MISC_H__
#define __OMBLE_MISC_H__
#include <stdint.h>
#include "omble_range.h"
#include "omble_gap.h"

/// omble optional parameter type
enum ob_opt_cfg_type {
    OB_OPT_CFG_TYPE_GAP_MAX_EXT_ADV_EVENT                   = 0,
    OB_OPT_CFG_TYPE_GAP_ADV_SECD_MAX_SKIP                   = 1,
    OB_OPT_CFG_TYPE_GAP_ADV_SCAN_REQ_NTF_EN                 = 2,
    OB_OPT_CFG_TYPE_GAP_CE_LENGTH                           = 3,
    OB_OPT_CFG_TYPE_GAP_PIN_NONE_REQ_ENABLE                 = 4,
    OB_OPT_CFG_TYPE_SMP_PIN_CODE                            = 5,
    OB_OPT_CFG_TYPE_SMP_PRIVATE_KEY                         = 6,
    OB_OPT_CFG_TYPE_SMP_MIN_KEY_SIZE                        = 7,
    OB_OPT_CFG_TYPE_GAP_PADV_DISABLE_EXADV                  = 8,
    OB_OPT_CFG_TYPE_GAP_PHY_OPTIONS                         = 9,
};

/// omble gap_ce_length
struct gap_ce_length {
    uint16_t min; ///< ce length min value
    uint16_t max; ///< ce length max value
};

/// omble optional parameter structure
typedef union {
    uint8_t gap_max_ext_adv_event;      ///< Value of advertise parameter Max_Extended_Advertising_Events
    uint8_t gap_max_adv_secd_max_skip;  ///< Value of advertise parameter Secondary_Advertising_Max_Skip
    uint8_t gap_adv_scan_req_ntf_en;    ///< Value of advertise parameter Scan_Request_Notification_Enable
    struct gap_ce_length gap_ce_length; ///< GAP connection parametar Min_CE_Length/Max_CE_Length
    uint8_t gap_pin_none_req_enable;    ///< Whether to report OB_GAP_EVT_PIN_REQUEST message when pairing in NO_INPUT_NO_OUTPUT mode
    struct {                            ///< Pin code display for pairing, Write Only
        uint8_t conn_idx;
        int pin_code;
    } smp_pin_code;
    uint8_t smp_private_key[32];        ///< Private key, Write Only
    uint8_t smp_min_key_size;           ///< Min key size for Pairing(default:16)
    uint8_t gap_padv_disable_exadv;     ///< Donnot enable extended advertising in period advertising
    uint8_t gap_phy_options;            ///< Host to specify options for PHYs, @ref ob_gap_phy_ops
} ob_opt_cfg_t;


#endif /* __OMBLE_MISC_H__ */

/// @}
