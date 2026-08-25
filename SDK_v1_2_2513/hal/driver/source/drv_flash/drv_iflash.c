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
#if (RTE_FLASH0)
#include "om_device.h"
#include "om_driver.h"
#include <stddef.h>


/*******************************************************************************
 * MACROS
 */
#if RTE_FLASH0_XIP
#define __IFLASH_CODE               __RAM_CODE
#else
#define __IFLASH_CODE
#endif

#ifndef RTE_FLASH_SAFETY_ENHANCE_DISABLE
#define RTE_FLASH_SAFETY_ENHANCE_DISABLE 0
#define IFLASH_PROTECT_HEAD_LEN          0x3000
#endif

#define POWER_UP_RETRY_CNT          2

#define CMD_READ                    1
#define CMD_WRITE                   2
#define CMD_BITS                    8
#define CMD_CFG_ITEM(value, item)                                                       \
            ((((uint32_t)(value)) << (item##_POS)) & (item##_MASK))
#define CMD_CFG(cmd_code, rw_sel, addr_bits, dummy_bits)                                \
            {                                                                           \
                cmd_code,                                                               \
                CMD_CFG_ITEM(rw_sel, SF_COMMAND_RW_SELECT)                          |   \
                CMD_CFG_ITEM(0, SF_COMMAND_CHIP_SELECT)                             |   \
                CMD_CFG_ITEM(0, SF_COMMAND_KEEP_CS)                                 |   \
                CMD_CFG_ITEM(CMD_BITS + addr_bits + dummy_bits, SF_COMMAND_CMD_BITS)|   \
                CMD_CFG_ITEM(0, SF_COMMAND_DATA_BYTES)                                  \
            }
#define CMD_FRAME_SET(pframe, cfg, addr)                                                \
            do {                                                                        \
                ((cmd_frame_t *)(pframe))->cmd_cfg = ((cmd_cfg_t)cfg).cmd_cfg;          \
                ((cmd_frame_t *)(pframe))->cmd_data[0] =                                \
                            ((((cmd_cfg_t)cfg).cmd_code << 24) | (addr & 0xFFFFFF));    \
                ((cmd_frame_t *)(pframe))->cmd_data[1] = 0;                             \
            } while (0)

// Flash commands defines, |cmd, read or write, address bits, dummy bits|
#define FLASH_READ_ID_CFG               CMD_CFG(0x9FU, CMD_READ , 0 , 0)
#define FLASH_READ_CFG                  CMD_CFG(0x03U, CMD_READ , 24, 0)
#define FLASH_FAST_READ_CFG             CMD_CFG(0x0BU, CMD_READ , 24, 8)
#define FLASH_FAST_READ_DO_CFG          CMD_CFG(0x3BU, CMD_READ , 24, 8)
#define FLASH_FAST_READ_DIO_CFG         CMD_CFG(0xBBU, CMD_READ , 32, 0)
#define FLASH_FAST_READ_QO_CFG          CMD_CFG(0x6BU, CMD_READ , 24, 8)
#define FLASH_FAST_READ_QIO_CFG         CMD_CFG(0xEBU, CMD_READ , 32, 4)
#define FLASH_PROGRAM_CFG               CMD_CFG(0x02U, CMD_WRITE, 24, 0)
#define FLASH_SECTOR_ERASE_4K_CFG       CMD_CFG(0x20U, CMD_READ , 24, 0)
#define FLASH_BLOCK_ERASE_32K_CFG       CMD_CFG(0x52U, CMD_READ , 24, 0)
#define FLASH_BLOCK_ERASE_64K_CFG       CMD_CFG(0xD8U, CMD_READ , 24, 0)
#define FLASH_CHIP_ERASE_CFG            CMD_CFG(0x60U, CMD_READ , 0 , 0)
#define FLASH_READ_STATUS_REG_1_CFG     CMD_CFG(0x05U, CMD_READ , 0 , 0)
#define FLASH_READ_STATUS_REG_2_CFG     CMD_CFG(0x35U, CMD_READ , 0 , 0)
#define FLASH_WRITE_STATUS_REGS_CFG     CMD_CFG(0x01U, CMD_WRITE, 0 , 0)
#define FLASH_READ_CONFIG_REG_CFG       CMD_CFG(0x15U, CMD_READ , 0 , 0)
#define FLASH_WRITE_CONFIG_REG_CFG      CMD_CFG(0x11U, CMD_WRITE, 0 , 0)
#define FLASH_WRITE_ENABLE_VSR_CFG      CMD_CFG(0x50U, CMD_READ , 0 , 0)
#define FLASH_WRITE_ENABLE_CFG          CMD_CFG(0x06U, CMD_READ , 0 , 0)
#define FLASH_WRITE_DISABLE_CFG         CMD_CFG(0x04U, CMD_READ , 0 , 0)
#define FLASH_RESET_ENABLE_CFG          CMD_CFG(0x66U, CMD_READ , 0 , 0)
#define FLASH_RESET_CFG                 CMD_CFG(0x99U, CMD_READ , 0 , 0)
#define FLASH_DEEP_POWERDOWN_CFG        CMD_CFG(0xB9U, CMD_READ , 0 , 0)
#define FLASH_RELEASE_POWERDOWN_CFG     CMD_CFG(0xABU, CMD_READ , 0 , 0)
#define FLASH_SUSPEND_CFG               CMD_CFG(0x75U, CMD_READ , 0 , 0)
#define FLASH_RESUME_CFG                CMD_CFG(0x7AU, CMD_READ , 0 , 0)

// used for EXT_FRAME_CONFIG xxx_width
#define EXT_1BIT                        0
#define EXT_2BIT                        1
#define EXT_4BIT                        2

// Extend cmd frame config, the cmd is build by software, not use sf internal cmd
#define EXT_FRAME_CONFIG(cmd, rw_sel, cmd_width, cmd_bits, addr_width, addr_bits, dummy, data_width)  \
    {                                                                                                 \
        cmd,                                                                                          \
        rw_sel,                                                                                       \
        {                                                                                             \
            ((cmd_width << SF_SW_SPI_CFG0_P0_WIDTH_POS) & SF_SW_SPI_CFG0_P0_WIDTH_MASK)       |       \
            ((cmd_bits << SF_SW_SPI_CFG0_P0_BITS_POS) & SF_SW_SPI_CFG0_P0_BITS_MASK)          |       \
            ((addr_width << SF_SW_SPI_CFG0_P1_WIDTH_POS) & SF_SW_SPI_CFG0_P1_WIDTH_MASK)      |       \
            ((addr_bits << SF_SW_SPI_CFG0_P1_BITS_POS) & SF_SW_SPI_CFG0_P1_BITS_MASK)         |       \
            ((dummy << SF_SW_SPI_CFG0_DUMMY_CNT_POS) & SF_SW_SPI_CFG0_DUMMY_CNT_MASK),                \
            ((data_width << SF_SW_SPI_CFG1_DATA_WIDTH_POS) & SF_SW_SPI_CFG1_DATA_WIDTH_MASK)  |       \
            ((4U << SF_SW_SPI_CFG1_BUF_BYTES_POS) & SF_SW_SPI_CFG1_BUF_BYTES_MASK)            |       \
            ((1U << SF_SW_SPI_CFG1_SW_CFG_EN_POS) & SF_SW_SPI_CFG1_SW_CFG_EN_MASK)                    \
        }                                                                                             \
    }

#define FLASH_SECURE_REG_ERASE_EXT_CFG  EXT_FRAME_CONFIG(0x44U, CMD_READ, EXT_1BIT, 8, EXT_1BIT, 24, 0, EXT_1BIT)
#define FLASH_SECURE_REG_READ_EXT_CFG   EXT_FRAME_CONFIG(0x48U, CMD_READ, EXT_1BIT, 8, EXT_1BIT, 24, 8, EXT_1BIT)
#define FLASH_SECURE_REG_WRITE_EXT_CFG  EXT_FRAME_CONFIG(0x42U, CMD_WRITE,EXT_1BIT, 8, EXT_1BIT, 24, 0, EXT_1BIT)
#define FLASH_READ_UID_EXT_CFG          EXT_FRAME_CONFIG(0x4BU, CMD_READ, EXT_1BIT, 8, EXT_1BIT, 24, 8, EXT_1BIT)

/*******************************************************************************
 * TYPE DEFINITIONS
 */
typedef struct {
    uint32_t cmd_cfg;
    uint32_t cmd_data[2];
} cmd_frame_t;

typedef struct {
    uint32_t cmd_code;
    uint32_t cmd_cfg;
} cmd_cfg_t;

typedef struct {
    uint8_t ext_cmd_code;
    uint8_t rw_sel;
    uint32_t ext_cmd_cfg[2];
} ext_cmd_frame_t;

typedef struct {
    drv_isr_callback_t isr_cb;          /*!< Interrupt callback */
    uint32_t addr;                      /*!< Flash address to write */
    uint8_t *data;                      /*!< Data buffer to write */
    uint32_t data_len;                  /*!< Total length of data to write */
    uint32_t data_cnt;                  /*!< Length of data written */
    cmd_cfg_t read_id_cmd_cfg;          /*!< Read ID frame configuration */
    flash_delay_info_t delay_info;      /*!< Delay information */
    flash_id_t id;                      /*!< Flash id, see@flash_id_t */
    flash_trans_state_t write_int_s;    /*!< Write state, used for write interrupt */
    flash_read_t read_cmd;              /*!< Flash cmd used in reading data */
    flash_write_t write_cmd;            /*!< Flash cmd used in writing data */
    cmd_frame_t dp_enter_frame;         /*!< Enter deep powerdown cmd frame */
    cmd_frame_t dp_exit_frame;          /*!< Exit deep powerdown cmd frame */
    flash_state_t state;                /*!< Indicate flash is state on reading or writing */
} iflash_env_t;

typedef struct {
    uint32_t cap;                       /*!< Capacity */
    IRQn_Type irq_num;                  /*!< IRQ number */
    uint8_t irq_prio;                   /*!< IRQ priority */
} flash_resource_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
static iflash_env_t flash0_env = {
    .isr_cb = NULL,
    .addr = 0,
    .data = NULL,
    .data_len = 0,
    .data_cnt = 0,
    .id.id = 0,
    .read_id_cmd_cfg = FLASH_READ_ID_CFG,
    .state = FLASH_STATE_UNINIT,
};

static const flash_resource_t flash0_resource = {
    .irq_num = SF0_IRQn,
#if defined(RTE_FLASH0_IRQ_PRIORITY)
    .irq_prio = RTE_FLASH0_IRQ_PRIORITY,
#endif
};


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
__IFLASH_CODE static void sf_hardware_init(const flash_config_t *config)
{
    // 1. Clock division must be even
    // 2. SPI mode 0/3
    // 3. Enable clock bypass when clock division is 0/1
    // 4. Use OPCODE for transparent read opcode
    // 5. Disable 4 byte address
    // 6. Data unit 8bit

    OM_CRITICAL_BEGIN();
    register_set(&OM_SF0->CFG[0].SPI_CFG,
                  MASK_4REG(SF_CONFIG_CLK_DIV, config->clk_div & (~(0x1)),
                            SF_CONFIG_SPI_MODE, config->spi_mode,
                            SF_CONFIG_CLK_BYPASS, config->clk_div < 2,
                            SF_CONFIG_DATA_UNIT, 0));
    register_set(&OM_SF0->DELAY_CTRL, MASK_1REG(SF_DELAY_CTRL_DLY, config->delay));
    // Clear interrupt status
    OM_SF0->RAW_INT_STATUS = SF_RAW_INT_STATUS_CMD_DONE_MASK;
    // Disable interrupt of sf0
    OM_SF0->INT_EN = 0;
    // Disable software control
    OM_SF0->SW_CFG1 = 0;
    OM_CRITICAL_END();
}

__IFLASH_CODE static void sf_trans_dma_start(cmd_frame_t *cmd_frame, uint8_t *data, uint32_t data_len)
{
    OM_SF0->ADDR = (uint32_t)data;
    OM_SF0->CMD_DATA0 = cmd_frame->cmd_data[0];
    OM_SF0->CMD_DATA1 = cmd_frame->cmd_data[1];
    OM_SF0->RAW_INT_STATUS = SF_RAW_INT_STATUS_CMD_DONE_MASK;
    OM_SF0->CMD = cmd_frame->cmd_cfg | (data_len << (SF_COMMAND_DATA_BYTES_POS));
}

__RAM_CODE static void sf_wait_trans_done(void)
{
    // Wait for trans done and clear the status
    while (!(OM_SF0->RAW_INT_STATUS & SF_RAW_INT_STATUS_CMD_DONE_MASK));
    OM_SF0->RAW_INT_STATUS = SF_RAW_INT_STATUS_CMD_DONE_MASK;
}

__IFLASH_CODE static void sf_trans_dma_start_wait_done(cmd_frame_t *cmd_frame,
                                                       volatile uint8_t *data,
                                                       uint32_t data_len)
{
    #if (RTE_FLASH0_XIP)
    OM_CRITICAL_BEGIN();
    #endif
    sf_trans_dma_start(cmd_frame, (uint8_t *)data, data_len);
    sf_wait_trans_done();
    #if (RTE_FLASH0_XIP)
    OM_CRITICAL_END();
    #endif
}

__IFLASH_CODE static void sf_read_opcode_set(uint8_t opcode)
{
    // Update read frame config
    register_set(&(OM_SF0->OPCODE), MASK_1REG(SF_READ_OPCODE_CS0_OPCODE, opcode));
}

/* data length must be less than or equal to 8 */
__RAM_CODE static void iflash_read_reg(cmd_frame_t *cmd_frame, uint8_t *data, uint32_t data_len)
{
    OM_SF_Type *sf = OM_SF0;
    uint32_t cmd_bits = register_get(&(cmd_frame->cmd_cfg), MASK_POS(SF_COMMAND_CMD_BITS));
    uint32_t rd_data[2];
    uint8_t *dp = ((uint8_t *)rd_data) + data_len - 1;
    uint32_t cmd_cfg = cmd_frame->cmd_cfg;

    // set cmd bits(cmd + data)
    cmd_bits += (data_len << 3);
    register_set(&cmd_cfg, MASK_1REG(SF_COMMAND_CMD_BITS, cmd_bits));

    #if (RTE_FLASH0_XIP)
    OM_CRITICAL_BEGIN();
    #endif
    // send
    sf->ADDR = 0;
    sf->CMD_DATA0 = cmd_frame->cmd_data[0];
    sf->CMD_DATA1 = cmd_frame->cmd_data[1];
    sf->RAW_INT_STATUS = SF_RAW_INT_STATUS_CMD_DONE_MASK;
    sf->CMD = cmd_cfg;
    sf_wait_trans_done();
    // read data
    if (data_len > 0 && data != NULL) {
        rd_data[0] = sf->RD_DATA0;
        rd_data[1] = sf->RD_DATA1;
        // MSB 2 LSB
        for (uint8_t i = 0; i < data_len; i++) {
            data[i] = *(dp - i);
        }
    }
    #if (RTE_FLASH0_XIP)
    OM_CRITICAL_END();
    #endif
}

__IFLASH_CODE static om_error_t iflash_poll_wip(cmd_frame_t *frame)
{
    uint8_t status = 0;

    do {
        iflash_read_reg(frame, &status, 1);
    } while (status & FLASH_STATUS_1_WIP_MASK);
    return OM_ERROR_OK;
}

__IFLASH_CODE static void iflash_write_enable(void)
{
    cmd_frame_t frame;

    CMD_FRAME_SET(&frame, FLASH_WRITE_ENABLE_CFG, 0);
    iflash_read_reg(&frame, NULL, 0);
}

// VSR: volatile status register
__IFLASH_CODE static void iflash_write_enable_for_vsr(void)
{
    cmd_frame_t frame;

    CMD_FRAME_SET(&frame, FLASH_WRITE_ENABLE_VSR_CFG, 0);
    iflash_read_reg(&frame, NULL, 0);
}

/* data length must be less than or equal to 8 */
__IFLASH_CODE static om_error_t iflash_write_reg(cmd_frame_t *write_reg_frame,
                                                 cmd_frame_t *wip_frame,
                                                 uint8_t *data,
                                                 uint32_t data_len,
                                                 flash_reg_write_en_t wr_en_type)
{
    om_error_t error;
    OM_SF_Type *sf = OM_SF0;
    uint32_t cmd_bits = register_get(&(write_reg_frame->cmd_cfg), MASK_POS(SF_COMMAND_CMD_BITS));
    uint32_t wr_data[2] = {0, 0};
    uint8_t *dp = (uint8_t *)wr_data;

    // set data(LSB 2 MSB)
    for (uint8_t i = 0; i < data_len; i++) {
        dp[sizeof(wr_data) - 1 - i] = data[i];
    }
    if (cmd_bits < (sizeof(uint32_t) << 3)) {
        write_reg_frame->cmd_data[0] |= wr_data[1] >> cmd_bits;
        write_reg_frame->cmd_data[1] = (wr_data[1] << ((sizeof(uint32_t) << 3) - cmd_bits)) |
                                 (wr_data[0] >> cmd_bits);
    } else {
        write_reg_frame->cmd_data[1] |= wr_data[1] >> (cmd_bits - (sizeof(uint32_t) << 3));
    }
    // set cmd bits(cmd + data)
    cmd_bits += (data_len << 3);
    register_set(&(write_reg_frame->cmd_cfg), MASK_1REG(SF_COMMAND_CMD_BITS, cmd_bits));

    #if (RTE_FLASH0_XIP)
    OM_CRITICAL_BEGIN();
    #endif

    if (wr_en_type == SR_WRITE_EN_VOLATILE) {
        iflash_write_enable_for_vsr();
    } else if (wr_en_type == SR_WRITE_EN_PERMANENT) {
        iflash_write_enable();
    }

    // send
    sf->ADDR = 0;
    sf->CMD_DATA0 = write_reg_frame->cmd_data[0];
    sf->CMD_DATA1 = write_reg_frame->cmd_data[1];
    sf->RAW_INT_STATUS = SF_RAW_INT_STATUS_CMD_DONE_MASK;
    sf->CMD = write_reg_frame->cmd_cfg;
    sf_wait_trans_done();
    error = iflash_poll_wip(wip_frame);

    #if (RTE_FLASH0_XIP)
    OM_CRITICAL_END();
    #endif
    return error;
}

static om_error_t iflash_read_frame_get(uint32_t addr, cmd_frame_t *frame)
{
    iflash_env_t *env = &flash0_env;

    switch (env->read_cmd) {
        case FLASH_READ:
            CMD_FRAME_SET(frame, FLASH_READ_CFG, addr);
            break;
        case FLASH_FAST_READ:
            CMD_FRAME_SET(frame, FLASH_FAST_READ_CFG, addr);
            break;
        case FLASH_FAST_READ_DO:
            CMD_FRAME_SET(frame, FLASH_FAST_READ_DO_CFG, addr);
            break;
        case FLASH_FAST_READ_DIO:
            CMD_FRAME_SET(frame, FLASH_FAST_READ_DIO_CFG, addr);
            break;
        case FLASH_FAST_READ_QO:
            CMD_FRAME_SET(frame, FLASH_FAST_READ_QO_CFG, addr);
            break;
        case FLASH_FAST_READ_QIO:
            CMD_FRAME_SET(frame, FLASH_FAST_READ_QIO_CFG, addr);
            break;
        default:
            return OM_ERROR_PARAMETER;
    }
    return OM_ERROR_OK;
}

static om_error_t iflash_write_frame_get(uint32_t addr, cmd_frame_t *frame)
{
    iflash_env_t *env = &flash0_env;

    switch (env->write_cmd) {
        case FLASH_PAGE_PROGRAM:
            CMD_FRAME_SET(frame, FLASH_PROGRAM_CFG, addr);
            break;
        case FLASH_PAGE_PROGRAM_QI:
        default:
            return OM_ERROR_UNSUPPORTED;
    }
    return OM_ERROR_OK;
}

static om_error_t iflash_erase_frame_get(flash_erase_t erase_type, uint32_t addr, cmd_frame_t *frame)
{
    switch (erase_type) {
        case FLASH_ERASE_4K:
            CMD_FRAME_SET(frame, FLASH_SECTOR_ERASE_4K_CFG, addr);
            break;
        case FLASH_ERASE_32K:
            CMD_FRAME_SET(frame, FLASH_BLOCK_ERASE_32K_CFG, addr);
            break;
        case FLASH_ERASE_64K:
            CMD_FRAME_SET(frame, FLASH_BLOCK_ERASE_64K_CFG, addr);
            break;
        case FLASH_ERASE_CHIP:
            CMD_FRAME_SET(frame, FLASH_CHIP_ERASE_CFG, addr);
            break;
        default:
            return OM_ERROR_RESOURCES;
    }
    return OM_ERROR_OK;
}

__IFLASH_CODE static void iflash_ext_cmd_send(ext_cmd_frame_t *frame, uint32_t addr, uint8_t *data, uint16_t data_len)
{
    OM_CRITICAL_BEGIN();
    OM_SF0->SW_CFG0 = frame->ext_cmd_cfg[0];
    OM_SF0->SW_CFG1 = (frame->ext_cmd_cfg[1] & (~SF_SW_SPI_CFG1_DATA_BYTES_MASK)) |
                      ((data_len << SF_SW_SPI_CFG1_DATA_BYTES_POS) & SF_SW_SPI_CFG1_DATA_BYTES_MASK);
    OM_SF0->CMD_DATA0 = (frame->ext_cmd_code) << 24;
    OM_SF0->CMD_DATA1 = addr << 8;
    OM_SF0->ADDR = (uint32_t)data;
    OM_SF0->RAW_INT_STATUS = SF_RAW_INT_STATUS_CMD_DONE_MASK;
    REGW(&OM_SF0->CMD, MASK_2REG(SF_COMMAND_RW_SELECT, frame->rw_sel, SF_COMMAND_DATA_BYTES, data_len));
    sf_wait_trans_done();
    // disable sw cfg
    OM_SF0->SW_CFG1 = 0;
    OM_CRITICAL_END();
}

#if (RTE_FLASH0_XIP)
__IFLASH_CODE om_error_t iflash_suspend(cmd_frame_t *suspend_frame)
{
    iflash_env_t *env = &flash0_env;
    uint32_t wait_us;

    iflash_read_reg(suspend_frame, NULL, 0);

    // CS# High To Next Command After Suspend
    switch (env->id.man_id) {
        case FLASH_MID_PUYA:        // (0x856014) tPSL = 30us
        case FLASH_MID_GIGADEVICE:  // (0xc86514) tSUS = 40us
        default:
            wait_us = 50;
            break;
    }
    DRV_DELAY_US(wait_us);

    return OM_ERROR_OK;
}

__IFLASH_CODE om_error_t iflash_resume(cmd_frame_t *resume_frame)
{
    iflash_env_t *env = &flash0_env;
    uint32_t wait_us;

    iflash_read_reg(resume_frame, NULL, 0);
    // Latency between Program/Erase Resume and next Suspend
    switch (env->id.man_id) {
        case FLASH_MID_PUYA:        // (0x856014) tPRS = 20us
        case FLASH_MID_GIGADEVICE:  // (0xc86514) tRS = 100us
        default:
            wait_us = 110;
            break;
    }
    DRV_DELAY_US(wait_us);

    return OM_ERROR_OK;
}

__IFLASH_CODE static om_error_t iflash_poll_wip_with_suspend(cmd_frame_t *wip_frame,
                                                             cmd_frame_t *suspend_frame,
                                                             cmd_frame_t *resume_frame,
                                                             uint32_t irq_save)
{
    om_error_t ret;
    uint8_t status;

    while (iflash_read_reg(wip_frame, &status, 1), status & FLASH_STATUS_1_WIP_MASK) {
        // Delay 100us
        DRV_WAIT_US_UNTIL_TO(!(drv_irq_is_any_ext_pending() && !irq_save), 100, ret);
        // is pending
        if (ret == OM_ERROR_OK) {
            // suspend
            iflash_suspend(suspend_frame);
            // like call __enable_irq()
            OM_CRITICAL_END_EX(irq_save);
            // ENTER irq

            // like call __disable_irq()
            OM_CRITICAL_BEGIN_EX(irq_save);
            // resume
            iflash_resume(resume_frame);
        }
    }
    return OM_ERROR_OK;
}

__IFLASH_CODE static om_error_t iflash_trans_start_with_suspend_wip(cmd_frame_t *trans_frame,
                                                                    cmd_frame_t *wip_frame,
                                                                    cmd_frame_t *suspend_frame,
                                                                    cmd_frame_t *resume_frame,
                                                                    volatile uint8_t *data,
                                                                    uint32_t data_len)
{
    om_error_t error;
    uint32_t irq_save;

    OM_CRITICAL_BEGIN_EX(irq_save);
    sf_trans_dma_start_wait_done(trans_frame, data, data_len);
    error = iflash_poll_wip_with_suspend(wip_frame, suspend_frame, resume_frame, irq_save);
    OM_CRITICAL_END_EX(irq_save);
    return error;
}

#else

__IFLASH_CODE static om_error_t iflash_trans_start_with_wip(cmd_frame_t *trans_frame,
                                                      cmd_frame_t *wip_frame,
                                                      uint8_t *data,
                                                      uint32_t data_len)
{
    sf_trans_dma_start_wait_done(trans_frame, data, data_len);
    return iflash_poll_wip(wip_frame);
}
#endif

__IFLASH_CODE static om_error_t iflash_ext_cmd_send_with_wip(ext_cmd_frame_t *cmd_frame,
                                                             cmd_frame_t *wip_frame,
                                                             uint32_t addr,
                                                             uint8_t *data,
                                                             uint16_t data_len)
{
    om_error_t error;

    OM_CRITICAL_BEGIN();
    iflash_ext_cmd_send(cmd_frame, addr, data, data_len);
    error = iflash_poll_wip(wip_frame);
    OM_CRITICAL_END();

    return error;
}

__RAM_CODE static void iflash_deep_powerdown_enter(void)
{
    uint32_t wait_us = 20;

    iflash_read_reg(&(flash0_env.dp_enter_frame), NULL, 0);
    switch (flash0_env.id.man_id) {
        case FLASH_MID_PUYA:
        case FLASH_MID_GIGADEVICE:
        case FLASH_MID_GT:
            // tDP: 3us
            wait_us = 5;
            break;
        default:
            break;
    }
    DRV_DELAY_US(wait_us);
}

__RAM_CODE static void iflash_deep_powerdown_exit(void)
{
    uint32_t wait_us = 45;

    iflash_read_reg(&(flash0_env.dp_exit_frame), NULL, 0);
    switch (flash0_env.id.man_id) {
        case FLASH_MID_PUYA:
            // tRES1:8us
            wait_us = 12;
            break;
        case FLASH_MID_GIGADEVICE:
            // tRES1:30us
            wait_us = 45;
            break;
        case FLASH_MID_GT:
            // tRES1:20us
            wait_us = 30;
            break;
        default:
            break;
    }
    DRV_DELAY_US(wait_us);
}

__IFLASH_CODE static om_error_t iflash_read_id(cmd_frame_t *read_id_frame, flash_id_t *id)
{
    flash_id_t id_read;

    id_read.id = 0;
    iflash_read_reg(read_id_frame, (uint8_t*)&id_read, 3);
    if ((id_read.id & 0x00FFFFFF) == 0x00FFFFFF || id_read.id == 0x0) {
        return OM_ERROR_FAIL;
    }
    *id = id_read;
    return OM_ERROR_OK;
}

__IFLASH_CODE static om_error_t iflash_detect(cmd_frame_t *read_id_frame)
{
    iflash_env_t *env = &flash0_env;
    flash_id_t retry_id;
    uint8_t power_retry_cnt = 0;
    om_error_t error = OM_ERROR_OK;

    OM_CRITICAL_BEGIN();
    // Check flash power status
    if (!drv_pmu_iflash_ready_state_get()) {
        drv_pmu_iflash_power_mode_set(PMU_IFLASH_MANU_POWER_UP);
        // wait (tVSL, tPUW), worst case 10ms
        DRV_DELAY_MS(10);
    }
    // clear id
    env->id.id = 0;
    // Detect flash by read id twice
    while (!(iflash_read_id(read_id_frame, &env->id) == OM_ERROR_OK &&
                iflash_read_id(read_id_frame, &retry_id) == OM_ERROR_OK &&
                env->id.id == retry_id.id)) {
        if (power_retry_cnt > POWER_UP_RETRY_CNT) {
            error = OM_ERROR_HARDWARE;
            break;
        }
        if (power_retry_cnt == 0) {
            iflash_deep_powerdown_exit();
        } else {
            drv_pmu_iflash_power_mode_set(PMU_IFLASH_MANU_POWER_DOWN);
            DRV_DELAY_MS(50);
            drv_pmu_iflash_power_mode_set(PMU_IFLASH_MANU_POWER_UP);
            // wait (tVSL, tPUW), worst case 10ms
            DRV_DELAY_MS(10);
        }
        power_retry_cnt++;
    }
    // if detect ok, setup default setting
    // HW wakeup flow: from PD=0 to SW-run is about 167us
    // GD: tVSL=1ms (GD25WQ80E)  PUYA: tVSL=150us (P25Q40SU)
    if (error == OM_ERROR_OK && env->id.man_id == FLASH_MID_GIGADEVICE) {
        drv_pmu_iflash_power_mode_set(PMU_IFLASH_AUTO_SLEEP_POWER_DOWN_DISABLE);
    } else {
        drv_pmu_iflash_power_mode_set(PMU_IFLASH_AUTO_SLEEP_POWER_DOWN_ENABLE);
    }
    // Disable auto send deep sleep power down command
    drv_pmu_iflash_sleep_auto_send_power_cmd_disable();
    // Enable low voltage detection
    drv_pmu_iflash_low_voltage_detection_enable();
    OM_CRITICAL_END();
    return error;
}

__IFLASH_CODE static om_error_t iflash_auto_delay_init(cmd_frame_t *read_id_frame, flash_config_t *config)
{
    flash_id_t id1, id2;
    int32_t delayi, delay1 = -1, delay2 = FLASH_DELAY_MAX;
    flash_config_t sfcfg = *config;
    uint8_t retry_cnt = 0;
    om_error_t error = OM_ERROR_FAIL;

    OM_CRITICAL_BEGIN();
    // set flash low frequency so it can work
    sfcfg.clk_div = drv_rcc_clock_get(RCC_CLK_SF0) / FLASH_FREQ_HZ_DEFAULT;
    sfcfg.delay = FLASH_DELAY_DEFAULT;
    if (sfcfg.clk_div < 2) {
        OM_ASSERT(sfcfg.spi_mode == FLASH_SPI_MODE_0);
    }
    sf_hardware_init(&sfcfg);
    // read id twice, save the true id to id1
    while (1) {
        if ((iflash_read_id(read_id_frame, &id1) == OM_ERROR_OK) && (iflash_read_id(read_id_frame, &id2) == OM_ERROR_OK) && (id1.id == id2.id)) {
            break;
        }
        retry_cnt++;
        if (retry_cnt >= FLASH_AUTO_DLY_RETYR_CNT) {
            goto EXIT;
        }
    };

    // poll, delayi from 0 to DELAY_MAX
    for (delayi = 0; delayi <= FLASH_DELAY_MAX; ++delayi) {
        sfcfg.clk_div = config->clk_div;
        sfcfg.delay = delayi;
        sf_hardware_init(&sfcfg);
        if ((iflash_read_id(read_id_frame, &id2) == OM_ERROR_OK) && (id1.id == id2.id)) {
            if (delay1 == -1) {
                delay1 = delayi;
            }
            delay2 = delayi;
        } else {
            if (delay1 != -1) {
                break;
            }
        }
    }
    if (delay1 == -1) {
        // faild, then set flash low frequency so it can work
        sfcfg.clk_div = 8;
        sfcfg.delay = FLASH_DELAY_DEFAULT;
    } else {
        sfcfg.delay = (delay1 + delay2) / 2;
        error = OM_ERROR_OK;
    }
    sf_hardware_init(&sfcfg);

EXIT:
    flash0_env.delay_info.auto_delay = sfcfg.delay;
    flash0_env.delay_info.valid_delay_max = delay2;
    flash0_env.delay_info.valid_delay_min = delay1;
    flash0_env.delay_info.auto_delay_en = 1;
    OM_CRITICAL_END();
    return error;
}


static om_error_t iflash_write_status(OM_SF_Type *om_flash, uint8_t *status, uint8_t status_len, uint8_t is_volatile)
{
    cmd_frame_t write_reg_frame;
    cmd_frame_t wip_frame;
    uint8_t rd_status[2];

    CMD_FRAME_SET(&write_reg_frame, FLASH_WRITE_STATUS_REGS_CFG, 0);
    CMD_FRAME_SET(&wip_frame, FLASH_READ_STATUS_REG_1_CFG, 0);

    #if (RTE_FLASH0_XIP)
    OM_CRITICAL_BEGIN();
    #endif
    iflash_write_reg(&write_reg_frame, &wip_frame, status, status_len, is_volatile ? SR_WRITE_EN_VOLATILE : SR_WRITE_EN_PERMANENT);
    #if (RTE_FLASH0_XIP)
    OM_CRITICAL_END();
    #endif
    // read back to verify
    drv_iflash_read_status_reg1(om_flash, &rd_status[0]);
    if ((rd_status[0] & (~(FLASH_STATUS_1_WIP_MASK | FLASH_STATUS_1_WEL_MASK))) !=
        (status[0] & (~(FLASH_STATUS_1_WIP_MASK | FLASH_STATUS_1_WEL_MASK)))) {
            return OM_ERROR_VERIFY;
    }
    if (status_len > 1) {
        drv_iflash_read_status_reg2(om_flash, &rd_status[1]);
        if (rd_status[1] != status[1]) {
            return OM_ERROR_VERIFY;
        }
    }
    return OM_ERROR_OK;
}

static om_error_t iflash_write_config_reg(OM_SF_Type *om_flash, uint8_t *config, uint8_t is_volatile)
{
    cmd_frame_t write_reg_frame;
    cmd_frame_t wip_frame;

    CMD_FRAME_SET(&write_reg_frame, FLASH_WRITE_CONFIG_REG_CFG, 0);
    CMD_FRAME_SET(&wip_frame, FLASH_READ_STATUS_REG_1_CFG, 0);
    #if (RTE_FLASH0_XIP)
    OM_CRITICAL_BEGIN();
    #endif
    iflash_write_reg(&write_reg_frame, &wip_frame, config, 1, is_volatile ? SR_WRITE_EN_VOLATILE : SR_WRITE_EN_PERMANENT);
    #if (RTE_FLASH0_XIP)
    OM_CRITICAL_END();
    #endif
    return OM_ERROR_OK;
}


static om_error_t iflash_modifiy_status_bits(OM_SF_Type *om_flash,
                        uint8_t status[2], uint8_t mask[2], uint8_t is_volatile)
{
    uint8_t s1[2] = {0, 0};
    uint8_t s2[2] = {0, 0};
    om_error_t error;

    // Read status reg
    if ((error = drv_iflash_read_status_reg1(om_flash, &s1[0])) != OM_ERROR_OK) {
        goto ERR;
    }
    if ((error = drv_iflash_read_status_reg2(om_flash, &s1[1])) != OM_ERROR_OK) {
        goto ERR;
    }
    // Read status reg again
    if ((error = drv_iflash_read_status_reg1(om_flash, &s2[0])) != OM_ERROR_OK) {
        goto ERR;
    }
    if ((error = drv_iflash_read_status_reg2(om_flash, &s2[1])) != OM_ERROR_OK) {
        goto ERR;
    }
    // check status reg
    if (s1[0] != s2[0] || s1[1] != s2[1]) {
        return OM_ERROR_VERIFY;
    }
    // if not volatile, then force to write status reg
    if (((s1[0] & mask[0]) != (status[0] & mask[0])) || ((s1[1] & mask[1]) != (status[1] & mask[1]))) {
        s1[0] &= ~mask[0];
        s1[0] |= (status[0] & mask[0]);
        s1[1] &= ~mask[1];
        s1[1] |= (status[1] & mask[1]);
    } else if (is_volatile) {
        return OM_ERROR_OK;
    }
    return iflash_write_status(om_flash, s1, 2, is_volatile);

ERR:
    return error;
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
om_error_t drv_iflash_init(OM_SF_Type *om_flash, const flash_config_t *config)
{
    const flash_resource_t *resource = &flash0_resource;
    iflash_env_t *env = &flash0_env;
    om_error_t error = OM_ERROR_FAIL;
    cmd_frame_t read_id_frame;
    flash_config_t sfcfg;
    uint8_t status[2] = {
        (uint8_t)FLASH_PROTECT_UPPER_4_SECTOR_UNPROTECTED,
        (1 << FLASH_STATUS_2_CMP_POS)
    };
    uint8_t mask[2] = {
        FLASH_STATUS_1_BP_MASK | FLASH_STATUS_1_SRP0_MASK,
        FLASH_STATUS_2_CMP_MASK | FLASH_STATUS_2_SRP1_MASK
    };

    // clock enable
    if (drv_rcc_clock_get(RCC_CLK_SF0) == 0) {
        DRV_RCC_CLOCK_ENABLE(RCC_CLK_SF0, 1);
    }
    if (!config) {
        // If config is NULL, set default config, freq is 8MHz, delay is 2, bus width is 2
        sfcfg.clk_div = drv_rcc_clock_get(RCC_CLK_SF0) / FLASH_FREQ_HZ_DEFAULT;
        sfcfg.delay = FLASH_DELAY_DEFAULT;
        sfcfg.spi_mode = FLASH_SPI_MODE_0;
        sfcfg.read_cmd = FLASH_FAST_READ_DO;
        sfcfg.write_cmd = FLASH_PAGE_PROGRAM;
    } else {
        if (config->spi_mode == FLASH_SPI_MODE_1 ||
            config->spi_mode == FLASH_SPI_MODE_2 ||
            (config->spi_mode == FLASH_SPI_MODE_3 && config->clk_div < 2)) {
                // Flash only support SPI mode 0/3,
                // and in SPI mode 3, clock division must be at least 2
                return OM_ERROR_PARAMETER;
        }
        sfcfg = *config;
    }
    // sf hardware init
    read_id_frame.cmd_cfg = env->read_id_cmd_cfg.cmd_cfg;
    read_id_frame.cmd_data[0] = env->read_id_cmd_cfg.cmd_code << 24;
    read_id_frame.cmd_data[1] = 0;
    if (sfcfg.delay == FLASH_DELAY_AUTO) {
        if(iflash_auto_delay_init(&read_id_frame, (flash_config_t *)&sfcfg) != OM_ERROR_OK) {
            return OM_ERROR_FAIL;
        }
    } else {
        sf_hardware_init((flash_config_t *)&sfcfg);
        flash0_env.delay_info.auto_delay_en = 0;
    }
    // Detect iflash by read flash id
    CMD_FRAME_SET(&(env->dp_enter_frame), FLASH_DEEP_POWERDOWN_CFG, 0);
    CMD_FRAME_SET(&(env->dp_exit_frame), FLASH_RELEASE_POWERDOWN_CFG, 0);
    if ((error = iflash_detect(&read_id_frame)) != OM_ERROR_OK) {
        goto INIT_EXIT;
    }
    #if (!RTE_FLASH_SAFETY_ENHANCE_DISABLE)
    // check important bits in registers
    switch (env->id.man_id) {
        case FLASH_MID_GIGADEVICE:
            {
                // GD: clear DC bit in status register 2
                uint8_t dc_mask[2] = {0, GD_FLASH_STATUS_2_DC_MASK};
                uint8_t dc_status[2] = {0, 0};
                if ((error = drv_iflash_modifiy_status_bits(om_flash, dc_status, dc_mask))!= OM_ERROR_OK) {
                    goto INIT_EXIT;
                }
            }
            break;
        case FLASH_MID_PUYA:
            {
                // Puya: clear DC/WPS/HOLD_RST bit in config register
                uint8_t cfg_val;
                if ((error = drv_iflash_read_config_reg(om_flash, &cfg_val)) != OM_ERROR_OK) {
                    goto INIT_EXIT;
                }
                if (cfg_val & (PUYA_FLASH_CONFIG_DC_MASK | PUYA_FLASH_CONFIG_WPS_MASK | PUYA_FLASH_CONFIG_HD_RST_MASK)) {
                    cfg_val &= ~(PUYA_FLASH_CONFIG_DC_MASK | PUYA_FLASH_CONFIG_WPS_MASK | PUYA_FLASH_CONFIG_HD_RST_MASK);
                    if ((error = drv_iflash_write_config_reg(om_flash, &cfg_val)) != OM_ERROR_OK) {
                        goto INIT_EXIT;
                    }
                }
            }
            break;
        case FLASH_MID_GT:
            {
                // GT: clear DRV0/DRV1 bit in config register
                uint8_t cfg_val;
                if ((error = drv_iflash_read_config_reg(om_flash, &cfg_val)) != OM_ERROR_OK) {
                    goto INIT_EXIT;
                }
                if (cfg_val & (GT_FLASH_CONFIG_DRV0_MASK | GT_FLASH_CONFIG_DRV1_MASK)) {
                    cfg_val &= ~(GT_FLASH_CONFIG_DRV0_MASK | GT_FLASH_CONFIG_DRV1_MASK);
                    if ((error = drv_iflash_write_config_reg(om_flash, &cfg_val)) != OM_ERROR_OK) {
                        goto INIT_EXIT;
                    }
                }
            }
            break;
        default:
            break;
    }
    // update status(WP, SRP)
    if ((error = drv_iflash_modifiy_status_bits_volatile(om_flash, status, mask)) != OM_ERROR_OK) {
        goto INIT_EXIT;
    }
    #endif /* RTE_FLASH_SAFETY_ENHANCE_DISABLE */
    // check quad mode
    if (sfcfg.read_cmd == FLASH_FAST_READ_QO || sfcfg.read_cmd == FLASH_FAST_READ_QIO) {
        if (drv_iflash_write_protect_set_volatile(om_flash, FLASH_PROTECT_UPPER_4_SECTOR_UNPROTECTED) != OM_ERROR_OK) {
            goto INIT_EXIT;
        }
    }
    // Set read/write command and frame
    if ((error = drv_iflash_read_cmd_set(om_flash, sfcfg.read_cmd)) != OM_ERROR_OK) {
        goto INIT_EXIT;
    }
    if ((error = drv_iflash_write_cmd_set(om_flash, sfcfg.write_cmd)) != OM_ERROR_OK) {
        goto INIT_EXIT;
    }
    // NVIC clear and Enable IRQ
    NVIC_ClearPendingIRQ(resource->irq_num);
    NVIC_SetPriority(resource->irq_num, resource->irq_prio);
    NVIC_EnableIRQ(resource->irq_num);
    env->state = FLASH_STATE_INIT;
    return OM_ERROR_OK;

INIT_EXIT:
    // Set state to none
    env->state = FLASH_STATE_UNINIT;
    return error;
}

__IFLASH_CODE om_error_t drv_iflash_delay_recalib(void)
{
    flash_config_t flash_cfg;
    iflash_env_t *env = &flash0_env;
    uint32_t spi_cfg_reg;
    cmd_frame_t read_id_frame;

    // fill flash_cfg with current config
    flash_cfg.read_cmd = env->read_cmd;
    flash_cfg.write_cmd = env->write_cmd;
    spi_cfg_reg = OM_SF0->CFG[0].SPI_CFG;
    flash_cfg.spi_mode = (flash_spi_mode_t)((spi_cfg_reg & SF_CONFIG_SPI_MODE_MASK) >> SF_CONFIG_SPI_MODE_POS);
    if (spi_cfg_reg & SF_CONFIG_CLK_BYPASS_MASK) {
        flash_cfg.clk_div = 0;
    } else {
        flash_cfg.clk_div = (spi_cfg_reg & SF_CONFIG_CLK_DIV_MASK) >> SF_CONFIG_CLK_DIV_POS;
    }
    flash_cfg.delay = FLASH_DELAY_AUTO;
    // fill read_id_frame
    read_id_frame.cmd_cfg = env->read_id_cmd_cfg.cmd_cfg;
    read_id_frame.cmd_data[0] = env->read_id_cmd_cfg.cmd_code << 24;
    read_id_frame.cmd_data[1] = 0;
    // re-init SF0
    return iflash_auto_delay_init(&read_id_frame, &flash_cfg);
}

void drv_iflash_get_delay_info(flash_delay_info_t *info)
{
    if (info) {
        *info = flash0_env.delay_info;
    }
}

om_error_t drv_iflash_id_get(OM_SF_Type *om_flash, flash_id_t *id)
{
    if ((flash0_env.id.id & 0x00FFFFFF) == 0x00FFFFFF || flash0_env.id.id == 0x0) {
        return OM_ERROR_FAIL;
    }
    *id = flash0_env.id;
    return OM_ERROR_OK;
}

om_error_t drv_iflash_read_cmd_set(OM_SF_Type *om_flash, flash_read_t read_cmd)
{
    iflash_env_t *env = &flash0_env;
    uint32_t cmd = 0;

    switch (read_cmd) {
        case FLASH_READ:
            cmd = ((cmd_cfg_t)FLASH_READ_CFG).cmd_code;
            break;
        case FLASH_FAST_READ:
            cmd = ((cmd_cfg_t)FLASH_FAST_READ_CFG).cmd_code;
            break;
        case FLASH_FAST_READ_DO:
            cmd = ((cmd_cfg_t)FLASH_FAST_READ_DO_CFG).cmd_code;
            break;
        case FLASH_FAST_READ_DIO:
            cmd = ((cmd_cfg_t)FLASH_FAST_READ_DIO_CFG).cmd_code;
            break;
        case FLASH_FAST_READ_QO:
            cmd = ((cmd_cfg_t)FLASH_FAST_READ_QO_CFG).cmd_code;
            break;
        case FLASH_FAST_READ_QIO:
            cmd = ((cmd_cfg_t)FLASH_FAST_READ_QIO_CFG).cmd_code;
            break;
        default:
            return OM_ERROR_PARAMETER;
    }
    // Update read frame config
    sf_read_opcode_set(cmd);
    // Update read frame of env
    env->read_cmd = read_cmd;
    return OM_ERROR_OK;
}

om_error_t drv_iflash_write_cmd_set(OM_SF_Type *om_flash, flash_write_t write_cmd)
{
    iflash_env_t *env = &flash0_env;

    if (write_cmd > FLASH_PAGE_PROGRAM_QI) {
        return OM_ERROR_PARAMETER;
    }
    // Update write frame of env
    env->write_cmd = write_cmd;
    return OM_ERROR_OK;
}

om_error_t drv_iflash_read_uid(OM_SF_Type *om_flash, uint8_t *uid, uint32_t len)
{
    iflash_env_t *env = &flash0_env;
    ext_cmd_frame_t ext_frame = FLASH_READ_UID_EXT_CFG;

    if (env->state != FLASH_STATE_INIT) {
        return OM_ERROR_BUSY;
    }
    iflash_ext_cmd_send(&ext_frame, 0, uid, len);
    return OM_ERROR_OK;
}

om_error_t drv_iflash_read(OM_SF_Type *om_flash,
                           const uint32_t addr,
                           uint8_t *data,
                           const uint32_t data_len)
{
    iflash_env_t *env = &flash0_env;
    cmd_frame_t frame;

    if (env->state != FLASH_STATE_INIT) {
        return OM_ERROR_BUSY;
    }
    #if (!RTE_FLASH_SAFETY_ENHANCE_DISABLE)
    uint32_t cap = FLASH_ID2CAP(env->id);
    if ((addr >= cap) || (data_len > cap - addr)) {
        return OM_ERROR_PERMISSION;
    }
    #endif
    env->state = FLASH_STATE_READING;
    iflash_read_frame_get(addr, &frame);
    sf_trans_dma_start_wait_done(&frame, data, data_len);
    env->state = FLASH_STATE_INIT;
    return OM_ERROR_OK;
}

om_error_t drv_iflash_read_int(OM_SF_Type *om_flash,
                               uint32_t addr,
                               uint8_t *data,
                               uint32_t data_len)
{
    iflash_env_t *env = &flash0_env;
    cmd_frame_t frame;
    om_error_t error;

    // Do not use read/write with interrupt interfaces for internal flash
    #if (RTE_FLASH0_XIP)
    return OM_ERROR_PERMISSION;
    #endif
    if (env->state != FLASH_STATE_INIT) {
        return OM_ERROR_BUSY;
    }
    #if (!RTE_FLASH_SAFETY_ENHANCE_DISABLE)
    uint32_t cap = FLASH_ID2CAP(env->id);
    if ((addr >= cap) || (data_len > cap - addr)) {
        return OM_ERROR_PERMISSION;
    }
    #endif
    if ((error = iflash_read_frame_get(addr, &frame)) != OM_ERROR_OK) {
        return error;
    }
    env->state = FLASH_STATE_READING;
    env->addr = addr;
    env->data = data;
    env->data_len = data_len;
    // Enable command done interrupt
    om_flash->INT_EN = SF_INT_EN_CMD_DONE_MASK;
    sf_trans_dma_start(&frame, data, data_len);
    return OM_ERROR_OK;
}

om_error_t drv_iflash_write(OM_SF_Type *om_flash,
                            uint32_t addr,
                            volatile uint8_t *data,
                            uint32_t data_len)
{
    iflash_env_t *env = &flash0_env;
    om_error_t error = OM_ERROR_OK;
    cmd_frame_t write_frame;
    cmd_frame_t wip_frame;
    #if RTE_FLASH0_XIP
    cmd_frame_t suspend_frame;
    cmd_frame_t resume_frame;
    CMD_FRAME_SET(&suspend_frame, FLASH_SUSPEND_CFG, 0);
    CMD_FRAME_SET(&resume_frame, FLASH_RESUME_CFG, 0);
    #endif

    if (env->state != FLASH_STATE_INIT) {
        return OM_ERROR_BUSY;
    }
    #if (!RTE_FLASH_SAFETY_ENHANCE_DISABLE)
    uint32_t cap = FLASH_ID2CAP(env->id);
    if ((addr < IFLASH_PROTECT_HEAD_LEN) || (addr >= cap) || (data_len > cap - addr)) {
        return OM_ERROR_PERMISSION;
    }
    #endif
    CMD_FRAME_SET(&wip_frame, FLASH_READ_STATUS_REG_1_CFG, 0);
    env->state = FLASH_STATE_WRITING;
    while (data_len) {
        iflash_write_enable();
        // Write to the next Page boundary
        uint32_t write_len = FLASH_PAGE_SIZE - (addr & (FLASH_PAGE_SIZE - 1));
        write_len = (data_len >= write_len) ? write_len : data_len;
        if ((error = iflash_write_frame_get(addr, &write_frame)) != OM_ERROR_OK) {
            goto EXIT;
        }
        #if RTE_FLASH0_XIP
        error = iflash_trans_start_with_suspend_wip(&write_frame, &wip_frame,
                    &suspend_frame, &resume_frame, data, write_len);
        #else
        error = iflash_trans_start_with_wip(&write_frame, &wip_frame,
                    (uint8_t *)data, write_len);
        #endif
        if (error != OM_ERROR_OK) {
            goto EXIT;
        }
        data_len -= write_len;
        addr += write_len;
        data += write_len;
    }

EXIT:
    env->state = FLASH_STATE_INIT;
    return error;
}

om_error_t drv_iflash_write_int_start(OM_SF_Type *om_flash,
                                      uint32_t addr,
                                      volatile uint8_t *data,
                                      uint32_t data_len)
{
    iflash_env_t *env = &flash0_env;
    om_error_t error = OM_ERROR_OK;
    cmd_frame_t frame;

    // Do not use read/write with interrupt interfaces for internal flash
    #if (RTE_FLASH0_XIP)
    return OM_ERROR_PERMISSION;
    #endif

    if (env->state != FLASH_STATE_INIT) {
        return OM_ERROR_BUSY;
    }
    #if (!RTE_FLASH_SAFETY_ENHANCE_DISABLE)
    uint32_t cap = FLASH_ID2CAP(env->id);
    if ((addr < IFLASH_PROTECT_HEAD_LEN) || (addr >= cap) || (data_len > cap - addr)) {
        return OM_ERROR_PERMISSION;
    }
    #endif
    if ((error = iflash_write_frame_get(addr, &frame)) != OM_ERROR_OK) {
        return error;
    }
    env->state = FLASH_STATE_WRITING;
    iflash_write_enable();
    // Write to the next Page boundary
    uint32_t write_len = FLASH_PAGE_SIZE - (addr & (FLASH_PAGE_SIZE - 1));
    write_len = (data_len >= write_len) ? write_len : data_len;

    env->write_int_s = FLASH_TRANS_BUSY;
    env->addr = addr + write_len;
    env->data = (uint8_t *)data + write_len;
    env->data_cnt = write_len;
    env->data_len = data_len;
    // Enable command done interrupt
    om_flash->INT_EN = SF_INT_EN_CMD_DONE_MASK;
    sf_trans_dma_start(&frame, (uint8_t *)data, write_len);

    return error;
}

om_error_t drv_iflash_write_int_status_get(OM_SF_Type *om_flash, uint8_t *is_wip)
{
    cmd_frame_t wip_frame;
    uint8_t status;

    // Waiting for the command done interrupt
    if (flash0_env.write_int_s == FLASH_TRANS_BUSY) {
        *is_wip = 1;
        return OM_ERROR_OK;
    }
    // Check spi bus busy again
    if (om_flash->RAW_INT_STATUS & SF_RAW_INT_STATUS_CMD_DONE_MASK) {
        return OM_ERROR_BUSY;
    }
    // Send command to check wip status
    CMD_FRAME_SET(&wip_frame, FLASH_READ_STATUS_REG_1_CFG, 0);
    iflash_read_reg(&wip_frame, &status, 1);
    *is_wip = (status & FLASH_STATUS_1_WIP_MASK) ? 1 : 0;
    return OM_ERROR_OK;
}

om_error_t drv_iflash_write_int_continue(OM_SF_Type *om_flash)
{
    om_error_t error;
    iflash_env_t *env = &flash0_env;

    if ((env->state == FLASH_STATE_WRITING) && (env->data_cnt < env->data_len)) {
        iflash_write_enable();
        // Write to the next Page boundary
        uint32_t data_remain = env->data_len - env->data_cnt;
        uint32_t write_len = (data_remain >= FLASH_PAGE_SIZE) ? FLASH_PAGE_SIZE : data_remain;
        // Write next data
        cmd_frame_t write_frame;
        if ((error = iflash_write_frame_get(env->addr, &write_frame)) != OM_ERROR_OK) {
            return error;
        }
        env->write_int_s = FLASH_TRANS_BUSY;
        env->addr += write_len;
        env->data += write_len;
        env->data_cnt += write_len;
        // Enable command done interrupt
        om_flash->INT_EN = SF_INT_EN_CMD_DONE_MASK;
        sf_trans_dma_start(&write_frame, env->data - write_len, write_len);

        return OM_ERROR_OK;
    }

    return OM_ERROR_STATUS;
}

om_error_t drv_iflash_erase_start(OM_SF_Type *om_flash, uint32_t addr, flash_erase_t erase_type)
{
    // if xip mode, use drv_iflash_erase function
    #if (RTE_FLASH0_XIP)
    OM_ASSERT(0);
    return OM_ERROR_PERMISSION;
    #else
    iflash_env_t *env = &flash0_env;
    cmd_frame_t erase_frame;
    om_error_t error;

    if (env->state != FLASH_STATE_INIT) {
        return OM_ERROR_BUSY;
    }
    #if (!RTE_FLASH_SAFETY_ENHANCE_DISABLE)
    uint32_t cap = FLASH_ID2CAP(env->id);
    if ((addr < IFLASH_PROTECT_HEAD_LEN) || (addr >= cap)) {
        return OM_ERROR_PERMISSION;
    }
    #endif
    if ((error = iflash_erase_frame_get(erase_type, addr, &erase_frame)) != OM_ERROR_OK) {
        return error;
    }
    env->state = FLASH_STATE_ERASING;
    iflash_write_enable();
    sf_trans_dma_start_wait_done(&erase_frame, NULL, 0);
    return OM_ERROR_OK;
    #endif
}

uint8_t drv_iflash_erase_is_done(OM_SF_Type *om_flash)
{
    // if xip mode, use drv_iflash_erase function
    #if (RTE_FLASH0_XIP)
    OM_ASSERT(0);
    return 1;
    #else
    iflash_env_t *env = &flash0_env;
    cmd_frame_t wip_frame;
    uint8_t status;

    if (env->state != FLASH_STATE_ERASING) {
        return 0;
    }
    CMD_FRAME_SET(&wip_frame, FLASH_READ_STATUS_REG_1_CFG, 0);
    iflash_read_reg(&wip_frame, &status, 1);
    if (!(status & FLASH_STATUS_1_WIP_MASK)) {
        env->state = FLASH_STATE_INIT;
    }
    return (status & FLASH_STATUS_1_WIP_MASK) ? 0 : 1;
    #endif
}

om_error_t drv_iflash_erase(OM_SF_Type *om_flash,
                            uint32_t addr,
                            flash_erase_t erase_type)
{
    iflash_env_t *env = &flash0_env;
    cmd_frame_t erase_frame;
    cmd_frame_t wip_frame;
    om_error_t error;

    if (env->state != FLASH_STATE_INIT) {
        return OM_ERROR_BUSY;
    }
    #if (!RTE_FLASH_SAFETY_ENHANCE_DISABLE)
    uint32_t cap = FLASH_ID2CAP(env->id);
    if ((addr < IFLASH_PROTECT_HEAD_LEN) || (addr >= cap)) {
        return OM_ERROR_PERMISSION;
    }
    #endif
    if ((error = iflash_erase_frame_get(erase_type, addr, &erase_frame)) != OM_ERROR_OK) {
        return error;
    }
    CMD_FRAME_SET(&wip_frame, FLASH_READ_STATUS_REG_1_CFG, 0);
    env->state = FLASH_STATE_ERASING;
    iflash_write_enable();
    #if RTE_FLASH0_XIP
    cmd_frame_t suspend_frame;
    cmd_frame_t resume_frame;
    CMD_FRAME_SET(&suspend_frame, FLASH_SUSPEND_CFG, 0);
    CMD_FRAME_SET(&resume_frame, FLASH_RESUME_CFG, 0);
    error = iflash_trans_start_with_suspend_wip(&erase_frame, &wip_frame,
                &suspend_frame, &resume_frame, NULL, 0);
    #else
    error = iflash_trans_start_with_wip(&erase_frame, &wip_frame, NULL, 0);
    #endif
    env->state = FLASH_STATE_INIT;
    return error;
}

om_error_t drv_iflash_read_id(OM_SF_Type *om_flash, flash_id_t *id)
{
    cmd_frame_t frame;

    CMD_FRAME_SET(&frame, FLASH_READ_ID_CFG, 0);
    return iflash_read_id(&frame, id);
}

om_error_t drv_iflash_read_status_reg1(OM_SF_Type *om_flash, uint8_t *status)
{
    cmd_frame_t frame;

    CMD_FRAME_SET(&frame, FLASH_READ_STATUS_REG_1_CFG, 0);
    iflash_read_reg(&frame, status, 1);
    return OM_ERROR_OK;
}

om_error_t drv_iflash_read_status_reg2(OM_SF_Type *om_flash, uint8_t *status)
{
    cmd_frame_t frame;

    CMD_FRAME_SET(&frame, FLASH_READ_STATUS_REG_2_CFG, 0);
    iflash_read_reg(&frame, status, 1);
    return OM_ERROR_OK;
}

om_error_t drv_iflash_read_config_reg(OM_SF_Type *om_flash, uint8_t *config)
{
    cmd_frame_t frame;

    CMD_FRAME_SET(&frame, FLASH_READ_CONFIG_REG_CFG, 0);
    iflash_read_reg(&frame, config, 1);
    return OM_ERROR_OK;
}

om_error_t drv_iflash_write_status(OM_SF_Type *om_flash, uint8_t *status, uint8_t status_len)
{
    return iflash_write_status(om_flash, status, status_len, 0);
}

om_error_t drv_iflash_write_status_volatile(OM_SF_Type *om_flash, uint8_t *status, uint8_t status_len)
{
    return iflash_write_status(om_flash, status, status_len, 1);
}

om_error_t drv_iflash_write_config_reg(OM_SF_Type *om_flash, uint8_t *config)
{
    return iflash_write_config_reg(om_flash, config, 0);
}

om_error_t drv_iflash_write_config_reg_volatile(OM_SF_Type *om_flash, uint8_t *config)
{
    return iflash_write_config_reg(om_flash, config, 1);
}

om_error_t drv_iflash_modifiy_status_bits(OM_SF_Type *om_flash, uint8_t status[2], uint8_t mask[2])
{
    return iflash_modifiy_status_bits(om_flash, status, mask, 0);
}

om_error_t drv_iflash_modifiy_status_bits_volatile(OM_SF_Type *om_flash, uint8_t status[2], uint8_t mask[2])
{
    return iflash_modifiy_status_bits(om_flash, status, mask, 1);
}

om_error_t drv_iflash_write_protect_set(OM_SF_Type *om_flash, flash_protect_t protect)
{
    // Attention:
    // This function support the following flash:
    // GD25WQ16E, GD25WQ80E, GD25WQ40E,
    // P25Q40SU, P25Q80SU, P25Q16SU,
    // GT25Q40D, GT25Q80A
    // Other flash without testing,
    // if not supported, please use drv_oflash_modifiy_status_bits to set the status register

    uint8_t mask[2] = {FLASH_STATUS_1_BP_MASK, FLASH_STATUS_2_CMP_MASK};
    uint8_t status[2] = {protect, FLASH_STATUS_2_CMP_MASK};

    return iflash_modifiy_status_bits(om_flash, status, mask, 0);
}

om_error_t drv_iflash_write_protect_set_volatile(OM_SF_Type *om_flash, flash_protect_t protect)
{
    // Attention:
    // This function support the following flash:
    // GD25WQ16E, GD25WQ80E, GD25WQ40E,
    // P25Q40SU, P25Q80SU, P25Q16SU,
    // GT25Q40D, GT25Q80A
    // Other flash without testing,
    // if not supported, please use drv_oflash_modifiy_status_bits to set the status register

    uint8_t mask[2] = {FLASH_STATUS_1_BP_MASK, FLASH_STATUS_2_CMP_MASK};
    uint8_t status[2] = {protect, FLASH_STATUS_2_CMP_MASK};

    return iflash_modifiy_status_bits(om_flash, status, mask, 1);
}

om_error_t drv_iflash_quad_enable(OM_SF_Type *om_flash, bool enable)
{
    uint8_t mask[2] = {0, FLASH_STATUS_2_QE_MASK};
    uint8_t status[2] = {0, 0};

    if (enable) {
        status[1] = FLASH_STATUS_2_QE_MASK;
    }

    return iflash_modifiy_status_bits(om_flash, status, mask, 0);
}

om_error_t drv_iflash_quad_enable_volatile(OM_SF_Type *om_flash, bool enable)
{
    uint8_t mask[2] = {0, FLASH_STATUS_2_QE_MASK};
    uint8_t status[2] = {0, 0};

    if (enable) {
        status[1] = FLASH_STATUS_2_QE_MASK;
    }

    return iflash_modifiy_status_bits(om_flash, status, mask, 1);
}

om_error_t drv_iflash_reset(OM_SF_Type *om_flash)
{
    cmd_frame_t rst_en_frame;
    cmd_frame_t rst_frame;

    CMD_FRAME_SET(&rst_en_frame, FLASH_RESET_ENABLE_CFG, 0);
    CMD_FRAME_SET(&rst_frame, FLASH_RESET_CFG, 0);
    iflash_read_reg(&rst_en_frame, NULL, 0);
    iflash_read_reg(&rst_frame, NULL, 0);
    return OM_ERROR_OK;
}

om_error_t drv_iflash_encrypt_enable(OM_SF_Type *om_flash, uint8_t enable)
{
    #if (RTE_EFUSE)
    if (enable) {
        drv_efuse_control(EFUSE_CONTROL_FETCH_UID, (void *)0);
    }
    #endif /* RTE_EFUSE */
    register_set(&om_flash->ENCRYPT_CTRL,
                 MASK_2REG(SF_ENCRYPT_CTRL_ENCRYPT_EN, (enable ? 1 : 0),
                           SF_ENCRYPT_CTRL_DECRYPT_EN, (enable ? 1 : 0)));
    return OM_ERROR_OK;
}

om_error_t drv_iflash_secure_register_erase(OM_SF_Type *om_flash, uint8_t secure_register)
{
    iflash_env_t *env = &flash0_env;
    ext_cmd_frame_t erase_frame = FLASH_SECURE_REG_ERASE_EXT_CFG;
    cmd_frame_t wip_frame;
    om_error_t error;

    CMD_FRAME_SET(&wip_frame, FLASH_READ_STATUS_REG_1_CFG, 0);

    env->state = FLASH_STATE_ERASING;
    OM_CRITICAL_BEGIN();
    iflash_write_enable();
    error = iflash_ext_cmd_send_with_wip(&erase_frame, &wip_frame,
                FLASH_SECURE_REG_ADDR_HIGH(secure_register), NULL, 0);
    OM_CRITICAL_END();
    env->state = FLASH_STATE_INIT;
    return error;
}

om_error_t drv_iflash_secure_register_write(OM_SF_Type *om_flash,
                                            uint8_t secure_register,
                                            uint16_t addr,
                                            uint8_t *data,
                                            uint16_t data_len)
{
    iflash_env_t *env = &flash0_env;
    ext_cmd_frame_t write_frame = FLASH_SECURE_REG_WRITE_EXT_CFG;
    cmd_frame_t wip_frame;
    om_error_t error;

    CMD_FRAME_SET(&wip_frame, FLASH_READ_STATUS_REG_1_CFG, 0);

    env->state = FLASH_STATE_WRITING;
    OM_CRITICAL_BEGIN();
    iflash_write_enable();
    error = iflash_ext_cmd_send_with_wip(&write_frame, &wip_frame,
                FLASH_SECURE_REG_ADDR(secure_register, addr), data, data_len);
    OM_CRITICAL_END();
    env->state = FLASH_STATE_INIT;
    return error;
}

om_error_t drv_iflash_secure_register_read(OM_SF_Type *om_flash,
                                           uint8_t secure_register,
                                           uint16_t addr,
                                           uint8_t *data,
                                           uint16_t data_len)
{
    iflash_env_t *env = &flash0_env;
    ext_cmd_frame_t read_frame = FLASH_SECURE_REG_READ_EXT_CFG;

    env->state = FLASH_STATE_READING;
    iflash_ext_cmd_send(&read_frame, FLASH_SECURE_REG_ADDR(secure_register, addr), data, data_len);
    env->state = FLASH_STATE_INIT;
    return OM_ERROR_OK;
}

__RAM_CODE void drv_iflash_store(void)
{
    if (flash0_env.id.man_id == FLASH_MID_GIGADEVICE) {
        OM_CRITICAL_BEGIN();
        // enter deep powerdown
        iflash_deep_powerdown_enter();
        OM_CRITICAL_END();
    }
}

__RAM_CODE void drv_iflash_restore(void)
{
    if (flash0_env.id.man_id == FLASH_MID_GIGADEVICE) {
        OM_CRITICAL_BEGIN();
        volatile uint32_t iflash_spi_cfg_0_reg = OM_SF0->CFG[0].SPI_CFG;

        // set sf0 to safe frequency
        register_set(&OM_SF0->CFG[0].SPI_CFG, MASK_2REG(SF_CONFIG_CLK_DIV, 4, SF_CONFIG_CLK_BYPASS, 0));
        // exit deep powerdown
        iflash_deep_powerdown_exit();
        // restore sf0 spi cfg
        OM_SF0->CFG[0].SPI_CFG = iflash_spi_cfg_0_reg;
        OM_CRITICAL_END();
    }
}

#if (RTE_FLASH0_REGISTER_CALLBACK)
om_error_t drv_iflash_register_isr_callback(OM_SF_Type *om_flash, drv_isr_callback_t isr_cb)
{
    iflash_env_t *env = &flash0_env;

    env->isr_cb = isr_cb;
    return OM_ERROR_OK;
}
#endif

__WEAK void drv_iflash_isr_callback(OM_SF_Type *om_flash, drv_event_t event)
{
#if (RTE_FLASH0_REGISTER_CALLBACK)
    iflash_env_t *env = &flash0_env;

    if (env->isr_cb) {
        env->isr_cb(om_flash, event, (void *)env->data, (void *)env->data_len);
    }
#endif
}

void drv_iflash_isr(OM_SF_Type *om_flash)
{
    iflash_env_t *env = &flash0_env;
    drv_event_t isr_evt = DRV_EVENT_COMMON_ERROR;
    flash_state_t state = env->state;

    // There are two conditions to check which signal triggered the interrupt:
    // 1. Interrupt status is set
    // 2. Interrupt is enabled
    if ((om_flash->INT_STATUS & SF_INT_STATUS_CMD_DONE_MASK)
            && (om_flash->INT_EN & SF_INT_EN_CMD_DONE_MASK)) {
        // Clear cmd done status
        om_flash->RAW_INT_STATUS = SF_RAW_INT_STATUS_CMD_DONE_MASK;
        // Disable interrupt
        om_flash->INT_EN = 0;
        if (state == FLASH_STATE_WRITING) {
            env->write_int_s = FLASH_TRANS_IDLE;
            if (env->data_cnt >= env->data_len) {
                env->data -= env->data_cnt;
                isr_evt = DRV_EVENT_FLASH_WRITE_TRANSFER_COMPLETED;
            } else {
                // Writing not finished, wait for write continue
                return;
            }
        } else if (state == FLASH_STATE_READING) {
            isr_evt = DRV_EVENT_COMMON_READ_COMPLETED;
        }
        env->state = FLASH_STATE_INIT;
        drv_iflash_isr_callback(om_flash, isr_evt);
    }
}

#endif /* (RTE_FLASH) */

/** @} */
