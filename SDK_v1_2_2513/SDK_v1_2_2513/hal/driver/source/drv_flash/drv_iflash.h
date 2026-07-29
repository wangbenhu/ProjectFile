/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup FLASH FLASH
 * @ingroup  HAL_Driver
 * @brief    FLASH Driver for om66xx
 * @details  FLASH Driver for om66xx
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_IFLASH_H
#define __DRV_IFLASH_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_FLASH0)
#include <stdint.h>
#include "drv_flash_common.h"
#include "om_driver.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Initialize internal FLASH controller with specified configuration struct
 *        Note:
 *          - This function will enable write protect,
 *            and the type of write protect is FLASH_PROTECT_UPPER_4_SECTOR_UNPROTECTED,
 *            if you want to change the type of write protect,
 *            please call drv_iflash_write_protect_set function.
 *
 * @param om_flash  The FLASH controller device address
 * @param config    The configuration struct pointer, see@ref flash_config_t
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_init(OM_SF_Type *om_flash, const flash_config_t *config);

/**
 *******************************************************************************
 * @brief           iflash delay recalib
 *
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_delay_recalib(void);

/**
 *******************************************************************************
 * @brief           iflash get delay info
 *
 * @param info      Delay information pointer, see@ref flash_delay_info_t
 *
 *******************************************************************************
 */
extern void drv_iflash_get_delay_info(flash_delay_info_t *info);

/**
 *******************************************************************************
 * @brief internal FLASH read id
 * @param om_flash  The internal FLASH controller device address
 * @param id        The ID struct pointer, see@ref flash_id_t
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_read_id(OM_SF_Type *om_flash, flash_id_t *id);

/**
 *******************************************************************************
 * @brief internal FLASH read uid
 * @param om_flash  The internal FLASH controller device address
 * @param uid       The unique ID buffer
 * @param len       The length of UID buffer
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_read_uid(OM_SF_Type *om_flash, uint8_t *uid, uint32_t len);

/**
 *******************************************************************************
 * @brief Get internal FLASH id
 *
 * @param om_flash  The FLASH controller device address
 * @param id        The ID struct pointer, see@ref flash_id_t
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_id_get(OM_SF_Type *om_flash, flash_id_t *id);

/**
 *******************************************************************************
 * @brief Set new command for reading
 *
 * @param om_flash  The internal FLASH controller device address
 * @param read_cmd  The new read command, see@ref flash_read_t
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_read_cmd_set(OM_SF_Type *om_flash, flash_read_t read_cmd);

/**
 *******************************************************************************
 * @brief Set new command for writing
 *
 * @param om_flash  The internal FLASH controller device address
 * @param write_cmd The new write command, see@ref flash_write_t
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_write_cmd_set(OM_SF_Type *om_flash, flash_write_t write_cmd);

/**
 *******************************************************************************
 * @brief internal FLASH read, wait for operation done
 *
 * @param om_flash   The internal FLASH controller device address
 * @param addr       The address of FLASH
 * @param data       The reading data buffer pointer in RAM
 * @param data_len   The reading data length
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_read(OM_SF_Type *om_flash,
                                  uint32_t addr,
                                  uint8_t *data,
                                  uint32_t data_len);

/**
 *******************************************************************************
 * @brief internal FLASH read with interrupt enabled
 *
 * @param om_flash  The internal FLASH controller device address
 * @param addr      The address of FLASH
 * @param data      The reading data buffer pointer in RAM
 * @param data_len  The reading data length
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_read_int(OM_SF_Type *om_flash, uint32_t addr, uint8_t *data, uint32_t data_len);

/**
 *******************************************************************************
 * @brief internal FLASH write, wait for operation done
 *
 * @param om_flash  The internal FLASH controller device address
 * @param addr      The address of FLASH
 * @param data      The writing data buffer pointer in RAM,
 *                  Attention: the type must be volatile to avoid compiler optimization
 *                              when Link Time Optimization is enabled,
 *                              if the data is from heap, it should be sure to be volatile,
 *                              for example: volatile uint8_t *data = om_mem_malloc(X, X);
 * @param data_len  The writing data length
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_write(OM_SF_Type *om_flash,
                                   uint32_t addr,
                                   volatile uint8_t *data,
                                   uint32_t data_len);

/**
 *******************************************************************************
 * @brief internal FLASH write with interrupt enabled
 *          1. drv_iflash_write_int_start
 *          2. drv_iflash_write_int_status_get
 *          3. if is_wip is 1, go to step 2, else go to step 4
 *          4. drv_iflash_write_int_continue
 *          5. go to step 2
 *          6. loop step 2 - 5,
 *             until interrupt callback DRV_EVENT_COMMON_WRITE_COMPLETED event is triggered
 *
 * @param om_flash  The internal FLASH controller device address
 * @param addr      The address of FLASH
 * @param data      The writing data buffer pointer in RAM
 *                  Attention: the type must be volatile to avoid compiler optimization
 *                              when Link Time Optimization is enabled,
 *                              if the data is from heap, it should be sure to be volatile,
 *                              for example: volatile uint8_t *data = om_mem_malloc(X, X);
 * @param data_len  The writing data length
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_write_int_start(OM_SF_Type *om_flash,
                                             uint32_t addr,
                                             volatile uint8_t *data,
                                             uint32_t data_len);

/**
 *******************************************************************************
 * @brief FLASH is write in progress, it should be called after drv_flash_write_int_start
 *
 * @param om_flash  The FLASH controller device address
 * @param is_wip    The Flash is in write progress status or not, 1 means yes, 0 means no
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_write_int_status_get(OM_SF_Type *om_flash, uint8_t *is_wip);

/**
 *******************************************************************************
 * @brief FLASH write with interrupt continue, it should be called when flash wip
 *        cleared(value:0), the wip statue is queried by drv_iflash_write_int_status_get,
 *        and it will finished when the write interrupt is occured.
 *
 * @param om_flash  The FLASH controller device address
 *
 * @return    Error code, see@ref om_error_t
 *            OM_ERROR_OK indicates continue writing
 *            OM_ERROR_RESOURCES indicates writed completed
 *            OM_ERROR_STATUS indicates iflash not in writing/programing state
 *******************************************************************************
 */
extern om_error_t drv_iflash_write_int_continue(OM_SF_Type *om_flash);

/**
 *******************************************************************************
 * @brief internal FLASH erase
 *
 * @param om_flash      The internal FLASH controller device address
 * @param addr          The address of FLASH
 * @param erase_type    The erase size, see@ref flash_erase_t
 *
 * @return              Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_erase(OM_SF_Type *om_flash, uint32_t addr, flash_erase_t erase_type);

/**
 *******************************************************************************
 * @brief internal FLASH erase start, it is not supported in xip mode
 *        note: call drv_iflash_erase_is_done to check if erase is done,
 *              if not, call again until erase is done
 *
 * @param om_flash      The internal FLASH controller device address
 * @param addr          The address of FLASH
 * @param erase_type    The erase size, see@ref flash_erase_t
 *
 * @return              Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_erase_start(OM_SF_Type *om_flash, uint32_t addr, flash_erase_t erase_type);

/**
 *******************************************************************************
 * @brief internal FLASH erase is done, it is not supported in xip mode
 *        note: it should be called after drv_iflash_erase_start to check if erase is done,
 *              if not, call again until erase is done
 *
 * @param om_flash      The internal FLASH controller device address
 *
 * @return              1 means erase is done, 0 means erase is not done
 *
 *******************************************************************************
 */
extern uint8_t drv_iflash_erase_is_done(OM_SF_Type *om_flash);

/**
 *******************************************************************************
 * @brief internal FLASH read status 1 register
 *
 * @param om_flash  The internal FLASH controller device address
 * @param status    Status 1 register read buffer pointer
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_read_status_reg1(OM_SF_Type *om_flash, uint8_t *status);

/**
 *******************************************************************************
 * @brief internal FLASH read status 2 register
 *
 * @param om_flash  The internal FLASH controller device address
 * @param status    Status 2 register read buffer pointer
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_read_status_reg2(OM_SF_Type *om_flash, uint8_t *status);

/**
 *******************************************************************************
 * @brief internal FLASH read config register
 *
 * @param om_flash  The internal FLASH controller device address
 * @param config    Config register read buffer pointer
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_read_config_reg(OM_SF_Type *om_flash, uint8_t *config);

/**
 *******************************************************************************
 * @brief internal FLASH write status 1 or 1&2 register, check ID for writing separately or together
 *
 * @param om_flash   The internal FLASH controller device address
 * @param status     Status register read buffer pointer
 * @param status_len Status register length
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_write_status(OM_SF_Type *om_flash, uint8_t *status, uint8_t status_len);

/**
 *******************************************************************************
 * @brief internal FLASH write status 1 or 1&2 register, check ID for writing separately or together
 *        note: this is a volatile function, it will lost after power down
 *
 * @param om_flash   The internal FLASH controller device address
 * @param status     Status register read buffer pointer
 * @param status_len Status register length
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_write_status_volatile(OM_SF_Type *om_flash, uint8_t *status, uint8_t status_len);

/**
 *******************************************************************************
 * @brief internal FLASH modify status 1&2 register bits
 *
 * @param om_flash  The internal FLASH controller device address
 * @param status    Status register read buffer pointer
 * @param mask      Status register mask
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_modifiy_status_bits(OM_SF_Type *om_flash, uint8_t status[2], uint8_t mask[2]);

/**
 *******************************************************************************
 * @brief internal FLASH modify status 1&2 register bits
 *        note: this is a volatile function, it will lost after power down
 *
 * @param om_flash  The internal FLASH controller device address
 * @param status    Status register read buffer pointer
 * @param mask      Status register mask
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_modifiy_status_bits_volatile(OM_SF_Type *om_flash, uint8_t status[2], uint8_t mask[2]);

/**
 *******************************************************************************
 * @brief internal FLASH write config register
 *
 * @param om_flash  The internal FLASH controller device address
 * @param config    Config register read buffer pointer
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_write_config_reg(OM_SF_Type *om_flash, uint8_t *config);

/**
 *******************************************************************************
 * @brief internal FLASH write config register
 *        note: this is a volatile function, it will lost after power down
 *
 * @param om_flash  The internal FLASH controller device address
 * @param config    Config register read buffer pointer
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_write_config_reg_volatile(OM_SF_Type *om_flash, uint8_t *config);

/**
 *******************************************************************************
 * @brief internal FLASH set write protect region, the protected region cannot be erased or written
 *
 *      Attention:
 *      This function support the following flash:
 *      GD25WQ16E, GD25WQ80E, GD25WQ40E,
 *      P25Q40SU, P25Q80SU, P25Q16SU,
 *      GT25Q40D, GT25Q80A
 *      Other flash without testing,
 *      if not supported, please use drv_oflash_modifiy_status_bits to set the status register
 *
 * @param om_flash  The internal FLASH controller device address
 * @param protect   Write protect region, see@ref flash_protect_t
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
*/
extern om_error_t drv_iflash_write_protect_set(OM_SF_Type *om_flash, flash_protect_t protect);

/**
 *******************************************************************************
 * @brief internal FLASH set write protect region, the protected region cannot be erased or written
 *        note: this is a volatile function, it will lost after power down
 *
 *      Attention:
 *      This function support the following flash:
 *      GD25WQ16E, GD25WQ80E, GD25WQ40E,
 *      P25Q40SU, P25Q80SU, P25Q16SU,
 *      GT25Q40D, GT25Q80A
 *      Other flash without testing,
 *      if not supported, please use drv_oflash_modifiy_status_bits to set the status register
 *
 * @param om_flash  The internal FLASH controller device address
 * @param protect   Write protect region, see@ref flash_protect_t
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
*/
extern om_error_t drv_iflash_write_protect_set_volatile(OM_SF_Type *om_flash, flash_protect_t protect);

/**
 *******************************************************************************
 * @brief internal FLASH enable WP and HOLD pin as data pin for quad read and write
 *
 * @param om_flash  The internal FLASH controller device address
 * @param enable    Enable or disable
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_quad_enable(OM_SF_Type *om_flash, bool enable);

/**
 *******************************************************************************
 * @brief internal FLASH enable WP and HOLD pin as data pin for quad read and write
 *        note: this is a volatile function, it will lost after power down
 *
 * @param om_flash  The internal FLASH controller device address
 * @param enable    Enable or disable
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_quad_enable_volatile(OM_SF_Type *om_flash, bool enable);

/**
 *******************************************************************************
 * @brief internal FLASH reset
 *
 * @param om_flash  The internal FLASH controller device address
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_reset(OM_SF_Type *om_flash);

/**
 *******************************************************************************
 * @brief internal FLASH encrypt enable
 *
 * @param om_flash  The internal FLASH controller device address
 * @param enable    Enable or disable
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_encrypt_enable(OM_SF_Type *om_flash, uint8_t enable);

/**
 *******************************************************************************
 * @brief internal FLASH store
 *
 *******************************************************************************
 */
extern void drv_iflash_store(void);

/**
 *******************************************************************************
 * @brief internal FLASH restore
 *
 *******************************************************************************
 */
extern void drv_iflash_restore(void);

/**
 *******************************************************************************
 * @brief internal FLASH secure register erase
 *
 * @param om_flash  The internal FLASH controller device address
 * @param secure_register  Secure register(0, 1, 2...)
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_secure_register_erase(OM_SF_Type *om_flash,
                                                   uint8_t secure_register);

/**
 *******************************************************************************
 * @brief internal FLASH secure register write
 *
 * @param om_flash  The internal FLASH controller device address
 * @param secure_register  Secure register(0, 1, 2...)
 * @param addr  Secure register address offset
 * @param data  Data to write
 * @param data_len  Data length
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_secure_register_write(OM_SF_Type *om_flash,
                                                   uint8_t secure_register,
                                                   uint16_t addr,
                                                   uint8_t *data,
                                                   uint16_t data_len);

/**
 *******************************************************************************
 * @brief internal FLASH secure register read
 *
 * @param om_flash  The internal FLASH controller device address
 * @param secure_register  Secure register(0, 1, 2...)
 * @param addr  Secure register address offset
 * @param data  Data to read
 * @param data_len  Data length
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_secure_register_read(OM_SF_Type *om_flash,
                                                  uint8_t secure_register,
                                                  uint16_t addr,
                                                  uint8_t *data,
                                                  uint16_t data_len);

#if (RTE_FLASH0_REGISTER_CALLBACK)
/**
 *******************************************************************************
 * @brief internal FLASH controller register callback
 *
 * @param om_flash  The internal FLASH controller device address
 * @param isr_cb    Callback
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_iflash_register_isr_callback(OM_SF_Type *om_flash, drv_isr_callback_t isr_cb);
#endif

/**
 *******************************************************************************
 * @brief The interrupt callback for internal FLASH controller driver. It is a weak function. User should define
 *        their own callback in user file, other than modify it in the FLASH driver.
 *
 * @param om_flash  The internal FLASH controller device address
 * @param event     The driver event
 *                  - DRV_EVENT_COMMON_WRITE_COMPLETED
 *                  - DRV_EVENT_COMMON_READ_COMPLETED
 *                  - DRV_EVENT_COMMON_ERROR
 *******************************************************************************
 */
extern void drv_iflash_isr_callback(OM_SF_Type *om_flash, drv_event_t event);

/**
 *******************************************************************************
 * @brief The internal FLASH controller interrupt handler
 *
 * @param om_flash  The internal FLASH controller device address
 *******************************************************************************
 */
extern void drv_iflash_isr(OM_SF_Type *om_flash);

#ifdef __cplusplus
}
#endif

#endif  /* (RTE_FLASH) */
#endif  /* __DRV_IFLASH_H */

/** @} */
