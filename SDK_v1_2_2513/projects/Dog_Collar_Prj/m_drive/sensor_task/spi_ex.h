#ifndef __SPI_EX_H__
#define __SPI_EX_H__
#include <stdint.h>

#define SPI_RX_BUFFER_SIZE 40U
#define SPI_TX_BUFFER_SIZE 40U

/* A read transfers the register address plus payload in both directions. */
#define SPI_TRANSFER_BUFFER_SIZE \
    ((SPI_RX_BUFFER_SIZE < SPI_TX_BUFFER_SIZE) ? SPI_RX_BUFFER_SIZE : SPI_TX_BUFFER_SIZE)
#define SPI_MAX_READ_PAYLOAD     (SPI_TRANSFER_BUFFER_SIZE - 1U)
#define SPI_MAX_WRITE_PAYLOAD    (SPI_TX_BUFFER_SIZE - 1U)

int spi_write(uint8_t reg, const uint8_t* tx_data, uint32_t length);
int spi_read(uint8_t reg, uint8_t* rx_data, uint32_t length);
void imu_hw_init(void);

#endif

