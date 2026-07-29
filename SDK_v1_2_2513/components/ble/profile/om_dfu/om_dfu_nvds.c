/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @version
 * Version V20210119.1.0
 *  - Initial release
 *
 * @{
 */

/*******************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "om_driver.h"
#include "om_dfu.h"
#include "om_dfu_nvds.h"

/*********************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static uint8_t om_dfu_nvds_enable_dummy(void)
{
    return OM_DFU_NVDS_ST_SUCCESS;
}
static uint8_t om_dfu_nvds_get_dummy(uint32_t id, uint32_t *lengthPtr, void *buf)
{
    memset(buf, 0xFF, *lengthPtr);
    return OM_DFU_NVDS_ST_SUCCESS;
}
static uint8_t om_dfu_nvds_put_dummy(uint32_t id, uint32_t length, void *buf)
{
    return OM_DFU_NVDS_ST_SUCCESS;
}
static uint8_t om_dfu_nvds_del_dummy(uint32_t id, uint32_t length)
{
    return OM_DFU_NVDS_ST_SUCCESS;
}
static uint8_t om_dfu_nvds_disable_dummy(void)
{
    return OM_DFU_NVDS_ST_SUCCESS;
}

static uint8_t om_dfu_nvds_enable_mbr(void)
{
    drv_flash_write_protect_set(OM_FLASH0, FLASH_PROTECT_NONE);
    return OM_DFU_NVDS_ST_SUCCESS;
}
static uint8_t om_dfu_nvds_get_mbr(uint32_t id, uint32_t *lengthPtr, void *buf)
{
    *lengthPtr = sizeof(dfu_image_mbr_info);
    memset(buf, 0, sizeof(dfu_image_mbr_info));
    return OM_DFU_NVDS_ST_SUCCESS;
}
static uint8_t om_dfu_nvds_put_mbr(uint32_t id, uint32_t length, void *buf)
{
    return OM_DFU_NVDS_ST_SUCCESS;
}
static uint8_t om_dfu_nvds_del_mbr(uint32_t id, uint32_t length)
{
    return OM_DFU_NVDS_ST_FAILED;
}
static uint8_t om_dfu_nvds_disable_mbr(void)
{
    return OM_DFU_NVDS_ST_SUCCESS;
}

static uint8_t om_dfu_nvds_enable_flash(void)
{
    drv_flash_write_protect_set(OM_FLASH0, FLASH_PROTECT_NONE);
    return OM_DFU_NVDS_ST_SUCCESS;
}
static uint8_t om_dfu_nvds_get_flash(uint32_t id, uint32_t *lengthPtr, void *buf)
{
    om_error_t res = drv_flash_read(OM_FLASH0, id, buf, *lengthPtr);
    if (res == OM_ERROR_OK) {
        return OM_DFU_NVDS_ST_SUCCESS;
    } else {
        return OM_DFU_NVDS_ST_FAILED;
    }
}
static uint8_t om_dfu_nvds_put_flash(uint32_t id, uint32_t length, void *buf)
{
    om_error_t res = drv_flash_write(OM_FLASH0, id, buf, length);
    if (res == OM_ERROR_OK) {
        return OM_DFU_NVDS_ST_SUCCESS;
    } else {
        return OM_DFU_NVDS_ST_FAILED;
    }
}
static uint8_t om_dfu_nvds_del_flash(uint32_t id, uint32_t length)
{
    uint32_t st = id & ~0x0FFFUL, en = (id + length + 0xFFF) & ~0x0FFFUL;
    for (int i = st; i < en; i += 0x1000) {
        om_error_t res = drv_flash_erase(OM_FLASH0, i, FLASH_ERASE_4K);
        if (res != OM_ERROR_OK) {
            return OM_DFU_NVDS_ST_FAILED;
        }
    }
    return OM_DFU_NVDS_ST_SUCCESS;
}
static uint8_t om_dfu_nvds_disable_flash(void)
{
    return OM_DFU_NVDS_ST_SUCCESS;
}

const dfu_nvds_itf_t dfu_nvds_itf[] = {
    { //DFU_NVDS_ITF_TYPE_MBR,
        om_dfu_nvds_enable_mbr,
        om_dfu_nvds_get_mbr,
        om_dfu_nvds_put_mbr,
        om_dfu_nvds_del_mbr,
        om_dfu_nvds_disable_mbr,
    },
    { //DFU_NVDS_ITF_TYPE_FLASH,
        om_dfu_nvds_enable_flash,
        om_dfu_nvds_get_flash,
        om_dfu_nvds_put_flash,
        om_dfu_nvds_del_flash,
        om_dfu_nvds_disable_flash,
    },
    { //DFU_NVDS_ITF_TYPE_CFG,
        om_dfu_nvds_enable_dummy,
        om_dfu_nvds_get_dummy,
        om_dfu_nvds_put_dummy,
        om_dfu_nvds_del_dummy,
        om_dfu_nvds_disable_dummy
    },
    { //DFU_NVDS_ITF_TYPE_EXT_FLASH,
        om_dfu_nvds_enable_dummy,
        om_dfu_nvds_get_dummy,
        om_dfu_nvds_put_dummy,
        om_dfu_nvds_del_dummy,
        om_dfu_nvds_disable_dummy
    },
    { //DFU_NVDS_ITF_TYPE_DUMMY,
        om_dfu_nvds_enable_dummy,
        om_dfu_nvds_get_dummy,
        om_dfu_nvds_put_dummy,
        om_dfu_nvds_del_dummy,
        om_dfu_nvds_disable_dummy
    },
};

/** @} */
