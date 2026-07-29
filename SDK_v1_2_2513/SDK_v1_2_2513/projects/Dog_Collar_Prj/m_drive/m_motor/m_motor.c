#include "m_motor.h"
#include "cmsis_os2.h"
#include "../source/drv_gpio.h"
#include "common_def.h"

/**
 ******************************************************************************
 * @Notes
 *	待改写模式和时间传入规则
 ******************************************************************************
 **/
#define EVENT_SYSTEM_RESERVE_MASK   0x00FF

#define MOTOR_TASK_PRIORITY (osPriorityNormal)
#define MOTOR_TASK_STACK_SIZE (1024)

#define MOTOR_DEBUG_CONFIG 1
#if (MOTOR_DEBUG_CONFIG == 1)
#define MOTOR_LOG_DEBUG(format, ...)               	log_debug( format,  ## __VA_ARGS__)
#else
#define MOTOR_LOG_DEBUG(format, ...) 
#endif

static uint32_t moduleCounter = 0;  
static uint32_t modeRunMode = 0;
static uint32_t LevelInversionFlag = 0;
static uint8_t m_motor_status = 0;
osMutexId_t m_motor_mutex;	
osEventFlagsId_t motor_id;			
osTimerId_t motor_timer;

/// Test pad for gpio output
#define M_MOTOR_PAD_GPIO         0

/*******************************************************************************
 * CONST & VARIABLES
 */
/// Pinmux Configuration
static pin_config_t m_motor_pin_config[] = {
    {M_MOTOR_PAD_GPIO, {PINMUX_GPIO_MODE_CFG}, PMU_PIN_MODE_PP, PMU_PIN_DRIVER_CURRENT_NORMAL},
};

/// GPIO Configuration
static gpio_config_t m_motor_gpio_config[] = {
    {OM_GPIO0, M_MOTOR_PAD_GPIO,  GPIO_DIR_OUTPUT, GPIO_LEVEL_LOW, GPIO_TRIG_NONE},
};

static void m_motor_start(void)
{
	m_motor_status = 1;
	// output high level
    drv_gpio_write(OM_GPIO0, GPIO_MASK(M_MOTOR_PAD_GPIO), GPIO_LEVEL_HIGH);
}
static void m_motor_stop(void)
{
	m_motor_status = 0;
	// output high level
    drv_gpio_write(OM_GPIO0, GPIO_MASK(M_MOTOR_PAD_GPIO), GPIO_LEVEL_LOW);
}
void my_timer_callback(void *argument) {
	if(modeRunMode == MOTOR_INTERVAL_RUN)
    {
     //   MOTOR_LOG_DEBUG("LevelInversionFlag = %d\r\n",LevelInversionFlag);
        if(LevelInversionFlag)
        {
            LevelInversionFlag = 0;
            m_motor_stop();
        }else
        {
            LevelInversionFlag = 1; 
            m_motor_start();
        }
    }else{
        m_motor_set(MOTOR_STOP_RUN,0);
        osTimerStop(motor_timer);	
    }
	
}
void m_motor_init(void)
{
    // 创建互斥锁
    const osMutexAttr_t mutex_attr = { .name = "m_motor_mutex" };
    m_motor_mutex = osMutexNew(&mutex_attr);
	//请求互斥锁
	osMutexAcquire(m_motor_mutex, osWaitForever);
	//初始化GPIO
	// drv_pin_init(m_motor_pin_config, sizeof(m_motor_pin_config) / sizeof(m_motor_pin_config[0]));
    // drv_gpio_init(m_motor_gpio_config, sizeof(m_motor_gpio_config) / sizeof(m_motor_gpio_config[0]));
    // output high level
    // drv_gpio_write(OM_GPIO0, GPIO_MASK(M_MOTOR_PAD_GPIO), GPIO_LEVEL_LOW);
	//初始化事件组
	set_motor_id();
	//初始化定时器
	motor_timer = osTimerNew(my_timer_callback, osTimerPeriodic, NULL, NULL);
	//释放互斥锁
	osMutexRelease(m_motor_mutex);
}
//1/0 on/off
uint8_t m_motor_is_run_get(void)
{
	return m_motor_status;
}

void m_motor_set(Motor_Run_Mode type,uint32_t run_time)
{
	osMutexAcquire(m_motor_mutex, osWaitForever);
	moduleCounter=run_time;	
    modeRunMode=type;  
    LevelInversionFlag = 0;
    
	if (osEventFlagsSet(motor_id, type) == osFlagsError) {
		MOTOR_LOG_DEBUG("m_motor_set error\r\n");
	} 
	//MOTOR_LOG_DEBUG("m_motor_set = %d %d\r\n",moduleCounter,type);	
	osMutexRelease(m_motor_mutex);
}

void set_motor_id(void)
{
	motor_id = osEventFlagsNew(NULL);
	if (motor_id == NULL) 
	{
		MOTOR_LOG_DEBUG("set_motor_id failed\r\n");
    }
	else
	{
	    MOTOR_LOG_DEBUG("set_motor_id finish\r\n");
	}
}
static void vMotorScheduleTask(void *argument)
{	 
	MOTOR_LOG_DEBUG("vMotorScheduleTask INIT OK \r\n");
	while(1)
	{
		 uint32_t flags = osEventFlagsWait(motor_id, MOTOR_ALWAYS_RUN | MOTOR_INTERVAL_RUN | MOTOR_TIME_RUN | MOTOR_STOP_RUN,
                                          osFlagsWaitAny, osWaitForever);
        if (flags & MOTOR_ALWAYS_RUN) {
            osTimerStop(motor_timer);	
			m_motor_start();
        }
        if (flags & MOTOR_INTERVAL_RUN) {
             //开启前判断是否关闭
             if(osTimerIsRunning(motor_timer))
             {
                 osTimerStop(motor_timer);	
             }
             osTimerStart(motor_timer, moduleCounter * 1000);
             m_motor_start();
        }
		if (flags & MOTOR_TIME_RUN) {
            //开启前判断是否关闭
             if(osTimerIsRunning(motor_timer))
             {
                 osTimerStop(motor_timer);	
             }
    		osTimerStart(motor_timer, moduleCounter * 1000);
			m_motor_start();
        }
		if (flags & MOTOR_STOP_RUN) {
			
			m_motor_stop();
			
			osTimerStop(motor_timer);		
        }
	}
}

osThreadId_t vStartMotorTask(void)
{
    const osThreadAttr_t motorThreadAttr = {
        .name = "MotorTask",
        .attr_bits = 0,
        .cb_mem = NULL,
        .cb_size = 0,
        .stack_mem = NULL,
        .stack_size = MOTOR_TASK_STACK_SIZE,
        .priority = MOTOR_TASK_PRIORITY,
        .tz_module = 0,
    };
 // Create pm Task
    return osThreadNew(vMotorScheduleTask, NULL, &motorThreadAttr);
   /* // Create ble Task
   osThreadId_t thread_id =  osThreadNew(vMotorScheduleTask, NULL, &motorThreadAttr);
	if (thread_id == NULL) {
    // 线程创建失败处理
    MOTOR_LOG_DEBUG("osThreadId_t error =%d\r\n",thread_id);
		 MOTOR_LOG_DEBUG("heap: %d bytes\r\n", osThreadGetStackSpace(thread_id));
    MOTOR_LOG_DEBUG("stack: %d\r\n", osThreadGetCount());
    // 可以添加错误恢复逻辑，如释放资源或重启系统
} else {
    // 线程创建成功
    MOTOR_LOG_DEBUG("osThreadId_t finish: %p\r\n", thread_id);
	}*/
}
