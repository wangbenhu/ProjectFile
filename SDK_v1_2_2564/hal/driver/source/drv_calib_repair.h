/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup CALIB_REPAIR CALIB_REPAIR
 * @ingroup  DRIVER
 * @brief    CALIB_REPAIR driver
 * @details  CALIB_REPAIR driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_CALIB_REPAIR_H
#define __DRV_CALIB_REPAIR_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_CALIB)
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
#define DRV_CALIB_REPAIR_DCDC_MAXDBM_DELTA                6
#define DRV_CALIB_REPAIR_LDO_MAXDBM_DELTA                 4

#define DRV_CALIB_REPAIR_DCDC_8DBM_DELTA                  4
#define DRV_CALIB_REPAIR_LDO_8DBM_DELTA                   2

#define DRV_CALIB_REPAIR_DCDC_7DBM_DELTA                  1
#define DRV_CALIB_REPAIR_LDO_7DBM_DELTA                   0

#define DRV_CALIB_REPAIR_DCDC_HIGHT_T_DELTA               3
#define DRV_CALIB_REPAIR_LDO_HIGHT_T_DELTA                1


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    // rf and rc temperature repiar
    uint32_t rc_rf_repair_time;        // repair PMU_Timer count
    int16_t  rc_repair_temperature;
    int16_t  rf_repair_temperature;

    // rc32k temperature repair
    uint32_t rc32k_repair_time;       // repair PMU_Timer count

    // Fix RF bug: temperature
    int16_t temperature;
    // sys voltage
    int8_t trim_vref;           // in CP
    int8_t dig_ldo;             // in CP
    int8_t pa_ldo;              // in CP
    int8_t dcdc_vout;           // in CP
    int8_t pfd_ldo;             // in CP
    int8_t vco_ldo;             // in CP
    int8_t buff_ldo;            // in CP
    int8_t ana_ldo;             // in CP/FT
    uint8_t  soft_major;        // in FT
    uint8_t  soft_minor;        // in FT
    uint16_t soft_svn;          // in FT
    uint8_t  ate_svn;           // in FT
    uint8_t kdco_lut_1m_2406;   // in FT
    uint8_t kdco_lut_1m_2420;   // in FT
    uint8_t kdco_lut_1m_2434;   // in FT
    uint8_t kdco_lut_1m_2448;   // in FT
    uint8_t kdco_lut_1m_2462;   // in FT
    uint8_t kdco_lut_1m_2476;   // in FT
    uint8_t kdco_lut_2m_2406;   // in FT
    uint8_t kdco_lut_2m_2420;   // in FT
    uint8_t kdco_lut_2m_2434;   // in FT
    uint8_t kdco_lut_2m_2448;   // in FT
    uint8_t kdco_lut_2m_2462;   // in FT
    uint8_t kdco_lut_2m_2476;   // in FT
    int8_t con_bias_idac_pll;   // in FT
    int8_t vco_cur;             // in CP/FT
    float kdco_df1_1m;       // in FT, float 253.2k
    float kdco_df1_2m;       // in FT, float 253.2k
} drv_calib_repair_t;


/*******************************************************************************
 * EXTERN VARIABLES
 */
extern drv_calib_repair_t drv_calib_repair_env;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief  calib repair rc rf init
 * @param  t        temperature
 *******************************************************************************
 **/
extern void drv_calib_repair_init(int16_t t);

/**
 *******************************************************************************
 * @brief rf pll temperature repair
 *******************************************************************************
 **/
extern void drv_calib_repair_rf_pll_temperature_repair(bool is_calib_start);

/**
 *******************************************************************************
 * @brief  calib repair rc rf temperature check
 * @return repaired
 *******************************************************************************
 **/
extern bool drv_calib_repair_rc_rf_temperature_check(void);

/**
 *******************************************************************************
 * @brief  calib repair rc32k temperature check
 * @return repaired
 *******************************************************************************
 **/
extern bool drv_calib_repair_rc32k_temperature_check(void);


#ifdef __cplusplus
}
#endif

#endif  /* RTE_CALIB */

#endif  /* __DRV_CALIB_REPAIR_H */


/** @} */
