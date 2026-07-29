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
 * @brief    example for using efuse driver
 * @details  example for using efuse driver: read and write efuse
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
#include "om_driver.h"


/*******************************************************************************
 * MACROS
 */


/*******************************************************************************
 * TYPEDEFS
 */


/*******************************************************************************
 * CONST & VARIABLES
 */
static uint8_t tx_data[2];
static uint8_t rx_data[2];


/*******************************************************************************
 * LOCAL FUNCTIONS
 */


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief examples of reading and writing efuse
 *
 *******************************************************************************
 */
void example_efuse(void)
{
    drv_efuse_init();

    tx_data[0] = 0xaa;
    tx_data[1] = 0xbb;

    drv_efuse_control(EFUSE_CONTROL_PROGRAM_EN, (void *)1);

    drv_efuse_write(0x0, tx_data, 2);

    drv_efuse_read(0x0, rx_data, 2);

    om_printf("read data: 0x%x 0x%x\r\n", rx_data[0], rx_data[1]);
}


/** @} */