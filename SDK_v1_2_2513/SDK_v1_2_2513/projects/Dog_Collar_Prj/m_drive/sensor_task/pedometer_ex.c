#include "common_def.h"

#include "pedometer_ex.h"
#include "inv_imu_transport.h"
#include "inv_imu_driver.h"
#include "inv_imu_apex.h"

static struct inv_imu_device imu_dev; /* Driver structure */
static uint8_t dmp_odr_hz; /* DMP ODR */
static bool int1_status = false;
static uint64_t all_step_num = 0;

static uint64_t last_step_num = 0;
extern int inv_io_hal_read_reg(struct inv_imu_serif *serif, uint8_t reg, uint8_t *rbuffer, uint32_t rlen);
extern int inv_io_hal_write_reg(struct inv_imu_serif *serif, uint8_t reg, const uint8_t *tbuffer, uint32_t rlen);
// 加速度数据结构
typedef struct {
    int16_t x;
    int16_t y; 
    int16_t z;
    float x_g;
    float y_g;
    float z_g;
    uint32_t timestamp;
} accel_data_t;
// 加速度计配置（根据实际传感器调整）
typedef struct {
    float sensitivity;      // 灵敏度 (LSB/g)
    float zero_g_offset;    // 零g偏置
    int16_t x_offset;       // X轴校准偏移
    int16_t y_offset;       // Y轴校准偏移  
    int16_t z_offset;       // Z轴校准偏移
    float x_scale;          // X轴缩放因子
    float y_scale;          // Y轴缩放因子
    float z_scale;          // Z轴缩放因子
} accel_config_t;

// 默认配置（需要根据实际传感器调整）
static accel_config_t accel_config = {
    .sensitivity = 16384.0f,    // ±2g范围，典型值
    .zero_g_offset = 0.0f,
    .x_offset = 0,
    .y_offset = 0,
    .z_offset = 0,
    .x_scale = 1.0f,
    .y_scale = 1.0f,
    .z_scale = 1.0f
};
// 读取单个加速度计轴数据
bool read_accel_axis(uint16_t reg_high, uint16_t reg_low, int16_t *axis_data) {
    uint8_t data_high = 0;
    uint8_t data_low = 0;
    // 读取高字节
	
    if (inv_imu_read_reg(&imu_dev,reg_high,1, &data_high) != 0) {
        return false;
    }
    
    // 读取低字节
    if (inv_imu_read_reg(&imu_dev,reg_low,1, &data_low) != 0) {
        return false;
    }
    
    // 组合成16位有符号整数
    *axis_data = (int16_t)((data_high << 8) | data_low);
    
    return true;
}
// 批量读取（优化版本）
bool read_accelerometer_bulk(accel_data_t *accel_data) {
    uint8_t accel_raw_data[6] = {0};
    
    // 一次性读取所有加速度寄存器（假设寄存器连续）
    if (inv_imu_read_reg(&imu_dev,ACCEL_DATA_X1,6,&accel_raw_data[0])!= 0) {
        log_debug("sensor read error!\r\n");
        return false;
    }
    
    // 解析数据（注意字节顺序）
    accel_data->x = (int16_t)((accel_raw_data[0] << 8) | accel_raw_data[1]);
    accel_data->y = (int16_t)((accel_raw_data[2] << 8) | accel_raw_data[3]);
    accel_data->z = (int16_t)((accel_raw_data[4] << 8) | accel_raw_data[5]);
   // accel_data->timestamp = HAL_GetTick();  
    return true;
}
// 将原始数据转换为g值
void convert_raw_to_g(accel_data_t *accel_data) {
    // 应用校准偏移
    int16_t calibrated_x = accel_data->x - accel_config.x_offset;
    int16_t calibrated_y = accel_data->y - accel_config.y_offset;
    int16_t calibrated_z = accel_data->z - accel_config.z_offset;
    
    // 转换为g值
    accel_data->x_g = (calibrated_x / accel_config.sensitivity) * accel_config.x_scale;
    accel_data->y_g = (calibrated_y / accel_config.sensitivity) * accel_config.y_scale;
    accel_data->z_g = (calibrated_z / accel_config.sensitivity) * accel_config.z_scale;
}
uint8_t get_sensor_xyz_data(void)
{
	accel_data_t accel_data;
	read_accelerometer_bulk(&accel_data);
	log_debug("x = %d y = %d z = %d \r\n",accel_data.x,accel_data.y,accel_data.z);
//	if((accel_data.z != 0) && (accel_data.z>accel_data.x) && (accel_data.z>accel_data.y))
	if((accel_data.z != 0) && (accel_data.x != 0) && (accel_data.y != 0))
	{
		return 1;
	}
	return 0;
}

static int inv_imu_init_t(struct inv_imu_device *s, const struct inv_imu_serif *serif,
                 void (*sensor_event_cb)(inv_imu_sensor_event_t *event))
{
	int status = 0;
	memset(s, 0, sizeof(*s));

	/* Verify validity of `serif` variable */
	if (serif == NULL || serif->read_reg == NULL || serif->write_reg == NULL)
		return INV_ERROR;

	s->transport.serif = *serif;

//	/* Supply ramp time max is 3 ms */
//	inv_imu_sleep_ms(3);
	/* Register sensor event callback */
	s->sensor_event_cb = sensor_event_cb;

	/* Make sure `need_mclk_cnt` is cleared */
	s->transport.need_mclk_cnt = 0;

	return status;	
}

int inv_imu_set_config_pedometer(struct inv_imu_device *s, const inv_imu_interrupt_parameter_t *it)
{
	int     status = 0;
	uint8_t data[2];
	
	status |= inv_imu_read_reg(s, INT_SOURCE6_MREG1, 1, &data[0]);
	data[0] |= ((it->INV_STEP_DET != 0) << INT_SOURCE6_STEP_DET_INT1_EN_POS);
	data[0] |= ((it->INV_STEP_CNT_OVFL != 0) << INT_SOURCE6_STEP_CNT_OFL_INT1_EN_POS);
	status |= inv_imu_write_reg(s, INT_SOURCE6_MREG1, 1, &data[0]);

	return status;
}

static int configure_pedometer()
{
	int                       rc = 0;
	inv_imu_apex_parameters_t apex_inputs;
	bool use_lp_config = true;

	/* Disable Pedometer before configuring it */
	rc |= inv_imu_apex_disable_pedometer(&imu_dev);

	if(use_lp_config) 
	{
			rc |= inv_imu_set_accel_frequency(&imu_dev, ACCEL_CONFIG0_ODR_25_HZ);
			rc |= inv_imu_apex_set_frequency(&imu_dev, APEX_CONFIG1_DMP_ODR_25Hz);
			dmp_odr_hz = 25;
	} 
	else 
	{
			rc |= inv_imu_set_accel_frequency(&imu_dev, ACCEL_CONFIG0_ODR_50_HZ);
			rc |= inv_imu_apex_set_frequency(&imu_dev, APEX_CONFIG1_DMP_ODR_50Hz);
			dmp_odr_hz = 50;
	}

	/* Set APEX parameters */
	rc |= inv_imu_apex_init_parameters_struct(&imu_dev, &apex_inputs);
	
	rc |= inv_imu_apex_configure_parameters(&imu_dev, &apex_inputs);

	/* If POWER_SAVE mode is enabled, WOM has to be enabled */  //已配置
//	if (apex_inputs.power_save == APEX_CONFIG0_DMP_POWER_SAVE_EN) {
//		/* Configure and enable WOM to wake-up the DMP once it goes in power save mode */
//		rc |= inv_imu_configure_wom(&imu_dev, WOM_THRESHOLD, WOM_THRESHOLD, WOM_THRESHOLD,
//		                            WOM_CONFIG_WOM_INT_MODE_ANDED, WOM_CONFIG_WOM_INT_DUR_1_SMPL);
//		rc |= inv_imu_enable_wom(&imu_dev);
//	}

	/* Enable Pedometer */
	rc |= inv_imu_apex_enable_pedometer(&imu_dev);

	return rc;
}

static int setup_pedometer(void)
{
	int                           rc = 0;
	struct inv_imu_serif          imu_serif;
	uint8_t                       whoami;
//	inv_imu_int1_pin_config_t     int1_pin_config;
	inv_imu_interrupt_parameter_t int1_config = { (inv_imu_interrupt_value)0 };

	/* Initialize serial interface between MCU and IMU */
	imu_serif.context    = 0; /* no need */
	imu_serif.read_reg   = inv_io_hal_read_reg;
	imu_serif.write_reg  = inv_io_hal_write_reg;
	imu_serif.max_read   = 1024 * 32; /* maximum number of bytes allowed per serial read */
	imu_serif.max_write  = 1024 * 32; /* maximum number of bytes allowed per serial write */
	imu_serif.serif_type = UI_SPI4;

//	/* Init device */
	rc |= inv_imu_init_t(&imu_dev, &imu_serif, NULL);

	
//#if SERIF_TYPE == UI_SPI4
//	/* Configure slew-rate to 19 ns (required when using EVB) */
//	rc |= inv_imu_set_spi_slew_rate(&imu_dev, DRIVE_CONFIG3_SPI_SLEW_RATE_MAX_19_NS);
//	SI_CHECK_RC(rc);
//#endif

	/* Check WHOAMI */
	rc |= inv_imu_get_who_am_i(&imu_dev, &whoami);
	log_debug("pedometer i am %x\r\n", whoami);
	

//	/* Configure interrupts sources */
	int1_config.INV_STEP_DET      = INV_IMU_ENABLE;
	int1_config.INV_STEP_CNT_OVFL = INV_IMU_ENABLE;
	rc |= inv_imu_set_config_pedometer(&imu_dev, &int1_config);

//	/* Disabling FIFO usage to optimize power consumption */
//	rc |= inv_imu_configure_fifo(&imu_dev, INV_IMU_FIFO_DISABLED); //已配置

//	/* Set 2X averaging to minimize power consumption */
//	rc |= inv_imu_set_accel_lp_avg(&imu_dev, ACCEL_CONFIG1_ACCEL_FILT_AVG_2);  //已配置

//	/* Enable accel in LP mode */
//	rc |= inv_imu_enable_accel_low_power_mode(&imu_dev); //已配置

	rc |= configure_pedometer();

	return rc;
}

int pedometer_init(void)
{
		int rc = 0;
		rc |= setup_pedometer();
		return rc;
}

void set_int1_status(void)
{
		int1_status = true;
}

bool get_int1_status(void)
{
		return int1_status;
}	

void clear_int1_status(void)
{
	 int1_status = false;
}

uint64_t get_step_num(void)
{
	uint64_t all_step_num_tmp=all_step_num;
	all_step_num_tmp = all_step_num - last_step_num ;
	last_step_num = all_step_num;
	return all_step_num_tmp;
}

void set_step_num(void)
{
		if(get_int1_status())
		{
				uint8_t         int_status3;
				int        			rc;
				static uint8_t  step_cnt_ovflw = 0;
				static uint64_t step_cnt       = 0;

				/* Clear interrupt flag */
				clear_int1_status();


				/* Read Pedometer interrupt status */
				rc |= inv_imu_read_reg(&imu_dev, INT_STATUS3, 1, &int_status3);

				if(int_status3 & INT_STATUS3_STEP_CNT_OVF_INT_MASK) 
				{
						step_cnt_ovflw++;
				}

				if (int_status3 & INT_STATUS3_STEP_DET_INT_MASK) 
				{
						inv_imu_apex_step_activity_t ped_data;

						rc |= inv_imu_apex_get_data_activity(&imu_dev, &ped_data);

						/* Compute step count */
						step_cnt = ped_data.step_cnt + step_cnt_ovflw * UINT16_MAX;
					
						all_step_num = step_cnt;

//						log_debug("step_cnt = %d\r\n", step_cnt);

				}
		}		
}


