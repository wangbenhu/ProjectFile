/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup ERROR ERROR
 * @ingroup  COMMON
 * @brief    templete
 * @details  templete, templete for .h header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __OM_ERROR_H
#define __OM_ERROR_H


/*******************************************************************************
 * TYPEDEFS
 */
typedef enum {
    OM_ERROR_OK                      =  0,
    OM_ERROR_UNSPECIFIC              =  1,
    OM_ERROR_BUSY                    =  2,
    OM_ERROR_TIMEOUT                 =  3,
    OM_ERROR_UNSUPPORTED             =  4,
    OM_ERROR_PARAMETER               =  5,
    OM_ERROR_RESOURCES               =  6,
    OM_ERROR_PERMISSION              =  7,
    OM_ERROR_OUT_OF_RANGE            =  8,      /* parameter out of range */
    OM_ERROR_ALIGN                   =  9,      /* memory align */
    OM_ERROR_STATUS                  =  10,
    OM_ERROR_VERIFY                  =  11,
    OM_ERROR_FAIL                    =  12,
    OM_ERROR_HARDWARE                =  13,

    OM_ERROR_MAX                     = 0xFFFFFFFFU,
} om_error_t;

#endif  /* __OM_ERROR_H */


/** @} */
