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
 * @brief    trace
 * @details  trace using IO
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
#if (CONFIG_TRACE_IO)
#include <stdint.h>
#include "om_device.h"
#include "om_driver.h"
#include "trc_io.h"


/*******************************************************************************
 * MACROS
 */
#if (!CONFIG_TRACE_IO_EVENT_NUM)
#define CONFIG_TRACE_IO_EVENT_NUM         4U
#endif


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    uint8_t pad_idx    : 7U;
    uint8_t idle_level : 1U;
} trc_io_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
static trc_io_t trc_io_evt2io_tbl[CONFIG_TRACE_IO_EVENT_NUM] = {0};


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief  trc io event handler
 *
 * @param[in] event  event
 * @param[in] is_high  IO output state
 *******************************************************************************
 */
void trc_io_event_handler(trc_io_event_t event, int is_high)
{
    uint8_t pad_idx;

    pad_idx = trc_io_evt2io_tbl[event].pad_idx ;
    if(pad_idx != 0) {
        OM_GPIO_Type *om_gpio = drv_gpio_idx2base(pad_idx >> 5U);
        uint32_t idle_level =  trc_io_evt2io_tbl[event].idle_level;
        drv_gpio_write(om_gpio, 1U << (pad_idx & 0x1FU), (gpio_level_t)(is_high ? (!idle_level) : idle_level));
    }
}

void trc_io_set(trc_io_event_t event, uint8_t pad_idx, uint8_t idle_level)
{
    OM_ASSERT(event < TRC_IO_EVENT_NUM);
    OM_ASSERT(pad_idx);

    // Write new configuration
    trc_io_evt2io_tbl[event] = pad_idx + (idle_level ? (1U << 7) : 0U);
    trc_io_event_handler(event, 0);
}

#endif  /* CONFIG_TRACE_IO */


/** @} */
