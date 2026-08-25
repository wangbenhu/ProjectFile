/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup FLASH FLASH
 * @ingroup  DRIVER
 * @brief    FLASH Driver
 * @details  FLASH Driver
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_FLASH_H
#define __DRV_FLASH_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_FLASH0 || RTE_FLASH1)
#include <stdint.h>
#include "om_driver.h"
#include "drv_iflash.h"
#include "drv_oflash.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
#define FLASH_READ_PARAM_DMY_4      ((1 << 4) | (0 << 5))
#define FLASH_READ_PARAM_DMY_6      ((0 << 4) | (1 << 5))
#define FLASH_READ_PARAM_DMY_8      ((1 << 4) | (1 << 5))
#define FLASH_READ_PARAM_DMY_10     ((0 << 4) | (0 << 5))


/*******************************************************************************
 * TYPEDEFS
 */
typedef enum {
    OM_FLASH0 = (uint32_t)OM_SF0,
    OM_FLASH1 = (uint32_t)OM_OSPI1,
} OM_FLASH_Type;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Initialize FLASH controller with specified configuration struct
 *        Note:
 *          - This function will enable write protect for FLASH0, but not for FLASH1,
 *            if you want to change the type of write protect for FLASH0,
 *            please call drv_flash_write_protect_set function,
 *            and the type of write protect for FLASH1 will not be changed in this function.
 *          - For Flash1, the default mode is 3-Byte address.
 *            If you want to change the mode to 4-Byte address,
 *            please call drv_flash_4byte_addr_enable function,
 *            and the mode should be enabled again after the FLASH1 is initialized every time,
 *            because the initialization will reset the mode to 3-Byte address.
 *
 * @param om_flash  The FLASH controller device address
 * @param config    The configuration struct pointer
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_init(OM_FLASH_Type om_flash, const flash_config_t *config);

/**
 *******************************************************************************
 * @brief FLASH read id, it will send command to flash
 * @param om_flash  The FLASH controller device address
 * @param id        The ID struct pointer
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_read_id(OM_FLASH_Type om_flash, flash_id_t *id);

/**
 *******************************************************************************
 * @brief FLASH read uid
 * @param om_flash  The FLASH controller device address
 * @param uid       The unique ID buffer
 * @param len       The length of UID buffer
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_read_uid(OM_FLASH_Type om_flash, uint8_t *uid, uint32_t len);

/**
 *******************************************************************************
 * @brief Get FLASH id, the id is saved in buffer already
 *
 * @param om_flash  The FLASH controller device address
 * @param id        The ID struct pointer
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_id_get(OM_FLASH_Type om_flash, flash_id_t *id);

/**
 *******************************************************************************
 * @brief Set new command for reading
 *
 * @param om_flash  The FLASH controller device address
 * @param read_cmd  The new read command
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_read_cmd_set(OM_FLASH_Type om_flash, flash_read_t read_cmd);

/**
 *******************************************************************************
 * @brief Set new command for writing
 *
 * @param om_flash  The FLASH controller device address
 * @param write_cmd The new write command
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_write_cmd_set(OM_FLASH_Type om_flash, flash_write_t write_cmd);

/**
 *******************************************************************************
 * @brief FLASH read, wait for operation done
 *
 * @param om_flash    The FLASH controller device address
 * @param addr        The address of FLASH
 * @param data        The reading data buffer pointer in RAM
 * @param data_len    The reading data length
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_read(OM_FLASH_Type om_flash,
                                 uint32_t addr,
                                 uint8_t *data,
                                 uint32_t data_len);

/**
 *******************************************************************************
 * @brief FLASH read with interrupt enabled
 *
 * @param om_flash    The FLASH controller device address
 * @param addr        The address of FLASH
 * @param data        The reading data buffer pointer in RAM
 * @param data_len    The reading data length
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_read_int(OM_FLASH_Type om_flash, uint32_t addr, uint8_t *data, uint32_t data_len);

/**
 *******************************************************************************
 * @brief FLASH write, wait for operation done
 *
 * @param om_flash    The FLASH controller device address
 * @param addr        The address of FLASH
 * @param data        The writing data buffer pointer in RAM
 *                    Attention: the type must be volatile to avoid compiler optimization
 *                               when Link Time Optimization is enabled,
 *                               if the data is from heap, it should be sure to be volatile,
 *                               for example: volatile uint8_t *data = om_mem_malloc(X, X);
 * @param data_len    The writing data length
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_write(OM_FLASH_Type om_flash,
                                  uint32_t addr,
                                  volatile uint8_t *data,
                                  uint32_t data_len);

/**
 *******************************************************************************
 * @brief FLASH write with interrupt enabled, the write interrupt step is:
 *          1. drv_flash_write_int_start
 *          2. drv_flash_write_int_status_get
 *          3. if is_wip is 1, go to step 2, else go to step 4
 *          4. drv_flash_write_int_continue
 *          5. go to step 2
 *          6. loop step 2 - 5,
 *             until the return value of step 4 is OM_ERROR_STATUS
 *
 * @param om_flash    The FLASH controller device address
 * @param addr        The address of FLASH
 * @param data        The writing data buffer pointer in RAM
 *                    Attention: the type must be volatile to avoid compiler optimization
 *                               when Link Time Optimization is enabled,
 *                               if the data is from heap, it should be sure to be volatile,
 *                               for example: volatile uint8_t *data = om_mem_malloc(X, X);
 * @param data_len    The writing data length
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_write_int_start(OM_FLASH_Type om_flash,
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
extern om_error_t drv_flash_write_int_status_get(OM_FLASH_Type om_flash, uint8_t *is_wip);

/**
 *******************************************************************************
 * @brief FLASH write with interrupt continue, it should be called when flash wip cleared(value:0),
 *          the wip state is queried by drv_flash_write_int_status_get, and this process should finish
 *          when the return value is OM_ERROR_STATUS.
 *
 * @param om_flash  The FLASH controller device address
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_write_int_continue(OM_FLASH_Type om_flash);

/**
 *******************************************************************************
 * @brief FLASH erase
 *
 * @param om_flash      The FLASH controller device address
 * @param addr          The address of FLASH
 * @param erase_type    The erase size
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_erase(OM_FLASH_Type om_flash,
                                  uint32_t addr,
                                  flash_erase_t erase_type);

/**
 *******************************************************************************
 * @brief FLASH erase start, it is not supported in xip mode
 *        note: call drv_flash_erase_is_done to check if erase is done,
 *              if not, call again until erase is done
 *
 * @param om_flash      The FLASH controller device address
 * @param addr          The address of FLASH
 * @param erase_type    The erase size
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_erase_start(OM_FLASH_Type om_flash,
                                        uint32_t addr,
                                        flash_erase_t erase_type);

/**
 *******************************************************************************
 * @brief FLASH erase is done, it is not supported in xip mode
 *        note: it should be called after drv_flash_erase_start to check if erase is done,
 *              if not, call again until erase is done
 *
 * @param om_flash      The FLASH controller device address
 *
 * @return              1 means erase is done, 0 means erase is not done
 *******************************************************************************
 */
extern uint8_t drv_flash_erase_is_done(OM_FLASH_Type om_flash);

/**
 *******************************************************************************
 * @brief FLASH set write protect region, the protected region cannot be erased or written
 *
 *      Attention:
 *      This function support the following flash:
 *      GD25WQ16E, GD25WQ80E, GD25WQ40E,
 *      P25Q40SU, P25Q80SU, P25Q16SU,
 *      GT25Q40D, GT25Q80A
 *      Other flash without testing,
 *      if not supported, please use drv_oflash_modifiy_status_bits to set the status register
 *
 * @param om_flash  The FLASH controller device address
 * @param protect   Write protect region
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
*/
extern om_error_t drv_flash_write_protect_set(OM_FLASH_Type om_flash, flash_protect_t protect);

/**
 *******************************************************************************
 * @brief FLASH set write protect region, the protected region cannot be erased or written
 *        note: this function is volatile, it will lost after power down
 *
 *      Attention:
 *      This function support the following flash:
 *      GD25WQ16E, GD25WQ80E, GD25WQ40E,
 *      P25Q40SU, P25Q80SU, P25Q16SU,
 *      GT25Q40D, GT25Q80A
 *      Other flash without testing,
 *      if not supported, please use drv_oflash_modifiy_status_bits to set the status register
 *
 * @param om_flash  The FLASH controller device address
 * @param protect   Write protect region
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
*/
extern om_error_t drv_flash_write_protect_set_volatile(OM_FLASH_Type om_flash, flash_protect_t protect);

/**
 *******************************************************************************
 * @brief FLASH enable WP and HOLD pin as data pin for quad read and write
 *
 * @param om_flash  The FLASH controller device address
 * @param enable    Enable or disable
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_quad_enable(OM_FLASH_Type om_flash, bool enable);

/**
 * @brief FLASH enable WP and HOLD pin as data pin for quad read and write
 *        note: this function is volatile, it will lost after power down
 *
 * @param om_flash  The FLASH controller device address
 * @param enable    Enable or disable
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_quad_enable_volatile(OM_FLASH_Type om_flash, bool enable);

/**
 *******************************************************************************
 * @brief FLASH reset
 *
 * @param om_flash  The FLASH controller device address
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_reset(OM_FLASH_Type om_flash);

/**
 *******************************************************************************
 * @brief FLASH encrypt enable
 *
 * @param om_flash  The FLASH controller device address
 * @param enable    Enable or disable
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_encrypt_enable(OM_FLASH_Type om_flash, uint8_t enable);

/**
 *******************************************************************************
 * @brief FLASH read status 1 register
 *
 * @param om_flash  The FLASH controller device address
 * @param status    Status 1 register read buffer pointer
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_read_status_reg1(OM_FLASH_Type om_flash, uint8_t *status);

/**
 *******************************************************************************
 * @brief FLASH read status 2 register
 *
 * @param om_flash  The FLASH controller device address
 * @param status    Status 2 register read buffer pointer
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_read_status_reg2(OM_FLASH_Type om_flash, uint8_t *status);

/**
 *******************************************************************************
 * @brief FLASH read config register
 *
 * @param om_flash  The FLASH controller device address
 * @param config    Config register read buffer pointer
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_read_config_reg(OM_FLASH_Type om_flash, uint8_t *config);

/**
 *******************************************************************************
 * @brief FLASH write status 1&2 register, check ID for writing separately or together
 *
 * @param om_flash  The FLASH controller device address
 * @param status    Status register read buffer pointer
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_write_status(OM_FLASH_Type om_flash, uint8_t status[2]);

/**
 * @brief FLASH write status 1&2 register, check ID for writing separately or together
 *        note: this function is volatile, it will lost after power down
 *
 * @param om_flash  The FLASH controller device address
 * @param status    Status register read buffer pointer
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_write_status_volatile(OM_FLASH_Type om_flash, uint8_t status[2]);

/**
 *******************************************************************************
 * @brief FLASH modify status 1&2 register bits
 *
 * @param om_flash  The FLASH controller device address
 * @param status    Status register read buffer pointer
 * @param mask      Status register mask
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_modifiy_status_bits(OM_FLASH_Type om_flash, uint8_t status[2], uint8_t mask[2]);

/**
 * @brief FLASH modify status 1&2 register bits
 *        note: this function is volatile, it will lost after power down
 *
 * @param om_flash  The FLASH controller device address
 * @param status    Status register read buffer pointer
 * @param mask      Status register mask
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_modifiy_status_bits_volatile(OM_FLASH_Type om_flash, uint8_t status[2], uint8_t mask[2]);

/**
 *******************************************************************************
 * @brief FLASH write status 1 register
 *
 * @param om_flash  The FLASH controller device address
 * @param status    Status register read buffer pointer
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_write_status_reg1(OM_FLASH_Type om_flash, uint8_t *status);

/**
 * @brief FLASH write status 1 register
 *        note: this function is volatile, it will lost after power down
 *
 * @param om_flash  The FLASH controller device address
 * @param status    Status register read buffer pointer
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_write_status_reg1_volatile(OM_FLASH_Type om_flash, uint8_t *status);

/**
 *******************************************************************************
 * @brief FLASH write status 2 register
 *
 * @param om_flash  The FLASH controller device address
 * @param status    Status register read buffer pointer
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_write_status_reg2(OM_FLASH_Type om_flash, uint8_t *status);

/**
 * @brief FLASH write status 2 register
 *        note: this function is volatile, it will lost after power down
 *
 * @param om_flash  The FLASH controller device address
 * @param status    Status register read buffer pointer
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_write_status_reg2_volatile(OM_FLASH_Type om_flash, uint8_t *status);

/**
 *******************************************************************************
 * @brief FLASH write config register
 *
 * @param om_flash  The FLASH controller device address
 * @param config    Config register read buffer pointer
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_write_config_reg(OM_FLASH_Type om_flash, uint8_t *config);

/**
 * @brief FLASH write config register
 *        note: this function is volatile, it will lost after power down
 *
 * @param om_flash  The FLASH controller device address
 * @param config    Config register read buffer pointer
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_write_config_reg_volatile(OM_FLASH_Type om_flash, uint8_t *config);

#if (RTE_FLASH1)

/**
 *******************************************************************************
 * @brief FLASH set read parameter
 *
 * @param om_flash  The FLASH controller device address
 * @param param     flash read parameter, see flash datasheet for details
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_read_param_set(OM_FLASH_Type om_flash, uint8_t param);

/**
 *******************************************************************************
 * @brief FLASH enable 4-Byte address mode
 *
 * @param om_flash  The FLASH controller device address
 * @param enable    Enable or disable
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_4byte_addr_enable(OM_FLASH_Type om_flash, uint8_t enable);

/**
 *******************************************************************************
 * @brief FLASH set a list node by configuration
 *        attention:
 *            the node only support read operation, write is not supported
 *
 * @param om_flash  The FLASH controller device address
 * @param node      The node to be set
 * @param cfg       The configuration of node
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_node_setup(OM_FLASH_Type om_flash, flash_list_node_t *node, flash_list_node_cfg_t *cfg);

/**
 *******************************************************************************
 * @brief FLASH list start, the list node only support read operation
 *
 * @param om_flash  The FLASH controller device address
 * @param list_head The node of list header
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_list_start(OM_FLASH_Type om_flash, flash_list_node_t *list_head);

#endif /* RTE_FALSH1 */

/**
 *******************************************************************************
 * @brief FLASH secure register erase
 *
 * @param om_flash  The FLASH controller device address
 * @param secure_register  Secure register(0, 1, 2...)
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_secure_register_erase(OM_FLASH_Type om_flash,
                                                  uint8_t secure_register);

/**
 *******************************************************************************
 * @brief FLASH secure register read
 *
 * @param om_flash  The FLASH controller device address
 * @param secure_register  Secure register(0, 1, 2...)
 * @param addr  Secure register address offset
 * @param data  Data to read
 * @param data_len  Data length
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_secure_register_read(OM_FLASH_Type om_flash,
                                                 uint8_t secure_register,
                                                 uint16_t addr,
                                                 uint8_t *data,
                                                 uint16_t data_len);

/**
 *******************************************************************************
 * @brief FLASH secure register write
 *
 * @param om_flash  The FLASH controller device address
 * @param secure_register  Secure register(0, 1, 2...)
 * @param addr  Secure register address offset
 * @param data  Data to write
 * @param data_len  Data length
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 */
extern om_error_t drv_flash_secure_register_write(OM_FLASH_Type om_flash,
                                                  uint8_t secure_register,
                                                  uint16_t addr,
                                                  uint8_t *data,
                                                  uint16_t data_len);

#if (RTE_FLASH0_REGISTER_CALLBACK || RTE_OSPI1_REGISTER_CALLBACK)
/**
 *******************************************************************************
 * @brief FLASH controller register callback
 *
 * @param om_flash  The FLASH controller device address
 * @param isr_cb    Callback
 *
 * @return          Error code, see@ref om_error_t
 *******************************************************************************
 **/
extern om_error_t drv_flash_register_isr_callback(OM_FLASH_Type om_flash, drv_isr_callback_t isr_cb);
#endif

#ifdef __cplusplus
}
#endif

#endif  /* RTE_FLASH0 || RTE_FLASH1 */
#endif  /* __drv_flash_H */


/** @} */
