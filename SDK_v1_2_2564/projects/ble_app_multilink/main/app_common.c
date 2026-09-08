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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "omble.h"
#include "om_driver.h"
#include "app_common.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static int conn_num_cnt;

/*********************************************************************
 * EXTERN FUNCTIONS
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
 
void log_debug_array(const uint8_t *data, int len)
{
    char buffer[128 + 1];
    memset(buffer, 0, sizeof(buffer));
    for(int i=0;i < len && i < sizeof(buffer) / 3;i++){
        sprintf(&buffer[i * 3], "%02X ", data[i]);
    }
    log_debug("%s\n", buffer);
}

bool app_get_conn_num_get(void)
{
    if (conn_num_cnt < OB_LE_HOST_CONNECTION_NB) {
        conn_num_cnt++;
        return true;
    } else {
        return false;
    }
}

void app_get_conn_num_put(void)
{
    if (conn_num_cnt > 0) {
        conn_num_cnt--;
    }
}

void app_common_init(void)
{
    conn_num_cnt = 0;
}


/** @} */


