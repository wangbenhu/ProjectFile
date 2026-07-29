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

#ifndef __ANCS_CLIENT_H__
#define __ANCS_CLIENT_H__

/*********************************************************************
 * MACROS
 */
#define log_debug(...)

#ifndef __WEAK
#define __WEAK __attribute__((weak))
#endif

#include <stdio.h>
#include <stdint.h>
#include "ancs_common.h"
/// Max Size of APP_ID
#define ANCS_APPID_CAP     32
/// Max Size of message Title
#define ANCS_TITLE_CAP     32
/// Max Size of message subtitle
#define ANCS_SUBTITLE_CAP  4
/// Max Size of date
#define ANCS_DATE_CAP      16
/// Max Size of message
#define ANCS_MSG_CAP       100

/*********************************************************************
 * TYPEDEFS
 */
/// ancs_notify_t
typedef struct {
    /// @ref ancs_eventID_values
    uint8_t evt_id;
    /// @ref ancs_event_flags
    uint8_t evt_flags;
    /// @ref ancs_categoryID_values
    uint8_t cate_id;
    /// category count
    uint8_t cate_count;
    /// message UID
    uint32_t notify_uid;
} ancs_notify_t;

/// ancs_data_t
typedef struct {
    /// Notify UID
    uint32_t notify_uid;
    /// Received length of date
    uint8_t date_len;
    /// Date in string format
    uint8_t date[ANCS_DATE_CAP];
    /// Received length of app_id
    uint8_t app_id_len;
    /// App id
    uint8_t app_id[ANCS_APPID_CAP];
    /// Recevied length of title
    uint8_t title_len;
    /// Message Title
    uint8_t title[ANCS_TITLE_CAP];
    /// Recevied length of subtitle
    uint8_t subtitle_len;
    /// Message Subtitle
    uint8_t subtitle[ANCS_SUBTITLE_CAP];
    /// Received length of message
    uint8_t msg_len;
    /// Message detail
    uint8_t msg[ANCS_MSG_CAP];
} ancs_data_t;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Init ancs client
 *******************************************************************************
 */
void ancs_client_init(void);
/**
 *******************************************************************************
 * @brief Get message detail data by UID
 * @param[in] uid        UID
 *******************************************************************************
 */
void ancs_get_data(uint32_t uid);
/**
 *******************************************************************************
 * @brief Proform action by UID & action
 * @param[in] uid       UID
 * @param[in] act       action
 *******************************************************************************
 */
void ancs_perform_act(uint32_t uid, uint8_t act);
/**
 *******************************************************************************
 * @brief Callback for process notify
 * @param[in] notify       ancs notification
 *******************************************************************************
 */
extern void ancs_notify_evt(ancs_notify_t *notify);
/**
 *******************************************************************************
 * @brief Callback for process message detail data
 * @param[in] data       ancs data
 *******************************************************************************
 */
extern void ancs_data_evt(ancs_data_t *data);

#endif /* __ANCS_CLIENT_H__ */

/** @} */
