/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @file     drv_psram.c
 * @brief    PSRAM  driver
 * @date     30 September 2024
 * @author   OnMicro SW Team
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
#include "om_driver.h"
#if (RTE_PSRAM)


/*******************************************************************************
 * MACROS
 */
#define M_SPI                   0 /* Do not modify, must be 0 */
#define M_QPI                   1 /* Do not modify, must be 1 */
#define PSRAM_TIMEOUT_DEFAULT   1000


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    uint8_t *data;
    uint32_t data_num;
    uint32_t addr;
    drv_isr_callback_t isr_cb;
    psram_cmd_t read_cmd;
    psram_cmd_t write_cmd;
    uint8_t xpi_mode;
    drv_state_t state;
} psram_env_t;


/*******************************************************************************
 * CONST & VARIABLES
*/
static psram_env_t psram_env = {
    .isr_cb    = NULL,
    .data      = NULL,
    .data_num  = 0,
    .addr      = 0,
    .xpi_mode  = M_SPI,
    .read_cmd  = PSRAM_RESET,
    .write_cmd = PSRAM_RESET,
    .state     = DRV_STATE_UNINIT,
};

static const ospi_frame_t qpi_frame_config[] = {
    // cmd                   (cmd_bus,cmd_bit,addr_bus,addr_bit,dummy,data_bus)
    // QSPI commands
    {0x03U, OSPI_FRAME_CONFIG(BUS_1BIT, 8, BUS_1BIT, 24, 0, BUS_1BIT)},    // PSRAM_READ,
    {0x0BU, OSPI_FRAME_CONFIG(BUS_1BIT, 8, BUS_1BIT, 24, 8, BUS_1BIT)},    // PSRAM_FAST_READ,
    {0xEBU, OSPI_FRAME_CONFIG(BUS_1BIT, 8, BUS_4BIT, 24, 6, BUS_4BIT)},    // PSRAM_FAST_READ_QUAD,
    {0x02U, OSPI_FRAME_CONFIG(BUS_1BIT, 8, BUS_1BIT, 24, 0, BUS_1BIT)},    // PSRAM_WRITE,
    {0x38U, OSPI_FRAME_CONFIG(BUS_1BIT, 8, BUS_4BIT, 24, 0, BUS_4BIT)},    // PSRAM_QUAD_WRITE,
    {0x35U, OSPI_FRAME_CONFIG(BUS_1BIT, 8, BUS_1BIT, 0,  0, BUS_1BIT)},    // PSRAM_ENTER_QUAD,
    {0xF5U, OSPI_FRAME_CONFIG(       0, 0,        0, 0,  0,        0)},    // PSRAM_EXIT_QUAD(DO NOT USE),
    {0x66U, OSPI_FRAME_CONFIG(BUS_1BIT, 8, BUS_1BIT, 0,  0, BUS_1BIT)},    // PSRAM_RESET_ENABLE,
    {0x99U, OSPI_FRAME_CONFIG(BUS_1BIT, 8, BUS_1BIT, 0,  0, BUS_1BIT)},    // PSRAM_RESET,
    {0xC0U, OSPI_FRAME_CONFIG(BUS_1BIT, 8, BUS_1BIT, 0,  0, BUS_1BIT)},    // PSRAM_SET_BURST_LEN,
    {0x9FU, OSPI_FRAME_CONFIG(BUS_1BIT, 8, BUS_1BIT, 24, 0, BUS_1BIT)},    // PSRAM_READ_ID,
    // QPI commands
    {0x03U, OSPI_FRAME_CONFIG(       0, 0,        0, 0,  0,        0)},    // PSRAM_READ(DO NOT USE),
    {0x0BU, OSPI_FRAME_CONFIG(BUS_4BIT, 8, BUS_4BIT, 24, 4, BUS_4BIT)},    // PSRAM_FAST_READ,
    {0xEBU, OSPI_FRAME_CONFIG(BUS_4BIT, 8, BUS_4BIT, 24, 6, BUS_4BIT)},    // PSRAM_FAST_READ_QUAD,
    {0x02U, OSPI_FRAME_CONFIG(BUS_4BIT, 8, BUS_4BIT, 24, 0, BUS_4BIT)},    // PSRAM_WRITE,
    {0x38U, OSPI_FRAME_CONFIG(BUS_4BIT, 8, BUS_4BIT, 24, 0, BUS_4BIT)},    // PSRAM_QUAD_WRITE,
    {0x35U, OSPI_FRAME_CONFIG(       0, 0,        0, 0,  0,        0)},    // PSRAM_ENTER_QUAD(DO NOT USE),
    {0xF5U, OSPI_FRAME_CONFIG(BUS_4BIT, 8, BUS_4BIT, 0,  0, BUS_4BIT)},    // PSRAM_EXIT_QUAD,
    {0x66U, OSPI_FRAME_CONFIG(BUS_4BIT, 8, BUS_4BIT, 0,  0, BUS_4BIT)},    // PSRAM_RESET_ENABLE,
    {0x99U, OSPI_FRAME_CONFIG(BUS_4BIT, 8, BUS_4BIT, 0,  0, BUS_4BIT)},    // PSRAM_RESET,
    {0xC0U, OSPI_FRAME_CONFIG(BUS_4BIT, 8, BUS_4BIT, 0,  0, BUS_4BIT)},    // PSRAM_SET_BURST_LEN,
    {0x9FU, OSPI_FRAME_CONFIG(       0, 0,        0, 0,  0,        0)},    // PSRAM_READ_ID(DO NOT USE),
};


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static void psram_read_cmd_get(uint8_t *data, uint32_t data_len, uint32_t cmd[2])
{
    psram_env_t *env = &psram_env;
    uint8_t *pcmd1 = (uint8_t *)(cmd + 1);

    if (env->xpi_mode == M_SPI) {
        cmd[0] = (qpi_frame_config[env->read_cmd].cmd << 24);
    } else {
        cmd[0] = (qpi_frame_config[env->read_cmd + (uint32_t)PSRAM_QPI_CMD_MAX].cmd << 24);
    }
    cmd[1] = 0;
    if (data_len > 0 && data_len < 4) {
        for (uint8_t i = 0; i < data_len; i++) {
            pcmd1[2 - i] = data[data_len - 1 - i];
        }
    }
}

static void psram_write_cmd_get(uint8_t *data, uint32_t data_len, uint32_t cmd[2])
{
    psram_env_t *env = &psram_env;

    if (env->xpi_mode == M_SPI) {
        cmd[0] = (qpi_frame_config[env->write_cmd].cmd << 24);
    } else {
        cmd[0] = (qpi_frame_config[env->write_cmd + (uint32_t)PSRAM_QPI_CMD_MAX].cmd << 24);
    }

    if (data != NULL) {
        // data should be set from 3rd byte of cmd[1], as address is 24bit
        uint32_t data_u32 = (data[0] << 0) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
        cmd[1] = data_u32 << (24 - (data_len << 3));
    }
}

static void psram_read_cmd_set(psram_cmd_t read_cmd)
{
    psram_env_t *env = &psram_env;

    if (env->xpi_mode == M_SPI) {
        drv_ospi_read_frame_set(OM_OSPI1, &qpi_frame_config[read_cmd]);
    } else {
        drv_ospi_read_frame_set(OM_OSPI1, &qpi_frame_config[read_cmd + (uint32_t)PSRAM_QPI_CMD_MAX]);
    }
    // store xpi_mode for read
    env->read_cmd = read_cmd;
}

static void psram_write_cmd_set( psram_cmd_t write_cmd)
{
    psram_env_t *env = &psram_env;

    if (env->xpi_mode == M_SPI) {
        drv_ospi_write_frame_set(OM_OSPI1, &qpi_frame_config[write_cmd]);
    } else {
        drv_ospi_write_frame_set(OM_OSPI1, &qpi_frame_config[write_cmd + (uint32_t)PSRAM_QPI_CMD_MAX]);
    }
    // store xpi_mode for write
    env->write_cmd = write_cmd;
}

static om_error_t psram_write_reg(uint8_t *data, uint32_t data_len, psram_cmd_t qpi_cmd)
{
    psram_env_t *env = &psram_env;
    om_error_t error;
    uint32_t cmd[2] = {0};
    psram_cmd_t psram_read_mode;

    if (data_len > 3) {
        return OM_ERROR_UNSUPPORTED;
    }
    // store write mode
    psram_read_mode = env->read_cmd;
    // set read frame
    psram_read_cmd_set(qpi_cmd);
    // when read/write without dma, controller will use read sw_cfg
    psram_read_cmd_get(data, data_len, cmd);
    // program process
    error = drv_ospi_read(OM_OSPI1, cmd, NULL, 0);
    // restore write mode
    psram_read_cmd_set(psram_read_mode);

    return error;
}

static int32_t psram_auto_delay_init(ospi_config_t *config)
{
    psram_id_t id1, id2;
    int32_t delayi, delay1 = -1, delay2 = PSRAM_DELAY_MAX;
    ospi_config_t ospicfg = *config;
    uint8_t retry_cnt = 0;
    om_error_t error = OM_ERROR_FAIL;

    // set flash low frequency so it can work
    ospicfg.clk_div = 16;
    ospicfg.sample_cfg.sdr_async.sdr_async_dly = PSRAM_DELAY_DEFAULT;
    drv_ospi_init(OM_OSPI1, &ospicfg);
    // read id twice, save the true id to id1
    while (!((drv_psram_read_id(OM_OSPI1, &id1) == OM_ERROR_OK) &&
             (drv_psram_read_id(OM_OSPI1, &id2) == OM_ERROR_OK) &&
             (!memcmp(&id1, &id2, sizeof(psram_id_t))))) {
        if (++retry_cnt > PSRAM_AUTO_DLY_RETYR_CNT) {
            return error;
        }
    }
    // poll, delayi from 0 to DRV_SF_DELAY_MAX
    for (delayi = 0; delayi <= FLASH_DELAY_MAX; ++delayi) {
        ospicfg.clk_div = config->clk_div;
        ospicfg.sample_cfg.sdr_async.sdr_async_dly = delayi;
        drv_ospi_init(OM_OSPI1, &ospicfg);
        if ((drv_psram_read_id(OM_OSPI1, &id2) == OM_ERROR_OK) &&
                (!memcmp(&id1, &id2, sizeof(psram_id_t)))) {
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
        ospicfg.clk_div = 16;
        ospicfg.sample_cfg.sdr_async.sdr_async_dly = PSRAM_DELAY_DEFAULT;
    } else {
        ospicfg.sample_cfg.sdr_async.sdr_async_dly = (delay1 + delay2) / 2;
        error = OM_ERROR_OK;
    }
    drv_ospi_init(OM_OSPI1, &ospicfg);
    return error;
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
om_error_t drv_psram_init(OM_OSPI_Type *om_psram, const psram_config_t *psram_config)
{
    psram_env_t *env = &psram_env;
    om_error_t error;
    ospi_config_t ospi_config = {
        .cs_config          = NULL,
        .mode               = OSPI_MODE_0,
        .sdr_async_en       = 1,
        .sample_cfg.sdr_async.sdr_async_dly
                            = psram_config->delay,
        .clk_div            = psram_config->clk_div,
        .page_cross_en      = psram_config->page_cross_en,
        .page_size          = psram_config->page_size,
        .rw_data_width      = 0,
        .opcode_bypass_en   = 0,
        .decrypt_en         = 0,
        .encrypt_en         = 0,
        .is_normal_protocol = 1,
        .is_4bytes_addr     = 0,
    };
    env->state = DRV_STATE_INIT;
    // auto delay
    if (psram_config->delay == PSRAM_DELAY_AUTO) {
        if (psram_auto_delay_init(&ospi_config) != OM_ERROR_OK) {
            return OM_ERROR_HARDWARE;
        }
    } else {
        drv_ospi_init(om_psram, &ospi_config);
    }
    psram_read_cmd_set(psram_config->read_cmd);
    psram_write_cmd_set(psram_config->write_cmd);
    // force exit quad mode
    env->xpi_mode = M_QPI;
    if ((error = drv_psram_quad_mode_enable(om_psram, 0)) != OM_ERROR_OK) {
        return error;
    }
    #if (RTE_PSRAM_REGISTER_CALLBACK)
    drv_ospi_register_isr_callback(om_psram, (drv_isr_callback_t)drv_psram_isr_callback);
    #endif
    return error;
}

om_error_t drv_psram_read(OM_OSPI_Type *om_psram, uint32_t addr, uint8_t *data, uint32_t data_len)
{
    psram_env_t *env = &psram_env;
    uint32_t cmd[2] = {0};
    om_error_t error;

    if (env->state != DRV_STATE_INIT) {
        return OM_ERROR_BUSY;
    }
    env->state = DRV_STATE_RX;
    psram_read_cmd_get((uint8_t *)&addr, 3, cmd);
    error = drv_ospi_read(om_psram, cmd, data, data_len);
    env->state = DRV_STATE_INIT;
    return error;
}

om_error_t drv_psram_read_int(OM_OSPI_Type *om_psram, uint32_t addr, uint8_t *data, uint32_t data_len)
{
    psram_env_t *env = &psram_env;
    uint32_t cmd[2] = {0};
    om_error_t error;

    if (env->state != DRV_STATE_INIT) {
        return OM_ERROR_BUSY;
    }
    env->state = DRV_STATE_RX;
    psram_read_cmd_get((uint8_t *)&addr, 3, cmd);
    env->data = data;
    env->data_num = data_len;
    env->addr = addr;
    error = drv_ospi_read_int(om_psram, cmd, data, data_len);
    return error;
}

om_error_t drv_psram_write(OM_OSPI_Type *om_psram, uint32_t addr, uint8_t *data, uint32_t data_len)
{
    psram_env_t *env = &psram_env;
    uint32_t cmd[2] = {0};
    om_error_t error;

    if (env->state != DRV_STATE_INIT) {
        return OM_ERROR_BUSY;
    }
    env->state = DRV_STATE_TX;
    psram_write_cmd_get((uint8_t *)&addr, 3, cmd);
    error = drv_ospi_write(om_psram, cmd, data, data_len);
    env->state = DRV_STATE_INIT;
    return error;
}

om_error_t drv_psram_write_int(OM_OSPI_Type *om_psram, uint32_t addr, uint8_t *data, uint32_t data_len)
{
    psram_env_t *env = &psram_env;
    uint32_t cmd[2] = {0};
    om_error_t error;

    if (env->state != DRV_STATE_INIT) {
        return OM_ERROR_BUSY;
    }
    env->state = DRV_STATE_TX;
    psram_write_cmd_get((uint8_t *)&addr, 3, cmd);
    env->data = data;
    env->data_num = data_len;
    env->addr = addr;
    error = drv_ospi_write_int(om_psram, cmd, data, data_len);
    return error;
}

om_error_t drv_psram_read_id(OM_OSPI_Type *om_psram, psram_id_t *id)
{
    psram_env_t *env = &psram_env;
    psram_cmd_t psram_read_mode = env->read_cmd;
    om_error_t error;

    if (env->state != DRV_STATE_INIT) {
        return OM_ERROR_BUSY;
    }
    // check current status, should be spi status
    if (env->xpi_mode != M_SPI) {
        return OM_ERROR_PERMISSION;
    }
    psram_read_cmd_set(PSRAM_READ_ID);
    // issue a dummy read id
    error = drv_psram_read(om_psram, 0, NULL, 0);
    error = drv_psram_read(om_psram, 0, (uint8_t *)id, 8);
    psram_read_cmd_set(psram_read_mode);
    return error;
}

om_error_t drv_psram_read_cmd_set(OM_OSPI_Type *om_psram, psram_cmd_t read_cmd)
{
    psram_env_t *env = &psram_env;

    if (env->state != DRV_STATE_INIT) {
        return OM_ERROR_BUSY;
    }
    psram_read_cmd_set(read_cmd);
    return OM_ERROR_OK;
}

om_error_t drv_psram_write_cmd_set(OM_OSPI_Type *om_psram, psram_cmd_t write_cmd)
{
    psram_env_t *env = &psram_env;

    if (env->state != DRV_STATE_INIT) {
        return OM_ERROR_BUSY;
    }
    psram_write_cmd_set(write_cmd);
    return OM_ERROR_OK;
}

om_error_t drv_psram_quad_mode_enable(OM_OSPI_Type *om_psram, uint8_t enable)
{
    psram_env_t *env = &psram_env;
    om_error_t error;
    psram_cmd_t cmd = enable ? PSRAM_ENTER_QUAD : PSRAM_EXIT_QUAD;

    // check current status, should be spi status
    if ((enable && env->xpi_mode != M_SPI) || ((!enable) && env->xpi_mode != M_QPI)) {
        return OM_ERROR_PERMISSION;
    }
    // enter qpi status
    if ((error = psram_write_reg(0, 0, cmd)) == OM_ERROR_OK) {
        env->xpi_mode = enable ? M_QPI : M_SPI;
        psram_read_cmd_set(env->read_cmd);
        psram_write_cmd_set(env->write_cmd);
    }
    return error;
}

om_error_t drv_psram_reset(OM_OSPI_Type *om_psram)
{
    om_error_t error;

    if ((error = psram_write_reg(0, 0, PSRAM_RESET_ENABLE)) != OM_ERROR_OK) {
        return error;
    }
    return psram_write_reg(0, 0, PSRAM_RESET);
}

om_error_t drv_psram_set_burst_len(OM_OSPI_Type *om_psram)
{
    return psram_write_reg(0, 0, PSRAM_SET_BURST_LEN);
}

om_error_t drv_psram_list_start(OM_OSPI_Type *om_psram, psram_list_node_t *list_head)
{
    psram_env_t *env = &psram_env;
    uint8_t dummy;

    if (env->state != DRV_STATE_INIT || drv_ospi_is_busy(om_psram)) {
        return OM_ERROR_BUSY;
    }
    if (env->xpi_mode == M_SPI) {
        om_psram->CMD_DATA0 = qpi_frame_config[env->read_cmd].cmd << 24;
        om_psram->CMD_WR_DATA0 = qpi_frame_config[env->write_cmd].cmd << 24;
    } else {
        om_psram->CMD_DATA0 = qpi_frame_config[env->read_cmd + (uint32_t)PSRAM_QPI_CMD_MAX].cmd << 24;
        om_psram->CMD_WR_DATA0 = qpi_frame_config[env->write_cmd + (uint32_t)PSRAM_QPI_CMD_MAX].cmd << 24;
    }
    drv_ospi_list_start(om_psram, list_head);
    // a dummy read or write is required for activating list operation
    return drv_psram_read(om_psram, 0, &dummy, 1);
}

#if (RTE_PSRAM_REGISTER_CALLBACK)
void drv_psram_register_isr_callback(OM_OSPI_Type *om_psram, drv_isr_callback_t isr_cb)
{
    psram_env_t *env = &psram_env;
    env->isr_cb = isr_cb;
}

__WEAK void drv_psram_isr_callback(OM_OSPI_Type *om_psram, drv_event_t event)
{
    psram_env_t *env = &psram_env;
    drv_state_t state = env->state;
    drv_event_t isr_evt = DRV_EVENT_COMMON_NONE;

    env->state = DRV_STATE_INIT;

    if (event & DRV_EVENT_COMMON_TRANSFER_COMPLETED) {
        if (state == DRV_STATE_TX) {
            isr_evt |= DRV_EVENT_COMMON_WRITE_COMPLETED;
        } else if (state == DRV_STATE_RX) {
            isr_evt |= DRV_EVENT_COMMON_READ_COMPLETED;
        }
    }

    if (event & DRV_EVENT_OSPI_LIST_COMPLETED) {
        // list completed
        isr_evt |= DRV_EVENT_PSRAM_LIST_COMPLETED;
    }
    if (event & DRV_EVENT_OSPI_LIST_NODE_COMPLETED) {
        // node completed
        isr_evt |= DRV_EVENT_PSRAM_LIST_NODE_COMPLETED;
    }

    if (isr_evt && env->isr_cb) {
        env->isr_cb((void *)om_psram, isr_evt, (void *)(env->data), (void *)(env->data_num));
    }
}
#endif /* RTE_PSRAM_REGISTER_CALLBACK */

#endif  /* RTE_PSRAM */

/** @} */
