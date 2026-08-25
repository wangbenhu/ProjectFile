/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup DRIVER Driver
 * @ingroup  HAL
 * @brief    driver header
 * @details
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __OM_DRIVER_H
#define __OM_DRIVER_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#include "om_common.h"

#include "../source/drv_cortex.h"
#include "../source/drv_dwt.h"
#include "../source/drv_utils.h"
#include "../source/drv_common.h"
#include "../source/drv_rcc.h"
#include "../source/drv_pmu.h"
#include "../source/drv_pin.h"
#include "../source/drv_calib.h"
#include "../source/drv_calib_repair.h"
#include "../source/drv_gpadc.h"
#include "../source/drv_cache.h"
#include "../source/drv_efuse.h"
#include "../source/drv_uart.h"
#include "../source/drv_spi.h"
#include "../source/drv_i2c.h"
#include "../source/drv_wdt.h"
#include "../source/drv_pmu_timer.h"
#include "../source/drv_om24g.h"
#include "../source/drv_aes.h"
#include "../source/drv_sm/drv_sm2.h"
#include "../source/drv_sm/drv_sm3.h"
#include "../source/drv_sm/drv_sm4.h"
#include "../source/drv_gpio.h"
#include "../source/drv_gpdma.h"
#include "../source/drv_rtc.h"
#include "../source/drv_radio.h"
#include "../source/drv_tim.h"
#include "../source/drv_lptim.h"
#include "../source/drv_rng.h"
#include "../source/drv_i2s.h"
#include "../source/drv_audio.h"
#include "../source/drv_lcd.h"
#include "../source/drv_irtx.h"
#include "../source/drv_sha256.h"
#include "../source/drv_ecdsa/drv_ecdsa.h"
#include "../source/drv_ospi.h"
#include "../source/drv_flash/drv_flash.h"
#include "../source/drv_psram.h"
#include "../source/drv_qdec.h"
#include "../source/drv_rgb.h"


/// @cond
/*******************************************************************************
 * MACROS
 */
// common peripheral
#define DRV_DELAY_CYCLES(cycles)        drv_dwt_delay_cycles(cycles)
#define DRV_DELAY_US(us)                drv_dwt_delay_us(us)
#define DRV_DELAY_MS(ms)                drv_dwt_delay_ms(ms)

#define DRV_WAIT_MS_UNTIL_TO(wait_val, to_ms, ret)  DRV_DWT_WAIT_MS_UNTIL_TO(wait_val, to_ms, ret)
#define DRV_WAIT_US_UNTIL_TO(wait_val, to_us, ret)  DRV_DWT_WAIT_US_UNTIL_TO(wait_val, to_us, ret)

/// @endcond

#endif  /* __OM_DRIVER_H */


/** @} */
