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

/* WOM threshold to be applied to IMU, ranges from 1 to 255, in 4mg unit */
static uint8_t wom_threshold = WOM_THRESHOLD_INITIAL_MG / 4;

static struct inv_imu_serif icm_serif;
static struct inv_imu_device icm_driver;

static Motion_Level motion_level = MOTION_LESS;  //运动等级
static Motion_Level last_motion_level = MOTION_LESS;  //运动等级
static uint32_t motion_int_num_per_min = 0; //每分钟运动中断个数
static uint8_t last_min = 0; //记录时间，用以实现每分钟输出一次运动强度
static bool imu_init_ok = false;

uint32_t get_motion_int_num(void);
void clear_motion_int_num(void);

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
int setup_imu_device(const struct inv_imu_serif *icm_serif)
{
	int     rc = 0;
	uint8_t who_am_i = 0;;

	/* Init device */
	rc = inv_imu_init(&icm_driver, icm_serif, NULL);
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

	/* Disabling FIFO to avoid extra power consumption due to ALP config */
	rc |= inv_imu_configure_fifo(&icm_driver, INV_IMU_FIFO_DISABLED);

	rc |= inv_imu_set_accel_frequency(&icm_driver, ACCEL_CONFIG0_ODR_12_5_HZ);

	rc |= pedometer_init();	 //计步配置	
	
	/* Set 2x averaging, in order to minimize power consumption (16x by default) */
	rc |= inv_imu_set_accel_lp_avg(&icm_driver, ACCEL_CONFIG1_ACCEL_FILT_AVG_2);
	rc |= inv_imu_enable_accel_low_power_mode(&icm_driver);

	/* Configure WOM to produce signal when at least one axis exceed 200 mg */
	rc |= inv_imu_configure_wom(&icm_driver, wom_threshold, wom_threshold, wom_threshold,
	                            WOM_CONFIG_WOM_INT_MODE_ORED, WOM_CONFIG_WOM_INT_DUR_1_SMPL);
	
	rc |= inv_imu_enable_wom(&icm_driver);
	
	if (rc)
		log_debug("Error while %s\r\n", __func__);

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

	imu_init_ok = true;
}

void test_test(void) //llx test
{
	int rc = 0;
		rc |= setup_mcu(&icm_serif);
		if(rc != INV_ERROR_SUCCESS) 
		{
				log_debug("setup mcu fail!, rc = %d\r\n", rc);
		}
		else
		{
				log_debug("setup mcu success!\r\n");
		}
		
	uint8_t who_am_i = 0;
	rc = spi_read(0x75, &who_am_i, 1);
	if (rc != INV_ERROR_SUCCESS) 
	{
			log_debug("Failed to read whoami!, rc = %d\r\n", rc);
	}

	if (who_am_i != ICM_WHOAMI) 
	{
			log_debug("Bad WHOAMI!!!\r\n");
			log_debug("Read 0x%02x, expected 0x%02x\r\n", who_am_i, ICM_WHOAMI);
	}
	else
	{
			log_debug("WHOAMI OK = 0x%02x!\r\n", who_am_i);
	}		
}

/**
*** get_imu_data
**/
int get_imu_data(void)
{
	uint8_t  int_status;
	int      rc            = 0;

	/*
	 *  Read WOM interrupt status
	 */
	rc = inv_imu_read_reg(&icm_driver, INT_STATUS2, 1, &int_status);
	if (rc != INV_ERROR_SUCCESS)
		return rc;

	if (int_status & (INT_STATUS2_WOM_X_INT_MASK | INT_STATUS2_WOM_Y_INT_MASK | INT_STATUS2_WOM_Z_INT_MASK)) 
	{
		/*
		 * Extract the timestamp that was buffered when current packet IRQ fired. See 
		 * ext_interrupt_cb() in main.c for more details.
		 * As timestamp buffer is filled in interrupt handler, we should pop it with
		 * interrupts disabled to avoid any concurrent access.
		 */
			log_debug("WoM interrupt at (X, Y, Z): %d, %d, %d\r\n",
		        (int_status & INT_STATUS2_WOM_X_INT_MASK) ? 1 : 0,
		        (int_status & INT_STATUS2_WOM_Y_INT_MASK) ? 1 : 0,
		        (int_status & INT_STATUS2_WOM_Z_INT_MASK) ? 1 : 0);
	}

	return 0;	
	
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
		if(int_status & INT_STATUS2_WOM_Z_INT_MASK) 
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
