#include "m_i2c.h"
#include "om_log.h"

#if SOFT_IIC_ENABLE
static I2C_Soft_Config i2c_config;

void I2C_Soft_Init(I2C_Soft_Config config) 
{
    // i2c_config = *config;
	
	i2c_config = config;
    
	pin_config_t pin_config[] = {
		{i2c_config.scl_pin, {PINMUX_GPIO_MODE_CFG}, PMU_PIN_MODE_OD, PMU_PIN_DRIVER_CURRENT_NORMAL},
		{i2c_config.sda_pin, {PINMUX_GPIO_MODE_CFG}, PMU_PIN_MODE_OD, PMU_PIN_DRIVER_CURRENT_NORMAL},
		// {i2c_config.int_pin, {PINMUX_GPIO_MODE_CFG}, PMU_PIN_MODE_PU, PMU_PIN_DRIVER_CURRENT_NORMAL},
	};

	gpio_config_t gpio_config[] = {
		{OM_GPIO0, i2c_config.scl_pin,   GPIO_DIR_OUTPUT, GPIO_LEVEL_HIGH, GPIO_TRIG_NONE},
		{OM_GPIO0, i2c_config.sda_pin,   GPIO_DIR_OUTPUT, GPIO_LEVEL_HIGH, GPIO_TRIG_NONE},
		// {OM_GPIO0, i2c_config.int_pin,   GPIO_DIR_INPUT,  GPIO_LEVEL_HIGH, GPIO_TRIG_FALLING_EDGE},
	};

    // Init PIN
    drv_pin_init(pin_config, sizeof(pin_config) / sizeof(pin_config[0]));
    // Init GPIO
    drv_gpio_init(gpio_config, sizeof(gpio_config) / sizeof(gpio_config[0]));

    // 初始状态: SCL和SDA都为高
	I2C_SET_PIN(i2c_config.scl_pin, 1);
	I2C_SET_PIN(i2c_config.sda_pin, 1);

#if REG_ALL_WRITTEN_ENABLE
	reg_tracker_init();
#endif
}

static void I2C_Delay(void) 
{
    DRV_DELAY_US(i2c_config.delay_us); 
}

static void I2C_Soft_Start(void) 
{
	I2C_SET_PIN(i2c_config.sda_pin, 1); // SDA = 1
	I2C_SET_PIN(i2c_config.scl_pin, 1); // SCL = 1
    I2C_Delay();
    
    I2C_SET_PIN(i2c_config.sda_pin, 0); // SDA = 0
    I2C_Delay();
    
    I2C_SET_PIN(i2c_config.scl_pin, 0);; // SCL = 0
    I2C_Delay();
}

static void I2C_Soft_Stop(void) 
{
    I2C_SET_PIN(i2c_config.sda_pin, 0); // SDA = 0
    I2C_SET_PIN(i2c_config.scl_pin, 0);; // SCL = 0
    I2C_Delay();
    
    I2C_SET_PIN(i2c_config.scl_pin, 1); // SCL = 1
    I2C_Delay();
    
   	I2C_SET_PIN(i2c_config.sda_pin, 1); // SDA = 1
    I2C_Delay();
}

static void I2C_Soft_Ack(void) 
{
    I2C_SET_PIN(i2c_config.sda_pin, 0); // SDA = 0
    I2C_Delay();
    
    I2C_SET_PIN(i2c_config.scl_pin, 1);   // SCL = 1
    I2C_Delay();
    
    I2C_SET_PIN(i2c_config.scl_pin, 0); // SCL = 0
    I2C_Delay();
}

static void I2C_Soft_NAck(void) 
{
    I2C_SET_PIN(i2c_config.sda_pin, 1);   // SDA = 1
    I2C_Delay();
    
    I2C_SET_PIN(i2c_config.scl_pin, 1);   // SCL = 1
    I2C_Delay();
    
    I2C_SET_PIN(i2c_config.scl_pin, 0); // SCL = 0
    I2C_Delay();
}

static bool I2C_Soft_WaitAck(void) 
{
    I2C_SET_PIN(i2c_config.sda_pin, 1);   // 释放SDA
    I2C_SET_PIN_MODE(i2c_config.sda_pin, 1); // SDA改为输入
    
    I2C_SET_PIN(i2c_config.scl_pin, 1);   // SCL = 1
    I2C_Delay();
    
    bool ack = !I2C_READ_PIN(i2c_config.sda_pin); // 读取ACK
    
    I2C_SET_PIN(i2c_config.scl_pin, 0); // SCL = 0
    I2C_SET_PIN_MODE(i2c_config.sda_pin, 0); // SDA改回输出
    
    return ack;
}

static bool I2C_Soft_WriteByte(uint8_t byte) 
{
    for (uint8_t i = 0; i < 8; i++) {
        if (byte & 0x80) {
            I2C_SET_PIN(i2c_config.sda_pin, 1); // SDA = 1
        } else {
            I2C_SET_PIN(i2c_config.sda_pin, 0); // SDA = 0
        }
        I2C_Delay();
        
        I2C_SET_PIN(i2c_config.scl_pin, 1); // SCL = 1
        I2C_Delay();
        
        I2C_SET_PIN(i2c_config.scl_pin, 0); // SCL = 0
        I2C_Delay();
        
        byte <<= 1;
    }
    
    return I2C_Soft_WaitAck();
}

static uint8_t I2C_Soft_ReadByte(void) 
{
    uint8_t byte = 0;
    
    I2C_SET_PIN(i2c_config.sda_pin, 1);   // 释放SDA
    I2C_SET_PIN_MODE(i2c_config.sda_pin, 1); // SDA改为输入
    
    for (uint8_t i = 0; i < 8; i++) {
        I2C_SET_PIN(i2c_config.scl_pin, 1); // SCL = 1
        I2C_Delay();
        
        byte <<= 1;
        if (I2C_READ_PIN(i2c_config.sda_pin))
            byte |= 0x01;
        
        I2C_SET_PIN(i2c_config.scl_pin, 0); // SCL = 0
        I2C_Delay();
    }
    
    I2C_SET_PIN_MODE(i2c_config.sda_pin, 0); // SDA改回输出
    
    return byte;
}

bool I2C_Soft_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data)
{
    I2C_Soft_Start();
    
    bool success = I2C_Soft_WriteByte(dev_addr << 1);  // 设备地址 + 写
    success &= I2C_Soft_WriteByte(reg_addr);          // 寄存器地址
    success &= I2C_Soft_WriteByte(data);              // 数据
    
    I2C_Soft_Stop();

#if REG_ALL_WRITTEN_ENABLE
	if(success)
	{
		// OM_LOG_DEBUG("reg_addr:0x%02X, data:0x%02X\n", reg_addr, data);
		// 记录写过的寄存器
		reg_tracker_log_write(reg_addr, data);	
	}
#endif
    return success;
}

bool I2C_Soft_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data) 
{
    I2C_Soft_Start();
    
    bool success = I2C_Soft_WriteByte(dev_addr << 1);  // 设备地址 + 写
    success &= I2C_Soft_WriteByte(reg_addr);          // 寄存器地址

    I2C_Soft_Start();                                 
    success &= I2C_Soft_WriteByte((dev_addr << 1) | 1); // 设备地址 + 读
    
    if (success) {
        *data = I2C_Soft_ReadByte();
        I2C_Soft_NAck();  // 发送NACK结束读取
    }
    
    I2C_Soft_Stop();
    return success;
}

bool I2C_Soft_WriteMulti(uint8_t dev_addr, uint8_t reg_addr, const uint8_t *data, uint16_t count) 
{
    I2C_Soft_Start();
    
    bool success = I2C_Soft_WriteByte(dev_addr << 1);  // 设备地址 + 写
    success &= I2C_Soft_WriteByte(reg_addr);          // 寄存器地址
    
    for (uint16_t i = 0; i < count && success; i++) {
        success &= I2C_Soft_WriteByte(data[i]);
    }
    
    I2C_Soft_Stop();
    return success;
}

bool I2C_Soft_ReadMulti(uint8_t dev_addr, uint8_t reg_addr, uint8_t *buf, uint16_t count) 
{
    I2C_Soft_Start();
    
    bool success = I2C_Soft_WriteByte(dev_addr << 1);  // 设备地址 + 写
    success &= I2C_Soft_WriteByte(reg_addr);          // 寄存器地址
    
    I2C_Soft_Start();                                 
    success &= I2C_Soft_WriteByte((dev_addr << 1) | 1); // 设备地址 + 读
    
    if (success) {
        for (uint16_t i = 0; i < count; i++) {
            buf[i] = I2C_Soft_ReadByte();
            // 最后一个字节发送NACK，其他发送ACK
            if (i == count - 1) {
                I2C_Soft_NAck();
            } else {
                I2C_Soft_Ack();
            }
        }
    }
    
    I2C_Soft_Stop();
    return success;
}


#endif

#if HARDWRAE_IIC_ENABLE
/*************************
 * 		硬件I2C
 * 
 ************************/
/// I2C pin configuration
//static pin_config_t pin_i2c0_cfg [] = {
//	{PD_I2C0_SCL, {MUX_I2C0_SCL}, PMU_PIN_MODE_OD_PU, PMU_PIN_DRIVER_CURRENT_NORMAL},
//    {PD_I2C0_SDA, {MUX_I2C0_SDA}, PMU_PIN_MODE_OD_PU, PMU_PIN_DRIVER_CURRENT_NORMAL},
//};

// I2C init
om_error_t I2C_Init(OM_I2C_Type *om_i2c)
{
	if (om_i2c == NULL) {
        return OM_ERROR_UNSUPPORTED;
    }

    i2c_config_t cfg;
	memset(&cfg, 0, sizeof(i2c_config_t));
    cfg.mode = I2C_MODE_MASTER;

    // 根据I2C实例设置速率
    if (om_i2c == OM_I2C0) {
        cfg.speed = I2C_SPEED_100K;  // 100K

//		drv_pin_init(pin_i2c0_cfg, sizeof(pin_i2c0_cfg) / sizeof(pin_i2c0_cfg[0]));
    } 
	// else if (om_i2c == OM_I2C1) {
    //     cfg.speed = I2C_SPEED_400K;  // 400K

	// 	drv_pin_init(pin_i2c1_cfg, sizeof(pin_i2c1_cfg) / sizeof(pin_i2c1_cfg[0]));
	// 	drv_gpio_init(gpio_i2c1_cfg, sizeof(gpio_i2c1_cfg) / sizeof(gpio_i2c1_cfg[0]));
    // } 
	else {
        return OM_ERROR_UNSUPPORTED;
    }

    return drv_i2c_init(om_i2c, &cfg);

}

/**
 *******************************************************************************
 * @brief 硬件I2C write
 * @param[in]   om_i2c   		指向 I2C 控制器实例的指针
 * @param[in]   dev_addr   		设备地址
 * @param[in]   reg_addr		寄存器地址	
 * @param[in]   write_data		写的数据
 * @param[in]   data_length		数据长度
 * @return status:
 *    - OM_ERROR_OK:         write done
 *    - others:              No
 *******************************************************************************
 */
om_error_t I2C_Write(OM_I2C_Type *om_i2c, uint8_t dev_addr, uint8_t reg_addr,
							uint8_t *write_data, uint8_t data_length)
{
	//	dev_addr = (dev_addr << 1) & 0xFE;
	
    uint8_t new_data[data_length + 1];
    new_data[0] = reg_addr; // 寄存器地址

	if (write_data != NULL && data_length > 0)
    {
        for (int i = 0; i < data_length; i++)
        {
            new_data[i + 1] = write_data[i];
        }
    }
    
    return drv_i2c_master_write(om_i2c, dev_addr, new_data, data_length + 1, DRV_MAX_DELAY);
}


/**
 *******************************************************************************
 * @brief 硬件I2C read
 * @param[in]   dev_addr   		设备地址
 * @param[in]   reg_addr		寄存器地址	
 * @param[in]   read_data		读取的数据
 * @param[in]   data_length		数据长度
 * @return status:
 *    - OM_ERROR_OK:         receive done
 *    - others:              No
 *******************************************************************************
 */
om_error_t I2C_Read(OM_I2C_Type *om_i2c, uint8_t dev_addr, uint8_t reg_addr, 
					uint8_t *read_data, uint8_t data_length)
{	
//	dev_addr = (dev_addr << 1) | 0x01;

	return drv_i2c_master_read(om_i2c, dev_addr, &reg_addr, 1, read_data, data_length, DRV_MAX_DELAY);
}

#endif