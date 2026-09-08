#ifndef _M_I2C_H_
#define _M_I2C_H_


#include "om_driver.h"
#include "evt.h"
#include "pm.h"
#include "nvds.h"

#include "ob_config.h"
#include "omble.h"
#include "om_device.h"
#include "om_error.h"

// #include "reg_tracker.h"
// #include "m_board_app.h"

#define SOFT_IIC_ENABLE         0   // 使用软件I2C
#define HARDWRAE_IIC_ENABLE     1   // 使用硬件I2C

#if HARDWRAE_IIC_ENABLE

om_error_t I2C_Init(OM_I2C_Type *om_i2c);

om_error_t I2C_Write(OM_I2C_Type *om_i2c, uint8_t dev_addr, uint8_t reg_addr,
							uint8_t *write_data, uint8_t data_length);	
om_error_t I2C_Read(OM_I2C_Type *om_i2c, uint8_t dev_addr, uint8_t reg_addr, 
					uint8_t *read_data, uint8_t data_length);
#endif

#if SOFT_IIC_ENABLE
#define I2C_SET_PIN(pin, level) \
    ((level) ? drv_gpio_write(OM_GPIO0, GPIO_MASK(pin), GPIO_LEVEL_HIGH) \
             : drv_gpio_write(OM_GPIO0, GPIO_MASK(pin), GPIO_LEVEL_LOW))

// 0U----output, 1U----input
#define I2C_SET_PIN_MODE(pin, mode)	\
	((mode) ? drv_gpio_set_dir(OM_GPIO0, GPIO_MASK(pin), GPIO_DIR_INPUT) \
			: drv_gpio_set_dir(OM_GPIO0, GPIO_MASK(pin), GPIO_DIR_OUTPUT))

#define	I2C_READ_PIN(pin)	(drv_gpio_read(OM_GPIO0, GPIO_MASK(pin))>>pin ? 1 : 0)	

typedef struct {
    uint8_t device_addr;   // 从机设备地址
    uint8_t scl_pin;       // SCL引脚
    uint8_t sda_pin;       // SDA引脚
	uint8_t int_pin;	   // 中断
    uint32_t delay_us;     // 延迟(微秒)
} I2C_Soft_Config;
// extern I2C_Soft_Config i2c_cfg;

void I2C_Soft_Init(I2C_Soft_Config config);

// void I2C_Soft_Start(void);
// void I2C_Soft_Stop(void);
// void I2C_Soft_Ack(void);
// void I2C_Soft_NAck(void);
// bool I2C_Soft_WaitAck(void);

// bool I2C_Soft_WriteByte(uint8_t byte);
// uint8_t I2C_Soft_ReadByte(void);

bool I2C_Soft_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data);
bool I2C_Soft_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data);
bool I2C_Soft_WriteMulti(uint8_t dev_addr, uint8_t reg_addr, const uint8_t *data, uint16_t count);
bool I2C_Soft_ReadMulti(uint8_t dev_addr, uint8_t reg_addr, uint8_t *buf, uint16_t count);


#endif

					
#endif

