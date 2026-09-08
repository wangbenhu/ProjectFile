#include "FreeRTOS.h"
#include "task.h"
#include "imutask_ex.h"
#include "cmsis_os2.h"
#include "om_log.h"
#include "om_driver.h"
#include "imu_ex.h"
#include "rtc_ex.h"

// 定义任务句柄变量（在全局或适当作用域）
osThreadId_t IMUTaskHandle = NULL;  // 存储任务句柄

enum
{
		IMU_NULL,
		IMU_WORK_INIT,
		IMU_WORKING,
		IMU_SLEEP_INIT,
		IMU_SLEEPING,
};
uint8_t device_work_mode = 1; //工作模式
uint8_t imu_work_mode = IMU_NULL;

void work_out_task_size(uint32_t stack_size) //(osThreadId_t TaskHandle)
{
		// 获取任务句柄（以当前任务为例）
		TaskHandle_t xTask = xTaskGetCurrentTaskHandle();
	
		// 获取栈高水位线（单位：字，4字节/字）
		UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(xTask);
		double per = uxHighWaterMark/stack_size*1.0;
	
		OM_LOG_INF("Task used: %.1f%% (%d/%d)\r\n", per, uxHighWaterMark, stack_size);
}


static void imutask_fun(void *argument)
{
		while(1)
		{
				if(device_work_mode) 
				{
						if(imu_work_mode == IMU_NULL ||  imu_work_mode == IMU_SLEEPING)
						{
								imu_work_mode = IMU_WORK_INIT; //切换到初始化
								//初始化IMU
								imu_init(); //只配置一次
								imu_work_mode = IMU_WORKING; //切换到循环工作
								OM_LOG_INF("imu working\r\n");	
						}
				}
				else
				{
						if(imu_work_mode != IMU_SLEEPING)
						{
								imu_work_mode = IMU_SLEEP_INIT;
								OM_LOG_INF("imu sleep\r\n");								
						}	
				}
					
#if 1			
				if(imu_work_mode == IMU_WORKING)
				{
						get_timestamp_date(&now_tm);
						set_motion_level(&motion_int_num_per_min, now_tm.tm_min);
				}
				else if(imu_work_mode == IMU_SLEEP_INIT)
				{
						//关闭IMU运动中断检测
						OM_LOG_INF("sleep_mode, close imu\r\n");
						imu_enter_sleep_mode(); //只配置一次
						imu_work_mode = IMU_SLEEPING;
				}				

#else

				////test
				extern void inv_imu_sleep_ms(uint32_t ms);
				inv_imu_sleep_ms(100);
				//get_imu_data();
				//work_out_task_size(2048*2);
				extern void test_test(void);
				test_test();
#endif
		}
}

void vStartImuTask(void)
{
    const osThreadAttr_t imuThreadAttr = {
        .name = NULL,
        .attr_bits = 0,
        .cb_mem = NULL,
        .cb_size = 0,
        .stack_mem = NULL,
        .stack_size = 1024*2,
        .priority = osPriorityNormal,
        .tz_module = 0,
    };

    // Create ble Task
    IMUTaskHandle = osThreadNew(imutask_fun, NULL, &imuThreadAttr);
    
    if (IMUTaskHandle == NULL) 
		{
        // 错误处理：任务创建失败
        OM_LOG_INF("IMUTaskHandle ERROR!!!\r\n");
    }
		else
		{
				OM_LOG_INF("IMUTaskHandle SUCCESS!!!\r\n");
		}
}


