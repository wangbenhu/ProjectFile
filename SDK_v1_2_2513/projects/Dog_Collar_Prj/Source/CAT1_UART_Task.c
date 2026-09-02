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
#include "CAT1_UART_Task.h"

// Controller header
#include "obc.h"

/* Kernel includes. */
#include "cmsis_os2.h"
#include "semphr.h"
// #include "event_groups.h"
#include "timers.h"
#include "cJSON.h"
#include "lfs_port.h"
#include "nvds.h"
#include "Comm_Task.h"
/*********************************************************************
 * MACROS
 */
#define EVENT_SYSTEM_RESERVE_MASK 0x00FF

#define CAT1_UART_TASK_PRIORITY (osPriorityNormal)
#define CAT1_UART_TASK_STACK_SIZE (8192)

#define CAT1_POWER_VCC_TIME (2000)

// 定义事件标志
#define LTE_EVENT_CMD_READY (1 << 0)
#define LTE_EVENT_RESP_RECEIVED (1 << 1)
#define LTE_EVENT_CMD_COMPLETE (1 << 2)
#define LTE_EVENT_TASK_START (1 << 3)
#define LTE_EVENT_ERROR (1 << 4)
#define LTE_EVENT_TASK_BLOCK (1 << 5)      // 任务阻塞标志
#define LTE_EVENT_TASK_UNBLOCK (1 << 6)    // 任务解除阻塞标志

#define CAT1_SSL_CONFIG_UPDATE	1
#define INDEX_ERROR (101)
#define CAT1_PORT_1S_DELAY (1000)
#define CAT1_COMMAND_TIMEOUT (60000)
#define LTE_INIT_RDY_TIMEOUT (10000)  // lteInit() 后等待 RDY 的超时时间 (10秒)

#define CAT1_LOG_DEBUG(format, ...)               	log_debug(format,  ## __VA_ARGS__)
/// log array
#define CAT1_LOG_ARRAY(array, len)            do{int __i; for(__i=0;__i<(len);++__i)CAT1_LOG_DEBUG("%02X ",((uint8_t *)(array))[__i]);}while(0)

/*********************************************************************
 * TYPEDEFS
 */
osEventFlagsId_t LteEventId;
static char response_mqtt_data[MQTT_DATA_MTU_MAX];
static int response_mqtt_data_len = 0;
/* QMTPUBEX">"后模组还在等待的payload字节数：>0 表示模组处于数据接收态，
 * 此期间任何AT指令(检活AT/CFUN/重连指令)都会被当作payload字节吞掉 */
static uint16_t g_pubdata_pending_len = 0;

// MQTT 连接状态
static bool isMqttConnected = false;

// MQTT连接状态 getter（供COMM任务检查）
bool CAT1_IsMqttConnected(void)
{
    return isMqttConnected;
}
static bool is_reconnecting = false;

static bool qmtconn_info_seen = false;
static bool g_need_subscribe_on_reconnect = false;  // M5热恢复时需补订阅，定时器检查不需
static osMutexId_t ReconnectMutex = NULL;

/* ===== WiFi scan 累积缓冲区===== */
static DeviceWifiSsid_t wifi_scan_accum = {0};
static bool             wifi_scan_accum_active = false;
/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
typedef enum
{
	CMD_STATUS_PENDING, // 指令待处理
	CMD_STATUS_SUCCESS, // 指令成功
	CMD_STATUS_FAILED	// 指令失败
} cmd_status_t;

typedef struct
{
	lte_at_type_t type;
	const char *ack;
	uint16_t retry_max;
	uint16_t retry_count;
	uint32_t waitTime_ms;
	uint8_t has_extra_response;
	cmd_status_t status; // 指令状态
	uint8_t is_firmware_upgrade; // 新增：固件升级命令
} lteCmdItem_t;
static lteCmdItem_t lteCmdQueue[AT_CMD_QUEUE_SIZE];
lteCmdItem_t *currentCmd = NULL;
osMutexId_t LteMutex;

static uint8_t lteCmdQueueHead = 0;
static uint8_t lteCmdQueueTail = 0;

// 链表数据
typedef struct PacketNode
{
	uint8_t data[MQTT_DATA_MTU_MAX];
	uint16_t length;
	struct PacketNode *next;
} PacketNode;

PacketNode *head = NULL, *tail = NULL;
osMutexId_t PacketListMutex;

// 指令解析
typedef void (*pFunc)(uint8_t *str, uint16_t len, cmd_status_t *status);
typedef struct
{
	RECV_AT_CMD recvAtCmd;
	char *str;
	pFunc exe;
} Recv_At_Func;

// 信号质量数据
static CAT1_STATUS_t cat1_Status = {0};
// CAT1 模块序列号
static uint8_t cat1Sn[32];
osMutexId_t LteDeviceInfoMutex;


#define CAT1_POWER_CHECK_TIMEOUT 1000  // 1秒超时
static osMutexId_t lteTaskStateMutex;
// 添加硬件关机检测信号量
osSemaphoreId_t Cat1PowerCheckSem = NULL;
//任务阻塞信号量
static osSemaphoreId_t Cat1TaskBlockSem = NULL;
// CAT1任务开关状态
typedef enum
{
	LTE_TASK_INIT,
	LTE_TASK_RUNNING, // 任务正常运行
	LTE_TASK_STOPPED  // 任务已停止
} lte_task_state_t;

// CAT1任务状态切换
static lte_task_state_t lte_task_state = LTE_TASK_INIT;
// 标记是否正在检测电源状态
bool is_power_checking = false; 

// 存储当前上报主题
static char device_sn[32] = {0};
static char report_topic[128] = {0};

//重启AT指令计数器
static uint8_t lte_reboot_count = 0;
//最大重启次数
#define MAX_REBOOT_RETRY 5

//HTTP实时音频
static CAT1_HttpData_t http_data = {0};
extern uint8_t Audio_Play_Request(const char *filename, uint8_t audio_reset);

//差分固件升级
static CAT1_Updata_t firmware_url = {0};
static uint8_t firmware_upgrade_state = 0; // 0:未开始, 1:下载中, 2:升级中, 3:完成, 4:失败

//证书写入
static CertWriteState_t cert_write_state = CERT_STATE_IDLE;
static CACERT_Type_t current_cert_type = CACERT_CA;
static uint16_t expected_cert_len = 0;

// 存储基站信息
static CellInfo_t cell_info = {0};
static osMutexId_t CellInfoMutex = NULL;

//产测功能标志
struct ProductionFlags production_flag = {0};
extern message_data_t b_message_data;

//cat1版本号
static uint8_t cat1Version[100];  // 存储版本号字符串

//是否删除证书（恢复出厂设置）
static bool certificate_deletion_mode = false;

//首次进入M5时标志，首次进入上报模式状态，后续CAT1自重启不上报（备注：软重启了，CAT1一直保持开，后续CAT1重启也不会上报）
static bool first_enter_M5_mode = false;

// lteInit 开机恢复过程中标记：在此期间收到的 POWERED DOWN 不应触发 lteEventPowerDown()
// 防止模块实际在线但 AT 检活异常导致误关机后，二次 lteInit 补开机被阻塞
static volatile bool is_lte_init_recovery = false;
static uint8_t mqtt_client_idx = 0;

//寻宠/常规模式
uint32_t cat1CheckMqttStateTimeout = 30*60*1000;
MODE_M5_t Cat1RecvMode = MODE_STANDARD;
osTimerId_t checkMqttState_startTimer_ID;
osTimerType_t checkMqttState_startTimer_type = osTimerPeriodic;
osTimerAttr_t checkMqttState_startTimer_attr = {
	.name = "checkMqttState_Timer",
};

// 删除设备回包完成后的延时定时器
static osTimerId_t deleteResponseTimer_ID = NULL;
static osTimerType_t deleteResponseTimer_type = osTimerOnce;
static osTimerAttr_t deleteResponseTimer_attr = {
    .name = "DeleteResponse_Timer",
};

// lteInit() 等待 RDY 状态
static bool lte_init_pending = false;       // 是否正在等待 RDY
static uint32_t lte_init_wait_start = 0;    // 开始等待 RDY 的时间戳

// checkCat1PowerState() 返回 OFF 后的 lteInit 硬件重试控制
#define LTE_HW_RETRY_MAX       10                    // 最大硬件重试次数
#define LTE_HW_RETRY_INTERVAL  (10 * 60 * 1000UL)   // 重试间隔 10 分钟 (ms)
static uint8_t  lte_hw_retry_count = 0;             // 硬件重试已执行次数
static uint32_t lte_hw_retry_last_ts = 0;           // 上次硬件重试时间戳

/**
 * @brief  统一清除所有 LTE 恢复相关的计数器、标志和时间戳
 *         在模块恢复正常（RDY / 正常通信 / 任务重启）时调用
 */
static void lte_recovery_clear(void)
{
	lte_reboot_count = 0;
	lte_init_pending = false;
	lte_init_wait_start = 0;
	lte_hw_retry_count = 0;
	lte_hw_retry_last_ts = 0;
}

//1分钟定时查询设备状态
uint32_t cat1CheckDeviceStateTimeout = 3 * 60 * 1000;
osTimerId_t checkDeviceState_startTimer_ID;
osTimerType_t checkDeviceState_startTimer_type = osTimerPeriodic;
osTimerAttr_t checkDeviceState_startTimer_attr = {
	.name = "checkDeviceState_Timer",
};

typedef struct{
    mqtt_packet_t packets[MQTT_QUEUE_SIZE];
    int head;
    int tail;
    int count;
    osMutexId_t mutex;
}mqtt_queue_t;

static mqtt_queue_t *g_mqtt_queue = NULL;
static uint8_t g_mqtt_sending = 0;  // 0: 空闲, 1: 发送中, 2: 发送主动上报
// pending环形缓冲: 支持4条COMM回包排队, 防止压力场景下互相覆盖
#define PENDING_BUF_SIZE 4
static unsigned char *pending_buf_data[PENDING_BUF_SIZE] = {NULL};
static unsigned int  pending_buf_len[PENDING_BUF_SIZE] = {0};
static uint8_t pending_buf_head = 0;  // 出队位置
static uint8_t pending_buf_tail = 0;  // 入队位置
static uint8_t pending_buf_count = 0;
static uint8_t g_current_send_type = 0; // 0:无,1:云指令,2:主动上报
static uint32_t mqtt_send_start_tick = 0;  // g_mqtt_sending=1 的时刻, 用于超时看门狗
static bool g_skip_pubmsgd = false;          // ★ PUB的>+OK同帧时跳过下一条PUBMESSAGEDATA

bool flag_check_device_status = false;
static bool is_retransmitting = false;	//MQTT是否重发，重发队列不取数据

// 功能到主题后缀的映射
typedef struct {
    mqtt_function_t func_type;
    const char* cmd_suffix;
    const char* report_suffix;
} mqtt_func_mapping_t;
/* 	*索引  
	*服务发布主题:       固定前缀（pet/collar/{SN}/）+ cmd/    +  "query/deviceState"   
	*设备发布消息的主题: 固定前缀（pet/collar/{SN}/） + report/ + "query/deviceState"
*/
const mqtt_func_mapping_t func_mappings[] = {
    {FUNC_QUERY_DEVICE_STATE, 	"query/deviceState", 		"query/deviceState"},		//查询设备状态
	{FUNC_AUTO_DEVICE_STATE, 	"query/deviceState", 		"query/deviceState"},		//主动上报设备状态
    {FUNC_CONTROL_DEVICE_MODE, 	"control/deviceMode", 		"control/deviceMode"},		//控制设备模式
    {FUNC_QUERY_ENODEB,			"query/eNodeBInformation", 	"query/eNodeBInformation"},	//查询基站信息
    {FUNC_CONTROL_AUDIOCMD, 	"control/audioCmdSend", 	"control/audioCmdSend"},	//发送语音指令
    {FUNC_CONFIG_TIMESYNC, 		"config/timeSync", 			"config/timeSync"},			//时间同步
    {FUNC_CONFIG_WIFIMAC, 		"config/wifiMacConfig", 	"config/wifiMacConfig"},	//wifi设置
	{FUNC_CONFIG_DEVICEDELTE, 	"control/deviceDelete", 	"control/deviceDelete"},		//设备删除
	{FUNC_REPORT_WARNINGSTATE, 	"report/warningState", 		"report/warningState"},		//异常上报
	{FUNC_CONTROL_VIBRATE, 		"control/deviceVibrate", 	"control/deviceVibrate"},	//发起震动
	{FUNC_CONTROL_LIGHT, 		"control/deviceLight", 	    "control/deviceLight"},		//控制灯光
	{FUNC_CONFIG_FENCE, 		"config/deviceFence", 		"config/deviceFence"},		//设置/查询围栏
	{FUNC_QUERY_HEALTH, 		"query/healthInfo", 		"query/healthInfo"},		//定频运动状态
	{FUNC_QUERY_VOICE, 			"query/voiceID", 			"query/voiceID"},			//V1.6查询语音包
	{FUNC_REPORT_INSTANT_STATE, "query/deviceState",		"query/deviceState"},		//V1.6即时上报
	
	// pet/collar/{SN}/report/report/productTest
	{FUNC_PRODUCT_TEST, 		"report/productTest", 		"report/productTest"},		//产测测试主题
};
#define FUNC_MAPPING_COUNT (sizeof(func_mappings) / sizeof(func_mappings[0]))

// 设备状态缓存结构体
typedef struct {
    // CSQ信号质量
    struct {
        uint8_t value;          // CSQ值 (0-31有效, 99表示无信号)
        uint32_t update_time;   // 更新时间戳（0表示从未更新）
    } csq;
    
    // 基站信息
    struct {
        CellInfo_t info;        // 基站信息
        uint32_t update_time;   // 更新时间戳（0表示从未更新）
    } cell_info;
} DeviceStateCache_t;
// 定义全局缓存
DeviceStateCache_t g_device_cache = {0};
static osMutexId_t g_cache_mutex = NULL;

int build_report_topic(mqtt_function_t func_type, const char* sn, char* topic_buf, size_t buf_size);
int build_auto_topic(mqtt_function_t func_type, char* topic_buf, size_t buf_size);
BaseType_t send_data_to_comm_task(TASK_ID_T dest_task_id, TASK_CMD_T command, uint8_t *data, uint16_t length);
extern uint8_t Message_Cmd_Put(TASK_ID_T source_id,
                                       TASK_ID_T dest_id,
                                       TASK_CMD_T command,
                                       void *data,
                                       uint16_t data_length);
									   
extern void Production_Result_Report(uint8_t error_code);
									   
extern bool safe_unblock_uart_task(void);
extern int product_build_subscribe_topic(char* topic_buf, size_t buf_size);
extern bool checkCat1PowerState(void);
extern uint8_t get_product_errorcode(void);
extern void lteExitsleepDtrLow(void);
extern void lte_wifi_scan_reset(void);
extern void lte_wifi_scan_flush(void);
									   
/******************CAT1主动上报*****************/
// 主动上报相关变量
static osTimerId_t autoReportTimer_ID = NULL;
static osTimerType_t autoReportTimer_type = osTimerPeriodic;
static osTimerAttr_t autoReportTimer_attr = {
    .name = "AutoReport_Timer",
};

// 主动上报模式
typedef enum {
    REPORT_MODE_NONE = 0,      // 无上报
    REPORT_MODE_STANDARD = 1,   // 常规模式：10分钟上报一次
    REPORT_MODE_SEARCH = 2      // 寻宠模式：3秒上报一次
} REPORT_MODE_t;

static REPORT_MODE_t current_report_mode = REPORT_MODE_STANDARD;
static bool is_auto_reporting = false;  				// 是否正在主动上报中
static uint32_t report_interval_ms = 10 * 60 * 1000;    // 默认10分钟

/* ---------- 开机高频上报阶段 ----------
 * 收到 TASK_CMD_START 后前 2 分钟以 15s 为周期高频上报，
 * 2 分钟后自动降为 10 分钟低频。
 */
#define BOOT_HIGH_FREQ_INTERVAL_MS   (15  * 1000UL)   // 高频：15 秒
#define BOOT_HIGH_FREQ_DURATION_MS   (2   * 60 * 1000UL) // 高频持续：2 分钟
//#define BOOT_LOW_FREQ_INTERVAL_MS    (10  * 60 * 1000UL) // 低频：10 分钟

/* 将以上 ms 转换为 tick（供时间比较，运行时计算以避免编译期 / 频率未知问题） */
#define MS_TO_TICKS(ms)  ((uint32_t)((uint64_t)(ms) * osKernelGetTickFreq() / 1000UL))

static bool     boot_high_freq_active = false;   // 是否处于开机高频阶段
static uint32_t boot_start_tick       = 0;       // 高频阶段起始 tick

static void mqtt_send_done_handler(uint8_t is_cloud_cmd);
//入队计时
static uint64_t queue_entry_tick = 0;  // 0 = 队列空闲，非0 = 正在等待处理

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERN FUNCTIONS
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void set_certificate_deletion_mode(bool mode) {
    certificate_deletion_mode = mode;
}

bool get_certificate_deletion_mode(void) {
    return certificate_deletion_mode;
}

void set_cat1_state(lte_task_state_t new_state)
{
	osMutexAcquire(lteTaskStateMutex, osWaitForever);
	lte_task_state = new_state;
	osMutexRelease(lteTaskStateMutex);
}

lte_task_state_t get_cat1_state(void)
{
	return lte_task_state;
}

/**
 * @brief 阻塞CAT1任务
 * @return NULL
 */
void block_cat1_task(void)
{
    osEventFlagsClear(LteEventId, LTE_EVENT_TASK_UNBLOCK);
    osEventFlagsSet(LteEventId, LTE_EVENT_TASK_BLOCK);
}

/**
 * @brief 解除阻塞CAT1任务
 * @return NULL
 */
void unblock_cat1_task(void)
{
    osEventFlagsClear(LteEventId, LTE_EVENT_TASK_BLOCK);
    osEventFlagsSet(LteEventId, LTE_EVENT_TASK_UNBLOCK);
}

/**
 * @brief 解除阻塞并重启 CAT1 任务与 UART 接收任务
 * @param source_id 发起重启的源任务 ID (如 ENTRY_TASK_ID)
 * @return uint8_t 0-成功，1-消息发送失败
 */
uint8_t cat1_task_restart(TASK_ID_T source_id)
{
    safe_unblock_uart_task();
    unblock_cat1_task();
    return Message_Cmd_Put(source_id, CAT1_UART_TASK_ID, TASK_CMD_START, NULL, 0);
}

/**
 * @brief 等待任务解除阻塞
 * @return NULL
 */
void wait_for_task_unblock(void)
{
    // 等待解除阻塞标志
    osEventFlagsWait(LteEventId, LTE_EVENT_TASK_UNBLOCK, osFlagsWaitAny, osWaitForever);
    osEventFlagsClear(LteEventId, LTE_EVENT_TASK_UNBLOCK);
}

/**
 * @brief 检查任务是否应该阻塞
 * @return bool true-应该阻塞 false-不应该阻塞
 */
bool should_task_block(void)
{
    bool should_block = (osEventFlagsGet(LteEventId) & LTE_EVENT_TASK_BLOCK) != 0;
    if (should_block) {
    }
    return should_block;
}

/**
 * @brief 判断任务是否已经阻塞
 * @return bool true-已阻塞 false-未阻塞
 */
bool is_task_blocked(void)
{
    // 如果BLOCK标志被设置且UNBLOCK标志未被设置，则认为任务已阻塞
    uint32_t flags = osEventFlagsGet(LteEventId);
    bool blocked = ((flags & LTE_EVENT_TASK_BLOCK) != 0) && 
                   ((flags & LTE_EVENT_TASK_UNBLOCK) == 0);
    
    if (blocked) {
        log_debug("[CAT1][STA] task is blocked\r\n");
    }
    return blocked;
}

void cat1_task_start(void)
{
	if(is_task_blocked())
	{
		unblock_cat1_task();
		safe_unblock_uart_task();
	}
	
	 Message_Cmd_Put(ENTRY_TASK_ID,CAT1_UART_TASK_ID,TASK_CMD_START,NULL,0);
}

void cat1_task_stop(void)
{
	if(!is_task_blocked())
	{
		Message_Cmd_Put(ENTRY_TASK_ID,CAT1_UART_TASK_ID,TASK_CMD_STOP,NULL,0);
	}
	
	 
}

// 初始化设备状态缓存
void init_device_cache(void)
{
    if (g_cache_mutex == NULL) {
        g_cache_mutex = osMutexNew(NULL);
        if (g_cache_mutex == NULL) {
            log_debug("[CAT1][ERR] Failed to create mutex\r\n");
            return;
        }
    }
    
    osMutexAcquire(g_cache_mutex, osWaitForever);
    memset(&g_device_cache, 0, sizeof(DeviceStateCache_t));
    osMutexRelease(g_cache_mutex);
}

mqtt_queue_t* mqtt_queue_create(void)
{
    mqtt_queue_t *queue = DEMO_BT_Malloc(sizeof(mqtt_queue_t));
    if (!queue) {
        log_debug("[CAT1][ERR] Failed to create MQTT queue\r\n");
        return NULL;
    }
    
    memset(queue, 0, sizeof(mqtt_queue_t));
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    
    queue->mutex = osMutexNew(NULL);
    if (!queue->mutex) {
        DEMO_BT_Free(queue);
        return NULL;
    }
    
    return queue;
}

// 初始化队列
static void init_mqtt_queue(void)
{
    if (g_mqtt_queue == NULL) {
        g_mqtt_queue = mqtt_queue_create();
        g_mqtt_sending = 0;
    }
}

// 删除队列中第 index 个元素（0 表示队首）
int mqtt_queue_remove_at(mqtt_queue_t *queue, int index) {
    if (!queue || index < 0 || index >= queue->count) {
        log_debug("[CAT1][ERR] mqtt queue invalid index\r\n");
        return -1;
    }

    // 计算实际物理位置
    int remove_phys = (queue->head + index) % MQTT_QUEUE_SIZE;
    mqtt_packet_t *p = &queue->packets[remove_phys];
    
    // 释放数据
    if (p->data) {
        DEMO_BT_Free(p->data);
        p->data = NULL;
    }
    
    // 将后面的所有元素向前移动一位
    for (int i = index; i < queue->count - 1; i++) {
        int curr = (queue->head + i) % MQTT_QUEUE_SIZE;
        int next = (queue->head + i + 1) % MQTT_QUEUE_SIZE;
        // 拷贝整个结构体
        queue->packets[curr] = queue->packets[next];
    }
    
    // 清空最后一个位置
    int last_phys = (queue->head + queue->count - 1) % MQTT_QUEUE_SIZE;
    memset(&queue->packets[last_phys], 0, sizeof(mqtt_packet_t));
    
    // 调整 tail
    queue->tail = (queue->tail - 1 + MQTT_QUEUE_SIZE) % MQTT_QUEUE_SIZE;
    queue->count--;

    return 0;
}

/**
 * @brief isPacketListEmpty - 检测链表是否为空
 * @return uint8_t - 1:链表为空 0:链表非空
 *
 * @note 此函数是线程安全的，会获取互斥锁后检查链表状态
 */
uint8_t isPacketListEmpty(void)
{
	uint8_t isEmpty = 0;

	osMutexAcquire(PacketListMutex, osWaitForever);
	isEmpty = (head == NULL) ? 1 : 0;
	osMutexRelease(PacketListMutex);

	return isEmpty;
}

/**
 * @brief fetch packet	从链表头部提取数据包
 * @param out_data 		输出数据指针（需外部保证内存足够）
 * @param out_len 		输出数据长度
 * @return uint8_t 		1-成功 0-失败
 */
uint8_t fetch_packet(uint8_t *out_data, uint16_t *out_len)
{
	if (!out_data || !out_len)
		return 0;
	// 取出链表数据
	PacketNode *temp = NULL;
	osMutexAcquire(PacketListMutex, osWaitForever);

	if (!head)
	{
		osMutexRelease(PacketListMutex);
		return 0;
	}

	temp = head;
	head = head->next;
	if (!head)
		tail = NULL;

	memcpy(out_data, temp->data, temp->length);
	*out_len = temp->length;

	osMutexRelease(PacketListMutex);

	//	// 打印释放前的信息
//   log_debug(
//           "Before free - Addr: 0x%p, Free heap: %u bytes\n",
//           temp,
//           xPortGetFreeHeapSize());

// 在临界区外拷贝数据和释放内存

	DEMO_BT_Free(temp);
// 打印释放后的信息
//    log_debug(
//           "After free - Freed: %u bytes, Free heap now: %u bytes\n",
//           sizeof(PacketNode),
//           xPortGetFreeHeapSize());
	return 1;
}

void printPacketList(void)
{
    int count = 0;
    
    // 保护链表操作
    osMutexAcquire(PacketListMutex, osWaitForever);
    
    PacketNode *current = head;
    while (current)
    {
        count++;
        // 打印数据内容（以十六进制格式）
        for (int i = 0; i < current->length; i++)
        {
            log_debug("%02X\r\n", current->data[i]);
            if ((i + 1) % 16 == 0 && i + 1 < current->length)
            {
            }
        }
        
        // 如果需要打印ASCII格式的数据
        if (current->length > 0 && current->data[current->length-1] == '\0')
        {
            log_debug("  Text: %s\r\n", current->data);
        }
        
        current = current->next;
    }
    
    if (count == 0)
    {
        log_debug("Packet list is empty\r\n");
    }
    else
    {
        log_debug("Total packets: %d\r\n", count);
    }
    
    osMutexRelease(PacketListMutex);
}

/**
 * @brief insertPacket	lte串口解析的每包数据入队
 * @param data  		解析的数据
 * @param length 		解析的数据长度
 * @return NULL
 */
void insertPacket(unsigned char *data, int length)
{
	if (length > MQTT_DATA_MTU_MAX || !data)
		return;

//	// 打印当前堆内存状态
    size_t freeHeapBefore = xPortGetFreeHeapSize();
//    log_debug("Before malloc - Free heap: %u bytes\n", freeHeapBefore);

	// 创建新节点
	PacketNode *newNode = DEMO_BT_Malloc(sizeof(PacketNode));

	if (!newNode)
	{
		log_debug("[CAT1][ERR] PacketNode fail\r\n");
		return;
	}

	memset(newNode->data, 0, MQTT_DATA_MTU_MAX); // 清空数据区
	memcpy(newNode->data, data, length);
	newNode->length = length;
	newNode->next = NULL;

	// 保护链表操作
	osMutexAcquire(PacketListMutex, osWaitForever);

	if (tail)
	{
		tail->next = newNode;
	}
	else
	{
		head = newNode;
	}
	tail = newNode;
	osMutexRelease(PacketListMutex);

//	printPacketList();
	
	// 通知主任务有新数据到达
	osEventFlagsSet(LteEventId, LTE_EVENT_RESP_RECEIVED);
}

/**
 * @brief clearPacketList - 清空并释放链表中所有数据
 * @return uint8_t - 1:成功 0:失败
 *
 * @note
 */
void clearPacketList(void)
{
	PacketNode *current = NULL;
	PacketNode *next = NULL;
	uint16_t freedCount = 0;

	osMutexAcquire(PacketListMutex, osWaitForever);

	// 遍历链表并释放所有节点
	current = head;
	while (current != NULL)
	{
		next = current->next;

		DEMO_BT_Free(current);
		freedCount++;
		current = next;
	}

	// 重置链表头尾指针
	head = NULL;
	tail = NULL;

	osMutexRelease(PacketListMutex);

	// 清除事件标志，表示链表已清空
    osEventFlagsClear(LteEventId, LTE_EVENT_RESP_RECEIVED);
}

 char *get_device_sn(void)
{
	static char device_sub_sn[40] = "89430103223249467298"; // 示例SN PETTEST1
	 const char *sn_from_driver = system_info_get(SYS_DEVICE_SN_ID);
    if (sn_from_driver && sn_from_driver[0] != '\0') {
        memcpy(device_sub_sn, sn_from_driver, strlen(sn_from_driver));
        device_sub_sn[strlen(sn_from_driver)] = '\0';
    }
	return device_sub_sn;
 }

/* ===== eSIM ICCID 前缀 → APN 匹配表 =====
 * ICCID 前6位为厂商/运营商固定字段, 生产时经AT+QCCID写入文件系统(SYS_DEVICE_SN_ID),
 * 配置APN前根据卡号前缀决定写入哪个APN, 支持后续新增厂商只改本表 */
typedef struct {
    const char *iccid_prefix;   /* ICCID 前6位前缀 */
    const char *apn;            /* 对应 APN */
} ApnMatchEntry_t;

static const ApnMatchEntry_t s_apn_match_table[] = {
    { "894301", "linksnet" },   /* 领科 */
    { "893204", "bicsapn" },    /* 广和通 */
    { "898604", "cmiot" },      /* 中国移动 */
    { NULL, NULL }              /* 表尾 */
};

/* 从eSIM SN中提取前6位数字作为匹配前缀 */
static void get_iccid_prefix(const char *sn, char *prefix_out, uint8_t prefix_len)
{
    uint8_t idx = 0;
    for (const char *p = sn; *p != '\0' && idx < prefix_len; p++) {
        if (*p >= '0' && *p <= '9') {
            prefix_out[idx++] = *p;
        }
    }
    prefix_out[idx] = '\0';
}

/* 根据eSIM ICCID前缀匹配APN; 无匹配返回默认APN */
static const char *get_apn_by_esim_sn(void)
{
    char prefix[7] = {0};
    get_iccid_prefix(get_device_sn(), prefix, sizeof(prefix) - 1);

    if (strlen(prefix) == 6) {
        for (int i = 0; s_apn_match_table[i].iccid_prefix != NULL; i++) {
            if (strncmp(prefix, s_apn_match_table[i].iccid_prefix, 6) == 0) {
                return s_apn_match_table[i].apn;
            }
        }
    }
    return "linksnet";  /* 默认APN */
}

/* 查询到的APN是否在匹配表内(用于AT+CGDCONT?回读校验) */
static bool is_apn_valid(const char *apn)
{
    if (apn == NULL || apn[0] == '\0') {
        return false;
    }
    for (int i = 0; s_apn_match_table[i].iccid_prefix != NULL; i++) {
        if (strcmp(apn, s_apn_match_table[i].apn) == 0) {
            return true;
        }
    }
    return false;
}

 /**
 * @brief 检查CAT1固件版本类型（从文件系统读取）
 * @return CAT1_CONFIG_OLD 旧版本配置
 *         CAT1_CONFIG_NEW 新版本配置
 */
cat1_config_type_t get_cat1_config_type(void)
{
//    uint8_t *version_str = DEVICE_GetCat1Version();
	const char* version_str = lfs_system_read(SYS_FIRMWARE_LTE_VER_ID);
	
    if (version_str != NULL && strlen((char *)version_str) > 0)
    {
        log_debug("[CAT1][DAT] CAT1 Version: %s\r\n", version_str);
        
        // NA区域的旧版本
        if (strncmp((char *)version_str, "EG800QNALCR01A06M04_A0.001.A0.001", 
                    strlen("EG800QNALCR01A06M04_A0.001.A0.001")) == 0)
        {
            return CAT1_CONFIG_OLD;
        }
        // EU区域的旧版本
        else if (strncmp((char *)version_str, "EG800QEULCR01A11M04_A0.300.A0.300", 
                         strlen("EG800QEULCR01A11M04_A0.300.A0.300")) == 0)
        {
            return CAT1_CONFIG_OLD;
        }
		else
		{
			return CAT1_CONFIG_NEW;
		}
    }
    else
    {
        log_debug("[CAT1][ERR] No CAT1 version available\r\n");
    }
    
    return CAT1_CONFIG_NEW;
}

/**
 * @brief 检查是否为旧固件
 * @return true 旧固件，false 新固件
 */
bool is_old_firmware(void)
{
    return (get_cat1_config_type() == CAT1_CONFIG_OLD);
}

 //构建订阅主题函数（返回主题字符串）
 int build_subscribe_topic(mqtt_function_t func_type, char* topic_buf, size_t buf_size) {
    if (func_type >= FUNC_MAPPING_COUNT || !topic_buf) {
        return -1;
    }

    const char* sn = get_device_sn();
    const char* cmd_suffix = func_mappings[func_type].cmd_suffix;
	if(func_type == FUNC_PRODUCT_TEST)
	{
		return snprintf(topic_buf, buf_size, "pet/collar/%s/report/%s", sn, cmd_suffix);
	}
	else
	{
		return snprintf(topic_buf, buf_size, "pet/collar/%s/cmd/%s", sn, cmd_suffix);
	}
}

// 主要接口函数：根据功能枚举返回主题字符串
 const char* handle_mqtt_subscribe(mqtt_function_t func_type) {
     static char topic_buffer[128]; // 静态缓冲区存储主题

     if (build_subscribe_topic(func_type, topic_buffer, sizeof(topic_buffer)) > 0) {
         return topic_buffer;
     }

     return NULL;
 }

 // 入队
int mqtt_queue_push(mqtt_queue_t *queue, const uint8_t *data, uint16_t len, const char *topic)
{
    if (!queue || !data || len == 0 || !topic) {
        return -1;
    }
    
    int ret = -1;
    osMutexAcquire(queue->mutex, osWaitForever);
    
    if (queue->count < MQTT_QUEUE_SIZE) {
        uint8_t *data_copy = DEMO_BT_Malloc(len);
        if (!data_copy) {
            log_debug("[CAT1][ERR] Failed to allocate memory\r\n");
            osMutexRelease(queue->mutex);
            return -1;
        }
        memcpy(data_copy, data, len);
        
        mqtt_packet_t *p = &queue->packets[queue->tail];
        p->data = data_copy;
        p->data_len = len;
        strncpy(p->topic, topic, sizeof(p->topic) - 1);
        p->topic[sizeof(p->topic) - 1] = '\0';
        
        queue->tail = (queue->tail + 1) % MQTT_QUEUE_SIZE;
        queue->count++;
        
        ret = 0;
    } else {
        log_debug("[CAT1][ERR] MQTT queue full! count=%d\r\n", queue->count);
    }
    
    osMutexRelease(queue->mutex);
    return ret;
}

int mqtt_queue_pop_with_data(mqtt_queue_t *queue, mqtt_packet_t *packet)
{
    if (!queue || !packet || queue->count == 0) {
        return -1;
    }
    
    osMutexAcquire(queue->mutex, osWaitForever);
    
    mqtt_packet_t *p = &queue->packets[queue->head];
    
    // 把指针转移给调用者
    packet->data = p->data;
    packet->data_len = p->data_len;
    strncpy(packet->topic, p->topic, sizeof(packet->topic) - 1);
    packet->topic[sizeof(packet->topic) - 1] = '\0';
    
    // 清空原节点（但 data 指针已经转移，所以不能 free）
    p->data = NULL;
    p->data_len = 0;
    memset(p->topic, 0, sizeof(p->topic));
    
    queue->head = (queue->head + 1) % MQTT_QUEUE_SIZE;
    queue->count--;
    
//    log_debug("[CAT1][DAT] MQTT queue pop with data: count=%d, head=%d, tail=%d\r\n", 
//             queue->count, queue->head, queue->tail);
    
    osMutexRelease(queue->mutex);
    return 0;
}

// 判断队列是否为空
int mqtt_queue_is_empty(mqtt_queue_t *queue)
{
    if (!queue) return 1;
    
    int empty = 0;
    osMutexAcquire(queue->mutex, osWaitForever);
    empty = (queue->count == 0);
    osMutexRelease(queue->mutex);
    return empty;
}

// 查看队首（不弹出）
int mqtt_queue_peek(mqtt_queue_t *queue, mqtt_packet_t *packet)
{
    if (!queue || !packet || queue->count == 0) {
        return -1;
    }
    
    osMutexAcquire(queue->mutex, osWaitForever);
    
    mqtt_packet_t *p = &queue->packets[queue->head];
    packet->data = p->data;
    packet->data_len = p->data_len;
    strncpy(packet->topic, p->topic, sizeof(packet->topic) - 1);
    packet->topic[sizeof(packet->topic) - 1] = '\0';
    
    osMutexRelease(queue->mutex);
    return 0;
}

// 清空队列（释放所有数据）
void mqtt_queue_clear(mqtt_queue_t *queue)
{
    if (!queue) return;
    
    osMutexAcquire(queue->mutex, osWaitForever);
    
    while (queue->count > 0) {
        mqtt_packet_t *p = &queue->packets[queue->head];
        if (p->data) {
            DEMO_BT_Free(p->data);
            p->data = NULL;
        }
        queue->head = (queue->head + 1) % MQTT_QUEUE_SIZE;
        queue->count--;
    }
    queue_entry_tick = 0;
    
    osMutexRelease(queue->mutex);
}

void at_uart_send_block(const char *cmd, uint16_t len)
{
	if (cmd == NULL || len == 0)
	{
		return;
	}

	// 测试LOG
	log_debug("[CAT1][SND] %s\r\n",cmd);
//	drv_uart_write(LOG_UART, (uint8_t *)cmd, (uint32_t)len, 10);
	
	// 发送LTE
	drv_uart_write(CAT1_AT_UART, (uint8_t *)cmd, (uint32_t)len, 10);
}

/**
 * @brief 根据证书类型释放内存
 */
void free_cert_data()
{
//	cacert_data.cacertData = NULL;
	memset(b_message_data.appProduct_cacert,0,b_message_data.appProduct_cacertLen);
	b_message_data.appProduct_cacertLen = 0;
            
}

/**
 * @brief  func_lte_type	cat1指令发送
 * @param type 					指令类型
 * @return NULL
 **/
static void func_lte_type(lte_at_type_t type)
{
	static char mqtt_payload_str[MQTT_DATA_MTU_MAX];
	static int mqtt_payload_len = 0;

	memset(mqtt_payload_str, 0x0, MQTT_DATA_MTU_MAX);
	switch (type)
	{
		case LTE_TEST: // 测试AT
			sprintf(mqtt_payload_str, "AT\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QFLST: // 检查UFS是否有证书
			sprintf(mqtt_payload_str, "AT+QFLST=\"UFS:*\"\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		
		case LTE_QMTCFG_RECV: // 设置MQTT接收模式
			sprintf(mqtt_payload_str, "AT+QMTCFG=\"recv/mode\",%d,0,1\r\n", mqtt_client_idx);
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_SET_APN: // 设置APN(根据eSIM ICCID前缀匹配, 见s_apn_match_table)
			sprintf(mqtt_payload_str, "AT+CGDCONT=%d,%s,%s\r\n", 1, "IP", get_apn_by_esim_sn());
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_GET_APN: // 查询APN
			sprintf(mqtt_payload_str, "AT+CGDCONT?\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_GET_CGACT: //查询上下文
			sprintf(mqtt_payload_str, "AT+CGACT?\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_SET_CGACT: // 激活PDP上下文
			sprintf(mqtt_payload_str, "AT+CGACT=1,1\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_DEQIACT: // 未激活PDP上下文
			sprintf(mqtt_payload_str, "AT+CGACT=0,1\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_PDP: // 激活PDP上下文
			sprintf(mqtt_payload_str, "AT+QMTCFG=\"pdpcid\",0,1\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QIACT: // 激活PDP上下文
			sprintf(mqtt_payload_str, "AT+QIACT=%d\r\n",1);
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_GET_QIACT: // 查询PDP上下文
			sprintf(mqtt_payload_str, "AT+QIACT?\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QFUPL_CACERT: // 写入CA证书到UFS
			sprintf(mqtt_payload_str, "AT+QFUPL=\"ca.pem\",%d,%d\r\n",b_message_data.appProduct_cacertLen,100);
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QFUPL_CLIENT: // 写入Client.pem到UFS
			sprintf(mqtt_payload_str, "AT+QFUPL=\"client.pem\",%d,%d\r\n",b_message_data.appProduct_cacertLen,100);
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QFUPL_USERKEY: // 写入私钥到UFS
			sprintf(mqtt_payload_str, "AT+QFUPL=\"key.pem\",%d,%d\r\n",b_message_data.appProduct_cacertLen,100);
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QFUPL_CACERT_DATA: 
			if (b_message_data.appProduct_cacertLen > 0 && b_message_data.appProduct_cacertLen < sizeof(mqtt_payload_str)) {
				strncpy(mqtt_payload_str, (char *)b_message_data.appProduct_cacert, b_message_data.appProduct_cacertLen);
				mqtt_payload_len = b_message_data.appProduct_cacertLen;
			}
			break;
		case LTE_QFUPL_CLIENT_DATA: 
			if (b_message_data.appProduct_cacertLen > 0 && b_message_data.appProduct_cacertLen < sizeof(mqtt_payload_str)) {
				strncpy(mqtt_payload_str, (char *)b_message_data.appProduct_cacert, b_message_data.appProduct_cacertLen);
				mqtt_payload_len = b_message_data.appProduct_cacertLen;
			}
			break;
		case LTE_QFUPL_USERKEY_DATA: 
			if (b_message_data.appProduct_cacertLen > 0 && b_message_data.appProduct_cacertLen < sizeof(mqtt_payload_str)) {
				strncpy(mqtt_payload_str, (char *)b_message_data.appProduct_cacert, b_message_data.appProduct_cacertLen);
				mqtt_payload_len = b_message_data.appProduct_cacertLen;
			}
			break;
		case LTE_QFDEL_CACERT: // 删除CA证书文件
			sprintf(mqtt_payload_str, "AT+QFDEL=\"ca.pem\"\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QFDEL_CLIENT: // 删除Client证书文件
			sprintf(mqtt_payload_str, "AT+QFDEL=\"client.pem\"\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QFDEL_USERKEY: // 删除Userkey证书文件
			sprintf(mqtt_payload_str, "AT+QFDEL=\"key.pem\"\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QMTCFG_SSL: // 配置SSL
			if(is_old_firmware())
			{
				sprintf(mqtt_payload_str, "AT+QMTCFG=\"SSL\",0,1,0\r\n");
			}
			else
			{
				sprintf(mqtt_payload_str, "AT+QMTCFG=\"SSL\",%d,1,0\r\n",mqtt_client_idx);
			}
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QMTCFG_SELECT_VERSION: // 查询版本
			sprintf(mqtt_payload_str, "AT+QMTCFG=\"VERSION\"?\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QMTCFG_VERSION: // 设置版本
			sprintf(mqtt_payload_str, "AT+QMTCFG=\"VERSION\",%d,4\r\n",mqtt_client_idx);
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QSSLCFG_SECLEVEL: // 设置SSL鉴权模式
			sprintf(mqtt_payload_str, "AT+QSSLCFG=\"seclevel\",0,2\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QSSLCFG_SSLVERSION: // 设置SSL 协议版本（TLS1.2）
			sprintf(mqtt_payload_str, "AT+QSSLCFG=\"sslversion\",0,4\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QSSLCFG_CIPHERSUITE: // 设置SSL 加密套件
			sprintf(mqtt_payload_str, "AT+QSSLCFG=\"ciphersuite\",0,0xFFFF\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QSSLCFG_SNI: // 设置SSL SNI
			sprintf(mqtt_payload_str, "AT+QSSLCFG=\"SNI\",0,1\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QSSLCFG_IGNOREMULTICA: // 忽略多级证书链
			sprintf(mqtt_payload_str, "AT+QSSLCFG=\"ignoremulticertchainverify\",0,1\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QSSLCFG_IGNOREINVALIDCA: // 忽略无效证书
			sprintf(mqtt_payload_str, "AT+QSSLCFG=\"ignoremulticertchainverify\",0,1\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QSSLCFG_CACERT: // 设置CA SSL
			sprintf(mqtt_payload_str, "AT+QSSLCFG=\"cacert\",0,\"UFS:ca.pem\"\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QSSLCFG_CLIENTCERT: // 设置CLIENT SSL
			sprintf(mqtt_payload_str, "AT+QSSLCFG=\"clientcert\",0,\"UFS:client.pem\"\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QSSLCFG_CLIENTKEY: // 设置KEY SSL
			sprintf(mqtt_payload_str, "AT+QSSLCFG=\"clientkey\",0,\"UFS:key.pem\"\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_CFUN_REBOOT: // 模块重启
			// DTR设置
			lteExitsleepDtrLow();
			sprintf(mqtt_payload_str, "AT+CFUN=1,1\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_ENTER_SLEEP: // 进入sleep
			sprintf(mqtt_payload_str, "AT+QSCLK=1\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_CFUN_SHUTDOWN: // 模块关机
			sprintf(mqtt_payload_str, "AT+QPOWD\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_ATE0: // 关闭回显
			sprintf(mqtt_payload_str, "ATE0\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_DATAFORMAT: // 数据收发格式（HEX）
			sprintf(mqtt_payload_str, "AT+QMTCFG=\"dataformat\",%d,0,0\r\n", mqtt_client_idx); //MQTT_CLIENT_IDX
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_VERSION: // 查询版本号
			sprintf(mqtt_payload_str, "AT+QGMR\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_CREG: // 查询网络注册状态
			sprintf(mqtt_payload_str, "AT+CEREG?\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_CGATT: // 查询网络附着状态
			sprintf(mqtt_payload_str, "AT+CGATT?\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_CPIN: // 查询SIM卡
			sprintf(mqtt_payload_str, "AT+CPIN?\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_CSQ: // 查询网络信号
			// DTR设置
			lteExitsleepDtrLow();
			sprintf(mqtt_payload_str, "AT+CSQ\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_GET_COPS: // 查询运营商
			// DTR设置
	lteExitsleepDtrLow();
			sprintf(mqtt_payload_str, "AT+COPS?\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QENG_SERVE: // 查询基站信息
			// DTR设置
	lteExitsleepDtrLow();
			sprintf(mqtt_payload_str, "AT+QENG=\"servingcell\"\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QENG_NEIGHBOUR: // 查询邻近小区信息
			// DTR设置
	lteExitsleepDtrLow();
			sprintf(mqtt_payload_str, "AT+QENG=\"neighbourcell\"\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_RANDIS: // 开关RNDIS网卡（自动拨号）
			//			sprintf(mqtt_payload_str,"AT+RNDISCALL=%d\r\n", LTE_DISABLE);
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MAIN_RING: // 
			sprintf(mqtt_payload_str, "AT+QCFG=\"urc/ri/ring\"\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_CIMI: // 
			sprintf(mqtt_payload_str, "AT+CIMI\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_COPS: // AT+COPS=1,2,"46000",7
			sprintf(mqtt_payload_str, "AT+COPS=1,2,\"46000\",7\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_OPEN: // 设置服务端IP和端口并打开
		#if MQTT_CONN_PASSWORD
			sprintf(mqtt_payload_str, "AT+QMTOPEN=%d,\"%s\",%d\r\n", mqtt_client_idx, MQTT_HOST_NAME_TEST, MQTT_TPORT_TEST);
		#else
			sprintf(mqtt_payload_str, "AT+QMTOPEN=%d,\"%s\",%d\r\n", mqtt_client_idx, MQTT_HOST_NAME, MQTT_TPORT);
		#endif
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_CLOSE: // 关闭服务端IP和端口
			sprintf(mqtt_payload_str, "AT+QMTCLOSE=%d\r\n", mqtt_client_idx);
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_CONN: // 连接MQTT服务端
		#if MQTT_CONN_PASSWORD
			sprintf(mqtt_payload_str,"AT+QMTCONN=%d,\"%s\",\"%s\"\r\n",mqtt_client_idx,get_device_sn(), MQTT_PASSWORD);
		#else
			sprintf(mqtt_payload_str, "AT+QMTCONN=%d,\"%s\"\r\n", mqtt_client_idx, get_device_sn());
		#endif
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_ISSTATE: // 查询MQTT连接状态
			// DTR设置
			lteExitsleepDtrLow();
			qmtconn_info_seen = false;  // 新一轮查询，清除信息行标记
			sprintf(mqtt_payload_str, "AT+QMTCONN?\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_DISCONN: // 断连MQTT服务端
			// DTR设置
			lteExitsleepDtrLow();
			sprintf(mqtt_payload_str, "AT+QMTDISC=%d\r\n", mqtt_client_idx);
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		//配置持久会话
		case LTE_MQTT_SESSION: // 断连MQTT服务端
			sprintf(mqtt_payload_str, "AT+QMTCFG=\"session\",%d,0\r\n", mqtt_client_idx);
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_SUB_PRODUCT_TEST:	
			sprintf(mqtt_payload_str, "AT+QMTSUB=%d,%d,\"%s\",0\r\n", mqtt_client_idx, MQTT_MES_ID, handle_mqtt_subscribe(FUNC_PRODUCT_TEST));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_SUB_CHECK:
			sprintf(mqtt_payload_str, "AT+QMTSUB=?\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_SUB_DEVICESTATUE:	   			//查询设备状态
			sprintf(mqtt_payload_str, "AT+QMTSUB=%d,%d,\"%s\",0\r\n", mqtt_client_idx, MQTT_MES_ID, handle_mqtt_subscribe(FUNC_QUERY_DEVICE_STATE));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_SUB_DEVICEMODE:				//设置设备模式
			sprintf(mqtt_payload_str,"AT+QMTSUB=%d,%d,\"%s\",1\r\n",mqtt_client_idx,MQTT_MES_ID,handle_mqtt_subscribe(FUNC_CONTROL_DEVICE_MODE));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_SUB_ENODEB:				//查询基站消息
			sprintf(mqtt_payload_str,"AT+QMTSUB=%d,%d,\"%s\",0\r\n",mqtt_client_idx,MQTT_MES_ID,handle_mqtt_subscribe(FUNC_QUERY_ENODEB));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
//		case LTE_MQTT_SUB_AUDIOREALTIME:				//发送实时音频
//			sprintf(mqtt_payload_str,"AT+QMTSUB=%d,%d,\"%s\",0\r\n",mqtt_client_idx,MQTT_MES_ID,handle_mqtt_subscribe(FUNC_CONTROL_AUDIOREALTIME));
//			mqtt_payload_len = strlen(mqtt_payload_str);
//			break;
		case LTE_MQTT_SUB_AUDIOCMD:				//发送语音
			sprintf(mqtt_payload_str,"AT+QMTSUB=%d,%d,\"%s\",1\r\n",mqtt_client_idx,MQTT_MES_ID,handle_mqtt_subscribe(FUNC_CONTROL_AUDIOCMD));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_SUB_DEVICEDELTE:				//删除设备
			sprintf(mqtt_payload_str,"AT+QMTSUB=%d,%d,\"%s\",1\r\n",mqtt_client_idx,MQTT_MES_ID,handle_mqtt_subscribe(FUNC_CONFIG_DEVICEDELTE));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_SUB_WARNINGSTATE:				//设备状态上报
			sprintf(mqtt_payload_str,"AT+QMTSUB=%d,%d,\"%s\",0\r\n",mqtt_client_idx,MQTT_MES_ID,handle_mqtt_subscribe(FUNC_REPORT_WARNINGSTATE));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_SUB_VIBRATE:				//设备震动
			sprintf(mqtt_payload_str,"AT+QMTSUB=%d,%d,\"%s\",1\r\n",mqtt_client_idx,MQTT_MES_ID,handle_mqtt_subscribe(FUNC_CONTROL_VIBRATE));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_SUB_TIMESYNC:				//时间同步
			sprintf(mqtt_payload_str,"AT+QMTSUB=%d,%d,\"%s\",0\r\n",mqtt_client_idx,MQTT_MES_ID,handle_mqtt_subscribe(FUNC_CONFIG_TIMESYNC));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_SUB_WIFIMAC:				//wifi设置/查询
			sprintf(mqtt_payload_str,"AT+QMTSUB=%d,%d,\"%s\",1\r\n",mqtt_client_idx,MQTT_MES_ID,handle_mqtt_subscribe(FUNC_CONFIG_WIFIMAC));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_SUB_FENCE:				//V1.6设置/查询围栏
			sprintf(mqtt_payload_str,"AT+QMTSUB=%d,%d,\"%s\",1\r\n",mqtt_client_idx,MQTT_MES_ID,handle_mqtt_subscribe(FUNC_CONFIG_FENCE));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_SUB_VOICE:				//V1.6查询语音包
			sprintf(mqtt_payload_str,"AT+QMTSUB=%d,%d,\"%s\",0\r\n",mqtt_client_idx,MQTT_MES_ID,handle_mqtt_subscribe(FUNC_QUERY_VOICE));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_SUB_LIGHT:				//V1.6控制灯光
			sprintf(mqtt_payload_str,"AT+QMTSUB=%d,%d,\"%s\",1\r\n",mqtt_client_idx,MQTT_MES_ID,handle_mqtt_subscribe(FUNC_CONTROL_LIGHT));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_USUB_DEVICESTATUE: // 取消订阅的主题
			sprintf(mqtt_payload_str, "AT+QMTUNS=%d,%d,\"%s\"\r\n", mqtt_client_idx, MQTT_MES_ID, handle_mqtt_subscribe(FUNC_QUERY_DEVICE_STATE));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_USUB_DEVICEMODE: // 取消订阅的主题
			sprintf(mqtt_payload_str, "AT+QMTUNS=%d,%d,\"%s\"\r\n", mqtt_client_idx, MQTT_MES_ID, handle_mqtt_subscribe(FUNC_CONTROL_DEVICE_MODE));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_USUB_ENODEB: // 取消订阅的主题
			sprintf(mqtt_payload_str, "AT+QMTUNS=%d,%d,\"%s\"\r\n", mqtt_client_idx, MQTT_MES_ID, handle_mqtt_subscribe(FUNC_QUERY_ENODEB));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
//		case LTE_MQTT_USUB_AUDIOREALTIME: // 取消订阅的主题
//			sprintf(mqtt_payload_str, "AT+QMTUNS=%d,%d,\"%s\"\r\n", mqtt_client_idx, MQTT_MES_ID, handle_mqtt_subscribe(FUNC_CONTROL_AUDIOREALTIME));
//			mqtt_payload_len = strlen(mqtt_payload_str);
//			break;
		case LTE_MQTT_USUB_AUDIOCMD: // 取消订阅的主题
			sprintf(mqtt_payload_str, "AT+QMTUNS=%d,%d,\"%s\"\r\n", mqtt_client_idx, MQTT_MES_ID, handle_mqtt_subscribe(FUNC_CONTROL_AUDIOCMD));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_USUB_DEVICEDELTE: // 取消订阅的主题
			sprintf(mqtt_payload_str, "AT+QMTUNS=%d,%d,\"%s\"\r\n", mqtt_client_idx, MQTT_MES_ID, handle_mqtt_subscribe(FUNC_CONFIG_DEVICEDELTE));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_USUB_WARNINGSTATE: // 取消订阅的主题
			sprintf(mqtt_payload_str, "AT+QMTUNS=%d,%d,\"%s\"\r\n", mqtt_client_idx, MQTT_MES_ID, handle_mqtt_subscribe(FUNC_REPORT_WARNINGSTATE));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_USUB_VIBRATE: // 取消订阅的主题
			sprintf(mqtt_payload_str, "AT+QMTUNS=%d,%d,\"%s\"\r\n", mqtt_client_idx, MQTT_MES_ID, handle_mqtt_subscribe(FUNC_CONTROL_VIBRATE));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_USUB_TIMESYNC: // 取消订阅的主题
			sprintf(mqtt_payload_str, "AT+QMTUNS=%d,%d,\"%s\"\r\n", mqtt_client_idx, MQTT_MES_ID, handle_mqtt_subscribe(FUNC_CONFIG_TIMESYNC));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_USUB_WIFIMAC: // 取消订阅的主题
			sprintf(mqtt_payload_str, "AT+QMTUNS=%d,%d,\"%s\"\r\n", mqtt_client_idx, MQTT_MES_ID, handle_mqtt_subscribe(FUNC_CONFIG_WIFIMAC));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_USUB_FENCE: // 取消订阅的主题
			sprintf(mqtt_payload_str, "AT+QMTUNS=%d,%d,\"%s\"\r\n", mqtt_client_idx, MQTT_MES_ID, handle_mqtt_subscribe(FUNC_CONFIG_FENCE));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_USUB_VOICE: // 取消订阅的主题
			sprintf(mqtt_payload_str, "AT+QMTUNS=%d,%d,\"%s\"\r\n", mqtt_client_idx, MQTT_MES_ID, handle_mqtt_subscribe(FUNC_QUERY_VOICE));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_USUB_LIGHT: // 取消订阅的主题
			sprintf(mqtt_payload_str, "AT+QMTUNS=%d,%d,\"%s\"\r\n", mqtt_client_idx, MQTT_MES_ID, handle_mqtt_subscribe(FUNC_CONTROL_LIGHT));
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_PUB:																																								// 发布消息指令
			sprintf(mqtt_payload_str, "AT+QMTPUBEX=%d,%d,%d,%d,\"%s\",%d\r\n", mqtt_client_idx, 0, MQTT_SUBSCRIBE_QOS, MQTT_MSG_ISRETAIN, report_topic, response_mqtt_data_len); //, response_mqtt_data_len
			mqtt_payload_len = strlen(mqtt_payload_str);
			/* 此命令发出后模组将进入">"数据接收态，等待response_mqtt_data_len字节payload */
			g_pubdata_pending_len = (uint16_t)response_mqtt_data_len;
			break;
		case LTE_MQTT_PRODUCT_PUB:
			product_build_subscribe_topic(report_topic, sizeof(report_topic));
			// 发布消息指令
			sprintf(mqtt_payload_str, "AT+QMTPUBEX=%d,%d,%d,%d,\"%s\",%d\r\n", mqtt_client_idx, 0, MQTT_SUBSCRIBE_QOS, MQTT_MSG_ISRETAIN, report_topic, response_mqtt_data_len); //, response_mqtt_data_len
			mqtt_payload_len = strlen(mqtt_payload_str);
			g_pubdata_pending_len = (uint16_t)response_mqtt_data_len;
			break;
		case LTE_MQTT_PUBMESSAGEDATA: // 发布消息数据
			memcpy(mqtt_payload_str, response_mqtt_data, response_mqtt_data_len);
			mqtt_payload_len = response_mqtt_data_len;
			/* payload已发到模组，数据接收态结束 */
			g_pubdata_pending_len = 0;
			
			// 如果是重发，不处理队列
			if (is_retransmitting) {
				is_retransmitting = false;
				g_mqtt_sending = 1;
				break;
			}
			
//			// 保存当前发送类型
//			if (is_auto_reporting) {
//				g_current_send_type = 2;  // 主动上报
//			} else {
//				g_current_send_type = 1;  // 云指令
//			}
//			
//			// 清除主动上报标志（如果当前完成的是主动上报）
//			if (is_auto_reporting) {
//				is_auto_reporting = false;
//				log_debug("Auto report completed\n");
//			}
//			
//			// 检查队列中是否有下一包数据
//			mqtt_send_done_handler(g_current_send_type);
//			  if (g_current_send_type != 2) {
//                // 云指令发送完成，处理队列中的下一条
//                if (g_mqtt_queue && !mqtt_queue_is_empty(g_mqtt_queue)) {
//                    mqtt_packet_t packet;
//                    if (mqtt_queue_pop_with_data(g_mqtt_queue, &packet) == 0) {
//                        log_debug("Sending next queued cloud command, len=%d\n", packet.data_len);
//                        
//                        if (send_data_to_comm_task(COMM_TASK_ID, TASK_COMM_DATAJSON, 
//                                                   packet.data, packet.data_len) == pdTRUE) {
//                            DEMO_BT_Free(packet.data);
//                            // g_mqtt_sending 保持为 1，g_current_send_type 保持为 1
//                            log_debug("Next cloud command sent, waiting for response\n");
//                        } else {
//                            DEMO_BT_Free(packet.data);
//                            g_mqtt_sending = 0;
//                            g_current_send_type = 0;
//                            log_debug("Failed to send next cloud command\n");
//                        }
//                    } else {
//                        g_mqtt_sending = 0;
//                        g_current_send_type = 0;
//                        log_debug("Queue pop failed, no more commands\n");
//                    }
//                } else {
//                    g_mqtt_sending = 0;
//                    g_current_send_type = 0;
//                    log_debug("No more cloud commands in queue\n");
//                }
//            } else if (g_current_send_type == 2) {
//                // 主动上报完成
//                g_mqtt_sending = 0;
//                g_current_send_type = 0;
//                log_debug("Auto report completed, system idle\n");
//                
//                // 检查是否有待处理的云指令，如果有则立即发送
//                if (g_mqtt_queue && !mqtt_queue_is_empty(g_mqtt_queue)) {
//                    mqtt_packet_t packet;
//                    if (mqtt_queue_peek(g_mqtt_queue, &packet) == 0) {
//                        log_debug("Pending cloud command found after auto report, sending now\n");
//                        // 主动上报完成后立即处理队列中的云指令
//                        osEventFlagsSet(LteEventId, LTE_EVENT_CMD_READY);
//                    }
//                }
//            }
			break;
		case LTE_MQTT_PRODUCT_PUBMESSAGEDATA: // 发布消息数据
			memcpy(mqtt_payload_str, response_mqtt_data, response_mqtt_data_len);
			mqtt_payload_len = response_mqtt_data_len; // response_mqtt_data_len;
			/* payload已发到模组，数据接收态结束 */
			g_pubdata_pending_len = 0;
			break;
		case LTE_KEEPALIVE: // 心跳包
			sprintf(mqtt_payload_str, "AT+QMTCFG=\"%s\",%d,%d\r\n", MQTT_CFG_TYPE, mqtt_client_idx, MQTT_KEEPALIVETIME);
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_MQTT_MSG: // 上报云端的MQTT数据
			sprintf(mqtt_payload_str, "\"%s\"\r\n", "TEST");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_DEVICE_GSN: // LTE SN
			sprintf(mqtt_payload_str, "AT+GSN=0\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_DEVICE_QCCID: // LTE esim SN
			sprintf(mqtt_payload_str, "AT+QCCID\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_WIFI_SCAN: // LTE WiFi scan AT+QWIFISCAN=12000,1,6,5,0,1\r\n
	//		sprintf(mqtt_payload_str, "AT+QWIFISCAN=%d,%d,%d,%d,%d,%d\r\n", WIFISCAN_TIME, WIFISCAN_ROUND, WIFISCAN_NUM, WIFISCAN_TIMEOUT, WIFISCAN_PRIORITY,WIFISCAN_SSID_TYPE);
			sprintf(mqtt_payload_str, "AT+QWIFISCAN=12000,3,8,5,1,1\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		//http GET实时音频数据
//		case LTE_SET_APN1: // 设置APN
//			sprintf(mqtt_payload_str, "AT+CGDCONT=%d,%s,%s\r\n", 1, "IP", "linksnet");
//			mqtt_payload_len = strlen(mqtt_payload_str);
//			break;
//		case LTE_QIACT1: // 激活PDP上下文
//			sprintf(mqtt_payload_str, "AT+QIACT=%d\r\n",2);
//			mqtt_payload_len = strlen(mqtt_payload_str);
//			break;
		case LTE_QHTTPCFG_PDP: 
			sprintf(mqtt_payload_str, "AT+QHTTPCFG=\"contextid\",1\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QHTTPCFG_HEADER: 
			sprintf(mqtt_payload_str, "AT+QHTTPCFG=\"responseheader\",0\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		
		// HTTPS SSL配置 (上下文1)  
		case LTE_QSSLCFG_HTTPS_SSLCTXID:
			sprintf(mqtt_payload_str, "AT+QHTTPCFG=\"sslctxid\",1\r\n");
		mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QSSLCFG_HTTPS_SECLEVEL:
			sprintf(mqtt_payload_str, "AT+QSSLCFG=\"seclevel\",1,0\r\n");
		mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QSSLCFG_HTTPS_SSLVERSION:
			sprintf(mqtt_payload_str, "AT+QSSLCFG=\"sslversion\",1,3\r\n");
		mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QSSLCFG_HTTPS_CIPHERSUITE:
			sprintf(mqtt_payload_str, "AT+QSSLCFG=\"ciphersuite\",1,0xFFFF\r\n");
		mqtt_payload_len = strlen(mqtt_payload_str);
			break;
	
		case LTE_QHTTP_QSSLCFG_CLIENTKEY: // 设置KEY SSL
			sprintf(mqtt_payload_str, "AT+QSSLCFG=\"clientkey\",1,\"UFS:user_key.pem\"\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		//UPL GET
		case LTE_QHTTPURL: 
			sprintf(mqtt_payload_str, "AT+QHTTPURL=%d,80\r\n",http_data.httpUrlLength);
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QHTTPURL_DATA: 
			if (http_data.httpUrlLength > 0 && http_data.httpUrlLength < sizeof(mqtt_payload_str)) {
                strncpy(mqtt_payload_str, (char *)http_data.httpUrlData, http_data.httpUrlLength);
                mqtt_payload_len = http_data.httpUrlLength;
            }
			break;
		case LTE_QHTTGET: 
			sprintf(mqtt_payload_str, "AT+QHTTPGET=80\r\n");
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		 case LTE_QFDEL_AUDIO: // 删除音频文件
            sprintf(mqtt_payload_str, "AT+QFDEL=\"audio_realtime\"\r\n");
            mqtt_payload_len = strlen(mqtt_payload_str);
            break;
		//写入文件系统
		case LTE_QHTTPREADFILE: 
			sprintf(mqtt_payload_str, "AT+QHTTPREADFILE=\"UFS:%s\",80\r\n",HTTP_READFILE_AUDIONAME);
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QFOPEN: 
			if (strlen(http_data.filename) > 0)
            {
				sprintf(mqtt_payload_str, "AT+QFOPEN=\"UFS:%s\",0\r\n", http_data.filename);
			}
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QFREAD: 
				sprintf(mqtt_payload_str, "AT+QFREAD=%d,%d\r\n", http_data.file_handle, http_data.file_size);
				mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QFCLOSE: 
			sprintf(mqtt_payload_str, "AT+QFCLOSE=%d\r\n",http_data.file_handle);
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_QFDEL:
			sprintf(mqtt_payload_str, "AT+QFDEL=\"UFS:%s\"\r\n",http_data.filename);
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
		case LTE_FIRMWARE_UPDATA:
			sprintf(mqtt_payload_str, "AT+QFOTADL=\"%s\",1,50\r\n",firmware_url.firmwareUrlData);
			mqtt_payload_len = strlen(mqtt_payload_str);
			break;
	}
	at_uart_send_block(mqtt_payload_str, mqtt_payload_len);
}

/**
 * @brief  lte_check_cmd_rtos	cat1 指令入队接口
 * @param type 					指令类型
 * @param ack 					期望收到的回复
 * @param retry_max 			最大重试次数
 * @param waitTime_ms 			超时时间
 * @param has_extra_response 	收到回复数据num的标识符
 * @return BaseType_t pdTRUE-成功入队 pdFALSE-队列已满
 **/
static BaseType_t lte_check_cmd_rtos(lte_at_type_t type, const char *ack, uint16_t retry_max, uint32_t waitTime_ms, uint8_t has_extra_response)
{
	BaseType_t result = pdFALSE;
	osMutexAcquire(LteMutex, osWaitForever);
	
	if ((lteCmdQueueTail + 1) % AT_CMD_QUEUE_SIZE != lteCmdQueueHead)
	{
		lteCmdQueue[lteCmdQueueTail] = (lteCmdItem_t){
			.type = type,
			.ack = ack,
			.retry_max = retry_max,
			.retry_count = 0,
			.waitTime_ms = waitTime_ms,
			.has_extra_response = has_extra_response,
			.status = CMD_STATUS_PENDING, // 初始化状态
			.is_firmware_upgrade = (type == LTE_FIRMWARE_UPDATA) ? 1 : 0 // 标记固件升级
		};
		lteCmdQueueTail = (lteCmdQueueTail + 1) % AT_CMD_QUEUE_SIZE;
		result = pdTRUE;

		// 通知有新命令入队
		osEventFlagsSet(LteEventId, LTE_EVENT_CMD_READY);
	}

	osMutexRelease(LteMutex);

	return result;
}

/**
 * @brief 启动差分固件 HTTP(s)下载和升级流程
 * @param url 要下载的固件URL地址
 * @param url_len URL长度
 * @return BaseType_t pdTRUE-成功 pdFALSE-失败
 */
BaseType_t start_delta_firmware(const char *url, uint16_t url_len)
{
    BaseType_t result = pdFALSE;

    if (url == NULL || url_len == 0 || url_len >= sizeof(firmware_url.firmwareUrlData)) {
        log_debug("[CAT1][ERR] Invalid URL\r\n");
        return pdFALSE;
    }
    
    // 保存URL和长度
    memset(firmware_url.firmwareUrlData, 0, sizeof(firmware_url.firmwareUrlData));
    strncpy((char *)firmware_url.firmwareUrlData, url, url_len);
    firmware_url.firmwareUrlData[url_len] = '\0';
    firmware_url.firmwareUrlLength = strlen((char *)firmware_url.firmwareUrlData);
    
//    log_debug("[CAT1][DAT] firmware upgrade, URL: %s, Length: %d\r\n", 
//              firmware_url.firmwareUrlData, firmware_url.firmwareUrlLength);
    
	lte_check_cmd_rtos(LTE_ATE0, "OK\r\n", 1, 500, 0);

	lte_check_cmd_rtos(LTE_CPIN, "OK\r\n", 100, 2000, 0);
	
    // 配置PDP上下文
    if (lte_check_cmd_rtos(LTE_SET_APN, "OK\r\n", 1, 1000, 0) != pdTRUE) {
        return pdFALSE;
    }
	lte_check_cmd_rtos(LTE_SET_CGACT, "OK\r\n", 1, 1000, 0);
	
	lte_check_cmd_rtos(LTE_CREG, "OK\r\n", 300, 1000, 0);

    // 执行差分固件升级命令
	if (lte_check_cmd_rtos(LTE_FIRMWARE_UPDATA, "OK\r\n", 1, 1000, 0) != pdTRUE) {
        return pdFALSE;
    }
	
    result = pdTRUE;
    
    return result;
}

/**
 * @brief 启动HTTP下载流程
 * @param url 要下载的URL地址
 * @param url_len URL长度
 * @return BaseType_t pdTRUE-成功入队 pdFALSE-队列已满
 */
BaseType_t start_http_download(const char *url, uint16_t url_len)
{
    BaseType_t result = pdFALSE;

	// 保存URL数据到http_data结构
    if (url == NULL || url_len == 0 || url_len >= sizeof(http_data.httpUrlData)) {
        log_debug("[CAT1][ERR] Invalid URL parameters\r\n");
        return pdFALSE;
    }
    
    // 保存URL和长度
    memset(http_data.httpUrlData, 0, sizeof(http_data.httpUrlData));
    strncpy((char *)http_data.httpUrlData, url, url_len);
    http_data.httpUrlData[url_len] = '\0'; // 确保字符串终止
    http_data.httpUrlLength = strlen((char *)http_data.httpUrlData);
	
    // 初始化读取参数
	http_data.read_size = 0;
    http_data.read_offset = 0;
	http_data.file_size = HTTP_READFILE_BLOCKSIZE;
	
	//取消订阅的主题
	if (lte_check_cmd_rtos(LTE_MQTT_USUB_DEVICESTATUE, "OK\r\n", 1, 1000, 1) != pdTRUE)
		return pdFALSE;
	if (lte_check_cmd_rtos(LTE_MQTT_USUB_DEVICEMODE, "OK\r\n", 1, 1000, 1) != pdTRUE)
		return pdFALSE;
	if (lte_check_cmd_rtos(LTE_MQTT_USUB_ENODEB, "OK\r\n", 1, 1000, 1) != pdTRUE)
		return pdFALSE;
//	if (lte_check_cmd_rtos(LTE_MQTT_USUB_AUDIOREALTIME, "OK\r\n", 1, 1000, 1) != pdTRUE)
//		return pdFALSE;
	if (lte_check_cmd_rtos(LTE_MQTT_USUB_AUDIOCMD, "OK\r\n", 1, 1000, 1) != pdTRUE)
		return pdFALSE;
	if (lte_check_cmd_rtos(LTE_MQTT_USUB_VIBRATE, "OK\r\n", 1, 1000, 1) != pdTRUE)
		return pdFALSE;
	if (lte_check_cmd_rtos(LTE_MQTT_USUB_DEVICEDELTE, "OK\r\n", 1, 1000, 1) != pdTRUE)
		return pdFALSE;
	if (lte_check_cmd_rtos(LTE_MQTT_USUB_WARNINGSTATE, "OK\r\n", 1, 1000, 1) != pdTRUE)
		return pdFALSE;
	/* V1.6 新增主题取消订阅 */
	if (lte_check_cmd_rtos(LTE_MQTT_USUB_TIMESYNC, "OK\r\n", 1, 1000, 1) != pdTRUE)
		return pdFALSE;
	if (lte_check_cmd_rtos(LTE_MQTT_USUB_WIFIMAC, "OK\r\n", 1, 1000, 1) != pdTRUE)
		return pdFALSE;
	if (lte_check_cmd_rtos(LTE_MQTT_USUB_FENCE, "OK\r\n", 1, 1000, 1) != pdTRUE)
		return pdFALSE;
	if (lte_check_cmd_rtos(LTE_MQTT_USUB_VOICE, "OK\r\n", 1, 1000, 1) != pdTRUE)
		return pdFALSE;
	if (lte_check_cmd_rtos(LTE_MQTT_USUB_LIGHT, "OK\r\n", 1, 1000, 1) != pdTRUE)
		return pdFALSE;
	//关闭MQTT
	if (lte_check_cmd_rtos(LTE_MQTT_CLOSE, "OK\r\n", 1, 1000, 1) != pdTRUE)
        return pdFALSE;
//	if (lte_check_cmd_rtos(LTE_GET_QIACT, "OK\r\n", 1, 1000, 0) != pdTRUE)
//        return pdFALSE;
//	if (lte_check_cmd_rtos(LTE_SET_APN, "OK\r\n", 1, 1000, 0) != pdTRUE)
//        return pdFALSE;
//	lte_check_cmd_rtos(LTE_GET_CGACT, "OK\r\n", 1, 1000, 0);
	lte_check_cmd_rtos(LTE_SET_APN, "OK\r\n", 1, 1000, 0);
	lte_check_cmd_rtos(LTE_SET_CGACT, "OK\r\n", 1, 1000, 0);
//        return pdFALSE;
    // 配置HTTP上下文
    if (lte_check_cmd_rtos(LTE_QHTTPCFG_PDP, "OK\r\n", 1, 1000, 0) != pdTRUE)
        return pdFALSE;
    
    if (lte_check_cmd_rtos(LTE_QHTTPCFG_HEADER, "OK\r\n", 1, 1000, 0) != pdTRUE)
        return pdFALSE;
        
//	lte_check_cmd_rtos(LTE_SET_APN, "OK\r\n", 1, 1000, 0);
	
//	lte_check_cmd_rtos(LTE_QIACT, "OK\r\n", 30, 3000, 0);
	
    // SSL配置
    if (lte_check_cmd_rtos(LTE_QSSLCFG_HTTPS_SSLCTXID, "OK\r\n", 1, 1000, 0) != pdTRUE)
        return pdFALSE;
        
    if (lte_check_cmd_rtos(LTE_QSSLCFG_HTTPS_SSLVERSION, "OK\r\n", 1, 1000, 0) != pdTRUE)
        return pdFALSE;
        
    if (lte_check_cmd_rtos(LTE_QSSLCFG_HTTPS_CIPHERSUITE, "OK\r\n", 1, 1000, 0) != pdTRUE)
        return pdFALSE;
        
    if (lte_check_cmd_rtos(LTE_QSSLCFG_HTTPS_SECLEVEL, "OK\r\n", 1, 1000, 0) != pdTRUE)
        return pdFALSE;
    
//	if (lte_check_cmd_rtos(LTE_QSSLCFG_CACERT, "OK\r\n", 1, 1000, 0) != pdTRUE)
//        return pdFALSE;
//	if (lte_check_cmd_rtos(LTE_QSSLCFG_CLIENTCERT, "OK\r\n", 1, 1000, 0) != pdTRUE)
//        return pdFALSE;
//	if (lte_check_cmd_rtos(LTE_QSSLCFG_CLIENTKEY, "OK\r\n", 1, 1000, 0) != pdTRUE)
//        return pdFALSE;

	 if (lte_check_cmd_rtos(LTE_QFLST, "OK\r\n", 1, 3000, 0) != pdTRUE)
        return pdFALSE;
	 
    // 设置URL
    if (lte_check_cmd_rtos(LTE_QHTTPURL, "CONNECT", 1, 3000, 0) != pdTRUE)
        return pdFALSE;
    
    // 发送URL数据
    if (lte_check_cmd_rtos(LTE_QHTTPURL_DATA, "OK\r\n", 1, 3000, 0) != pdTRUE)
        return pdFALSE;
    
    // 执行HTTP GET
    lte_check_cmd_rtos(LTE_QHTTGET, "OK\r\n", 1, 1000, 1);
    
    // 读取文件到UFS
	lte_check_cmd_rtos(LTE_QHTTPREADFILE, "OK\r\n", 1, 3000, 1);
    
	lte_check_cmd_rtos(LTE_QFLST, "OK\r\n", 1, 3000, 0);
//        return pdFALSE;
    lte_check_cmd_rtos(LTE_QFOPEN, "OK\r\n", 1, 3000, 0);
	
	lte_check_cmd_rtos(LTE_QFREAD, "OK\r\n", 1, 3000, 0);
	
	
    result = pdTRUE;
    
    return result;
}

/**
 * @brief 从UFS删除证书文件
 */
void delete_certificate_from_ufs(CACERT_Type_t cert_type)
{
    switch(cert_type) {
        case CACERT_CA:
            log_debug("[CAT1][STA] Deleting CA\r\n");
            lte_check_cmd_rtos(LTE_QFDEL_CACERT, "OK\r\n", 1, 3000, 0);
            break;
        case CACERT_CLIENT:
            log_debug("[CAT1][STA] Deleting CLIENT\r\n");
            lte_check_cmd_rtos(LTE_QFDEL_CLIENT, "OK\r\n", 1, 3000, 0);
            break;
        case CACERT_USERKEY:
            log_debug("[CAT1][STA] Deleting USERKEY\r\n");
            lte_check_cmd_rtos(LTE_QFDEL_USERKEY, "OK\r\n", 1, 3000, 0);
            break;
        default:
            log_debug("[CAT1][ERR] Unknown certificate\r\n");
            return;
    }
}


/**
 * @brief 证书写入流程
 * @param cacert_type 证书类型
 * @param data 要写入的证书内容
 * @param len 证书长度
 * @return BaseType_t pdTRUE-成功入队 pdFALSE-队列已满
 */
BaseType_t start_cacert_config(CACERT_Type_t cacert_type, uint8_t *data, uint16_t len)
{
    BaseType_t result = pdFALSE;

    if (data == NULL || len == 0) {
        log_debug("[CAT1][ERR] Invalid certificate data: data=%p, len=%d\r\n", data, len);
        return pdFALSE;
    }

    // 设置当前证书写入状态
    current_cert_type = cacert_type;
    expected_cert_len = len;
    cert_write_state = CERT_STATE_WRITING;
	
//	memset(cacert_data.cacertData, 0, len);
//	memcpy(cacert_data.cacertData, data, len);
//	cacert_data.cacertLength = len;
	
	lte_at_type_t upload_cmd;
    switch(cacert_type) {
        case CACERT_CA:
            upload_cmd = LTE_QFUPL_CACERT;
            break;
        case CACERT_CLIENT:
            upload_cmd = LTE_QFUPL_CLIENT;
            break;
        case CACERT_USERKEY:
            upload_cmd = LTE_QFUPL_USERKEY;
            break;
        default:
            log_debug("[CAT1][ERR] Unknown certificate type: %d\r\n", cacert_type);
            cert_write_state = CERT_STATE_ERROR;
            return pdFALSE;
    }
	
	lte_check_cmd_rtos(LTE_ATE0, "OK\r\n", 1, 500, 0);
	if (lte_check_cmd_rtos(upload_cmd, "CONNECT", 3, 5000, 0) != pdTRUE) {
        cert_write_state = CERT_STATE_ERROR;
        return pdFALSE;
    }

    // 发送写入命令
    lte_at_type_t upload_data;
       switch(cacert_type) {
        case CACERT_CA:
            upload_data = LTE_QFUPL_CACERT_DATA;
            break;
        case CACERT_CLIENT:
            upload_data = LTE_QFUPL_CLIENT_DATA;
            break;
        case CACERT_USERKEY:
            upload_data = LTE_QFUPL_USERKEY_DATA;
            break;
		default:
            log_debug("[CAT1][ERR] Unknown certificate qfupl: %d\r\n", cacert_type);
            cert_write_state = CERT_STATE_ERROR;
            return pdFALSE;
    }
	   
    if (lte_check_cmd_rtos(upload_data, "+QFUPL", 3, 10000, 0) != pdTRUE) {
        cert_write_state = CERT_STATE_ERROR;
        return pdFALSE;
    }

    cert_write_state = CERT_STATE_WAITING_RESPONSE;
    result = pdTRUE;
    return result;
}

/**
 * @brief product_mqtt_publish
 * @param data 产测发布的测试消息
 * @param len 数据长度
 * @return BaseType_t pdTRUE-成功入队 pdFALSE-队列已满
 */
BaseType_t product_mqtt_publish(void)
{
    BaseType_t result = pdFALSE;
    
    const char *test_data = "PRODUCT TEST";
    size_t test_data_len = strlen(test_data);
    
    // 将测试数据拷贝到响应缓冲区
    memcpy(response_mqtt_data, test_data, test_data_len);
    response_mqtt_data_len = test_data_len;

    if (lte_check_cmd_rtos(LTE_MQTT_PRODUCT_PUB, ">", 1, 3000, 0) != pdTRUE)
    {
        return pdFALSE;
    }
    else
    {
        result = pdTRUE;
    }
    
    if (lte_check_cmd_rtos(LTE_MQTT_PRODUCT_PUBMESSAGEDATA, "OK\r\n", 1, 3000, 0) != pdTRUE)
    {
        return pdFALSE;
    }
    else
    {
        result = pdTRUE;
    }
    
    return result;
}

/**
 * @brief mqtt_publish
 * @param data 要发送的数据
 * @param len 数据长度
 * @return BaseType_t pdTRUE-成功入队 pdFALSE-队列已满
 */
BaseType_t mqtt_publish(unsigned char *data, unsigned int len)
{
    BaseType_t result = pdFALSE;  // 改为初始化为 FALSE
    mqtt_function_t topic = FUNC_UNKNOWN;
    unsigned char *actual_data;
    unsigned int actual_len;
    
    // 存储MQTT要发送的数据包
    if (data == NULL)
    {
        return pdFALSE;
    }

    // 标记发送中, 防止第二个 mqtt_publish() 覆盖全局 buffer
    g_mqtt_sending = 1;
    mqtt_send_start_tick = osKernelGetTickCount();
    queue_entry_tick = mqtt_send_start_tick; 
	memcpy(&topic, data, sizeof(mqtt_function_t));
	
	actual_data = data + sizeof(mqtt_function_t);
	actual_len = len - sizeof(mqtt_function_t);
	
	if(topic == FUNC_AUTO_DEVICE_STATE)
	{
		if (build_auto_topic(topic, report_topic, sizeof(report_topic)) < 0) {
			goto PUBLISH_FAIL;
		}
	}
	else
	{
		if (build_report_topic(topic, get_device_sn(), report_topic, sizeof(report_topic)) < 0) {
			goto PUBLISH_FAIL;
		}
	}

	// 只先设置长度（AT命令需要len参数），payload在收到">"提示符后再拷贝
	// 这样消除 osDelay+等">" 期间 response_mqtt_data 被覆盖导致topic+payload串包的隐患
    response_mqtt_data_len = actual_len;
    // DTR设置
    drv_gpio_write(OM_GPIO0, GPIO_MASK(PAD_CAT1_DTR), GPIO_LEVEL_LOW);

    osDelay(osMS2TicksRound(200));
    
    // 发送发布命令（topic + length）
    if (lte_check_cmd_rtos(LTE_MQTT_PUB, ">", 5, 3000, 0) != pdTRUE)
    {
        goto PUBLISH_FAIL;
    }
    
    // 收到">"提示符后才拷贝payload，确保topic和payload来自同一条消息
    // 此时 data 指针仍有效（mqtt_publish返回后才DEMO_BT_Free）
    memcpy(response_mqtt_data, actual_data, actual_len);
    
    // 发送实际数据
    if (lte_check_cmd_rtos(LTE_MQTT_PUBMESSAGEDATA, "OK\r\n", 5, 3000, 0) != pdTRUE)
    {
        goto PUBLISH_FAIL;
    }
    
    // 删除设备回包发送成功后，启动3秒延时定时器
    if (topic == FUNC_CONFIG_DEVICEDELTE)
    {
        if (deleteResponseTimer_ID) {
            if (osTimerIsRunning(deleteResponseTimer_ID))
                osTimerStop(deleteResponseTimer_ID);
            osTimerStart(deleteResponseTimer_ID, osMS2TicksRound(3000));
            log_debug("[CAT1][STA] Delete, start 3s delay\r\n");
        }
    }
    
    result = pdTRUE;
    // 成功: g_mqtt_sending 由 LTE_MQTT_PUBMESSAGEDATA 回调自动清零
    return result;

PUBLISH_FAIL:
    // 入队失败时重置标志, 防止永久卡死
    g_mqtt_sending = 0;
    mqtt_send_start_tick = 0;
    return pdFALSE;
}


// 从主题中提取功能类型
 mqtt_function_t extract_function_from_topic(const char* topic) {
    if (!topic) {
        return FUNC_UNKNOWN;
    }

    // 验证基本格式
    if (strncmp(topic, "pet/collar/", 11) != 0) {
        log_debug("[CAT1][STA] topic err\r\n");
        return FUNC_UNKNOWN;
    }

    // 检查是否是测试主题（包含/report/路径）
    const char* report_ptr = strstr(topic, "/report/");
    if (report_ptr) {
        report_ptr += strlen("/report/"); 
        
        // 检查是否是productTest
        if (strstr(report_ptr, "productTest") != NULL) {
            return FUNC_PRODUCT_TEST;
        }
        return FUNC_UNKNOWN;
    }

    // 正常主题：查找cmd/部分
    const char* cmd_ptr = strstr(topic, "/cmd/");
    if (!cmd_ptr) {
        return FUNC_UNKNOWN;
    }

    cmd_ptr += strlen("/cmd/"); // 移动到命令部分开始

    // 遍历所有映射进行匹配
    for (int i = 0; i < FUNC_MAPPING_COUNT; i++) {
        if (strcmp(cmd_ptr, func_mappings[i].cmd_suffix) == 0) {
            return (mqtt_function_t)i;
        }
    }

    return FUNC_UNKNOWN;
}

// 从主题中提取设备SN
 int extract_sn_from_topic(const char* topic, char* sn_buf, size_t buf_size) {
     if (!topic || !sn_buf) return -1;

     // 主题格式: pet/collar/{sn}/cmd/...
     const char* sn_start = strstr(topic, "pet/collar/");
     if (!sn_start) return -1;

     sn_start += strlen("pet/collar/");
     const char* sn_end = strchr(sn_start, '/');
     if (!sn_end) return -1;

     size_t sn_len = sn_end - sn_start;
     if (sn_len >= buf_size) return -1;

     strncpy(sn_buf, sn_start, sn_len);
     sn_buf[sn_len] = '\0';

     return 0;
 }

 void send_to_uartTask(TASK_CMD_T command)
{
	TaskInfo_t *cat1_uart_task_info = GetTaskInfo(UART_DATARECV_ID);
	
	safe_unblock_uart_task();
	
	// START UART1
	Message_t send_start_msg = {
		.source_id = CAT1_UART_TASK_ID,
		.dest_id = UART_DATARECV_ID,
		.command = command,
	};
	if (osOK != osMessageQueuePut(cat1_uart_task_info->queue_handle, &send_start_msg, NULL, 0))
	{
		LOG_LOC();
	}
}

 /**
 * @brief  cat1_send_reply_entry_task	cat1 消息队列发送函数
 * @param data 数据
 * @param length 数据长度
 * @return BaseType_t pdTRUE-成功入队 pdFALSE-队列已满
 **/
BaseType_t cat1_send_reply_task(TASK_ID_T dest_task_id, TASK_CMD_T command)
{
	TaskInfo_t *my_task_info = GetTaskInfo(CAT1_UART_TASK_ID);
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

BaseType_t production_cat1_send_errorcode_task(TASK_ID_T dest_task_id, TASK_CMD_T command, uint8_t errorcode)
{
	TaskInfo_t *my_task_info = GetTaskInfo(CAT1_UART_TASK_ID);
	TaskInfo_t *dest_task_info = GetTaskInfo(dest_task_id);
	static uint8_t temp;
	temp = errorcode;
	
	if (!dest_task_info || !dest_task_info->queue_handle)
	{
		return pdFALSE;
	}
	Message_t msg = {
		.source_id = my_task_info->task_id,
		.dest_id = dest_task_id,
		.data = &temp,
		.command = command
	};
	return osMessageQueuePut(dest_task_info->queue_handle, &msg, NULL, 0) == osOK;
}

// 构建上报主题
 int build_report_topic(mqtt_function_t func_type, const char* sn, char* topic_buf, size_t buf_size) {
     if (func_type >= FUNC_MAPPING_COUNT || !sn || !topic_buf) return -1;
     const char* report_suffix = func_mappings[func_type].report_suffix;
     return snprintf(topic_buf, buf_size, "pet/collar/%s/report/%s", sn, report_suffix);
 }

 int product_build_subscribe_topic(char* topic_buf, size_t buf_size) {
	memset(topic_buf, 0, buf_size);
    const char* sn = get_device_sn();

    return snprintf(topic_buf, buf_size, "pet/collar/%s/report/report/productTest", sn);
}

 int build_auto_topic(mqtt_function_t func_type, char* topic_buf, size_t buf_size) {
	if (func_type >= FUNC_MAPPING_COUNT || !topic_buf) return -1;
     const char* report_suffix = func_mappings[func_type].report_suffix;
    const char* sn = get_device_sn();

    return snprintf(topic_buf, buf_size, "pet/collar/%s/report/%s", sn, report_suffix);
}
 
/**
 * @brief  send_json_to_comm_task	cat1 消息队列发送函数
 * @param data 数据
 * @param length 数据长度
 * @return BaseType_t pdTRUE-成功入队 pdFALSE-队列已满
 **/
BaseType_t send_data_to_comm_task(TASK_ID_T dest_task_id, TASK_CMD_T command, uint8_t *data, uint16_t length)
{
	TaskInfo_t *my_task_info = GetTaskInfo(CAT1_UART_TASK_ID);
	TaskInfo_t *comm_task_info = GetTaskInfo(dest_task_id);
	if (!comm_task_info || !comm_task_info->queue_handle)
	{
		return pdFALSE;
	}
	uint8_t *data_copy = DEMO_BT_Malloc(length);
	if (!data_copy)
	{
		return pdFALSE;
	}
	memcpy(data_copy, data, length);
	Message_t msg = {
		.source_id = my_task_info->task_id,
		.dest_id = dest_task_id,
		.command = command,
		.data = data_copy,
		.data_length = length};
	return osMessageQueuePut(comm_task_info->queue_handle, &msg, NULL, 0) == osOK;
}

BaseType_t send_data_to_test_task(TASK_ID_T dest_task_id, TASK_CMD_T command, uint8_t *data, uint16_t length)
{
	TaskInfo_t *my_task_info = GetTaskInfo(CAT1_UART_TASK_ID);
	TaskInfo_t *comm_task_info = GetTaskInfo(dest_task_id);
	if (!comm_task_info || !comm_task_info->queue_handle)
	{
		return pdFALSE;
	}
	uint8_t *data_copy = DEMO_BT_Malloc(length);
	if (!data_copy)
	{
		return pdFALSE;
	}
	memcpy(data_copy, data, length);
	data_copy[length] = '\0'; // 添加null终止符

	Message_t msg = {
		.source_id = my_task_info->task_id,
		.dest_id = dest_task_id,
		.command = command,
		.data = data_copy,
		.data_length = strlen((char *)data_copy)};
	return osMessageQueuePut(comm_task_info->queue_handle, &msg, NULL, 0) == osOK;
}

/**
 * @brief 发送WiFi扫描数据到通信任务
 * @param wifi_data WiFi扫描数据
 */
void send_wifi_scan_data_to_comm_task(DeviceWifiSsid_t *wifi_data)
{
    if (wifi_data == NULL)
    {
        return;
    }

//    // 调试打印发送的数据
//    for (int i = 0; i < wifi_data->wifi_count && i < 10; i++)
//    {
//        log_debug("Send WiFi %d: SSID='%s', MAC='%s', RSSI='%s'\n", 
//                 i + 1,
//                 wifi_data->wifi_list[i].ssid,
//                 wifi_data->wifi_list[i].mac,
//                 wifi_data->wifi_list[i].rssi);
//    }
    
    // 使用您的发送函数
    BaseType_t result = send_data_to_comm_task(COMM_TASK_ID, TASK_START_WIFISCAN, 
                                              (uint8_t *)wifi_data, 
                                              sizeof(DeviceWifiSsid_t));
    
    if (result != pdTRUE)
    {
        log_debug("[CAT1][ERR] Failed to send WiFi —> comm\r\n");
    }
}

/**
 * @brief lteExitsleepDtrLow		DTR拉低退出睡眠
 */
void lteExitsleepDtrLow(void)
{
	// DTR设置
	drv_gpio_write(OM_GPIO0, GPIO_MASK(PAD_CAT1_DTR), GPIO_LEVEL_LOW);
	osDelay(osMS2TicksRound(100));
}

void lteAt(void)
{
	// DTR设置
	lteExitsleepDtrLow();
	func_lte_type(LTE_TEST);
	
}

/**
 * @brief  lteInit		cat1初始化
 * @return NULL
 **/
void lteInit(void)
{
//	drv_gpio_write(OM_GPIO0, GPIO_MASK(PAD_CAT1_POWER), GPIO_LEVEL_HIGH);		//23 LTE电源拉高开机 200ms后拉低
//	osDelay(osMS2TicksRound(200));
//	//拉低
	//lteExitsleepDtrLow();
	drv_gpio_write(OM_GPIO0, GPIO_MASK(PAD_CAT1_POWER), GPIO_LEVEL_LOW);
	osDelay(osMS2TicksRound(100));
	drv_gpio_write(OM_GPIO1, GPIO_MASK(PAD_CAT1_POWERKEY - 32), GPIO_LEVEL_HIGH);	//powerkey 先拉拉高再拉低
	osDelay(osMS2TicksRound(CAT1_POWER_VCC_TIME));
	drv_gpio_write(OM_GPIO1, GPIO_MASK(PAD_CAT1_POWERKEY-32), GPIO_LEVEL_LOW);
	log_debug("[CAT1][STA] lteInit success\r\n");
}

/**
 * @brief lteShutdown		cat1关机
 * @return NULL
 */
void lteShutdown(void)
{
//	drv_gpio_write(OM_GPIO1, GPIO_MASK(PAD_CAT1_POWERKEY - 32), GPIO_LEVEL_HIGH);	//powerkey 先拉拉高再拉低
//	osDelay(osMS2TicksRound(CAT1_POWER_VCC_TIME));
//	drv_gpio_write(OM_GPIO1, GPIO_MASK(PAD_CAT1_POWERKEY-32), GPIO_LEVEL_LOW);
	
	lteExitsleepDtrLow();
	// 指令关机
	lte_check_cmd_rtos(LTE_CFUN_SHUTDOWN, "OK\r\n", 1, 500, 1);
}

/**
 * @brief lteEnterSleep		进入sleep模式
 * @return NULL
 */
void lteEnterSleep(void)
{
	// 进入睡眠
	drv_gpio_write(OM_GPIO0, GPIO_MASK(PAD_CAT1_DTR), GPIO_LEVEL_HIGH);
	osDelay(osMS2TicksRound(1000));
	// 指令进入
	lte_check_cmd_rtos(LTE_ENTER_SLEEP, "OK\r\n", 2, 1000, 0);
}

/**
 * @brief lteEntersleepDtrHigh		DTR拉高准备进入sleep（幂等，2秒内重复调用跳过）
 */
void lteEntersleepDtrHigh(void)
{
//	static uint32_t last_restore_tick = 0;
	
	if(Cat1RecvMode != MODE_STANDARD || 
		production_flag.flag_set_cacert || 
		production_flag.flag_set_client || 
		production_flag.flag_set_userkey || 
		production_flag.flag_cat1_conn || 
		production_flag.flag_factory_reset ||
		production_flag.flag_get_version||
		production_flag.flag_set_lte_updata || 
		production_flag.flag_set_lte_usb_updata ||
		production_flag.flag_set_sn)
		return;
	
//	uint32_t now = osKernelGetTickCount();
//	// 2秒内已执行过DTR恢复，跳过（MQTT send_done_handler和主循环队列清空可能连续触发）
//	if ((now - last_restore_tick) < osMS2TicksRound(2000))
//		return;
//	
	osDelay(osMS2TicksRound(1000));
	// DTR设置
	drv_gpio_write(OM_GPIO0, GPIO_MASK(PAD_CAT1_DTR), GPIO_LEVEL_HIGH);
//	last_restore_tick = osKernelGetTickCount();
}

/**
 * @brief lteReboot		重启CAT1
 * @return NULL
 */
void lteReboot(void)
{
	// 指令重启
	lte_check_cmd_rtos(LTE_CFUN_REBOOT, "OK\r\n", 1, 1000, 0);
}

/**
 * @brief lteMqttDisConn		MQTT断开连接
 * @return NULL
 */
void lteMqttDisConn(void)
{
	// 判断网络连接状态
	if (isMqttConnected)
	{
		lte_check_cmd_rtos(LTE_MQTT_DISCONN, "OK\r\n", 5, 3000, 1);
	}
}

void lteWriteCacert(void)
{
	lte_check_cmd_rtos(LTE_QFUPL_CACERT, "OK\r\n", 1, 500, 0);
	lte_check_cmd_rtos(LTE_QFUPL_CLIENT, "OK\r\n", 1, 500, 0);
	lte_check_cmd_rtos(LTE_QFUPL_USERKEY, "OK\r\n", 1, 500, 0);
}

void lteQsslCfg(void)
{
#if MQTT_CONN_PASSWORD
	// 建立MQTT连接
	lte_check_cmd_rtos(LTE_MQTT_OPEN, "OK\r\n", 3, 3000, 1);
	lte_check_cmd_rtos(LTE_MQTT_CONN, "OK\r\n", 3, 3000, 1);
#else
	if(is_old_firmware())
	{
		lte_check_cmd_rtos(LTE_QMTCFG_SSL, "OK\r\n", 1, 500, 0);
//		lte_check_cmd_rtos(LTE_QMTCFG_VERSION, "OK\r\n", 1, 500, 0);
	
//		lte_check_cmd_rtos(LTE_QFLST, "OK\r\n", 1, 3000, 0);
	
		lte_check_cmd_rtos(LTE_QSSLCFG_CACERT, "OK\r\n", 1, 1000, 0);
		lte_check_cmd_rtos(LTE_QSSLCFG_CLIENTCERT, "OK\r\n", 1, 1000, 0);
		lte_check_cmd_rtos(LTE_QSSLCFG_CLIENTKEY, "OK\r\n", 1, 1000, 0);
		lte_check_cmd_rtos(LTE_QSSLCFG_SECLEVEL, "OK\r\n", 1, 1000, 0);
		lte_check_cmd_rtos(LTE_QSSLCFG_SSLVERSION, "OK\r\n", 1, 1000, 0);
		lte_check_cmd_rtos(LTE_QSSLCFG_CIPHERSUITE, "OK\r\n", 1, 1000, 0);
//		lte_check_cmd_rtos(LTE_QSSLCFG_IGNORETIME, "OK\r\n", 1, 1000, 0);
	}
	else
	{
		lte_check_cmd_rtos(LTE_QMTCFG_SSL, "OK\r\n", 1, 500, 0);
		lte_check_cmd_rtos(LTE_QMTCFG_VERSION, "OK\r\n", 1, 1000, 0);

		lte_check_cmd_rtos(LTE_QSSLCFG_SECLEVEL, "OK\r\n", 1, 1000, 0);
		lte_check_cmd_rtos(LTE_QSSLCFG_SSLVERSION, "OK\r\n", 1, 1000, 0);
		lte_check_cmd_rtos(LTE_QSSLCFG_CIPHERSUITE, "OK\r\n", 1, 1000, 0);
		lte_check_cmd_rtos(LTE_QSSLCFG_SNI, "OK\r\n", 1, 1000, 0);
		lte_check_cmd_rtos(LTE_QSSLCFG_IGNOREMULTICA, "OK\r\n", 1, 1000, 0);
		lte_check_cmd_rtos(LTE_QSSLCFG_IGNOREINVALIDCA, "OK\r\n", 1, 1000, 0);
		
		lte_check_cmd_rtos(LTE_QSSLCFG_CACERT, "OK\r\n", 1, 1000, 0);
		lte_check_cmd_rtos(LTE_QSSLCFG_CLIENTCERT, "OK\r\n", 1, 1000, 0);
		lte_check_cmd_rtos(LTE_QSSLCFG_CLIENTKEY, "OK\r\n", 1, 1000, 0);
	}
	
	osMutexAcquire(ReconnectMutex, osWaitForever);
	if (is_reconnecting) {
		is_reconnecting = false;
	}
	osMutexRelease(ReconnectMutex);
	
	// 建立MQTT连接
	lte_check_cmd_rtos(LTE_MQTT_OPEN, "OK\r\n", 3, 3000, 1);
	lte_check_cmd_rtos(LTE_MQTT_CONN, "OK\r\n", 3, 3000, 1);
	
#endif
}

void testLteQsslCfg(void)
{
	lteQsslCfg();
	
	// 订阅测试主题
	lte_check_cmd_rtos(LTE_MQTT_SUB_PRODUCT_TEST, "OK\r\n", 1, 1000, 1);
	
	osDelay(osMS2TicksRound(2000));
	//发布测试消息
	product_mqtt_publish();
}

/**
 * @brief 查询设备所有状态
 * @return NULL
 **/
void lteQueryDeviceInfo(void)
{
//    // DTR设置
//	lteExitsleepDtrLow();
	lte_check_cmd_rtos(LTE_CSQ, "OK\r\n", 10, 1000, 1);
	lte_check_cmd_rtos(LTE_GET_COPS, "OK\r\n", 5, 3000, 0);
    lte_check_cmd_rtos(LTE_QENG_SERVE, "OK\r\n", 5, 3000, 0);
}

// 订阅主题
void lteSubTopic(void)
{
	// DTR设置
	lteExitsleepDtrLow();
	
	lte_check_cmd_rtos(LTE_MQTT_SUB_CHECK, "OK\r\n", 5, 1000, 1);
//	lte_check_cmd_rtos(LTE_MQTT_SUB_AUDIOREALTIME, "OK\r\n", 5, 1000, 1);
	lte_check_cmd_rtos(LTE_MQTT_SUB_AUDIOCMD, "OK\r\n", 5, 1000, 1);
	lte_check_cmd_rtos(LTE_MQTT_SUB_VIBRATE, "OK\r\n", 5, 1000, 1);
	lte_check_cmd_rtos(LTE_MQTT_SUB_DEVICEDELTE, "OK\r\n", 5, 1000, 1);
	lte_check_cmd_rtos(LTE_MQTT_SUB_WARNINGSTATE, "OK\r\n", 5, 1000, 1);
	lte_check_cmd_rtos(LTE_MQTT_SUB_DEVICEMODE, "OK\r\n", 5, 1000, 1);
	lte_check_cmd_rtos(LTE_MQTT_SUB_ENODEB, "OK\r\n", 5, 1000, 1);
	lte_check_cmd_rtos(LTE_MQTT_SUB_DEVICESTATUE, "OK\r\n", 5, 1000, 1);
	/* V1.6 新增主题订阅 */
	lte_check_cmd_rtos(LTE_MQTT_SUB_TIMESYNC, "OK\r\n", 5, 1000, 1);
	lte_check_cmd_rtos(LTE_MQTT_SUB_WIFIMAC, "OK\r\n", 5, 1000, 1);
	lte_check_cmd_rtos(LTE_MQTT_SUB_FENCE, "OK\r\n", 5, 1000, 1);
	lte_check_cmd_rtos(LTE_MQTT_SUB_VOICE, "OK\r\n", 5, 1000, 1);
	lte_check_cmd_rtos(LTE_MQTT_SUB_LIGHT, "OK\r\n", 5, 1000, 1);
	
	lteEnterSleep();
	
	/* ---- 开机高频上报：已迁移到COMM任务，CAT1不再启动主动上报定时器 ---- */
	// if (autoReportTimer_ID) {
	// 	if (osTimerIsRunning(autoReportTimer_ID)) {
	// 		osTimerStop(autoReportTimer_ID);
	// 	}
	// 	boot_high_freq_active = true;
	// 	boot_start_tick = osKernelGetTickCount();
	// 	osTimerStart(autoReportTimer_ID, BOOT_HIGH_FREQ_INTERVAL_MS);
	// 	log_debug("[CAT1][STA] high-freq report started (15s x 2min)\r\n");
	// }
}

void reStartModeQiCgact(void)
{
	// DTR设置
	lteExitsleepDtrLow();
	lte_check_cmd_rtos(LTE_CREG, "OK\r\n", 300, 1000, 0);//用CEREG查询
	lteQueryDeviceInfo();
	
	//激活PDP上下文
//	lte_check_cmd_rtos(LTE_QIACT, "OK\r\n", 30, 3000, 0);
	lte_check_cmd_rtos(LTE_MQTT_PDP, "OK\r\n", 30, 3000, 0);
	lte_check_cmd_rtos(LTE_MQTT_SESSION, "OK\r\n", 1, 500, 0);
	// 设置心跳包
	lte_check_cmd_rtos(LTE_KEEPALIVE, "OK\r\n", 1, 1000, 0);
	//配置ssl
	lteQsslCfg();
}

/**
 * @brief cat1_escape_pubdata_mode  让模组退出QMTPUBEX">"后的payload数据接收态
 * @note  AT+QMTPUBEX发出后模组回">"并等待length字节payload
 */
static void cat1_escape_pubdata_mode(void)
{
	uint16_t pending = g_pubdata_pending_len;

	if (pending == 0 || pending > sizeof(response_mqtt_data)) {
		g_pubdata_pending_len = 0;
		return;
	}

	log_debug("[CAT1][STA] module stuck in PUB data-wait, complete publish with payload (%d bytes)\r\n", pending);

	/* 补发真实payload + 行结束符：模组收满length字节结束QMTPUBEX */
	drv_uart_write(CAT1_AT_UART, (uint8_t *)response_mqtt_data, pending, 10);
	drv_uart_write(CAT1_AT_UART, (const uint8_t *)"\r\n", 2, 10);
	g_pubdata_pending_len = 0;

	/* 等模组消化完这条publish并输出OK/ERROR，回到命令态 */
	osDelay(osMS2TicksRound(300));

	/* 丢弃padding触发的OK/ERROR响应 */
	clearPacketList();
}

/**
 * @brief mqtt_disconnect_flush_queue  MQTT断连后、重新发起重连前清空遗留AT状态
 * @note  
 */
static void mqtt_disconnect_flush_queue(void)
{
	log_debug("[CAT1][STA] MQTT lost, flush stale AT cmds before reconnect\r\n");

	cat1_escape_pubdata_mode();

	/* 丢弃未发送的AT指令并中止currentCmd重试循环（PUB/订阅等） */
	osMutexAcquire(LteMutex, osWaitForever);
	currentCmd = NULL;
	lteCmdQueueHead = lteCmdQueueTail = 0;
	osMutexRelease(LteMutex);

	/* 丢弃串口已收到但未处理的残留回包（含断连前已发出指令的ERROR） */
	clearPacketList();

	/* 复位MQTT发送状态，防止g_mqtt_sending卡住阻塞后续上报 */
	if (g_mqtt_queue) {
		mqtt_queue_clear(g_mqtt_queue);
	}
	g_mqtt_sending = 0;
	g_current_send_type = 0;
	g_skip_pubmsgd = false;
	mqtt_send_start_tick = 0;
	queue_entry_tick = 0;
	is_retransmitting = false;
	/* PUB数据接收态已被escape处理；此处兜底清零，防止状态残留 */
	g_pubdata_pending_len = 0;
}

void reStartModeConn(void)
{
	// 防止重复进入
    osMutexAcquire(ReconnectMutex, osWaitForever);
    if (is_reconnecting) {
        osMutexRelease(ReconnectMutex);
        return;
    }
    is_reconnecting = true;
    osMutexRelease(ReconnectMutex);
	
	// DTR设置
	lteExitsleepDtrLow();
	lte_check_cmd_rtos(LTE_ATE0, "OK\r\n", 1, 500, 0);
	lte_check_cmd_rtos(LTE_CREG, "OK\r\n", 300, 1000, 0);
	lte_check_cmd_rtos(LTE_GET_CGACT, "OK\r\n", 2, 1000, 1);
}

/**
 * @brief lteHighspeedModeConn		MQTT连接模式
 * @return NULL
 */
void lteHighspeedModeConn(void)
{
	lte_check_cmd_rtos(LTE_TEST, "OK\r\n", 1, 500, 0);
	lte_check_cmd_rtos(LTE_ATE0, "OK\r\n", 1, 500, 0);

	// 设置数据收发格式（HEX）
	//	lte_check_cmd_rtos(LTE_DATAFORMAT, "OK\r\n", 1, 3000);

	lte_check_cmd_rtos(LTE_CPIN, "OK\r\n", 100, 2000, 0);	//读到卡后设置APN
	
	// 设置APN，先查询APN，有则配置
//	lte_check_cmd_rtos(LTE_SET_APN, "OK\r\n", 1, 1000, 0);
	lte_check_cmd_rtos(LTE_GET_APN, "OK\r\n", 1, 1000, 0);
	
	lte_check_cmd_rtos(LTE_QMTCFG_RECV, "OK\r\n", 1, 500, 0);

	
	lte_check_cmd_rtos(LTE_MAIN_RING, "OK\r\n", 1, 500, 0);
//	lte_check_cmd_rtos(LTE_CIMI, "OK\r\n", 1, 2000, 0);
	
//	// 设置心跳包
//	lte_check_cmd_rtos(LTE_KEEPALIVE, "OK\r\n", 1, 1000, 0);
	
//	lte_check_cmd_rtos(LTE_COPS, "OK\r\n", 1, 1000, 0);
//	reStartModeConn();
	lte_check_cmd_rtos(LTE_ATE0, "OK\r\n", 1, 500, 0);
	lte_check_cmd_rtos(LTE_CREG, "OK\r\n", 300, 1000, 0);
	lte_check_cmd_rtos(LTE_GET_CGACT, "OK\r\n", 2, 1000, 1);
}

/**
 * @brief lteTestHighspeedModeConn		LTE产测连接通信
 * @return NULL
 */
void lteTestHighspeedModeConn(void)
{
	lte_check_cmd_rtos(LTE_ATE0, "OK\r\n", 1, 500, 0);

	lte_check_cmd_rtos(LTE_CPIN, "OK\r\n", 100, 2000, 0);
	
	// 设置APN，先查询APN，有则配置
	lte_check_cmd_rtos(LTE_SET_APN, "OK\r\n", 1, 1000, 0);
	lte_check_cmd_rtos(LTE_CREG, "OK\r\n", 300, 1000, 0);
	lte_check_cmd_rtos(LTE_CSQ, "OK\r\n", 10, 1000, 1);
	
	lte_check_cmd_rtos(LTE_SET_CGACT, "OK\r\n", 1, 1000, 0);

	lte_check_cmd_rtos(LTE_QMTCFG_RECV, "OK\r\n", 1, 500, 0);
	lte_check_cmd_rtos(LTE_MAIN_RING, "OK\r\n", 1, 500, 0);

//	lte_check_cmd_rtos(LTE_CREG, "OK\r\n", 300, 1000, 0);
//	lte_check_cmd_rtos(LTE_CSQ, "OK\r\n", 1, 1000, 1);
	
	lte_check_cmd_rtos(LTE_MQTT_PDP, "OK\r\n", 30, 3000, 0);
	
	//配置ssl
	testLteQsslCfg();
}


void lteMqttConnState(void)
{
//	// DTR设置
//	lteExitsleepDtrLow();
	// 查询连接状态
	if(isMqttConnected)
	{
		lte_check_cmd_rtos(LTE_MQTT_ISSTATE, "OK\r\n", 1, 500, 1);
	}
	else
	{
		if (!is_reconnecting) {
			reStartModeConn();
		}
	}
}

/**
 * @brief lteTestDeleteCacert		LTE删除证书
 * @return NULL
 */
void lteTestDeleteCacert(void)
{
	set_certificate_deletion_mode(true);
    
	lte_check_cmd_rtos(LTE_ATE0, "OK\r\n", 1, 500, 0);
    if (lte_check_cmd_rtos(LTE_QFLST, "OK\r\n", 1, 3000, 0) != pdTRUE) {
        set_certificate_deletion_mode(false);
    }
}

/**
 * @brief lteDeviceInfo		查询设备信息
 * @return NULL
 */
void lteDeviceInfo(void)
{
	// DTR设置
	lteExitsleepDtrLow();
	lte_check_cmd_rtos(LTE_DEVICE_GSN, "OK\r\n", 2, 3000, 0);
}

/**
 * @brief lteDeviceInfo		查询SN信息
 * @return NULL
 */
void lteSnInfo(void)
{
	// DTR设置
	lteExitsleepDtrLow();
	lte_check_cmd_rtos(LTE_ATE0, "OK\r\n", 1, 500, 0);
	lte_check_cmd_rtos(LTE_DEVICE_QCCID, "OK\r\n", 2, 3000, 0);
}

/**
 * @brief lteVersion		查询版本号
 * @return NULL
 */
void lteVersion(void)
{
	// DTR设置
	lteExitsleepDtrLow();
	lte_check_cmd_rtos(LTE_ATE0, "OK\r\n", 1, 500, 0);
	
	//设置APN
	lte_check_cmd_rtos(LTE_CPIN, "OK\r\n", 100, 2000, 0);
	lte_check_cmd_rtos(LTE_SET_APN, "OK\r\n", 1, 1000, 0);
	
	lte_check_cmd_rtos(LTE_VERSION, "OK\r\n", 1, 500, 0);
}

bool CELL_Init(void)
{
    if (CellInfoMutex == NULL) {
        CellInfoMutex = osMutexNew(NULL);
    }
    
    memset(&cell_info, 0, sizeof(CellInfo_t));
    
    return (CellInfoMutex != NULL);
}

/**
 * @brief 查询基站信息（服务小区和邻近小区）
 * @return NULL
 **/
void CELL_QueryCellInfo(void)
{
    // DTR设置
//	lteExitsleepDtrLow();
	lte_check_cmd_rtos(LTE_GET_COPS, "OK\r\n", 1, 3000, 0);
    lte_check_cmd_rtos(LTE_QENG_SERVE, "OK\r\n", 1, 3000, 0);
    
//    lte_check_cmd_rtos(LTE_QENG_NEIGHBOUR, "OK\r\n", 1, 3000, 0);
}

/**
 * @brief lteControlWifi	WiFi扫描
 * @return NULL
 */
void lteControlWifi(void)
{
	// DTR设置
	lteExitsleepDtrLow();
	
	// 新一轮 scan
	lte_wifi_scan_reset();
	lte_check_cmd_rtos(LTE_WIFI_SCAN, "OK\r\n", 2, 3000, 0);
}

/**
 * @brief ltePowerDownPrepare		CAT1关机时状态机清空
 * @return NULL
 */
void ltePowerDownPrepare(void)
{
	// 清空指令队列
	currentCmd = NULL;
	osMutexAcquire(LteMutex, osWaitForever);
	if (lteCmdQueueHead != lteCmdQueueTail)
	{
		lteCmdQueueHead = lteCmdQueueTail = 0;
	}
	osMutexRelease(LteMutex);

	// 清空缓冲区数据
	clearPacketList();
	if (g_mqtt_queue) {
        mqtt_queue_clear(g_mqtt_queue);
        g_mqtt_sending = 0;
    }
}

void lteCloseTimer(void)
{
	//3分钟查询设备状态定时器
	if(checkDeviceState_startTimer_ID)
	{
		if(osTimerIsRunning(checkDeviceState_startTimer_ID))
		{
			osTimerStop(checkDeviceState_startTimer_ID);    
		}
	}
	
	//1分钟/30分钟检查MQTT连接状态
	if(checkMqttState_startTimer_ID)
	{
		if(osTimerIsRunning(checkMqttState_startTimer_ID))
		{
			osTimerStop(checkMqttState_startTimer_ID);    
		}
	}
	
	//2分钟高频上报——>10分钟上报
	if(autoReportTimer_ID)
	{
		if(osTimerIsRunning(autoReportTimer_ID))
		{
			osTimerStop(autoReportTimer_ID);    
		}
	}
}


void restartMqttConnectState()
{
	if(isMqttConnected == false)
	{
		lte_check_cmd_rtos(LTE_MQTT_ISSTATE, "OK\r\n", 1, 500, 1);
	}
}

// 添加函数：重置CAT1所有状态
void reset_cat1_state_before_poweron(void)
{
    // 清空指令队列
    currentCmd = NULL;
    osMutexAcquire(LteMutex, osWaitForever);
    if (lteCmdQueueHead != lteCmdQueueTail)
    {
        lteCmdQueueHead = lteCmdQueueTail = 0;
    }
    osMutexRelease(LteMutex);
    
    // 清空缓冲区数据
    clearPacketList();
    
    // 清空MQTT队列
    if (g_mqtt_queue) {
        mqtt_queue_clear(g_mqtt_queue);
        g_mqtt_sending = 0;
    }
    
    // 重置重连标志
	isMqttConnected = false;
    osMutexAcquire(ReconnectMutex, osWaitForever);
    is_reconnecting = false;
    osMutexRelease(ReconnectMutex);
    
	g_mqtt_sending = 0;
	g_current_send_type = 0;
	/* 模组即将重启/已重启，QMTPUBEX数据接收态不复存在，复位跟踪标志 */
	g_pubdata_pending_len = 0;
	// 清除主动上报标志（如果当前完成的是主动上报）
	if (is_auto_reporting) {
		is_auto_reporting = false;
	}
	
//    // 清空事件标志
//    osEventFlagsClear(LteEventId, LTE_EVENT_CMD_READY | LTE_EVENT_RESP_RECEIVED);
}

/**
 * @brief lteEventPowerDown		CAT1任务关闭
 * @return NULL
 */
void lteEventPowerDown(void)
{
	//回复entryreply
	cat1_send_reply_task(ENTRY_TASK_ID, TASK_STOP_REPLY);
	
	reset_cat1_state_before_poweron();
	lteCloseTimer();
	// STOT UART1
	send_to_uartTask(TASK_CMD_STOP);
	osEventFlagsClear(LteEventId, LTE_EVENT_TASK_START);

	// 停止任务
	set_cat1_state(LTE_TASK_STOPPED);
	
	// 设置阻塞标志
    block_cat1_task();
	
	//IO23 拉高关机
	drv_gpio_write(OM_GPIO0, GPIO_MASK(PAD_CAT1_POWER), GPIO_LEVEL_HIGH);
	
	lteCloseTimer();
}

/**
 * @brief lteEntryEventPowerDown		ENTRY CAT1任务关闭
 * @return NULL
 */
void lteEntryEventPowerDown(void)
{
	//回复TEST reply
	cat1_send_reply_task(ENTRY_TASK_ID, TASK_STOP_REPLY);
	// STOT UART1
	send_to_uartTask(TASK_CMD_STOP);
	osEventFlagsClear(LteEventId, LTE_EVENT_TASK_START);

	// 停止任务
	set_cat1_state(LTE_TASK_STOPPED);
	
	// 设置阻塞标志
    block_cat1_task();
	
	//IO23 拉高关机
	drv_gpio_write(OM_GPIO0, GPIO_MASK(PAD_CAT1_POWER), GPIO_LEVEL_HIGH);
	
	//关闭定时器
	lteCloseTimer();
}

/**
 * @brief lteTestEventPowerDown		TEST CAT1任务关闭
 * @return NULL
 */
void lteTestEventPowerDown(void)
{
	//回复TEST reply
	cat1_send_reply_task(TEST_TASK_ID, TASK_STOP_REPLY);
	// STOT UART1
	send_to_uartTask(TASK_CMD_STOP);
	osEventFlagsClear(LteEventId, LTE_EVENT_TASK_START);

	// 停止任务
	set_cat1_state(LTE_TASK_STOPPED);
	
	// 设置阻塞标志
    block_cat1_task();
	
	//IO23 拉高关机
	drv_gpio_write(OM_GPIO0, GPIO_MASK(PAD_CAT1_POWER), GPIO_LEVEL_HIGH);
	
	lteCloseTimer();
}

/**
 * @brief MQTT发送完成处理
 * @param is_cloud_cmd 是否为云指令（1-云指令，0-主动上报）
 */
static void mqtt_send_done_handler(uint8_t is_cloud_cmd)
{
    if (is_cloud_cmd != 2) {
        // 云指令：处理队列中的下一条
        if (g_mqtt_queue && !mqtt_queue_is_empty(g_mqtt_queue)) {
            mqtt_packet_t packet;
            if (mqtt_queue_pop_with_data(g_mqtt_queue, &packet) == 0) {
                // 只转发给COMM, 不设g_mqtt_sending=1
                // COMM组包回传后才调mqtt_publish, 那时由mqtt_publish设g_mqtt_sending=1
                // 否则COMM组包期间g_mqtt_sending=1会堵死所有后续消息
                send_data_to_comm_task(COMM_TASK_ID, TASK_COMM_DATAJSON, 
                                       packet.data, packet.data_len);
                DEMO_BT_Free(packet.data);
                return;
            }
			else
			{
				g_mqtt_sending = 0;
				g_current_send_type = 0;
				lteEntersleepDtrHigh();
			}
        }
		else
		{
			g_mqtt_sending = 0;
			g_current_send_type = 0;
			lteEntersleepDtrHigh();
		}
//        g_mqtt_sending = 0;
//        g_current_send_type = 0;
    } else {
        // 主动上报：只重置状态
        g_mqtt_sending = 0;
        g_current_send_type = 0;
		lteEntersleepDtrHigh();
    }
}
	
/**
 * @brief  at_mqttData		cat1 接收数据处理
 * @param str 数据
 * @param len 数据长度
 * @param status 指令执行状态
 * @return NULL
 **/
void at_mqttData(uint8_t *str, uint16_t len, cmd_status_t *status)
{
    char *ptr = (char *)str;
	uint16_t json_len = 0;
	char local_topic[128] = {0};
	
	//前缀：0,0,
    char *comma = strchr(ptr, ',');
    if (!comma)
    {
        *status = CMD_STATUS_FAILED;
        return;
    }
    *comma = '\0';
    int number1 = atoi(ptr);

    // 解析第二个数字
    ptr = comma + 1;
    comma = strchr(ptr, ',');
    if (!comma)
    {
        *status = CMD_STATUS_FAILED;
        return;
    }
    *comma = '\0';
    int number2 = atoi(ptr);

    // 解析主题
    ptr = comma + 1;
    char *quote_start = strchr(ptr, '"');
    if (!quote_start)
    {
        *status = CMD_STATUS_FAILED;
        return;
    }
    quote_start++;
    char *quote_end = strchr(quote_start, '"');
    if (!quote_end)
    {
        *status = CMD_STATUS_FAILED;
        return;
    }

    // 提取和解析主题
    *quote_end = '\0';
    const char* received_topic = quote_start;

    // 识别功能类型和设备SN
    mqtt_function_t current_func = extract_function_from_topic(received_topic);
    if (current_func == FUNC_UNKNOWN) {
        log_debug("[CAT1][ERR] func unknow\r\n");
        *status = CMD_STATUS_FAILED;
        return;
    }

    // 提取设备SN并存储
    if (extract_sn_from_topic(received_topic, device_sn, sizeof(device_sn)) != 0) {
        *status = CMD_STATUS_FAILED;
        return;
    }

    // 解析JSON数据
    ptr = quote_end + 1;
    quote_start = strchr(ptr, '"');
    if (!quote_start)
    {
        *status = CMD_STATUS_FAILED;
        return;
    }

    quote_end = strrchr(quote_start + 1, '"');
    if (!quote_end)
    {
        *status = CMD_STATUS_FAILED;
        return;
    }
	
    json_len = quote_end - (quote_start + 1);

    // 检查是否为产测命令
    if (production_flag.flag_cat1_conn)
    {
        if (json_len > 0)
        {
            // 获取数据
            char json_data[json_len + 1];
            memcpy(json_data, quote_start + 1, json_len);
            json_data[json_len] = '\0';
            
            if (strstr(json_data, "PRODUCT TEST") != NULL)
            {
				production_cat1_send_errorcode_task(TEST_TASK_ID, TASK_CAT1_AWS_WITHCA_TEST_REPLY,0);
                *status = CMD_STATUS_SUCCESS;
            }
            else
            {
                log_debug("[CAT1][ERR] Product test MQTT failed\r\n");
				production_cat1_send_errorcode_task(TEST_TASK_ID, TASK_CAT1_AWS_WITHCA_TEST_REPLY,1);
                *status = CMD_STATUS_SUCCESS;
            }
			
        }
        else
        {
			production_cat1_send_errorcode_task(TEST_TASK_ID, TASK_CAT1_AWS_WITHCA_TEST_REPLY,1);
            *status = CMD_STATUS_SUCCESS;
        }
        return;
    }

    // 正常流程
    if (json_len > 0)
    {
		if (lte_reboot_count || lte_hw_retry_count || lte_init_pending) {
			lte_recovery_clear();
		}

		if ((CurrentChargeStatusDataGet() == CURRENT_CHARGE_STATUS_CHARGING || CurrentChargeStatusDataGet() == CURRENT_CHARGE_STATUS_LOW_BATTERY))
		{
			log_debug("[CAT1][STA] M3/M4 low-power: 4G downlink dropped\r\n");
			*status = CMD_STATUS_SUCCESS;
			return;
		}

		//添加数据到队列，判断当前状态是否为IDLE，idle则发送并修改状态，否则则添加到队列。在回复包发送后释放状态
//        send_data_to_comm_task(COMM_TASK_ID, TASK_COMM_DATAJSON, (uint8_t *)quote_start + 1, json_len);
//        *status = CMD_STATUS_SUCCESS;
		
//        drv_uart_write(LOG_UART, (uint8_t *)"mqtt:", (uint32_t)5, 10);
//		drv_uart_write(LOG_UART, (uint8_t *)local_topic, (uint32_t)sizeof(local_topic), 10);
        // 分配内存保存数据
        uint8_t *data_copy = DEMO_BT_Malloc(json_len);
        if (!data_copy) {
            log_debug("[CAT1][ERR] MQTT failed to allocate\r\n");
            *status = CMD_STATUS_FAILED;
            return;
        }
        memcpy(data_copy, (uint8_t *)quote_start + 1, json_len);
        log_debug("[CAT1][STA] MQTT g_mqtt_sending=%d, is_auto_reporting=%d\r\n", g_mqtt_sending, is_auto_reporting);
        if (g_mqtt_sending == 0&& is_auto_reporting == false) { // && is_auto_reporting == false
            // 空闲状态，直接发送
            
            if (send_data_to_comm_task(COMM_TASK_ID, TASK_COMM_DATAJSON, data_copy, json_len) == pdTRUE) {
//				log_debug("[CAT1][STA] MQTT idle, cmd sent to COMM\r\n");
                // 不设 g_mqtt_sending=1: 云端指令流控靠 g_mqtt_queue,
                // 设了会阻塞 COMM 的回包被 mqtt_publish() 处理, 导致永久卡死
                DEMO_BT_Free(data_copy);
                *status = CMD_STATUS_SUCCESS;
            } else {
                DEMO_BT_Free(data_copy);
                *status = CMD_STATUS_FAILED;
            }
        } else {
            // 忙碌状态，需要入队
            log_debug("[CAT1][STA] MQTT busy, func=%d, queue count=%d\r\n", current_func, g_mqtt_queue->count);
			
			osMutexAcquire(g_mqtt_queue->mutex, osWaitForever);
            
            // 检索队列中是否有相同主题
            int duplicate_idx = -1;
            for (int i = 0; i < g_mqtt_queue->count; i++) {
                int idx = (g_mqtt_queue->head + i) % MQTT_QUEUE_SIZE;
                mqtt_packet_t *p = &g_mqtt_queue->packets[idx];
                mqtt_function_t func = extract_function_from_topic(p->topic);
				
                if (func == current_func) {
					
                    duplicate_idx = i;
                    break;
                }
            }
            
            // 有相同主题 - 删掉原数据，继续入队
            if (duplicate_idx >= 0) {
                log_debug("[CAT1][STA] Found duplicate index: %d, discarding old\r\n", duplicate_idx);
//                DEMO_BT_Free(data_copy);
//                *status = CMD_STATUS_SUCCESS;
//                osMutexRelease(g_mqtt_queue->mutex);
//                return;
				mqtt_queue_remove_at(g_mqtt_queue, duplicate_idx);
            }
            else
			{
				// 没有相同主题
				log_debug("[CAT1][STA] No duplicate topic\r\n");
            }
            // 检查队列是否已满
            if (g_mqtt_queue->count < MQTT_QUEUE_SIZE) {
                // 队列未满，正常入队
                uint8_t *queue_data = DEMO_BT_Malloc(json_len);
                if (!queue_data) {
                    DEMO_BT_Free(data_copy);
                    osMutexRelease(g_mqtt_queue->mutex);
//                    log_debug("Failed to allocate memory for queue data\n");
                    *status = CMD_STATUS_FAILED;
                    return;
                }
                memcpy(queue_data, data_copy, json_len);
                
                mqtt_packet_t *p = &g_mqtt_queue->packets[g_mqtt_queue->tail];
                p->data = queue_data;
                p->data_len = json_len;
                strncpy(p->topic, received_topic, sizeof(p->topic) - 1);
                p->topic[sizeof(p->topic) - 1] = '\0';

                g_mqtt_queue->tail = (g_mqtt_queue->tail + 1) % MQTT_QUEUE_SIZE;
                g_mqtt_queue->count++;

                *status = CMD_STATUS_SUCCESS;
                osMutexRelease(g_mqtt_queue->mutex);
                DEMO_BT_Free(data_copy);
                return;
            } else {
                // 队列已满且没有重复主题：丢弃新包
                DEMO_BT_Free(data_copy);
                *status = CMD_STATUS_SUCCESS;
                osMutexRelease(g_mqtt_queue->mutex);
                return;
            }
        }
    }
    else
    {
        *status = CMD_STATUS_FAILED;
    }
}


/**
 * @brief  at_conn		cat1 MQTT 连接
 * @param str 数据
 * @param len 数据长度
 * @param status 指令执行状态
 * @return NULL
 **/
void at_conn(uint8_t *str, uint16_t len, cmd_status_t *status)
{
    *status = CMD_STATUS_SUCCESS;
    // 标记已收到+QMTCONN信息行
    qmtconn_info_seen = true;

    char *params = (char *)str;
    int client_idx, param1, param2;

    // 解析参数
    int parsed_count = sscanf(params, "%d,%d,%d", &client_idx, &param1, &param2);

    if (parsed_count == 2)
    {
        // 两个参数：<client_idx>,<state>
        isMqttConnected = (param1 == 3) ? true : false;
    }
    else if (parsed_count >= 2)
    {
        // 三个或更多参数：<client_idx>,<result>[,<ret_code>]
        isMqttConnected = (param2 == 0) ? true : false;
    }
    else
    {
        *status = CMD_STATUS_FAILED;
        return;
    }

		// 连接状态变化处理
		if (isMqttConnected) 
		{
			if (production_flag.flag_cat1_conn) {
				*status = CMD_STATUS_SUCCESS;
				return;
			}
			
			// 网络已连接：正常QMTCONN流程 或 M5热恢复补订阅（定时器检查不触发）
			if(currentCmd->type == LTE_MQTT_CONN || g_need_subscribe_on_reconnect)
			{
				lteSubTopic();
				g_need_subscribe_on_reconnect = false;
			}
		}
		else 
		{
			if (production_flag.flag_cat1_conn) {
				production_cat1_send_errorcode_task(TEST_TASK_ID, TASK_CAT1_AWS_WITHCA_TEST_REPLY,1);
				*status = CMD_STATUS_SUCCESS;
				return;
			}
			 // 连接失败，发起重连（需去重保护）
			if (!is_reconnecting) {
				/* 清掉队列中残留的MQTT_OPEN/CONN等指令，避免重复指令报ERROR堵住重连 */
				mqtt_disconnect_flush_queue();
//				log_debug("[CAT1][STA] MQTT connect restart\r\n");
				reStartModeConn();
			}
		}
}

/**
 * @brief  at_qmtsub		cat1 MQTT主题订阅
 * @param str 数据
 * @param len 数据长度
 * @param status 指令执行状态
 * @return NULL
 **/
void at_qmtsub(uint8_t *str, uint16_t len, cmd_status_t *status)
{
    *status = CMD_STATUS_SUCCESS;
    
	char *params = (char *)str;
    int client_idx, msg_id, result, value;
    int parsed_count;
    
    // 去除 "+QMTSUB: " 前缀
    char *data_start = params;
    if (strncmp(params, "+QMTSUB: ", 8) == 0) {
        data_start = params + 8;
        // 跳过可能存在的空格
        while (*data_start == ' ') data_start++;
    }
    
    // 先尝试解析4个参数（格式：0,1,0,0）
    parsed_count = sscanf(data_start, "%d,%d,%d,%d", &client_idx, &msg_id, &result, &value);
    
    if (parsed_count == 4)
    {
        // 4个参数格式：<client_idx>,<msg_id>,<qos>,<value>
        // 第三个参数 result 表示订阅结果：0-成功，1-重传，2-失败
        if (result == 0)
        {
            // 订阅成功：Packet sent successfully and ACK received from the server
//            log_debug("[CAT1][STA] MQTT subscribe success\r\n");
            *status = CMD_STATUS_SUCCESS;
        }
        else if (result == 2)
        {
            // 失败：Failed to send packet
            log_debug("[CAT1][ERR] MQTT Subscribe FAILED\r\n");
            *status = CMD_STATUS_FAILED;
        }
        else
        {
            // result == 1 或其他值，重传情况，保持成功状态
            log_debug("[CAT1][STA] MQTT Subscribe retransmission, result=%d\r\n", result);
            *status = CMD_STATUS_SUCCESS;
        }
    }
    else
    {
        // 尝试解析3个参数（格式：0,1,2）
        parsed_count = sscanf(data_start, "%d,%d,%d", &client_idx, &msg_id, &result);
        
        if (parsed_count == 3)
        {
            if (result == 2)
            {
                // 不标记失败状态，不重发，在超时时间内继续等待订阅结果
                log_debug("[CAT1][STA] QMTSUB packet send failed (result=2), waiting!\r\n");
                // 不设置失败状态，保持等待
                *status = CMD_STATUS_PENDING;
            }
            else
            {
                // result == 0 或 1，表示成功或重传，保持成功状态继续等待最终确认
                *status = CMD_STATUS_SUCCESS;
            }
        }
        else
        {
            // 其他格式（如查询响应），认为是成功的
//            log_debug("[CAT1][DAT] MQTT Subscribe other: %s\r\n", data_start);
            *status = CMD_STATUS_SUCCESS;
        }
    }
}

void at_qmtuns(uint8_t *str, uint16_t len, cmd_status_t *status)
{
    *status = CMD_STATUS_SUCCESS;
   
}

/**
 * @brief  at_cereg		cat1 网络注册状态
 * @param str 数据
 * @param len 数据长度
 * @param status 指令执行状态
 * @return NULL
 **/
void at_cereg(uint8_t *str, uint16_t len, cmd_status_t *status)
{
	if (str != NULL)
	{
		// 解析字符串，格式为 "0,2"
		// 查找逗号位置
		uint8_t *comma_pos = (uint8_t *)memchr(str, ',', len);

		if (comma_pos != NULL && (comma_pos - str + 1) < len)
		{
			// 获取第二个数字（逗号后的第一个字符）
			uint8_t second_value = *(comma_pos + 1) - '0'; // 将字符转换为数字
			if (second_value == 1 || second_value == 5)
			{
				*status = CMD_STATUS_SUCCESS; // 已注册
			}
			else if (second_value == 2)
			{
				*status = CMD_STATUS_FAILED; // 需要重试
			}
			else
			{
				*status = CMD_STATUS_FAILED; // 注册失败
			}
		}
		else
		{
			*status = CMD_STATUS_FAILED;
		}
	}
	else
	{
		*status = CMD_STATUS_FAILED;
	}
	osDelay(osMS2TicksRound(100));
}

/**
 * @brief  at_cgatt		cat1 PDP处理
 * @param str 数据
 * @param len 数据长度
 * @param status 指令执行状态
 * @return NULL
 **/
void at_cgatt(uint8_t *str, uint16_t len, cmd_status_t *status)
{
	*status = CMD_STATUS_SUCCESS;
}

/**
 * @brief  at_cpin		cat1 sim卡状态
 * @param str 数据
 * @param len 数据长度
 * @param status 指令执行状态
 * @return NULL
 **/
void at_cpin(uint8_t *str, uint16_t len, cmd_status_t *status)
{
	if (str != NULL)
	{
		if (strnstr((char *)str, "READY", len) != NULL)
		{
			*status = CMD_STATUS_SUCCESS;
		}
		else
		{
			*status = CMD_STATUS_FAILED;
			log_debug("[CAT1][ERR] SIM card not ready\r\n");
		}
	}
	else
	{
		*status = CMD_STATUS_FAILED;
	}
}

// 更新CSQ缓存
static void update_csq_cache(uint8_t csq_value)
{
    if (g_cache_mutex == NULL) return;
    
    osMutexAcquire(g_cache_mutex, osWaitForever);
    g_device_cache.csq.value = csq_value;
    g_device_cache.csq.update_time = osKernelGetTickCount();
    osMutexRelease(g_cache_mutex);
}

/**
 * @brief  at_csq		cat1 信号质量处理
 * @param str 数据
 * @param len 数据长度
 * @param status 指令执行状态
 * @return NULL
 **/
void at_csq(uint8_t *str, uint16_t len, cmd_status_t *status)
{
	if (str == NULL)
	{
		*status = CMD_STATUS_FAILED;
		return;
	}
	int rssi, ber;
	
	if (sscanf((char *)str, "%d,%d", &rssi, &ber) == 2)
	{
//		cat1_Status.cat1_signal = rssi;
//		cat1_Status.cat1_signal_status = SIGNAL_IS_DETECTABLE;
//		log_debug("Signal successful: RSSI=%d\n", cat1_Status.cat1_signal);
		
		// 直接存储到缓存
		if(rssi == 99 || rssi == 199)
		{
			*status = CMD_STATUS_FAILED;
		}
		else
		{
			update_csq_cache(rssi);
			*status = CMD_STATUS_SUCCESS;
		}
		
//		//当前指令是查询设备状态
//		if(flag_check_device_status == true)
//		{
//			flag_check_device_status = false;
//			cat1_send_reply_task(COMM_TASK_ID, TASK_DATA_VAL_UPDATA);
//		}
	}
	else
	{
		*status = CMD_STATUS_FAILED;
	}
}

// 更新基站信息缓存
static void update_cell_info_cache(CellInfo_t *info)
{
    if (g_cache_mutex == NULL) return;
    
    osMutexAcquire(g_cache_mutex, osWaitForever);
    memcpy(&g_device_cache.cell_info.info, info, sizeof(CellInfo_t));
    g_device_cache.cell_info.update_time = osKernelGetTickCount();
    osMutexRelease(g_cache_mutex);
}

/**
 * @brief  解析服务小区信息
 * @param  params: 要解析的参数字符串
 * @return 成功解析的字段数
 * 
 * 模块返回格式：
 * "servingcell","NOCONN","LTE","FDD",460,11,3FD5413,206,2452,5,3,3,4041,-106,-7,-81,12,18
 */
static int parse_serving_cell(char *params)
{
    char state[16] = {0}; 
    char rat[8] = {0};
    char duplex[8] = {0};
    int mcc, mnc;
    char cell_id_str[20] = {0};  // 直接存储十六进制字符串
    int earfcn, band, ul_bw, dl_bw;  // ul_bw, dl_bw不上报
	char pcid_str[20] = {0};
	char tac_str[20] = {0};
    int rsrp, rsrq, rssi, sinr, s_rxlev;  // s_rxlev不上报

    char *data_start = strchr(params, ',');
    if (!data_start) {
        return 0;
    }
    data_start++;

    int parsed = sscanf(data_start, 
                        "\"%15[^\"]\",\"%7[^\"]\",\"%7[^\"]\",%d,%d,%[^,],%d,%[^,],%d,%d,%d,%[^,],%d,%d,%d,%d,%d",
                        state, rat, duplex, &mcc, &mnc, cell_id_str, &earfcn, 
                        pcid_str, &band, &ul_bw, &dl_bw, tac_str, 
                        &rsrp, &rsrq, &rssi, &sinr, &s_rxlev);
	
    if (parsed == 17) {
        // 保存上报字段 - 严格匹配JSON格式
        strncpy(cell_info.cell_rat, rat, sizeof(cell_info.cell_rat) - 1);
        strncpy(cell_info.cell_duplex, duplex, sizeof(cell_info.cell_duplex) - 1);
        cell_info.mcc = mcc;
        cell_info.mnc = mnc;
        
        // cell_id: 直接保存原始字符串
        strncpy(cell_info.cell_id, cell_id_str, sizeof(cell_info.cell_id) - 1);
        cell_info.cell_id[sizeof(cell_info.cell_id) - 1] = '\0';
        
        cell_info.cell_earfcn = earfcn;
        
		// pcid: 保存字符串
        strncpy(cell_info.cell_pcid, pcid_str, sizeof(cell_info.cell_pcid) - 1);
        cell_info.cell_pcid[sizeof(cell_info.cell_pcid) - 1] = '\0';
		
        cell_info.cell_band = band;
        
         // tac: 保存十六进制字符串
        strncpy(cell_info.cell_tac, tac_str, sizeof(cell_info.cell_tac) - 1);
        cell_info.cell_tac[sizeof(cell_info.cell_tac) - 1] = '\0';
        
        cell_info.cell_rsrp = rsrp;
        cell_info.cell_rsrq = rsrq;
        cell_info.cell_rssi = rssi;
        cell_info.cell_sinr = sinr;
		
//		osMutexRelease(CellInfoMutex);
        
        // 更新缓存
        update_cell_info_cache(&cell_info);
		
//		cat1_send_reply_task(COMM_TASK_ID, TASK_CAT1_QUERY_ENODEB);
    }
	else
	{
		log_debug("[CAT1][ERR] parse_serving_cell Fail\r\n");
//		cat1_send_reply_task(COMM_TASK_ID, TASK_CAT1_QUERY_ENODEB);
    }
    
    return parsed;
}

/**
 * @brief  解析邻近小区信息
 */
static int parse_neighbour_cell(char *params)
{
    /* V1.6: 删除临近小区, CellInfo_t已移除neighbour_*, 空壳 */
    (void)params; return 0;
}

/**
 * @brief  设置运营商名称
 * @param  carrier: 运营商名称字符串（如 "CHN-UNICOM"）
 */
void set_carrier_name(const char *carrier)
{
    if (!carrier || strlen(carrier) == 0) {
        return;
    }
    
    if (CellInfoMutex == NULL) {
        CellInfoMutex = osMutexNew(NULL);
    }
    
    osMutexAcquire(CellInfoMutex, osWaitForever);

	// 清零
    memset(cell_info.carrier, 0, sizeof(cell_info.carrier));
    // 直接复制运营商名称
    strncpy(cell_info.carrier, carrier, sizeof(cell_info.carrier) - 1);
    cell_info.carrier[sizeof(cell_info.carrier) - 1] = '\0';
    
    osMutexRelease(CellInfoMutex);
}

/**
 * @brief  设置运营商名称
 */
void at_cops(uint8_t *str, uint16_t len, cmd_status_t *status)
{
    char *data = (char *)str;
    int mode, format, act;
    char oper[64] = {0};  // 运营商名称缓冲区
	
    // 直接解析传入的数据：0,0,"CHN-UNICOM",7
    int parsed = sscanf(data, "%d,%d,\"%63[^\"]\",%d", 
                        &mode, &format, oper, &act);
    
    if (parsed == 4) {
        // 直接设置运营商名称
        set_carrier_name(oper);
        *status = CMD_STATUS_SUCCESS;
    } else {
        *status = CMD_STATUS_FAILED;
    }
}

/**
 * @brief  at_qeng - 处理基站信息
 */
void at_qeng(uint8_t *str, uint16_t len, cmd_status_t *status)
{
    char *data = (char *)str;
    char cell_type[32] = {0};
    
    char *first_quote = strchr(data, '"');
    if (!first_quote) {
        *status = CMD_STATUS_FAILED;
        return;
    }
    
    char *second_quote = strchr(first_quote + 1, '"');
    if (!second_quote) {
        *status = CMD_STATUS_FAILED;
        return;
    }
    
    uint16_t type_len = second_quote - first_quote - 1;
    if (type_len >= sizeof(cell_type)) {
        type_len = sizeof(cell_type) - 1;
    }
    strncpy(cell_type, first_quote + 1, type_len);
    cell_type[type_len] = '\0';
    
    if (CellInfoMutex == NULL) {
        CellInfoMutex = osMutexNew(NULL);
    }
    
    osMutexAcquire(CellInfoMutex, osWaitForever);
    
    int parsed_fields = 0;
    if (strcmp(cell_type, "servingcell") == 0) {
        parsed_fields = parse_serving_cell(data);
        *status = (parsed_fields == 17) ? CMD_STATUS_SUCCESS : CMD_STATUS_FAILED;
    } else if (strcmp(cell_type, "neighbourcell intra") == 0) {
//        parsed_fields = parse_neighbour_cell(data);
//        *status = (parsed_fields == 5) ? CMD_STATUS_SUCCESS : CMD_STATUS_FAILED;
    } else {
        *status = CMD_STATUS_FAILED;
    }
    
    osMutexRelease(CellInfoMutex);
}

void send_empty_wifi_scan_result(void)
{
    DeviceWifiSsid_t empty_result = {0};
    empty_result.wifi_count = 0;
    memset(empty_result.wifi_list, 0, sizeof(empty_result.wifi_list));
    
    send_wifi_scan_data_to_comm_task(&empty_result);
}

uint8_t parse_single_wifi_info(char *data, uint16_t len, WifiInfo_t *wifi_info)
{
    char temp_buf[256];
    char *fields[5] = {0};
    uint8_t field_count = 0;
    
    if (len >= sizeof(temp_buf) || len == 0)
        return 0;
        
    // 清空wifi_info结构体
    memset(wifi_info, 0, sizeof(WifiInfo_t));
    
    strncpy(temp_buf, data, len);
    temp_buf[len] = '\0';

    // 检查数据是否以括号开始
    char *data_start = temp_buf;
    if (*data_start == '(') {
        data_start++; // 跳过左括号
    }
    
    // 分割字段
    char *token = strtok(data_start, ",");
    while (token != NULL && field_count < 5)
    {
        // 去除首尾空格
        while (*token == ' ') token++;
        char *end = token + strlen(token) - 1;
        while (end > token && *end == ' ') *end-- = '\0';
        
        // 如果字段以右括号结束，去除右括号
        if (*end == ')') {
            *end = '\0';
        }
        
        fields[field_count] = token;
        field_count++;
        token = strtok(NULL, ",");
    }
    
    // 检查字段数量
    if (field_count < 5)
    {
        log_debug("[CAT1][DAT] Insufficient fields: %d\r\n", field_count);
        return 0;
    }
    
    // 字段1: SSID (第二个字段)
    if (fields[1] != NULL)
    {
        // 去除引号
        char *ssid = fields[1];
        if (ssid[0] == '"' && ssid[strlen(ssid)-1] == '"')
        {
            ssid[strlen(ssid)-1] = '\0';
            ssid = ssid + 1;
        }
        
        // 如果SSID为空，设置为"Hidden"
        if (strlen(ssid) == 0 || strcmp(ssid, "") == 0 || strcmp(ssid, "-") == 0)
        {
            strncpy(wifi_info->ssid, "[Hidden]", sizeof(wifi_info->ssid)-1);
        }
        else
        {
            strncpy(wifi_info->ssid, ssid, sizeof(wifi_info->ssid)-1);
        }
        wifi_info->ssid[sizeof(wifi_info->ssid)-1] = '\0';
    }
    
    // 字段2: RSSI (第三个字段)
    if (fields[2] != NULL)
    {
        strncpy(wifi_info->rssi, fields[2], sizeof(wifi_info->rssi)-1);
        wifi_info->rssi[sizeof(wifi_info->rssi)-1] = '\0';
    }
    
    // 字段3: MAC地址 (第四个字段)
    if (fields[3] != NULL)
    {
        // 去除引号
        char *mac = fields[3];
        if (mac[0] == '"' && mac[strlen(mac)-1] == '"')
        {
            mac[strlen(mac)-1] = '\0';
            mac = mac + 1;
        }
        strncpy(wifi_info->mac, mac, sizeof(wifi_info->mac)-1);
        wifi_info->mac[sizeof(wifi_info->mac)-1] = '\0';
    }

    return (strlen(wifi_info->mac) > 0 && strlen(wifi_info->rssi) > 0) ? 1 : 0;
}

void at_cgdcont(uint8_t *str, uint16_t len, cmd_status_t *status)
{
    int cid, d_comp, h_comp, ipv4_addr_alloc, ipv6_addr_alloc;
    char pdp_type[16] = {0};
    char apn[64] = {0};
    char pdp_addr[32] = {0};
    
    if (str == NULL || len == 0 || str[0] == '\0' || str[0] == '\r' || str[0] == '\n')
    {
        *status = CMD_STATUS_FAILED;
        return;
    }

	int parsed_count = sscanf((char *)str, "%d,\"%15[^\"]\",\"%63[^\"]\",\"%31[^\"]\",%d,%d,%d,%d",
               &cid, pdp_type, apn, pdp_addr, &d_comp, &h_comp, &ipv4_addr_alloc, &ipv6_addr_alloc);
	
    // 解析格式: 1,"IP","linksnet",... 或 1,"IP","linkset","0.0.0.0",...
    if (parsed_count >= 2)
    {
		log_debug("[CAT1][DAT] cid: %d\r\n", cid);
        if (cid == 1)
        {
            if (is_apn_valid(apn))
            {
                *status = CMD_STATUS_SUCCESS;  // CID=1 且 APN 正确
            }
            else
            {
                // CID=1 但 APN 不正确，重新设置
				// DTR设置
				lteExitsleepDtrLow();
                lte_check_cmd_rtos(LTE_SET_APN, "OK\r\n", 1, 1000, 0);
                lte_check_cmd_rtos(LTE_SET_CGACT, "OK\r\n", 1, 1000, 0);
                lte_check_cmd_rtos(LTE_QIACT, "OK\r\n", 1, 1000, 0);
                *status = CMD_STATUS_SUCCESS;
            }
        }
        else
        {
            // CID != 1，不处理
            *status = CMD_STATUS_SUCCESS;
        }
    }
    else
    {
        // 解析失败，重新设置
		// DTR设置
		lteExitsleepDtrLow();
        lte_check_cmd_rtos(LTE_SET_APN, "OK\r\n", 1, 1000, 0);
        lte_check_cmd_rtos(LTE_SET_CGACT, "OK\r\n", 1, 1000, 0);
        lte_check_cmd_rtos(LTE_QIACT, "OK\r\n", 1, 1000, 0);
        *status = CMD_STATUS_FAILED;
    }
}

/**
 * @brief  at_qmtpubex
 * @param str 数据
 * @param len 数据长度  
 * @param status 指令执行状态
 * @return NULL
 * 
 * 响应格式: +QMTPUBEX: 0,0,0 或 +QMTPUBEX: 0,1,0
 */
void at_qmtpubex(uint8_t *str, uint16_t len, cmd_status_t *status)
{
	// 修复: has_extra_response=0 后, OK\r\n 已触发 currentCmd 释放 + g_mqtt_sending 清零
	// +QMTPUBEX 作为独立 URC 在 currentCmd=NULL 的"无指令上报"分支处理, 仅做 broker 确认日志
	int client_idx = -1, msg_id = -1, result_code = -1;

	if (sscanf((char *)str, "%d,%d,%d", &client_idx, &msg_id, &result_code) == 3) {
		if (result_code == 0) {
			log_debug("[CAT1][STA] MQTT publish confirmed by broker\r\n");
		} else {
			// result=1: 重传; result=2: Failed to send packet(发送失败)
			log_debug("[CAT1][ERR] MQTT publish failed, broker result=%d\r\n", result_code);
		}
	} else {
		log_debug("[CAT1][STA] +QMTPUBEX received\r\n");
	}
	
	*status = CMD_STATUS_SUCCESS;
}

/**
 * @brief  at_qmtstat		cat1 异常断连
 * @param str 数据    
 * @param len 数据长度
 * @param status 指令执行状态
 * @return NULL
 **/
void at_qmtstat(uint8_t *data, uint16_t len, cmd_status_t *status)
{
    // 检查输入参数是否有效
    if (data == NULL || status == NULL) {
        *status = CMD_STATUS_FAILED;
        return;
    }
    
	if (production_flag.flag_cat1_conn) {
		production_cat1_send_errorcode_task(TEST_TASK_ID, TASK_CAT1_AWS_WITHCA_TEST_REPLY,1);
        *status = CMD_STATUS_SUCCESS;
        return;
    }
	
    // 解析错误码，格式为：<client_index>,<error_code>
    int client_index = -1;
    int error_code = -1;
    
    if (sscanf((char*)data, "%d,%d", &client_index, &error_code) == 2) {
        // 根据错误码进行相应处理
        switch (error_code) {
            case 1:
				//Connection is closed or reset by a peer.
            case 2:
                // Sending PINGREQ packet timed out or failed.
                // 先停用和激活PDP，然后重新打开MQTT连接
            case 3:
				//Sending CONNECT packet timed out or failed
            case 4:
                // Sending CONNECT/CONNACK packet timed out or failed.
                // 检查用户名密码，确保客户端ID未被使用，重新连接
				isMqttConnected = false;
				log_debug("[CAT1][STA] qmtstat is_reconnecting: %d\r\n",is_reconnecting);
                if (!is_reconnecting) {
					/* 重连前清空遗留的PUB/订阅等指令，避免断网期间报ERROR堵住/触发重启 */
					mqtt_disconnect_flush_queue();
					reStartModeConn();
				}
                *status = CMD_STATUS_SUCCESS;
                break;

            case 5:
				//正常流程
                *status = CMD_STATUS_SUCCESS;
                break;

            case 6:
				isMqttConnected = false;
				if (!is_reconnecting) {
					mqtt_disconnect_flush_queue();
					reStartModeConn();
				}
                *status = CMD_STATUS_SUCCESS;
                break;
                
            case 7:
                // 严重错误，需要重启
                *status = CMD_STATUS_FAILED;
                break;
                
            case 8:
				isMqttConnected = false;
                // The client closed the MQTT connection.
                // 尝试重新连接
				if (!is_reconnecting) {
					mqtt_disconnect_flush_queue();
					reStartModeConn();
				}
                *status = CMD_STATUS_SUCCESS;
                break;
                
            default:
                // 9-255: Reserved for future use.
                // 未知错误码，按严重错误处理
                if (error_code >= 9 && error_code <= 255) {
                    *status = CMD_STATUS_FAILED;
                } else {
                    // 无效的错误码
                    *status = CMD_STATUS_FAILED;
                }
                break;
        }
        
    } else {
        // 解析失败，格式错误
        *status = CMD_STATUS_FAILED;
    }
}


/* 新一轮 scan 开始前复位累积缓冲区（由 lteControlWifi 调用） */
void lte_wifi_scan_reset(void)
{
    memset(&wifi_scan_accum, 0, sizeof(wifi_scan_accum));
    wifi_scan_accum.wifi_count = 0;
    wifi_scan_accum_active = false;
}

/* AT 命令完成（收到 OK 或超时/错误）后 flush 累积的 URC 到 COMM */
void lte_wifi_scan_flush(void)
{
    if (wifi_scan_accum_active) {
        if (wifi_scan_accum.wifi_count > 0) {
//            log_debug("[CAT1][STA] WiFi scan flush: %d APs → COMM\r\n", wifi_scan_accum.wifi_count);
            send_wifi_scan_data_to_comm_task(&wifi_scan_accum);
        } else {
//            log_debug("[CAT1][STA] WiFi scan flush: empty → COMM\r\n");
            send_empty_wifi_scan_result();
        }
        wifi_scan_accum_active = false;
    } else {
        log_debug("[CAT1][DBG] WiFi scan flush called but accum inactive\r\n");
    }
    memset(&wifi_scan_accum, 0, sizeof(wifi_scan_accum));
    wifi_scan_accum.wifi_count = 0;
}

/**
 * @brief  at_wifiScan		cat1 wifi scan（累积模式）
 * @param str 数据    
 * @param len 数据长度
 * @param status 指令执行状态
 * @return NULL
 **/
void at_wifiScan(uint8_t *data, uint16_t len, cmd_status_t *status)
{
    char *scan_data = (char *)data;
    char *current_pos = scan_data;

//    log_debug("[CAT1][DBG] at_wifiScan called, len=%d\r\n", len);

    /* 错误/TIMEOUT：立即发送空结果并重置 */
    if (strstr(scan_data, "TIMEOUT") != NULL) {
        log_debug("[CAT1][ERR] WiFi scan timeout\r\n");
        *status = CMD_STATUS_FAILED;
        lte_wifi_scan_flush();
        return;
    }

    if (strstr(scan_data, "ERROR") != NULL) {
        log_debug("[CAT1][ERR] WiFi scan error\r\n");
        *status = CMD_STATUS_FAILED;
        lte_wifi_scan_flush();
        return;
    }

    /* 首次 URC：初始化累积 */
    if (!wifi_scan_accum_active) {
        memset(&wifi_scan_accum, 0, sizeof(wifi_scan_accum));
        wifi_scan_accum.wifi_count = 0;
        wifi_scan_accum_active = true;
    }

    uint8_t found_entries = 0;

    /* 循环处理本调用中的所有 URC */
    while (current_pos < scan_data + len && wifi_scan_accum.wifi_count < 10)
    {
        char *cmd_start = strstr(current_pos, "+QWIFISCAN");
        if (cmd_start == NULL) break;

        char *colon_pos = strchr(cmd_start, ':');
        if (colon_pos == NULL) break;

        char *data_start = colon_pos + 1;
        while (*data_start == ' ') data_start++;

        char *next_cmd = strstr(data_start, "+QWIFISCAN");
        char *paren_end = strchr(data_start, ')');
        char *line_end = strstr(data_start, "\r\n");

        char *data_end;
        if (next_cmd != NULL)
            data_end = next_cmd;
        else if (paren_end != NULL)
            data_end = paren_end + 1;
        else if (line_end != NULL)
            data_end = line_end;
        else
            data_end = scan_data + len;

        if (parse_single_wifi_info(data_start, data_end - data_start,
                                  &wifi_scan_accum.wifi_list[wifi_scan_accum.wifi_count]))
        {
            wifi_scan_accum.wifi_count++;
            found_entries = 1;
        }

        if (next_cmd != NULL)
            current_pos = next_cmd;
        else
            break;
    }

    /*  lte_wifi_scan_flush() 在命令完成时统一发送 */
//    log_debug("[CAT1][DBG] at_wifiScan accum count=%d\r\n", wifi_scan_accum.wifi_count);
    *status = CMD_STATUS_SUCCESS;
}


/**
 * @brief  at_qflst - 处理文件列表响应
 * @param str 数据
 * @param len 数据长度  
 * @param status 指令执行状态
 * @return NULL
 */
void at_qflst(uint8_t *str, uint16_t len, cmd_status_t *status)
{
    uint16_t pos = 0;
    
    // 如果是删除模式，需要记录找到的证书
    static bool found_cacert = false;
    static bool found_client = false;
    static bool found_userkey = false;
    static uint8_t certificates_found = 0;
    static uint8_t current_delete_stage = 0;
    
    // 如果不是删除模式，先重置静态变量并正常解析
    if (!get_certificate_deletion_mode()) {
        // 重置静态变量
        found_cacert = false;
        found_client = false;
        found_userkey = false;
        certificates_found = 0;
        current_delete_stage = 0;
    }
    
    // 解析每一行文件列表
    while (pos < len) {
        // 跳过空白字符
        while (pos < len && (str[pos] == ' ' || str[pos] == '\r' || str[pos] == '\n')) {
            pos++;
        }
        
        if (pos >= len) break;
        
        // 检查是否是 +QFLST: 响应
        if (pos + 7 < len && 
            str[pos] == '+' && str[pos+1] == 'Q' && str[pos+2] == 'F' && 
            str[pos+3] == 'L' && str[pos+4] == 'S' && str[pos+5] == 'T' && str[pos+6] == ':') {
            
            pos += 7; // 跳过"+QFLST:"
            
            // 跳过空格
            while (pos < len && str[pos] == ' ') pos++;
            
            // 检查引号
            if (pos >= len || str[pos] != '"') {
                log_debug("[CAT1][ERR] No quote after\r\n");
                // 移动到下一行继续解析
                while (pos < len && str[pos] != '\n') pos++;
                if (pos < len) pos++;
                continue;
            }
            
            // 跳过第一个引号
            pos++;
            uint16_t filename_start = pos;
            
            // 找到第二个引号
            while (pos < len && str[pos] != '"' && str[pos] != '\n' && str[pos] != '\r') {
                pos++;
            }
            
            if (pos >= len || str[pos] != '"') {
                log_debug("[CAT1][ERR] No closing quote found\r\n");
                // 移动到下一行继续解析
                while (pos < len && str[pos] != '\n') pos++;
                if (pos < len) pos++;
                continue;
            }
            
            uint16_t filename_end = pos;
            pos++; // 跳过第二个引号
            
            // 提取文件名
            char filename[64] = {0};
            uint16_t filename_len = filename_end - filename_start;
            
            if (filename_len == 0 || filename_len >= sizeof(filename)) {
                while (pos < len && str[pos] != '\n') pos++;
                if (pos < len) pos++;
                continue;
            }
            
            memcpy(filename, &str[filename_start], filename_len);
            filename[filename_len] = '\0';

            // 处理文件名（去掉UFS:前缀）
            char *filename_ptr = filename;
            if (strncmp(filename_ptr, "UFS:", 4) == 0) {
                filename_ptr += 4;
            }
            
            // 检查是否是证书文件
            bool is_certificate = false;
            if (strcmp(filename_ptr, "ca.pem") == 0) {
                found_cacert = true;
                certificates_found++;
                is_certificate = true;
            } else if (strcmp(filename_ptr, "client.pem") == 0) {
                found_client = true;
                certificates_found++;
                is_certificate = true;
            } else if (strcmp(filename_ptr, "key.pem") == 0) {
                found_userkey = true;
                certificates_found++;
                is_certificate = true;
            }
            
            // 如果不是证书文件，检查是否是音频文件
            if (!is_certificate) {
                // 检查文件扩展名
                char *ext = strrchr(filename_ptr, '.');
                bool is_audio_file = false;
                
                if (ext != NULL) {
                    // 检查常见音频扩展名
                    if (strcmp(ext, ".wav")) {
                        is_audio_file = true;
                    } else if (strstr(filename_ptr, "audio") != NULL || 
                               strstr(filename_ptr, "realtime") != NULL) {
                        // 文件名包含audio或realtime也认为是音频文件
                        is_audio_file = true;
                    }
                } else {
                    // 没有扩展名，检查文件名是否包含音频相关关键字
                    if (strstr(filename_ptr, "audio") != NULL || 
                        strstr(filename_ptr, "realtime") != NULL ||
                        strstr(filename_ptr, "record") != NULL) {
                        is_audio_file = true;
                    }
                }
                
                if (is_audio_file) {
                    // 跳过逗号和空格以获取文件大小
                    while (pos < len && (str[pos] == ' ' || str[pos] == ',' || str[pos] == '\t')) {
                        pos++;
                    }
                    
                    // 提取文件大小
                    uint16_t size_start = pos;
                    while (pos < len && str[pos] >= '0' && str[pos] <= '9') {
                        pos++;
                    }
                    
                    if (pos > size_start) {
                        char size_str[16] = {0};
                        uint16_t size_len = pos - size_start;
                        if (size_len < sizeof(size_str)) {
                            memcpy(size_str, &str[size_start], size_len);
                            size_str[size_len] = '\0';
                            
                            uint32_t file_size = atoi(size_str);
                            http_data.read_size = file_size;
                            http_data.read_offset = 0;
                            // 根据文件大小合理设置每次读取的块大小
                            http_data.file_size = (file_size > HTTP_READFILE_BLOCKSIZE) ? 
                                                   HTTP_READFILE_BLOCKSIZE : file_size;
                            
                            // 安全复制文件名
                            strncpy(http_data.filename, filename_ptr, sizeof(http_data.filename) - 1);
                            http_data.filename[sizeof(http_data.filename) - 1] = '\0';
                        }
                    } else {
                        log_debug("[CAT1][ERR] No file found for audio: %s\r\n", filename_ptr);
                    }
                }
            }
            
            // 移动到行尾
            while (pos < len && str[pos] != '\n' && str[pos] != '\r') {
                pos++;
            }
        }
        else {
            // 不是+QFLST:格式，移动到下一行
            while (pos < len && str[pos] != '\n') {
                pos++;
            }
        }
        
        // 跳过换行符
        if (pos < len && str[pos] == '\n') pos++;
        if (pos < len && str[pos] == '\r') pos++;
    }
    
    // 状态设置逻辑
    if (get_certificate_deletion_mode()) {
//        log_debug("Certificate deletion mode: found_cacert=%d, found_client=%d, found_userkey=%d, stage=%d\n", 
//                  found_cacert, found_client, found_userkey, current_delete_stage);
        
        // 根据删除阶段发送删除命令
        if (current_delete_stage == 0) {
            // 首次进入，检查并发送第一个删除命令
            if (found_cacert) {
//                log_debug("Stage 0->1: Sending delete command for ca.pem\n");
                current_delete_stage = 1;
                lte_check_cmd_rtos(LTE_QFDEL_CACERT, "OK\r\n", 1, 3000, 0);
                *status = CMD_STATUS_SUCCESS;
                return;
            } else if (found_client) {
//                log_debug("Stage 0->2: Sending delete command for client.pem\n");
                current_delete_stage = 2;
                lte_check_cmd_rtos(LTE_QFDEL_CLIENT, "OK\r\n", 1, 3000, 0);
                *status = CMD_STATUS_SUCCESS;
                return;
            } else if (found_userkey) {
//                log_debug("Stage 0->3: Sending delete command for key.pem\n");
                current_delete_stage = 3;
                lte_check_cmd_rtos(LTE_QFDEL_USERKEY, "OK\r\n", 1, 3000, 0);
                *status = CMD_STATUS_SUCCESS;
                return;
            } else {
                // 没有证书，直接完成
                log_debug("[CAT1][STA] No certificates found, completing deletion\r\n");
                set_certificate_deletion_mode(false);
                found_cacert = false;
                found_client = false;
                found_userkey = false;
                certificates_found = 0;
                current_delete_stage = 0;
                Message_Cmd_Put(CAT1_UART_TASK_ID, ENTRY_TASK_ID, TASK_FACTORY_RESET_REPLY, NULL, 0);
                Production_Result_Report(REPORT_OK);
                *status = CMD_STATUS_SUCCESS;
                return;
            }
        }
        else if (current_delete_stage == 1) {
            // 刚删除完ca.pem，检查是否需要删除client.pem
            if (found_client) {
//                log_debug("Stage 1->2: Sending delete command for client.pem\n");
                current_delete_stage = 2;
                lte_check_cmd_rtos(LTE_QFDEL_CLIENT, "OK\r\n", 1, 3000, 0);
                *status = CMD_STATUS_SUCCESS;
                return;
            } else if (found_userkey) {
//                log_debug("Stage 1->3: Sending delete command for key.pem\n");
                current_delete_stage = 3;
                lte_check_cmd_rtos(LTE_QFDEL_USERKEY, "OK\r\n", 1, 3000, 0);
                *status = CMD_STATUS_SUCCESS;
                return;
            } else {
                // 没有更多证书，删除完成
//                log_debug("No more certificates after ca.pem, deletion completed\n");
                set_certificate_deletion_mode(false);
                found_cacert = false;
                found_client = false;
                found_userkey = false;
                certificates_found = 0;
                current_delete_stage = 0;
                Message_Cmd_Put(CAT1_UART_TASK_ID, ENTRY_TASK_ID, TASK_FACTORY_RESET_REPLY, NULL, 0);
                Production_Result_Report(REPORT_OK);
                *status = CMD_STATUS_SUCCESS;
                return;
            }
        }
        else if (current_delete_stage == 2) {
            // 刚删除完client.pem，检查是否需要删除key.pem
            if (found_userkey) {
//                log_debug("Stage 2->3: Sending delete command for key.pem and re-query\n");
                current_delete_stage = 3;
                lte_check_cmd_rtos(LTE_QFDEL_USERKEY, "OK\r\n", 1, 3000, 0);
                lte_check_cmd_rtos(LTE_QFLST, "OK\r\n", 1, 3000, 0);
                *status = CMD_STATUS_SUCCESS;
                return;
            } else {
                // 没有更多证书，删除完成
                set_certificate_deletion_mode(false);
                found_cacert = false;
                found_client = false;
                found_userkey = false;
                certificates_found = 0;
                current_delete_stage = 0;
                Message_Cmd_Put(CAT1_UART_TASK_ID, ENTRY_TASK_ID, TASK_FACTORY_RESET_REPLY, NULL, 0);
                Production_Result_Report(REPORT_OK);
                *status = CMD_STATUS_SUCCESS;
                return;
            }
        }
        else if (current_delete_stage == 3) {
            // 删除key.pem后的再次查询结果
            if (found_cacert || found_client || found_userkey) {
                set_certificate_deletion_mode(false);
                found_cacert = false;
                found_client = false;
                found_userkey = false;
                certificates_found = 0;
                current_delete_stage = 0;
                Message_Cmd_Put(CAT1_UART_TASK_ID, ENTRY_TASK_ID, TASK_FACTORY_RESET_REPLY, NULL, 0);
                Production_Result_Report(REPORT_CAT1_CMD_ERROR);
                *status = CMD_STATUS_SUCCESS;
                return;
            } else {
                // 所有证书都已删除，发送成功报告
                set_certificate_deletion_mode(false);
                found_cacert = false;
                found_client = false;
                found_userkey = false;
                certificates_found = 0;
                current_delete_stage = 0;
                Message_Cmd_Put(CAT1_UART_TASK_ID, ENTRY_TASK_ID, TASK_FACTORY_RESET_REPLY, NULL, 0);
                Production_Result_Report(REPORT_OK);
                *status = CMD_STATUS_SUCCESS;
                return;
            }
        }
    } else {
        // 普通模式：解析完成就返回成功
        *status = CMD_STATUS_SUCCESS;
        // 重置静态变量
        found_cacert = false;
        found_client = false;
        found_userkey = false;
        certificates_found = 0;
        current_delete_stage = 0;
    }
}

/**
 * @brief 处理QFOPEN文件打开响应
 */
void at_qfopen(uint8_t *str, uint16_t len, cmd_status_t *status)
{
    // 直接解析文件句柄数字
    char temp_buf[32];
    if (len >= sizeof(temp_buf)) {
        len = sizeof(temp_buf) - 1;
    }
    memcpy(temp_buf, str, len);
    temp_buf[len] = '\0';
    
    // 去除可能的空格和换行符
    char *num_start = temp_buf;
    while (*num_start == ' ' || *num_start == '\r' || *num_start == '\n') {
        num_start++;
    }
    
    http_data.file_handle = atoi(num_start);
    log_debug("[CAT1][DAT] File opened with handle: %d\r\n", http_data.file_handle);
    
    *status = CMD_STATUS_SUCCESS;
}

/**
 * @brief 处理QHTTPGET响应
 * 格式: +QHTTPGET: 0,200,115
 */
void at_qhttpget(uint8_t *str, uint16_t len, cmd_status_t *status)
{
    // 解析响应格式: <error>,<http_status>,<content_length>
    int error, http_status, content_length;
    
    // 首先检查是否为701错误（单独的错误代码格式）
    if (sscanf((char *)str, "%d", &error) == 1 && error == 701)
    {
        lte_check_cmd_rtos(LTE_QFDEL_AUDIO, "OK\r\n", 1, 3000, 0);
        *status = CMD_STATUS_FAILED;
        return;
    }
    
    // 正常解析响应格式: <error>,<http_status>,<content_length>
    if (sscanf((char *)str, "%d,%d,%d", &error, &http_status, &content_length) == 3)
    {
        if (error == 0 && http_status == 200)  // 成功且HTTP状态为200
        {
            // 记录文件大小
            http_data.read_size = content_length;
            http_data.read_offset = 0;
            
            *status = CMD_STATUS_SUCCESS;
        }
        else if (error == 701)  // 再次检查701错误（理论上不会走到这里）
        {
            lte_check_cmd_rtos(LTE_QFDEL_AUDIO, "OK\r\n", 1, 3000, 0);
            *status = CMD_STATUS_FAILED;
        }
        else
        {
            log_debug("[CAT1][ERR] HTTP GET failed, error: %d, http status: %d\r\n", 
                     error, http_status);
            *status = CMD_STATUS_FAILED;
        }
    }
    else
    {
//        log_debug("[CAT1][STA] Invalid QHTTPGET response format\r\n");
        *status = CMD_STATUS_FAILED;
    }
}

/**
 * @brief 处理QHTTPREADFILE响应
 */
void at_qhttpreadfile(uint8_t *str, uint16_t len, cmd_status_t *status)
{
    // 直接解析返回的数字结果
    char temp_buf[32];
    if (len >= sizeof(temp_buf)) {
        len = sizeof(temp_buf) - 1;
    }
    memcpy(temp_buf, str, len);
    temp_buf[len] = '\0';
    
    // 去除空格和换行符
    char *num_start = temp_buf;
    while (*num_start == ' ' || *num_start == '\r' || *num_start == '\n') {
        num_start++;
    }
    
    int result = atoi(num_start);
    
    if (result == 0) {
        log_debug("[CAT1][STA] HTTP file read to UFS success\r\n");
        *status = CMD_STATUS_SUCCESS;
    } else {
        log_debug("[CAT1][ERR] QHTTPREAD failed with error code: %d\r\n", result);
        *status = CMD_STATUS_FAILED;
    }
}


/**
 * @brief 解析QFREAD的数据并存储到文件
 */
cmd_status_t parse_qfread_data(uint8_t *str, uint16_t len)
{
    char *response = (char *)str;
    static bool isLfsNull = true;
	
    char *connect_start = strstr(response, "CONNECT ");
    if (!connect_start)
    {
        log_debug("[CAT1][ERR] CONNECT not found\r\n");
        return CMD_STATUS_FAILED;
    }
    
    // 解析数据长度
    uint16_t data_size = atoi(connect_start + 8); // 跳过"CONNECT "
    
    // 找到数据开始位置（CONNECT行后的第一个\r\n之后）
    char *data_start = strstr(connect_start, "\r\n");
    if (!data_start)
    {
        log_debug("[CAT1][ERR] No data start found\r\n");
        return CMD_STATUS_FAILED;
    }
    data_start += 2; // 跳过\r\n
    
    uint16_t actual_data_len = data_size;
    
    if (actual_data_len > 0)
    {
		if(isLfsNull)
		{
			isLfsNull = false;
			if(lfs_file_exists_and_empty("/audio/record/tmp.wav") == 2)
			{
				lfs_clear_file_content("/audio/record/tmp.wav");
			}
		}
		// 保存数据
        log_write_with_rotation("/audio/record/tmp.wav", 
                               (uint8_t *)data_start, 
                               actual_data_len);
        
        // 更新偏移量
        http_data.read_offset += actual_data_len;
        
        // 检查是否完成
        if (http_data.read_offset < http_data.read_size)
        {
            osDelay(osMS2TicksRound(20));
            lte_check_cmd_rtos(LTE_QFREAD, "CONNECT", 1, 3000, 0);
            return CMD_STATUS_SUCCESS;
        }
        else
        {
            // 读取完成
            log_debug("[CAT1][STA] File read complete: %d bytes\r\n", http_data.read_offset);
            lte_check_cmd_rtos(LTE_QFCLOSE, "OK\r\n", 1, 3000, 0);
            lte_check_cmd_rtos(LTE_QFDEL, "OK\r\n", 1, 3000, 0);
//			reStartModeConn();
			reStartModeQiCgact();
			//回复REPLY给COMM
			cat1_send_reply_task(COMM_TASK_ID, TASK_AUDIO_REALTIME);
            lfs_list_dir("/");
			Audio_Play_Request("/audio/record/tmp.wav",1);
			
			isLfsNull = true;
        }
        return CMD_STATUS_SUCCESS;
    }
    return CMD_STATUS_FAILED;
}

/**
 * @brief 处理CONNECT响应
 */
void at_connect(uint8_t *str, uint16_t len, cmd_status_t *status)
{
    // 检查当前指令类型来区分不同的CONNECT
    if (currentCmd != NULL)
    {
        switch (currentCmd->type)
        {
            case LTE_QHTTPURL:
                // HTTP URL设置的CONNECT，需要发送URL数据
                *status = CMD_STATUS_SUCCESS;
//                func_lte_type(LTE_QHTTPURL_DATA);
                break;
            case LTE_QFREAD:
                // 文件读取的CONNECT，解析数据
                log_debug("[CAT1][STA] File read CONNECT received\r\n");
                *status = parse_qfread_data(str, len);
                break;
			case LTE_QFUPL_CACERT:
            case LTE_QFUPL_CLIENT:  
            case LTE_QFUPL_USERKEY:
                // 证书写入的CONNECT，需要发送证书数据
                log_debug("[CAT1][STA] Certificate CONNECT received\r\n");
                *status = CMD_STATUS_SUCCESS;
                break;
            default:
                *status = CMD_STATUS_SUCCESS;
                break;
        }
    }
    else
    {
        *status = CMD_STATUS_SUCCESS;
    }
}

/**
 * @brief  at_qfupl - 处理证书写入响应
 * @param str 数据
 * @param len 数据长度  
 * @param status 指令执行状态
 * @return NULL
 */
void at_qfupl(uint8_t *str, uint16_t len, cmd_status_t *status)
{
    if (str == NULL || len == 0) {
        *status = CMD_STATUS_FAILED;
        return;
    }

    // 解析格式: +QFUPL: <upload_size>,<checksum>
    uint16_t upload_size = 0, checksum = 0;
    
    if (sscanf((char *)str, "%hu,%hu", &upload_size, &checksum) == 2) {
        // 验证上传长度是否匹配预期长度
        if (upload_size == expected_cert_len ) {
			if(currentCmd->type == LTE_QFUPL_CACERT_DATA)
			{
				production_cat1_send_errorcode_task(TEST_TASK_ID, TASK_CAT1_AWS_CACERT_REPLY, 0);
			}
			if(currentCmd->type == LTE_QFUPL_CLIENT_DATA)
			{
				production_cat1_send_errorcode_task(TEST_TASK_ID, TASK_CAT1_AWS_CLIENT_REPLY,0);
			}
			if(currentCmd->type == LTE_QFUPL_USERKEY_DATA)
			{
				production_cat1_send_errorcode_task(TEST_TASK_ID, TASK_CAT1_AWS_USERKEY_REPLY,0);
			}
            *status = CMD_STATUS_SUCCESS;
            cert_write_state = CERT_STATE_COMPLETE;
            log_debug("[CAT1][STA] Certificate write successful, size matched: %d\r\n", upload_size);
            
            // 写入成功，可以释放内存了
            free_cert_data();
            
        } else {
			if(currentCmd->type == LTE_QFUPL_CACERT_DATA)
			{
				production_cat1_send_errorcode_task(TEST_TASK_ID, TASK_CAT1_AWS_CACERT_REPLY, 1);
			}
			if(currentCmd->type == LTE_QFUPL_CLIENT_DATA)
			{
				production_cat1_send_errorcode_task(TEST_TASK_ID, TASK_CAT1_AWS_CLIENT_REPLY,1);
			}
			if(currentCmd->type == LTE_QFUPL_USERKEY_DATA)
			{
				production_cat1_send_errorcode_task(TEST_TASK_ID, TASK_CAT1_AWS_USERKEY_REPLY,1);
			}
            *status = CMD_STATUS_FAILED;
            cert_write_state = CERT_STATE_ERROR;
            log_debug("[CAT1][ERR] Certificate write failed: upload_size(%d) != expected(%d)\r\n", 
                     upload_size, expected_cert_len);
            
            // 写入失败，删除UFS中的文件并发送错误消息
            delete_certificate_from_ufs(current_cert_type);
			cat1_send_reply_task(COMM_TASK_ID, CERT_PRODUCT_ERROR);
            
            // 释放内存
            free_cert_data();
        }
    } else {
        *status = CMD_STATUS_FAILED;
        cert_write_state = CERT_STATE_ERROR;
        log_debug("[CAT1][ERR] Invalid QFUPL response format: %s\r\n", str);
        
        // 释放内存
        free_cert_data();
    }
}

/**
 * @brief  at_device_version		cat1 版本号处理
 * @param str 数据
 * @param len 数据长度
 * @param status 指令执行状态
 * @return NULL
 **/
void at_device_version(uint8_t *str, uint16_t len, cmd_status_t *status)
{
    if (str == NULL || len == 0)
    {
        *status = CMD_STATUS_FAILED;
        return;
    }

    uint8_t *clean_str = str;
    uint16_t clean_len = len;

    // 去除前后空白字符、回车换行
    while (clean_len > 0 && (clean_str[0] == ' ' || clean_str[0] == '\r' || clean_str[0] == '\n'))
    {
        clean_str++;
        clean_len--;
    }
    while (clean_len > 0 && (clean_str[clean_len - 1] == ' ' || clean_str[clean_len - 1] == '\r' || clean_str[clean_len - 1] == '\n' || clean_str[clean_len - 1] == '\0'))
    {
        clean_len--;
    }

    if (clean_len == 0)
    {
        *status = CMD_STATUS_FAILED;
        return;
    }

    // 检查是否包含 EG800Q 前缀
    if (clean_len >= 5 && strncmp((char *)clean_str, "EG800Q", 5) == 0)
    {
        // 查找第一个空白字符或换行符作为结束位置
        uint16_t version_len = 0;
        for (version_len = 0; version_len < clean_len; version_len++)
        {
            char c = clean_str[version_len];
            if (c == ' ' || c == '\r' || c == '\n' || c == '\t' || c == '\0')
            {
                break;
            }
        }

        // 确保不会超出cat1Version数组大小
        if (version_len >= sizeof(cat1Version))
        {
            version_len = sizeof(cat1Version) - 1;
        }

        // 复制版本号到cat1Version
        memcpy(cat1Version, clean_str, version_len);
        cat1Version[version_len] = '\0';
        
		//写入文件系统
		lfs_system_write((char *)cat1Version, strlen((char *)cat1Version),SYS_FIRMWARE_LTE_VER_ID);
        production_cat1_send_errorcode_task(TEST_TASK_ID, TASK_CMD_CAT1_VERSION_REPLY, 0);
        
        log_debug("[CAT1][DAT] Cat1 version stored: %s\r\n", cat1Version);
        *status = CMD_STATUS_SUCCESS;
    }
    else
    {
        production_cat1_send_errorcode_task(TEST_TASK_ID, TASK_CMD_CAT1_VERSION_REPLY, 1);
        log_debug("[CAT1][ERR] Invalid version format, expected EG800Q prefix, got: %.*s\r\n", clean_len, clean_str);
        
        *status = CMD_STATUS_FAILED;
    }
}

/**
 * @brief  at_device_cat1_sn		cat1 SN 处理
 * @param str 数据
 * @param len 数据长度
 * @param status 指令执行状态
 * @return NULL
 **/
void at_device_cat1_sn(uint8_t *str, uint16_t len, cmd_status_t *status)
{
	if (str == NULL || len == 0)
	{
		*status = CMD_STATUS_FAILED;
		return;
	}

	// 移除双引号
	uint8_t *clean_str = str;
	uint16_t clean_len = len;

	if (len >= 2 && str[0] == '"' && str[len - 1] == '"')
	{
		clean_str = str + 1;
		clean_len = len - 2;
	}

	if (clean_len >= sizeof(cat1Sn))
	{
		clean_len = sizeof(cat1Sn) - 1;
	}

	if (osMutexAcquire(LteDeviceInfoMutex, osWaitForever) == osOK)
	{
		memcpy(cat1Sn, clean_str, clean_len);
		cat1Sn[clean_len] = '\0';
		osMutexRelease(LteDeviceInfoMutex);
	}

	*status = CMD_STATUS_SUCCESS;
}

/**
 * @brief  at_device_esim_sn		eSIM SN 处理函数
 * @param str 数据
 * @param len 数据长度
 * @param status 指令执行状态
 * @return NULL
 **/
void at_device_esim_sn(uint8_t *str, uint16_t len, cmd_status_t *status)
{
	if (str == NULL || len == 0)
	{
		*status = CMD_STATUS_FAILED;
		return;
	}

	uint8_t *clean_str = str;
	uint16_t clean_len = len + 1;

	// if (clean_len >= MAX_ESIM_SN_LENGTH) {
	//     clean_len = MAX_ESIM_SN_LENGTH - 1;
	// }
//	log_debug("SN: %s\n", clean_str);
	
	BaseType_t result = send_data_to_test_task(TEST_TASK_ID, 
	                                          TASK_CMD_CAT1_TEST_REPLAY,
	                                          clean_str, 
	                                          clean_len);
	if (result == pdTRUE) {
		*status = CMD_STATUS_SUCCESS;
	} else {
		*status = CMD_STATUS_FAILED;
	}
}

/**
 * @brief 处理固件升级响应
 * @param str 响应数据
 * @param len 数据长度
 * @param status 指令状态指针
 */
void at_firmware_upgrade(uint8_t *str, uint16_t len, cmd_status_t *status)
{
    char *response = (char *)str;
    char *fota_ptr = strstr(response, "FOTA");
    if (fota_ptr == NULL) {
        *status = CMD_STATUS_FAILED;
        return;
    }
    // 查找是否包含升级成功标志
    if (strstr(response, "\"FOTA\",\"END\",0") != NULL) 
    {
        log_debug("[CAT1][STA] Firmware upgrade success!\r\n");
        firmware_upgrade_state = 3;
        *status = CMD_STATUS_SUCCESS;
        
        // 通知上层固件升级成功
        production_cat1_send_errorcode_task(TEST_TASK_ID, TASK_CAT1_UPDATA_REPLY,0);
        return;
    }
    
    // 检查升级失败的情况（END后面不是0）
    char *end_ptr = strstr(response, "\"END\"");
    if (end_ptr != NULL) {
        // 找到END，检查后面是否跟,0
        char *comma_ptr = strchr(end_ptr + 5, ',');
        if (comma_ptr != NULL) {
            int error_code = atoi(comma_ptr + 1);
            if (error_code != 0) {
                firmware_upgrade_state = 4; // 失败状态
                *status = CMD_STATUS_SUCCESS;
                
                // 通知上层固件升级失败
                production_cat1_send_errorcode_task(TEST_TASK_ID, TASK_CAT1_UPDATA_REPLY,1);
                return;
            }
        }
    }
    
    // 检查是否是错误响应
    if (strstr(response, "ERROR") != NULL) {
        firmware_upgrade_state = 4;
        *status = CMD_STATUS_FAILED;
        
        // 通知上层固件升级失败
        production_cat1_send_errorcode_task(TEST_TASK_ID, TASK_CAT1_UPDATA_REPLY,1);
        return;
    }
    
    // 检查进度信息
//    if (strstr(response, "DOWNLOADING") != NULL || strstr(response, "UPDATING") != NULL) {
//        char *comma_ptr = strrchr(response, ',');
//        if (comma_ptr != NULL) {
//            int progress = atoi(comma_ptr + 1);
//            log_debug("Firmware upgrade progress: %d%%\n", progress);
//        }
//    }
    
    // 升级进行中
    *status = CMD_STATUS_PENDING;
}

/**
 * @brief 解析PDP上下文状态数据（纯数据格式）
 * @param str 数据字符串，格式如："1,0" 或 "2,0"
 * @param len 字符串长度
 * @param status 返回状态：
 *               CMD_STATUS_SUCCESS - 至少有一个PDP上下文已激活
 *               CMD_STATUS_FAILED  - 所有PDP上下文都未激活或解析失败
 */
void at_cgact(uint8_t *str, uint16_t len, cmd_status_t *status)
{
    if (str == NULL)
    {
        *status = CMD_STATUS_FAILED;
        return;
    }

    // 查找逗号位置
    uint8_t *comma_pos = (uint8_t *)memchr(str, ',', len);
    
    if (comma_pos != NULL)
    {
        // 获取PDP ID（逗号前的数字）
        uint8_t pdp_id = 0;
        if (str < comma_pos)
        {
            pdp_id = *str - '0';
        }
        
        // 获取PDP状态（逗号后的数字）
        uint8_t pdp_state = 0;
        if (comma_pos + 1 < str + len)
        {
            pdp_state = *(comma_pos + 1) - '0';
        }
        
        // 只处理上下文1
        if (pdp_id == 1)
        {
            if (pdp_state == 1)
            {
                log_debug("[CAT1][STA] Context 1 is activated\r\n");
                // 上下文1已激活，可以执行后续指令
				reStartModeQiCgact();
            }
            else
            {
                log_debug("[CAT1][STA] Context 1 is not activated, setting APN\r\n");
                // 上下文1未激活，设置APN并激活
                lte_check_cmd_rtos(LTE_SET_APN, "OK\r\n", 1, 1000, 0);
                lte_check_cmd_rtos(LTE_SET_CGACT, "OK\r\n", 1, 1000, 0);
				lte_check_cmd_rtos(LTE_QIACT, "OK\r\n", 1, 1000, 0);
				lte_check_cmd_rtos(LTE_GET_CGACT, "OK\r\n", 1, 1000, 0);
            }
        }
        else
        {
            log_debug("[CAT1][STA] Ignoring context %d\r\n", pdp_id);
        }
        
        *status = CMD_STATUS_SUCCESS;
    }
    else
    {
        *status = CMD_STATUS_FAILED;
    }
}

/**
 * @brief  at_cmeError		ERROR 处理函数
 * @param str 数据
 * @param len 数据长度
 * @param status 指令执行状态
 * @return NULL
 **/
void at_cmeError(uint8_t *str, uint16_t len, cmd_status_t *status)
{
    if (currentCmd != NULL) {
        // 处理删除证书指令的错误
        if (currentCmd->type == LTE_QFDEL_CACERT || 
            currentCmd->type == LTE_QFDEL_CLIENT || 
            currentCmd->type == LTE_QFDEL_USERKEY) {

            // 检查错误码为418（文件不存在）
            if (strstr((char *)str, "418") != NULL) {
                log_debug("[CAT1][ERR] File not found error (418), certificate already deleted?\r\n");
                // 发送错误响应给TEST任务
                Message_Cmd_Put(CAT1_UART_TASK_ID, ENTRY_TASK_ID, TASK_FACTORY_RESET_REPLY, NULL, 0);
                Production_Result_Report(REPORT_CAT1_CMD_ERROR);
                *status = CMD_STATUS_SUCCESS;
            } else {
                Message_Cmd_Put(CAT1_UART_TASK_ID, ENTRY_TASK_ID, TASK_FACTORY_RESET_REPLY, NULL, 0);
                Production_Result_Report(REPORT_CAT1_CMD_ERROR);
                *status = CMD_STATUS_SUCCESS;
            }
            return;
        }
        // 处理写入证书指令的错误（文件名冲突）
        else if (currentCmd->type == LTE_QFUPL_CACERT || 
                 currentCmd->type == LTE_QFUPL_CLIENT || 
                 currentCmd->type == LTE_QFUPL_USERKEY) {
            
            // 错误码为407（文件名已存在）
            if (strstr((char *)str, "407") != NULL) {
                log_debug("[CAT1][ERR] File already exists error (407), attempting delete and retry: %d\r\n",currentCmd->retry_count);
                
				
                // 是否超过最大重试次数
//                if (currentCmd->retry_count >= 3) {
//                    log_debug("Max retry count (%d) reached, giving up\n", currentCmd->retry_count);
//                    
//                    // 根据证书类型发送对应的错误响应
//                    if (currentCmd->type == LTE_QFUPL_CACERT) {
//                        production_cat1_send_errorcode_task(TEST_TASK_ID, TASK_CAT1_AWS_CACERT_REPLY, 1);
//                    } else if (currentCmd->type == LTE_QFUPL_CLIENT) {
//                        production_cat1_send_errorcode_task(TEST_TASK_ID, TASK_CAT1_AWS_CLIENT_REPLY, 1);
//                    } else if (currentCmd->type == LTE_QFUPL_USERKEY) {
//                        production_cat1_send_errorcode_task(TEST_TASK_ID, TASK_CAT1_AWS_USERKEY_REPLY, 1);
//                    }
//                    
//                    *status = CMD_STATUS_SUCCESS;
//                    return;
//                }
//                
//                // 增加重试计数
////                currentCmd->retry_count++;
//                log_debug("Retry attempt %d for certificate write\n", currentCmd->retry_count);

                if (currentCmd->type == LTE_QFUPL_CACERT) {
					// 重置命令队列和状态
					osMutexAcquire(LteMutex, osWaitForever);
					lteCmdQueueHead = lteCmdQueueTail = 0;
					currentCmd = NULL;
					osMutexRelease(LteMutex);
                    lte_check_cmd_rtos(LTE_QFDEL_CACERT, "OK\r\n", 1, 3000, 0);
					start_cacert_config(CACERT_CA, b_message_data.appProduct_cacert, b_message_data.appProduct_cacertLen);
                } else if (currentCmd->type == LTE_QFUPL_CLIENT) {
					// 重置命令队列和状态
					osMutexAcquire(LteMutex, osWaitForever);
					lteCmdQueueHead = lteCmdQueueTail = 0;
					currentCmd = NULL;
					osMutexRelease(LteMutex);
                    lte_check_cmd_rtos(LTE_QFDEL_CLIENT, "OK\r\n", 1, 3000, 0);
					start_cacert_config(CACERT_CLIENT, b_message_data.appProduct_cacert, b_message_data.appProduct_cacertLen);
                } else { // LTE_QFUPL_USERKEY
					// 重置命令队列和状态
					osMutexAcquire(LteMutex, osWaitForever);
					lteCmdQueueHead = lteCmdQueueTail = 0;
					currentCmd = NULL;
					osMutexRelease(LteMutex);
                    lte_check_cmd_rtos(LTE_QFDEL_USERKEY, "OK\r\n", 1, 3000, 0);
					start_cacert_config(CACERT_USERKEY, b_message_data.appProduct_cacert, b_message_data.appProduct_cacertLen);
                }
                
                // 当前指令完成
                *status = CMD_STATUS_SUCCESS;
                return;
            }
            // 其他错误码的处理
            else {
                log_debug("[CAT1][ERR] Certificate write failed with error: %s\r\n", str);
                
                // 根据证书类型发送对应的错误响应
                if (currentCmd->type == LTE_QFUPL_CACERT) {
                    production_cat1_send_errorcode_task(TEST_TASK_ID, TASK_CAT1_AWS_CACERT_REPLY, 1);
                } else if (currentCmd->type == LTE_QFUPL_CLIENT) {
                    production_cat1_send_errorcode_task(TEST_TASK_ID, TASK_CAT1_AWS_CLIENT_REPLY, 1);
                } else if (currentCmd->type == LTE_QFUPL_USERKEY) {
                    production_cat1_send_errorcode_task(TEST_TASK_ID, TASK_CAT1_AWS_USERKEY_REPLY, 1);
                }
                
                *status = CMD_STATUS_SUCCESS;
                return;
            }
        }
        else {
            *status = CMD_STATUS_FAILED;
        }
    }
    else {
        *status = CMD_STATUS_FAILED;
    }
    
    // 默认返回失败
    *status = CMD_STATUS_FAILED;
}

void at_test(uint8_t *str, uint16_t len, cmd_status_t *status)
{
	*status = CMD_STATUS_SUCCESS;
}



/**
 * @brief at cmd argv		匹配AT指令列表
 * @return NULL
 */
const Recv_At_Func at_cmd_argv[] =
	{
		/*网络*/
		{RECV_AT_CREG,	 	"+CEREG", 		at_cereg},
		{RECV_AT_CGATT, 	"+CGATT", 		at_cgatt},
		{RECV_AT_CPIN, 		"+CPIN", 		at_cpin},
		{RECV_AT_CSQ, 		"+CSQ", 		at_csq},
		{RECV_AT_COPS, 		"+COPS", 		at_cops},
		{RECV_AT_QENG, 		"+QENG", 		at_qeng},
		
		/*MQTT通信指令*/
		{RECV_AT_QMTOPEN, 	"+QMTOPEN", 	at_test},
		{RECV_AT_QMTCONN, 	"+QMTCONN", 	at_conn},
		{RECV_AT_QMTSUB, 	"+QMTSUB", 		at_qmtsub},
		{RECV_AT_QMTUNS, 	"+QMTUNS", 		at_qmtuns},
		{RECV_AT_QMTCFG, 	"+QMTCFG", 		at_test},
		{RECV_AT_QMTCLOSE, 	"+QMTCLOSE", 	at_test},
		{RECV_AT_QMTDISCS, 	"+QMTDISC", 	at_test},
		{RECV_AT_QMTPING, 	"+QMTPING", 	at_test},
		
		{RECV_AT_QMTRECV, 	"+QMTRECV", 	at_mqttData},
		{RECV_AT_CGDCONT, 	"+CGDCONT", 	at_cgdcont},
		{RECV_AT_QMTSTAT, 	"+QMTSTAT", 	at_qmtstat},
		{RECV_AT_QMTPUBEX,  "+QMTPUBEX",    at_qmtpubex},
		
		{RECV_AT_WIFISCAN, 	"+QWIFISCAN", 	at_wifiScan},
		
		 /*HTTP和文件系统指令*/
		{RECV_AT_QHTTPREADFILE, "+QHTTPREADFILE", 	at_qhttpreadfile},
		{RECV_AT_QFLST,      	"+QFLST",       	at_qflst},
		{RECV_AT_QFDEL,      	"+QFDEL",       	at_test},
		{RECV_AT_QFOPEN,     	"+QFOPEN",      	at_qfopen},
		{RECV_AT_CONNECT, 		"CONNECT", 			at_connect},
		{RECV_AT_QHTTPGET, 		"+QHTTPGET", 		at_qhttpget},
		{RECV_AT_QFUPL, 		"+QFUPL", 			at_qfupl},
		
		{RECV_AT_GSN, 			"+GSN", 			at_device_cat1_sn},
		{RECV_AT_QCCID,			"+QCCID", 			at_device_esim_sn},
		{RECV_AT_UPDATA,		"+QIND", 			at_firmware_upgrade},
		{RECV_AT_CGACT,			"+CGACT", 			at_cgact},

		{RECV_AT_QIURC,			"+QIURC", 			at_test},
		
		/*错误指令*/
		{RECV_AT_ERROR, 		"+CME ERROR", 		at_cmeError},
		{RECV_AT_END, NULL, NULL}
	};

/**
 * @brief at cmd search		查询对应指令
 * @param ptr  				匹配字符串
 * @param len 				匹配字符串长度
 * @return res 				1-成功，0-失败
 */
uint16_t at_cmd_search(char *ptr, uint16_t len)
{
    int i = 0;
    char temp_buf[256];
    
    // 确保以\0结尾以便使用strstr
    if (len >= sizeof(temp_buf)) {
        len = sizeof(temp_buf) - 1;
    }
    memcpy(temp_buf, ptr, len);
    temp_buf[len] = '\0';
    
    for (i = 0; at_cmd_argv[i].recvAtCmd != RECV_AT_END; i++)
    {
        // strstr在字符串中搜索
        if (at_cmd_argv[i].str != NULL && strstr(temp_buf, at_cmd_argv[i].str) != NULL)
        {
            return i;  // 返回找到的索引
        }
    }
    
    return INDEX_ERROR;  // 没有找到任何AT命令
}

uint8_t check_powered_down(uint8_t *data, uint16_t len)
{
    if (data == NULL || len < 12) // "POWERED DOWN" 最少需要12字符
        return 0;
    
    // 确保字符串以null结尾
    char *search_str = (char *)data;

    if (strstr(search_str, "POWERED DOWN") != NULL)
    {
        return 1;
    }
    
    return 0;
}

uint8_t check_powered_on(uint8_t *data, uint16_t len)
{
    if (data == NULL || len < 3) // "RDY" 最少需要3字符
        return 0;
    
    // 确保字符串以null结尾
    char *search_str = (char *)data;

    if (strstr(search_str, "RDY") != NULL)
    {
        return 1;
    }
    
    return 0;
}

// 处理单行AT指令
static uint8_t process_single_line(uint8_t *ptr, uint16_t len, cmd_status_t *status)
{
	char *a_fistseat;
	uint16_t index = INDEX_ERROR;
	uint16_t res = 0;

	static uint8_t cmd_payload[30];
	static uint8_t param_payload[MQTT_DATA_MTU_MAX];

	memset(cmd_payload, '\0', sizeof(cmd_payload));
	memset(param_payload, '\0', sizeof(param_payload));

	// 处理关机指令
	if (check_powered_down(ptr, len))
	{
		log_debug("[CAT1][STA] PowerDown\r\n");
			if(production_flag.flag_set_sn)
			{
				production_flag.flag_set_sn = 0;
				lteTestEventPowerDown();
			}
			else if(production_flag.flag_get_version)
			{
				production_flag.flag_get_version = 0;
				lteTestEventPowerDown();
			}
			else if(production_flag.flag_cat1_conn)
			{
				production_flag.flag_cat1_conn = 0;
				lteTestEventPowerDown();
			}
			else if(production_flag.flag_set_cacert)
			{
				production_flag.flag_set_cacert = 0;
				Production_Result_Report(get_product_errorcode());
				lteTestEventPowerDown();
			}
			else if(production_flag.flag_set_client)
			{
				production_flag.flag_set_client = 0;
				Production_Result_Report(get_product_errorcode());
				lteTestEventPowerDown();
			}
			else if(production_flag.flag_set_userkey)
			{
				production_flag.flag_set_userkey = 0;
				Production_Result_Report(get_product_errorcode());
				lteTestEventPowerDown();
			}
			else if(production_flag.flag_factory_reset)
			{
				production_flag.flag_factory_reset = 0;
				lteEntryEventPowerDown();
			}
			else if(production_flag.flag_set_lte_updata)
			{
				production_flag.flag_set_lte_updata = 0;
				lteTestEventPowerDown();
			}
			else if(production_flag.flag_set_lte_usb_updata)
			{
				production_flag.flag_set_lte_usb_updata = 0;
				lteTestEventPowerDown();
			}
			else
			{
				// 恢复过程中（lteInit开机/60s超时POWERKEY）的POWERED DOWN，跳过lteEventPowerDown
				if (is_lte_init_recovery) {
					log_debug("[CAT1][STA] PowerDown ignored\r\n");
					if(!checkCat1PowerState())
					{
						is_lte_init_recovery = false;  // 恢复过程结束
//						log_debug("[CAT1][STA] retry power on\r\n");
						lteInit();
					}
					//连接状态查询？
				} else {
					lteEventPowerDown();
				}
			}
			*status = CMD_STATUS_SUCCESS;
		return 1;
	}
	
	// 处理4G开机的指令
//	else if (len >= 3 && ptr[0] == 'R' && ptr[1] == 'D' && ptr[2] == 'Y') 
	else if (check_powered_on(ptr, len))
	{
		log_debug("[CAT1][STA] Ready!\r\n");
		lte_recovery_clear();
		is_lte_init_recovery = false;  // RDY到达，恢复过程结束
			reset_cat1_state_before_poweron();
			lteCloseTimer();
			if(production_flag.flag_set_sn)
			{
				lteSnInfo();
			}
			else if(production_flag.flag_get_version)
			{
				//查询版本号
				lteVersion();
			}
			else if(production_flag.flag_cat1_conn)
			{
				//测试连接
				lteTestHighspeedModeConn();
			}
			else if(production_flag.flag_set_cacert)
			{
				start_cacert_config(CACERT_CA, b_message_data.appProduct_cacert, b_message_data.appProduct_cacertLen);
			}
			else if(production_flag.flag_set_client)
			{
				start_cacert_config(CACERT_CLIENT, b_message_data.appProduct_cacert, b_message_data.appProduct_cacertLen);
			}
			else if(production_flag.flag_set_userkey)
			{
				start_cacert_config(CACERT_USERKEY, b_message_data.appProduct_cacert, b_message_data.appProduct_cacertLen);
			}
			else if(production_flag.flag_factory_reset)
			{
				lteTestDeleteCacert();
			}
			else if(production_flag.flag_set_lte_updata)
			{
				if(firmware_upgrade_state != 3 && firmware_upgrade_state != 4)
				{
					//b_message_data.appControl_lteFirmwareUrl  strlen((char *)b_message_data.appControl_lteFirmwareUrl)
					start_delta_firmware((char *)b_message_data.appControl_lteFirmwareUrl, strlen((char *)b_message_data.appControl_lteFirmwareUrl));
//					start_delta_firmware("http://4c5966cd.r10.cpolar.top/download/signed_EG800QEULCR01A11M04_A0.300.A0.300-EG800QEULCR01A11M04_BETA2025071002.par", 119);
				}
				// 重置状态
				firmware_upgrade_state = 0;
			}
			else if(production_flag.flag_set_lte_usb_updata)
			{
				//只开机，硬件操作
			}
			else
			{
				if(Cat1RecvMode == MODE_STANDARD)
				{
					cat1CheckMqttStateTimeout = 30*60*1000;
				}
				else if(Cat1RecvMode == MODE_SEARCH_PET)
				{
					cat1CheckMqttStateTimeout = 3*60*1000;
				}
				
				if (checkMqttState_startTimer_ID) {
					if(osTimerIsRunning(checkMqttState_startTimer_ID))
					{
						osTimerStop(checkMqttState_startTimer_ID);	
					}
					osTimerStart(checkMqttState_startTimer_ID, cat1CheckMqttStateTimeout);
				}
				// 启动定时器
				if (checkDeviceState_startTimer_ID) {
					if (osTimerIsRunning(checkDeviceState_startTimer_ID)) {
						osTimerStop(checkDeviceState_startTimer_ID);
					}
					osTimerStart(checkDeviceState_startTimer_ID, cat1CheckDeviceStateTimeout);
				}
	
				set_cat1_state(LTE_TASK_RUNNING);
				lteHighspeedModeConn();
			}
		*status = CMD_STATUS_SUCCESS;
		return 1;
	}
	
	// 处理版本号响应
	else if (len >= 5 && strncmp((char *)ptr, "EG800Q", 5) == 0)
	{
		at_device_version(ptr, len, status);
		return 1;
	}
	
	// 处理CONNECT响应
	else if (len >= 7 && strncmp((char *)ptr, "CONNECT", 7) == 0)
	{
		if (currentCmd != NULL)
		{
			at_connect(ptr, len, status);
		}
		else
		{
			*status = CMD_STATUS_SUCCESS;
		}
		return 1;
	}

	// 查找冒号位置
	a_fistseat = strchr((char *)ptr, ':');
	if (a_fistseat != NULL)
	{
		// 解析 CMD 字符串 cmd_payload
		uint16_t cmd_len = a_fistseat - (char *)ptr;
		if (cmd_len >= sizeof(cmd_payload))
		{
			return 0;
		}
		
		strncpy((char *)cmd_payload, (char *)ptr, cmd_len);
		cmd_payload[cmd_len] = '\0';

		// 在这里进行命令查找
		index = at_cmd_search((char *)cmd_payload, cmd_len);

		// 解析参数 param_payload
		char *param_start = a_fistseat + 1; // 指向冒号后面的字符
		if (*param_start == ' ')
			param_start++; // 跳过空格

		// 计算参数长度，直到 \r\n 或字符串结束
		char *param_end = strstr(param_start, "\r\n");
		if (param_end == NULL)
			param_end = (char *)ptr + len; // 使用传入的长度

		uint16_t param_len = param_end - param_start;
		if (param_len >= sizeof(param_payload))
		{
			return 0;
		}
		
		strncpy((char *)param_payload, param_start, param_len);
		param_payload[param_len] = '\0';
		
		if (index != INDEX_ERROR)
		{
			// 对于WiFi扫描等命令，传递整个剩余数据用于解析多个条目
			if (index == RECV_AT_WIFISCAN || index == RECV_AT_QFLST || 
				index == RECV_AT_QMTRECV || index == RECV_AT_QMTSUB)
			{
				// 对于这些命令，传递整行数据
				at_cmd_argv[index].exe(ptr, len, status);
			}
			else
			{
				at_cmd_argv[index].exe(param_payload, param_len, status);
			}
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		// 检查是否是OK/ERROR等无冒号的响应
		if (len == 2 && strncmp((char *)ptr, "OK", 2) == 0)
		{
			log_debug("[CAT1][STA] OK response\r\n");
			*status = CMD_STATUS_SUCCESS;
			return 1;
		}
		else if (len >= 5 && strncmp((char *)ptr, "ERROR", 5) == 0)
		{
			log_debug("[CAT1][ERR] ERROR response\r\n");
			*status = CMD_STATUS_FAILED;
			return 1;
		}
		return 0;
	}
}

/**
 * @brief at cmd analysis	带参数的串口数据解析参数
 * @param ptr  				解析的数据
 * @param len 				解析的数据长度
 * @return res 				1-成功，0-失败
 */
uint8_t at_cmd_analysis(uint8_t *ptr, uint16_t len, cmd_status_t *status)
{
	char *a_fistseat;
	char a_cmd = ':';
	uint16_t index = INDEX_ERROR;
	uint16_t res = 0;

	static uint8_t cmd_payload[30];
	static uint8_t param_payload[MQTT_DATA_MTU_MAX];

	memset(cmd_payload, '\0', sizeof(cmd_payload));
	memset(param_payload, '\0', sizeof(param_payload));

	if (ptr == NULL)
	{
		return res;
	}

	// 检查是否以\r\n结尾（原有逻辑）
	if (len < 2 || (ptr[len - 2] != '\r' && ptr[len - 1] != '\n'))
	{
		return 0;
	}

	// 跳过前面的 \r\n
	while (len > 0 && (*ptr == '\r' || *ptr == '\n'))
	{
		ptr++;
		len--;
	}

	if (len == 0)
	{
		return 0;
	}

    // 查找多种可能的分隔符
    char *next_cmd = NULL;
    uint16_t first_cmd_len = 0;
    
    // 1. 先查找 \r\n+ (以+开头的指令)
    next_cmd = strstr((char *)ptr + 1, "\r\n+");
    
    // 2. 如果没有找到，查找 \r\nOK\r\n
    if (next_cmd == NULL)
    {
        char *ok_pos = strstr((char *)ptr, "\r\nOK\r\n");
        if (ok_pos != NULL)
        {
            // 找到OK，它本身是一条完整的指令
            next_cmd = ok_pos + 2; // 指向OK后面的\r\n
        }
    }
    
    // 3. 如果没有找到，查找 \r\nERROR\r\n
    if (next_cmd == NULL)
    {
        char *error_pos = strstr((char *)ptr, "\r\nERROR\r\n");
        if (error_pos != NULL)
        {
            next_cmd = error_pos + 2; // 指向ERROR后面的\r\n
        }
    }
    
    // 4. 如果还没找到，查找 \r\n 后跟数字或字母的（其他响应）
    if (next_cmd == NULL)
    {
        // 查找下一个\r\n
        char *search_ptr = (char *)ptr + 1;
        while ((search_ptr = strstr(search_ptr, "\r\n")) != NULL)
        {
            // 检查\r\n后面的字符
            if (search_ptr + 2 < (char *)ptr + len)
            {
                char next_char = search_ptr[2];
                // 如果是数字、字母（但不是+），可能是一条新指令
                if ((next_char >= '0' && next_char <= '9') || 
                    (next_char >= 'A' && next_char <= 'Z') ||
                    (next_char >= 'a' && next_char <= 'z'))
                {
                    next_cmd = search_ptr;
                    break;
                }
            }
            search_ptr += 2;
        }
    }

    if (next_cmd != NULL)
    {
        // 找到多条指令，先处理第一条指令
        first_cmd_len = next_cmd - (char *)ptr;
        
        // 特殊处理：如果第一条指令包含OK，确保包含完整的OK\r\n
        char *ok_check = strstr((char *)ptr, "OK\r\n");
        if (ok_check != NULL && ok_check < next_cmd)
        {
            // 调整长度到OK\r\n结束
            first_cmd_len = (ok_check + 4) - (char *)ptr;
        }
        
        // 特殊处理：如果第一条指令包含ERROR，确保包含完整的ERROR\r\n
        char *error_check = strstr((char *)ptr, "ERROR\r\n");
        if (error_check != NULL && error_check < next_cmd)
        {
            first_cmd_len = (error_check + 7) - (char *)ptr;
        }
       
        res = process_single_line(ptr, first_cmd_len, status);

        // 关机确认后终止本帧解析：POWERED DOWN 同帧携带的 +QIURC/+QMTSTAT 等
        if (res && check_powered_down(ptr, first_cmd_len))
        {
            return res;
        }
        
        if (res)
        {
            // 处理剩余指令（递归调用）
            char *remaining_data = next_cmd;
            
            // 跳过分隔符
            while (remaining_data < (char *)ptr + len && 
                   (*remaining_data == '\r' || *remaining_data == '\n'))
            {
                remaining_data++;
            }
            
            uint16_t remaining_len = len - (remaining_data - (char *)ptr);
            
            if (remaining_len > 0)
            {
                // 递归处理剩余指令
                uint8_t remaining_res = at_cmd_analysis((uint8_t *)remaining_data, remaining_len, status);
                res = res || remaining_res;
            }
        }
        return res;
    }
    
    return process_single_line(ptr, len, status);
}

static bool is_only_ok_response(uint8_t *recv_buf, uint16_t recv_len)
{
    if (recv_buf == NULL || recv_len < 2) {
        return false;
    }
    
    char *response = (char *)recv_buf;
    
    // 去除前后的空白字符
    while (*response == ' ' || *response == '\r' || *response == '\n') {
        response++;
        recv_len--;
    }
    
    // 检查剩余部分是否以"OK"开头
    if (recv_len >= 2 && response[0] == 'O' && response[1] == 'K') {
        return true;
    }
    
    return false;
}

static bool response_is_bare_ok(uint8_t *recv_buf, uint16_t recv_len)
{
	char *s = (char *)recv_buf;
	if (!s || recv_len < 2) return false;

	if (strstr(s, "+QMTCONN:") != NULL) return false;

	if (strstr(s, "ERROR") != NULL) return false;

	if (is_only_ok_response(recv_buf, recv_len)) return true;

	if (strstr(s, "\r\nOK") != NULL) return true;
	if (strstr(s, "\nOK")   != NULL) return true;

	return false;
}

/**
 * @brief process_response	lte任务解析串口回复的数据
 * @param currentCmd 		当前指令
 * @param recv_buf 		接收缓冲区
 * @param recv_len 		接收长度
 * @return ResponseResult 	响应结果
 */
ResponseResult process_response(lteCmdItem_t *currentCmd, uint8_t *recv_buf, uint16_t recv_len)
{
    ResponseResult result = RESPONSE_NONE;
    cmd_status_t cmd_status = CMD_STATUS_PENDING;
	bool urc_processed = false;
	
    if (currentCmd && production_flag.flag_factory_reset && is_only_ok_response(recv_buf, recv_len)) {
        if (currentCmd->type == LTE_QFLST && get_certificate_deletion_mode()) {
            // 没有找到证书文件，             是正常情况
            log_debug("[CAT1][STA] QFLST in deletion mode: no certificates found\r\n");
            set_certificate_deletion_mode(false);
            Message_Cmd_Put(CAT1_UART_TASK_ID,ENTRY_TASK_ID,TASK_FACTORY_RESET_REPLY,NULL,0);
            Production_Result_Report(REPORT_OK);
        }
        
        result = RESPONSE_SUCCESS;
        return result;
    }
    
	// AT+QMTCONN? 只回OK（无+QMTCONN行）
	if (currentCmd && currentCmd->type == LTE_MQTT_ISSTATE && !qmtconn_info_seen &&
	    response_is_bare_ok(recv_buf, recv_len))
	{
		log_debug("[CAT1][STA] QMTCONN? bare OK, no mqtt session, restart conn\r\n");
		isMqttConnected = false;
		if (!is_reconnecting) {
			mqtt_disconnect_flush_queue();
			reStartModeConn();
		}
		return RESPONSE_COMPLETE;
	}

	// AT+QWIFISCAN 的裸 OK ACK（不含 URC）→ 忽略，继续等 URC
	if (currentCmd && currentCmd->type == LTE_WIFI_SCAN &&
	    is_only_ok_response(recv_buf, recv_len)) {
		return RESPONSE_NONE;
	}

	// 检查是否包含URC命令
	bool is_urc = false;
    for (int i = 0; urc_commands[i] != NULL; i++) {
        if (strstr((char *)recv_buf, urc_commands[i]) != NULL) {
            is_urc = true;
            break;
        }
    }
    // 如果有URC，无论是否有当前命令，都要先解析处理（不丢失主动上报）
    if (is_urc) {
		// 复制一份缓冲区，避免 URC 解析代码破坏原始数据
		uint8_t urc_buf[MQTT_DATA_URC];
		uint16_t copy_len = (recv_len < MQTT_DATA_URC) ? recv_len : (MQTT_DATA_URC - 1);
		memcpy(urc_buf, recv_buf, copy_len);
		urc_buf[copy_len] = '\0';
		
		cmd_status_t urc_status;
		at_cmd_analysis(urc_buf, recv_len, &urc_status);  // ← 用副本
		log_debug("[CAT1][DAT] URC processed, urc_status=%d\r\n", urc_status);
		urc_processed = true;
        //继续检查命令响应
    }
	
    // WIFI_SCAN：命令完成只认扫描自己的信号（+QWIFISCAN 结果+OK，或 TIMEOUT/ERROR）。
    if (currentCmd && currentCmd->type == LTE_WIFI_SCAN) {
        if (strstr((char *)recv_buf, "+QWIFISCAN") == NULL &&
            strstr((char *)recv_buf, "TIMEOUT") == NULL &&
            strstr((char *)recv_buf, "ERROR") == NULL) {
            return RESPONSE_NONE;
        }
    }

    // 优先处理MQTT PUB和PUBMESSAGEDATA指令的响应
    if (currentCmd && (currentCmd->type == LTE_MQTT_PUB || currentCmd->type == LTE_MQTT_PUBMESSAGEDATA))
    {
        // 检查是否收到ERROR
        if (strstr((char *)recv_buf, "\r\nERROR\r\n") != NULL || 
            strstr((char *)recv_buf, "ERROR\r\n") != NULL) {
            log_debug("[CAT1][ERR] MQTT command ERROR, need retry\r\n");
            result = RESPONSE_ERROR;
            return result;
        }
        
        // 检查是否收到期望的ACK
        bool has_ack = (strstr((char *)recv_buf, currentCmd->ack) != NULL);
        
        // PUB(等待>) + OK\r\n 被EG800Q打包成一帧的特殊处理:
        // > 匹配PUB的ACK, 但OK\r\n在帧里被吞 → PUBMESSAGEDATA永远等不到OK → g_mqtt_sending永不清零
        // 发现>+OK同帧时: 立刻清g_mqtt_sending + mqtt_send_done_handler + 标记skip_pubmsgd
        if (has_ack && currentCmd->type == LTE_MQTT_PUB && 
            strstr((char *)recv_buf, "OK\r\n") != NULL &&
            strstr((char *)recv_buf, "+QWIFISCAN") == NULL) {
            g_mqtt_sending = 0;
            mqtt_send_start_tick = 0;
            g_current_send_type = 0;
            if (is_auto_reporting) { g_current_send_type = 2; }
            else                    { g_current_send_type = 1; }
            if (is_auto_reporting)  { is_auto_reporting = false; }
            mqtt_send_done_handler(g_current_send_type);
            g_skip_pubmsgd = true;  // 通知main loop跳过下一条PUBMESSAGEDATA
            result = RESPONSE_SUCCESS;
            return result;
        }
        
        // 收到ACK，返回成功
        if (has_ack) {
            log_debug("[CAT1][STA] MQTT command ACK (%s) received\r\n", currentCmd->ack);
            result = RESPONSE_SUCCESS;

			if (currentCmd->type == LTE_MQTT_PUBMESSAGEDATA) {
				char *urc_start = strstr((char *)recv_buf, "+QMTPUBEX");
				if (urc_start != NULL) {
					cmd_status_t urc_st;
					at_cmd_analysis((uint8_t *)urc_start,
					                recv_len - (uint16_t)(urc_start - (char *)recv_buf), &urc_st);
				}
			}

			if(currentCmd->type == LTE_MQTT_PUBMESSAGEDATA)
			{
				queue_entry_tick = 0;  // ← 处理完成，清零
				g_mqtt_sending = 0;
				mqtt_send_start_tick = 0;
				g_current_send_type = 0;
				//连接状态恢复
				restartMqttConnectState();
				
				// 保存当前发送类型
				if (is_auto_reporting) {
					g_current_send_type = 2;  // 主动上报
				} else {
					g_current_send_type = 1;  // 云指令
				}
				
				// 清除主动上报标志（如果当前完成的是主动上报）
				if (is_auto_reporting) {
					is_auto_reporting = false;
				}
				
				// 检查队列中是否有下一包数据
				mqtt_send_done_handler(g_current_send_type);
			}
            return result;
        }

        // 修复: has_ack=false时返回RESPONSE_NONE, 不释放currentCmd
        // 之前返回RESPONSE_SUCCESS导致currentCmd被释放但g_mqtt_sending不清 → 永久死锁
        log_debug("[CAT1][DBG] MQTT ack NOT matched, keeping currentCmd type=%d\r\n", currentCmd->type);
        result = RESPONSE_NONE;
        return result;
    }
    
    // 检查是否为固件升级命令
    if (currentCmd && currentCmd->is_firmware_upgrade) {
        // 多条响应
        char *response = (char *)recv_buf;
        
        // "OK"（第一响应）
        if (strstr(response, "OK") != NULL && 
            (strstr(response, "+QIND") == NULL) && 
            (strstr(response, "FOTA") == NULL)) {
            result = RESPONSE_SUCCESS;
            return result;
        }
			
        if (at_cmd_analysis(recv_buf, recv_len, &cmd_status)) {
            currentCmd->status = cmd_status;
            
            if (cmd_status == CMD_STATUS_FAILED) {
                result = RESPONSE_ERROR;
            } 
            else if (cmd_status == CMD_STATUS_SUCCESS) {
                result = RESPONSE_COMPLETE;
            }
            else {
                // CMD_STATUS_PENDING 表示还在进行中
                result = RESPONSE_SUCCESS;
            }
        }
        else {
            // 无法解析的响应，保持等待
            result = RESPONSE_SUCCESS;
        }
        
        return result;
    }

    // 有当前指令时的处理
    if (!urc_processed && at_cmd_analysis(recv_buf, recv_len, &cmd_status))
    {
        if (currentCmd)
        {
            currentCmd->status = cmd_status;

            if (cmd_status == CMD_STATUS_FAILED)
            {
//                log_debug("[CAT1][ERR] enter at cmd analysis\r\n");
                result = RESPONSE_ERROR;
            }
            else if (cmd_status == CMD_STATUS_SUCCESS)
            {
                // 只有当这是第二包数据（+XXX格式）并且包含OK时，才认为是完整响应
                if (currentCmd->has_extra_response && 
                    (strstr((char *)recv_buf, "\r\nOK\r\n") != NULL || 
                     strstr((char *)recv_buf, "OK\r\n") != NULL))
                {
                    // 这是第二包数据，同时包含+XXX和OK，完整响应
                    result = RESPONSE_COMPLETE;
					
					// 此时MQTT网络注册成功，主题订阅完成并且第一次进入M5模式而非CAT1重启
//                    if ((currentCmd->type == LTE_MQTT_SUB_WARNINGSTATE) && first_enter_M5_mode) {
//                        log_debug("LTE_MQTT_SUB_WARNINGSTATE response complete, sending messages\n");
//						first_enter_M5_mode = false;
//                        cat1_send_reply_task(COMM_TASK_ID, TASK_COMM_MODE_REPORT);
//                    }
                }
                else
                {
                    // 第一包数据，只有+XXX没有OK，继续等待
                    result = RESPONSE_SUCCESS;
                }
            }
            else
            {
                result = RESPONSE_ERROR;
            }
        }
        else
        {
            // 无指令时，主动上报的数据（如+QMTRECV）
            result = RESPONSE_SUCCESS;
        }
    }
    else if (currentCmd && strnstr((char *)recv_buf, currentCmd->ack, recv_len))
    {
        // 这是第一包数据（ACK响应），继续等待第二包
        result = RESPONSE_SUCCESS;
    }
    else if (strstr((char *)recv_buf, "\r\nERROR\r\n"))
    {
        result = RESPONSE_ERROR;
    }
    else
    {
        //当前有指令
        if (currentCmd)
        {
            bool is_urc = false;
        
            // 遍历URC命令列表
            for (int i = 0; urc_commands[i] != NULL; i++) {
                if (strstr((char *)recv_buf, urc_commands[i]) != NULL) {
                    is_urc = true;
                    break;
                }
            }
            
            if (is_urc) {
//                cmd_status_t urc_status;
//                // 解析并处理URC数据（会调用对应的处理函数，如at_mqttData）
//                at_cmd_analysis(recv_buf, recv_len, &urc_status);
//                // URC数据已处理，返回ERROR让上层继续等待（不触发重发）
                result = RESPONSE_SUCCESS;
            }
            else {
                // 不是URC，也不是命令响应，可能是乱码或无效数据
                log_debug("[CAT1][ERR] Unknown data (not URC), discarding and returning ERROR\r\n");
                result = RESPONSE_ERROR;
            }
        }
        else
        {
            //无指令，主动上报的未知的URC，不需要处理，直接返回success
            result = RESPONSE_SUCCESS;
        }
        
    }
    return result;
}

/**
 * @brief 获取CAT1版本号（直接返回指针）
 * @return 返回CAT1版本号的指针，失败返回NULL
 **/
uint8_t* DEVICE_GetCat1Version(void)
{
	if (strlen((char *)cat1Version) > 0)
	{
		return cat1Version;
	}

    return NULL;
}

/**
 * @brief  CAT1_GetCurrentCsq		cat1 信号质量获取
 * @param cat1_data 数据
 * @return NULL
 **/
uint8_t CAT1_GetCurrentCsq(void)
{
	uint8_t csq_value = 99;  // 默认无信号
	
	// 优先从缓存读取
    if (g_cache_mutex != NULL) {
        osMutexAcquire(g_cache_mutex, osWaitForever);
        if (g_device_cache.csq.update_time != 0) {
            csq_value = g_device_cache.csq.value;
            osMutexRelease(g_cache_mutex);
            return csq_value;
        }
        osMutexRelease(g_cache_mutex);
    }
	
    return cat1_Status.cat1_signal;
}

/**
 * @brief 获取CAT1序列号（直接返回指针）
 * @return 返回CAT1 SN的指针，失败返回NULL
 **/
uint8_t* DEVICE_GetCat1Sn(void)
{
    if (osMutexAcquire(LteDeviceInfoMutex, osWaitForever) == osOK)
    {
        if (strlen((char *)cat1Sn) > 0)
        {
            osMutexRelease(LteDeviceInfoMutex);
            return cat1Sn;
        }
        osMutexRelease(LteDeviceInfoMutex);
    }
    return NULL;
}

/**
 * @brief 获取基站信息
 * @param cell_info_out 输出参数，存储基站信息
 * @return bool true-成功 false-失败
 **/
bool CELL_GetCurrentData(CellInfo_t *cell_info_out)
{
    if (cell_info_out == NULL) {
        return false;
    }
    
//    osMutexAcquire(CellInfoMutex, osWaitForever);
//    memcpy(cell_info_out, &cell_info, sizeof(CellInfo_t));
//    osMutexRelease(CellInfoMutex);
    
	// 从缓存读取
	if (g_cache_mutex != NULL) {
      osMutexAcquire(g_cache_mutex, osWaitForever);
      memcpy(cell_info_out, &g_device_cache.cell_info.info, sizeof(CellInfo_t));
      osMutexRelease(g_cache_mutex);
	}
    return true;
}

// 删除设备回包完成后延时回调
static void DeleteResponseTimerCallback(void *argument)
{
    log_debug("[CAT1][STA] DeleteResponseTimer fired\r\n");
	Message_Cmd_Put(CAT1_UART_TASK_ID,ENTRY_TASK_ID,TASK_CAT1_DELETE_DEVICE,NULL,0);
}

void TimerCallback_checkMqttState(void *argument)
{
	log_debug("[CAT1][STA] TimerCallback checkMqtt\r\n");
	if(g_mqtt_sending == 0 || !isMqttConnected)
	{
		lteMqttConnState();
	}
	else
	{
		// 重启定时器，下次再检查
        if(osTimerIsRunning(checkMqttState_startTimer_ID))
        {
            osTimerStop(checkMqttState_startTimer_ID);    
        }
        osTimerStart(checkMqttState_startTimer_ID, cat1CheckMqttStateTimeout);
	}
	
}

void TimerCallback_checkDeviceState(void *argument)
{
	log_debug("[CAT1][STA] Device TimerCallback\r\n");
	
		if(g_mqtt_sending != 0 || is_auto_reporting)
		{	
		if (queue_entry_tick != 0) {
			uint32_t elapsed = osKernelGetTickCount() - queue_entry_tick;
			if (elapsed >= osMS2TicksRound(2 * 60 * 1000)) {
				log_debug("[CAT1][DAT] queue non-empty >2min (%lu ms), reset all\r\n", elapsed);
				g_mqtt_sending = 0;
				mqtt_send_start_tick = 0;
				is_auto_reporting = false;
				g_current_send_type = 0;
				queue_entry_tick = 0;
				is_retransmitting = false; 
				if (g_mqtt_queue) mqtt_queue_clear(g_mqtt_queue);
			}

			// g_mqtt_sending 看门狗: 30秒收不到LTE响应, 强制恢复
			if (g_mqtt_sending == 1 && mqtt_send_start_tick != 0) {
				uint32_t send_elapsed = osKernelGetTickCount() - mqtt_send_start_tick;
				if (send_elapsed >= osMS2TicksRound(30 * 1000)) {
					log_debug("[CAT1][WDT] MQTT send timeout 30s, force reset\r\n");
					g_mqtt_sending = 0;
					mqtt_send_start_tick = 0;
					// 检查并处理暂存的COMM消息
					if (pending_buf_count > 0) {
						unsigned char *data = pending_buf_data[pending_buf_head];
						unsigned int  len   = pending_buf_len[pending_buf_head];
						pending_buf_head = (pending_buf_head + 1) % PENDING_BUF_SIZE;
						pending_buf_count--;
						mqtt_publish(data, len);
						DEMO_BT_Free(data);
					}
				}
			}	
		}
		else
		{
		}
	}
	
	if(g_mqtt_sending == 0)
	{
		lteQueryDeviceInfo();
	}
	else
	{
		// 重启定时器，下次再查询
        if(osTimerIsRunning(checkDeviceState_startTimer_ID))
        {
            osTimerStop(checkDeviceState_startTimer_ID);    
        }
        osTimerStart(checkDeviceState_startTimer_ID, cat1CheckDeviceStateTimeout);
	}
}

bool checkCat1PowerState(void)
{
	send_to_uartTask(TASK_CMD_START);
	
	is_power_checking = true;
	set_cat1_state(LTE_TASK_INIT);
	
	// 清残留数据(重启/模式切换后模组可能有旧的OK/+QMTPUBEX在FIFO里) + 发送AT
	clearPacketList();
	lteAt();

	// 死等1秒，等待OK响应
	osStatus_t sem_status = osSemaphoreAcquire(Cat1PowerCheckSem, osMS2TicksRound(CAT1_POWER_CHECK_TIMEOUT));

	// 电源检测完成
	is_power_checking = false;
	
	if (sem_status == osOK) {
		log_debug("[CAT1][STA] hardware is Power on\r\n");
		// 检活成功: 清除残留的命令状态和UART数据, 确保后续命令从干净状态开始
		if (currentCmd) {
			osMutexAcquire(LteMutex, osWaitForever);
			currentCmd = NULL;
			osMutexRelease(LteMutex);
		}
		g_mqtt_sending = 0;
		mqtt_send_start_tick = 0;
		clearPacketList();  // 丢弃UART残留数据, 避免干扰下一条命令的响应解析
		return true;
	} else {
		// 超时未收到OK
		log_debug("[CAT1][STA] hardware is Power off\r\n");
		return false;
	}
}

/**
 * @brief  schedule task
 *
 * @param[in] pvParameters  pv parameters
 **/
static void vCAT1UartTask(void *argument)
{
	// TaskInfo_t *my_info = (TaskInfo_t *)pvParameters;
	TaskInfo_t *my_task_info = GetTaskInfo(CAT1_UART_TASK_ID);
	TaskInfo_t *cat1_uart_task_info = GetTaskInfo(UART_DATARECV_ID);
	uint32_t uxBits; // 事件组标志位

	Message_t cat1_receive_msg;
	static uint8_t recv_buf[MQTT_DATA_MTU_MAX];
	static uint16_t recv_len = 0;
	static TickType_t LastWakeTime = 0;

	log_debug("[CAT1][STA] Task %d started\r\n", my_task_info->task_id);

	static uint8_t expecting_extra_response = 0; // 标记是否期待额外响应
	
	//根据模式定时查询连接状态
	checkMqttState_startTimer_ID = osTimerNew(TimerCallback_checkMqttState,checkMqttState_startTimer_type,NULL,&checkMqttState_startTimer_attr);
	if(!checkMqttState_startTimer_ID)
		LOG_LOC();
	
	//1分钟查询一次信号量和基站信息
	checkDeviceState_startTimer_ID = osTimerNew(TimerCallback_checkDeviceState,checkDeviceState_startTimer_type,NULL,&checkDeviceState_startTimer_attr);
	if(!checkDeviceState_startTimer_ID)
		LOG_LOC();
	
	// 删除设备回包延时定时器
	deleteResponseTimer_ID = osTimerNew(DeleteResponseTimerCallback, deleteResponseTimer_type, NULL, &deleteResponseTimer_attr);
	if (!deleteResponseTimer_ID)
		LOG_LOC();

	// 主动上报定时器（已迁移到COMM任务，CAT1不再负责主动上报定时）

	for (;;)
	{
		// 检查任务是否应该阻塞
        if (should_task_block()) {
            log_debug("[CAT1][STA] task blocked, waiting for START command\r\n");
            wait_for_task_unblock();
            
            // 重新设置任务状态为运行中
            set_cat1_state(LTE_TASK_RUNNING);
        }
		
		// 接收串口任务消息及数据
		if (osOK == osMessageQueueGet(my_task_info->queue_handle, &cat1_receive_msg, NULL, 100))
		{
			// 回复消息给Entry任务
			if (cat1_receive_msg.source_id == ENTRY_TASK_ID)
			{
				if (cat1_receive_msg.command == TASK_CMD_START)
				{
					/* M3等模式切换：模组退出数据态、清掉
					 * 在途PUB指令，再做检活。 */
					if (g_pubdata_pending_len > 0 || g_mqtt_sending != 0) {
						cat1_escape_pubdata_mode();
						mqtt_disconnect_flush_queue();
					}
					if(!checkCat1PowerState())
					{
						// 开机前清队列：防止关机期间残留的指令
						reset_cat1_state_before_poweron();
						unblock_cat1_task();  // 解除阻塞
						osEventFlagsSet(LteEventId, LTE_EVENT_TASK_START);
						first_enter_M5_mode = true;
						// 初始化LTE
						is_lte_init_recovery = true;  // 标记恢复过程，阻止POWERED DOWN时阻塞任务
						lteInit();
					}
					else
					{
						//不重复执行
						lte_recovery_clear();
						set_cat1_state(LTE_TASK_RUNNING);
						unblock_cat1_task();  // 解除阻塞
						//检查网络是否正常连接
	//					lteMqttConnState();
//						drv_gpio_write(OM_GPIO0, GPIO_MASK(PAD_CAT1_DTR), GPIO_LEVEL_LOW);
						g_need_subscribe_on_reconnect = true;  // M5热恢复时需补订阅
						lte_check_cmd_rtos(LTE_MQTT_ISSTATE, "OK\r\n", 1, 500, 1);
					}
				}
				else if (cat1_receive_msg.command == TASK_CMD_STOP)
				{
					if(checkCat1PowerState())
					{
						is_lte_init_recovery = false;
						ltePowerDownPrepare();
						// 断开网络连接
						lteMqttDisConn();

						// 关机
						lteShutdown();
					}
					else
					{
						lteEventPowerDown();
						
					}
				}
			}
			if(cat1_receive_msg.source_id == TEST_TASK_ID)
			{
				if(cat1_receive_msg.command == TASK_CMD_CAT1_TEST_START)
				{
					send_to_uartTask(TASK_CMD_START);
					production_flag.flag_set_sn = 1;
					unblock_cat1_task();  // 解除阻塞
					lteInit();
				}
				else if(cat1_receive_msg.command == TASK_CMD_STOP)
				{
					is_lte_init_recovery = false;  // 恢复过程结束
					ltePowerDownPrepare();
					lteShutdown();
				}
				else if(cat1_receive_msg.command == TASK_CAT1_AWS_NO_TEST)
				{
					production_flag.flag_get_version = 1;
					send_to_uartTask(TASK_CMD_START);
					unblock_cat1_task();  // 解除阻塞
					lteInit();
				}
				else if(cat1_receive_msg.command == TASK_CAT1_AWS_WITHCA_TEST)
				{
					production_flag.flag_cat1_conn = 1;
					send_to_uartTask(TASK_CMD_START);
					unblock_cat1_task();  // 解除阻塞
					lteInit();
				}
				else if (cat1_receive_msg.command == TASK_CAT1_AWS_CACERT)  // 写入CACERT证书
				{
					production_flag.flag_set_cacert = 1;
					send_to_uartTask(TASK_CMD_START);
					unblock_cat1_task();  // 解除阻塞
					lteInit();
				}
				else if (cat1_receive_msg.command == TASK_CAT1_AWS_CLIENT)  // 写入CLIENT
				{
					production_flag.flag_set_client = 1;
					send_to_uartTask(TASK_CMD_START);
					unblock_cat1_task();  // 解除阻塞
					lteInit();
				}
				else if (cat1_receive_msg.command == TASK_CAT1_AWS_USERKEY)  // 写入USERKEY
				{
					production_flag.flag_set_userkey = 1;
					send_to_uartTask(TASK_CMD_START);
					unblock_cat1_task();  // 解除阻塞
					lteInit();
				}
				else if (cat1_receive_msg.command == TASK_CAT1_UPDATA)  // LTE差分升级
				{
					production_flag.flag_set_lte_updata = 1;
					send_to_uartTask(TASK_CMD_START);
					unblock_cat1_task();
					lteInit();
				}
				else if (cat1_receive_msg.command == TASK_CAT1_USB_UPDATA)  // LTEUSB升级
				{
					production_flag.flag_set_lte_usb_updata = 1;
//					production_flag.flag_set_lte_updata = 1;
					send_to_uartTask(TASK_CMD_START);
					unblock_cat1_task();
					lteInit();
				}
			}
			if (cat1_receive_msg.source_id == UART_DATARECV_ID && cat1_receive_msg.data != NULL)
			{
//				log_debug("[CAT1][RCV] receive length: %d\r\n", cat1_receive_msg.data_length);
				// 将数据插入缓冲区
				insertPacket(cat1_receive_msg.data, cat1_receive_msg.data_length);
				DEMO_BT_Free(cat1_receive_msg.data);
			}
			// 收到通信任务数据
			if ((cat1_receive_msg.source_id == COMM_TASK_ID))
			{
				if(cat1_receive_msg.command == TASK_SYSTEM_MODE)
				{
					if(cat1_receive_msg.data != NULL)
					{
						Cat1RecvMode = *(MODE_M5_t *)cat1_receive_msg.data;
						
						if(Cat1RecvMode == MODE_STANDARD)
						{
							cat1CheckMqttStateTimeout = 30*60*1000;
						}
						else if(Cat1RecvMode == MODE_SEARCH_PET)
						{
							cat1CheckMqttStateTimeout = 3*60*1000;
						}
						if(osTimerIsRunning(checkMqttState_startTimer_ID))
						{
							osTimerStop(checkMqttState_startTimer_ID);	
						}
						osTimerStart(checkMqttState_startTimer_ID, cat1CheckMqttStateTimeout);
					}
				}
				else if (cat1_receive_msg.command == TASK_START_WIFISCAN)
				{
					// wifi scan 开启
					lteControlWifi();
				}
				else if (cat1_receive_msg.command == TASK_COMM_DATAJSON)
				{
					if (cat1_receive_msg.data != NULL)
					{
						if (g_mqtt_sending == 0) {
							mqtt_publish(cat1_receive_msg.data, cat1_receive_msg.data_length);
							DEMO_BT_Free(cat1_receive_msg.data);
						} else {
							// 繁忙中入队pending缓冲(支持4条, 满了丢最旧的)
							if (pending_buf_count >= PENDING_BUF_SIZE) {
								DEMO_BT_Free(pending_buf_data[pending_buf_head]);
								pending_buf_head = (pending_buf_head + 1) % PENDING_BUF_SIZE;
								pending_buf_count--;
							}
							pending_buf_data[pending_buf_tail] = cat1_receive_msg.data;
							pending_buf_len[pending_buf_tail]  = cat1_receive_msg.data_length;
							pending_buf_tail = (pending_buf_tail + 1) % PENDING_BUF_SIZE;
							pending_buf_count++;
							log_debug("[CAT1][STA] MQTT busy, COMM msg pending (buf=%d)\\r\\n", pending_buf_count);
						}
					}
				}
				else if (cat1_receive_msg.command == TASK_AUDIO_REALTIME)  // 新增HTTP下载命令
				{
					if (cat1_receive_msg.data != NULL)
					{
						// 启动HTTP下载流程
//						log_debug("TASK_AUDIO_REALTIME:%s\n",cat1_receive_msg.data);
						start_http_download(cat1_receive_msg.data, cat1_receive_msg.data_length); 
						DEMO_BT_Free(cat1_receive_msg.data);
					}
				}
				else if(cat1_receive_msg.command == TASK_FACTORY_RESET)
				{
					production_flag.flag_factory_reset = 1;
					send_to_uartTask(TASK_CMD_START);
					unblock_cat1_task();  // 解除阻塞
					lteInit();
				}
			}
		}

		uxBits = osEventFlagsGet(LteEventId);

		// 处理命令就绪事件
		if (uxBits & LTE_EVENT_CMD_READY) {
			//去掉 isPacketListEmpty() 门槛——高频URC到达时会让AT命令队列饿死
			osMutexAcquire(LteMutex, osWaitForever);
				// 检查currentCmd是否为NULL，确保只有空闲时才处理新指令
				if (!currentCmd && lteCmdQueueHead != lteCmdQueueTail) {
					currentCmd = &lteCmdQueue[lteCmdQueueHead];
//					log_debug("Processing cmd type: %d, queue head: %d, tail: %d\n", 
//							 currentCmd->type, lteCmdQueueHead, lteCmdQueueTail);
					// 集中化DTR管理：发送指令前统一拉低DTR唤醒模块
					// DTR=LOW后加延时确保模组从睡眠中完全唤醒(EG800Q需~20ms)
					drv_gpio_write(OM_GPIO0, GPIO_MASK(PAD_CAT1_DTR), GPIO_LEVEL_LOW);
					osDelay(osMS2TicksRound(30));
					func_lte_type(currentCmd->type);
					LastWakeTime = osKernelGetTickCount();
					
					if (currentCmd->has_extra_response) {
						expecting_extra_response = 1;
					}
					osEventFlagsSet(LteEventId, LTE_EVENT_RESP_RECEIVED);
				} else {
					// 如果currentCmd不为NULL，说明有指令正在处理，清除就绪标志
					osEventFlagsClear(LteEventId, LTE_EVENT_CMD_READY);
				}
				osMutexRelease(LteMutex);
		}

		if (uxBits & LTE_EVENT_RESP_RECEIVED)
		{
			memset(recv_buf, 0, MQTT_DATA_MTU_MAX);
			recv_len = 0;
			if (fetch_packet(recv_buf, &recv_len))
			{
				ResponseResult result = process_response(currentCmd, recv_buf, recv_len);
				// 有指令的数据处理
				if (currentCmd)
				{
					switch (result)
					{
					case RESPONSE_COMPLETE:
						// WiFi scan：命令完成，flush 累积的 URC 到 COMM
						if (currentCmd->type == LTE_WIFI_SCAN) {
							lte_wifi_scan_flush();
						}
						// 完成当前指令，移动到下一条
						osMutexAcquire(LteMutex, osWaitForever);
						lteCmdQueueHead = (lteCmdQueueHead + 1) % AT_CMD_QUEUE_SIZE;

						//PUB的>+OK同帧时跳过PUBMESSAGEDATA
						if (g_skip_pubmsgd && lteCmdQueueHead != lteCmdQueueTail) {
							lteCmdItem_t *next = &lteCmdQueue[lteCmdQueueHead];
							if (next->type == LTE_MQTT_PUBMESSAGEDATA) {
								lteCmdQueueHead = (lteCmdQueueHead + 1) % AT_CMD_QUEUE_SIZE;
							}
							g_skip_pubmsgd = false;
						}

						currentCmd = NULL;
						expecting_extra_response = 0;  // 重置标志
						osMutexRelease(LteMutex);
						LastWakeTime = osKernelGetTickCount();

						// 如果队列非空，触发下一条指令；否则恢复DTR准备sleep
						if (lteCmdQueueHead != lteCmdQueueTail)
						{
							osEventFlagsClear(LteEventId, LTE_EVENT_RESP_RECEIVED);
							osEventFlagsSet(LteEventId, LTE_EVENT_CMD_READY);
						}
						else
						{
							lteEntersleepDtrHigh();
						}

						//在 currentCmd 释放后安全处理 pending COMM 消息
						if (pending_buf_count > 0 && g_mqtt_sending == 0) {
							unsigned char *p_data = pending_buf_data[pending_buf_head];
							unsigned int  p_len   = pending_buf_len[pending_buf_head];
							pending_buf_head = (pending_buf_head + 1) % PENDING_BUF_SIZE;
							pending_buf_count--;
							log_debug("[CAT1][STA] Processing pending COMM msg (COMPLETE), len=%u\r\n", p_len);
							mqtt_publish(p_data, p_len);
							DEMO_BT_Free(p_data);
						}
						break;
					case RESPONSE_SUCCESS:
						// WiFi scan flush
						if (currentCmd->type == LTE_WIFI_SCAN) {
							lte_wifi_scan_flush();
						}
						if (expecting_extra_response)
						{
							// 继续等待额外响应
							expecting_extra_response = 0;
							LastWakeTime = osKernelGetTickCount();
						}
						else
						{
							// 完成当前指令，移动到下一条
							osMutexAcquire(LteMutex, osWaitForever);
							lteCmdQueueHead = (lteCmdQueueHead + 1) % AT_CMD_QUEUE_SIZE;

							// PUB的>+OK同帧: skip_pubmsgd=true → 跳过下一条PUBMESSAGEDATA
							if (g_skip_pubmsgd && lteCmdQueueHead != lteCmdQueueTail) {
								lteCmdItem_t *next = &lteCmdQueue[lteCmdQueueHead];
								if (next->type == LTE_MQTT_PUBMESSAGEDATA) {
									lteCmdQueueHead = (lteCmdQueueHead + 1) % AT_CMD_QUEUE_SIZE;
									log_debug("[CAT1][DBG] skip PUBMESSAGEDATA (OK bundled with >)\r\n");
								}
								g_skip_pubmsgd = false;
							}

							currentCmd = NULL;
							osMutexRelease(LteMutex);
							LastWakeTime = osKernelGetTickCount();

							// 如果队列非空，触发下一条指令；否则恢复DTR准备sleep
							if (lteCmdQueueHead != lteCmdQueueTail)
							{
								osEventFlagsClear(LteEventId, LTE_EVENT_RESP_RECEIVED);
								osEventFlagsSet(LteEventId, LTE_EVENT_CMD_READY);
							}
							else
							{
								lteEntersleepDtrHigh();
							}
						}

						// currentCmd 释放后安全处理 pending COMM 消息
						if (pending_buf_count > 0 && g_mqtt_sending == 0) {
							unsigned char *p_data = pending_buf_data[pending_buf_head];
							unsigned int  p_len   = pending_buf_len[pending_buf_head];
							pending_buf_head = (pending_buf_head + 1) % PENDING_BUF_SIZE;
							pending_buf_count--;
							log_debug("[CAT1][STA] Processing pending COMM msg (SUCCESS), len=%u\r\n", p_len);
							mqtt_publish(p_data, p_len);
							DEMO_BT_Free(p_data);
						}
						break;
					case RESPONSE_ERROR:
						// WiFi scan：错误也 flush 已累积的 URC（防止泄漏）
						if (currentCmd->type == LTE_WIFI_SCAN) {
							lte_wifi_scan_flush();
						}
						currentCmd->status = CMD_STATUS_FAILED;
						if (currentCmd->retry_count < currentCmd->retry_max)
						{
							currentCmd->retry_count++;
							LastWakeTime = osKernelGetTickCount();
							if (currentCmd->type == LTE_MQTT_PUBMESSAGEDATA) {
								is_retransmitting = true;
							}
							func_lte_type(currentCmd->type);	   // 重发
						}
						else
						{
							expecting_extra_response = 0;  // 重置标志
							osEventFlagsClear(LteEventId, LTE_EVENT_RESP_RECEIVED);
			
							reset_cat1_state_before_poweron();
							lteCloseTimer();
							lteReboot(); // QIYOU 目前ERROR重启4G模块
						}
						break;
					default:
						break;
					}
				}
				// 无指令的上报的数据
				else
				{
					switch (result)
					{
					case RESPONSE_SUCCESS:
					case RESPONSE_COMPLETE:
						LastWakeTime = osKernelGetTickCount(); // 重置超时计时器
						expecting_extra_response = 0;  // 清残留, 防止后续命令误等第二包
						osEventFlagsClear(LteEventId, LTE_EVENT_RESP_RECEIVED);
						osEventFlagsSet(LteEventId, LTE_EVENT_CMD_READY);
						break;
				case RESPONSE_ERROR:
					// 重连/断连期间收到的残留ERROR，不触发整机重启，直接丢弃，等重连成功后业务重新入队
					if (is_reconnecting || !isMqttConnected)
					{
						log_debug("[CAT1][STA] stale ERROR ignored during reconnect\r\n");
						osEventFlagsClear(LteEventId, LTE_EVENT_RESP_RECEIVED);
						break;
					}
					// 未知错误
					osEventFlagsClear(LteEventId, LTE_EVENT_RESP_RECEIVED);

					reset_cat1_state_before_poweron();
					lteCloseTimer();
					lteReboot(); // QIYOU 目前ERROR重启4G模块
//                            osEventFlagsSet(LteEventId, LTE_EVENT_ERROR);
						break;
					default:
						break;
					}
				}
			}
		}
		// 检查固定60秒超时 - 如果60秒没有收到任何响应，重启LTE
		if ((currentCmd || g_mqtt_sending != 0) && (osKernelGetTickCount() - LastWakeTime) > osMS2TicksRound(CAT1_COMMAND_TIMEOUT))
		{
			log_debug("[CAT1][ERR] LTE no response for 60s, rebooting(%d)\r\n",lte_reboot_count);

			// 增加重启计数
			lte_reboot_count++;
			cat1_escape_pubdata_mode();

			reset_cat1_state_before_poweron();
			
			// 进入错误处理任务：轻微，不能影响正常功能；中等，自适应切换；严重，重启
			// 检查是否达到最大重启次数
			if (lte_reboot_count >= MAX_REBOOT_RETRY)
			{
				lte_recovery_clear();

				log_debug("[CAT1][ERR] send POWERKEY pulse, wait for RDY\r\n");
				is_lte_init_recovery = true;  // 标记恢复过程，阻止POWERED DOWN时阻塞任务
				lteInit();

				lte_init_pending = true;
				lte_init_wait_start = osKernelGetTickCount();
			}
			else
			{
				lteReboot();
			}
			LastWakeTime = osKernelGetTickCount();
		}

		// RDY 等待超时检查：lteInit() 后 10s 无 RDY，则 AT 探活
		if (lte_init_pending && (osKernelGetTickCount() - lte_init_wait_start) > osMS2TicksRound(LTE_INIT_RDY_TIMEOUT))
		{
			lte_init_pending = false;  // 清除标志，避免重复进入
			is_lte_init_recovery = false;  // 超时退出恢复过程
			log_debug("[CAT1][STA] wait timeout 10s, send AT\r\n");

			// AT 探活，判断模块开机/关机状态
			if (checkCat1PowerState())
			{
				// 已开机，正常恢复运行
				log_debug("[CAT1][STA] checkCat1PowerState: module is ON, resuming\r\n");
				set_cat1_state(LTE_TASK_RUNNING);
				unblock_cat1_task();
//				drv_gpio_write(OM_GPIO0, GPIO_MASK(PAD_CAT1_DTR), GPIO_LEVEL_LOW);
				lte_check_cmd_rtos(LTE_MQTT_ISSTATE, "OK\r\n", 1, 500, 1);
			}
			else
			{
				// 模块未开机：限速重试 lteInit()，最多 LTE_HW_RETRY_MAX 次，间隔 LTE_HW_RETRY_INTERVAL
				if (lte_hw_retry_count >= LTE_HW_RETRY_MAX)
				{
					// 已达最大次数，停止硬件重试，等待外部事件(充电）
					ltePowerDownPrepare();
					// 关机
					lteShutdown();
					log_debug("[CAT1][ERR] checkCat1PowerState: module is OFF\r\n");
				}
				else if (lte_hw_retry_count > 0 &&
				         (osKernelGetTickCount() - lte_hw_retry_last_ts) < osMS2TicksRound(LTE_HW_RETRY_INTERVAL))
				{
					// 未到间隔时间，继续等待；重新挂起 pending 以便下次循环再判断
					log_debug("[CAT1][ERR] checkCat1PowerState: module is OFF, HW retry %d/%d, wait interval\r\n",
					          lte_hw_retry_count, LTE_HW_RETRY_MAX);
					lte_init_pending = true;
					lte_init_wait_start = osKernelGetTickCount();
				}
				else
				{
					// 可以执行本次硬件重试
					lte_hw_retry_count++;
					lte_hw_retry_last_ts = osKernelGetTickCount();
					log_debug("[CAT1][ERR] checkCat1PowerState: module is OFF, lteInit() again (%d/%d)\r\n",
					          lte_hw_retry_count, LTE_HW_RETRY_MAX);
					is_lte_init_recovery = true;
					lteInit();
					// 重新设标志，继续等待 RDY
					lte_init_pending = true;
					lte_init_wait_start = osKernelGetTickCount();
				}
			}
		}

		// g_mqtt_sending 快速恢复: 主循环检测到currentCmd=NULL+队列空+sending=1超过5秒
		// 说明MQTT publish链断(OK被吞), 强清标志恢复流控
		if (g_mqtt_sending == 1 && !currentCmd && lteCmdQueueHead == lteCmdQueueTail) {
			if (mqtt_send_start_tick != 0 &&
			    (osKernelGetTickCount() - mqtt_send_start_tick) > osMS2TicksRound(5000)) {
				log_debug("[CAT1][WDT] g_mqtt_sending=1 but no active cmd for 5s, force clear\r\n");
				g_mqtt_sending = 0;
				mqtt_send_start_tick = 0;
			}
		}

		osDelay(osMS2TicksRound(10));
	}
}

void atcmd_rtos_init(void)
{
	// 创建互斥锁
	LteMutex = osMutexNew(NULL);
	PacketListMutex = osMutexNew(NULL);
	LteDeviceInfoMutex = osMutexNew(NULL);
	lteTaskStateMutex = osMutexNew(NULL);
	
	// 创建事件组
	LteEventId = osEventFlagsNew(NULL);
	
	init_mqtt_queue();
	if (Cat1PowerCheckSem == NULL) {
		    Cat1PowerCheckSem = osSemaphoreNew(1, 0, NULL);
    } else {
    }
	init_device_cache();
	CELL_Init();
	
	ReconnectMutex = osMutexNew(NULL);
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
osThreadId_t vStartCAT1UartTask(void)
{
	atcmd_rtos_init();

	const osThreadAttr_t CAT1UartThreadAttr = {
		.name = "CAT1_UART_Task",
		.attr_bits = 0,
		.cb_mem = NULL,
		.cb_size = 0,
		.stack_mem = NULL,
		.stack_size = CAT1_UART_TASK_STACK_SIZE,
		.priority = CAT1_UART_TASK_PRIORITY,
		.tz_module = 0,
	};

	// Create pm Task
	return osThreadNew(vCAT1UartTask, NULL, &CAT1UartThreadAttr);
}

/** @} */

// vim: fdm=marker
