#include "common_def.h"
#include "spi_ex.h"
#include "om_driver.h"

static uint8_t rx_buffer[SPI_RX_BUFFER_SIZE] = {0};
static uint8_t tx_buffer[SPI_TX_BUFFER_SIZE] = {0};
/// Transfer finish flag
static volatile uint8_t int_transfer_is_done;

static void spi_transfer_cb(void *om_spi, drv_event_t event, void *rx_buf, void *rx_cnt)
{
    if (event == DRV_EVENT_COMMON_TRANSFER_COMPLETED) {
        int_transfer_is_done = 1;
    }
}

void imu_hw_init(void)
{	
    spi_config_t   	spi_cfg;
    spi_cfg.freq        = 8*1000*1000;
    spi_cfg.role        = SPI_ROLE_MASTER;
    //spi_cfg.role        = SPI_ROLE_SLAVE;
    spi_cfg.mode        = SPI_MODE_0;
    spi_cfg.wire        = SPI_WIRE_4;
    spi_cfg.first_bit   = SPI_MSB_FIRST;
    spi_cfg.cs_valid    = SPI_CS_LOW;		

//		icm42607_gpio_init();
    drv_spi_init(OM_SPI0, &spi_cfg);
    drv_spi_register_isr_callback(OM_SPI0, spi_transfer_cb);
}

int spi_write(uint8_t reg, const uint8_t* tx_data, uint32_t length)
{
		if(length == 0U || length > SPI_MAX_WRITE_PAYLOAD)
		{
				log_debug("********* error spi write length = %d, expect: 0 < length <= %d !!!\r\n", length, SPI_MAX_WRITE_PAYLOAD);
				return -1;
		}
		uint8_t re = 0xff;
		memset(tx_buffer, 0, SPI_TX_BUFFER_SIZE);
		tx_buffer[0] = reg & 0x7F; //WRITE(0)

		memcpy(&tx_buffer[1], tx_data, length);
		re = drv_spi_transfer_int(OM_SPI0, tx_buffer, length+1, rx_buffer, 0);
    while (!int_transfer_is_done);
    int_transfer_is_done = 0;

		if(re != OM_ERROR_OK)
		{
				log_debug("********* spi write fail,re = %d\r\n", re);
				return -1;
		}
//		OM_LOG_INF("spi write, reg=%x, data:\r\n", reg);
//		OM_LOG_INF_HEX(tx_data, length);
		return re;
}

int spi_read(uint8_t reg, uint8_t* rx_data, uint32_t length)
{
		if(length == 0U || length > SPI_MAX_READ_PAYLOAD)
		{
				log_debug("********* error spi read length = %d, expect: 0 < length <= %d !!!\r\n", length, SPI_MAX_READ_PAYLOAD);
				return -1;
		}	
		uint8_t re = 0xff;
		memset(rx_buffer, 0, SPI_RX_BUFFER_SIZE);
		memset(tx_buffer, 0, SPI_TX_BUFFER_SIZE);
		tx_buffer[0] = reg | 0x80;  //READ(1);
		
		re = drv_spi_transfer_int(OM_SPI0, tx_buffer, length+1, rx_buffer, length+1);
//		re = drv_spi_transfer_int(OM_SPI0, tx_buffer, 2, rx_buffer, length+1); //要发u16格式数据  //llx test
    while (!int_transfer_is_done);
    int_transfer_is_done = 0;

		if(re != OM_ERROR_OK)
		{
				log_debug("********* spi read fail,re = %d\r\n", re);
				return -1;
		}
		memcpy(rx_data, &rx_buffer[1], length); //rx_buffer[0] from search reg reply, need delete
		
//		OM_LOG_INF("spi read, reg=%x, data:\r\n", reg);
//		OM_LOG_INF_HEX(rx_data, length);
		return re;
}


