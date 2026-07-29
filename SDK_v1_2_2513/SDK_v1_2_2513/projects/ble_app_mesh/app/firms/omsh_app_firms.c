/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup DOC DOC
 * @ingroup  DOCUMENT
 * @brief    Mesh app of firms source file
 * @details  Mesh app of firms source file
 * @version
 *
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

/*******************************************************************************
 * INCLUDES
 */
#include "omsh_app.h"


/*******************************************************************************
 * STATIC VARIABLE
 */
static msh_api_node_id_t node_info = {
    .uuid           = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff},
    .company_id     = 0x01BF,
    .product_id     = 0x0001,
    .version_id     = 0x0001
};

static uint8_t bt_addr[6] = {0xEE, 0xEE, 0x66, 0x27, 0xEE, 0xEE};


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
void msh_app_firms_init(void)
{
    // Set local bluetooth address
    ob_gap_addr_set(OB_ADV_ADDR_TYPE_PUBLIC, &bt_addr[0]);

    // Set mesh node identify
    msh_api_set_uuid(node_info.uuid);
    msh_api_set_cid(node_info.company_id);
    msh_api_set_pid(node_info.product_id);
    msh_api_set_vid(node_info.version_id);
}

void msh_app_firms_start(uint8_t prov_state)
{
    // Check proved
    if(true == prov_state) {

    } else {
        /* TODO: Test API of omesh_auto_prov_handle
        msh_api_auto_prov_t auto_prov = {
            .dev_key        = {0x37,0xB5,0xAD,0x9D,0xD7,0xBD,0x7C,0xA1,0xA1,0xAB,0x32,0x4C,0xED,0x0C,0xB8,0x0C},
            .net_key        = {0xC6,0x98,0x82,0x7C,0x34,0x23,0x79,0xCE,0xBE,0x10,0x1D,0x57,0x2B,0xBB,0x14,0x86},
            .net_key_id     = 0,
            .flags          = 0,
            .iv             = 0x0a,
            .unicast_addr   = 0x02
        };

        msh_api_auto_prov_handle(&auto_prov);
        */
    }
}

/** @} */