#include "om_driver.h"
#include "imu_ex.h"
#include "spi_ex.h"
#include "inv_imu_transport.h"
#include "cmsis_os2.h"
#include "om_log.h"
#include "InvError.h"
#include "inv_imu_driver.h"
#include "common_def.h"

#include "pedometer_ex.h"
#include "RingBuffer.h"
#include "service_tspp_define.h"

#include "pet_behavior_ai.h"
#include "inv_imu_apex.h"
#define PET_AI_CONTEXT_SIZE 	6016
static uint8_t ctx[PET_AI_CONTEXT_SIZE] __attribute__((aligned(8))); 
static char sensor_ble_log_bug[50] = {0};
static pet_ai_sample_t pet_ai_sample;   //算法参数结构体
static pet_ai_result_t pet_ai_result;		//算法返回结果
static bool ai_behavior_state_flag = false;

static state_posture_t pet_ai_result_class_id_vlue = STATE_POSTURE_OTHER;
/* WOM threshold to be applied to IMU, ranges from 1 to 255, in 4mg unit */
static uint8_t wom_threshold = WOM_THRESHOLD_INITIAL_MG / 4;

static struct inv_imu_serif icm_serif;
static struct inv_imu_device icm_driver;
extern RINGBUFFER(timestamp_buffer, 64, uint64_t);
static Motion_Level motion_level = MOTION_LESS;  //运动等级
static Motion_Level last_motion_level = MOTION_LESS;  //运动等级
static uint32_t motion_int_num_per_min = 0; //每分钟运动中断个数
static uint8_t last_min = 0; //记录时间，用以实现每分钟输出一次运动强度

static bool imu_init_ok = false;
static uint8_t last_class_id = 0;
uint32_t get_motion_int_num(void);
void clear_motion_int_num(void);
/**
*** get pet ai result class id
**/
state_posture_t get_pet_ai_result_class_id(void)
{
	return pet_ai_result_class_id_vlue;
}
/*
* 设置宠物AI结果类ID
*/
void set_pet_ai_result_class_id(state_posture_t class_id)
{
	pet_ai_result_class_id_vlue = class_id;
}
/**
*** get ai behavior state
**/
bool get_ai_behavior_state(void)
{
	return ai_behavior_state_flag;
}

// 辅助函数：发送AI状态
static void send_ai_status(int ret_code, uint8_t class_id) 
{   
    int len = 0;
    
    if (ret_code == PET_AI_HAS_RESULT) 
		{
        len = snprintf(sensor_ble_log_bug, sizeof(sensor_ble_log_bug), "CLASS_ID:%d\r\n", class_id);  // 例如发送 "CLASS_ID:3\r\n"
    } 
		else if (ret_code < 0) 
		{
        len = snprintf(sensor_ble_log_bug, sizeof(sensor_ble_log_bug), "AI_ERR:%d\r\n", ret_code); // 发送 "AI_ERR:-1\r\n"
    } 
		else 
		{
        return; // ret == 0 不发送
    }
    
    if (len > 0 && len < sizeof(sensor_ble_log_bug)) 
		{
        tspp_send((uint8_t *)sensor_ble_log_bug, len);
    }
}

/***********************************************************************
 * @brief  delay ms
 ***********************************************************************/
void inv_imu_sleep_ms(uint32_t ms)
{
   osDelay(osMS2TicksRound(ms));
}

/***********************************************************************
 * @brief  get sys time us
 ***********************************************************************/
uint64_t inv_imu_get_time_ms(void)
{
		return osKernelGetTickCount();
}

void inv_imu_read_reg_clear(void)
{	uint8_t  int_status = 0;
		inv_imu_read_reg(&icm_driver, INV_INT_STATUS, 1, &int_status);   // 清除中断标志
}
/**
*** spi read
**/
int inv_io_hal_read_reg(struct inv_imu_serif *serif, uint8_t reg, uint8_t *rbuffer, uint32_t rlen)
{
		(void)serif;
		if (rlen > UINT16_MAX - 1)
		{
			return -1; /* Not supported */
		}
		return spi_read(reg, rbuffer, rlen);
}

/**
*** spi write
**/
int inv_io_hal_write_reg(struct inv_imu_serif *serif, uint8_t reg, const uint8_t *tbuffer, uint32_t rlen)
{
	(void)serif;

	if (rlen > UINT16_MAX - 1)
	{
		return -1; /* Not supported */
	}
	return spi_write(reg, tbuffer, rlen);

}

/**
*** setup_mcu
**/
int setup_mcu(struct inv_imu_serif *icm_serif)
{
	int rc = 0;

	imu_hw_init();

	/* Initialize serial interface between MCU and IMU */
	icm_serif->context    = 0; /* no need */
	icm_serif->read_reg   = inv_io_hal_read_reg;
	icm_serif->write_reg  = inv_io_hal_write_reg;
	icm_serif->max_read   = 1024 * 32; /* maximum number of bytes allowed per serial read */
	icm_serif->max_write  = 1024 * 32; /* maximum number of bytes allowed per serial write */
	icm_serif->serif_type = UI_SPI4;
	
	return rc;
}

/**
*** setup_imu_device
**/
#define SCALED_DATA_G_DPS 0
#define USE_FIFO 1
#define INT20_MAX 524287

#define USE_HIGH_RES_MODE 0

#if SCALED_DATA_G_DPS
static void get_accel_and_gyr_fsr(uint16_t *accel_fsr_g, uint16_t *gyro_fsr_dps)
{
	ACCEL_CONFIG0_FS_SEL_t accel_fsr_bitfield;
	GYRO_CONFIG0_FS_SEL_t  gyro_fsr_bitfield;

	inv_imu_get_accel_fsr(&icm_driver, &accel_fsr_bitfield);
	switch (accel_fsr_bitfield) {
#if !ICM_HFSR_SUPPORTED
	case ACCEL_CONFIG0_FS_SEL_2g:
		*accel_fsr_g = 2;
		break;
#endif
	case ACCEL_CONFIG0_FS_SEL_4g:
		*accel_fsr_g = 4;
		break;
	case ACCEL_CONFIG0_FS_SEL_8g:
		*accel_fsr_g = 8;
		break;
	case ACCEL_CONFIG0_FS_SEL_16g:
		*accel_fsr_g = 16;
		break;
#if ICM_HFSR_SUPPORTED
	case ACCEL_CONFIG0_FS_SEL_32g:
		*accel_fsr_g = 32;
		break;
#endif
	default:
		*accel_fsr_g = -1;
		break;
	}

	inv_imu_get_gyro_fsr(&icm_driver, &gyro_fsr_bitfield);
	switch (gyro_fsr_bitfield) {
#if !ICM_HFSR_SUPPORTED
	case GYRO_CONFIG0_FS_SEL_250dps:
		*gyro_fsr_dps = 250;
		break;
#endif
	case GYRO_CONFIG0_FS_SEL_500dps:
		*gyro_fsr_dps = 500;
		break;
	case GYRO_CONFIG0_FS_SEL_1000dps:
		*gyro_fsr_dps = 1000;
		break;
	case GYRO_CONFIG0_FS_SEL_2000dps:
		*gyro_fsr_dps = 2000;
		break;
#if ICM_HFSR_SUPPORTED
	case GYRO_CONFIG0_FS_SEL_4000dps:
		*gyro_fsr_dps = 4000;
		break;
#endif
	default:
		*gyro_fsr_dps = -1;
		break;
	}
}
#endif
extern uint8_t m_trans_enable_get(void);
	int32_t  accel[3]={0};
	int32_t  gyro[3]={0};
		uint64_t timestamp = 0;
	uint8_t tspp_send(uint8_t *data, tspp_size_t len);
	
uint8_t send_data_tmp[36]={0};
void sensor_data_send(int64_t time,int32_t *accel,int32_t *gyro)
{
	 // 帧头
    send_data_tmp[0] = 0x5A;
    send_data_tmp[1] = 0xA5;
	 // 1. 填充时间戳 (8字节，大端)
	uint8_t *p = send_data_tmp + 2;
	for (int i = 0; i < 8; i++) {
			p[i] = (time >> ((7 - i) * 8)) & 0xFF;   // 最高字节在最低地址
	}

	// 2. 填充加速度 (3个int32_t，共12字节，大端)
	p = send_data_tmp + 10;
	for (int i = 0; i < 3; i++) {
			int32_t val = accel[i];
			for (int j = 0; j < 4; j++) {
					p[i * 4 + j] = (val >> ((3 - j) * 8)) & 0xFF;   // 最高字节在前
			}
	}

	// 3. 填充角速度 (3个int32_t，共12字节，大端)
	p = send_data_tmp + 22;
	for (int i = 0; i < 3; i++) {
			int32_t val = gyro[i];
			for (int j = 0; j < 4; j++) {
					p[i * 4 + j] = (val >> ((3 - j) * 8)) & 0xFF;
			}
	}
	    // 帧尾
    send_data_tmp[34] = 0x0D;
    send_data_tmp[35] = 0x0A;
	
//		for(int i=0;i<sizeof(send_data_tmp);i++)
//		{
//			log_debug("%x ",send_data_tmp[i]);
//		}
//		log_debug("\r\n");
	//	OM_LOG_HEXDUMP(OM_LOG_INFO,send_data_tmp,sizeof(send_data_tmp),1);
	tspp_send(send_data_tmp,sizeof(send_data_tmp));
}
/*
* @brief  获取宠物AI结果的分类ID
*/
uint8_t pet_ai_result_class_id_get(void)
{
	return pet_ai_result.class_id;
}
void imu_callback(inv_imu_sensor_event_t *event)
{


#if SCALED_DATA_G_DPS
	float    accel_g[3];
	float    gyro_dps[3];
	float    temp_degc;
	uint16_t accel_fsr_g;
	uint16_t gyro_fsr_dps;
	int      data_length_max;
#endif

#if USE_FIFO
	static uint64_t last_fifo_timestamp = 0;
	static uint32_t rollover_num        = 0;

	// Handle rollover
	if (last_fifo_timestamp > event->timestamp_fsync)
		rollover_num++;
	last_fifo_timestamp = event->timestamp_fsync;

	// Compute timestamp in us
	timestamp = event->timestamp_fsync + rollover_num * UINT16_MAX;
	timestamp *= inv_imu_get_timestamp_resolution_us(&icm_driver);

	if (icm_driver.fifo_highres_enabled) {
		accel[0] = ((int32_t)event->accel[0] << 4) | event->accel_high_res[0];
		accel[1] = ((int32_t)event->accel[1] << 4) | event->accel_high_res[1];
		accel[2] = ((int32_t)event->accel[2] << 4) | event->accel_high_res[2];

		gyro[0] = ((int32_t)event->gyro[0] << 4) | event->gyro_high_res[0];
		gyro[1] = ((int32_t)event->gyro[1] << 4) | event->gyro_high_res[1];
		gyro[2] = ((int32_t)event->gyro[2] << 4) | event->gyro_high_res[2];

	} else {
		accel[0] = event->accel[0];
		accel[1] = event->accel[1];
		accel[2] = event->accel[2];

		gyro[0] = event->gyro[0];
		gyro[1] = event->gyro[1];
		gyro[2] = event->gyro[2];
	}
#else
	inv_disable_irq();
	if (!RINGBUFFER_EMPTY(&timestamp_buffer))
		RINGBUFFER_POP(&timestamp_buffer, &timestamp);
	inv_enable_irq();

	accel[0] = event->accel[0];
	accel[1] = event->accel[1];
	accel[2] = event->accel[2];

	gyro[0] = event->gyro[0];
	gyro[1] = event->gyro[1];
	gyro[2] = event->gyro[2];

	// Force sensor_mask so it gets displayed below
	event->sensor_mask |= (1 << INV_SENSOR_TEMPERATURE);
	event->sensor_mask |= (1 << INV_SENSOR_ACCEL);
	event->sensor_mask |= (1 << INV_SENSOR_GYRO);
#endif

#if SCALED_DATA_G_DPS
	/*
	 * Convert raw data into scaled data in g and dps
	*/
	get_accel_and_gyr_fsr(&accel_fsr_g, &gyro_fsr_dps);
	if (icm_driver.fifo_highres_enabled)
		data_length_max = INT20_MAX;
	else
		data_length_max = INT16_MAX;

	accel_g[0]  = (float)(accel[0] * accel_fsr_g) / (float)data_length_max;
	accel_g[1]  = (float)(accel[1] * accel_fsr_g) / (float)data_length_max;
	accel_g[2]  = (float)(accel[2] * accel_fsr_g) / (float)data_length_max;
	gyro_dps[0] = (float)(gyro[0] * gyro_fsr_dps) / (float)data_length_max;
	gyro_dps[1] = (float)(gyro[1] * gyro_fsr_dps) / (float)data_length_max;
	gyro_dps[2] = (float)(gyro[2] * gyro_fsr_dps) / (float)data_length_max;
	if (USE_HIGH_RES_MODE || !USE_FIFO)
		temp_degc = 25 + ((float)event->temperature / 128);
	else
		temp_degc = 25 + ((float)event->temperature / 2);

	
	if(m_trans_enable_get())
	{
		if (event->sensor_mask & (1 << INV_SENSOR_ACCEL) && event->sensor_mask & (1 << INV_SENSOR_GYRO))
		{
			tspp_send(send_data_tmp,20);
			
		}
//	log_debug( "data = %u: %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f\r\n",
//		        (uint32_t)timestamp, accel_g[0], accel_g[1], accel_g[2], temp_degc, gyro_dps[0],
//		        gyro_dps[1], gyro_dps[2]);
		else if (event->sensor_mask & (1 << INV_SENSOR_GYRO))
		{
			tspp_send(send_data_tmp,20);
		}
//		log_debug(  "data = %u: NA, NA, NA, %.3f, %.3f, %.3f, %.3f\r\n", (uint32_t)timestamp,
//		        temp_degc, gyro_dps[0], gyro_dps[1], gyro_dps[2]);
		else if (event->sensor_mask & (1 << INV_SENSOR_ACCEL))
		{
			tspp_send(send_data_tmp,20);
		}
//	
//		log_debug("data = %u: %.3f, %.3f, %.3f, %.3f, NA, NA, NA\r\n", (uint32_t)timestamp,
//		        accel_g[0], accel_g[1], accel_g[2], temp_degc);
	}
	/*
	 * Output scaled data on UART link
	 */

#else

	if (event->sensor_mask & (1 << INV_SENSOR_ACCEL) && event->sensor_mask & (1 << INV_SENSOR_GYRO))
	{
//		OM_LOG_DEBUG("11 =%u: %d, %d, %d, %d, %d, %d, %d\r\n", (uint32_t)timestamp, accel[0],
//		        accel[1], accel[2], event->temperature, gyro[0], gyro[1], gyro[2]);
		if(get_ai_behavior_state())
		{
				// 组装样本
				pet_ai_sample.t_ms = timestamp / 1000;   // us to ms
				pet_ai_sample.ax = accel[0];
				pet_ai_sample.ay = accel[1];
				pet_ai_sample.az = accel[2];
				pet_ai_sample.gx = gyro[0];
				pet_ai_sample.gy = gyro[1];
				pet_ai_sample.gz = gyro[2];

				int ret = pet_ai_push_sample(ctx, &pet_ai_sample, &pet_ai_result);

				if(ret == PET_AI_HAS_RESULT) 
				{
						// 新行为识别结果产生
						log_debug("Behavior result: class_id = %d\r\n", pet_ai_result.class_id);
						set_pet_ai_result_class_id(pet_ai_result.class_id);
				} 
				else if(ret == PET_AI_ERR_TIME) 
				{
						// 时间戳异常，复位算法
						pet_ai_reset(ctx);
						
				} 
				else if(ret < 0) 
				{
						// 其他错误，可根据需要处理
						log_debug("pet_ai_push_sample error: %d\r\n", ret);
				}
				
//				if(m_trans_enable_get() && last_class_id != pet_ai_result.class_id)
//				{
//					last_class_id = pet_ai_result.class_id;
//					send_ai_status(ret, pet_ai_result.class_id);
//				}	
		}	
	}
	else if (event->sensor_mask & (1 << INV_SENSOR_GYRO))
	{
//		OM_LOG_DEBUG("22 =%u: NA, NA, NA, %d, %d, %d, %d\r\n", (uint32_t)timestamp,
//		        event->temperature, gyro[0], gyro[1], gyro[2]);
	}
	else if (event->sensor_mask & (1 << INV_SENSOR_ACCEL))
	{
//		OM_LOG_DEBUG("3333 =%u: %d, %d, %d, %d, NA, NA, NA\r\n", (uint32_t)timestamp, accel[0],
//		        accel[1], accel[2], event->temperature);

	}
	
#endif
}
int setup_imu_device(const struct inv_imu_serif *icm_serif)
{
	int     rc = 0;
	uint8_t who_am_i = 0;;

	/* Init device */
	rc = inv_imu_init(&icm_driver, icm_serif, imu_callback);
	if (rc != INV_ERROR_SUCCESS) 
	{
			log_debug("Failed to initialize IMU!\r\n");
			return rc;
	}

	/* Check WHOAMI */
	rc = inv_imu_get_who_am_i(&icm_driver, &who_am_i);
	if (rc != INV_ERROR_SUCCESS) 
	{
			log_debug("Failed to read whoami!, rc = %d\r\n", rc);
			return rc;
	}

	if (who_am_i != ICM_WHOAMI) 
	{
			log_debug("Bad WHOAMI!!!\r\n");
			log_debug("Read 0x%02x, expected 0x%02x\r\n", who_am_i, ICM_WHOAMI);
			return INV_ERROR;
	}
	else
	{
			log_debug("WHOAMI OK = 0x%02x!\r\n", who_am_i);
	}

	return rc;
}

/**
*** configure_imu_device
**/
int configure_imu_device()
{
	int rc = 0;

	//rc |= inv_imu_configure_fifo(&icm_driver, INV_IMU_FIFO_ENABLED);
		rc |= inv_imu_set_accel_fsr(&icm_driver, ACCEL_CONFIG0_FS_SEL_4g);
		rc |= inv_imu_set_gyro_fsr(&icm_driver, GYRO_CONFIG0_FS_SEL_2000dps);
	
	rc |= inv_imu_set_accel_frequency(&icm_driver, ACCEL_CONFIG0_ODR_50_HZ);
	rc |= inv_imu_set_gyro_frequency(&icm_driver, GYRO_CONFIG0_ODR_50_HZ);
	rc |= inv_imu_enable_accel_low_noise_mode(&icm_driver);
	
	rc |= inv_imu_enable_gyro_low_noise_mode(&icm_driver);
	
	inv_imu_sleep_ms(1);
	
	/* Disabling FIFO to avoid extra power consumption due to ALP config */
	//rc |= inv_imu_configure_fifo(&icm_driver, INV_IMU_FIFO_DISABLED);
//
//	rc |= inv_imu_set_accel_frequency(&icm_driver, ACCEL_CONFIG0_ODR_800_HZ);
rc |= pedometer_init(&icm_driver);
//	rc |= pedometer_init();	 //???????	
//	
//	/* Set 2x averaging, in order to minimize power consumption (16x by default) */
//	rc |= inv_imu_set_accel_lp_avg(&icm_driver, ACCEL_CONFIG1_ACCEL_FILT_AVG_2);
//	rc |= inv_imu_enable_accel_low_power_mode(&icm_driver);

//	/* Configure WOM to produce signal when at least one axis exceed 200 mg */
//	rc |= inv_imu_configure_wom(&icm_driver, wom_threshold, wom_threshold, wom_threshold,
//	                            WOM_CONFIG_WOM_INT_MODE_ORED, WOM_CONFIG_WOM_INT_DUR_1_SMPL);
//	
//	rc |= inv_imu_enable_wom(&icm_driver);
//	
//	if (rc)
//		log_debug("Error while %s\r\n", __func__);

	return rc;
}

/**
*** imu_init
**/
void imu_init(void)
{
	int    rc = 0;

	rc |= setup_mcu(&icm_serif);
	if(rc != INV_ERROR_SUCCESS) 
	{
			log_debug("setup mcu fail!, rc = %d\r\n", rc);
	}
	else
	{
			log_debug("setup mcu success!\r\n");
	}
	
	
	rc |= setup_imu_device(&icm_serif);
	if(rc != INV_ERROR_SUCCESS) 
	{
			log_debug("setup_imu_device fail!, rc = %d\r\n", rc);
	}
	else
	{
			log_debug("setup_imu_device success!\r\n");
	}		
	
	rc |= configure_imu_device();	
	if(rc != INV_ERROR_SUCCESS) 
	{
			log_debug("config imu fail!, rc = %d\r\n", rc);
	}
	else
	{
			log_debug("config imu success!\r\n");
	}	
	
	//初始化宠物运动检查AI算法
	if(pet_ai_context_size() > sizeof(ctx)) 
	{
			// 触发错误报警
			ai_behavior_state_flag = false;
	}
	else
	{
			memset(ctx, 0, sizeof(ctx));
			int ret = pet_ai_init(ctx);
			if(ret == PET_AI_OK)
			{
					ai_behavior_state_flag = true;			
			}
			else
			{
					ai_behavior_state_flag = false;		
			}
	}
	
	
	imu_init_ok = true;
}


/**
*** get_imu_data
**/
int get_imu_data(void)
{
	return inv_imu_get_data_from_fifo(&icm_driver);
}
int imu_fifo_timeout_recover(void)
{
    int rc;

    /* 清空可能残留或异常的FIFO */
    rc = inv_imu_reset_fifo(&icm_driver);
    if (rc != INV_ERROR_SUCCESS)
        return rc;

    /* 检查设备通信是否正常 */
    uint8_t who_am_i = 0;
    rc = inv_imu_get_who_am_i(&icm_driver, &who_am_i);

    if ((rc != INV_ERROR_SUCCESS) ||
        (who_am_i != ICM_WHOAMI)) {
        /* SPI通信或IMU状态异常，重新初始化 */
        imu_init();
        return INV_ERROR;
    }

    return INV_ERROR_SUCCESS;
}
int get_imu_data_safe(void)
{
    uint8_t int_status = 0;
    int rc;

    rc = inv_imu_read_reg(&icm_driver,
                          INV_INT_STATUS,
                          1,
                          &int_status);
    if (rc != INV_ERROR_SUCCESS)
        return rc;

    if (int_status & INT_STATUS_FIFO_FULL_INT_MASK) {
        /* 数据连续性已经破坏，丢弃旧数据 */
        inv_imu_reset_fifo(&icm_driver);

//        /* 同时清空动作识别算法的滑动窗口 */
//        motion_algorithm_reset();

        return INV_ERROR;
    }

    return inv_imu_get_data_from_fifo(&icm_driver);
}

/**
*** judge_int_at_Z
**/
bool judge_int_at_Z(void)
{
		bool rst = false;
		uint8_t  int_status = 0;
		/*
		 *  Read WOM interrupt status
		 */
		inv_imu_read_reg(&icm_driver, INT_STATUS2, 1, &int_status);
		if(int_status & INT_STATUS_FIFO_THS_INT_MASK) 
		{
//				log_debug("WoM interrupt at (X, Y, Z): %d, %d, %d\r\n",
//							(int_status & INT_STATUS2_WOM_X_INT_MASK) ? 1 : 0,
//							(int_status & INT_STATUS2_WOM_Y_INT_MASK) ? 1 : 0,
//							(int_status & INT_STATUS2_WOM_Z_INT_MASK) ? 1 : 0);
				rst = true;
		}

		return rst;
}


/**
*** config_wom_int1
**/
int config_wom_int1(void)
{
		int rc = -1;
		uint8_t value = 0;
	inv_imu_apex_disable_pedometer(&icm_driver);
    // 配置INT1为低电平有效 (Bank0: INT_CONFIG)
	  value =  ((0 << 3) | // INT2_POLARITY=0 (无关)
							(0 << 0) | //(1 << 0) 高电平有效  (0 << 0) 低电平有效
							(0 << 2) | // 脉冲模式
							(1 << 1)); // INT1_DRIVE_CIRCUIT=1 (推挽输出)
		rc = inv_imu_write_reg(&icm_driver, INT_CONFIG, 1, &value);  //已有, 但配置不一样

    // 映射WOM中断到INT1 (Bank0: INT_SOURCE1)
		value = 0;
		value = ((1 << 0) | // WOM_X_INT1_EN
						 (1 << 1) | // WOM_Y_INT1_EN
						 (1 << 2)); // WOM_Z_INT1_EN
		rc = inv_imu_write_reg(&icm_driver, INT_SOURCE1, 1, &value);
		return rc;
}

/**
*** imu_enter_sleep_mode
*** 配置ICM42607进入休眠状态
**/
int imu_enter_sleep_mode(void) 
{
		int rc = -1;
		uint8_t value = 0;
	log_debug("sensor enter sleep\r\n");
// 禁用WoM
	rc = inv_imu_write_reg(&icm_driver, WOM_CONFIG, 1, &value);

// 禁用WoM中断
	rc = inv_imu_write_reg(&icm_driver, INT_SOURCE1, 1, &value);	
	
// 关闭加速度计和陀螺仪
	rc = inv_imu_write_reg(&icm_driver, PWR_MGMT0, 1, &value);
	
	imu_init_ok = false;
	return rc;
}

/**
*** get_imu_state
*** 获取imu是否初始化完成
**/
bool get_imu_state(void)
{
		return imu_init_ok;
}

/**
*** set_motion_level
*** 根据前1min内的运动次数，判断运动强度等级
**/
void set_motion_level(void)
{			
		uint32_t int_num = get_motion_int_num();
		if(int_num <= MOTION_LESS_INT_NUM)
		{
				//静止
				motion_level = MOTION_LESS;
		}
		else if(int_num > MOTION_LESS_INT_NUM && int_num <= MOTION_1_INT_NUM)
		{
				//轻度
				motion_level = MOTION_S;
		}
		else
		{
				//重度
				motion_level = MOTION_L;
		}
		log_debug("set motion_level = %d, int_num = %d\r\n", motion_level, int_num);
		clear_motion_int_num();
}

/**
*** judge_motion_level_change
*** 判断运动状态是否改变
**/
bool judge_motion_level_change(void)
{
	bool rst = false;
	if(last_motion_level != motion_level)
	{
			rst = true;
	}
	last_motion_level = motion_level;
	return rst;
}

/**
*** motion_int_num_add
*** 递增中断计数
**/
void motion_int_num_add(void)
{
		motion_int_num_per_min++;
}

/**
*** get_motion_int_num
*** 获取中断计数
**/
uint32_t get_motion_int_num(void)
{
		return motion_int_num_per_min;
}

/**
*** clear_motion_int_num
*** 置零中断计数
**/
void clear_motion_int_num(void)
{
	 motion_int_num_per_min = 0;
}

/**
*** get_motion_level
*** 获取运动等级
**/
Motion_Level get_motion_level(void)
{
		return motion_level;
}

/**
*** get_last_motion_level
*** 获取上一次运动等级
**/
Motion_Level get_last_motion_level(void)
{
		return last_motion_level;
}
