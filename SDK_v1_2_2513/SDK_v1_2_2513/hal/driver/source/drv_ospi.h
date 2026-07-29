/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup OSPI OSPI
 * @ingroup  DRIVER
 * @brief    OSPI driver For External Memory
 * @details  OSPI driver For External Memory

 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_OSPI_H
#define __DRV_OSPI_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_OSPI1)
#include <stdint.h>
#include "om_error.h"
#include "om_utils.h"
#include "om_device.h"
#include "om_compiler.h"


#ifdef  __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
/**
 * There are four phase in a frame
 * 1. command phase
 * 2. address phase
 * 3. dummy cycle phase
 * 4. data phase
 * Except dummy cycle phase, each phase can be configured as
 * different bus width, which means parallel data number, and
 * bit count, which means data bit count number.
 * Dummy cycle phase just indicate the clock latency between
 * address phase and data phase.
 */
#define OSPI_FRAME_CONFIG(cmd_width, cmd_bits, addr_width, addr_bits, dummy, data_width)    \
    {                                                                                       \
        ((cmd_width & 0xFF)     << OSPI_SW_CFG0_P0_BUS_WIDTH_POS)      |                    \
        ((cmd_bits & 0xFF)      << OSPI_SW_CFG0_P0_BIT_CNT_POS)        |                    \
        ((addr_width & 0xFF)    << OSPI_SW_CFG0_P1_BUS_WIDTH_POS)      |                    \
        ((addr_bits & 0xFF)     << OSPI_SW_CFG0_P1_BIT_CNT_POS)        |                    \
        ((dummy & 0xFF)         << OSPI_SW_CFG0_DUMMY_CYCLE_POS),                           \
        ((data_width & 0xFF)    << OSPI_SW_CFG1_SDATA_BUS_WIDTH_POS)   |                    \
        ((4U & 0xFF)            << OSPI_SW_CFG1_BUF_WIDTH_BYTES_POS)   |                    \
        ((1U & 0x1)             << OSPI_SW_CFG1_FIXED_LIST_EN_POS)     |                    \
        ((0U & 0xFF)            << OSPI_SW_CFG1_HB_POS)                |                    \
        ((1U & 0xFF)            << OSPI_SW_CFG1_EN_POS)                                     \
    }


/*******************************************************************************
 * TYPEDEFS
 */
typedef enum {
    OSPI_MODE_0 = 0U,
    OSPI_MODE_1 = 1U,
    OSPI_MODE_2 = 2U,
    OSPI_MODE_3 = 3U,
} ospi_mode_t;

typedef enum {
    BUS_1BIT  = 0U,
    BUS_2BIT  = 1U,
    BUS_4BIT  = 2U,
    BUS_8BIT  = 3U,
    BUS_16BIT = 4U,
} ospi_bus_width_t;

typedef enum {
    OSPI_CONTROL_CRYPT_CTRL     = 0U,   /**< Crypt control for OSPI0/OSPI1, argu bit0 is encrypt enable, bit1 is decrypt enable; return OM_ERROR_OK */
    OSPI_CONTROL_CS_CONFIG      = 1U,   /**< CSN config, argu is ospi_config_t *; return OM_ERROR_OK */
    OSPI_CONTROL_SET_CLK_PARAM  = 2U,   /**< Set OSPI controller clock parameter. argu is ospi_clk_param_t. return OM_ERROR_OK */
    OSPI_CONTROL_GET_CLK_PARAM  = 3U,   /**< Get OSPI controller clock parameter. argu is ospi_clk_param_t. return OM_ERROR_OK */
    OSPI_CONTROL_SET_WR_OPCODE  = 4U,   /**< Set write opcode and frame config. argu is ospi_frame_t *,  return OM_ERROR_OK */
    OSPI_CONTROL_SET_RD_OPCODE  = 5U,   /**< Set read opcode and frame config. argu is ospi_frame_t *,  return OM_ERROR_OK */
} ospi_control_t;

typedef struct {
    uint8_t cs_active_high;             /**< chip select polarity when active is 1 or not */
    uint8_t cs_recover;                 /**< chip select recover time */
    uint8_t cs_hold;                    /**< chip select hold time */
    uint8_t cs_setup;                   /**< chip select setup time */
    uint8_t transp_head_ignore_cnt;     /**< transparent bus transfer packet number in one CS */
} ospi_cs_config_t;

typedef union {
    struct {
        uint32_t clk_div        : 8;    /**< clock divider */
        uint32_t delay_sample   : 2;    /**< delay sample point */
        uint32_t falling_sample : 1;    /**< sample at the falling edge */
        uint32_t reserve        : 5;    /**< reserved */
    };
    uint32_t param_all;
} ospi_clk_param_t;

typedef struct {
    uint32_t cmd;                       /**< read or write opcode, only valid [0-7] bit */
    uint32_t frame_cfg[2];              /**< frame config for cmd, see OSPI_FRAME_CONFIG define */
} ospi_frame_t;

typedef struct {
    ospi_cs_config_t *cs_config;        /**< cs config, NULL using default config */
    uint8_t read_opcode;                /**> read opcode, frame config see read_frame_cfg field */
    uint8_t write_opcode;               /**> write opcode, frame config see write_frame_cfg field */
    uint32_t read_frame_cfg[2];         /**< read frame config, read opcode see read_opcode field */
    uint32_t write_frame_cfg[2];        /**< write frame config, write opcode see write_opcode field */
    ospi_mode_t mode;                   /**< standard spi mode, see@ref ospi_mode_t */
    uint8_t page_cross_en;              /**< enable auto page cross */
    uint16_t page_size;                 /**< indicate page size, range in [1, 0xFFFF] */
    uint8_t clk_div;                    /**< clock divider */
    uint8_t sdr_async_en;               /**< SDR async mode enable, 0:sync 1:async */
    union {
        struct {
            uint8_t sdr_async_dly;      /**< SDR async mode delay sample point, used for sdr_async_en = 1 */
        } sdr_async;
        struct {
            uint8_t delay_sample;       /**< SDR sync mode delay sample point, range in [0, 3], used for sdr_async_en = 0 */
            uint8_t falling_sample;     /**< SDR sync mode sample at the clock falling edge, should be 0 or 1, used for sdr_async_en = 0 */
        } sdr_sync;
    } sample_cfg;
    uint8_t rw_data_width      : 2;     /**< width of the data read/write, 0: 8bit, 1: 16bit, 2:32bit */
    uint8_t opcode_bypass_en   : 1;     /**< bypass SPI_OPCODE register and use SPI_CMD_DATA0 to place more than 1 byte opcode */
    uint8_t encrypt_en         : 1;     /**< encrypt enable */
    uint8_t decrypt_en         : 1;     /**< decrypt enable */
    uint8_t is_normal_protocol : 1;     /**< is normal protocol or APS3208K */
    uint8_t is_4bytes_addr     : 1;     /**< 4byte address */
    uint8_t res                : 1;     /**< reserved */
} ospi_config_t;

typedef struct {
    uint32_t next_node_addr;            /**< the address of next node */
    uint32_t data_addr;                 /**< the address of internal memory, normally sram */
    uint32_t dev_addr;                  /**< the address of external memory, normally psram or flash */
    uint32_t shadow_cfg;                /**< the shadow config register val, including interrupt, read/write etc. */
} ospi_list_node_t;

typedef struct {
    uint32_t next_node_addr;            /**< the address of next node */
    uint32_t dev_addr;                  /**< the address of external memory */
    uint8_t *data;                      /**< the pointer of internal memory */
    uint32_t data_len;                  /**< the length of data to transfer */
    uint8_t cmd_type;                   /**< 0-read command, 1-write command */
    uint8_t node_int_en;                /**< enable an interrupt at the node transfer completed */
    uint8_t list_int_en;                /**< enable an interrupt at the end of list */
    uint8_t list_end;                   /**< 0-not end, 1-end */
} ospi_list_node_cfg_t;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief ospi initialization
 *
 * @param[in] om_ospi        Pointer to OSPI
 * @param[in] ospi_config    ospi config struct
 *
 *******************************************************************************
 */
extern void drv_ospi_init(OM_OSPI_Type *om_ospi, const ospi_config_t *ospi_config);

#if (RTE_OSPI1_REGISTER_CALLBACK)
/**
 *******************************************************************************
 * @brief ospi register interrupt callback
 *
 * @param[in] om_ospi        Pointer to OSPI
 * @param[in] isr_cb         callback function
 *
 *******************************************************************************
 */
extern void drv_ospi_register_isr_callback(OM_OSPI_Type *om_ospi, drv_isr_callback_t isr_cb);
#endif

/**
 *******************************************************************************
 * @brief Read data with OSPI
 *
 * @param[in] om_ospi        Pointer to ospi
 * @param[in] cmd            The command field, should be at most 64bit
 * @param[in] data           The address to store read data
 * @param[in] data_len       The data length
 * @return The error infomation
 *
 *******************************************************************************
 */
extern om_error_t drv_ospi_read(OM_OSPI_Type *om_ospi, uint32_t cmd[2], uint8_t *data, uint32_t data_len);

/**
 *******************************************************************************
 * @brief Read data with interrupt enabled
 *
 * @param[in] om_ospi        Pointer to ospi
 * @param[in] cmd            The command field, should be at most 64bit
 * @param[in] data           The address to store read data
 * @param[in] data_len       The data length
 * @return The error infomation
 *
 *******************************************************************************
 */
extern om_error_t drv_ospi_read_int(OM_OSPI_Type *om_ospi, uint32_t cmd[2], uint8_t *data, uint32_t data_len);

/**
 *******************************************************************************
 * @brief Write data OSPI
 *
 * @param[in] om_ospi        Pointer to ospi
 * @param[in] cmd            The command field, should be at most 64bit
 * @param[in] data           The address to store read data
 *                           Attention: the type must be volatile to avoid compiler optimization
 *                                      when Link Time Optimization is enabled,
 *                                      if the data is from heap, it should be sure to be volatile,
 *                                      for example: volatile uint8_t *data = om_mem_malloc(X, X);
 * @param[in] data_len       The data length
 * @return The error infomation
 *
 *******************************************************************************
 */
extern om_error_t drv_ospi_write(OM_OSPI_Type *om_ospi,
                                 uint32_t cmd[2],
                                 volatile uint8_t *data,
                                 uint32_t data_len);

/**
 *******************************************************************************
 * @brief Write data with interrupt enabled
 *
 * @param[in] om_ospi        Pointer to ospi
 * @param[in] cmd            The command field, should be at most 64bit
 * @param[in] data           The address to store read data
 *                           Attention: the type must be volatile to avoid compiler optimization
 *                                      when Link Time Optimization is enabled,
 *                                      if the data is from heap, it should be sure to be volatile,
 *                                      for example: volatile uint8_t *data = om_mem_malloc(X, X);
 * @param[in] data_len       The data length
 * @return The error infomation
 *
 *******************************************************************************
 */
extern om_error_t drv_ospi_write_int(OM_OSPI_Type *om_ospi,
                                     uint32_t cmd[2],
                                     volatile uint8_t *data,
                                     uint32_t data_len);

/**
 *******************************************************************************
 * @brief Store OSPI status before sleep.
 *******************************************************************************
 */
extern void drv_ospi_store(void);

/**
 *******************************************************************************
 * @brief Restore OSPI status after wake from sleep.
 *******************************************************************************
 */
extern void drv_ospi_restore(void);

/**
 *******************************************************************************
 * @brief Control OSPI interface.
 *
 * @param[in] om_ospi        Pointer to OSPI
 * @param[in] control        Operation
 * @param[in] argu           Operation argument
 *
 * @return status:           Control status, always return OM_ERROR_OK
 *******************************************************************************
 */
__STATIC_INLINE void *drv_ospi_control(OM_OSPI_Type *om_ospi, ospi_control_t control, void *argu)
{
    switch (control) {
        case OSPI_CONTROL_CRYPT_CTRL:
            if ((uint32_t)argu & OSPI_SEC_CTRL_ENC_MASK) {
                DRV_RCC_CLOCK_ENABLE(RCC_CLK_AES, 1U);
            }
            om_ospi->SEC_CTRL = (uint32_t)argu;
            break;
        case OSPI_CONTROL_CS_CONFIG:
            OM_ASSERT(argu);
            do {
                ospi_cs_config_t *cs_config;
                cs_config = (ospi_cs_config_t *)argu;
                om_ospi->CFG[0].CS_CFG = ((cs_config)->cs_active_high << OSPI_CS_CFG_CSP_POS)
                                       | ((cs_config)->transp_head_ignore_cnt << OSPI_CS_CFG_TRSP_IGNORE_CNT_POS)
                                       | ((cs_config)->cs_setup << OSPI_CS_CFG_CS_SETUP_POS)
                                       | ((cs_config)->cs_hold << OSPI_CS_CFG_CS_HOLD_POS)
                                       | ((cs_config)->cs_recover << OSPI_CS_CFG_CS_RECOVER_POS);
            } while(0);
            break;
        case OSPI_CONTROL_SET_CLK_PARAM:
            OM_ASSERT(argu);
            do {
                uint32_t cfg;
                uint8_t cpol, cpha;
                ospi_clk_param_t clk_param;
                uint8_t falling_sample, bypass_clk_div_en;

                clk_param.param_all = (uint32_t)argu;
                cfg = om_ospi->CFG[0].SPI_CFG;
                cpol = (cfg & OSPI_CFG_CPOL_MASK) ? 1U : 0U;
                cpha = (cfg & OSPI_CFG_CPHA_MASK) ? 1U : 0U;
                falling_sample = ((clk_param.clk_div < 2U) || (cpol != cpha)) ? 0U : clk_param.falling_sample;
                bypass_clk_div_en = (clk_param.clk_div < 2) ? 1U : 0U;
                register_set(&om_ospi->CFG[0].SPI_CFG, MASK_4REG(OSPI_CFG_CLK_DIV, clk_param.clk_div,
                                                                 OSPI_CFG_BPC,     bypass_clk_div_en,
                                                                 OSPI_CFG_DLY,     clk_param.delay_sample,
                                                                 OSPI_CFG_FDS,     falling_sample));
            } while(0);
            break;
        case OSPI_CONTROL_GET_CLK_PARAM:
            OM_ASSERT(argu);
            do {
                ospi_clk_param_t *clk_param = (ospi_clk_param_t *)argu;
                uint32_t cfg;
                cfg = om_ospi->CFG[0].SPI_CFG;
                clk_param->clk_div = (cfg & OSPI_CFG_CLK_DIV_MASK) >> OSPI_CFG_CLK_DIV_POS;
                clk_param->delay_sample = (cfg & OSPI_CFG_DLY_MASK) >> OSPI_CFG_DLY_POS;
                clk_param->falling_sample = (cfg & OSPI_CFG_FDS_MASK) >> OSPI_CFG_FDS_POS;
            } while(0);
            break;
        case OSPI_CONTROL_SET_RD_OPCODE:
            OM_ASSERT(argu);
            do {
                ospi_frame_t *frame = (ospi_frame_t *)argu;
                register_set(&om_ospi->OPCODE, MASK_1REG(OSPI_OPCODE_RD0, frame->cmd));
                om_ospi->SW_CFG0 = frame->frame_cfg[0];
                om_ospi->SW_CFG1 = frame->frame_cfg[1];
            } while(0);
            break;
        case OSPI_CONTROL_SET_WR_OPCODE:
            OM_ASSERT(argu);
            do {
                ospi_frame_t *frame = (ospi_frame_t *)argu;
                register_set(&om_ospi->OPCODE, MASK_1REG(OSPI_OPCODE_WR0, frame->cmd));
                om_ospi->SW_WR_CFG0 = frame->frame_cfg[0];
                om_ospi->SW_WR_CFG1 = frame->frame_cfg[1];
            } while(0);
            break;
        default:
            break;
    }

    return (void *)OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Set a list node by configuration
 *
 * @param[in] om_ospi       Pointer to ospi
 * @param[in] node          The node to be set
 * @param[in] cfg           The configuration of node
 *
 *******************************************************************************
 */
extern void drv_ospi_node_setup(OM_OSPI_Type *om_ospi, ospi_list_node_t *node, ospi_list_node_cfg_t *cfg);

/**
 *******************************************************************************
 * @brief Start list transfer. Shall be lock code if transfer from in XIP flash
 *
 * @param[in] om_ospi       Pointer to ospi
 * @param[in] list_head     The first list node pointer
 *******************************************************************************
 */
__STATIC_INLINE void drv_ospi_list_start(OM_OSPI_Type *om_ospi, ospi_list_node_t *list_head)
{
    om_ospi->LIST_LOAD_ADDR = (uint32_t)list_head;
    om_ospi->SW_CFG1 |= OSPI_SW_CFG1_LLEN_MASK;
}

/**
 *******************************************************************************
 * @brief ospi set transfer rate
 *
 * @param[in] om_ospi          Pointer to OSPI
 * @param[in] data_width_16b   single transfer rate or double transfer rate
 *******************************************************************************
 */
__STATIC_INLINE void drv_ospi_set_transfer_rate(OM_OSPI_Type *om_ospi,
                                                uint8_t data_width_16b)
{
    register_set(&om_ospi->CFG[0].SPI_CFG, MASK_1REG(OSPI_CFG_WIDTH, data_width_16b ? 1U : 0U));
}

/**
 *******************************************************************************
 * @brief ospi set secure configuration
 *
 * @param[in] om_ospi        Pointer to OSPI
 * @param[in] cfg            Secure configuration
 *
 *******************************************************************************
 */
__STATIC_INLINE void drv_ospi_sec_cfg_set(OM_OSPI_Type *om_ospi, uint32_t cfg)
{
    om_ospi->SEC_CTRL = cfg;
}

/**
 *******************************************************************************
 * @brief ospi get secure configuration
 *
 * @param[in] om_ospi        Pointer to OSPI
 * @param[in] cfg            Secure configuration
 *
 *******************************************************************************
 */
__STATIC_INLINE void drv_ospi_sec_cfg_get(OM_OSPI_Type *om_ospi, uint32_t *cfg)
{
    *cfg = om_ospi->SEC_CTRL;
}

/**
 *******************************************************************************
 * @brief ospi set read configuration
 *
 * @param[in] om_ospi        Pointer to OSPI
 * @param[in] cfg            Read configuration
 *
 *******************************************************************************
 */
__STATIC_INLINE void drv_ospi_read_cfg_set(OM_OSPI_Type *om_ospi, const uint32_t cfg[2])
{
    om_ospi->SW_CFG0 = cfg[0];
    om_ospi->SW_CFG1 = cfg[1];
}

/**
 *******************************************************************************
 * @brief ospi get read configuration
 *
 * @param[in] om_ospi        Pointer to OSPI
 * @param[in] cfg            Read configuration
 *
 *******************************************************************************
 */
__STATIC_INLINE void drv_ospi_read_cfg_get(OM_OSPI_Type *om_ospi, uint32_t cfg[2])
{
    cfg[0] = om_ospi->SW_CFG0;
    cfg[1] = om_ospi->SW_CFG1;
}

/**
 *******************************************************************************
 * @brief ospi set write configuration
 *
 * @param[in] om_ospi        Pointer to OSPI
 * @param[in] cfg            Write configuration
 *
 *******************************************************************************
 */
__STATIC_INLINE void drv_ospi_write_cfg_set(OM_OSPI_Type *om_ospi, const uint32_t cfg[2])
{
    om_ospi->SW_WR_CFG0 = cfg[0];
    om_ospi->SW_WR_CFG1 = cfg[1];
}

/**
 *******************************************************************************
 * @brief ospi get write configuration
 *
 * @param[in] om_ospi        Pointer to OSPI
 * @param[in] cfg            Write configuration
 *
 *******************************************************************************
 */
__STATIC_INLINE void drv_ospi_write_cfg_get(OM_OSPI_Type *om_ospi, uint32_t cfg[2])
{
    cfg[0] = om_ospi->SW_WR_CFG0;
    cfg[1] = om_ospi->SW_WR_CFG1;
}

/**
 *******************************************************************************
 * @brief ospi set read operate code for transparent read
 *
 * @param[in] om_ospi       Pointer to OSPI
 * @param[in] read_frame    Read opcode pointer
 *******************************************************************************
 */
__STATIC_INLINE void drv_ospi_read_frame_set(OM_OSPI_Type *om_ospi,
                                             const ospi_frame_t *read_frame)
{
    OM_CRITICAL_BEGIN();
    register_set(&om_ospi->OPCODE, MASK_1REG(OSPI_OPCODE_RD0, read_frame->cmd));
    drv_ospi_read_cfg_set(om_ospi, read_frame->frame_cfg);
    OM_CRITICAL_END();
}

/**
 *******************************************************************************
 * @brief ospi set write operate code for transparent write
 *
 * @param[in] om_ospi        Pointer to OSPI
 * @param[in] write_frame    Read opcode pointer
 *
 *******************************************************************************
 */
__STATIC_INLINE void drv_ospi_write_frame_set(OM_OSPI_Type *om_ospi,
                                              const ospi_frame_t *write_frame)
{
    OM_CRITICAL_BEGIN();
    register_set(&om_ospi->OPCODE, MASK_1REG(OSPI_OPCODE_WR0, write_frame->cmd));
    drv_ospi_write_cfg_set(om_ospi, write_frame->frame_cfg);
    OM_CRITICAL_END();
}

/**
 *******************************************************************************
 * @brief ospi set addr length, 3 byte or 4 byte
 *
 * @param[in] om_ospi        Pointer to OSPI
 * @param[in] addr_4byte     1 for 4 byte address, 0 for 3 byte address
 *
 *******************************************************************************
 */
__STATIC_INLINE void drv_ospi_set_addr_4byte(OM_OSPI_Type *om_ospi, uint8_t addr_4byte)
{
    register_set(&om_ospi->MEM_TYPE0, MASK_1REG(OSPI_MEM_TYPE_ADDR_WIDTH, addr_4byte));
}

/**
 *******************************************************************************
 * @brief ospi set crypt control
 *
 * @param[in] om_ospi        Pointer to OSPI
 * @param[in] encrypt_en     encrypt enable
 * @param[in] decrypt_en     decrypt enable
 *
 *******************************************************************************
 */
__STATIC_INLINE void drv_ospi_set_crypt_control(OM_OSPI_Type *om_ospi, uint8_t encrypt_en, uint8_t decrypt_en)
{
    om_ospi->SEC_CTRL = (encrypt_en ? OSPI_SEC_CTRL_ENC_MASK : 0U) |
                        (decrypt_en ? OSPI_SEC_CTRL_DEC_MASK : 0U);
}

/**
 *******************************************************************************
 * @brief Set comp delay and clock division
 *
 * @param om_ospi           Pointer to OSPI
 * @param comp_delay        The combination of delay sample and falling sample,
 *                          the value is 0, 5, 1, 6, 2, 7, 3
 * @param div               The clock division, should be even
 *
 *******************************************************************************
 */
__STATIC_INLINE void drv_ospi_set_comp_delay_div(OM_OSPI_Type *om_ospi, uint8_t comp_delay, uint8_t div)
{
    register_set(&om_ospi->CFG[0].SPI_CFG, MASK_3REG(OSPI_CFG_CLK_DIV, div,
                                                     OSPI_CFG_BPC, (div < 2) ? 1 : 0,
                                                     OSPI_CFG_DLY_COMP, comp_delay));
}

/**
 *******************************************************************************
 * @brief get ospi busy
 *
 * @param[in] om_ospi       pointer to ospi
 * @return 1-busy 0-idle
 *
 *******************************************************************************
 */
__STATIC_INLINE uint8_t drv_ospi_is_busy(OM_OSPI_Type  *om_ospi)
{
    return !!(om_ospi->STATUS & OSPI_STATUS_BUSY_MASK);
}

/**
 *******************************************************************************
 * @brief ospi interrupt service routine
 *
 * @param[in] om_ospi       pointer to OSPI
 *
 *******************************************************************************
 */
extern void drv_ospi_isr(OM_OSPI_Type *om_ospi);

#ifdef  __cplusplus
}
#endif

#endif  /* (RTE_OSPI1) */
#endif  /* __DRV_OSPI_H */


/** @} */
