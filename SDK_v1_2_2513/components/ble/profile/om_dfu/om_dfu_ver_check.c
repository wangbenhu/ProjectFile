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
 * @brief    fault handle
 * @details  fault handle
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


/*******************************************************************************
 * INCLUDES
 */
#include "autoconf.h"
#include <stdint.h>
#include <stddef.h>
#include "om_driver.h"
#include "om_log.h"


/*******************************************************************************
 * MACROS
 */
#define SECURE_BIT_IN_EFUSE_POS      496U

#define CFG_MAGIC_NUM                (('C' << 0) + ('F' << 8) + ('G' << 16))
#define MBR_MAGIC_NUM                (('M' << 0) + ('B' << 8) + ('R' << 16))

#define CUR_IMG_HDR_ADDR             0x00402000U


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct __PACKED {
    uint8_t  rsvd0[0x100];
    uint32_t mbr_magic_num;
    uint32_t len;
    uint32_t img_ver;
} img_hdr_t;


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
om_error_t upgrade_check(uint32_t upgrade_img_ver)
{
    uint8_t secure_is_en;
    uint32_t cur_img_ver;

    drv_efuse_init();

    secure_is_en = OM_EFUSE->READ_DATA[SECURE_BIT_IN_EFUSE_POS / 8] & (1 << (SECURE_BIT_IN_EFUSE_POS % 8)) ? 1U : 0U;
    cur_img_ver  = ((img_hdr_t *)CUR_IMG_HDR_ADDR)->img_ver;

    if (secure_is_en) {
        if (upgrade_img_ver <= cur_img_ver) {
            return OM_ERROR_VERIFY;  // version error
        }
    }

    return OM_ERROR_OK;
}
