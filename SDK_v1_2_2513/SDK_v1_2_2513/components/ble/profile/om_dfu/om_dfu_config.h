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

#ifndef __OM_DFU_CONFIG_H__
#define __OM_DFU_CONFIG_H__

/*******************************************************************************
 * INCLUDES
 */
#include "stdlib.h"
#include "om_dfu_nvds.h"

//#define FLASH_ERASE_SIZE 0x1000   // Image写入地址必须以擦除大小对齐

/******* Warning: 不要在 om_dfu.c 以外的地方include该文件! *******/

// enum image_type{ //Defined in om_dfu_nvds.h
//     IMAGE_TYPE_APP,
//     IMAGE_TYPE_PATCH,
//     IMAGE_TYPE_CONFIG,
//     IMAGE_TYPE_MBR_USR1,
//     IMAGE_TYPE_MBR_USR2,
//     IMAGE_TYPE_MBR_USR3,
//     IMAGE_TYPE_MBR_USR4,
//     IMAGE_TYPE_DUMMY,
//     IMAGE_TYPE_CUSTOM = 0x10,
//     IMAGE_TYPE_RAW    = 0x5F,
// };

typedef struct {
	uint16_t type; // @ref enum image_type
	uint32_t base_address1;
	uint32_t base_address2;
	uint32_t max_length;
	const char *describe; // 功能标识，仅用于Debug
    const dfu_nvds_itf_t *image_ops_itf; // Imgage 读写接口
    const dfu_nvds_itf_t *info_ops_itf;  // Image info 存储接口
	uint16_t info_id; //Image 信息在 NVDS 中存放的位置
}dfu_image_t;

const dfu_image_t dfu_image_types[] = {
//----------------------------------------------------------------------------------------------------------
 /* Image type               |  Base address1 |  Base address2 |  Max length |  Image describe  |
        Image ops API                                | Image info ops API                       |  Info ID */
//----------------------------------------------------------------------------------------------------------
	{IMAGE_TYPE_APP          ,  0x00004000    ,  0x00077000    ,   0x00073000,  "Application"   ,//320KB APP，140KB audio
        &dfu_nvds_itf[DFU_NVDS_ITF_TYPE_FLASH]       , NULL                                     , 0x00    },
//----------------------------------------------------------------------------------------------------------
	{IMAGE_TYPE_MBR_USR1     ,  0x00003000    ,  0x00003000    ,   0x00001000,  "MBR"         ,
        &dfu_nvds_itf[DFU_NVDS_ITF_TYPE_FLASH]       , NULL                                     , 0x00    },
//----------------------------------------------------------------------------------------------------------
	//{IMAGE_TYPE_DUMMY        ,  0x00054000    ,  0x000C7000    ,   0x00023000,  "Dummy"         ,
	{IMAGE_TYPE_DUMMY        ,  0x00000000    ,  0x00000000    ,   0xFFFFFFFF,  "Dummy"         ,
   
	&dfu_nvds_itf[DFU_NVDS_ITF_TYPE_DUMMY]       , NULL                                     , 0x00    },
//----------------------------------------------------------------------------------------------------------
};
const uint8_t dfu_image_types_num = sizeof(dfu_image_types) / sizeof(dfu_image_types[0]);

#endif /* __OM_DFU_CONFIG_H__ */

/** @} */

