/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup SPI SPI
 * @ingroup  DRIVER
 * @brief    SPI driver
 * @details  SPI driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_SPI_H
#define __DRV_SPI_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_SPI0 || RTE_SPI1)
#include <stdint.h>
#include "om_driver.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * TYPEDEFS
 */
/// SPI Role
typedef enum {
    /// Slave
    SPI_ROLE_SLAVE  = 0,
    /// Master
    SPI_ROLE_MASTER = 1,
} spi_role_t;

/// Valid level of CS
typedef enum {
    /// Valid level of CS is Low
    SPI_CS_LOW      = 0,
    /// Valid level of CS is High
    SPI_CS_HIGH     = 1,
} spi_cs_t;

/// SPI Mode
typedef enum {
    /// MODE0: the idle time is low and the sampling starts from the first hop edge
    SPI_MODE_0      = 0,
    /// MODE1: the idle time is low and the sampling starts from the second hop edge
    SPI_MODE_1      = 1,
    /// MODE2: the idle time is high and the sampling starts from the first hop edge
    SPI_MODE_2      = 2,
    /// MODE3: the idle time is high and the sampling starts from the second hop edge
    SPI_MODE_3      = 3,

    SPI_MODE_NUM,
} spi_mode_t;

/// SPI Wire
typedef enum {
    /// 3 wire: CS<->CS, CLK<->CLK, DO<->DO
    SPI_WIRE_3      = 0,
    /// 4 wire: CS<->CS, CLK<->CLK, DO<->DI, DI<->DO
    SPI_WIRE_4      = 1,
} spi_wire_t;

/// SPI MSB LSB Transmission
typedef enum {
    /// data transfers start from LSB bit
    SPI_LSB_FIRST  = 0,
    /// data transfers start from MSB bit
    SPI_MSB_FIRST  = 1,
} spi_first_t;

/// SPI Control
typedef enum {
    SPI_CONTROL_CSN                      = 0U,    /**< Set SPI controller CSn low/high, argu indicate csn value **/
    SPI_CONTROL_DLY_SAMPLE_FE_SET        = 1U,    /**< When argu is NULL, disable falling edge delay sample and vice versa **/
    SPI_CONTROL_DLY_SAMPLE_CYCLE_NUM_SET = 2U,    /**< Set delay sample cycle num, argu is in the range [0, 3] **/
    SPI_CONTROL_TX_INT_MASK_SET          = 3U,    /**< Set SPI tx total interrupt mask，1 is enable interrupt，0 is mask interrupt **/
    SPI_CONTROL_RX_INT_MASK_SET          = 4U,    /**< Set SPI rx total interrupt mask，1 is enable interrupt，0 is mask interrupt **/
} spi_control_t;

/// SPI Configuration
typedef struct {
    /// Specifies SPI clock frequency, NOTE: spi_clock must <= cpu_clock / 2
    uint32_t        freq;
    /// Specifies SPI Mode
    spi_mode_t      mode;
    /// Specifies SPI Role
    spi_role_t      role;
    /// Specifies SPI Wire
    spi_wire_t      wire;
    /// Specifies whether data transfers start from LSB or MSB bit
    spi_first_t     first_bit;
    /// Specifies CS valid level
    spi_cs_t        cs_valid;
} spi_config_t;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Get SPI base from spi idx
 *
 * @param idx  Index of SPI peripheral
 *
 * @return OM_SPI Type pointer
 *******************************************************************************
 */
static inline OM_SPI_Type* drv_spi_idx2base(uint8_t idx)
{
    OM_SPI_Type *const spi[] = {
        #if (RTE_SPI0)
        OM_SPI0,
        #else
        NULL,
        #endif
        #if (RTE_SPI1)
        OM_SPI1,
        #else
        NULL,
        #endif
    };

    return (idx < sizeof(spi)/sizeof(spi[0])) ? spi[idx] : NULL;
}

/**
 *******************************************************************************
 * @brief SPI initialization
 *
 * @param[in] om_spi         Pointer to SPI
 * @param[in] cfg            Configuration for SPI
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_spi_init(OM_SPI_Type *om_spi, spi_config_t *cfg);

/**
 *******************************************************************************
 * @brief SPI uninitialize, gate SPI clock
 *
 * @param[in] om_spi    Pointer to SPI
 *******************************************************************************
 */
extern void drv_spi_uninit(OM_SPI_Type *om_spi);

#if (RTE_SPI_REGISTER_CALLBACK)
/**
 *******************************************************************************
 * @brief Register callback for SPI transfer
 *
 * @param[in] om_spi         Pointer to SPI
 * @param[in] isr_cb         Pointer to callback
 *******************************************************************************
 */
extern void drv_spi_register_isr_callback(OM_SPI_Type *om_spi, drv_isr_callback_t isr_cb);
#endif

/**
 *******************************************************************************
 * @brief The interrupt callback for SPI driver. It is a weak function. User should define
 *        their own callback in user file, other than modify it in the SPI driver.
 *
 * @param om_spi            The SPI device address
 * @param event             The driver uart event
 *                           - DRV_EVENT_COMMON_TRANSFER_COMPLETED
 *                           - DRV_EVENT_COMMON_READ_COMPLETED
 *                           - DRV_EVENT_COMMON_ABORT
 *                           - DRV_EVENT_COMMON_ERROR
 *                           - DRV_EVENT_COMMON_DMA2PERIPH_COMPLETED
 * @param data              The data pointer of data to be read or write
 * @param num               The data buffer valid data count
 *******************************************************************************
 */
extern void drv_spi_isr_callback(OM_SPI_Type *om_spi, drv_event_t event, uint8_t *data, uint32_t num);

/**
 *******************************************************************************
 * @brief Start sending data to SPI transmitter by block mode.
 *
 * @param[in]   om_spi   Pointer to SPI
 * @param[in]   tx_data  Pointer to buffer with data to send to master SPI transmitter
 * @param[in]   tx_num   Number of data items to send
 * @param[in]   rx_data  Pointer to buffer with data to send to master SPI receiver
 * @param[in]   rx_num   Number of data items to receive
 * @param[in]   timeout_ms   timeout
 *
 * @return error
 *******************************************************************************
 */
extern om_error_t drv_spi_transfer(OM_SPI_Type *om_spi, const uint8_t *tx_data, uint16_t tx_num, uint8_t *rx_data, uint16_t rx_num, uint32_t timeout_ms);

/**
 *******************************************************************************
 * @brief Start sending data to SPI transmitter by interrupt mode.
 *
 * @param[in]   om_spi   Pointer to SPI
 * @param[in]   tx_data  Pointer to buffer with data to send to master SPI transmitter
 * @param[in]   tx_num   Number of data items to send
 * @param[in]   rx_data  Pointer to buffer with data to send to master SPI receiver
 * @param[in]   rx_num   Number of data items to receive
 *
 * @return errno
 *******************************************************************************
 */

extern om_error_t drv_spi_transfer_int(OM_SPI_Type *om_spi, const uint8_t *tx_data, uint16_t tx_num, uint8_t *rx_data, uint16_t rx_num);

#if (RTE_GPDMA)
/**
 *******************************************************************************
 * @brief Allocate spi dma channel
 *
 * @param[in] om_spi    Pointer to spi
 * @param[in] channel   rx/tx channel
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_spi_gpdma_channel_allocate(OM_SPI_Type *om_spi, drv_gpdma_chan_t channel);

/**
 *******************************************************************************
 * @brief Allocate spi dma channel
 *
 * @param[in] om_spi    Pointer to spi
 * @param[in] channel   rx/tx channel
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_spi_gpdma_channel_release(OM_SPI_Type *om_spi, drv_gpdma_chan_t channel);

/**
 *******************************************************************************
 * @brief Start sending data to SPI transmitter by dma mode.
 *
 * @param[in]   om_spi   Pointer to SPI
 * @param[in]   tx_data  Pointer to buffer with data to send to master SPI transmitter
 * @param[in]   tx_num   Number of data items to send
 * @param[in]   rx_data  Pointer to buffer with data to send to master SPI receiver
 * @param[in]   rx_num   Number of data items to receive
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_spi_transfer_dma(OM_SPI_Type *om_spi, const uint8_t *tx_data, uint16_t tx_num, uint8_t *rx_data, uint16_t rx_num);
#endif

/**
 *******************************************************************************
 * @brief Get write count, supported IT and GPDMA write mode
 *
 * @param[in] om_spi    Pointer to spi
 *
 * @return count bytes of write completed
 *******************************************************************************
 */
extern uint32_t drv_spi_get_write_cnt(OM_SPI_Type *om_spi);

/**
 *******************************************************************************
 * @brief Get read count, supported IT and GPDMA read mode
 *
 * @param[in] om_spi    Pointer to spi
 *
 * @return count bytes of read completed
 *******************************************************************************
 */
extern uint32_t drv_spi_get_read_cnt(OM_SPI_Type *om_spi);

/**
 *******************************************************************************
 * @brief Control the SPI
 *
 * @param[in] om_spi         Pointer to SPI
 * @param[in] control        Control options
 * @param[in] argu           argument for control options
 *
 *******************************************************************************
 */
static inline void drv_spi_control(OM_SPI_Type *om_spi, spi_control_t control, void *argu)
{
    switch (control) {
        case SPI_CONTROL_CSN:
            om_spi->CSNCTRL = SPI_CSNCTRL_CS_MODE_MASK + ((uint32_t)argu ? SPI_CSNCTRL_CS_GPO_MASK : 0U);
            break;
        case SPI_CONTROL_DLY_SAMPLE_FE_SET:
            register_set(&om_spi->DELAY_CFG, MASK_1REG(SPI_DELAY_CFG_SAMPLE_FE_EN, ((uint32_t)argu) ? 1U : 0U));
            break;
        case SPI_CONTROL_DLY_SAMPLE_CYCLE_NUM_SET:
            OM_ASSERT((uint32_t)argu <= 3);
            register_set(&om_spi->DELAY_CFG, MASK_1REG(SPI_DELAY_CFG_SAMPLE_CYCLE_NUM, (uint32_t)argu));
            break;
        case SPI_CONTROL_TX_INT_MASK_SET:
            register_set(&om_spi->DELAY_CFG, MASK_1REG(SPI_DELAY_CFG_TX_INT, (uint32_t)argu));
            break;
        case SPI_CONTROL_RX_INT_MASK_SET:
            register_set(&om_spi->DELAY_CFG, MASK_1REG(SPI_DELAY_CFG_RX_INT, (uint32_t)argu));
            break;
        default:
            break;
    }
}


/**
 *******************************************************************************
 * @brief spi interrupt service routine
 *
 * @param[in] om_spi         Pointer to SPI
 *******************************************************************************
 */
extern void drv_spi_isr(OM_SPI_Type *om_spi);


#ifdef __cplusplus
}
#endif

#endif  /* (RTE_SPI0 || RTE_SPI1) */

#endif  /* __DRV_SPI_H */


/** @} */