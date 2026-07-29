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
 * @brief    example for using spi
 * @details  example for using spi: wire4, wire3 transmission
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

/*
Note:
When the SPI operates as a slave device, if the clock of the other host does not
have an interval, since the SPI module needs at least 3 bus cycles to sample and
latch data into the FIFO, it is necessary to ensure that the bus clock of the SPI
module (i.e., the CPU clock) is at least 4 times the communication clock;
otherwise, the host needs to increase the communication clock interval.
When the bus clock of the SPI slave is twice the communication clock,
the host needs to increase the communication clock interval time equivalent to
bus cycles of the slave SPI. When it is three times, the host needs to
increase the communication clock interval time by 1 bus cycle.
*/
/*******************************************************************************
 * MACROS
 */
#define PAD_SPI0_CS                     18
#define MUX_SPI0_CS                     PINMUX_PAD18_SPI0_CS_CFG
#define PAD_SPI0_CLK                    19
#define MUX_SPI0_CLK                    PINMUX_PAD19_SPI0_SCK_CFG
#define PAD_SPI0_DI                     16
#define MUX_SPI0_DI                     PINMUX_PAD16_SPI0_DI_CFG
#define PAD_SPI0_DIO                    17
#define MUX_SPI0_DIO                    PINMUX_PAD17_SPI0_DIO_CFG

#define TEST_TRANS_SIZE                 100


/*******************************************************************************
 * TYPEDEFS
 */


/*******************************************************************************
 * CONST & VARIABLES
 */
/// Pin configuration
static const pin_config_t pin_cfg_cnt [] = {
    {PAD_SPI0_CS,  {MUX_SPI0_CS},  PMU_PIN_MODE_PU, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_SPI0_CLK, {MUX_SPI0_CLK}, PMU_PIN_MODE_PU, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_SPI0_DI,  {MUX_SPI0_DI},  PMU_PIN_MODE_PU, PMU_PIN_DRIVER_CURRENT_NORMAL},
    {PAD_SPI0_DIO, {MUX_SPI0_DIO}, PMU_PIN_MODE_PU, PMU_PIN_DRIVER_CURRENT_NORMAL},
};
/// Buffer that stores the data to be sent
static uint8_t spi_tx_buf[TEST_TRANS_SIZE];
/// Buffer that stores the data to be received
static uint8_t spi_rx_buf[TEST_TRANS_SIZE];
/// Transfer finish flag
static volatile uint8_t int_transfer_is_done;
/// DMA Transfer finish flag
static volatile uint8_t dma_transfer_is_done;

/*******************************************************************************
 * LOCAL FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief callback for spi interrupt
 *
 * @param[in] om_spi    Pointer to spi
 * @param[in] event     SPI event
 *                      - DRV_EVENT_COMMON_TRANSFER_COMPLETED
 *                      - DRV_EVENT_COMMON_DMA2PERIPH_COMPLETED
 *                      - DRV_EVENT_COMMON_ABORT
 *                      - DRV_EVENT_COMMON_ERROR
 * @param[in] rx_buf    Pointer to rx_buf
 * @param[in] rx_cnt    Number of rx data
 *******************************************************************************
 */
static void spi_transfer_cb(void *om_spi, drv_event_t event, void *rx_buf, void *rx_cnt)
{
    if (event == DRV_EVENT_COMMON_TRANSFER_COMPLETED) {
        int_transfer_is_done = 1;
    }
}

/**
 *******************************************************************************
 * @brief callback for spi DMA
 *
 * @param[in] om_spi    Pointer to spi
 * @param[in] event     SPI event
 *                      - DRV_EVENT_COMMON_TRANSFER_COMPLETED
 *                      - DRV_EVENT_COMMON_DMA2PERIPH_COMPLETED
 *                      - DRV_EVENT_COMMON_ABORT
 *                      - DRV_EVENT_COMMON_ERROR
 * @param[in] rx_buf    Pointer to rx_buf
 * @param[in] rx_cnt    Number of rx data
 *******************************************************************************
 */
static void spi_transfer_dma_cb(void *om_spi, drv_event_t event, void *rx_buf, void *rx_cnt)
{
    if ((event == DRV_EVENT_COMMON_GPDMA2PERIPH_COMPLETED)||(event == DRV_EVENT_COMMON_READ_COMPLETED)) {
        dma_transfer_is_done = 1;
    }
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief example of using spi wire4 to send and receive by int
 *
 *******************************************************************************
 */
void example_spi_wire4_trans_int(void)
{
    spi_config_t    spi_cfg;
    spi_cfg.freq        = 8*1000*1000;
    spi_cfg.role        = SPI_ROLE_MASTER;
    //spi_cfg.role        = SPI_ROLE_SLAVE;
    spi_cfg.mode        = SPI_MODE_0;
    spi_cfg.wire        = SPI_WIRE_4;
    spi_cfg.first_bit   = SPI_MSB_FIRST;
    spi_cfg.cs_valid    = SPI_CS_LOW;

    drv_pin_init(pin_cfg_cnt, sizeof(pin_cfg_cnt) / sizeof(pin_cfg_cnt[0]));
    drv_spi_init(OM_SPI0, &spi_cfg);
    drv_spi_register_isr_callback(OM_SPI0, spi_transfer_cb);

    for (uint16_t i = 0; i < TEST_TRANS_SIZE; i++) {
        spi_tx_buf[i] = i + 1;
    }
    memset(spi_rx_buf, 0xFF, sizeof(spi_rx_buf));

    drv_spi_transfer_int(OM_SPI0, spi_tx_buf, TEST_TRANS_SIZE, spi_rx_buf, TEST_TRANS_SIZE);
    while (!int_transfer_is_done);
    int_transfer_is_done = 0;
}

/**
 *******************************************************************************
 * @brief example of using spi wire4 to send and receive by DMA
 *
 *******************************************************************************
 */
void example_spi_wire4_trans_dma(void)
{
    spi_config_t    spi_cfg;
    spi_cfg.freq        = 8*1000*1000;
    spi_cfg.role        = SPI_ROLE_MASTER;
    //spi_cfg.role        = SPI_ROLE_SLAVE;
    spi_cfg.mode        = SPI_MODE_0;
    spi_cfg.wire        = SPI_WIRE_4;
    spi_cfg.first_bit   = SPI_MSB_FIRST;
    spi_cfg.cs_valid    = SPI_CS_LOW;

    drv_pin_init(pin_cfg_cnt, sizeof(pin_cfg_cnt) / sizeof(pin_cfg_cnt[0]));
    drv_spi_init(OM_SPI0, &spi_cfg);
    drv_spi_register_isr_callback(OM_SPI0, spi_transfer_dma_cb);

    for (uint16_t i = 0; i < TEST_TRANS_SIZE; i++) {
        spi_tx_buf[i] = i + 1;
    }
    memset(spi_rx_buf, 0xFF, sizeof(spi_rx_buf));

    drv_spi_gpdma_channel_allocate(OM_SPI0, DRV_GPDMA_CHAN_ALL);
    drv_spi_transfer_dma(OM_SPI0, spi_tx_buf, TEST_TRANS_SIZE, spi_rx_buf, TEST_TRANS_SIZE);
    while (!dma_transfer_is_done);
    dma_transfer_is_done = 0;

    drv_spi_gpdma_channel_release(OM_SPI0, DRV_GPDMA_CHAN_ALL);
}

/**
 *******************************************************************************
 * @brief example of using spi wire3 to send and receive by int
 * MUX_SPI0_CS, MUX_SPI0_CLK, MUX_SPI0_DIO are used
 *
 *******************************************************************************
 */
void example_spi_wire3_trans_int(void)
{
    spi_config_t    spi_cfg;
    spi_cfg.freq        = 8*1000*1000;
    spi_cfg.role        = SPI_ROLE_MASTER;
    //spi_cfg.role        = SPI_ROLE_SLAVE;
    spi_cfg.mode        = SPI_MODE_0;
    spi_cfg.wire        = SPI_WIRE_3;
    spi_cfg.first_bit   = SPI_MSB_FIRST;
    spi_cfg.cs_valid    = SPI_CS_LOW;

    drv_pin_init(pin_cfg_cnt, sizeof(pin_cfg_cnt) / sizeof(pin_cfg_cnt[0]));
    drv_spi_init(OM_SPI0, &spi_cfg);
    drv_spi_register_isr_callback(OM_SPI0, spi_transfer_cb);

    for (uint16_t i = 0; i < TEST_TRANS_SIZE; i++) {
        spi_tx_buf[i] = i + 1;
    }
    memset(spi_rx_buf, 0xFF, sizeof(spi_rx_buf));

    if(spi_cfg.role == SPI_ROLE_MASTER){
        drv_spi_transfer_int(OM_SPI0, spi_tx_buf, TEST_TRANS_SIZE, spi_rx_buf, 0U);
    }
    else if(spi_cfg.role == SPI_ROLE_SLAVE){
        drv_spi_transfer_int(OM_SPI0, spi_tx_buf, 0U, spi_rx_buf, TEST_TRANS_SIZE);
    }
    while (!int_transfer_is_done);
    int_transfer_is_done = 0;

    if(spi_cfg.role == SPI_ROLE_MASTER){
        drv_dwt_delay_ms(5);   // leave the time for slave switch to transmit only
    }

    if(spi_cfg.role == SPI_ROLE_MASTER){
        drv_spi_transfer_int(OM_SPI0, spi_tx_buf, 0U, spi_rx_buf, TEST_TRANS_SIZE);
    }
    else if(spi_cfg.role == SPI_ROLE_SLAVE){
        drv_spi_transfer_int(OM_SPI0, spi_tx_buf, TEST_TRANS_SIZE, spi_rx_buf, 0U);
    }

    while (!int_transfer_is_done);
    int_transfer_is_done = 0;
}

/**
 *******************************************************************************
 * @brief example of using spi wire3 to send and receive by int
 * MUX_SPI0_CS, MUX_SPI0_CLK, MUX_SPI0_DIO are used
 *
 *******************************************************************************
 */
void example_spi_wire3_trans_dma(void)
{
    spi_config_t    spi_cfg;
    spi_cfg.freq        = 8*1000*1000;
    spi_cfg.role        = SPI_ROLE_MASTER;
    //spi_cfg.role        = SPI_ROLE_SLAVE;
    spi_cfg.mode        = SPI_MODE_0;
    spi_cfg.wire        = SPI_WIRE_3;
    spi_cfg.first_bit   = SPI_MSB_FIRST;
    spi_cfg.cs_valid    = SPI_CS_LOW;

    drv_pin_init(pin_cfg_cnt, sizeof(pin_cfg_cnt) / sizeof(pin_cfg_cnt[0]));
    drv_spi_init(OM_SPI0, &spi_cfg);
    drv_spi_register_isr_callback(OM_SPI0, spi_transfer_dma_cb);

    for (uint16_t i = 0; i < TEST_TRANS_SIZE; i++) {
        spi_tx_buf[i] = i + 1;
    }
    memset(spi_rx_buf, 0xFF, sizeof(spi_rx_buf));

    drv_spi_gpdma_channel_allocate(OM_SPI0, DRV_GPDMA_CHAN_ALL);
    if(spi_cfg.role == SPI_ROLE_MASTER){
        drv_spi_transfer_dma(OM_SPI0, spi_tx_buf, TEST_TRANS_SIZE, spi_rx_buf, 0U);
    }
    else if(spi_cfg.role == SPI_ROLE_SLAVE){
        drv_spi_transfer_dma(OM_SPI0, spi_tx_buf, 0U, spi_rx_buf, TEST_TRANS_SIZE);
    }
    while (!dma_transfer_is_done);
    dma_transfer_is_done = 0;

    if(spi_cfg.role == SPI_ROLE_MASTER){
        drv_dwt_delay_ms(5);   // leave the time for slave switch to transmit only
    }

    if(spi_cfg.role == SPI_ROLE_MASTER){
        drv_spi_transfer_dma(OM_SPI0, spi_tx_buf, 0U, spi_rx_buf, TEST_TRANS_SIZE);
    }
    else if(spi_cfg.role == SPI_ROLE_SLAVE){
        drv_spi_transfer_dma(OM_SPI0, spi_tx_buf, TEST_TRANS_SIZE, spi_rx_buf, 0U);
    }

    while (!dma_transfer_is_done);
    dma_transfer_is_done = 0;

    drv_spi_gpdma_channel_release(OM_SPI0, DRV_GPDMA_CHAN_ALL);
}


/**
 *******************************************************************************
 * @brief example of using spi wire4 to send and receive by int freq 32M
 *
 *******************************************************************************
 */
void example_spi_wire4_trans_int_32m(void)
{
    spi_config_t    spi_cfg;
    spi_cfg.freq        = 32*1000*1000;
    spi_cfg.role        = SPI_ROLE_MASTER;
    //spi_cfg.role        = SPI_ROLE_SLAVE;
    spi_cfg.mode        = SPI_MODE_0;
    spi_cfg.wire        = SPI_WIRE_4;
    spi_cfg.first_bit   = SPI_MSB_FIRST;
    spi_cfg.cs_valid    = SPI_CS_LOW;

    drv_pin_init(pin_cfg_cnt, sizeof(pin_cfg_cnt) / sizeof(pin_cfg_cnt[0]));
    drv_spi_init(OM_SPI0, &spi_cfg);
    drv_spi_register_isr_callback(OM_SPI0, spi_transfer_cb);

    drv_spi_control(OM_SPI0, SPI_CONTROL_DLY_SAMPLE_FE_SET, (void *)1);
    drv_spi_control(OM_SPI0, SPI_CONTROL_DLY_SAMPLE_CYCLE_NUM_SET, (void *)1);

    for (uint16_t i = 0; i < TEST_TRANS_SIZE; i++) {
        spi_tx_buf[i] = i + 1;
    }
    memset(spi_rx_buf, 0xFF, sizeof(spi_rx_buf));

    drv_spi_transfer_int(OM_SPI0, spi_tx_buf, TEST_TRANS_SIZE, spi_rx_buf, TEST_TRANS_SIZE);
    while (!int_transfer_is_done);
    int_transfer_is_done = 0;
}

/** @} */