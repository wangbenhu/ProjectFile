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
#include <stdint.h>

#ifndef __SERVICE_COMMON_H__
#define __SERVICE_COMMON_H__

/*********************************************************************
 * MACROS
 */
#define SW_INTERNAL_DATE_TIME 	(__DATE__ " " __TIME__)


 //'M','e','e','X','H',' ','C','o','l','l','a','r',' ','M','a','x'
/// GAP Device Name, 
#define GAP_DEVICE_NAME   "MeeXH Collar Max"
/// GAP APPEARANCE
#define GAP_APPEARANCE    "\xc2\x03"
/// DIS SYSTEM ID
#define DIS_SYSTEM_ID     "\x00\x00\x00\x00\x00\x00\x00\x00"
/// DIS HARD VERSION
#define DIS_HARD_VERSION  "0105"
/// DIS SOFT VERSION 

#define DEVICE_VERSION      "02010120260729"
#define DIS_SOFT_VERSION    "V"DEVICE_VERSION

#define DIS_FIRM_REV_STR  SW_INTERNAL_DATE_TIME
/// DIS MANU NAME STR
#define DIS_MANU_NAME_STR "MeeXH & SHMotion"

/// DIS PNP ID
#define DIS_PNP_ID        "\x01\x02\x03\x04\x05\x06\x07"
/// Battery service, set 1 to Enable
#define SERVICE_BATTARY   1

/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Init common services, GAP, DIS, BATT included.
 *******************************************************************************
 */
void service_common_init(void);

#if SERVICE_BATTARY
/**
 *******************************************************************************
 * @brief Notify battery level changed.
 *
 * @param[in] val  The value of battary level
 *******************************************************************************
 */
void batt_level_change(uint8_t val);
#endif

#endif /* __SERVICE_COMMON_H__ */

/** @} */
