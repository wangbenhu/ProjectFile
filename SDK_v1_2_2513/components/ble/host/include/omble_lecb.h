/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @addtogroup OMBLE_LECB
 * @brief LECB
 * @version
 * Version 1.0
 *  - Initial release
 *
 */
/// @{

#ifndef __OMBLE_LECB_H__
#define __OMBLE_LECB_H__

#include <stdint.h>

typedef struct {
    /**@brief lecb channel connection established
     * @param[in]  conn_idx          BLE connection index
     * @param[in]  lecb_idx          lecb connection index
     */
    void (*connected)(uint8_t conn_idx, uint8_t lecb_idx);
    /**@brief lecb channel disconnected
     * @param[in]  conn_idx          BLE connection index
     * @param[in]  lecb_idx          lecb connection index
     */
    void (*disconnected)(uint8_t conn_idx, uint8_t lecb_idx);
    /**@brief lecb channel peer device credit update
     * @param[in]  conn_idx          BLE connection index
     * @param[in]  lecb_idx          lecb connection index
     * @param[in]  peer_credit       The credit after the peer updates
     */
    void (*credit_updated)(uint8_t conn_idx, uint8_t lecb_idx, uint16_t peer_credit);
    /**@brief lecb channel peer device credit update
     * @param[in]  conn_idx          BLE connection index
     * @param[in]  lecb_idx          lecb connection index
     * @param[in]  local_credit      Local credit (the number of packets number that can be sent by the peer)
     * @param[in]  data              data
     * @param[in]  len               length of data
     */
    void (*pdu_recv)(uint8_t conn_idx, uint8_t lecb_idx, uint16_t local_credit, const uint8_t *data, int len);
    /**@brief lecb channel connection request
     * @param[in]  conn_idx          BLE connection index
     * @param[out] local_credit      lecb connection index
     * @param[out] local_mtu         local MTU
     * @param[out] local_msp         local MSP
     * @return     response of connection
     *                  0x0000: Connection successful.
     *                  0x0001: Connection pending
     *                  0x0002: Connection refused – PSM not supported.
     *                  0x0003: Connection refused – security block.
     *                  0x0004: Connection refused – no resources available.
     *                  0x0006: Connection refused – invalid Source CID
     *                  0x0007: Connection refused – Source CID already allocated
     * @note       If the callback function is not defined, the lecb channel will be automatically connected and
	 *             the connection parameters will use the default parameters.
     * @note       If the callback function is defined but the parameters are not set, the default parameters are used.
     */
    int (*connect_req)(uint8_t conn_idx, uint16_t *local_credit, uint16_t *local_mtu, uint16_t *local_mps);
} ob_lecb_callbacks_t;

typedef struct {
    uint16_t init_credit; ///< Initial number of credit
    const ob_lecb_callbacks_t *callbacks; ///< Callback function structure, @warning The variable pointed to by the callbacks parameter must be a global or static variable
} ob_lecb_reg_param_t;

/**@brief Register LE Credit Based Flow Control callback
 * @param[in]  param             Initialization parameters
 * @return result, refer to @ref ob_error
 */
uint32_t ob_lecb_register(const ob_lecb_reg_param_t *param);

/**@brief Send data through lecb channel
 * @param[in]  conn_idx          BLE connection index
 * @param[in]  lecb_idx          lecb connection index
 * @param[in]  data              data
 * @param[in]  len               length of data
 * @return result, refer to @ref ob_error
 */
uint32_t ob_lecb_send(uint8_t conn_idx, uint8_t lecb_idx, const uint8_t *data, int len);

/**@brief Increase the number of credits
 * @param[in]  conn_idx          BLE connection index
 * @param[in]  lecb_idx          lecb connection index
 * @param[in]  credit_num        number
 * @return result, refer to @ref ob_error
 */
uint32_t ob_lecb_credit_add(uint8_t conn_idx, uint8_t lecb_idx, uint16_t credit_num);

/**@brief Disconnecting the channel of LECB
 * @param[in]  conn_idx          BLE connection index
 * @param[in]  lecb_idx          lecb connection index
 * @return result, refer to @ref ob_error
 */
uint32_t ob_lecb_disconnect(uint8_t conn_idx, uint8_t lecb_idx);

typedef struct {
    uint16_t local_cid;
    uint16_t local_credit;
    uint16_t local_mtu;
    uint16_t local_mps;
    uint16_t peer_cid;
    uint16_t peer_credit;
    uint16_t peer_mtu;
    uint16_t peer_mps;
    uint16_t spsm;
} ob_lecb_conn_info_t;

/**@brief Get lecb channel information
 * @param[in]  conn_idx          BLE connection index
 * @param[in]  lecb_idx          lecb connection index
 * @param[out] info              Channel information
 * @return result, refer to @ref ob_error
 */
uint32_t ob_lecb_conn_info_get(uint8_t conn_idx, uint8_t lecb_idx, ob_lecb_conn_info_t *info);

#endif /* __OMBLE_LECB_H__ */

/// @}
