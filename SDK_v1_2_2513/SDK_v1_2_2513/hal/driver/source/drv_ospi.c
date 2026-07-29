/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @file     drv_ospi.c
 * @brief    OSPI driver for external memory
 * @date     8 September 2024
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
#include "RTE_driver.h"
#if (RTE_OSPI1)
#include <stddef.h>
#include "om_device.h"
#include "om_driver.h"
#include "om_utils.h"


/*******************************************************************************
 * MACROS
 */
#if (RTE_FLASH1 && RTE_FLASH1_XIP)
#define __OSPI_CODE                __RAM_CODE
#else
#define __OSPI_CODE
#endif


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    uint32_t is_valid;          // valid status
    uint32_t CMD_DATA0;         // offset address 0x10, ospi command data0 register
    uint32_t CMD_DATA1;         // offset address 0x14, ospi command data1 register
    uint32_t TRANSP_REMAP;      // offset address 0x38, ospi transparent remap register
    uint32_t SW_CFG0;           // offset address 0x40, ospi cfg0 register
    uint32_t SW_CFG1;           // offset address 0x44, ospi cfg1 register
    uint32_t MEM_TYPE0;         // offset address 0x54, ospi external memory 0 information register
    uint32_t SW_WR_CFG0;        // offset address 0x64, ospi write cfg0 register
    uint32_t SW_WR_CFG1;        // offset address 0x68, ospi write cfg1 register
    uint32_t CMD_WR_DATA0;      // offset address 0x80, ospi command data0 for write
} ospi_store_reg_t;

typedef struct {
    ospi_store_reg_t store_reg; // ospi register store for sleep
    drv_isr_callback_t isr_cb;
} ospi_env_t;

typedef struct {
    OM_OSPI_Type *reg;
    IRQn_Type irq_num;
    uint8_t irq_prio;
} ospi_resource_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
static ospi_env_t ospi_env;
static const ospi_resource_t ospi_resource = {
    .reg      = OM_OSPI1,
    .irq_num  = OSPI1_IRQn,
    .irq_prio = RTE_OSPI1_IRQ_PRIORITY,
};


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static __OSPI_CODE void ospi_raw_read(OM_OSPI_Type *om_ospi,
                                      uint32_t cmd[2],
                                      uint8_t *data,
                                      uint32_t data_len)
{
    OM_CRITICAL_BEGIN();
    om_ospi->CMD_DATA0 = cmd[0];
    om_ospi->CMD_DATA1 = cmd[1];
    om_ospi->ADDR = (uint32_t)data;
    register_set(&om_ospi->SW_CFG1, MASK_1REG(OSPI_SW_CFG1_SDATA_BYTE_CNT, data_len));
    om_ospi->RAW_INT_STATUS = OSPI_RAW_INT_STATUS_SRIS_MASK;
    register_set_raw(&om_ospi->CMD, MASK_1REG(OSPI_CMD_COMMAND, 1U));
    OM_CRITICAL_END();
}

static __OSPI_CODE void ospi_raw_write(OM_OSPI_Type *om_ospi,
                                       uint32_t cmd[2],
                                       volatile uint8_t *data,
                                       uint32_t data_len)
{
    OM_CRITICAL_BEGIN();
    om_ospi->CMD_WR_DATA0 = cmd[0];
    om_ospi->CMD_DATA1 = cmd[1];
    om_ospi->ADDR = (uint32_t)data;
    register_set(&om_ospi->SW_WR_CFG1, MASK_1REG(OSPI_SW_CFG1_SDATA_BYTE_CNT, data_len));
    om_ospi->RAW_INT_STATUS = OSPI_RAW_INT_STATUS_SRIS_MASK;
    om_ospi->CMD = 2U << OSPI_CMD_COMMAND_POS;
    OM_CRITICAL_END();
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
__OSPI_CODE void drv_ospi_init(OM_OSPI_Type *om_ospi, const ospi_config_t *ospi_config)
{
    const ospi_resource_t *resource = &ospi_resource;
    uint8_t bypass;
    uint8_t clk_div = ospi_config->clk_div;
    uint8_t cpol = 0;
    uint8_t cpha = 0;
    uint8_t falling_sample = 0;
    uint8_t delay_sample = 0;
    uint8_t sdr_async_delay = 0;

    if (ospi_config != NULL) {
        DRV_RCC_CLOCK_ENABLE(RCC_CLK_OSPI1, 1);
        // in mode 3, the ospi could not enable bypass
        if ((ospi_config->mode == OSPI_MODE_3) && (clk_div < 2)) {
            clk_div = 2;
            bypass = 0;
        } else {
            bypass = (clk_div < 2) ? 1 : 0;
            // clock divider should be even and at least 2
            clk_div = (clk_div < 2) ? 2 : (clk_div & (~(0x1)));
        }
        // set cpol and cpha by ospi_mode
        switch (ospi_config->mode) {
            case OSPI_MODE_0:
                cpol = 0, cpha = 0;
                break;
            case OSPI_MODE_1:
                cpol = 0, cpha = 1;
                break;
            case OSPI_MODE_2:
                cpol = 1, cpha = 0;
                break;
            case OSPI_MODE_3:
                cpol = 1, cpha = 1;
                break;
            default:
                break;
        }
        if (ospi_config->sdr_async_en) {
            sdr_async_delay = ospi_config->sample_cfg.sdr_async.sdr_async_dly;
            // if the clock is greater than 12.5MHz, sdr async delay must be greater than 1
            if ((sdr_async_delay == 0) && (drv_rcc_clock_get(RCC_CLK_OSPI1) / (bypass ? 1 : clk_div)) >= 12500000U) {
                sdr_async_delay = 1;
            }
        } else {
            falling_sample = ((clk_div < 2U && ospi_config->sample_cfg.sdr_sync.delay_sample == 0) ||
                              (cpol != cpha)) ? 0 : ospi_config->sample_cfg.sdr_sync.falling_sample;
            delay_sample = ospi_config->sample_cfg.sdr_sync.delay_sample;
        }
        // set config register
        register_set_raw(&om_ospi->CFG[0].SPI_CFG,
                         MASK_9REG(OSPI_CFG_CLK_DIV, clk_div,
                                   OSPI_CFG_CPHA, cpha,
                                   OSPI_CFG_CPOL, cpol,
                                   OSPI_CFG_BPC, bypass,
                                   OSPI_CFG_SDR_ASYNC_EN, ospi_config->sdr_async_en,
                                   OSPI_CFG_SDR_ASYNC_DLY, sdr_async_delay,
                                   OSPI_CFG_DLY, delay_sample,
                                   OSPI_CFG_FDS, falling_sample,
                                   OSPI_CFG_WIDTH, ospi_config->rw_data_width));

        register_set(&om_ospi->MEM_TYPE0,
                     MASK_5REG(OSPI_MEM_TYPE_PAGE_SIZE, (ospi_config->page_size - 1),
                               OSPI_MEM_TYPE_PAGE_CROSS_EN, ospi_config->page_cross_en,
                               OSPI_MEM_TYPE_PROTOCAL, (ospi_config->is_normal_protocol ? 0U : 1U),
                               OSPI_MEM_TYPE_BPO, ospi_config->opcode_bypass_en,
                               OSPI_MEM_TYPE_ADDR_WIDTH, ospi_config->is_4bytes_addr));
        // TRANSP_REMAP will be ANDed to the address in transparent read or write when the MSb is 1.
        // 3FFFFFF is the aviliable address space mask which can clear unused address bit
        om_ospi->TRANSP_REMAP = 0x83FFFFFFU;
        if (ospi_config->cs_config) {
            drv_ospi_control(om_ospi, OSPI_CONTROL_CS_CONFIG, ospi_config->cs_config);
        }
        om_ospi->SEC_CTRL = (ospi_config->encrypt_en ? OSPI_SEC_CTRL_ENC_MASK : 0U) +
                            (ospi_config->decrypt_en ? OSPI_SEC_CTRL_DEC_MASK : 0U);
        om_ospi->OPCODE = (ospi_config->read_opcode << OSPI_OPCODE_RD0_POS) +
                            (ospi_config->write_opcode << OSPI_OPCODE_WR0_POS);
        om_ospi->SW_CFG0 = ospi_config->read_frame_cfg[0];
        om_ospi->SW_CFG1 = ospi_config->read_frame_cfg[1];
        om_ospi->SW_WR_CFG0 = ospi_config->write_frame_cfg[0];
        om_ospi->SW_WR_CFG1 = ospi_config->write_frame_cfg[1];
        // disable interrupts of ospi1
        om_ospi->INT_EN = 0;
        // clear and enable nvic interrupt
        NVIC_SetPriority(resource->irq_num, resource->irq_prio);
        NVIC_ClearPendingIRQ(resource->irq_num);
        NVIC_EnableIRQ(resource->irq_num);
    }
}

__OSPI_CODE om_error_t drv_ospi_read(OM_OSPI_Type *om_ospi,
                                     uint32_t cmd[2],
                                     uint8_t *data,
                                     uint32_t data_len)
{
    if (om_ospi->STATUS & OSPI_STATUS_BUSY_MASK) {
        return OM_ERROR_BUSY;
    }

    ospi_raw_read(om_ospi, cmd, data, data_len);
    // wait for transfer done
    while (!(om_ospi->RAW_INT_STATUS & OSPI_RAW_INT_STATUS_SRIS_MASK));
    om_ospi->RAW_INT_STATUS = OSPI_RAW_INT_STATUS_SRIS_MASK;
    return OM_ERROR_OK;
}

om_error_t drv_ospi_read_int(OM_OSPI_Type *om_ospi,
                             uint32_t cmd[2],
                             uint8_t *data,
                             uint32_t data_len)
{
    if (om_ospi->STATUS & OSPI_STATUS_BUSY_MASK) {
        return OM_ERROR_BUSY;
    }
    // enable interrupt
    register_set1(&om_ospi->INT_EN, OSPI_INT_MASK_CDM_MASK);
    ospi_raw_read(om_ospi, cmd, data, data_len);
    return OM_ERROR_OK;
}

__OSPI_CODE om_error_t drv_ospi_write(OM_OSPI_Type *om_ospi,
                                      uint32_t cmd[2],
                                      volatile uint8_t *data,
                                      uint32_t data_len)
{
    if (om_ospi->STATUS & OSPI_STATUS_BUSY_MASK) {
        return OM_ERROR_BUSY;
    }
    ospi_raw_write(om_ospi, cmd, data, data_len);
    // wait for transfer done
    while (!(om_ospi->RAW_INT_STATUS & OSPI_RAW_INT_STATUS_SRIS_MASK));
    om_ospi->RAW_INT_STATUS = OSPI_RAW_INT_STATUS_SRIS_MASK;
    return OM_ERROR_OK;
}

om_error_t drv_ospi_write_int(OM_OSPI_Type *om_ospi,
                              uint32_t cmd[2],
                              volatile uint8_t *data,
                              uint32_t data_len)
{
    if (om_ospi->STATUS & OSPI_STATUS_BUSY_MASK) {
        return OM_ERROR_BUSY;
    }
    // enable interrupt
    register_set1(&om_ospi->INT_EN, OSPI_INT_MASK_CDM_MASK);
    ospi_raw_write(om_ospi, cmd, data, data_len);
    return OM_ERROR_OK;
}

__OSPI_CODE void drv_ospi_store(void)
{
    OM_OSPI_Type *om_ospi = ospi_resource.reg;
    ospi_store_reg_t *store = &(ospi_env.store_reg);

    DRV_RCC_CLOCK_ENABLE(RCC_CLK_OSPI1, 1U);
    OM_CRITICAL_BEGIN();
    store->CMD_DATA0 = om_ospi->CMD_DATA0;
    store->CMD_DATA1 = om_ospi->CMD_DATA1;
    store->TRANSP_REMAP = om_ospi->TRANSP_REMAP;
    store->SW_CFG0 = om_ospi->SW_CFG0;
    store->SW_CFG1 = om_ospi->SW_CFG1;
    store->MEM_TYPE0 = om_ospi->MEM_TYPE0;
    store->SW_WR_CFG0 = om_ospi->SW_WR_CFG0;
    store->SW_WR_CFG1 = om_ospi->SW_WR_CFG1;
    store->CMD_WR_DATA0 = om_ospi->CMD_WR_DATA0;
    store->is_valid = 1;
    OM_CRITICAL_END();
    #if (!RTE_FLASH1_XIP)
    DRV_RCC_CLOCK_ENABLE(RCC_CLK_OSPI1, 0U);
    #endif
}

__OSPI_CODE void drv_ospi_restore(void)
{
    OM_OSPI_Type *om_ospi = ospi_resource.reg;
    ospi_store_reg_t *store = &(ospi_env.store_reg);

    if (!store->is_valid) {
        return;
    }
    DRV_RCC_CLOCK_ENABLE(RCC_CLK_OSPI1, 1U);
    OM_CRITICAL_BEGIN();
    om_ospi->CMD_DATA0 = store->CMD_DATA0;
    om_ospi->CMD_DATA1 = store->CMD_DATA1;
    om_ospi->TRANSP_REMAP = store->TRANSP_REMAP;
    om_ospi->SW_CFG0 = store->SW_CFG0;
    om_ospi->SW_CFG1 = store->SW_CFG1;
    om_ospi->MEM_TYPE0 = store->MEM_TYPE0;
    om_ospi->SW_WR_CFG0 = store->SW_WR_CFG0;
    om_ospi->SW_WR_CFG1 = store->SW_WR_CFG1;
    om_ospi->CMD_WR_DATA0 = store->CMD_WR_DATA0;
    store->is_valid = 0;
    OM_CRITICAL_END();
}

void drv_ospi_node_setup(OM_OSPI_Type *om_ospi, ospi_list_node_t *node, ospi_list_node_cfg_t *cfg)
{
    (void)om_ospi;
    node->next_node_addr = cfg->next_node_addr;
    node->dev_addr = cfg->dev_addr;
    node->data_addr = (uint32_t)cfg->data;
    node->shadow_cfg = (cfg->data_len << OSPI_SHADOW_CFG_DATA_BYTE_CNT_POS) |
                       (cfg->node_int_en << OSPI_SHADOW_CFG_TRANS_INT_EN_POS) |
                       (cfg->list_int_en << OSPI_SHADOW_CFG_LIST_INT_EN_POS) |
                       (cfg->cmd_type << OSPI_SHADOW_CFG_RW_CMD_POS) |
                       ((!(cfg->list_end)) << OSPI_SHADOW_CFG_LLEN_POS) |
                       (1U << OSPI_SHADOW_CFG_TRANS_INT_POS) |
                       (1U << OSPI_SHADOW_CFG_LIST_INT_POS);
}

#if (RTE_OSPI1_REGISTER_CALLBACK)
void drv_ospi_register_isr_callback(OM_OSPI_Type *om_ospi, drv_isr_callback_t isr_cb)
{
    ospi_env_t *env = &ospi_env;
    env->isr_cb = isr_cb;
}
#endif

void drv_ospi_isr(OM_OSPI_Type *om_ospi)
{
    ospi_env_t *env = &ospi_env;
    // run here, it indicates that a transfer is completed, so one node is completed.
    // if list is not used, ignore this event.
    drv_event_t isr_evt = DRV_EVENT_OSPI_LIST_NODE_COMPLETED;

    // there are two conditions to check which signal triggered the interrupt:
    // 1. interrupt status is set
    // 2. interrupt is enabled
    if ((om_ospi->RAW_INT_STATUS & OSPI_RAW_INT_STATUS_LLI_MASK) &&
            (om_ospi->INT_EN & OSPI_INT_MASK_LLIM_MASK)) {
        // clear interrupt
        om_ospi->RAW_INT_STATUS = OSPI_RAW_INT_STATUS_LLI_MASK;
        // disable interrupt
        om_ospi->INT_EN &= (~OSPI_INT_MASK_LLIM_MASK);
        // set list completed event
        if (om_ospi->INT_EN & OSPI_INT_MASK_CDM_MASK) {
            isr_evt |= (DRV_EVENT_OSPI_LIST_COMPLETED );
        } else {
            isr_evt = DRV_EVENT_OSPI_LIST_COMPLETED;
        }
    }
    if ((om_ospi->RAW_INT_STATUS & OSPI_RAW_INT_STATUS_SRIS_MASK) &&
            (om_ospi->INT_EN & OSPI_INT_MASK_CDM_MASK)) {
        // clear cmd done status
        om_ospi->RAW_INT_STATUS = OSPI_RAW_INT_STATUS_SRIS_MASK;
        // disable interrupt
        om_ospi->INT_EN &= (~OSPI_INT_MASK_CDM_MASK);
        isr_evt |= DRV_EVENT_COMMON_TRANSFER_COMPLETED;
    }
    if (env->isr_cb) {
        env->isr_cb((void *)om_ospi, isr_evt, (void *)0, (void *)0);
    }
}

#endif  /* (RTE_OSPI1) */

/** @} */
