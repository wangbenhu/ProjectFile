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

// Controller header
#include "obc.h"

/* Kernel includes. */
#include "cmsis_os2.h"
#include "common_def.h"
#include "service_tspp_define.h"
#include "cJSON.h"
#include "ble_op_queue.h"
#include "app_adv.h"
/*********************************************************************
 * MACROS
 */
#define EVENT_SYSTEM_RESERVE_MASK   0x00FF
#define BLE_PACKET_TIMEOUT_MS       3000    // 3秒超时
#define BLE_MUTEX_TIMEOUT_MS  100
#define BLE_RECV_QUEUE_SIZE         8       // BLE接收队列容量

// BLE接收环形队列：组包完成后入队，等COMM回包后再取队首发下一包
typedef struct {
    uint8_t *data[BLE_RECV_QUEUE_SIZE];     // 指针数组(每条消息独立malloc)
    uint16_t data_len[BLE_RECV_QUEUE_SIZE]; // 每条消息长度
    int head;
    int tail;
    int count;
} ble_recv_queue_t;

#define EVT_LOG_DEBUG(format, ...)               	log_debug(format,  ## __VA_ARGS__)
/// log array
#define EVT_LOG_ARRAY(array, len)            do{int __i; for(__i=0;__i<(len);++__i)EVT_LOG_DEBUG("%02X ",((uint8_t *)(array))[__i]);}while(0)

/*********************************************************************
 * TYPEDEFS
 */


/*********************************************************************
 * CONSTANTS
 */
/*********************************************************************
 * LOCAL VARIABLES
 */
static osSemaphoreId_t xSemBluetooth = NULL;
static ble_recv_queue_t g_ble_recv_queue = {0};

typedef struct {
    uint8_t buffer[BLE_DATA_BUFFER_MAX];
    size_t received_len;
    uint16_t expected_len;
    bool waiting_for_header;
    uint32_t last_receive_time;  // 最后接收时间戳
    bool in_large_data_mode;     // 是否处于大数据接收模式
} data_assembler_t;

ob_gap_conn_parameter_t conn_tmp = {
    0x18,   // 30 ms
    0x24,   // 45 ms
    4,      // Peripheral Latency，空闲时最多跳过4个连接事件
    600     // Supervision Timeout = 6秒
};
/*********************************************************************
 * CONSTANTS
 */


/*********************************************************************
 * LOCAL VARIABLES
 */

static data_assembler_t assembler = {0};

//设备信息
static uint8_t device_info_bleMac[13]; 

static uint8_t ble_status_flag = 0;
// 连接参数

/*********************************************************************
 * GLOBAL VARIABLES
 */

osMutexId_t BleDeviceInfoMutex;

/*********************************************************************
 * EXTERN FUNCTIONS
 */
void app_adv_init();
void app_sec_init();
void service_common_init(void);
void app_om_dfu_init(void);
void ancs_client_init(void);
void app_media_hid_init(void);
void app_tspp_init(void);
void app_wechat_lite_init();
void app_om_cgms_init(void);
void app_om_bms_init(void);
void app_om_audio_transfer_init(void);
extern void evt_app_adv_stop(void);
extern void evt_app_adv_start(void);
extern bool app_adv_get_conn_idx(uint8_t *conn_idx);
extern uint8_t* app_get_local_addr(void);
/*********************************************************************
 * LOCAL FUNCTIONS
 */

// BLE缓冲队列流控相关函数前向声明
static void on_ble_packet_assembled(uint8_t *json_data, uint16_t len);
static void ble_send_next_packet_to_comm(void);

static void hardware_init(void)
{
}

/*
* @brief 开始广播
* @param cmd_param Command parameter mask
* @param data Pointer to the command parameter data
* @param data_len Length of the command parameter data
* @param context User context pointer
* */
static void ble_op_on_start_adv(uint16_t cmd_param, const uint8_t *data,
                                uint32_t data_len, void *context)
{
    ble_adv_mode_t mode = (ble_adv_mode_t)cmd_param;
    (void)data;
    (void)data_len;
    (void)context;

    if ((mode != BLE_ADV_MODE_FAST) && (mode != BLE_ADV_MODE_SLOW)) {
        log_debug("Invalid advertising mode: %u\n", cmd_param);
        return;
    }
	
	

    log_debug("Start advertising mode=%u result=%lu\n", cmd_param,
              (unsigned long)app_adv_start(mode));
}
/*
* @brief 停止广播
* @param cmd_param Command parameter mask
* @param data Pointer to the command parameter data
* @param data_len Length of the command parameter data
* @param context User context pointer
* */
static void ble_op_on_stop_adv(uint16_t cmd_param, const uint8_t *data,
                               uint32_t data_len, void *context)
{
    (void)cmd_param;
    (void)data;
    (void)data_len;
    (void)context;
    log_debug("Stop advertising result=%lu\n",
              (unsigned long)app_adv_stop());
}
/*
* @brief 断开连接
* @param cmd_param Connection parameter mask
* @param data Pointer to the connection parameter data
* @param data_len Length of the connection parameter data
* @param context User context pointer
* */
static void ble_op_on_disconnect(uint16_t cmd_param, const uint8_t *data,
                                 uint32_t data_len, void *context)
{
    (void)cmd_param;
    (void)data;
    (void)data_len;
    (void)context;
    /* Reserved: disconnect a BLE connection from the scheduler thread. */
	app_adv_disconnect();
}
/*
* @brief 更新连接参数
* @param cmd_param Connection parameter mask
* @param data Pointer to the connection parameter data
* @param data_len Length of the connection parameter data
* @param context User context pointer
* */
static void ble_op_on_update_conn_param(uint16_t cmd_param,
                                        const uint8_t *data,
                                        uint32_t data_len, void *context)
{
    (void)cmd_param;
    (void)data;
    (void)data_len;
    (void)context;
	uint8_t conn_idx;
    /* Reserved: update BLE connection parameters from the scheduler thread. */
 if (app_adv_get_conn_idx(&conn_idx)) {
    ob_gap_conn_param_request(conn_idx, &conn_tmp);
	}
}

static const ble_op_callbacks_t ble_app_op_callbacks = {
    .start_adv = ble_op_on_start_adv,
    .stop_adv = ble_op_on_stop_adv,
    .disconnect = ble_op_on_disconnect,
    .update_conn_param = ble_op_on_update_conn_param,
};
/**
 *******************************************************************************
 * @brief  evt timer 0 handler
 *
 * @param[in] timer  timer
 * @param[in] param  param
 *******************************************************************************
 */

/**
 * @brief  bluetooth event handler
 **/
static void vEvtEventHandler(void)
{
    if (xSemBluetooth) {
        osSemaphoreRelease(xSemBluetooth);
    }
}


int ble_dataUpStream_subPackage(uint8_t *data, tspp_size_t len) 
{
    int ret = 0;
    uint16_t json_len = len;
    
    // 使用栈上的缓冲区（最大240字节）
    uint8_t packet[BLE_DATA_MTU_MAX];
    
    // 计算需要分多少包发送
    size_t max_data_per_packet = BLE_DATA_MTU_MAX - 2;
    size_t total_pkts = (json_len + max_data_per_packet - 1) / max_data_per_packet;
    
    size_t offset = 0;
    size_t remaining = json_len;
    
	log_debug("ble_dataUpStream_subPackage = %d %d\r\n",total_pkts,len);
    for (size_t pkt_idx = 1; pkt_idx <= total_pkts; pkt_idx++) 
    {
        // 计算当前包的数据部分长度
        size_t data_len_in_packet = (remaining > max_data_per_packet) ? max_data_per_packet : remaining;
        size_t packet_total_len = 2 + data_len_in_packet;
        
        // 填充长度头（小端序）
        packet[0] = json_len & 0xFF;        // 低字节
        packet[1] = (json_len >> 8) & 0xFF; // 高字节
        
        memcpy(packet + 2, data + offset, data_len_in_packet);
        
		log_debug("[BLE][SND] \r\n");
		drv_uart_write(LOG_UART, (uint8_t *)packet, (uint32_t)packet_total_len, 10);
        // 发送数据包
//		 log_debug("m_ble_op_tspp_send =" );
//		for(int i=0;i<packet_total_len;i++)
//		{ log_debug("%02x ",packet[i] );
//			
//		}log_debug("\r\n" );
		
        ret  =m_ble_op_tspp_send(packet, packet_total_len);
		//tspp_send(packet, packet_total_len);
		if (!ret) {
			log_debug("BLE packet enqueue failed: %zu\r\n", pkt_idx);
			DEMO_BT_Free(data);
			return -1;
		}
        
        offset += data_len_in_packet;
        remaining -= data_len_in_packet;
		
        if (pkt_idx < total_pkts) {
            osDelay(osMS2TicksRound(100));
        }
    }
    DEMO_BT_Free(data);
    return ret;
}

void reset_assembler(void) {
    assembler.received_len = 0;
    assembler.expected_len = 0;
    assembler.waiting_for_header = true;
    assembler.last_receive_time = 0;
    assembler.in_large_data_mode = false;
	memset(assembler.buffer, 0, sizeof(assembler.buffer));
}

/**
 * @brief 检查是否超时，如果超时则重置组装器
 */
bool check_timeout_and_reset(void) {
    if (assembler.last_receive_time == 0) {
        return false;
    }
    
    uint32_t current_time = osKernelGetTickCount();
    if (current_time - assembler.last_receive_time > osMS2TicksRound(BLE_PACKET_TIMEOUT_MS)) {
        log_debug("check_timeout_and_reset:%u,%u\r\n", 
                 assembler.expected_len, assembler.received_len);
        reset_assembler();
        return true;
    }
    return false;
}

/**
 * @brief 处理小数据包（<=240字节）- 原有逻辑
 */
/**
 * @brief 组包完成回调 — 入本地环形队列后立即尝试泵入COMM队列
 * @note  COMM串行消费自身队列(深16), 天然一包处理完(含回复)再取下一包;
 *        本地队列仅在COMM队列满时暂存, 由主循环唤醒时重试
 */
static void on_ble_packet_assembled(uint8_t *json_data, uint16_t len)
{
    ble_recv_queue_t *q = &g_ble_recv_queue;
    
    if (q->count >= BLE_RECV_QUEUE_SIZE) {
        log_debug("BLE recv queue full, drop packet\r\n");
        DEMO_BT_Free(json_data);
        return;
    }
    
    // 入队
    q->data[q->tail] = json_data;
    q->data_len[q->tail] = len;
    q->tail = (q->tail + 1) % BLE_RECV_QUEUE_SIZE;
    q->count++;
    
    // 立即入COMM队列(COMM队列满则留在本地队列, 主循环唤醒时重试)
    ble_send_next_packet_to_comm();
}

/**
 * @brief 将BLE接收队列中的数据依次入COMM队列
 * @note  COMM队列满时保留队首不出队(不free), 等下次主循环唤醒重试, 数据不丢
 */
static void ble_send_next_packet_to_comm(void)
{
    ble_recv_queue_t *q = &g_ble_recv_queue;
    TaskInfo_t *my_task_info = GetTaskInfo(BLE_SCHEDULE_TASK_ID);
    TaskInfo_t *comm_task_info = GetTaskInfo(COMM_TASK_ID);
    
    if (!comm_task_info || !comm_task_info->queue_handle) {
        log_debug("Comm task not ready for BLE send\r\n");
        return;
    }
    
    while (q->count > 0) {
        Message_t ble_msg = {
            .source_id = my_task_info->task_id,
            .dest_id = COMM_TASK_ID,
            .command = TASK_COMM_DATAJSON,
            .data = q->data[q->head],
            .data_length = q->data_len[q->head]
        };
        
        // 被动接收消息优先级高(3)，主动上报优先级低(1)
        // 这里统一用prio=3，因为BLE来的都是被动接收(APP指令)
        uint8_t msg_prio = 3;
        
        if (osMessageQueuePut(comm_task_info->queue_handle, &ble_msg, msg_prio, 0) != osOK) {
            // COMM队列满: 保留在本地队列, 下次主循环唤醒时重试(不free不出队)
            log_debug("COMM queue full, %d BLE packets pending retry\r\n", q->count);
            break;
        }
        
        // 发送成功，出队（data指针转移给COMM，COMM负责释放）
        q->head = (q->head + 1) % BLE_RECV_QUEUE_SIZE;
        q->count--;
        log_debug("[BLE][RECV] %s\r\n", ble_msg.data);
    }
}

void process_small_data_package(uint8_t *data, uint32_t len, TaskInfo_t *my_task_info, TaskInfo_t *comm_task_info) {
    // 分配内存并拷贝数据，然后入BLE接收队列
    uint8_t *json_data = DEMO_BT_Malloc(len + 1);  // +1: NUL终结, 下游按C字符串使用(%s日志/cJSON解析), 无'\0'会越界读
    if (json_data) {
        memcpy(json_data, data, len);
        json_data[len] = '\0';
        on_ble_packet_assembled(json_data, len);
    } else {
        log_debug("Failed to allocate memory for small JSON data\r\n");
    }
}

/**
 * @brief 处理大数据包（>240字节）- 新增逻辑
 */
void process_large_data_package(uint8_t *msg, uint32_t len) {
    uint8_t *current_pos = msg;
    uint32_t remaining_len = len;
    
    TaskInfo_t *my_task_info = GetTaskInfo(BLE_SCHEDULE_TASK_ID);
    TaskInfo_t *comm_task_info = GetTaskInfo(COMM_TASK_ID);
    
    if (!comm_task_info || !comm_task_info->queue_handle) {
        return;
    }

    // 更新最后接收时间
    assembler.last_receive_time = osKernelGetTickCount();

    while (remaining_len > 0) {
        // 检查超时
        if (check_timeout_and_reset()) {
            return;
        }
        
        if (assembler.waiting_for_header) {
            // 等待长度头
            if (remaining_len >= 2) {
                assembler.expected_len = current_pos[0] | (current_pos[1] << 8);
                current_pos += 2;
                remaining_len -= 2;
                assembler.received_len = 0;
                assembler.waiting_for_header = false;
                assembler.in_large_data_mode = true;
                
                log_debug("Large message mode - Expecting length: %u bytes:%x,%x\r\n", assembler.expected_len,current_pos[0],current_pos[1]);
                
                if (assembler.expected_len > BLE_DATA_BUFFER_MAX) {
                    log_debug("Message too large:%u\r\n", assembler.expected_len);
                    reset_assembler();
                    return;
                }
                
                if (assembler.expected_len <= BLE_DATA_MTU_MAX - 2) {
                    log_debug("Message is actually small, switching to small data mode\r\n");
                    assembler.in_large_data_mode = false;
                }
				continue;
            } else {
                break; // 数据不足，等待下一包
            }
        }
        
        // 接收数据部分
        if (!assembler.waiting_for_header && remaining_len > 0) {
            size_t data_to_copy = (assembler.expected_len - assembler.received_len < remaining_len) 
                                ? (assembler.expected_len - assembler.received_len) 
                                : remaining_len;
            
            memcpy(assembler.buffer + assembler.received_len, current_pos, data_to_copy);
            assembler.received_len += data_to_copy;
            current_pos += data_to_copy;
            remaining_len -= data_to_copy;
            
//            log_debug("Large data progress: %u/%u bytes (%.1f%%)", 
//                     assembler.received_len, assembler.expected_len,
//                     (assembler.received_len * 100.0) / assembler.expected_len);
            
            // 检查是否收到完整消息
            if (assembler.received_len >= assembler.expected_len) {
                log_debug("Large message complete! Total:%u\r\n", assembler.received_len);
                
                // 分配内存并复制数据
                uint8_t *json_data = DEMO_BT_Malloc(assembler.received_len + 1);  // +1: NUL终结, 同小包路径
                if (json_data) {
                    memcpy(json_data, assembler.buffer, assembler.received_len);
                    json_data[assembler.received_len] = '\0';
                    on_ble_packet_assembled(json_data, assembler.received_len);
                } else {
                    log_debug("Failed to allocate memory for large JSON data\r\n");
                }
                
                // 重置组装器，准备接收下一条消息
                reset_assembler();
				// BUG修复: remaining_len>0时不直接return，继续处理剩余数据（可能包含下一条消息）
				if (remaining_len > 0) {
					log_debug("Message complete with %u bytes remaining, processing next message\r\n", remaining_len);
				}
				// 不return，继续while循环处理剩余数据
				
            } else {
                // 更新接收时间，继续等待下一包
                assembler.last_receive_time = osKernelGetTickCount();
            }
        }
    }
}

void data_downStream_package(uint8_t *msg, uint32_t len) {
    // 参数检查
    if (!msg || len == 0) {
        log_debug("Invalid parameters to data_downStream_package\r\n");
        return;
    }
    
//    log_debug("Data received: %u bytes, current state: waiting_header=%d, in_large_mode=%d\r\n", 
//             len, assembler.waiting_for_header, assembler.in_large_data_mode);
    // 检查是否已经处于大数据接收模式且可能超时
    if (assembler.in_large_data_mode || !assembler.waiting_for_header) {
        if (check_timeout_and_reset()) {
            // 超时重置后，重新处理当前数据
            log_debug("Timeout reset, reprocessing current data as new message\r\n");
        }
    }
    
    TaskInfo_t *my_task_info = GetTaskInfo(BLE_SCHEDULE_TASK_ID);
    TaskInfo_t *comm_task_info = GetTaskInfo(COMM_TASK_ID);
    
    if (!comm_task_info || !comm_task_info->queue_handle) {
        log_debug("Comm task not ready\r\n");
        return;
    }
    
    // 判断数据包类型：如果当前在等待头部，且数据包<=240，按小数据处理
    if (assembler.waiting_for_header && len <= BLE_DATA_MTU_MAX) {
        // 检查是否包含长度头的小数据包
        if (len >= 2) {
            uint16_t potential_length = msg[0] | (msg[1] << 8);
            // 如果长度头与实际长度匹配（考虑2字节头），则是完整的小数据包
            if (potential_length + 2 == len) {
                log_debug("Detected small complete package: %u bytes\r\n", len);
                process_small_data_package(msg + 2, len - 2, my_task_info, comm_task_info);
                return;
            }
        }
    }
    
    // 其他情况按大数据处理（包括已经开始接收的大数据、或者新的大数据）
    process_large_data_package(msg, len);
}

void ble_dataDown_handler(uint8_t *data, uint16_t len) {
    // 添加互斥锁保护，防止并发访问
    if (osMutexAcquire(BleDeviceInfoMutex, BLE_MUTEX_TIMEOUT_MS) == osOK) {
        data_downStream_package(data, len);
        osMutexRelease(BleDeviceInfoMutex);
    } else {
        log_debug("Failed to acquire mutex in ble_dataDown_handler\r\n");
    }
}

uint8_t* DEVICE_GetMac(void)
{
    static uint8_t mac_str[13]; // 静态变量，避免返回局部变量
    uint8_t* mac_addr = app_get_local_addr();
        
	mac_str[0] = '\0';
	
    if (mac_addr == NULL) {
        log_debug("MAC address is NULL\r\n");
        return NULL;
    }
	
	// 将二进制MAC转换为十六进制字符串
	snprintf((char*)mac_str, sizeof(mac_str), "%02x%02x%02x%02x%02x%02x",
              mac_addr[5], mac_addr[4], mac_addr[3],  // 从后往前
              mac_addr[2], mac_addr[1], mac_addr[0]);
        
    log_debug("MAC String: %s\r\n", mac_str);
    return mac_str;
}
/**
 * @brief  bluetooth schedule task
 *
 * @param[in] pvParameters  pv parameters
 **/
static void vEvtScheduleTask(void *argument)
{

    TaskInfo_t *my_task_info = GetTaskInfo(BLE_SCHEDULE_TASK_ID);
    TaskInfo_t *entry_task_info = GetTaskInfo(ENTRY_TASK_ID);

    Message_t received_msg;
    
    log_debug("Task %d started\n", my_task_info->task_id);

	hardware_init();
	drv_rf_init();
	evt_init();
	
	struct ob_stack_param param = {
	    .max_connection = 1,
	    .max_ext_adv_set = 4,
	    .max_att_mtu = 243,
	    .max_gatt_serv_num = 8,
	    .max_gatt_write_cache = 128,
	    .smp_sc_support = true,
	};
	omble_init(&param);

	app_adv_init();
	app_sec_init();
	service_common_init();
	app_om_dfu_init();
	ancs_client_init();
	//app_media_hid_init();
	app_tspp_init();
	app_wechat_lite_init();
	app_om_audio_transfer_init();
	
	// Create semaphore
	if (xSemBluetooth == NULL) {
		xSemBluetooth = osSemaphoreNew(1, 0, NULL);
    } else {
        log_debug("Semaphore already exists, skip creation\r\n");
    }

	// set ke event callback
	evt_schedule_trigger_callback_set(vEvtEventHandler);
	
	   // 
	if (!ble_op_init()) {
		log_debug("ble_op_init failed\r\n");
	} //
    else if (!ble_op_register_callbacks(&ble_app_op_callbacks, NULL)) {
		log_debug("ble_op callback registration failed\r\n");
	}
	while (1) {
	    evt_schedule();
	
		if(osOK == osMessageQueueGet(my_task_info->queue_handle,&received_msg, NULL, 0))
		{
			// 回复消息给Entry任务
			if((received_msg.source_id == ENTRY_TASK_ID) && (received_msg.command == TASK_CMD_STOP))
			{ 
				evt_app_adv_stop();
			}
		/* BLE下行包直接入COMM队列(深16)
		 * 无流控信号量, COMM回包走m_ble_op_tspp_send(evt框架路径)自然唤醒本任务 */
	}
	ble_send_next_packet_to_comm();
		
	    // Wait for semaphore
	    osSemaphoreAcquire(xSemBluetooth, osWaitForever);
	}
}

void ble_rtos_init(void)
{
	// 创建互斥锁
	BleDeviceInfoMutex = osMutexNew(NULL);
    // 初始化组装器
    reset_assembler();
    // 初始化BLE接收队列
    memset(&g_ble_recv_queue, 0, sizeof(ble_recv_queue_t));
    g_ble_recv_queue.head = 0;
    g_ble_recv_queue.tail = 0;
    g_ble_recv_queue.count = 0;
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
//void vStartEvtTask(void)
//{
//    const osThreadAttr_t bluetoothThreadAttr = {
//        .name = NULL,
//        .attr_bits = 0,
//        .cb_mem = NULL,
//        .cb_size = 0,
//        .stack_mem = NULL,
//        .stack_size = 2048,
//        .priority = osPriorityRealtime,
//        .tz_module = 0,
//    };
//
//    // Create ble Task
//    osThreadNew(vEvtScheduleTask, NULL, &bluetoothThreadAttr);
//}

osThreadId_t vStartBLEScheduleTask(void)
{
	ble_rtos_init();
    const osThreadAttr_t bluetoothThreadAttr = {
        .name = "BLE_Task",
        .attr_bits = 0,
        .cb_mem = NULL,
        .cb_size = 0,
        .stack_mem = NULL,
        .stack_size = 4096,
        .priority = osPriorityRealtime,
        .tz_module = 0,
    };

    // Create ble Task
    return osThreadNew(vEvtScheduleTask, NULL, &bluetoothThreadAttr);
}

/** @} */

// vim: fdm=marker
