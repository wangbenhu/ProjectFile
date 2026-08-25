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
#include "GNSS_UART_Task.h"
// Controller header
#include "obc.h"
#include "cJSON.h"

/* Kernel includes. */
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "semphr.h"         // 信号量（含互斥锁）相关函数声明
#include "event_groups.h"  // 事件组核心头文件
/*********************************************************************
 * MACROS
 */
#define EVENT_SYSTEM_RESERVE_MASK   0x00FF

#define GNSS_UART_TASK_PRIORITY (osPriorityNormal)
#define GNSS_UART_TASK_STACK_SIZE (8192)

/*********************************************************************
 * TYPEDEFS
 */
// 定义事件标志
#define GPS_EVENT_CMD_READY       (1 << 0)
#define GPS_EVENT_RESP_RECEIVED   (1 << 1)
#define GPS_EVENT_TASK_START      (1 << 2)
#define GPS_EVENT_ERROR           (1 << 3)
#define GPS_EVENT_TASK_BLOCK      (1 << 4)      // 任务阻塞标志
#define GPS_EVENT_TASK_UNBLOCK    (1 << 5)      // 任务解除阻塞标志

#define AT_CMD_QUEUE_SIZE 16
#define INDEX_ERROR		(101)
#define MAX_FIELDS 10
#define SEND_BUF_SIZE 128


#define MAX_FIELDS 10
#define COORDINATE_LEN 20  // 经纬度字符串最大长度
#define TIME_LEN 12        // 时间戳字符串最大长度

#define GPS_VALID_TIMEOUT	30*1000	//GPS有效数据超时时间

#define GPS_COMMAND_TIMEOUT (5000)

// 高优先级请求标志（entry消息）
#define GPS_REQ_NONE        0
#define GPS_REQ_SHUTDOWN    1   // M3完全关机
#define GPS_REQ_STARTUP     2   // TASK_CMD_START开机
#define GPS_REQ_ENTER_BACKUP 3  // TASK_GPS_STOP进入BACKUP

// 双频模式状态
typedef enum {
    GPS_DUAL_BAND_UNKNOWN = 0,  // 未知（未查询过）
    GPS_DUAL_BAND_DISABLED = 1,  // 单频模式
    GPS_DUAL_BAND_ENABLED = 2,  // 双频模式
} gps_dual_band_state_t;

/*********************************************************************
 * CONSTANTS
 */


/*********************************************************************
 * LOCAL VARIABLES
 */
// GPS指令项
typedef struct {
    gps_at_type_t type;
    const char *ack;
    uint16_t retry_max;
    uint8_t retry_count;
} gpsCmdItem_t;

// 数据包链表
typedef struct GpsPacketNode {
    uint8_t data[UART_GNSS_RECV_DATA_SIZE];
    uint16_t length;
    struct GpsPacketNode *next;
} GpsPacketNode;
GpsPacketNode *gps_head = NULL, *gps_tail = NULL;
static osMutexId_t GpsPacketListMutex = NULL;

typedef struct {
    uint8_t buffer[UART_GNSS_RECV_DATA_SIZE];
    volatile uint32_t head; // 写指针（下一个要写入的位置）
    volatile uint32_t tail; // 读指针（下一个要读取的位置）
} circular_buffer_t;
circular_buffer_t gps_buffer; // 全局缓冲区


//GPS 指令队列
gpsCmdItem_t gpsCmdQueue[AT_CMD_QUEUE_SIZE];
uint8_t gpsCmdQueueHead = 0;
uint8_t gpsCmdQueueTail = 0;

TickType_t LastSendTime = 0;  // 初始化为 0
CmdUartState_t u_current_state;

//extern xQueueHandle gpsDataQueue;

//GPS任务互斥锁与事件组
static osMutexId_t GpsMutex;          // 保护共享资源
static osEventFlagsId_t GpsEventId;    // 事件组


//GPS 任务开关状态
typedef enum {
    GPS_TASK_RUNNING,    // 任务正常运行
    GPS_TASK_STOPPED     // 任务已停止
} gps_task_state_t;

//GPS任务状态
static gps_task_state_t gps_task_state = GPS_TASK_RUNNING;
osMutexId_t gpsTaskStateMutex;


// 在gps_fsm_context_t结构体中添加
typedef struct {
    gps_work_mode_t work_mode;      // 当前工作模式
    gps_hw_state_t hw_state;        // 软件认为的硬件状态
    gps_fsm_state_t fsm_state;      // 状态机状态
    
    // 定时器相关
    TickType_t timer_start_time;
    gps_timer_type_t active_timer;
    uint32_t timer_duration_ms;
    
    // 5秒周期检查相关
    TickType_t last_check_time;
    
    // 高优先级请求标志
    uint8_t pending_request;         // 待处理的高优先级请求
//    gps_work_mode_t pending_mode;    // 待切换的模式（用于GPS_START）
} gps_fsm_context_t;
static gps_fsm_context_t g_gps_fsm;
static osMutexId_t g_gps_fsm_mutex;

//GNSS QID
static uint8_t device_info_gnssSn[32];        // gnss sn
osMutexId_t GpsDeviceInfoMutex;

//bool gps_dataValid_reply = false;

//接收系统模式：在常规模式下，如果超时一分钟定位数据还是无效会回复STOP指令给entry
AppControlMode_t GpsRecvMode;
static TickType_t LastGpsTimeout = 0;

//gps任务开启成功标志，处理下一次重复进入
bool gps_startEntry_success = false;
//任务阻塞信号量
static osSemaphoreId_t GpsTaskBlockSem = NULL;

//gps产测标志位
struct ProductionGpsFlags production_gps_flag = {0};
static TickType_t ProductionLastGpsTimeout = 0;
static uint8_t device_info_gnssVersion[50];
static bool production_timeout_sent = false; 

// 双频模式状态
static gps_dual_band_state_t g_gps_dual_band_state = GPS_DUAL_BAND_UNKNOWN;
// 双频设置流程标志（防止重复触发）
static bool g_dual_band_config_in_progress = false; 

void backup_exit(void);

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERN FUNCTIONS
 */


/*********************************************************************
 * LOCAL FUNCTIONS
 */
/**
 * @brief  schedule task
 *
 * @param[in] pvParameters  pv parameters
 **/

void  gps_rtos_init(void);
static GPS_STATUS_t gps_Status = {
    .gps_status = CHARGE_STATUS_VOID,
    .longitude = "",           // 原始经度
    .latitude = "",            // 原始纬度
    .timestamp = "",           // 原始时间戳
    .lat_dir = 2,              // 南北纬（N=0, S=1, 无效=2）
    .lon_dir = 2 ,              // 东西经（E=0, W=1, 无效=2）
	.need_update_gnss=pdFALSE,        // GNSS数据更新使能标志（pdTRUE=需更新）
	.satellite_count = 0,
	.used_satellites = 0,
    .position_quality = GPS_QUALITY_WEAK,  // 初始为弱信号
	.last_valid_longitude = "", 
    .last_valid_latitude = "",  
    .last_valid_lat_dir = 2,    // 上次有效纬度方向初始无效
    .last_valid_lon_dir = 2,    // 上次有效经度方向初始无效
    .has_last_valid = false     // 初始没有历史有效数据
};

void gps_fsm_init(void)
{
    memset(&g_gps_fsm, 0, sizeof(gps_fsm_context_t));
    g_gps_fsm.work_mode = GPS_MODE_NORMAL;
//	g_gps_fsm.pending_mode = GPS_MODE_NORMAL;
    g_gps_fsm.hw_state = GPS_HW_OFF;
    g_gps_fsm.fsm_state = GPS_STATE_DEFAULT;
    g_gps_fsm.last_check_time = osKernelGetTickCount();
    g_gps_fsm.pending_request = GPS_REQ_NONE;
}

/**
 * @brief 安全设置GPS任务状态
 * @param new_state 新状态
 */
void safe_set_gps_task_state(gps_task_state_t new_state)
{
    osMutexAcquire(gpsTaskStateMutex, osWaitForever);
    gps_task_state = new_state;
    osMutexRelease(gpsTaskStateMutex);
}

/**
 * @brief 安全获取GPS任务状态
 * @return gps_task_state_t 当前状态
 */
gps_task_state_t safe_get_gps_task_state(void)
{
    gps_task_state_t current_state;
    osMutexAcquire(gpsTaskStateMutex, osWaitForever);
    current_state = gps_task_state;
    osMutexRelease(gpsTaskStateMutex);
    return current_state;
}

/**
 * @brief 安全阻塞GPS任务
 * @return bool true-成功阻塞 false-阻塞失败（已在阻塞状态）
 */
bool safe_block_gps_task(void)
{
    uint32_t current_flags = osEventFlagsGet(GpsEventId);
    
    // 如果已经在阻塞状态，不重复阻塞
    if (current_flags & GPS_EVENT_TASK_BLOCK) {
        log_debug("[GNSS][STA] task already blocked, skip block\r\n");
        return false;
    }
    
    osEventFlagsClear(GpsEventId, GPS_EVENT_TASK_UNBLOCK);
    osEventFlagsSet(GpsEventId, GPS_EVENT_TASK_BLOCK);
    return true;
}

/**
 * @brief 安全解除阻塞GPS任务
 * @return bool true-成功解除 false-解除失败（未在阻塞状态）
 */
bool safe_unblock_gps_task(void)
{
    uint32_t current_flags = osEventFlagsGet(GpsEventId);
    
    // 如果不在阻塞状态，不需要解除
    if (!(current_flags & GPS_EVENT_TASK_BLOCK)) {
        log_debug("[GNSS][STA] task not blocked, skip unblock\r\n");
        return false;
    }
    
    osEventFlagsClear(GpsEventId, GPS_EVENT_TASK_BLOCK);
    osEventFlagsSet(GpsEventId, GPS_EVENT_TASK_UNBLOCK);
    return true;
}

/**
 * @brief 检查任务是否应该阻塞
 * @return bool true-应该阻塞 false-不应该阻塞
 */
bool should_gps_task_block(void)
{
    return (osEventFlagsGet(GpsEventId) & GPS_EVENT_TASK_BLOCK) != 0;
}

/**
 * @brief 等待任务解除阻塞
 * @return NULL
 */
void wait_for_gps_task_unblock(void)
{
    osEventFlagsWait(GpsEventId, GPS_EVENT_TASK_UNBLOCK, osFlagsWaitAny, osWaitForever);
    osEventFlagsClear(GpsEventId, GPS_EVENT_TASK_UNBLOCK);
}

void gps_uart_send_block(const char *cmd, uint16_t len)
{
	if(cmd == NULL || len == 0)
	{
		return;
	}

//	log_debug("GPS Send:");
//	drv_uart_write(LOG_UART, (uint8_t *)cmd, (uint32_t)len, 10);
	drv_uart_write(GPS_AT_UART, (uint8_t *)cmd, (uint32_t)len, 10);
}

//GPS指令发送函数
void func_gps_type(gps_at_type_t type)
{
	char gps_payload_str[UART_GNSS_RECV_DATA_SIZE];
	
	memset(gps_payload_str,0x0, UART_GNSS_RECV_DATA_SIZE);
	switch(type)
	{
		case GPS_BACKUP:  //进入BACKUP模式
			log_debug("[GNSS][STA] Enter backup\r\n");
			sprintf(gps_payload_str,"$PAIR650,0*25\r\n");
		gps_uart_send_block(gps_payload_str,strlen(gps_payload_str));
			break;
		case GPS_BACKUP_EXIT:  //退出BACKUP模式
			log_debug("[GNSS][STA] Exit backup\r\n");
			backup_exit();
			break;
		case GPS_FIRMWARE:	//查询固件号 $PQTMVERNO*58
			sprintf(gps_payload_str,"$PQTMVERNO*58\r\n");
		gps_uart_send_block(gps_payload_str,strlen(gps_payload_str));
			break;
		case GPS_TURN_DOUBLE:	//GPS切换双频（已废弃，改用分步指令）
//			log_debug("GPS_TURN_DOUBLE deprecated, use GPS_DUAL_BAND_* commands\n");
			break;
		case GPS_TURN_SIMPLE: // GPS切换单频（已废弃，改用分步指令）
//			log_debug("GPS_TURN_SIMPLE deprecated, use GPS_DUAL_BAND_* commands\n");
			break;
		case GPS_SCANNING:	 //设置GPS卫星搜索模式
			sprintf(gps_payload_str,"ATE0\r\n");
		gps_uart_send_block(gps_payload_str,strlen(gps_payload_str));

			break;
		case GPS_TOTAL_COLD:	 //设置GPS完全冷启动
				sprintf(gps_payload_str,"$PAIR007*3D\r\n");
		gps_uart_send_block(gps_payload_str,strlen(gps_payload_str));
				break;		
		case GPS_COLD:	 //设置GPS冷启动
				sprintf(gps_payload_str,"$PAIR006*3C\r\n");
		gps_uart_send_block(gps_payload_str,strlen(gps_payload_str));
				break;
		case GPS_WARM:	 //设置GPS温启动
				sprintf(gps_payload_str,"$PAIR005*3F\r\n");
		gps_uart_send_block(gps_payload_str,strlen(gps_payload_str));
				break;
		case GPS_HOT:	 //设置GPS热启动
				sprintf(gps_payload_str,"$PAIR004*3E\r\n");
		gps_uart_send_block(gps_payload_str,strlen(gps_payload_str));
				break;
		case GPS_SET_OUT:	 //设置GPS输出格式
				sprintf(gps_payload_str,"$PAIR062,0,3*3D\r\n");
		gps_uart_send_block(gps_payload_str,strlen(gps_payload_str));
				break;	
		case GPS_SET_PQTMUNIQID:	 //查询QID
				sprintf(gps_payload_str,"$PQTMUNIQID*16\r\n");
			gps_uart_send_block(gps_payload_str,strlen(gps_payload_str));
				break;
		// ========== 双频模式分步指令 ==========
		case GPS_DUAL_BAND_READ:  // 读取双频状态
			sprintf(gps_payload_str,"$PAIR105*3E\r\n");	//$PAIR105*3E
			gps_uart_send_block(gps_payload_str,strlen(gps_payload_str));
			break;
			
		case GPS_DUAL_BAND_LOCK:  // 锁定休眠模式
			sprintf(gps_payload_str,"$PAIR382,1*2E\r\n");
			gps_uart_send_block(gps_payload_str,strlen(gps_payload_str));
			break;
		case GPS_DUAL_BAND_OFF:  // 关闭GNSS系统
			sprintf(gps_payload_str,"$PAIR003*39\r\n");
			gps_uart_send_block(gps_payload_str,strlen(gps_payload_str));
			break;
			
		case GPS_DUAL_BAND_ENABLE:  // 启用双频
			sprintf(gps_payload_str,"$PAIR104,1*2B\r\n");
			gps_uart_send_block(gps_payload_str,strlen(gps_payload_str));
			break;
			
		case GPS_DUAL_BAND_RESTART:  // 重新开启GNSS
			sprintf(gps_payload_str,"$PAIR002*38\r\n");
			gps_uart_send_block(gps_payload_str,strlen(gps_payload_str));
			break;
			
		case GPS_DUAL_BAND_COLD:  // 冷启动确保状态一致
			sprintf(gps_payload_str,"$PAIR006*3C\r\n");
			gps_uart_send_block(gps_payload_str,strlen(gps_payload_str));
				break;
	}
}

uint8_t isGpsPacketListEmpty(void)
{
    uint8_t isEmpty = 0;
    
    osMutexAcquire(GpsPacketListMutex, portMAX_DELAY);
    isEmpty = (gps_head == NULL) ? 1 : 0;
    osMutexRelease(GpsPacketListMutex);
    
    return isEmpty;
}

//将数据保存到链路
void insertGpsPacket(unsigned char *data, int length) {
	 if (length > UART_GNSS_RECV_DATA_SIZE || !data) return;
	
	// 创建新节点
    GpsPacketNode *newNode = DEMO_BT_Malloc(sizeof(GpsPacketNode));
	
	if (!newNode) 
	{
//		size_t freeHeapAfterFail = xPortGetFreeHeapSize();
//        log_debug("Malloc FAILED! Requested: %u bytes, Free heap: %u bytes, Min ever free: %u bytes\n",
//               sizeof(PacketNode), 
//               freeHeapAfterFail,
//               xPortGetMinimumEverFreeHeapSize());
		return;
	}
	
	memset(newNode->data, 0, UART_GNSS_RECV_DATA_SIZE);  // 清空数据区
    memcpy(newNode->data, data, length);
    newNode->length = length;
    newNode->next = NULL;
	
    // 保护链表操作
    osMutexAcquire(GpsPacketListMutex, portMAX_DELAY);
	
	if (gps_tail) {
        gps_tail->next = newNode;
    } else {
        gps_head = newNode;
    }
    gps_tail = newNode;
    osMutexRelease(GpsPacketListMutex);

	// 通知主任务有新数据到达
    osEventFlagsSet(GpsEventId, GPS_EVENT_RESP_RECEIVED);
}

// 清空并释放GPS数据链表中所有节点
void clearGpsPacketList(void)
{
    osMutexAcquire(GpsPacketListMutex, portMAX_DELAY);
    
    GpsPacketNode *current = gps_head;
    GpsPacketNode *next = NULL;
    
    // 遍历链表释放所有节点
    while (current != NULL) {
        next = current->next;
        DEMO_BT_Free(current);  // 使用对应的释放函数
        current = next;
    }
    
    // 重置链表头尾指针
    gps_head = NULL;
    gps_tail = NULL;
    
    osMutexRelease(GpsPacketListMutex);
    
    // 清除事件标志，表示链表已清空
    osEventFlagsClear(GpsEventId, GPS_EVENT_RESP_RECEIVED);
}

uint8_t fetchGpsacket(uint8_t *out_data, uint16_t *out_len) {
    if (!out_data || !out_len) return 0;

    GpsPacketNode *temp = NULL;

	osMutexAcquire(GpsPacketListMutex, 20);
	if (!gps_head) {
        osMutexRelease(GpsPacketListMutex);
        return 0;
    }
	
	temp = gps_head;
    gps_head = gps_head->next;
    if (!gps_head) gps_tail = NULL;
	
    memcpy(out_data, temp->data, temp->length);
    *out_len = temp->length;
	
	osMutexRelease(GpsPacketListMutex);
	
//	log_debug("Before free - Addr: 0x%p, Free heap: %u bytes\n",
//           temp,
//           xPortGetFreeHeapSize());
	
	vPortFree(temp);

//	 log_debug("After free - Freed: %u bytes, Free heap now: %u bytes\n",
//           sizeof(GpsPacketNode),
//           xPortGetFreeHeapSize());

    return 1; // 成功读取数据包
}

//将AT指令写入到队列中
BaseType_t gps_check_cmd_rtos(gps_at_type_t type, const char *ack, uint16_t retry_max)
{
    BaseType_t result = pdFALSE;
    
//    // 临界区保护
//    taskENTER_CRITICAL();
    
	osMutexAcquire(GpsMutex, portMAX_DELAY);
	
    if ((gpsCmdQueueTail + 1) % AT_CMD_QUEUE_SIZE != gpsCmdQueueHead) {
        gpsCmdQueue[gpsCmdQueueTail] = (gpsCmdItem_t){
            .type = type,
            .ack = ack,
            .retry_max = retry_max,
            .retry_count = 0
        };
        gpsCmdQueueTail = (gpsCmdQueueTail + 1) % AT_CMD_QUEUE_SIZE;
        result = pdTRUE;
    }
    
//    taskEXIT_CRITICAL();
    
	// 通知有新命令入队
    osEventFlagsSet(GpsEventId, GPS_EVENT_CMD_READY);
	
	osMutexRelease(GpsMutex);
    return result;
}

void initGps(void)
{
	drv_gpio_write(OM_GPIO0, GPIO_MASK(PAD_GNSS_BACKUPPOWER),GPIO_LEVEL_LOW);	//备份电源
	 vTaskDelay(pdMS_TO_TICKS(50));		
	drv_gpio_write(OM_GPIO1, GPIO_MASK(PAD_GNSS_POWER-32), GPIO_LEVEL_LOW);	//GPS主电源
	 vTaskDelay(pdMS_TO_TICKS(1000));		
}

void backup_enter(void)
{
	 gps_check_cmd_rtos(GPS_BACKUP,"$PAIR001,650,0*38\r\n$PAIR650,0*25\r\n",2);
	 
//	 drv_gpio_write(OM_GPIO1, GPIO_MASK(PAD_GNSS_POWER-32), GPIO_LEVEL_HIGH);
	//备份不拉高 关机再拉高
//	 drv_gpio_write(OM_GPIO0, GPIO_MASK(PAD_GNSS_BACKUPPOWER),GPIO_LEVEL_HIGH);	//备份电源  
}
void gps_shutDown(void)
{
	drv_gpio_write(OM_GPIO1, GPIO_MASK(PAD_GNSS_POWER-32), GPIO_LEVEL_HIGH);	//GPS主电源
	drv_gpio_write(OM_GPIO0, GPIO_MASK(PAD_GNSS_BACKUPPOWER),GPIO_LEVEL_HIGH);	//备份电源
}

//退出backup模式
void backup_exit(void)
{
	drv_gpio_write(OM_GPIO1, GPIO_MASK(PAD_GNSS_POWER-32), GPIO_LEVEL_LOW);			//GPS主电源
	drv_gpio_write(OM_GPIO0, GPIO_MASK(PAD_GNSS_WAKEUP), GPIO_LEVEL_HIGH);			//唤醒脚
    vTaskDelay(pdMS_TO_TICKS(150));		
	drv_gpio_write(OM_GPIO0, GPIO_MASK(PAD_GNSS_WAKEUP), GPIO_LEVEL_LOW);			//唤醒脚
}

/**
 * @brief 检查并触发双频配置（开机时调用）
 * 此函数会：
 * 1. 先查询当前双频状态
 * 2. 在 at_pair105_response() 回调中判断是否需要设置
 * @return bool true-开始查询流程
 */
bool gps_check_and_config_dual_band(void)
{
    // 重置配置标志
    g_dual_band_config_in_progress = false;
    g_gps_dual_band_state = GPS_DUAL_BAND_UNKNOWN;
    
    // 入队查询指令
    if (gps_check_cmd_rtos(GPS_DUAL_BAND_READ, "OK", 2)) {	//$PAIR105
        return true;
    }
    
    log_debug("[GNSS][ERR] Failed to enqueue dual-band query\r\n");
    return false;
}

void soft_rst(void)
{
//    drv_gpio_write(OM_GPIO0, GPIO_MASK(PAD_GNSS_REST), GPIO_LEVEL_HIGH);	//初始化拉低，复位
  //  vTaskDelay(pdMS_TO_TICKS(150));
//    drv_gpio_write(OM_GPIO0, GPIO_MASK(PAD_GNSS_REST), GPIO_LEVEL_LOW);
}

//查询GNSS QID
void gnssDeviceInfo(void)
{
	gps_check_cmd_rtos(GPS_SET_PQTMUNIQID,"OK\r\n",1);
}

//查询版本号
void gnssVersion(void)
{
	gps_check_cmd_rtos(GPS_FIRMWARE,"OK\r\n",1);
}

/**
 * @brief 设置GPS硬件状态（发送指令，状态等ACK更新）
 */
static void gps_set_hw_state(gps_hw_state_t target_state)
{
    if (g_gps_fsm.hw_state == target_state) {
        log_debug("[GNSS][STA] GPS HW already in state: %d\r\n", target_state);
        return;
    }
    if (target_state == GPS_HW_ON) {
        log_debug("[GNSS][STA] GPS HW: OFF -> ON (exit backup)\r\n");
		initGps();
        gps_check_cmd_rtos(GPS_BACKUP_EXIT, "ok\r\n", 2);
        // 注意：hw_state在收到ACK或GPIO完成后更新
    } else {
        log_debug("[GNSS][STA] GPS HW: ON -> OFF (enter backup)\r\n");
        gps_check_cmd_rtos(GPS_BACKUP, "$PAIR001,650,0*38\r\n$PAIR650,0*25\r\n", 2);
        // hw_state在收到ACK后更新
    }
}

/**
 * @brief 启动定时器
 */
static void gps_start_timer(gps_timer_type_t timer_type, uint32_t duration_ms)
{
    g_gps_fsm.active_timer = timer_type;
    g_gps_fsm.timer_duration_ms = duration_ms;
    g_gps_fsm.timer_start_time = osKernelGetTickCount();
    log_debug("[GNSS][STA] Timer started: type=%d, duration=%d ms\r\n", timer_type, duration_ms);
}

/**
 * @brief 停止定时器
 */
static void gps_stop_timer(void)
{
    g_gps_fsm.active_timer = TIMER_NONE;
    log_debug("[GNSS][STA] Timer stopped\r\n");
}

/**
 * @brief 检查定时器是否超时
 */
static uint8_t g_gps_is_timer_timeout(void)
{
    if (g_gps_fsm.active_timer == TIMER_NONE) {
        return 0;
    }
    
    TickType_t now = osKernelGetTickCount();
    TickType_t elapsed = now - g_gps_fsm.timer_start_time;
    
    if (elapsed >= pdMS_TO_TICKS(g_gps_fsm.timer_duration_ms)) {
        return 1;
    }
    return 0;
}

/**
 * @brief  send_reply_entry_task	cat1 消息队列发送函数
 * @param data 数据
 * @param length 数据长度
 * @return BaseType_t pdTRUE-成功入队 pdFALSE-队列已满
 **/
BaseType_t gps_send_reply_entry_task(TASK_CMD_T command)
{
	TaskInfo_t *my_task_info = GetTaskInfo(GNSS_UART_TASK_ID);
	TaskInfo_t *entry_task_info = GetTaskInfo(ENTRY_TASK_ID);
	if (!entry_task_info || !entry_task_info->queue_handle)
	{
		return pdFALSE;
	}

	Message_t msg = {
		.source_id = my_task_info->task_id,
		.dest_id = ENTRY_TASK_ID,
		.command = command
	};
	return osMessageQueuePut(entry_task_info->queue_handle, &msg, NULL, 0) == osOK;
}

/**
* @brief gpsPowerDownClear		GNSS关机时状态机清空
 * @return NULL
 */
void gpsPowerDownClear(void)
{
	// 清空指令队列
	osMutexAcquire(GpsMutex, osWaitForever);
	gpsCmdQueueHead = gpsCmdQueueTail = 0;
	osMutexRelease(GpsMutex);
	
	//清空缓冲区
	clearGpsPacketList();
}

/**
 * @brief lteEventPowerDown		GPS任务关闭
 * @return NULL
 */
void gpsEventPowerDown(void)
{
	osMutexAcquire(g_gps_fsm_mutex, osWaitForever);
    g_gps_fsm.hw_state = GPS_HW_OFF;
	osMutexRelease(g_gps_fsm_mutex);
    log_debug("[GNSS][STA] HW state updated: OFF (BACKUP ACK)\r\n");
	
	drv_gpio_write(OM_GPIO1, GPIO_MASK(PAD_GNSS_POWER-32), GPIO_LEVEL_HIGH);
	
	//清空定位数据
	gps_Status.gps_status = CHARGE_STATUS_VOID;
}

void send_to_gpsUartTask(TASK_CMD_T command)
{
	TaskInfo_t *cat1_uart_task_info = GetTaskInfo(UART_GNSSDATARECV_ID);
	
	// START UART1
	Message_t send_start_msg = {
		.source_id = GNSS_UART_TASK_ID,
		.dest_id = UART_GNSSDATARECV_ID,
		.command = command,
	};
	if (osOK != osMessageQueuePut(cat1_uart_task_info->queue_handle, &send_start_msg, NULL, 0))
	{
		LOG_LOC();
	}
}

BaseType_t production_gps_send_errorcode_task(TASK_ID_T dest_task_id, TASK_CMD_T command)
{
	TaskInfo_t *my_task_info = GetTaskInfo(GNSS_UART_TASK_ID);
	TaskInfo_t *dest_task_info = GetTaskInfo(dest_task_id);
	
	if (!dest_task_info || !dest_task_info->queue_handle)
	{
		return pdFALSE;
	}
	Message_t msg = {
		.source_id = my_task_info->task_id,
		.dest_id = dest_task_id,
		.command = command
	};
	return osMessageQueuePut(dest_task_info->queue_handle, &msg, NULL, 0) == osOK;
}

/**
 * @brief GPS数据解析函数，支持GNGLL和GNRMC格式
 * @param str: 原始GPS数据
 * @param len: 数据长度
 */
void at_gps_data(uint8_t *str, uint16_t len) {
    TaskInfo_t *comm_task_info = GetTaskInfo(COMM_TASK_ID);
    
    char work_buffer[UART_GNSS_RECV_DATA_SIZE];
    char *fields[16] = {0}; // 增加字段数量以支持RMC格式
    
    // 基本检查
    if (str == NULL || len == 0 || len >= sizeof(work_buffer)) return;
    
    memcpy(work_buffer, str, len);
    work_buffer[len] = '\0';
    
    // 手动解析所有字段
    char *ptr = work_buffer;
    int field_count = 0;
    for (int i = 0; i < 16 && *ptr != '\0'; i++) {
        fields[i] = ptr;
        field_count++;
        
        // 找到下一个逗号或结束
        while (*ptr != ',' && *ptr != '*' && *ptr != '\0') {
            ptr++;
        }
        
        // 终止当前字段并移动到下一个
        if (*ptr == ',' || *ptr == '*') {
            *ptr = '\0';
            ptr++;
        }
        
        // 遇到校验位(*)或结束符时停止
        if (*ptr == '*' || *ptr == '\0') {
            break;
        }
    }
    
    // 检查支持的语句类型
    int is_gngll = (strncmp(fields[0], "$GNGLL", 6) == 0);
    int is_gpgll = (strncmp(fields[0], "$GPGLL", 6) == 0);
    int is_gnrmc = (strncmp(fields[0], "$GNRMC", 6) == 0);
    int is_gprmc = (strncmp(fields[0], "$GPRMC", 6) == 0);
    
    if (!is_gngll && !is_gpgll && !is_gnrmc && !is_gprmc) {
        return; // 不支持的格式
    }
    
    char status = 'V';
    char *latitude = NULL;
    char *longitude = NULL;
    char *timestamp = NULL;
    char *lat_dir = NULL;
    char *lon_dir = NULL;
    
    // 根据格式解析不同的字段位置
    if (is_gngll || is_gpgll) {
        // GNGLL格式: $GNGLL,lat,N,lon,E,time,status,*checksum
        if (field_count >= 7) {
            latitude = fields[1];
            lat_dir = fields[2];
            longitude = fields[3];
            lon_dir = fields[4];
            timestamp = fields[5];
            status = (fields[6] && strlen(fields[6]) > 0) ? fields[6][0] : 'V';
        }
    } else if (is_gnrmc || is_gprmc) {
        // GNRMC格式: $GNRMC,time,status,lat,N,lon,E,speed,course,date,,,*checksum
        if (field_count >= 7) {
            timestamp = fields[1];
            status = (fields[2] && strlen(fields[2]) > 0) ? fields[2][0] : 'V';
            latitude = fields[3];
            lat_dir = fields[4];
            longitude = fields[5];
            lon_dir = fields[6];
        }
    }
    
	if(production_gps_flag.gps_flag_get_version)
	{
//		log_debug("GPS: %d, %d,%d\n", status, production_timeout_sent, ProductionLastGpsTimeout);
		// 更新GPS状态
        if (status == 'A') {
            gps_Status.gps_status = CHANGE_STATUS_POSITION;
            gps_Status.position_quality = GPS_QUALITY_GOOD;  // 定位成功，质量为良好
            
            // 产测模式搜到星，立即发送消息
            if (!production_timeout_sent) {
                production_timeout_sent = true;
                // 发送成功消息给产测任务
				production_gps_send_errorcode_task(TEST_TASK_ID, TASK_PRODUCT_GPS_TEST_REPLY);
                log_debug("[GNSS][STA] Production satellite found, send success message\r\n");
            }
            
            // 重置产测超时计时器（使用已有的全局变量）
            ProductionLastGpsTimeout = osKernelGetTickCount();
        } else {
            gps_Status.gps_status = CHARGE_STATUS_VOID;
            
            // 检查是否超时（1分钟）
            uint32_t current_time = osKernelGetTickCount();
            uint32_t timeout_ticks = osMS2TicksRound(60 * 1000);
            
            // 检查是否已经超时且未发送消息
            if (!production_timeout_sent && ProductionLastGpsTimeout > 0 && 
                (current_time - ProductionLastGpsTimeout) > timeout_ticks) {
                production_timeout_sent = true;
                // 超时未搜到星，发送超时消息
				production_gps_send_errorcode_task(TEST_TASK_ID, TASK_PRODUCT_GPS_TEST_REPLY);
                log_debug("[GNSS][STA] Production timeout, send timeout message\r\n");
            }
        }
	}
	else
	{
		// 更新GPS状态
		if (status == 'A') {
			gps_Status.gps_status = CHANGE_STATUS_POSITION;
			gps_Status.position_quality = GPS_QUALITY_GOOD;  // 定位成功，质量为良好
			if (latitude) strncpy((char *)gps_Status.latitude, latitude, COORDINATE_LEN-1);
			if (longitude) strncpy((char *)gps_Status.longitude, longitude, COORDINATE_LEN-1);
			if (timestamp) strncpy((char *)gps_Status.timestamp, timestamp, TIME_LEN-1);
			if (lat_dir) gps_Status.lat_dir = (lat_dir[0] == 'N') ? 0 : 1;
			if (lon_dir) gps_Status.lon_dir = (lon_dir[0] == 'E') ? 0 : 1;
			
			// 保存为上一次有效数据
			if (latitude && longitude && strlen(latitude) > 0 && strlen(longitude) > 0) {
				strncpy((char *)gps_Status.last_valid_latitude, latitude, COORDINATE_LEN-1);
				gps_Status.last_valid_latitude[COORDINATE_LEN-1] = '\0';
				strncpy((char *)gps_Status.last_valid_longitude, longitude, COORDINATE_LEN-1);
				gps_Status.last_valid_longitude[COORDINATE_LEN-1] = '\0';
				gps_Status.last_valid_lat_dir = gps_Status.lat_dir;	// 方向与经纬度同源保存
				gps_Status.last_valid_lon_dir = gps_Status.lon_dir;
				gps_Status.has_last_valid = true;
			}
			
			log_debug("[GNSS][STA] gps_status: %d\r\n",gps_Status.gps_status);

//			//数据有效，发送回复消息给到entry
//			if(gps_dataValid_reply)
//			{
//				gps_dataValid_reply = false;
//				gps_send_reply_entry_task(TASK_GPS_MODE_REPLY);
//			}
//			//重置超时计时器
//			LastGpsTimeout = osKernelGetTickCount();
			
			// ========== 新增：搜到星后立即进入休眠 ==========
			// 只有在常规模式且处于SEARCHING状态时，才进入休眠
			if (g_gps_fsm.work_mode == GPS_MODE_NORMAL && 
				g_gps_fsm.fsm_state == GPS_STATE_SEARCHING) {
				log_debug("[GNSS][STA] GPS locked! Immediately enter sleeping\r\n");
				gps_stop_timer();  // 停止1分钟定时器
				g_gps_fsm.fsm_state = GPS_STATE_SLEEPING;
				gps_start_timer(TIMER_10MIN_SLEEP, 10 * 60 * 1000);
				gps_set_hw_state(GPS_HW_OFF);  // 进入BACKUP
			}
		} else {
			gps_Status.gps_status = CHARGE_STATUS_VOID;
			if (timestamp) strncpy((char *)gps_Status.timestamp, timestamp, TIME_LEN-1);
	//        gps_Status.latitude[0] = '0';		//20251215修改：不清空经纬度，保持上一次的值
	//        gps_Status.longitude[0] = '0';
			
	//		log_debug("GPS raw: %d,%d,%d\n", GpsRecvMode.mode, osKernelGetTickCount(), LastGpsTimeout);
			//如果当前模式是常规，并且超时1分钟了，发送STOP，清空计时器
//			if(GpsRecvMode.mode == MODE_STANDARD && (osKernelGetTickCount() - LastGpsTimeout) > osMS2TicksRound(GPS_VALID_TIMEOUT))
//			{
//				log_debug("========GPS TASK_CMD_REPLY========\n");
//				LastGpsTimeout = osKernelGetTickCount();
//				gps_send_reply_entry_task(TASK_GPS_MODE_REPLY);
//			}
//	//        log_debug("Invalid GPS: Status=%c, Time=%s", status, gps_Status.timestamp);
		}
	}
}


void at_gps_sn(uint8_t *str, uint16_t len) //, cmd_status_t *status
{
    char work_buffer[UART_GNSS_RECV_DATA_SIZE];
    char *fields[16] = {0};
    
    // 基本检查
    if (str == NULL || len == 0 || len >= sizeof(work_buffer)) return;
    
    memcpy(work_buffer, str, len);
    work_buffer[len] = '\0';
    
    // 手动解析字段
    char *ptr = work_buffer;
    int field_count = 0;
    for (int i = 0; i < 16 && *ptr != '\0'; i++) {
        fields[i] = ptr;
        field_count++;
        
        // 找到下一个逗号或结束
        while (*ptr != ',' && *ptr != '*' && *ptr != '\0') {
            ptr++;
        }
        
        // 终止当前字段并移动到下一个
        if (*ptr == ',' || *ptr == '*') {
            *ptr = '\0';
            ptr++;
        }
        
        // 遇到校验位(*)或结束符时停止
        if (*ptr == '*' || *ptr == '\0') {
            break;
        }
    }
    
    // 检查是否为PQTMUNIQID响应
    if (field_count >= 4 && strcmp(fields[0], "$PQTMUNIQID") == 0) {
        // 字段格式: $PQTMUNIQID,OK,16,SN*checksum
        char *status = fields[1];      // "OK"
        char *length_str = fields[2];  // "16"
        char *sn_value = fields[3];    // SN字符串
        
        // 验证状态和长度
        if (strcmp(status, "OK") == 0 && strlen(sn_value) == 32) {
            // 获取互斥锁并更新设备信息
            if (osMutexAcquire(GpsDeviceInfoMutex, osWaitForever) == osOK) {
                strncpy((char*)device_info_gnssSn, sn_value, sizeof(device_info_gnssSn) - 1);
                device_info_gnssSn[sizeof(device_info_gnssSn) - 1] = '\0'; // 确保终止
                osMutexRelease(GpsDeviceInfoMutex);
                
            } else {
                log_debug("[GNSS][ERR] Failed to acquire device info mutex\r\n");
            }
        } else {
            log_debug("[GNSS][ERR] Invalid SN format: status=%s, length=%s, sn=%s\r\n", 
                     status, length_str, sn_value);
        }
    } else {
        log_debug("[GNSS][ERR] Not a valid PQTMUNIQID response\r\n");
    }
}

/**
 * @brief 解析GNSS版本号信息 (NMEA格式: $PQTMVERNO)
 * @param str 原始NMEA数据字符串
 * @param len 数据长度
 */
void at_gps_version(uint8_t *str, uint16_t len)
{
    char work_buffer[UART_GNSS_RECV_DATA_SIZE];
    char *fields[16] = {0};
    
    // 基本检查
    if (str == NULL || len == 0 || len >= sizeof(work_buffer)) {
        log_debug("[GNSS][ERR] Invalid input parameters\r\n");
        return;
    }
    
    // 复制数据并确保以NULL结尾
    memcpy(work_buffer, str, len);
    work_buffer[len] = '\0';

    char *ptr = work_buffer;
    int field_count = 0;
    
    // 解析字段，最多16个字段
    for (int i = 0; i < 16 && *ptr != '\0'; i++) {
        fields[i] = ptr;
        field_count++;
        
        // 找到下一个分隔符（逗号、星号或结束符）
        while (*ptr != ',' && *ptr != '*' && *ptr != '\0') {
            ptr++;
        }
        
        // 终止当前字段
        if (*ptr == ',' || *ptr == '*') {
            *ptr = '\0';    // 替换分隔符为字符串结束符
            ptr++;          // 移动到下一个字符
        }
        
        // 遇到校验位(*)或结束符时停止解析字段
        if (*ptr == '*' || *ptr == '\0') {
            break;
        }
    }

    // 版本响应（ $PQTMVER 和 $PQTMVERNO）
    if (field_count >= 4 && 
        (strcmp(fields[0], "$PQTMVER") == 0 || strcmp(fields[0], "$PQTMVERNO") == 0)) {
        
        char *version = NULL;
        
        if (strcmp(fields[0], "$PQTMVER") == 0) {
            version = fields[3];
        } else if (strcmp(fields[0], "$PQTMVERNO") == 0) {
            version = fields[1];
        }
        
        if (version != NULL && strlen(version) > 0) {
            // 存储版本号
            strncpy((char*)device_info_gnssVersion, version, 
                   sizeof(device_info_gnssVersion) - 1);
            device_info_gnssVersion[sizeof(device_info_gnssVersion) - 1] = '\0';
            
            // 只有在产测模式才写入文件系统
            if (production_gps_flag.gps_flag_get_version) {
                lfs_system_write((char *)device_info_gnssVersion, 
                               strlen((char *)device_info_gnssVersion),
                               SYS_FIRMWARE_GNSS_VER_ID);
            } else {
            }
        } else {
            log_debug("[GNSS][ERR] Invalid version format\r\n");
        }
    } else {
        log_debug("[GNSS][ERR] Not a valid version response. Field count: %d\r\n", field_count);
        if (field_count > 0 && fields[0]) {
            log_debug("First field: %s", fields[0]);
        }
    }
}

void at_gsv_satellite(uint8_t *str, uint16_t len)
{
    char work_buffer[256];
    char *fields[32] = {0};
    
    if (str == NULL || len == 0 || len >= sizeof(work_buffer)) return;
    
    memcpy(work_buffer, str, len);
    work_buffer[len] = '\0';
    
    // 手动解析字段
    char *ptr = work_buffer;
    int field_count = 0;
    for (int i = 0; i < 32 && *ptr != '\0'; i++) {
        fields[i] = ptr;
        field_count++;
        
        while (*ptr != ',' && *ptr != '*' && *ptr != '\0') {
            ptr++;
        }
        
        if (*ptr == ',' || *ptr == '*') {
            *ptr = '\0';
            ptr++;
        }
        
        if (*ptr == '*' || *ptr == '\0') {
            break;
        }
    }
    
    // 检查是否为GSV语句
    int is_gsv = (strncmp(fields[0], "$GNGSV", 6) == 0) ||
                 (strncmp(fields[0], "$GPGSV", 6) == 0) ||
                 (strncmp(fields[0], "$GLGSV", 6) == 0) ||
                 (strncmp(fields[0], "$GAGSV", 6) == 0) ||
                 (strncmp(fields[0], "$GBGSV", 6) == 0);
    
    if (!is_gsv || field_count < 7) {
        return;
    }
    
    uint8_t total_messages = atoi(fields[1]);
    uint8_t message_num = atoi(fields[2]);
    uint8_t total_sats = atoi(fields[3]);
    
    // 解析卫星信息 - 简单累积，无需关心重置
    int sat_index = 4;
    uint8_t satellites_added = 0;
    
    while (sat_index + 3 < field_count && gps_Status.satellite_count < 20) {
        if (fields[sat_index] && strlen(fields[sat_index]) > 0) {
            uint8_t prn = atoi(fields[sat_index]);
            uint8_t snr = (fields[sat_index + 3] && strlen(fields[sat_index + 3]) > 0) ? 
                         atoi(fields[sat_index + 3]) : 0;
            
            // 只添加有PRN和SNR>0的卫星
            if (prn > 0 && snr > 0) {
                // 去重检查
                uint8_t exists = 0;
                for (int i = 0; i < gps_Status.satellite_count; i++) {
                    if (gps_Status.satellites[i].satellite_id == prn) {
                        exists = 1;
                        // 更新为较大的SNR值
                        if (snr > gps_Status.satellites[i].snr) {
                            gps_Status.satellites[i].snr = snr;
                        }
                        break;
                    }
                }
                
                if (!exists && gps_Status.satellite_count < 20) {
                    gps_Status.satellites[gps_Status.satellite_count].satellite_id = prn;
                    gps_Status.satellites[gps_Status.satellite_count].snr = snr;
                    gps_Status.satellite_count++;
                    satellites_added++;
                }
            }
        }
        sat_index += 4;
    }
}
/**
 * @brief 解析PAIR105双频状态查询响应
 * @param str 原始NMEA数据字符串
 * @param len 数据长度
 */
void at_pair105_response(uint8_t *str, uint16_t len)
{
    char work_buffer[UART_GNSS_RECV_DATA_SIZE];
    char *fields[8] = {0};
    
    if (str == NULL || len == 0 || len >= sizeof(work_buffer)) {
        log_debug("[GNSS][ERR] Invalid input for PAIR105 response\r\n");
        return;
    }
    
    memcpy(work_buffer, str, len);
    work_buffer[len] = '\0';

    char *ptr = work_buffer;
    int field_count = 0;
    
    for (int i = 0; i < 8 && *ptr != '\0'; i++) {
        fields[i] = ptr;
        field_count++;
        
        while (*ptr != ',' && *ptr != '*' && *ptr != '\0') {
            ptr++;
        }
        
        if (*ptr == ',' || *ptr == '*') {
            *ptr = '\0';
            ptr++;
        }
        
        if (*ptr == '*' || *ptr == '\0') {
            break;
        }
    }

    // PAIR105响应格式: $PAIR105,<status>*checksum
    if (field_count >= 2 && strcmp(fields[0], "$PAIR105") == 0) {
        uint8_t dual_band_status = (uint8_t)atoi(fields[1]);

        // 直接在回调里判断状态并决定后续操作
        if (dual_band_status == 1) {
            g_gps_dual_band_state = GPS_DUAL_BAND_ENABLED;
            
            // 已是双频模式，清除配置标志
            g_dual_band_config_in_progress = false;
        } else {
            g_gps_dual_band_state = GPS_DUAL_BAND_DISABLED;
            log_debug("[GNSS][STA] GNSS dual-band mode is DISABLED, need to configure\r\n");
            
            // 不是双频，在回调里直接启动完整配置序列
            // 注意：根据文档，设置双频必须在GNSS关闭状态下进行
            // 配置序列：LOCK -> OFF -> ENABLE -> RESTART
            g_dual_band_config_in_progress = true;
            
            // 步骤1: 锁定休眠模式
            gps_check_cmd_rtos(GPS_DUAL_BAND_LOCK, "$PAIR001,382,0*32\r\n", 2);
            
            // 步骤2: 关闭GNSS
            gps_check_cmd_rtos(GPS_DUAL_BAND_OFF, "$PAIR001,003,0*38\r\n", 2);
            
            // 步骤3: 启用双频
            gps_check_cmd_rtos(GPS_DUAL_BAND_ENABLE, "$PAIR001,104,0*3E\r\n", 2);
            
            // 步骤4: 重启GNSS
            gps_check_cmd_rtos(GPS_DUAL_BAND_RESTART, "$PAIR001,002,0*37\r\n", 2);
            
//            log_debug("Dual-band config sequence enqueued from callback\n");
        }
    }
}

// 更新命令表，添加RMC支持
const Recv_At_Func gps_cmd_argv[] = {
	//device sn
	{RECV_GPS_PQTMUNIQID, "$PQTMUNIQID", at_gps_sn},
	{RECV_GPS_PQTMVERNO, "$PQTMVER", at_gps_version},
	
    {RECV_GPS_GNGLL, "$GNGLL", at_gps_data},
    {RECV_GPS_GPGLL, "$GPGLL", NULL},  // 支持GPGLL
    {RECV_GPS_GNRMC, "$GNRMC", at_gps_data},
    {RECV_GPS_GPRMC, "$GPRMC", NULL},  // 支持GPRMC
    {RECV_GPS_GNVTG, "$GNVTG", NULL},
    {RECV_GPS_GNGGA, "$GNGGA", NULL},
    {RECV_GPS_GPGGA, "$GPGGA", NULL},      // 支持GPGGA
    {RECV_GPS_GNGSA, "$GNGSA", NULL},
    {RECV_GPS_GPGSV, "$GPGSV", at_gsv_satellite},
    {RECV_GPS_GLGSV, "$GLGSV", at_gsv_satellite},
    {RECV_GPS_GAGSV, "$GAGSV", at_gsv_satellite},
    {RECV_GPS_GBGSV, "$GBGSV", at_gsv_satellite},
	{RECV_GPS_PAIR105, "$PAIR105", at_pair105_response},  // 双频状态查询响应
	
    RECV_AT_END
};

uint16_t cmd_search(char *ptr, uint16_t len, const Recv_At_Func *cmd_table) {
    if (ptr == NULL || cmd_table == NULL || len == 0) {
        log_debug("[GNSS][ERR] Cmd search: Invalid parameters\r\n");
        return INDEX_ERROR;
    }
    int i = 0;
    
    // 遍历命令数组
    while (cmd_table[i].recvAtCmd != RECV_AT_END) {
        // 获取当前命令字符串的长度
        uint16_t cmd_len = strlen(cmd_table[i].str);
        
        // 比较命令前缀（允许数据包含参数）
        if (len >= cmd_len && strncmp(ptr, cmd_table[i].str, cmd_len) == 0) {
            return i;
        }
        i++;
    }
    return INDEX_ERROR;
}


/**
 * @brief 命令解析函数（支持LTE和GPS，通过枚举区分）
 * @param ptr：待解析的原始数据指针
 * @param len：原始数据长度
 * @param module_type：模块类型（MODULE_LTE 或 MODULE_GPS）
 * @return 1-解析成功并执行命令，0-解析失败
 */
uint8_t cmd_analysis(uint8_t *ptr, uint16_t len) {
    char *line_start = (char *)ptr;
    char *data_end = (char *)ptr + len;
    uint16_t index = INDEX_ERROR;
    uint8_t res = 0;
    uint8_t has_gsv = 0;  // 标记本包数据中是否有GSV语句

    if (ptr == NULL || len < 2) {
        log_debug("[GNSS][ERR] Invalid data: NULL or too short\r\n");
        return 0;
    }

    uint8_t cmd_payload[100];
    uint8_t param_payload[UART_GNSS_RECV_DATA_SIZE];

    // 使用for循环处理每一行
    for (char *current = line_start; current < data_end - 1; ) {
        // 查找行结束符
        char *line_end = strstr(current, "\r\n");
        if (line_end == NULL || line_end >= data_end) {
//            log_debug("No line ending found\n");
            break;
        }

        uint16_t line_len = line_end - current;

        // 放宽条件：只要是以$开头的合理长度行都处理
        if (line_len > 6 && line_len < sizeof(cmd_payload) && *current == '$') {
            
            memset(cmd_payload, '\0', sizeof(cmd_payload));
            memset(param_payload, '\0', sizeof(param_payload));
            
            strncpy((char *)cmd_payload, current, line_len);
            cmd_payload[line_len] = '\0';
            
            strncpy((char *)param_payload, current, line_len);
            param_payload[line_len] = '\0';

//			log_debug("gps at analys: %s\r\n",cmd_payload);
            // 检查是否为关机命令
            if (strstr((char *)cmd_payload, "$PAIR001,650,0") != NULL || 
                strstr((char *)cmd_payload, "$PAIR650,0") != NULL) {
                log_debug("[GNSS][STA] Shutdown!\r\n");
                gpsEventPowerDown();
                res = 1;
                break;
            }
                
            // 查找命令索引
            index = cmd_search((char *)cmd_payload, line_len, gps_cmd_argv);

            // 如果是第一条GSV语句，重置卫星数据
            if (index != INDEX_ERROR && 
                (gps_cmd_argv[index].recvAtCmd == RECV_GPS_GPGSV ||
                 gps_cmd_argv[index].recvAtCmd == RECV_GPS_GLGSV ||
                 gps_cmd_argv[index].recvAtCmd == RECV_GPS_GAGSV ||
                 gps_cmd_argv[index].recvAtCmd == RECV_GPS_GBGSV)) {
                
                if (!has_gsv) {
                    // 本包数据中的第一条GSV语句，重置卫星数据
                    gps_Status.satellite_count = 0;
                    memset(gps_Status.satellites, 0, sizeof(gps_Status.satellites));
                    has_gsv = 1;
//                    log_debug("Reset satellite data for new packet");
                }
            }

            // 执行回调函数
            if (index != INDEX_ERROR && gps_cmd_argv[index].exe != NULL) {
                gps_cmd_argv[index].exe(param_payload, line_len);
                res = 1;
            }
        }

        current = line_end + 2;
    }

    return res;
}

GpsResponseResult process_gps_response(gpsCmdItem_t *currentCmd, uint8_t *recv_buf, uint16_t recv_len)
{
	if(currentCmd->type == GPS_BACKUP_EXIT)
	{
		// GPIO操作成功，立即更新软件状态
		osMutexAcquire(g_gps_fsm_mutex, osWaitForever);
		g_gps_fsm.hw_state = GPS_HW_ON;
		osMutexRelease(g_gps_fsm_mutex);
		gps_check_and_config_dual_band();
		
		return GPS_RESPONSE_SUCCESS;
	}
	
    // 有当前指令时的处理
    if (strnstr((char*)recv_buf, currentCmd->ack, strlen((char*)recv_buf))) {
		if(currentCmd->type == GPS_BACKUP)
		{
			gpsEventPowerDown();
		}
        return GPS_RESPONSE_SUCCESS;
    }
    else if (cmd_analysis(recv_buf, recv_len)) {
        return GPS_RESPONSE_SUCCESS;
    }
	else
	{
		  return GPS_RESPONSE_ERROR; 
	}
}


// 已有的模式设置函数（添加模式切换回调）
static BaseType_t gps_set_mode(Mode_t new_mode) {
    if (new_mode < MODE_HOME || new_mode > MODE_PET_FIND) {
        log_debug("[GNSS][ERR] Invalid mode: %d\r\n", new_mode);
        return pdFALSE;
    }
    // 加锁保护模式修改
    if (osMutexAcquire(GpsMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        log_debug("[GNSS][ERR] Failed to take mode mutex\r\n");
        return pdFALSE;
    }
    // 模式切换时的处理
    if (u_current_state.mode != new_mode) {
        // 退出旧模式（如果是BACKUP模式则调用exit函数）
        if (u_current_state.mode == MODE_BACKUP) {
            backup_exit();  // 从备份模式退出时调用
        }
        
        // 更新模式并重置状态机
        u_current_state.mode = new_mode;
        u_current_state.gps_state = GPS_STATE_IDLE;  // 重置状态机
        LastSendTime = osKernelGetTickCount();  // 重置发送计时
        
        // 进入新模式（如果是BACKUP模式则调用enter函数）
        if (new_mode == MODE_BACKUP) {
            backup_enter();  // 进入备份模式时调用
//           log_debug("Entered BACKUP mode");
        } else if (new_mode == MODE_HOME) {
//           log_debug("Entered NORMAL mode");
        }
    }
    osMutexRelease(GpsMutex);
    return pdTRUE;
}

uint8_t* DEVICE_GetGnssVersion(void)
{
	if (strlen((char *)device_info_gnssVersion) > 0)
	{
		return device_info_gnssVersion;
	}

    return NULL;
}

uint8_t* DEVICE_GetGnssSn(void)
{
    if (osMutexAcquire(GpsDeviceInfoMutex, osWaitForever) == osOK)
    {
        if (strlen((char *)device_info_gnssSn) > 0)
        {
            osMutexRelease(GpsDeviceInfoMutex);
            return device_info_gnssSn;
        }
        osMutexRelease(GpsDeviceInfoMutex);
    }
    return NULL;
}

void GPS_GetCurrentData(GPS_STATUS_t* gps_data)
{
    // 使用临界区或互斥锁保护数据拷贝
    taskENTER_CRITICAL();
    memcpy(gps_data, &gps_Status, sizeof(GPS_STATUS_t));
    taskEXIT_CRITICAL();
}

/**
 * @brief 5秒周期状态检查（在主循环中每隔5秒调用一次）
 */
void gps_periodic_check(void)
{
    TickType_t now = osKernelGetTickCount();
    
    // 5秒周期检查
    if ((now - g_gps_fsm.last_check_time) < pdMS_TO_TICKS(5000)) {
        return;
    }
    g_gps_fsm.last_check_time = now;
    
//    log_debug("[GNSS][STA] Periodic check: state=%d, mode=%d, timer=%d\n", 
//              g_gps_fsm.fsm_state, g_gps_fsm.work_mode, g_gps_fsm.active_timer);
    
    // ========== 第一步：处理高优先级请求（entry消息） ==========
    if (g_gps_fsm.pending_request != GPS_REQ_NONE) {
        switch (g_gps_fsm.pending_request) {
            case GPS_REQ_SHUTDOWN:  // M3完全关机
                log_debug("[GNSS][STA] Executing shutdown request\r\n");
                gpsPowerDownClear();
                gps_shutDown();
                send_to_gpsUartTask(TASK_CMD_STOP);
                gps_fsm_init();
                gps_send_reply_entry_task(TASK_STOP_REPLY);
                safe_block_gps_task();
                break;
                
            case GPS_REQ_STARTUP:   // 开机
                log_debug("[GNSS][STA] Executing start request\r\n");
                safe_unblock_gps_task();
                send_to_gpsUartTask(TASK_CMD_START);
                initGps();
//                LastGpsTimeout = osKernelGetTickCount();
                // 进入SEARCHING状态
                g_gps_fsm.fsm_state = GPS_STATE_SEARCHING;
                gps_start_timer(TIMER_1MIN_SEARCH, 2*60 * 1000);
                gps_set_hw_state(GPS_HW_ON);
                break;
//            case GPS_REQ_EXIT_BACKUP:  // 退出BACKUP
//                log_debug("Executing EXIT_BACKUP request, mode=%d\n", g_gps_fsm.pending_mode);
//				safe_unblock_gps_task();
//                gps_stop_timer();  // 停止10分钟定时器
//                g_gps_fsm.work_mode = g_gps_fsm.pending_mode;
//                g_gps_fsm.fsm_state = GPS_STATE_SEARCHING;
//                gps_start_timer(TIMER_1MIN_SEARCH, 60 * 1000);
//			send_to_gpsUartTask(TASK_CMD_START);
//                gps_set_hw_state(GPS_HW_ON);
//                break;
            case GPS_REQ_ENTER_BACKUP:  // 进入BACKUP
                log_debug("[GNSS][STA] Executing enter backup request\r\n");
                if (g_gps_fsm.work_mode == GPS_MODE_NORMAL) {
                    g_gps_fsm.fsm_state = GPS_STATE_SLEEPING;
                    gps_start_timer(TIMER_10MIN_SLEEP, 10 * 60 * 1000);
                    gps_set_hw_state(GPS_HW_OFF);
					gps_send_reply_entry_task(TASK_STOP_REPLY);
                }
                break;
        }
        g_gps_fsm.pending_request = GPS_REQ_NONE;
        return;  // 高优先级请求处理完，本次周期结束
    }
    
    // ========== 第二步：处理寻宠模式自维护逻辑 ==========
    if (g_gps_fsm.work_mode == GPS_MODE_PET_FIND) {
        switch (g_gps_fsm.fsm_state) {
            case GPS_STATE_ACTIVE:
                // 确保硬件开机
                if (g_gps_fsm.hw_state != GPS_HW_ON) {
                    log_debug("[GNSS][STA] Pet find mode: HW not ON, sending exit backup\r\n");
                    gps_set_hw_state(GPS_HW_ON);
                }
                break;
            case GPS_STATE_SEARCHING:
            case GPS_STATE_SLEEPING:
            default:
                // 切换到ACTIVE状态
                log_debug("[GNSS][STA] Pet find mode: switching to active\r\n");
                g_gps_fsm.fsm_state = GPS_STATE_ACTIVE;
                gps_stop_timer();
                gps_set_hw_state(GPS_HW_ON);
                break;
        }
        return;
    }
	
	 // ========== 第三步：处理常规模式自维护逻辑 ==========
    if (g_gps_fsm.work_mode == GPS_MODE_NORMAL) {
        switch (g_gps_fsm.fsm_state) {
            case GPS_STATE_SLEEPING:
                // 防御：如果定时器丢失（如快速模式切换导致），自动补上
                if (g_gps_fsm.active_timer == TIMER_NONE) {
                    log_debug("[GNSS][STA] SLEEPING but no timer, auto-start 10min\r\n");
                    gps_start_timer(TIMER_10MIN_SLEEP, 10 * 60 * 1000);
                }
                // 检查10分钟定时器是否超时
                if (g_gps_is_timer_timeout()) {
                    log_debug("[GNSS][STA] 10min timeout, enter searching\r\n");
                    g_gps_fsm.fsm_state = GPS_STATE_SEARCHING;
                    gps_start_timer(TIMER_1MIN_SEARCH, 60 * 1000);
                    gps_set_hw_state(GPS_HW_ON);  // 在这里操作硬件
                } else {
                    // 确保硬件关机
                    if (g_gps_fsm.hw_state != GPS_HW_OFF) {
                        gps_set_hw_state(GPS_HW_OFF);
                    }
                }
                break;
                
            case GPS_STATE_SEARCHING:
                // 检查1分钟定时器是否超时
                if (g_gps_is_timer_timeout()) {
                    log_debug("[GNSS][STA] 1min timeout, enter sleeping\r\n");
                    g_gps_fsm.fsm_state = GPS_STATE_SLEEPING;
                    gps_start_timer(TIMER_10MIN_SLEEP, 10 * 60 * 1000);
                    gps_set_hw_state(GPS_HW_OFF);  // 在这里操作硬件
                } else {
                    // 确保硬件开机
                    if (g_gps_fsm.hw_state != GPS_HW_ON) {
                        gps_set_hw_state(GPS_HW_ON);
                    }
                }
                break;
                
            default:
                // 默认进入SLEEPING
                if (g_gps_fsm.fsm_state == GPS_STATE_DEFAULT) {
                    g_gps_fsm.fsm_state = GPS_STATE_SLEEPING;
                    gps_start_timer(TIMER_10MIN_SLEEP, 10 * 60 * 1000);
                    gps_set_hw_state(GPS_HW_OFF);
                }
                break;
        }
    }
}

/**
 * @brief 处理entry消息（只设置请求标志，不立即执行）
 */
static void gps_handle_entry_message(Message_t *msg)
{
    switch (msg->command) {
        case TASK_CMD_STOP:     // M3完全关机
//            log_debug("[GNSS][STA] Entry: M3 SHUTDOWN, set request flag\r\n");
            g_gps_fsm.pending_request = GPS_REQ_SHUTDOWN;
            break;
            
        case TASK_CMD_START:    // 开机
//            log_debug("[GNSS][STA] Entry: CMD_START, set request flag\r\n");
            g_gps_fsm.pending_request = GPS_REQ_STARTUP;
            // 标记需要检查双频配置
            g_dual_band_config_in_progress = false;
            break;
//        case TASK_GPS_START:    // 退出BACKUP指令（开始定位）
//        {
//            AppControlMode_t *recv_mode = (AppControlMode_t*)msg->data;
//            log_debug("Entry: GPS_START, mode=%d, set request flag\n", recv_mode->mode);
//            
//            if (recv_mode->mode == MODE_STANDARD) {
//                g_gps_fsm.pending_mode = GPS_MODE_NORMAL;
//            } else if (recv_mode->mode == MODE_PET_FIND) {
//                g_gps_fsm.pending_mode = GPS_MODE_PET_FIND;
//            }
//            g_gps_fsm.pending_request = GPS_REQ_EXIT_BACKUP;
//            gps_dataValid_reply = true;
//            break;
//        }
        case TASK_GPS_STOP:     // 进入BACKUP指令
//            log_debug("[GNSS][STA] Entry: GPS_STOP, set request flag\r\n");
            g_gps_fsm.pending_request = GPS_REQ_ENTER_BACKUP;
            break;
            
        default:
            break;
    }
}

static void gps_handle_comm_message(Message_t *msg)
{
    /* ===== 居家模式：进入居家 → GPS 完全断电，停止周期状态机 =====
     * periodic_check 中 GPS_MODE_HOME 无任何周期维护动作，GPS 保持断电 */
    if (msg->command == TASK_HOME_ENTER) {
        if (g_gps_fsm.work_mode != GPS_MODE_HOME) {
            g_gps_fsm.work_mode = GPS_MODE_HOME;
            g_gps_fsm.fsm_state = GPS_STATE_SLEEPING;
            gps_stop_timer();
            gps_set_hw_state(GPS_HW_OFF);
            log_debug("[GNSS][STA] enter HOME mode, GPS power off\r\n");
        }
        return;
    }

    AppControlMode_t *mode_data = (AppControlMode_t *)msg->data;
    
//    log_debug("[GNSS][STA] mode update, new_mode=%d, workMode: %d\r\n", mode_data->mode,g_gps_fsm.work_mode);
    
     if (mode_data->mode == MODE_STANDARD) {
        if (g_gps_fsm.work_mode != GPS_MODE_NORMAL) {
            g_gps_fsm.work_mode = GPS_MODE_NORMAL;
            // 常规模式：进入SLEEPING状态，开启10分钟定时器
            g_gps_fsm.fsm_state = GPS_STATE_SLEEPING;
            gps_stop_timer();
//            gps_start_timer(TIMER_10MIN_SLEEP, 10 * 60 * 1000);
//            gps_set_hw_state(GPS_HW_OFF);  // 进入BACKUP
        }
    } else if (mode_data->mode == MODE_SEARCH_PET) {
        if (g_gps_fsm.work_mode != GPS_MODE_PET_FIND) {
            g_gps_fsm.work_mode = GPS_MODE_PET_FIND;
            // 切换到寻宠模式，应该进入ACTIVE状态
            g_gps_fsm.fsm_state = GPS_STATE_ACTIVE;
            gps_stop_timer();
//            gps_set_hw_state(GPS_HW_ON);
        }
    }
}

void gpsCmd_parse_task(void *pvParameters)
{
    gpsCmdItem_t *currentCmd = NULL;
    uint8_t recv_buf[UART_GNSS_RECV_DATA_SIZE];
    uint16_t recv_len = 0;
    uint32_t uxBits;		//事件组标志位
	static TickType_t LastWakeTime = 0;
	
    // 初始化状态
    u_current_state.gps_state = GPS_STATE_IDLE;
    u_current_state.mode = MODE_HOME;  // 默认正常模式
    
	// 初始化状态机
    gps_fsm_init();
	
	// 获取上次检查时间
    g_gps_fsm.last_check_time = osKernelGetTickCount();
	
    // 获取任务信息
    TaskInfo_t *my_task_info = GetTaskInfo(GNSS_UART_TASK_ID);
    TaskInfo_t *gpsuart_task_info = GetTaskInfo(UART_GNSSDATARECV_ID);
    Message_t received_msg;

    log_debug("[GNSS][STA] Task %d started\r\n", my_task_info->task_id);
	
    for(;;) 
    {
        if (should_gps_task_block()) {
            log_debug("[GNSS][STA] task blocked, waiting for START command\r\n");
            
            wait_for_gps_task_unblock();
            
            // 重新设置任务状态为运行中
            safe_set_gps_task_state(GPS_TASK_RUNNING);
        }
		
        if (osOK == osMessageQueueGet(my_task_info->queue_handle, &received_msg, NULL, 100)) {
            // 处理启动命令
            if ((received_msg.source_id == ENTRY_TASK_ID)) 
			{
				gps_handle_entry_message(&received_msg);
            }
			// 处理comm消息
            if (received_msg.source_id == COMM_TASK_ID) {
				if (received_msg.command == TASK_HOME_ENTER) {
					/* 居家模式 */
					gps_handle_comm_message(&received_msg);
				} else if (received_msg.data != NULL) {
					GpsRecvMode.mode = *(uint8_t *)received_msg.data;
//					
					log_debug("[GNSS][STA] GpsRecvMode.mode = %d \r\n",GpsRecvMode.mode);
					gps_handle_comm_message(&received_msg);
				}
            }
				
			if(received_msg.source_id == UART_GNSSDATARECV_ID && received_msg.data != NULL)
			{
				insertGpsPacket(received_msg.data, received_msg.data_length);
				DEMO_BT_Free(received_msg.data);
            }
			if(received_msg.source_id == TEST_TASK_ID)
			{
				if(received_msg.command == TASK_PRODUCT_GPS_TEST)
				{
					production_gps_flag.gps_flag_get_version = 1;
					production_timeout_sent = false;
					send_to_gpsUartTask(TASK_CMD_START);
					initGps();
					
					//读版本号
					gnssVersion();
					
					ProductionLastGpsTimeout = osKernelGetTickCount();
				}
			}
        }
		
		// 5秒周期状态检查
        gps_periodic_check();
		
		uxBits = osEventFlagsGet(GpsEventId);
		// 处理命令就绪事件
        if (uxBits & GPS_EVENT_CMD_READY) {
			if(isGpsPacketListEmpty())
			{
				// 检查并执行队列中的下一条指令
				osMutexAcquire(GpsMutex, portMAX_DELAY);
				if (!currentCmd && gpsCmdQueueHead != gpsCmdQueueTail) {
					currentCmd = &gpsCmdQueue[gpsCmdQueueHead];
//					log_debug("currentGpsCmd->type: %d\n", currentCmd->type);
					func_gps_type(currentCmd->type);
					LastWakeTime = osKernelGetTickCount();

					osEventFlagsSet(GpsEventId, GPS_EVENT_RESP_RECEIVED);
				}
				else
				{
					// 清除事件位
					osEventFlagsClear(GpsEventId, GPS_EVENT_CMD_READY);
				}
				osMutexRelease(GpsMutex);
			}
		}
		
		if (uxBits & GPS_EVENT_RESP_RECEIVED) {
			memset(recv_buf,0,UART_GNSS_RECV_DATA_SIZE);
			recv_len = 0;
			
			bool has_data = fetchGpsacket(recv_buf, &recv_len);
			bool is_backup_exit = (currentCmd != NULL && currentCmd->type == GPS_BACKUP_EXIT);
			
			if (has_data || is_backup_exit) {
				GpsResponseResult result = process_gps_response(currentCmd, recv_buf, recv_len);
                // 有指令的数据处理
                if (currentCmd) {
                    switch(result) {
                        case GPS_RESPONSE_SUCCESS:
							log_debug("[GNSS][STA] AT sucesss\r\n");
								// 完成当前指令，移动到下一条
								osMutexAcquire(GpsMutex, portMAX_DELAY);
								gpsCmdQueueHead = (gpsCmdQueueHead + 1) % AT_CMD_QUEUE_SIZE;
								currentCmd = NULL;
								osMutexRelease(GpsMutex);
								LastWakeTime = osKernelGetTickCount();
							
								// 如果队列非空，触发下一条指令
								if (gpsCmdQueueHead != gpsCmdQueueTail) {
									osEventFlagsClear(GpsEventId, GPS_EVENT_RESP_RECEIVED);
									osEventFlagsSet(GpsEventId, GPS_EVENT_CMD_READY);
								}
                            break;
                        case GPS_RESPONSE_ERROR:
//							 currentCmd->status = CMD_STATUS_FAILED;
							if (currentCmd->retry_count < currentCmd->retry_max) {
                                currentCmd->retry_count++;
//                                func_gps_type(currentCmd->type); // 重发
                            } else {
                                // 重试完，移动到下一条
                                osMutexAcquire(GpsMutex, portMAX_DELAY);
                                gpsCmdQueueHead = (gpsCmdQueueHead + 1) % AT_CMD_QUEUE_SIZE;
                                currentCmd = NULL;
                                osMutexRelease(GpsMutex);
                                osEventFlagsClear(GpsEventId, GPS_EVENT_RESP_RECEIVED);
//                                osEventFlagsSet(GpsEventId, LTE_EVENT_ERROR);
                            }
                            break;
                        default:
                            break;
                    }
                }
				//无指令的上报的数据
				else {
                    switch(result) {
                        case GPS_RESPONSE_SUCCESS:
							LastWakeTime = osKernelGetTickCount(); // 重置超时计时器
							osEventFlagsClear(GpsEventId, GPS_EVENT_RESP_RECEIVED);
							osEventFlagsSet(GpsEventId, GPS_EVENT_CMD_READY);
                            break;
                        case GPS_RESPONSE_ERROR:
                            // 未知错误
							osEventFlagsClear(GpsEventId, GPS_EVENT_RESP_RECEIVED);
//                            osEventFlagsSet(GpsEventId, GPS_EVENT_ERROR);
                            break;
                        default:
                            break;
                    }
                }
			}
		}
		// 检查固定5秒超时
		if (currentCmd && (osKernelGetTickCount() - LastWakeTime) > osMS2TicksRound(GPS_COMMAND_TIMEOUT))
		{
			log_debug("[GNSS][ERR] GPS no response for 5s, rebooting!\r\n");
			
			//判断指令有效性
			if (currentCmd->type < GPS_BACKUP || currentCmd->type > GPS_SET_PQTMUNIQID) {
			osMutexAcquire(GpsMutex, osWaitForever);
			gpsCmdQueueHead = gpsCmdQueueTail = 0;
			currentCmd = NULL;
			osMutexRelease(GpsMutex);
			LastWakeTime = osKernelGetTickCount();
				
			// ========== 添加：重置状态机 ==========
			gps_fsm_init();
			continue; // 跳过本次超时处理
		}
			osMutexAcquire(GpsMutex, osWaitForever);
			
			bool need_restart = false;
			
			// 如果当前超时的指令是关机指令（进入BACKUP模式）
			if (currentCmd->type == GPS_BACKUP) {
				// 检查队列中是否还有其他指令
				if (gpsCmdQueueHead != gpsCmdQueueTail) {
					// 获取队列中的最后一条指令
					uint8_t last_index = (gpsCmdQueueTail - 1 + AT_CMD_QUEUE_SIZE) % AT_CMD_QUEUE_SIZE;
					gps_at_type_t last_cmd_type = gpsCmdQueue[last_index].type;
					
					// 如果最后一条指令是开机指令（退出BACKUP模式）
					if (last_cmd_type == GPS_BACKUP_EXIT) {
						log_debug("[GNSS][ERR] Last command is BACKUP_EXIT (power on), will restart GPS\r\n");
						need_restart = true;
					} else {
						// 最后一条指令不是开机，只是清空队列，不需要激活GPS
						log_debug("[GNSS][ERR] Last command is not BACKUP_EXIT, just clearing queue, GPS is already off\r\n");
					}
				} else {
					// 队列中没有其他指令，只有当前这条关机指令超时
				}
				
				// 清空所有指令队列
				gpsCmdQueueHead = gpsCmdQueueTail = 0;
				currentCmd = NULL;
			} else {
				// 非关机指令超时，按原逻辑处理
				gpsCmdQueueHead = gpsCmdQueueTail = 0;
				currentCmd = NULL;
				
				// 非关机指令超时时，需要恢复GPS
				need_restart = true;
			}
			
			osMutexRelease(GpsMutex);
			
			// 只有在需要重启时才激活GPS
			if (need_restart) {
				log_debug("[GNSS][ERR] Activating GPS with backup_exit()\r\n");
				backup_exit();
			} else {
				log_debug("[GNSS][ERR] GPS is off, no need to activate, just clear queue\r\n");
			}
			// ========== 添加：无论哪种超时，都重置状态机 ==========
			gps_fsm_init();
			LastWakeTime = osKernelGetTickCount(); // 重置超时计时器
		}
		osDelay(osMS2TicksRound(10));
    }
}
    
void  gps_rtos_init(void)
{
    // 创建互斥锁
    GpsMutex = osMutexNew(NULL);	//GPS互斥锁
	    if (GpsMutex == NULL) {
       LOG_LOC();  // 致命错误：系统锁定
    }
    GpsPacketListMutex = osMutexNew(NULL);		//串口数据存入链表的互斥锁
	GpsDeviceInfoMutex = osMutexNew(NULL);
	gpsTaskStateMutex = osMutexNew(NULL); 
	
    u_current_state.gps_state = GPS_STATE_IDLE;
    u_current_state.mode = MODE_HOME;

	// 创建事件组
    GpsEventId = osEventFlagsNew(NULL);
   
    // 初始化命令队列
    gpsCmdQueueHead = gpsCmdQueueTail = 0;
	
	// 初始化任务状态
    safe_set_gps_task_state(GPS_TASK_RUNNING);
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
osThreadId_t vStartGNSSUartTask(void)
{
	gps_rtos_init();
    const osThreadAttr_t GNSSUartThreadAttr = {
        .name = "GNSS_UART_Task",
        .attr_bits = 0,
        .cb_mem = NULL,
        .cb_size = 0,
        .stack_mem = NULL,
        .stack_size = GNSS_UART_TASK_STACK_SIZE,
        .priority = GNSS_UART_TASK_PRIORITY,
        .tz_module = 0,
    };

    // Create pm Task
    return osThreadNew(gpsCmd_parse_task, NULL, &GNSSUartThreadAttr);
}

/** @} */

// vim: fdm=marker
