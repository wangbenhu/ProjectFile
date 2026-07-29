/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup CALIB CALIB
 * @ingroup  DRIVER
 * @brief    CALIB driver
 * @details  CALIB driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_CALIB_H
#define __DRV_CALIB_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief  calib rc32k accuracy check
 *
 * @param[in] win_32k_num  win 32k num
 *
 * @return ppm
 *******************************************************************************
 **/
extern int drv_calib_rc32k_accuracy_check(uint32_t win_32k_num);

/**
 *******************************************************************************
 * @brief calib rc32k
 *******************************************************************************
 **/
extern void drv_calib_rc32k(void);

/**
 *******************************************************************************
 * @brief calib sys rc32m
 *******************************************************************************
 **/
extern void drv_calib_sys_rc32m(void);

/**
 *******************************************************************************
 * @brief calib sys rc
 *******************************************************************************
 **/
extern void drv_calib_sys_rc(void);

/*
 *******************************************************************************
 * @brief calib rf
 * @param t     temperature
 *******************************************************************************
 **/
extern void drv_calib_rf(int16_t t);

/**
 *******************************************************************************
 * @brief  drv calib rf store
 *******************************************************************************
 */
extern void drv_calib_rf_store(void);

/**
 *******************************************************************************
 * @brief  drv calib rf restore
 *******************************************************************************
 */
extern void drv_calib_rf_restore(void);

/**
 *******************************************************************************
 * @brief  drv calib sys restore
 *******************************************************************************
 */
extern void drv_calib_sys_restore(void);

#ifdef __cplusplus
}
#endif

#endif  /* __DRV_CALIB_H */


/** @} */
