/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @file     obc_pta.h
 * @brief    PTA
 * @date     01. April 2020
 * @author   OnMicro SW Team
 *
 * @defgroup OBC_PTA PTA
 * @ingroup  OBC
 * @brief    PTA
 * @details  PTA
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __BB_PTA_H
#define __BB_PTA_H


/*******************************************************************************
 * INCLUDES
 */

#ifdef  __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */


/*******************************************************************************
 * TYPEDEFS
 */

typedef struct {
    uint8_t grant_active_level;
    uint8_t priority_txrx_include;
    uint8_t priority_txrx_tx_active_level;
    uint8_t priority_prio_active_keep_us;
    uint8_t priority_txrx_active_keep_us;
    uint8_t priority_threshold;

    // priority of eche ble state
    uint8_t priority_state_initiating_connection_ind_rsp;
    uint8_t priority_state_connection_llcp;
    uint8_t priority_state_connection_data_channel;
    uint8_t priority_state_initiating_scanning;
    uint8_t priority_state_active_scanning;
    uint8_t priority_state_connectable_advertising;
    uint8_t priority_state_non_connectable_advertising;
    uint8_t priority_state_passive_scanning;
} obc_pta_ctrl_t;

/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief  bb pta enable
 *
 * @detail
 *
 * SiliconLab PTA:
 *
 * 4line:
 *   grand      蓝牙端输入，WLAN TX工作时请求蓝牙关闭
 *   request    蓝牙端输出，同蓝牙的TXEN/RXEN
 *   priority   蓝牙端输出，蓝牙priority_state_xxx大于priority_threshold时产生高电平。request后延时priority_prio_active_keep_us之后输出是TX还是RX
 *   freq       蓝牙端输出，当蓝牙频率出现2412/2417/2422/2427/2432/2437/2442/2447/2452/2457/2462/2467/2472/2484MHz时输出高
 *
 *
 * @param[in] enable  enable
 * @param[in] ctrl  ctrl
 *******************************************************************************
 */
void obc_pta_enable(bool enable, const obc_pta_ctrl_t *ctrl);


#ifdef  __cplusplus
}
#endif

#endif  /* __EVT_H */

/** @} */

