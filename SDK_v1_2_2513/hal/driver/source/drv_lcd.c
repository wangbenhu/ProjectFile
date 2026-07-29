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
 * @brief    LCD driver source file
 * @details  LCD driver source file
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
#if (RTE_LCD)
#include "om_device.h"
#include "om_driver.h"


/*******************************************************************************
 * TYPEDEFS
 */
/// LCD environment structure
typedef struct {
    drv_isr_callback_t      isr_cb;
} lcd_env_t;

/// LCD resource structure
typedef struct {
    uint32_t      cap;         /**< capabilities               */
    OM_LCD_Type  *reg;         /**< peripheral registers base  */
    lcd_env_t    *env;         /**< peripheral environment     */
    IRQn_Type     irq_num;     /**< peripheral IRQn_Type       */
    uint8_t       irq_prio;    /**< peripheral irq priority    */
} lcd_resource_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
static lcd_env_t lcd_env = {
    .isr_cb = NULL,
};

static lcd_resource_t lcd_resource = {
    .cap      = 0,
    .reg      = OM_LCD,
    .env      = &lcd_env,
    .irq_num  = LCD_IRQn,
    .irq_prio = RTE_LCD_IRQ_PRIORITY,
};


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static void cmd_format(uint8_t *cmd, uint8_t cmd_bits, uint32_t cmd32[2])
{
    if ((cmd_bits + 1U) > 31U) {
        cmd32[1] = cmd[7] | (cmd[6] << 8) | (cmd[5] << 16) | (cmd[4] << 24);
    } else {
        cmd32[1] = 0U;
    }
    cmd32[0] = cmd[3] | (cmd[2] << 8) | (cmd[1] << 16) | (cmd[0] << 24);
}

static void drv_lcd_raw_read(uint32_t *cmd, uint8_t cmd_bits, uint8_t *data, uint16_t data_len)
{
    OM_ASSERT(!(((uint32_t)data) & 0x3));  // data must be 4 bytes aligned while reading

    OM_LCD_Type *LCD = lcd_resource.reg;
    OM_ASSERT(cmd_bits > 1);
    if (cmd_bits > 32) {
        LCD->COMMAND_DATA1_REG = cmd[1];
        register_set(&LCD->SW_SPI_CFG_0, MASK_2REG(LCD_SW_SPI_CFG0_P1_BIT_CNT, cmd_bits-32,
                                                   LCD_SW_SPI_CFG0_P0_BIT_CNT, 32));
    } else {
        register_set(&LCD->SW_SPI_CFG_0, MASK_2REG(LCD_SW_SPI_CFG0_P1_BIT_CNT, 1,
                                                   LCD_SW_SPI_CFG0_P0_BIT_CNT, cmd_bits-1));
        // if COMMAND_DATA1_REG is not used, data will be left shifted 4 bytes
        LCD->COMMAND_DATA1_REG = cmd[0] << (cmd_bits -1);
    }
    LCD->COMMAND_DATA0_REG = cmd[0];
    LCD->ADDRESS_REG = (uint32_t)data;
    register_set(&LCD->SW_SPI_CFG_1, MASK_1REG(LCD_SW_SPI_CFG1_SDATA_BYTE_CNT, data_len));
    register_set(&LCD->COMMAND, MASK_3REG(LCD_COMMAND_NUM_OF_DATA_BYTES, data_len,
                                          LCD_COMMAND_NUM_OF_CMD_BITS,   cmd_bits,
                                          LCD_COMMAND_CMD,               LCD_COMMAND_CMD_READ));
}

/**
 *******************************************************************************
 * @brief lcd raw write
 *
 * @param[in] cmd           command
 * @param[in] cmd_bits      command bits to send, can be 1 to 64
 * @param[in] data          data to send
 * @param[in] data_len      data length to send, must less than 65536
 *
 * @return None
 *******************************************************************************
 */
static void drv_lcd_raw_write(uint32_t *cmd, uint8_t cmd_bits, const uint8_t *data, uint16_t data_len)
{
    OM_LCD_Type *LCD = lcd_resource.reg;
    OM_ASSERT(cmd_bits > 1);
    if (cmd_bits > 36) {
        LCD->COMMAND_DATA1_REG = cmd[1];
        register_set(&LCD->SW_SPI_CFG_0, MASK_2REG(LCD_SW_SPI_CFG0_P1_BIT_CNT, cmd_bits-32,
                                                   LCD_SW_SPI_CFG0_P0_BIT_CNT, 32));
    } else {
        register_set(&LCD->SW_SPI_CFG_0, MASK_2REG(LCD_SW_SPI_CFG0_P1_BIT_CNT, 4,
                                                   LCD_SW_SPI_CFG0_P0_BIT_CNT, cmd_bits - 4));
        // if COMMAND_DATA1_REG is not used, data will be left shifted 4 bytes
        LCD->COMMAND_DATA1_REG = cmd[0] << (cmd_bits - 4);
    }
    LCD->COMMAND_DATA0_REG = cmd[0];
    LCD->ADDRESS_REG = (uint32_t)data;
    register_set(&LCD->SW_SPI_CFG_1, MASK_1REG(LCD_SW_SPI_CFG1_SDATA_BYTE_CNT, data_len));
    register_set(&LCD->COMMAND, MASK_3REG(LCD_COMMAND_NUM_OF_DATA_BYTES,  data_len,
                                          LCD_COMMAND_NUM_OF_CMD_BITS,    cmd_bits,
                                          LCD_COMMAND_CMD,                LCD_COMMAND_CMD_WRITE));
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
void drv_lcd_init(const lcd_config_t *config)
{
    OM_LCD_Type *LCD = lcd_resource.reg;
    DRV_RCC_RESET(RCC_CLK_LCD);
    // Clear and Enable IRQ
    NVIC_SetPriority(lcd_resource.irq_num, lcd_resource.irq_prio);
    NVIC_ClearPendingIRQ(lcd_resource.irq_num);
    NVIC_EnableIRQ(lcd_resource.irq_num);
    register_set_raw(&LCD->CONFIG[0].SPI_CFG, MASK_7REG(LCD_CONFIG_TX_DATA_MODE, config->rgb_mode,
                                                        LCD_CONFIG_SPI_IF_MODE,  ((config->data_mode == LCD_DATA_MODE_3WM1_PARALLEL) ? LCD_DATA_MODE_3WM1 : config->data_mode),
                                                        LCD_CONFIG_DATA_WIDTH,   0,
                                                        LCD_CONFIG_DLY_SMP,      config->dly_sample,
                                                        LCD_CONFIG_BP_CLK_DIV,   (!(config->clk_div>>1)),
                                                        LCD_CONFIG_MODE,         config->spi_mode,
                                                        LCD_CONFIG_CLK_DIV,      config->clk_div));
    register_set(&LCD->SW_SPI_CFG_1, MASK_4REG(LCD_SW_SPI_CFG1_CFG_EN,          ((config->data_mode == LCD_DATA_MODE_FLASH_LIKE) ? 1 : 0),
                                               LCD_SW_SPI_CFG1_LIST_LOAD_EN,    0,
                                               LCD_SW_SPI_CFG1_BUF_WIDTH_BYTES, 4,
                                               LCD_SW_SPI_CFG1_SDATA_BUS_WIDTH, 0));
    register_set(&LCD->SW_SPI_CFG_0, MASK_3REG(LCD_SW_SPI_CFG0_DUMMY_CYCLE,   0,
                                               LCD_SW_SPI_CFG0_P1_BUS_WIDTH,  0,
                                               LCD_SW_SPI_CFG0_P0_BUS_WIDTH,  0));
    register_set(&LCD->COMMAND, MASK_1REG(LCD_COMMAND_DATA_2_LANE_EN, (config->data_mode == LCD_DATA_MODE_3WM1_PARALLEL) ? 1 : 0));
}

#if (RTE_LCD_REGISTER_CALLBACK)
/**
 *******************************************************************************
 * @brief Register isr callback for SPI command is complete by LCD interrupt mode
 *
 * @param[in] isr_cb          Pointer to callback
 *
 *******************************************************************************
 */
void drv_lcd_register_isr_callback(drv_isr_callback_t isr_cb)
{
    lcd_env.isr_cb = isr_cb;
}
#endif

__WEAK void drv_lcd_isr_callback(void)
{
    #if (RTE_LCD_REGISTER_CALLBACK)
    if (lcd_env.isr_cb) {
        lcd_env.isr_cb(lcd_resource.reg, DRV_EVENT_COMMON_GENERAL, NULL, NULL);
    }
    #endif
}

void drv_lcd_read(uint8_t *cmd, uint8_t cmd_bits, uint8_t *data, uint16_t data_len)
{
    OM_LCD_Type *om_lcd = lcd_resource.reg;
    uint32_t cmd32[2];

    om_lcd->INTR_MASK &= ~LCD_INTR_MASK_SPI_CMD_DONE_MASK;
    cmd_format(cmd, cmd_bits, cmd32);
    drv_lcd_raw_read(cmd32, cmd_bits, data, data_len);
    while(!(om_lcd->RAW_INTR_STATUS & LCD_RAW_INTR_STATUS_SPI_CMD_DONE_MASK));
    om_lcd->RAW_INTR_STATUS = LCD_RAW_INTR_STATUS_SPI_CMD_DONE_MASK;
}

void drv_lcd_read_int(uint8_t *cmd, uint8_t cmd_bits, uint8_t *data, uint16_t data_len)
{
    OM_LCD_Type *om_lcd = lcd_resource.reg;
    uint32_t cmd32[2];
    cmd_format(cmd, cmd_bits, cmd32);

    om_lcd->INTR_MASK |= LCD_INTR_MASK_SPI_CMD_DONE_MASK;
    drv_lcd_raw_read(cmd32, cmd_bits, data, data_len);
}

void drv_lcd_write(uint8_t *cmd, uint8_t cmd_bits, const uint8_t *data, uint16_t data_len)
{
    OM_LCD_Type *om_lcd = lcd_resource.reg;
    uint32_t cmd32[2];

    om_lcd->INTR_MASK &= ~LCD_INTR_MASK_SPI_CMD_DONE_MASK;
    cmd_format(cmd, cmd_bits, cmd32);
    drv_lcd_raw_write(cmd32, cmd_bits, data, data_len);
    while(!(om_lcd->RAW_INTR_STATUS & LCD_RAW_INTR_STATUS_SPI_CMD_DONE_MASK));
    om_lcd->RAW_INTR_STATUS = LCD_RAW_INTR_STATUS_SPI_CMD_DONE_MASK;
}

void drv_lcd_write_int(uint8_t *cmd, uint8_t cmd_bits, uint8_t *data, uint16_t data_len)
{
    OM_LCD_Type *om_lcd = lcd_resource.reg;
    uint32_t cmd32[2];
    cmd_format(cmd, cmd_bits, cmd32);
    om_lcd->INTR_MASK |= LCD_INTR_MASK_SPI_CMD_DONE_MASK;
    drv_lcd_raw_write(cmd32, cmd_bits, data, data_len);
}

void drv_lcd_control(lcd_control_t control, void *argu)
{
    OM_LCD_Type *LCD = lcd_resource.reg;

    switch (control) {
        case LCD_CONTROL_SET_DATA_WIDTH:
            do {
                uint32_t data_width;
                data_width = (uint32_t)argu;
                OM_ASSERT((data_width == 8) || (data_width == 16) || (data_width == 32));
                register_set(&LCD->CONFIG[0].SPI_CFG, MASK_1REG(LCD_CONFIG_DATA_WIDTH, data_width >> 4));
            } while(0);
            break;
        case LCD_CONTROL_TX_DATA_MODE:
            do {
                uint32_t tx_data_mode;
                tx_data_mode = (uint32_t)argu;
                OM_ASSERT((tx_data_mode == 8) || (tx_data_mode == 16) || (tx_data_mode == 32));
                register_set(&LCD->CONFIG[0].SPI_CFG, MASK_1REG(LCD_CONFIG_TX_DATA_MODE, ((tx_data_mode >> 3) - 1)));
            } while(0);
            break;
        case LCD_CONTROL_DATA_2_LANE_EN:
            do {
                uint32_t data_2_lane_en;
                data_2_lane_en = (uint32_t)argu;
                OM_ASSERT((data_2_lane_en == 0) || (data_2_lane_en == 1));
                register_set(&LCD->COMMAND, MASK_1REG(LCD_COMMAND_DATA_2_LANE_EN, data_2_lane_en));
            } while(0);
            break;
        case LCD_CONTROL_SET_CMD_BIT:
            do {
                uint32_t cmd_bit;

                cmd_bit = (uint32_t)argu;
                OM_ASSERT((cmd_bit == 1) || (cmd_bit == 2) || (cmd_bit == 4));
                register_set(&LCD->SW_SPI_CFG_0, MASK_2REG(LCD_SW_SPI_CFG0_P1_BUS_WIDTH, (cmd_bit >> 1),
                                                           LCD_SW_SPI_CFG0_P0_BUS_WIDTH, (cmd_bit >> 1)));
            } while(0);
            break;
        case LCD_CONTROL_SET_DATA_BIT:
            do {
                uint32_t data_bit;

                data_bit = (uint32_t)argu;
                OM_ASSERT((data_bit == 1) || (data_bit == 2) || (data_bit == 4));
                register_set(&LCD->SW_SPI_CFG_1, MASK_1REG(LCD_SW_SPI_CFG1_SDATA_BUS_WIDTH, (data_bit >> 1)));
            } while(0);
            break;
        case LCD_CONTROL_SET_BUF_MODE_666_888:
            register_set(&LCD->SW_SPI_CFG_1, MASK_1REG(LCD_SW_SPI_CFG1_BUF_WIDTH_BYTES, 3));
            break;
        case LCD_CONTROL_SET_BUF_MODE_COMMON:
            register_set(&LCD->SW_SPI_CFG_1, MASK_1REG(LCD_SW_SPI_CFG1_BUF_WIDTH_BYTES, 4));
            break;
        case LCD_CONTROL_KEEP_CS:
            LCD->COMMAND |= LCD_COMMAND_KEEP_CS_MASK;
            break;
        case LCD_CONTROL_RELEASE_CS:
            LCD->COMMAND &= ~LCD_COMMAND_KEEP_CS_MASK;
            break;
        case LCD_CONTROL_SET_DUMMY_CYCLES:
            OM_ASSERT((uint32_t)argu <= 0x7F);
            register_set(&(LCD->SW_SPI_CFG_0), MASK_1REG(LCD_SW_SPI_CFG0_DUMMY_CYCLE, (uint32_t)argu));
            break;
        case LCD_CONTROL_SET_CS_CONFIG:
            do {
                lcd_cs_config_t* config = (lcd_cs_config_t*)argu;
                register_set_raw(&LCD->CONFIG[0].CS_CFG, MASK_5REG(LCD_CONFIG_CS_RECOVER,    config->cs_recover,
                                                                   LCD_CONFIG_CS_HOLD_TIME,  config->cs_hold,
                                                                   LCD_CONFIG_CS_SETUP_TIME, config->cs_setup,
                                                                   LCD_CONFIG_CS_IGNORE_CNT, config->transp_head_ignore_cnt,
                                                                   LCD_CONFIG_CS_POL,        config->cs_pol));
            } while(0);
            break;
        default:
            break;
    }
}

void drv_lcd_isr(void)
{
    OM_LCD->RAW_INTR_STATUS |= LCD_RAW_INTR_STATUS_SPI_CMD_DONE_MASK;
    drv_lcd_isr_callback();
}

#endif  /* (RTE_LCD) */


/** @} */
