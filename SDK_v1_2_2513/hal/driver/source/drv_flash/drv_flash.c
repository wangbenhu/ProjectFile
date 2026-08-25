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
 * @brief    INTERNAL FLASH controller driver
 * @details  INTERNAL FLASH controller driver
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
#include "RTE_driver.h"
#if (RTE_FLASH0 || RTE_FLASH1)
#include "om_driver.h"


/*******************************************************************************
 * MACROS
 */
#ifndef __RAM_BASE
#define __RAM_BASE 0x20000000U
#endif
#define CHECK_DATA_ADDR(data_ptr)                                                       \
            OM_ASSERT(((uint32_t)data_ptr >= OM_MEM_RAM_BASE &&                         \
                       (uint32_t)data_ptr < (OM_MEM_RAM_BASE + OM_MEM_RAM_SIZE)) ||     \
                      ((uint32_t)data_ptr >= __RAM_BASE &&                              \
                       (uint32_t)data_ptr < (__RAM_BASE + OM_MEM_RAM_SIZE)))


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
om_error_t drv_flash_init(OM_FLASH_Type om_flash, const flash_config_t *config)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_init((OM_SF_Type *)om_flash, config);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_init((OM_OSPI_Type *)om_flash, config);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_read_id(OM_FLASH_Type om_flash, flash_id_t *id)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_read_id((OM_SF_Type *)om_flash, id);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_read_id((OM_OSPI_Type *)om_flash, id);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_read_uid(OM_FLASH_Type om_flash, uint8_t *uid, uint32_t len)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_read_uid((OM_SF_Type *)om_flash, uid, len);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_read_uid((OM_OSPI_Type *)om_flash, uid, len);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_id_get(OM_FLASH_Type om_flash, flash_id_t *id)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_id_get((OM_SF_Type *)om_flash, id);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_id_get((OM_OSPI_Type *)om_flash, id);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_read(OM_FLASH_Type om_flash, uint32_t addr, uint8_t *data, uint32_t data_len)
{
    CHECK_DATA_ADDR(data);
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_read((OM_SF_Type *)om_flash, addr, data, data_len);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_read((OM_OSPI_Type *)om_flash, addr, data, data_len);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_read_int(OM_FLASH_Type om_flash, uint32_t addr, uint8_t *data, uint32_t data_len)
{
    CHECK_DATA_ADDR(data);
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_read_int((OM_SF_Type *)om_flash, addr, data, data_len);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_read_int((OM_OSPI_Type *)om_flash, addr, data, data_len);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_write(OM_FLASH_Type om_flash,
                           uint32_t addr,
                           volatile uint8_t *data,
                           uint32_t data_len)
{
    CHECK_DATA_ADDR(data);
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_write((OM_SF_Type *)om_flash, addr, data, data_len);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_write((OM_OSPI_Type *)om_flash, addr, data, data_len);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_write_int_start(OM_FLASH_Type om_flash,
                                     uint32_t addr,
                                     volatile uint8_t *data,
                                     uint32_t data_len)
{
    CHECK_DATA_ADDR(data);
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_write_int_start((OM_SF_Type *)om_flash, addr, data, data_len);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_write_int_start((OM_OSPI_Type *)om_flash, addr, data, data_len);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_write_int_status_get(OM_FLASH_Type om_flash, uint8_t *is_wip)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_write_int_status_get((OM_SF_Type *)om_flash, is_wip);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_write_int_status_get((OM_OSPI_Type *)om_flash, is_wip);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_write_int_continue(OM_FLASH_Type om_flash)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_write_int_continue((OM_SF_Type *)om_flash);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_write_int_continue((OM_OSPI_Type *)om_flash);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_erase(OM_FLASH_Type om_flash, uint32_t addr, flash_erase_t erase_type)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_erase((OM_SF_Type *)om_flash, addr, erase_type);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_erase((OM_OSPI_Type *)om_flash, addr, erase_type);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_erase_start(OM_FLASH_Type om_flash, uint32_t addr, flash_erase_t erase_type)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_erase_start((OM_SF_Type *)om_flash, addr, erase_type);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_erase_start((OM_OSPI_Type *)om_flash, addr, erase_type);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

uint8_t drv_flash_erase_is_done(OM_FLASH_Type om_flash)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_erase_is_done((OM_SF_Type *)om_flash);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_erase_is_done((OM_OSPI_Type *)om_flash);
    } else
    #endif
    return 0;
}

om_error_t drv_flash_write_protect_set(OM_FLASH_Type om_flash, flash_protect_t protect)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_write_protect_set((OM_SF_Type *)om_flash, protect);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_write_protect_set((OM_OSPI_Type *)om_flash, protect);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_write_protect_set_volatile(OM_FLASH_Type om_flash, flash_protect_t protect)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_write_protect_set_volatile((OM_SF_Type *)om_flash, protect);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_write_protect_set_volatile((OM_OSPI_Type *)om_flash, protect);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_read_cmd_set(OM_FLASH_Type om_flash, flash_read_t read_cmd)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_read_cmd_set((OM_SF_Type *)om_flash, read_cmd);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_read_cmd_set((OM_OSPI_Type *)om_flash, read_cmd);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_write_cmd_set(OM_FLASH_Type om_flash, flash_write_t write_cmd)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_write_cmd_set((OM_SF_Type *)om_flash, write_cmd);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_write_cmd_set((OM_OSPI_Type *)om_flash, write_cmd);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_quad_enable(OM_FLASH_Type om_flash, bool enable)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_quad_enable((OM_SF_Type *)om_flash, enable);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_quad_enable((OM_OSPI_Type *)om_flash, enable);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_quad_enable_volatile(OM_FLASH_Type om_flash, bool enable)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_quad_enable_volatile((OM_SF_Type *)om_flash, enable);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_quad_enable_volatile((OM_OSPI_Type *)om_flash, enable);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_reset(OM_FLASH_Type om_flash)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_reset((OM_SF_Type *)om_flash);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_reset((OM_OSPI_Type *)om_flash);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_encrypt_enable(OM_FLASH_Type om_flash, uint8_t enable)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_encrypt_enable((OM_SF_Type *)om_flash, enable);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_encrypt_enable((OM_OSPI_Type *)om_flash, enable);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_read_status_reg1(OM_FLASH_Type om_flash, uint8_t *status)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_read_status_reg1((OM_SF_Type *)om_flash, status);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_read_status_reg1((OM_OSPI_Type *)om_flash, status);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_read_status_reg2(OM_FLASH_Type om_flash, uint8_t *status)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_read_status_reg2((OM_SF_Type *)om_flash, status);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_read_status_reg2((OM_OSPI_Type *)om_flash, status);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_read_config_reg(OM_FLASH_Type om_flash, uint8_t *status)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_read_config_reg((OM_SF_Type *)om_flash, status);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_read_config_reg((OM_OSPI_Type *)om_flash, status);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_write_status(OM_FLASH_Type om_flash, uint8_t status[2])
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_write_status((OM_SF_Type *)om_flash, status, 2);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_write_status((OM_OSPI_Type *)om_flash, status);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_write_status_volatile(OM_FLASH_Type om_flash, uint8_t status[2])
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_write_status_volatile((OM_SF_Type *)om_flash, status, 2);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_write_status_volatile((OM_OSPI_Type *)om_flash, status);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_modifiy_status_bits(OM_FLASH_Type om_flash, uint8_t status[2], uint8_t mask[2])
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_modifiy_status_bits((OM_SF_Type *)om_flash, status, mask);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_modifiy_status_bits((OM_OSPI_Type *)om_flash, status, mask);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_modifiy_status_bits_volatile(OM_FLASH_Type om_flash, uint8_t status[2], uint8_t mask[2])
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_modifiy_status_bits_volatile((OM_SF_Type *)om_flash, status, mask);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_modifiy_status_bits_volatile((OM_OSPI_Type *)om_flash, status, mask);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_write_status_reg1(OM_FLASH_Type om_flash, uint8_t *status)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_write_status((OM_SF_Type *)om_flash, status, 1);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_write_status_reg1((OM_OSPI_Type *)om_flash, status);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_write_status_reg1_volatile(OM_FLASH_Type om_flash, uint8_t *status)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_write_status_volatile((OM_SF_Type *)om_flash, status, 1);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_write_status_reg1_volatile((OM_OSPI_Type *)om_flash, status);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_write_status_reg2(OM_FLASH_Type om_flash, uint8_t *status)
{
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_write_status_reg2((OM_OSPI_Type *)om_flash, status);
    }
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_write_status_reg2_volatile(OM_FLASH_Type om_flash, uint8_t *status)
{
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_write_status_reg2_volatile((OM_OSPI_Type *)om_flash, status);
    }
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_write_config_reg(OM_FLASH_Type om_flash, uint8_t *config)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_write_config_reg((OM_SF_Type *)om_flash, config);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_write_config_reg((OM_OSPI_Type *)om_flash, config);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_write_config_reg_volatile(OM_FLASH_Type om_flash, uint8_t *config)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_write_config_reg_volatile((OM_SF_Type *)om_flash, config);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_write_config_reg_volatile((OM_OSPI_Type *)om_flash, config);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

#if (RTE_FLASH1)
om_error_t drv_flash_4byte_addr_enable(OM_FLASH_Type om_flash, uint8_t enable)
{
    if (om_flash == OM_FLASH1) {
        return drv_oflash_4byte_addr_enable((OM_OSPI_Type *)om_flash, enable);
    }
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_read_param_set(OM_FLASH_Type om_flash, uint8_t param)
{
    if (om_flash == OM_FLASH1) {
        return drv_oflash_read_param_set((OM_OSPI_Type *)om_flash, param);
    }
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_node_setup(OM_FLASH_Type om_flash, flash_list_node_t *node, flash_list_node_cfg_t *cfg)
{
    if (om_flash == OM_FLASH1) {
        drv_oflash_node_setup((OM_OSPI_Type *)om_flash, node, cfg);
        return OM_ERROR_OK;
    }
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_list_start(OM_FLASH_Type om_flash, flash_list_node_t *list_head)
{
    if (om_flash == OM_FLASH1) {
        return drv_oflash_list_start((OM_OSPI_Type *)om_flash, list_head);
    }
    return OM_ERROR_UNSUPPORTED;
}
#endif /* RTE_FLASH1 */

om_error_t drv_flash_secure_register_erase(OM_FLASH_Type om_flash, uint8_t secure_register)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_secure_register_erase((OM_SF_Type *)om_flash, secure_register);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_secure_register_erase((OM_OSPI_Type *)om_flash, secure_register);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_secure_register_read(OM_FLASH_Type om_flash, uint8_t secure_register, uint16_t addr, uint8_t *data, uint16_t data_len)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_secure_register_read((OM_SF_Type *)om_flash, secure_register, addr, data, data_len);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_secure_register_read((OM_OSPI_Type *)om_flash, secure_register, addr, data, data_len);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_secure_register_write(OM_FLASH_Type om_flash, uint8_t secure_register, uint16_t addr, uint8_t *data, uint16_t data_len)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_secure_register_write((OM_SF_Type *)om_flash, secure_register, addr, data, data_len);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_secure_register_write((OM_OSPI_Type *)om_flash, secure_register, addr, data, data_len);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

om_error_t drv_flash_register_isr_callback(OM_FLASH_Type om_flash, drv_isr_callback_t isr_cb)
{
    #if (RTE_FLASH0)
    if (om_flash == OM_FLASH0) {
        return drv_iflash_register_isr_callback((OM_SF_Type *)om_flash, isr_cb);
    } else
    #endif
    #if (RTE_FLASH1)
    if (om_flash == OM_FLASH1) {
        return drv_oflash_register_isr_callback((OM_OSPI_Type *)om_flash, isr_cb);
    } else
    #endif
    return OM_ERROR_UNSUPPORTED;
}

#endif /* RTE_FLASH0 || RTE_FLASH1 */
