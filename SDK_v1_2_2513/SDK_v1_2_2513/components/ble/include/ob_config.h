/* ----------------------------------------------------------------------------
 * Copyright (c) 2020-2030 OnMicro Limited. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of OnMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT AESLL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * -------------------------------------------------------------------------- */

/**
 * @file     ob_llt.h
 * @brief    ob_llt
 * @date     15 December 2021
 * @author   OnMicro SW Team
 *
 * @defgroup OBLLT OBLLT
 * @ingroup  Peripheral
 * @brief    OBLLT Driver
 * @details  OBLLT Driver

 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __OB_CONFIG_H
#define __OB_CONFIG_H

#ifdef  __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 * INCLUDES
 */
#include <stddef.h>
#include <stdint.h>

/*******************************************************************************
 * MACROS
 */

/// Broadcaster
#if (defined(CONFIG_LE_BROADCASTER) || defined(CONFIG_LE_PERIPHERAL))
#define OB_LE_BROADCASTER      1
#else
#define OB_LE_BROADCASTER      0
#endif

/// Observer
#if (defined(CONFIG_LE_OBSERVER) || defined(CONFIG_LE_CENTRAL))
#define OB_LE_OBSERVER      1
#else
#define OB_LE_OBSERVER      0
#endif

/// Central
#if (defined(CONFIG_LE_CENTRAL))
#define OB_LE_CENTRAL      1
#else
#define OB_LE_CENTRAL      0
#endif

/// Peripheral
#if (defined(CONFIG_LE_PERIPHERAL))
#define OB_LE_PERIPHERAL      1
#else
#define OB_LE_PERIPHERAL      0
#endif

/// LL ECC
#if defined(CONFIG_LE_LL_SEC_CON)
#define OB_LE_LL_SEC_CON        1
#else
#define OB_LE_LL_SEC_CON        0
#endif

/// Maximum number of ADV reports in the HCI queue to Host
#define OB_LE_MAX_NB_ADV_REP_FRAG  2

/// Host ATT MTU
#if defined(CONFIG_LE_HOST_ATT_MTU)
#define OB_LE_HOST_ATT_MTU               CONFIG_LE_HOST_ATT_MTU
#else
#define OB_LE_HOST_ATT_MTU               247
#endif

/// Host ATT prepare write cache size
#if defined(CONFIG_LE_HOST_ATT_WRITE_CACHE_SIZE)
#define OB_LE_HOST_ATT_WRITE_CACHE_SIZE  CONFIG_LE_HOST_ATT_WRITE_CACHE_SIZE
#else
#define OB_LE_HOST_ATT_WRITE_CACHE_SIZE  23
#endif

/// Host GATT max service number
#if defined(CONFIG_LE_HOST_MAX_GATT_SERV_NUM)
#define OB_LE_HOST_MAX_GATT_SERV_NUM     CONFIG_LE_HOST_MAX_GATT_SERV_NUM
#else
#define OB_LE_HOST_MAX_GATT_SERV_NUM     8
#endif

/// Host support secure connection pairing
#if defined(CONFIG_LE_HOST_SC_PAIRING)
#define OB_LE_HOST_SC_PAIRING            1
#else
#define OB_LE_HOST_SC_PAIRING            0
#endif

/// Host max advertise number
#if defined(CONFIG_LE_HOST_ADV_SET_NUM)
#define OB_LE_HOST_ADV_SET_NUM           CONFIG_LE_HOST_ADV_SET_NUM
#else
#define OB_LE_HOST_ADV_SET_NUM           4
#endif

/// Host max connection number
#if defined(CONFIG_LE_HOST_CONNECTION_NB)
#define OB_LE_HOST_CONNECTION_NB         CONFIG_LE_HOST_CONNECTION_NB
#else
#define OB_LE_HOST_CONNECTION_NB         8
#endif

/// Host reserved message size
#if defined(CONFIG_LE_HOST_MSG_SIZE)
#define OB_LE_HOST_MSG_SIZE              CONFIG_LE_HOST_MSG_SIZE
#else
#define OB_LE_HOST_MSG_SIZE              512
#endif

/*******************************************************************************
 * TYPEDEFS
 */


/*******************************************************************************
 * EXTERN FUNCTIONS
 */

#ifdef  __cplusplus
}
#endif

#endif /* __OB_LLT_H */


/** @} */

