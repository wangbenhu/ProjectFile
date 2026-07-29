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
#ifndef __SERVICE_TSPP_H__
#define __SERVICE_TSPP_H__

/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include "omble.h"

/*********************************************************************
 * MACROS
 */
/// TSPP Stream mode, data send in buffer maybe stick together
#define TSPP_STREAM_MODE 1
/// Buffer size, MUST less than 0x8000
#define TSPP_BUFFER_SIZE 512
/// MAX send size
#define TSPP_MAX_MTU_SIZE 244
/// MAX send size
#define TSPP_UPLOAD_TEST 0  

/// type of tspp size
typedef uint16_t tspp_size_t;

/*********************************************************************
 * TYPEDEFS
 */
/// TSPP error code
enum {
    TSPP_ERR_NO_ERROR,
    TSPP_ERR_DISABLED,
    TSPP_ERR_EXCEED_MTU,
    TSPP_ERR_FULL,
};

/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Init TSPP service
 *******************************************************************************
 */
void app_tspp_init(void);

/**
 *******************************************************************************
 * @brief tspp send data
 *
 * @param[in] data       Data to send
 * @param[in] len        Length of data
 *
 * @return length of sent data
 *******************************************************************************
 */
uint8_t tspp_send(uint8_t *data, tspp_size_t len);
/**
 *******************************************************************************
 * @brief tspp clear send buffer
 *******************************************************************************
 */
void tspp_clear(void);
/**
 *******************************************************************************
 * @brief Get tspp buffer size available
 *
 * @return The size of available buffer
 *******************************************************************************
 */
tspp_size_t tspp_avail(void);

/**
 *******************************************************************************
 * @brief Callbck for indicate tspp status
 *
 * @param[in] enabled  is enabled
 *******************************************************************************
 */
extern void app_tspp_status_ind_handler(uint8_t enabled);
/**
 *******************************************************************************
 * @brief Callbck for indicate client write data by WRITE_CMD
 *
 * @param[in] data       Data to send
 * @param[in] len        Length of data
 *******************************************************************************
 */
extern void app_tspp_write_cmd_ind_handler(const uint8_t *data, uint16_t len);

/**
 *******************************************************************************
 * @brief Callbck for indicate data is sent
 *******************************************************************************
 */
extern void app_tspp_data_sent_ind_handler(void);

#endif /* __SERVICE_TSPP_H__ */

/** @} */
