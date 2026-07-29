/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup I2C I2C
 * @ingroup  DRIVER
 * @brief    I2C driver
 * @details  I2C driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_I2C_H
#define __DRV_I2C_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_I2C0 || RTE_I2C1)
#include <stdint.h>
#include "om_driver.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * TYPEDEFS
 */
/// I2C Mode
typedef enum {
    /// Master Mode, 7-bit addressing mode
    I2C_MODE_MASTER       = 0,
    /// Slave Mode,  7-bit addressing mode
    I2C_MODE_SLAVE        = 1,
    /// Slave Mode,  10-bit addressing mode
    I2C_MODE_SMBUS_DEVICE = 2,
    /// Master Mode, 10-bit addressing mode
    I2C_MODE_SMBUS_HOST   = 3,
} i2c_mode_t;

/// I2C Speed
typedef enum {
    /// Speed: 100khz
    I2C_SPEED_100K = 1,
    /// Speed: 400khz
    I2C_SPEED_400K = 2,
    /// Speed: 1Mhz
    I2C_SPEED_1M   = 3,
    /// Speed: 2Mhz
    I2C_SPEED_2M   = 4,
    /// Speed: MAX
    I2C_SPEED_MAX  = 5,
} i2c_speed_t;

/// I2C Config
typedef struct {
    i2c_mode_t  mode;
    i2c_speed_t speed;
} i2c_config_t;

/// I2C Control
typedef enum {
    /// check I2C is busy
    I2C_CONTROL_IS_BUSY         = 0U,
    /// check device is valid, argu is i2c_addr
    I2C_CONTROL_DEV_IS_VALID    = 1U,
} i2c_control_t;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Get I2C base from i2c idx
 *
 * @param[in] idx  Index of I2C peripheral
 *
 * @return OM_I2C Type pointer
 *******************************************************************************
 */
static inline OM_I2C_Type* drv_i2c_idx2base(uint8_t idx)
{
    OM_I2C_Type *const i2c[] = {
        #if (RTE_I2C0)
        OM_I2C0,
        #else
        NULL,
        #endif
        #if (RTE_I2C1)
        OM_I2C1,
        #else
        NULL,
        #endif
    };

    return (idx < sizeof(i2c)/sizeof(i2c[0])) ? i2c[idx] : NULL;
}

/**
 *******************************************************************************
 * @brief I2C initialization
 *
 * @param[in] om_i2c         Pointer to I2C
 * @param[in] cfg            Configuration for I2C
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_i2c_init(OM_I2C_Type *om_i2c, const i2c_config_t *cfg);

/**
 *******************************************************************************
 * @brief I2C deinitialization
 *
 * @param[in] om_i2c         Pointer to I2C
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_i2c_uninit(OM_I2C_Type *om_i2c);

#if (RTE_I2C_REGISTER_CALLBACK)
/**
 *******************************************************************************
 * @brief Register event callback for transmit/receive by interrupt & dma mode
 *
 * @param[in] om_i2c         Pointer to I2C
 * @param[in] isr_cb         Pointer to callback
 *******************************************************************************
 */
extern void drv_i2c_register_isr_callback(OM_I2C_Type *om_i2c, drv_isr_callback_t isr_cb);
#endif

/**
 *******************************************************************************
 * @brief The interrupt callback for I2C driver. It is a weak function. User should define
 *        their own callback in user file, other than modify it in the I2C driver.
 *
 * @param om_i2c            The I2C device address
 * @param event             The I2C event
 *                           - DRV_EVENT_COMMON_WRITE_COMPLETED
 *                           - DRV_EVENT_COMMON_READ_COMPLETED
 *                           - DRV_EVENT_COMMON_ABORT
 *                           - DRV_EVENT_COMMON_ERROR
 *                           - DRV_EVENT_COMMON_DMA2PERIPH_COMPLETED
 * @param data              The data pointer of data to be read or write
 * @param num               The data buffer valid data count
 *******************************************************************************
 */
extern void drv_i2c_isr_callback(OM_I2C_Type *om_i2c, drv_event_t event, uint8_t *data, uint32_t num);

/**
 *******************************************************************************
 * @brief I2C master start sending data to slave device by block mode.
 *
 * @param[in]   om_i2c       Pointer to I2C
 * @param[in]   dev_addr     Slave device address
 * @param[in]   tx_data      Pointer to buffer with data to send
 * @param[in]   tx_num       Number of data items to send
 * @param[in]   timeout_ms   timeout(ms)
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_i2c_master_write(OM_I2C_Type *om_i2c, uint16_t dev_addr, const uint8_t *tx_data, uint32_t tx_num, uint32_t timeout_ms);

/**
 *******************************************************************************
 * @brief I2C master start sending data to slave device by interrupt mode.
 *
 * @param[in]   om_i2c       Pointer to I2C
 * @param[in]   dev_addr     Slave device address
 * @param[in]   tx_data      Pointer to buffer with data to send
 * @param[in]   tx_num       Number of data items to send
 * @param[in]   timeout_ms   timeout(ms)
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_i2c_master_write_int(OM_I2C_Type *om_i2c, uint16_t dev_addr, const uint8_t *tx_data, uint32_t tx_num, uint32_t timeout_ms);

#if (RTE_GPDMA)
/**
 *******************************************************************************
 * @brief Allocate dma channel for i2c
 *
 * @param[in]   om_i2c       Pointer for I2C
 * @param[in]   channel      I2C rx/tx channel
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_i2c_gpdma_channel_allocate(OM_I2C_Type *om_i2c, drv_gpdma_chan_t channel);

/**
 *******************************************************************************
 * @brief Release dma channel for i2c
 *
 * @param[in]   om_i2c       Pointer for I2C
 * @param[in]   channel      I2C rx/tx channel
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_i2c_gpdma_channel_release(OM_I2C_Type *om_i2c, drv_gpdma_chan_t channel);

/**
 *******************************************************************************
 * @brief I2C master start sending data to slave device by interrupt dma mode.
 *
 * @param[in]   om_i2c       Pointer to I2C
 * @param[in]   dev_addr     Slave device address
 * @param[in]   tx_data      Pointer to buffer with data to send
 * @param[in]   tx_num       Number of data items to send
 * @param[in]   timeout_ms   timeout(ms)
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_i2c_master_write_dma(OM_I2C_Type *om_i2c, uint16_t dev_addr, const uint8_t *tx_data, uint32_t tx_num, uint32_t timeout_ms);
#endif

/**
 *******************************************************************************
 * @brief I2C master start receiving data from slave device by block mode.
 *
 * @param[in]   om_i2c       Pointer to I2C
 * @param[in]   dev_addr     Slave device address
 * @param[in]   tx_data      Pointer to buffer with data to send
 * @param[in]   tx_num       Number of data items to send
 * @param[out]  rx_data      Pointer to buffer with data to receive
 * @param[in]   rx_num       Number of data items to receive
 * @param[in]   timeout_ms   timeout(ms)
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_i2c_master_read(OM_I2C_Type *om_i2c, uint16_t dev_addr, const uint8_t *tx_data, uint16_t tx_num, uint8_t *rx_data, uint16_t rx_num, uint32_t timeout_ms);
/**
 *******************************************************************************
 * @brief I2C master start receiving data from slave device by interrupt mode.
 *
 * @param[in]   om_i2c       Pointer to I2C
 * @param[in]   dev_addr     Slave device address
 * @param[in]   tx_data      Pointer to buffer with data to send
 * @param[in]   tx_num       Number of data items to send(tx_num <= 16)
 * @param[out]  rx_data      Pointer to buffer with data to receive
 * @param[in]   rx_num       Number of data items to receive
 * @param[in]   timeout_ms   timeout(ms)
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_i2c_master_read_int(OM_I2C_Type *om_i2c, uint16_t dev_addr, const uint8_t *tx_data, uint16_t tx_num, uint8_t *rx_data, uint16_t rx_num, uint32_t timeout_ms);
#if (RTE_GPDMA)
/**
 *******************************************************************************
 * @brief I2C master start receiving data from slave device by dma mode.
 *
 * @param[in]   om_i2c       Pointer to I2C
 * @param[in]   dev_addr     Slave device address
 * @param[in]   tx_data      Pointer to buffer with data to send
 * @param[in]   tx_num       Number of data items to send
 * @param[out]  rx_data      Pointer to buffer with data to receive
 * @param[in]   rx_num       Number of data items to receive
 * @param[in]   timeout_ms   timeout(ms)
 *
 * @return errno
 *******************************************************************************
 */
extern om_error_t drv_i2c_master_read_dma(OM_I2C_Type *om_i2c, uint16_t dev_addr, const uint8_t *tx_data, uint16_t tx_num, uint8_t *rx_data, uint16_t rx_num, uint32_t timeout_ms);
#endif
/**
 *******************************************************************************
 * @brief I2C slave start sending data to master device by block mode.
 *
 * @param[in]   om_i2c       Pointer to I2C
 * @param[in]   dev_addr     Slave device address
 * @param[in]   tx_data      Pointer to buffer with data to send
 * @param[in]   tx_num       Number of data items to send
 *
 * @return errno
 *******************************************************************************
 */
om_error_t drv_i2c_slave_write(OM_I2C_Type *om_i2c, uint16_t dev_addr, const uint8_t *tx_data, uint16_t tx_num, uint32_t timeout_ms);
/**
 *******************************************************************************
 * @brief I2C slave start sending data to master device by interrupt mode.
 *
 * @param[in]   om_i2c       Pointer to I2C
 * @param[in]   dev_addr     Slave device address
 * @param[in]   tx_data      Pointer to buffer with data to send
 * @param[in]   tx_num       Number of data items to send
 *
 * @return errno
 *******************************************************************************
 */
om_error_t drv_i2c_slave_write_int(OM_I2C_Type *om_i2c, uint16_t dev_addr, const uint8_t *tx_data, uint16_t tx_num);
#if (RTE_GPDMA)
/**
 *******************************************************************************
 * @brief I2C slave start sending data to master device by dma mode.
 *
 * @param[in]   om_i2c       Pointer to I2C
 * @param[in]   dev_addr     Slave device address
 * @param[in]   tx_data      Pointer to buffer with data to send
 * @param[in]   tx_num       Number of data items to send
 *
 * @return errno
 *******************************************************************************
 */
om_error_t drv_i2c_slave_write_dma(OM_I2C_Type *om_i2c, uint16_t dev_addr, const uint8_t *tx_data, uint16_t tx_num);
#endif  /* (RTE_GPDMA) */
/**
 *******************************************************************************
 * @brief I2C slave start receiving data from master device by block mode.
 *
 * @param[in]   om_i2c       Pointer to I2C
 * @param[in]   dev_addr     Slave device address
 * @param[out]  rx_data      Pointer to buffer with data to receive
 * @param[in]   rx_num       Number of data items to receive
 *
 * @return errno
 *******************************************************************************
 */
om_error_t drv_i2c_slave_read(OM_I2C_Type *om_i2c, uint16_t dev_addr, uint8_t *rx_data, uint16_t rx_num, uint32_t timeout_ms);
/**
 *******************************************************************************
 * @brief I2C slave start receiving data from master device by interrupt mode.
 *
 * @param[in]   om_i2c       Pointer to I2C
 * @param[in]   dev_addr     Slave device address
 * @param[out]  rx_data      Pointer to buffer with data to receive
 * @param[in]   rx_num       Number of data items to receive
 *
 * @return errno
 *******************************************************************************
 */
om_error_t drv_i2c_slave_read_int(OM_I2C_Type *om_i2c, uint16_t dev_addr, uint8_t *rx_data, uint16_t rx_num);
#if (RTE_GPDMA)
/**
 *******************************************************************************
 * @brief I2C slave start receiving data from master device by dma mode.
 *
 * @param[in]   om_i2c       Pointer to I2C
 * @param[in]   dev_addr     Slave device address
 * @param[out]  rx_data      Pointer to buffer with data to receive
 * @param[in]   rx_num       Number of data items to receive
 *
 * @return errno
 *******************************************************************************
 */
om_error_t drv_i2c_slave_read_dma(OM_I2C_Type *om_i2c, uint16_t dev_addr, uint8_t *rx_data, uint16_t rx_num);
#endif  /* (RTE_GPDMA) */
/**
 *******************************************************************************
 * @brief Control i2c
 *
 * @param[in]   om_i2c       Pointer to I2C
 * @param[in]   control:     Control command
 * @param[in]   argu:        Control argument
 *
 * @return control status
 *******************************************************************************
 */
extern void *drv_i2c_control(OM_I2C_Type *om_i2c, i2c_control_t control, void *argu);

/**
 *******************************************************************************
 * @brief I2c interrupt service routine
 *
 * @param[in]   om_i2c       Pointer to I2C
 *******************************************************************************
 */
extern void drv_i2c_isr(OM_I2C_Type *om_i2c);

/**
 *******************************************************************************
 * @brief Get I2C TX ABRT SOURCE
 *
 * @param[in]   om_i2c       Pointer to I2C
 *
 * @return I2C TX ABRT SOURCE value
 *******************************************************************************
 */
__STATIC_FORCEINLINE uint32_t drv_i2c_get_tx_abrt(OM_I2C_Type *om_i2c)
{
    uint32_t result;
    uint32_t state;
    result = (om_i2c->RAW_INTR_STAT & I2C_INTR_TX_ABRT_MASK)&&(om_i2c->TX_ABRT_SOURCE & (I2C_TX_ABRT_SRC_ARB_LOST_MASK | I2C_TX_ABRT_SRC_7B_ADDR_NOACK_MASK | I2C_TX_ABRT_SRC_10ADDR1_NOACK_MASK | I2C_TX_ABRT_SRC_10ADDR2_NOACK_MASK | I2C_TX_ABRT_SRC_TXDATA_NOACK_MASK));
    state = (result != 0) ? 1 : 0;
    return (state);
}

/**
 *******************************************************************************
 * @brief Get I2C TIMEOUT RAW INTR STAT
 *
 * @param[in] om_i2c        Pointer to I2C
 *
 * @return I2C TIMEOUT RAW INTR STAT value
 *******************************************************************************
 */
__STATIC_FORCEINLINE uint32_t drv_i2c_get_timeout_rawstate(OM_I2C_Type *om_i2c)
{
    uint32_t state;
    state = ((om_i2c->RAW_INTR_STAT & I2C_INTR_TIME_OUT_MASK) != 0) ? 1 : 0;
    return (state);
}

/**
 *******************************************************************************
 * @brief Get I2C RX UNDER RAW INTR STAT
 *
 * @param[in] om_i2c        Pointer to I2C
 *
 * @return I2C RX UNDER RAW INTR STAT value
 *******************************************************************************
 */
__STATIC_FORCEINLINE uint32_t drv_i2c_get_rxunder_rawstate(OM_I2C_Type *om_i2c)
{
    uint32_t state;
    state = ((om_i2c->RAW_INTR_STAT & I2C_INTR_RX_UNDER_MASK) != 0) ? 1 : 0;
    return (state);
}

#ifdef __cplusplus
}
#endif


#endif  /* (RTE_I2C0 || RTE_I2C1) */

#endif  /* __DRV_I2C_H */


/** @} */
