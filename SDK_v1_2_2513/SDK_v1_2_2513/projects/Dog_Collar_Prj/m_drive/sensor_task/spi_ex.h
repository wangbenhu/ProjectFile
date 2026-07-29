#ifndef __SPI_EX_H__
#define __SPI_EX_H__
#include <stdint.h>

int spi_write(uint8_t reg, const uint8_t* tx_data, uint32_t length);
int spi_read(uint8_t reg, uint8_t* rx_data, uint32_t length);
void imu_hw_init(void);

#endif

