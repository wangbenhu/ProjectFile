/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @file     obc_cc.h
 * @brief    obc_cc
 * @date     15 December 2021
 * @author   OnMicro SW Team
 *
 * @defgroup OBC_CC OBC_CC
 * @ingroup  OBC
 * @brief    Onmicro BLE controller config
 * @details  Onmicro BLE controller config

 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __SC_H__
#define __SC_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include <stdbool.h>

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * TYPEDEFS
 */

/// stack config
typedef struct
{
    /// LP clock (32k) drift (ppm)
    uint16_t    lpclk_drift;
    /// Wakeup time
    uint8_t     pre_wakeup_time;
    /// Prog delay
    uint8_t     llt_prog_delay;
    /// Min prog delay
    uint8_t     llt_min_prog_delay;
    /// Min sleep space
    uint8_t     min_sleep_space;
    /// Use fixed p256key
    bool        dbg_fixed_p256_key;
    /// Channel selection algorithm #2
    bool        chsel2;
    /// Connection move enable
    bool        con_move_en;
    /// Enable channel assessment usage when building channel maps
    bool        ch_ass_en;
    /// Extended scanning
    bool        ext_scan;
    /// Default Scan event duration (in 31.25us)
    uint16_t    scan_evt_dur_dft;
    /// Company ID
    uint16_t    company_id;
    /// BT version
    uint8_t     version;
    /// Coded phy is 500k or 125k
    bool        coded_phy_500k;
    // CONNECT_IND rssi threshold
    int8_t      connect_ind_rssi_thr;
    /// Advertising accept all connect request
    bool        adv_accept_all_conn_req;
    /// Advertising Access Address
    uint32_t    adv_aa;
}obc_cc_t;

/*********************************************************************
 * EXTERN VARIABLES
 */
extern obc_cc_t obcc;

/*********************************************************************
 * EXTERN FUNCTIONS
 */
__STATIC_INLINE void obc_company_id_set(uint16_t company_id)
{
    obcc.company_id = company_id;
}

__STATIC_INLINE void obc_version_set(uint8_t version)
{
    obcc.version = version;
}

__STATIC_INLINE void obc_conn_rssi_set(int8_t conn_rssi)
{
    obcc.connect_ind_rssi_thr = conn_rssi;
}

__STATIC_INLINE void obc_adv_accept_all_conn_req_set(bool accept)
{
    obcc.adv_accept_all_conn_req = accept;
}

__STATIC_INLINE void obc_adv_aa_set(uint32_t adv_aa)
{
    obcc.adv_aa = adv_aa;
}

#ifdef __cplusplus
}
#endif

#endif

/** @} */

