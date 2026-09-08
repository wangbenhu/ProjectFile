/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup EFUSE EFUSE
 * @ingroup  DRIVER
 * @brief    EFUSE driver
 * @details  EFUSE driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


#ifndef __DRV_EFUSE_H
#define __DRV_EFUSE_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_EFUSE)
#include <stdint.h>
#include "om_driver.h"


#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * TYPEDEFS
 */
/// Program forbid
typedef enum {
    EFUSE_PROG_FORBID_0_63,         /* User Region */
    EFUSE_PROG_FORBID_RSVD0,        /* RSVD Region 0 */
    EFUSE_PROG_FORBID_RSVD1,        /* RSVD region 1 */
    EFUSE_PROG_FORBID_RSVD2,        /* RSVD region 2 */
} efuse_prog_forbid_t;

/// Efuse control
typedef enum {
    /// Get Efuse Size
    EFUSE_CONTROL_GET_SIZE      = 0x00,
    /// Fetch UID for flash encryption
    EFUSE_CONTROL_FETCH_UID     = 0x01,
    /// Configure efuse for forbid programming
    EFUSE_CONTROL_CFG_FORBID    = 0x02,
    /// Configure efuse for programming Enbale or Disable
    EFUSE_CONTROL_PROGRAM_EN    = 0x03,
    /// Get efsue forbid programming status
    EFUSE_CONTROL_GET_FORBID_STATUS = 0x04,
    /// Switch efuse clock source, 0: rc32mhz, 1: xtal32mhz
    EFUSE_CONTROL_CLK_SRC_SWITCH = 0x05,
} efuse_control_t;

/// Efuse clock source
typedef enum {
    EFUSE_CLK_SRC_RC32M   = 0x00,
    EFUSE_CLK_SRC_XTAL32M = 0x01,
} efuse_clk_src_t;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Initialize efuse
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_efuse_init(void);

/**
 *******************************************************************************
 * @brief Write efuse
 * NOTE: 1. before calling this function, the efuse program must be enabled
 *       2. write efuse need calibed rc32mhz or xtal32mhz
 *
 * @param[in] addr          Efuse address
 * @param[in] data          Pointer to data to be written
 * @param[in] length        Length of data to be written
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_efuse_write(uint32_t addr, const void *data, uint32_t length);

/**
 *******************************************************************************
 * @brief Write efuse using interrupt
 *
 * @param[in] addr          Efuse address
 * @param[in] data          Pointer to data to be written
 * @param[in] length        Length of data to be written
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_efuse_write_int(uint32_t addr, const void *data, uint32_t length);

/**
 *******************************************************************************
 * @brief Read efuse
 *
 * @param[in] addr          Efuse address
 * @param[out] data         Pointer to buffer to store read data
 * @param[in] length        Length of data to be read
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_efuse_read(uint32_t addr, void *data, uint32_t length);

#if (RTE_EFUSE_REGISTER_CALLBACK)
/**
 *******************************************************************************
 * @brief Register efuse callback
 *
 * @param[in] cb            Callback function
 *******************************************************************************
 */
extern void drv_efuse_resgister_isr_callback(drv_isr_callback_t cb);
#endif

/**
 *******************************************************************************
 * @brief Efuse interrupt handler
 *
 * @param[in] event         Event type
 * @param[in] addr          Efuse wrote address
 * @param[in] num           Efuse wrote length
 *******************************************************************************
 */
extern void drv_efuse_isr_callback(drv_event_t event, uint32_t addr, uint32_t num);

/**
 *******************************************************************************
 * @brief Efuse interrupt service routine
 *******************************************************************************
 */
extern void drv_efuse_isr(void);

/**
 *******************************************************************************
 * @brief Efuse forbid config
 *
 * @param[in] prog_forbid_type   Efuse forbid type
 *
 * @return om_error
 *******************************************************************************
 */
extern om_error_t drv_efuse_forbid_config(efuse_prog_forbid_t prog_forbid_type);

/**
 *******************************************************************************
 * @brief Efuse control
 *
 * @param[in] control       Efuse control type
 * @param[in] argu          Control argument
 *
 * @return om_error or efuse size
 *******************************************************************************
 */
__STATIC_INLINE void *drv_efuse_control(efuse_control_t control, void *argu)
{
    uint32_t ret;

    ret = (uint32_t)OM_ERROR_OK;
    switch (control) {
        case EFUSE_CONTROL_GET_SIZE:
            ret = (CAP_EFUSE & CAP_EFUSE_SIZE_MASK) >> CAP_EFUSE_SIZE_POS;
            break;

        case EFUSE_CONTROL_PROGRAM_EN:
            OM_EFUSE->PROGRAM_ENABLE = (uint32_t)argu ? 1 : 0;
            break;

        case EFUSE_CONTROL_FETCH_UID:
            OM_EFUSE->CTRL |= EFUSE_CTRL_UID_READ_MASK;
            while(OM_EFUSE->CTRL & EFUSE_CTRL_UID_READ_MASK);
            break;

        case EFUSE_CONTROL_CFG_FORBID:
            ret = (uint32_t)drv_efuse_forbid_config((efuse_prog_forbid_t)(uint32_t)argu);
            break;

        case EFUSE_CONTROL_GET_FORBID_STATUS:
            ret = register_get(&OM_EFUSE->CTRL, MASK_POS(EFUSE_CTRL_PMU_PROG_FORBID));
            break;

        case EFUSE_CONTROL_CLK_SRC_SWITCH:
            if ((uint32_t)argu) {
                OM_CPM->EFUSE_CFG |= CPM_EFUSE_CFG_CLK_SRC_SEL_MASK;
            } else {
                OM_CPM->EFUSE_CFG &= ~CPM_EFUSE_CFG_CLK_SRC_SEL_MASK;
            }
            while (!(OM_CPM->EFUSE_CFG & CPM_EFUSE_CFG_CLK_SYNC_DONE_MASK));

        default:
            ret = (uint32_t)OM_ERROR_PARAMETER;
            break;
    }

    return (void *)ret;
}


#ifdef __cplusplus
}
#endif

#endif  /* RTE_EFUSE */

#endif  /* __DRV_EFUSE_H */


/** @} */
