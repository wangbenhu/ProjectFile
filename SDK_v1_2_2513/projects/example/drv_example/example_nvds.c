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
 * @brief    example for using nvds
 * @details  example for using nvds:
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
#include "autoconf.h"
#if (CONFIG_NVDS)
#include "om_driver.h"
#include "nvds.h"
#include "nvds_tags.h"


/*******************************************************************************
 * MACROS
 */
#define NVDS_TAG_USER_1     101
#define NVDS_TAG_USER_2     102


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
void example_nvds(void)
{
    nvds_tag_len_t rd_len;
    uint8_t wr_data[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    uint8_t rd_data[10];

    // Flash should to be initialized before using nvds
    const flash_config_t config = {
        .clk_div = 0,
        .delay = 2,
        .read_cmd = FLASH_FAST_READ_DIO,
        .write_cmd = FLASH_PAGE_PROGRAM,
        .spi_mode = FLASH_SPI_MODE_0,
    };
    drv_flash_init(OM_FLASH0, &config);

    // by default, the last four sectors of Flash are used for nvds
    nvds_init(0);

    // put tags
    nvds_put((nvds_tag_id_t)NVDS_TAG_USER_1, 2, wr_data);
    nvds_put((nvds_tag_id_t)NVDS_TAG_USER_2, 3, wr_data);

    // get tags
    nvds_get((nvds_tag_id_t)NVDS_TAG_USER_1, &rd_len, rd_data);
    if (memcmp(rd_data, wr_data, 2) != 0) {
        /* error handle */
    }

    // show all tags
    nvds_dump(om_printf);
    // delete tags
    nvds_del((nvds_tag_id_t)NVDS_TAG_USER_1);
    nvds_dump(om_printf);
    nvds_del((nvds_tag_id_t)NVDS_TAG_USER_2);
    nvds_dump(om_printf);
}

#endif /* CONFIG_NVDS */
