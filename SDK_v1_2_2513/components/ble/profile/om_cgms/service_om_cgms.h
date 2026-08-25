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
#ifndef __SERVICE_OM_CGMS_H__
#define __SERVICE_OM_CGMS_H__

/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>

/*******************************************************************************
 * EXTERN FUNCTIONS
 */
void service_om_cgms_init(void);

// The following functions are provided for CGMS module calls
void service_om_cgms_write_resp(uint8_t gatt_state);
void service_om_cgms_racp_indicate(const uint8_t *data, int len);
void service_om_cgms_ops_ctrl_indicate(const uint8_t *data, int len);
void service_om_cgms_measurement_notify(const uint8_t *data, int len);

#endif /* __SERVICE_OM_CGMS_H__ */

/** @} */
