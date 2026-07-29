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
#ifndef __OM_CGMS_H__
#define __OM_CGMS_H__

#include <stdbool.h>
#include "om_cgms_type.h"

/*********************************************************************
 * TYPEDEFS
 */
typedef struct {
    uint8_t device_type;        ///< @ref cgm_type
    uint8_t device_location;    ///< @ref cgm_location
} om_cgms_config_t;

typedef struct {
    float glucose_concentration;
    uint16_t time_offset;
} om_cgms_measurement_t;

///< callback list for cgms process
typedef struct {
    // @brief get session start time
    // @param[out]  time       session start time, @ref om_cgm_time_t
    // @param[out]  time_zone  This field represent the offset from UTC in number of 15-minute increments.
    //                         Valid range from -48 to +56. A value of -128 means that the time zone offset is not known
    // @param[out]  dst_offset Daylight saving time information associated with time
    // @param[in]   p          user data
    void (*get_session_start_time)(om_cgm_time_t *time, int8_t *time_zone, uint8_t *dst_offset, void *p);
    // @brief set session start time
    // @param[in]   time       session start time, @ref om_cgm_time_t
    // @param[in]   time_zone  This field represent the offset from UTC in number of 15-minute increments.
    //                         Valid range from -48 to +56. A value of -128 means that the time zone offset is not known
    // @param[in]   dst_offset Daylight saving time information associated with time
    // @param[in]   p          user data
    void (*set_session_start_time)(const om_cgm_time_t *time, int8_t time_zone, uint8_t dst_offset, void *p);
    // @brief read session run time
    // @param[in]   p          user data
    // @return      Represents the expected run time of the CGM session in hours
    uint16_t (*read_session_run_time)(void *p);
    // @brief request the current status
    // @param[out]  status       CGM Status
    // @param[out]  time_offset  specify the actual relative time difference to the session start time
    // @param[in]   p            user data
    void (*get_status)(om_cgm_state_t *status, uint16_t *time_offset, void *p);
    // @brief Set CGM Communication Interval
    // @param[in]   interval     Communication interval in minutes
    // @param[in]   p            user data
    // @return      true if interval set success
    bool (*set_comm_interval)(uint8_t interval, void *p);
    // @brief Get CGM Communication Interval
    // @param[out]  interval     Communication interval in minutes
    // @param[in]   p            user data
    void (*get_comm_interval)(uint8_t *interval, void *p);
    // @brief Report Number of Stored Records
    // @param[in]   type         records type, @ref cgm_racp_op_value
    // @param[in]   time_offset  time offset, only used if type == CGM_RACP_VALUE_GREATER_EQUAL
    // @param[in]   p            user data
    // @return      The number of records
    uint16_t (*get_records_number)(uint8_t type, uint16_t time_offset, void *p);
    // @brief Report Stored Record
    // @param[in]   index        The index value of the requested record, which increases from 0,
    //                           used to distinguish record entries when obtaining multiple records.
    // @param[in]   type         records type, @ref cgm_racp_op_value
    // @param[in]   time_offset  time offset, only used if type == CGM_RACP_VALUE_GREATER_EQUAL
    // @param[out]  data         measurement data
    // @param[out]  status       CGM Status
    // @param[in]   p            user data
    // @return      true if data & state valid
    bool (*get_records)(int index, uint8_t type, uint16_t time_offset,
                        om_cgms_measurement_t *data, om_cgm_state_t *state, void *p);
} om_cgms_handlers_t;

/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Init CGMS(Continuous Glucose Monitoring Service) module
 * @param[in] config       module config, @ref om_cgms_config_t
 * @param[in] cbs          callback for cgms process, @ref om_cgms_handlers_t
 * @param[in] user_data    user data
 *cbs
 *******************************************************************************
 */
void om_cgms_init(const om_cgms_config_t *config, const om_cgms_handlers_t *cbs, void *user_data);


// The following functions are provided for CGMS GATT service calls
/** CGMS connected */
void om_cgms_connected_adapt(void);
/** CGMS feature uuid(0x2AA8) read */
uint8_t om_cgms_read_feature_adapt(uint8_t resp_data[6]);
/** CGMS session start time uuid(0x2AAA) read */
uint8_t om_cgms_read_session_start_time_adapt(uint8_t resp[11], uint8_t *crc_valid);
/** CGMS session run time uuid(0x2AAB) read */
uint8_t om_cgms_read_session_run_time_adapt(uint8_t resp[4], uint8_t *crc_valid);
/** CGMS status uuid(0x2AA9) read */
uint8_t om_cgms_read_state_adapt(uint8_t resp[7], uint8_t *crc_valid);
/** CGMS session start time uuid(0x2AAA) write */
uint8_t om_cgms_write_session_start_time_adapt(const uint8_t *data, int len);
/** CGMS RACP(record access point uuid(0x2A52)) write */
void om_cgms_write_racp_adapt(const uint8_t *data, int len);
/** CGMS ops control uuid(0x2AAC) write */
void om_cgms_write_ops_ctrl_adapt(const uint8_t *data, int len);
/** CGMS Indication confirmed */
void om_cgms_measurement_send_done_adapt(void);

#endif /* __OM_CGMS_H__ */

/** @} */
