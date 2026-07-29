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
 * @brief    example for using i2c
 * @details  example for using i2c: read and write eeprom
 * @version
 *
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


/*******************************************************************************
 * INCLUDES
 */
#include "om_driver.h"


/*******************************************************************************
 * MACROS
 */
/// Test pad i2c scl
#define PAD_I2C0_SCL                    8
/// Test pad mux i2c scl
#define MUX_I2C0_SCL                    PINMUX_PAD8_I2C0_SCK_CFG
/// Test pad i2c sda
#define PAD_I2C0_SDA                    9
/// Test pad mux i2c sda
#define MUX_I2C0_SDA                    PINMUX_PAD9_I2C0_SDA_CFG

/* Use AT24C02 */
/// Address of eeprom
#define EEPROM_ADDR                     0x50U
/// Capacity of eeprom
#define EEPROM_CAPACITY                 256U
/// Test address in eeprom
#define TEST_ADDR                       0x40

#define WRITE_INT_IS_DONE           (1 << 0)
#define WRITE_DMA_IS_DONE           (1 << 1)
#define READ_INT_IS_DONE            (1 << 2)
#define READ_DMA_IS_DONE            (1 << 3)
#define READ_WRITE_IS_TIMEOUT       (1 << 4)
#define TXADDR_IS_NACK              (1 << 5)
#define RXADDR_IS_NACK              (1 << 6)
#define TXDATA_IS_NACK              (1 << 7)
#define RXDATA_IS_UNDER             (1 << 8)


/*******************************************************************************
 * TYPEDEFS
 */
typedef enum {
    I2C_ADDR_TYPE_7BIT  = 0,
    I2C_ADDR_TYPE_10BIT = 1,

    I2C_ADDR_TYPE_NUM,
} i2c_addr_type_t;

typedef struct {
    i2c_addr_type_t addr_type;
    volatile uint8_t st;
} test_i2c_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
/// Buffer that stores data to be sent
static uint8_t eeprom_tx_buf[EEPROM_CAPACITY];
/// Buffer that stores data to be read
static uint8_t eeprom_rx_buf[EEPROM_CAPACITY];
/// I2C pin configuration
static const pin_config_t pin_cfg [] = {
    {PAD_I2C0_SCL, {MUX_I2C0_SCL}, PMU_PIN_MODE_OD, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_I2C0_SDA, {MUX_I2C0_SDA}, PMU_PIN_MODE_OD, PMU_PIN_DRIVER_CURRENT_NORMAL},
};
static test_i2c_env_t test_i2c_env;


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static void i2c_isr_callback(void *om_reg, drv_event_t event, void *tx_buf, void *tx_cnt)
{
    if (event == DRV_EVENT_COMMON_WRITE_COMPLETED) {
        test_i2c_env.st |= WRITE_INT_IS_DONE;
    } else if (event == DRV_EVENT_COMMON_GPDMA2PERIPH_COMPLETED) {
        test_i2c_env.st |= WRITE_DMA_IS_DONE;
    } else if (event == DRV_EVENT_COMMON_READ_COMPLETED) {
        test_i2c_env.st |= READ_INT_IS_DONE;
        test_i2c_env.st |= READ_DMA_IS_DONE;
    }
    if ((event & DRV_EVENT_I2C_TIMEOUT) == DRV_EVENT_I2C_TIMEOUT) {
        test_i2c_env.st |=READ_WRITE_IS_TIMEOUT;
    }
    if ((event & DRV_EVENT_I2C_TXADDR_NACK) == DRV_EVENT_I2C_TXADDR_NACK) {
        test_i2c_env.st |= TXADDR_IS_NACK;
    }
    if ((event & DRV_EVENT_I2C_RXADDR_NACK) == DRV_EVENT_I2C_RXADDR_NACK) {
        test_i2c_env.st |= RXADDR_IS_NACK;
    }
    if ((event & DRV_EVENT_I2C_TXDATA_NACK) == DRV_EVENT_I2C_TXDATA_NACK) {
        test_i2c_env.st |= TXDATA_IS_NACK;
    }
    if ((event & DRV_EVENT_I2C_RXDATA_UNDER) == DRV_EVENT_I2C_RXDATA_UNDER) {
        test_i2c_env.st |= RXDATA_IS_UNDER;
    }
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief example of using i2c: read ,write eeprom
 *
 * @note When the I2C operates in master polling mode and there is no slave device connected,
 * @note to prevent getting stuck in the read/write function interface,
 * @note an external pull-up resistor needs to be connected,
 * @note or the pin mode of the clock and data lines should be changed from
 * @note PMU_PIN_MODE_OD (open-drain mode) to PMU_PIN_MODE_OD_PU (open-drain mode with internal pull-up).
 * @note In this way, an internal pull-up resistor will be included.
 *******************************************************************************
 */
void example_i2c(void)
{
    uint32_t timeout_ms = 10000;
    drv_pin_init(pin_cfg, sizeof(pin_cfg) / sizeof(pin_cfg[0]));

    i2c_config_t cfg = {
        .mode  = I2C_MODE_MASTER,        // 7-bit addressing mode
        .speed = I2C_SPEED_400K,
    };
    drv_i2c_init(OM_I2C0, &cfg);

    for (uint8_t i = 1; i < 8; i++) {
        eeprom_tx_buf[i] = 0x80 + i;
    }
    eeprom_tx_buf[0] = TEST_ADDR;

    // write tx_buf[1:4] from address TEST_ADDR
    drv_i2c_master_write(OM_I2C0, EEPROM_ADDR, eeprom_tx_buf, 5, timeout_ms);

    drv_dwt_delay_ms(20);  // eeprom internal write takes time

    // read data from address TEST_ADDR
    uint8_t rd_addr = TEST_ADDR;
    drv_i2c_master_read(OM_I2C0, EEPROM_ADDR, &rd_addr, 1, eeprom_rx_buf, 4, timeout_ms);

    // check data
    for (uint32_t i=0; i<4; i++) {
        om_printf("\n 0x%x \n",eeprom_rx_buf[i]);
        OM_ASSERT(eeprom_tx_buf[i+1] == eeprom_rx_buf[i]);
    }
}


 *******************************************************************************
 * @brief example of using i2c with dma: read ,write eeprom
 *
 * @note When the I2C operates in master dma mode and there is no slave device connected,
 * @note to prevent getting stuck in the read/write function interface,
 * @note an external pull-up resistor needs to be connected,
 * @note or the pin mode of the clock and data lines should be changed from
 * @note PMU_PIN_MODE_OD (open-drain mode) to PMU_PIN_MODE_OD_PU (open-drain mode with internal pull-up).
 * @note In this way, an internal pull-up resistor will be included.
 *******************************************************************************
 */
void example_i2c_dma(void)
{
    uint32_t timeout_ms = 10000;
    drv_pin_init(pin_cfg, sizeof(pin_cfg) / sizeof(pin_cfg[0]));

    i2c_config_t cfg = {
        .mode  = I2C_MODE_MASTER,        // 7-bit addressing mode
        .speed = I2C_SPEED_400K,
    };
    drv_i2c_init(OM_I2C0, &cfg);

    for (uint8_t i = 1; i < 8; i++) {
        eeprom_tx_buf[i] = 0x80 + i;
    }
    eeprom_tx_buf[0] = TEST_ADDR;

    drv_i2c_register_isr_callback(OM_I2C0, i2c_isr_callback);
    // allocate all dma channels
    drv_i2c_gpdma_channel_allocate(OM_I2C0, DRV_GPDMA_CHAN_ALL);

    // write tx_buf[1:4] from address TEST_ADDR
    drv_i2c_master_write_dma(OM_I2C0, EEPROM_ADDR, eeprom_tx_buf, 5, timeout_ms);

    while (!(test_i2c_env.st & WRITE_DMA_IS_DONE));
    test_i2c_env.st &= ~WRITE_DMA_IS_DONE;

    //wait EEPROM internal write complete
    drv_dwt_delay_ms(10);

    // read data from address TEST_ADDR
    uint8_t rd_addr = TEST_ADDR;
    drv_i2c_master_read_dma(OM_I2C0, EEPROM_ADDR, &rd_addr, 1, eeprom_rx_buf, 4, timeout_ms);
    while (!(test_i2c_env.st & READ_DMA_IS_DONE));
    test_i2c_env.st &= ~READ_DMA_IS_DONE;
    // release all dma channels
    drv_i2c_gpdma_channel_release(OM_I2C0, DRV_GPDMA_CHAN_ALL);

    // check data
    for (uint32_t i=0; i<4; i++) {
        om_printf("\n 0x%x \n",eeprom_rx_buf[i]);
        OM_ASSERT(eeprom_tx_buf[i+1] == eeprom_rx_buf[i]);
    }
}


 *******************************************************************************
 * @brief example of using i2c with int: read ,write eeprom
 *
 * @note When the I2C operates in master int mode and there is no slave device connected,
 * @note to prevent getting stuck in the read/write function interface,
 * @note an external pull-up resistor needs to be connected,
 * @note or the pin mode of the clock and data lines should be changed from
 * @note PMU_PIN_MODE_OD (open-drain mode) to PMU_PIN_MODE_OD_PU (open-drain mode with internal pull-up).
 * @note In this way, an internal pull-up resistor will be included.
 *******************************************************************************
 */
void example_i2c_int(void)
{
    uint32_t timeout_ms = 10000;
    drv_pin_init(pin_cfg, sizeof(pin_cfg) / sizeof(pin_cfg[0]));

    i2c_config_t cfg = {
        .mode  = I2C_MODE_MASTER,        // 7-bit addressing mode
        .speed = I2C_SPEED_400K,
    };
    drv_i2c_init(OM_I2C0, &cfg);

    for (uint8_t i = 1; i < 8; i++) {
        eeprom_tx_buf[i] = 0x80 + i;
    }
    eeprom_tx_buf[0] = TEST_ADDR;

    drv_i2c_register_isr_callback(OM_I2C0, i2c_isr_callback);

    // write tx_buf[1:4] from address TEST_ADDR
    drv_i2c_master_write_int(OM_I2C0, EEPROM_ADDR, eeprom_tx_buf, 5, timeout_ms);

    while (!(test_i2c_env.st & WRITE_INT_IS_DONE));
    test_i2c_env.st &= ~WRITE_INT_IS_DONE;

    //wait EEPROM internal write complete
    drv_dwt_delay_ms(10);

    // read data from address TEST_ADDR
    uint8_t rd_addr = TEST_ADDR;
    drv_i2c_master_read_int(OM_I2C0, EEPROM_ADDR, &rd_addr, 1, eeprom_rx_buf, 4, timeout_ms);
    while (!(test_i2c_env.st & READ_INT_IS_DONE));
    test_i2c_env.st &= ~READ_INT_IS_DONE;

    // check data
    for (uint32_t i=0; i<4; i++) {
        om_printf("\n 0x%x \n",eeprom_rx_buf[i]);
        OM_ASSERT(eeprom_tx_buf[i+1] == eeprom_rx_buf[i]);
    }
}

/** @} */