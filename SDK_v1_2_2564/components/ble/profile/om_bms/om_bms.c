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

/*******************************************************************************
 * INCLUDES
 */
#include <string.h>
#include <math.h>
#include "om_bms.h"

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint32_t bms_feature_support;
static bms_control_handler control_handler;
static void *om_bms_user_data;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static uint8_t is_opcode_support(uint8_t opcode, uint8_t auth, uint32_t feature)
{
    return (opcode >= BMS_OPCODE_BR_EDR_LE_DEL_REQ_DEV && opcode <= BMS_OPCODE_LE_DEL_EXP_REQ_DEV) &&
           (((1 << (opcode - 1) * 2) & feature) || (auth && ((1 << (opcode * 2 - 1)) & feature)));
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
uint32_t om_bms_write_ops_ctrl_adapt(const uint8_t *data, int len)
{
    if (len > 0 && control_handler) {
        uint8_t opcode = *data;
        uint8_t auth_len = len - 1;
        const uint8_t *auth = auth_len == 0 ? NULL : &data[1];
        if (is_opcode_support(opcode, auth != NULL, bms_feature_support)) {
            return control_handler(opcode, auth, auth_len, om_bms_user_data);
        }
    }
    return BMS_ERR_OPCODE_NOT_SUPPORT;
}

void om_bms_read_feature_adapt(uint8_t resp_data[3])
{
    memcpy(resp_data, &bms_feature_support, 3);
}

void om_bms_init(uint32_t feature_bits, bms_control_handler callback, void *user_data)
{
    bms_feature_support = feature_bits;
    control_handler = callback;
    om_bms_user_data = user_data;
}

/** @} */
