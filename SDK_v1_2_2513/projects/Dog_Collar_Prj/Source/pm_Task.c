/**
 * @file  examples/ble_app_simple_server/src/main.c
 * @brief  simple server
 * @date Wed, Sep  5, 2018  5:19:05 PM
 * @author liqiang
 *
 * @addtogroup APP_SIMPLE_SERVER_MAIN main.c
 * @ingroup APP_SIMPLE_SERVER
 * @details simple server
 *
 * @{
 */

/*********************************************************************
 * INCLUDES
 */
#include "om_driver.h"
#if (CONFIG_SHELL)
#include "shell.h"
#endif
#include "evt.h"
#include "pm.h"
#include "bsp.h"
#include "omble.h"
#include "om_log.h"

#include "common_def.h"

// Controller header
#include "obc.h"

/* Kernel includes. */
#include "cmsis_os2.h"

#include "timers.h"
#include "m_battery.h"
#include "led_task.h"
/*********************************************************************
 * MACROS
 */
#define EVENT_SYSTEM_RESERVE_MASK   0x00FF

#define PM_TASK_PRIORITY (osPriorityNormal)
#define PM_TASK_STACK_SIZE (2048)

//电池分压计算公式
#define BATTERY_DIVIDER_R1 			2	//2M
#define BATTERY_DIVIDER_R2 			1	//1M
#define PM_BATTERY_COLLECTION_TIME_UNIT (1000)//S
#define DEFAULT_PM_SW_TIMER_VALUE 	1


/* ===== 基准参数（10s）===== */
#define BASE_PERIOD_S        	10
#define BASE_CHG_RISE_MV     	12    // 10s 内最大上升
#define BASE_CHG_FALL_MV     	20    // 10s 内最大下降
#define DISCHG_DEADBAND_MV                6    // 放电死区
#define DISCHG_FALL_MAX_MV_HIGH              15   // 10s 内最大下降
#define DISCHG_FALL_MAX_MV_AVG               12   // 10s 内最大下降
#define DISCHG_FALL_MAX_MV_LOW               8   // 10s 内最大下降
/* ===== 配置参数 ===== */
#define FULL_DELAY_TIME_S    1200     // 20分钟1200s
/*********************************************************************
 * TYPEDEFS
 */
 #define FULL_HOLD_EXIT_MV      4080U				// 拔掉充电器后，电压跌破该值才退出100%
#define FULL_HOLD_TIME_MS      (5UL * 60UL * 1000UL)	//100% 最少驻留10分钟
#define SOC_DROP_INTERVAL_MS   (5UL * 60UL * 1000UL)	//60s才运行下降1%


/*********************************************************************
 * CONSTANTS
 */
osTimerAttr_t PM_Timer_attr = {
	.name = "PM_Timer",
};

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint32_t g_PMBatTimerMs = (DEFAULT_PM_SW_TIMER_VALUE * PM_BATTERY_COLLECTION_TIME_UNIT); //ms

static uint8_t pm_task_cmd_status = TASK_CMD_END; 

static BAT_STATUS_t g_PMBatStatus = {
.charge_status = CHARGE_STATUS_INVALID,
.BAT_Capacity = 0,
};
static uint32_t voltage_mV=0;
/* ===== 内部状态 ===== */
static uint16_t full_cnt = 0;
static uint8_t full_done = 0;
/* ===== 内部状态 ===== */
static int32_t batt_mv_filt = 0;
static uint8_t batt_init = 0;
static uint8_t initialized = 0;  // 初始化标志

static uint8_t power_on_battery_vlue = 0;  // 初始化标志

static uint8_t battery_full_flag = 0;  
static uint8_t full_latch = 0;
static uint32_t full_hold_ms = 0;
static uint32_t soc_drop_ms = 0;
static osSemaphoreId_t g_pmEntryLowPowerSemaphore = NULL;//低功耗是否进入的标志
/*********************************************************************
 * GLOBAL VARIABLES
 */
osTimerType_t PM_Timer_type = osTimerPeriodic;

osTimerId_t PM_Timer_ID = NULL;

/*********************************************************************
 * EXTERN FUNCTIONS
 */
extern uint8_t product_flag_get(void);

/*********************************************************************
 * LOCAL FUNCTIONS
 */
/**
 * @brief 充电满判定（到压 + 延迟）
 * @param batt_mv    滤波后的电池电压（mV）
 * @param charging   1=充电中
 * @return 1=已满，0=未满
 */
uint8_t Battery_FullDetect(int32_t batt_mv,
                           CHARGE_STATUS_T charging);



				   

void reset_rom_battery_percent(uint8_t vlua)
{
	if(vlua>=0&&vlua<=100)
	{
		if((uint8_t)drv_pmu_retention_reg2_get()!=vlua)
		{
			drv_pmu_retention_reg2_set((uint16_t)vlua);
		}
	}
}
/**
 * @brief ADC值转电压(mV) - 通用版本
 * @param adc_value ADC原始值
 * @return 实际电压值(mV)
 */
uint32_t adc_to_mv_generic(uint32_t adc_value)
{
	uint32_t volatage_mv_tmp = 0;
	volatage_mv_tmp = adc_value * ((BATTERY_DIVIDER_R1+BATTERY_DIVIDER_R2)/BATTERY_DIVIDER_R2);
	return volatage_mv_tmp;
}


uint8_t Battery_SocUpdate(uint32_t batt_mv,
                          CHARGE_STATUS_T charging)
{
    static uint8_t soc_last = 0;
    uint8_t soc_now;
	soc_now = VoltageToPercent(batt_mv);//电压转换百分比
	/* 满电检测成立：锁定100% 充电时候触发 */
	if (Battery_FullDetect(batt_mv, charging))
	{		
		soc_now = 100;
        soc_last = 100;  // 同时更新soc_last
		
		initialized = 1;
	//	log_debug("Battery_FullDetect:%d,%d\r\n",soc_last,soc_now);
		return soc_now;
	}
//	log_debug("Battery_SocUpdate:%d,%d %d\r\n",soc_last,soc_now,g_PMBatTimerMs);


	/* 第一次调用时，直接使用计算值 */
   	if (!initialized)
    {
//        soc_last = soc_now;
//		initialized = 1;
//        return soc_now;
		 uint8_t power_on_soc = power_on_battery_vlue;

        if (power_on_soc > 100 || power_on_soc == 0)
            power_on_soc = soc_now;

        soc_last = power_on_soc;

        /*
         * 只有上次保存的是100%，并且当前电压还在高电压区，
         * 才恢复100%驻留逻辑。
         */
        if (power_on_soc == 100 &&
            batt_mv >= FULL_HOLD_EXIT_MV &&
            charging != CHARGE_STATUS_CHARGING)
        {
            full_latch = 1;
            full_hold_ms = 0;
            soc_last = 100;
            initialized = 1;
            return 100;
        }

        /*
         * 如果上次是50%、60%、80%这种，
         * 不允许触发100%驻留。
         */
        full_latch = 0;
        soc_drop_ms = 0;
		
        initialized = 1;

        return soc_last;
    } /*
     * 满电后拔掉充电器：
     * 电压 >= 4080mV 且保持时间 < 20分钟，则继续显示100%
     */
    if (full_latch && charging != CHARGE_STATUS_CHARGING)
    {
        full_hold_ms += g_PMBatTimerMs;
			if (full_hold_ms > FULL_HOLD_TIME_MS)
        full_hold_ms = FULL_HOLD_TIME_MS;
        /*
        * 拔充后：
        * 1. 未满最小驻留时间，继续显示100%
        * 2. 电压仍处于满电高压区，也继续显示100%
        */
        if (batt_mv >= FULL_HOLD_EXIT_MV ||
            full_hold_ms < FULL_HOLD_TIME_MS)
        {
            soc_last = 100;
            soc_drop_ms = 0;

            log_debug("Battery_FullHold:%d,%d,%d\r\n",
                            soc_last, soc_now, batt_mv);
			
			log_batt_debug("Battery_FullHold:%d,%d,%d\r\n",
                            soc_last, soc_now, batt_mv);
            return 100;
        }

        /* 退出满电锁存 */
        full_latch = 0;

           /* 退出100%驻留后，先到99%，避免100直接跳97 */
        if (soc_last == 100)
        {
            soc_now = 99;
            soc_last = 99;
            soc_drop_ms = 0;
            return 99;
        }
    }
    if (charging == CHARGE_STATUS_CHARGING)
    {
        /* 充电：只允许上升 */
        if (soc_now < soc_last)
            soc_now = soc_last;

        soc_drop_ms = 0;
    }
    else
    {
        /* 放电：不允许上升 */
        if (soc_now > soc_last)
        {
            soc_now = soc_last;
            soc_drop_ms = 0;
        }
        /* 放电：60秒最多下降1% */
        else if (soc_now < soc_last)
        {
            soc_drop_ms += g_PMBatTimerMs;

            if (soc_drop_ms >= SOC_DROP_INTERVAL_MS)
            {
                soc_now = soc_last - 1;
                soc_drop_ms = 0;
            }
            else
            {
                soc_now = soc_last;
            }
        }
        else
        {
            soc_drop_ms = 0;
        }
    }
    soc_last = soc_now;
	
    return soc_now;
}					   
/**
 * @brief 电池电压防跳变（采样周期自适应）
 * @param new_mv   ADC 电压（mV）
 * @param charging 1=充电中，0=未充电
 */
int32_t BatteryVoltage_AntiJump(int32_t new_mv,
                                CHARGE_STATUS_T charging)
{
    int32_t diff;
    int32_t rise_max;
    int32_t fall_max;

    if (!batt_init)
    {
		batt_mv_filt = new_mv;
        batt_init = 1;
        return batt_mv_filt;
    }
	uint32_t g_PMBatTimerS_Tmp = g_PMBatTimerMs/PM_BATTERY_COLLECTION_TIME_UNIT;
    /* 防止非法周期 */
    if (g_PMBatTimerS_Tmp < 1) 
		g_PMBatTimerS_Tmp = 1;
    if (g_PMBatTimerS_Tmp > BATTERY_COLLECT_INTERVAL_TIME_MAX) 
		g_PMBatTimerS_Tmp = BATTERY_COLLECT_INTERVAL_TIME_MAX;

    /* ===== 按时间缩放限幅 ===== */

    diff = new_mv - batt_mv_filt;
	
    if (charging == CHARGE_STATUS_CHARGING)
    {
		rise_max = (BASE_CHG_RISE_MV * g_PMBatTimerS_Tmp) / BASE_PERIOD_S;
		fall_max = (BASE_CHG_FALL_MV * g_PMBatTimerS_Tmp) / BASE_PERIOD_S;

		if (diff > rise_max)
			diff = rise_max;

		if (diff < -fall_max)
			diff = -fall_max;
    }
    else
    {
            // 先根据电压区间确定 fall_max
        if (batt_mv_filt > 3800)
            fall_max = DISCHG_FALL_MAX_MV_HIGH;
        else if (batt_mv_filt > 3600)
            fall_max = DISCHG_FALL_MAX_MV_AVG;
        else
            fall_max = DISCHG_FALL_MAX_MV_LOW;

        // 再处理死区和限幅
        if (diff > -DISCHG_DEADBAND_MV)
            diff = diff >> 1;

        int32_t fall_lim = (fall_max * g_PMBatTimerS_Tmp) / BASE_PERIOD_S;
        if (diff < -fall_lim)
            diff = -fall_lim;
    }
	uint8_t shift;
		// 增加快速响应模式
	if (abs(diff) > 30) {
		shift = 1;   // α=1/2，超快速跟随
	} else if (abs(diff) > 10) {
		shift = 2;   // α=1/4，快速跟随
	} else {
		shift = 3;   // α=1/8，慢速滤波
	}
	batt_mv_filt += diff >> shift;
	if (batt_mv_filt < 3300)
		batt_mv_filt = 3300;
    /* ===== IIR（α=1/4，时间自适应已体现在 diff 上） ===== */
	return batt_mv_filt;
}

// 获取电池充电状态
CHARGE_STATUS_T PM_GetChargeStatus(void)
{
    return g_PMBatStatus.charge_status;
}
// 获取电池充电状态
void PM_GetChargeStatus_Update(void)
{
      g_PMBatStatus.charge_status = Get_ChargeIO_Status();//初始化获取充电状态
}
// 获取电池容量百分比
uint8_t PM_GetBatteryCapacity(void)
{
    return g_PMBatStatus.BAT_Capacity;
}
// 设置电池容量百分比
void PM_SetBatteryCapacity(void)
{
	g_PMBatStatus.BAT_Capacity = Battery_SocUpdate(voltage_mV,Get_ChargeIO_Status());
}

static uint8_t PM_BatteryCollectAndUpdate(uint32_t adc_value)
{
	uint32_t voltage_mv = 0;
	int32_t voltage_mv_filter = 0;
	CHARGE_STATUS_T charge_status = PM_GetChargeStatus();

	voltage_mv = adc_to_mv_generic(adc_value);//原始数据转换MV
	voltage_mv_filter = BatteryVoltage_AntiJump(voltage_mv,charge_status);//原始电压滤波处理
	g_PMBatStatus.BAT_Capacity = Battery_SocUpdate(voltage_mv_filter,charge_status);//电压转换百分比
	//更新电量到不掉电ram区域
	reset_rom_battery_percent(g_PMBatStatus.BAT_Capacity);

	log_batt_debug("PM_BatteryCollectAndUpdate = ADC[%d] state[%d] Percentage[%d] filter[%d]\r\n",
				   adc_value,charge_status,g_PMBatStatus.BAT_Capacity,voltage_mv_filter);
	log_debug("PM_BatteryCollectAndUpdate = ADC[%d] state[%d] Percentage[%d] filter[%d]\r\n",
			  adc_value,charge_status,g_PMBatStatus.BAT_Capacity,voltage_mv_filter);

	return g_PMBatStatus.BAT_Capacity;
}

void  battery_voltage_state_update(uint32_t adv_vlue)
{
	PM_BatteryCollectAndUpdate(adv_vlue);
}
void PM_Charge_Battery_Init(void)
{
	//获取充电状态
	PM_GetChargeStatus_Update();//初始化获取充电状态

	power_on_battery_vlue = drv_pmu_retention_reg2_get();//获取最后一次充电的百分比数据

	PM_BatteryCollectAndUpdate(r_PowerOn_BatteryLevel());
}

/**
 * @brief 充电满判定（到压 + 延迟）
 * @param batt_mv    滤波后的电池电压（mV）
 * @param charging   1=充电中
 * @return 1=已满，0=未满
 */
uint8_t Battery_FullDetect(int32_t batt_mv,
                           CHARGE_STATUS_T charging)
{
    uint16_t need_cnt;

    /* 非充电状态，全部清零 */
    if (charging == CHARGE_STATUS_NO_CHARGE)
    {
        full_cnt = 0;
        full_done = 0;
        return 0;
    }
	uint32_t g_PMBatTimerS_Tmp = g_PMBatTimerMs/PM_BATTERY_COLLECTION_TIME_UNIT;
    /* 防止非法周期 */
    if (g_PMBatTimerS_Tmp < 1) 
		g_PMBatTimerS_Tmp = 1;
    if (g_PMBatTimerS_Tmp > BATTERY_COLLECT_INTERVAL_TIME_MAX) 
		g_PMBatTimerS_Tmp = BATTERY_COLLECT_INTERVAL_TIME_MAX;

    /* 需要连续满足的次数 */
    need_cnt = FULL_DELAY_TIME_S / g_PMBatTimerS_Tmp;
    if (need_cnt < 1) need_cnt = 1;

    /* 到压判定 */
    if (batt_mv >= BATTERY_VOLTAGE_FULL)
    {
		battery_full_flag =1;
    }
	
	if(battery_full_flag==0)
	{
		full_cnt = 0;
	}
	else
	{
        if (full_cnt < need_cnt)
            full_cnt++;
	}
    /* 满判定 */
    if (full_cnt >= need_cnt)
        full_done = 1;

    return full_done;
}

BAT_STATUS_t PM_Task_Bat_Status_Tmp;
void PM_battery_timeout(void)
{
    TaskInfo_t *my_task_info = GetTaskInfo(PM_TASK_ID);
    TaskInfo_t *entry_task_info = GetTaskInfo(ENTRY_TASK_ID);

	PM_Task_Bat_Status_Tmp.BAT_Capacity = PM_GetBatteryCapacity();
	PM_Task_Bat_Status_Tmp.charge_status = PM_GetChargeStatus();
	
	Message_t update_msg = {
		.source_id = my_task_info->task_id,
		.dest_id = ENTRY_TASK_ID,
		.command = TASK_INFO_CHARGE,
		.data = &PM_Task_Bat_Status_Tmp,
		.data_length=sizeof(PM_Task_Bat_Status_Tmp),
	};
//	log_debug("osMessageQueuePut PM_TASK_ID \r\n");
					// 添加队列监控
//uint32_t queue_count = osMessageQueueGetCount(entry_task_info->queue_handle);
//uint32_t queue_capacity = osMessageQueueGetCapacity(entry_task_info->queue_handle);
//	log_aging_debug("PM_battery_timeout: %lu/%lu messages\n", queue_count, queue_capacity);
	log_batt_debug("product_flag_get = %d\r\n",product_flag_get());
	if(product_flag_get() == 0)
	{
		if(osOK != osMessageQueuePut(entry_task_info->queue_handle, &update_msg, NULL, 0))
		{
			log_batt_debug("product_flag_get error!\r\n");
	//		LOG_LOC();
		}
		else
		{
				log_batt_debug("product_flag_get ok!\r\n");
		}
	}

	if(osTimerIsRunning(PM_Timer_ID))
	{
		osTimerStop(PM_Timer_ID);
	}
	osTimerStart(PM_Timer_ID, osMS2TicksRound(g_PMBatTimerMs));

}
void PMTimerCallback(void *argument)
{
    pm_vbattery_get_int();
}

/**
 * @brief  schedule task
 *
 * @param[in] pvParameters  pv parameters
 **/
static void vPMTask(void *argument)
{
    //TaskInfo_t *my_info = (TaskInfo_t *)pvParameters;
    TaskInfo_t *my_task_info = GetTaskInfo(PM_TASK_ID);
    TaskInfo_t *entry_task_info = GetTaskInfo(ENTRY_TASK_ID);
	
    Message_t received_msg;
	if (g_pmEntryLowPowerSemaphore == NULL) {
		g_pmEntryLowPowerSemaphore = osSemaphoreNew(1, 0, NULL);
    } else {
//        log_debug("vPMTask already exists, skip creation\r\n");
    }
    
	PM_Timer_ID = osTimerNew(PMTimerCallback,PM_Timer_type,NULL,&PM_Timer_attr);
	if(!PM_Timer_ID)
		LOG_LOC();

    for(;;) 
    {
			//获取充电状态
	//	log_debug("p\r\n");
		PM_GetChargeStatus_Update();
		if(battery_updata_flag_get())
		{
			battery_updata_flag_clear();
			
			battery_voltage_state_update(get_gpadc_read_data());//更新电量
            PM_battery_timeout();//更新
		}

        osSemaphoreRelease(g_pmEntryLowPowerSemaphore);

		
        if(osOK == osMessageQueueGet(my_task_info->queue_handle,&received_msg, NULL, 100))
        { 
            log_debug("vPMTask %d received from:%d,%lu,%d,%d\r\n", 
                  my_task_info->task_id, received_msg.source_id, received_msg.command,*(uint8_t*)received_msg.data,received_msg.data_length);
            if(received_msg.source_id == ENTRY_TASK_ID)
            { 
                if(received_msg.command == TASK_CMD_START) 
                {
					if(pm_task_cmd_status == TASK_CMD_START)//重入互斥
						continue;
					pm_task_cmd_status=TASK_CMD_START;
					
					PM_Charge_Battery_Init();

                    
                    g_PMBatTimerMs = *(uint8_t*)received_msg.data*PM_BATTERY_COLLECTION_TIME_UNIT;
                    
                    Message_t reply_msg = {
                        .source_id = my_task_info->task_id,
                        .dest_id = ENTRY_TASK_ID,
                        .command = TASK_CMD_REPLY,
                        .data	= &g_PMBatStatus,
                        .data_length	=sizeof(g_PMBatStatus),
                    };
                    if(osOK != osMessageQueuePut(entry_task_info->queue_handle, &reply_msg, NULL, 0))
                    {
                        LOG_LOC();
                    }
                    //timer one shot
                    if(osTimerIsRunning(PM_Timer_ID))
                    {
                        osTimerStop(PM_Timer_ID);
                    }
                    osTimerStart(PM_Timer_ID, osMS2TicksRound(g_PMBatTimerMs));
                }
                else if(received_msg.command == TASK_TIMER_VAL_UPDATE)
                {
                    g_PMBatTimerMs = *(uint8_t*)received_msg.data*PM_BATTERY_COLLECTION_TIME_UNIT;//directly send timer value
                    //timer one shot
                    if(osTimerIsRunning(PM_Timer_ID))
                    {
                        osTimerStop(PM_Timer_ID);
                    }
                    osTimerStart(PM_Timer_ID, osMS2TicksRound(g_PMBatTimerMs));
                }
                else if(received_msg.command == TASK_CMD_STOP)
                {
					if(pm_task_cmd_status == TASK_CMD_STOP)//重入互斥
						 continue;
					pm_task_cmd_status=TASK_CMD_STOP;
                    //timer one shot
                    if(osTimerIsRunning(PM_Timer_ID))
                    {
                        osTimerStop(PM_Timer_ID);
                    }
                    osSemaphoreAcquire(g_pmEntryLowPowerSemaphore, osWaitForever);
                }
                else
                {

                }
            }
        }
        // 尝试获取信号量，低功耗阻塞
        osSemaphoreAcquire(g_pmEntryLowPowerSemaphore, osWaitForever);
    }
}

/*********************************************************************
 * CONST VARIABLES
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/**
 * @brief  v start bluetooth task
 **/
osThreadId_t vStartPMTask(void)
{
    const osThreadAttr_t PMThreadAttr = {
        .name = "PM_Task",
        .attr_bits = 0,
        .cb_mem = NULL,
        .cb_size = 0,
        .stack_mem = NULL,
        .stack_size = PM_TASK_STACK_SIZE,
        .priority = PM_TASK_PRIORITY,
        .tz_module = 0,
    };

    // Create pm Task
    return osThreadNew(vPMTask, NULL, &PMThreadAttr);
}

/** @} */

// vim: fdm=marker
