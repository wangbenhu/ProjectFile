/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup DOC DOC
 * @ingroup  DOCUMENT
 * @brief    system
 * @details  system
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


/*******************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "omble.h"
#include "om_log.h"
#include "../source/drv_rng.h"
#include "nvds.h"
#include "common_def.h"


typedef enum {
    BLE_EVT_START_ADV = 0,    // 请求开启广播
    BLE_EVT_STOP_ADV,         // 请求停止广播
    BLE_EVT_CONNECT,          // 连接事件
    BLE_EVT_DISCONNECT,       // 断开事件
    BLE_EVT_TIMEOUT,          // 超时事件（可选）
    BLE_EVT_MAX
} BleEvent_t;
// 当前状态
static BleState_t currentState = BLE_STATE_IDLE;
typedef void (*BleStateHandler_t)(BleEvent_t evt);

static BleState_t userSetState = BLE_STATE_IDLE;
ble_auth_state_t ble_auth_state = BLE_AUTH_STATE_IDLE;

uint8_t systerm_set_data[6];
uint8_t Systerm_data_Addr[6];

// 1. 添加状态机锁，防止重入
static osMutexId_t bleStateMutex = NULL;
/*****
**************************************************************************
 * MACROS
 */
//#define log_debug(...) om_log(OM_LOG_INFO, ##__VA_ARGS__)
#define hexdump(d, l) do{for(int i=0;i<l;i++)log_debug("%02X ", ((uint8_t*)d)[i]);log_debug("\n");}while(0);
#define AUTH_START_TIMEOUT (30000)

/*******************************************************************************
 * LOCAL VARIABLES
 */
/*const static int  app_gap_appearance = 0x03C2;*/
/// Advertise data
static uint8_t sdata[] = {
    /* Flags: BLE limited discoverable mode and BR/EDR not supported */
    0x02, 0x01, 0x06,
    /* incomplete list of service class UUIDs: (0x1812) */
    /*0x03, 0x02, 0x12, 0x18,*/
    /* incomplete list of service class UUIDs: (0xFEE7) */
    /*0x03, 0x02, 0xE7, 0xFE,*/
    /* Apperance */
	//address
	0x07, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 预留位置给MAC地址
    /*0x03, 0x19, app_gap_appearance & 0xff, (app_gap_appearance>>8) & 0xff,*/
    /* Complete Local Name */
    0x11, 0x09,
    'M','e','e','X','H',' ','C','o','l','l','a','r',' ','M','a','x'
};

static uint8_t local_addr[] = { 0x02, 0xA1, 0x28, 0x66, 0xBF, 0x01 };
static ob_gap_addr_t peer_addr = {1, { 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC} };
static ob_gap_addr_t last_conn;
static ob_adv_param_t adv_param;
static ob_data_t adv_data = { sdata, sizeof(sdata) };
static ob_data_t scan_rsp_data = { sdata, sizeof(sdata) };
extern void evt_app_adv_stop(void);
extern void evt_app_adv_start(void);
osTimerId_t Disconn_Timeout_ID;
/*******************************************************************************
 * LOCAL FUNCTIONS
 */
/*******************************************************************************
 * LOCAL FUNCTIONS
 */

BleState_t get_ble_status(void)
{
	return currentState;
}
	
/*******************************************************
* Function name : addr_compare
* Description   : 比较地址
* Parameter     : void
* Return        : void
********************************************************/
void addr_compare(uint8_t *addr)
{
    int i;
   
    for(i=0; i<6; i++)
    {
        Systerm_data_Addr[i] = addr[i];
    }
}  

/*******************************************************
* Function name : flash_addr_judge
* Description   : 判断flash是否有地址
* Parameter     : void
* Return        : void
********************************************************/
bool flash_addr_judge(uint8_t *addr)
{
    bool index = false;
    int i;
    
    for(i = 0; i < 6; i++)
    {
        if(addr[i] == 0)
        {
            continue;
        }
        else
        {
            index = true;
            break;
        }
    }
    
    if(i == 5)
    {
        index = false;
    }
    return index;
}

/*******************************************************
* Function name : flash_addr_judge
* Description   : 判断flash是否有地址
* Parameter     : void
* Return        : void
********************************************************/
void nvds_addr_save(uint8_t *addr)
{
    int i;
   
    for(i=0; i<6; i++)
    {
        systerm_set_data[i] = addr[i];
    }
}  

void update_adv_data_with_mac(uint8_t *mac_addr)
{
	if (mac_addr == NULL) {
        log_debug("update_adv_data_with_mac: NULL pointer\r\n");
        return;
    }
    
    // 确保sdata有足够空间
    if (sizeof(sdata) < 11) {
        log_debug("update_adv_data_with_mac: sdata too small\r\n");
        return;
    }
    sdata[5] = mac_addr[5];  // MAC字节0
    sdata[6] = mac_addr[4];  // MAC字节1  
    sdata[7] = mac_addr[3];  // MAC字节2
    sdata[8] = mac_addr[2];  // MAC字节3
    sdata[9] = mac_addr[1];  // MAC字节4
    sdata[10] = mac_addr[0]; // MAC字节5
}

/*******************************************************
* Function name : updata_adv_addr
* Description   : 更新重设置的地址(广播)
* Parameter     : addr 地址
* Return        : void
********************************************************/
void app_set_addr(uint8_t *address)
{
    uint8_t local_addr[6] = {0};
    uint16_t lenth = 0;
    nvds_tag_len_t len = NVDS_LEN_BD_ADDRESS;

	//说明NVDS无地址
    if(NVDS_OK != nvds_get(NVDS_USER_TAG_BD_ADDRESS, &len, local_addr)) 
    {
        nvds_put(NVDS_USER_TAG_BD_ADDRESS, NVDS_LEN_BD_ADDRESS, address);
        memcpy(local_addr, address, NVDS_LEN_BD_ADDRESS);
    }

	log_debug("address22: %02X:%02X:%02X:%02X:%02X:%02X\n", 
              local_addr[5], local_addr[4], local_addr[3], 
              local_addr[2], local_addr[1], local_addr[0]);
	update_adv_data_with_mac(local_addr);
    nvds_addr_save(local_addr);
}

/*******************************************************
* Function name : product_set_addr
* Description   : 产测更新MAC
* Parameter     : addr 地址
* Return        : void
********************************************************/
void product_set_addr(uint8_t *address)
{
	if(address == NULL)
	{
		return;
	}
    uint8_t local_addr[6] = {0};
	
	nvds_del(NVDS_TAG_BD_ADDRESS);
	nvds_put(NVDS_TAG_BD_ADDRESS, 6, address);
	memcpy(local_addr, address, 6);
	
	log_debug("productAddress: %02X:%02X:%02X:%02X:%02X:%02X\n", 
              local_addr[5], local_addr[4], local_addr[3], 
              local_addr[2], local_addr[1], local_addr[0]);
	update_adv_data_with_mac(local_addr);
    nvds_addr_save(local_addr);
}
void BleState_Idle(BleEvent_t evt) {
	log_debug("BleState_Idle:%d,%d\r\n",evt,currentState);
    switch (evt) {
    case BLE_EVT_START_ADV:
        if(currentState==BLE_STATE_IDLE)
        {
            ob_gap_adv_start(0, &adv_param, &adv_data, NULL);
            
			currentState = BLE_STATE_ADVERTISING;
        }
        
        break;
    default:
        break;
    }
}

void BleState_Advertising(BleEvent_t evt) {
	log_debug("BleState_Advertising:%d,%d\r\n",evt,currentState);
    switch (evt) {
    case BLE_EVT_STOP_ADV:
		if(currentState ==BLE_STATE_CONNECTED)
		{
			ob_gap_disconnect(0,0x13);
		}
		else
		{
			ob_gap_adv_stop(0);
		}
        currentState = BLE_STATE_IDLE;
        break;
    case BLE_EVT_CONNECT:
        currentState = BLE_STATE_CONNECTED;
        break;
    default:
        break;
    }
}

void DisconnTimerAdvCallback(void *argument)
{
	  evt_app_adv_start();
}
void BleState_Connected(BleEvent_t evt) {
	 log_debug("BleState_Connected:%d,%d,%d\r\n",evt,currentState,userSetState);
    switch (evt) {
    case BLE_EVT_DISCONNECT:
        currentState = BLE_STATE_IDLE; // 回到空闲
        if(userSetState == BLE_STATE_ADVERTISING)
        {
			if (Disconn_Timeout_ID == NULL) {
				Disconn_Timeout_ID = osTimerNew(DisconnTimerAdvCallback,osTimerOnce,NULL,NULL);
				if(!Disconn_Timeout_ID)
					LOG_LOC();
			} 
			if(osTimerIsRunning(Disconn_Timeout_ID))
			{
				osTimerStop(Disconn_Timeout_ID);	
			}
			osTimerStart(Disconn_Timeout_ID, 100); // 100ms后重启广播
        }
        break;
	 case BLE_EVT_STOP_ADV:
	 {
		userSetState = BLE_STATE_IDLE;  // 更新用户设置
		ob_gap_disconnect(0,0x13);
	 }
        break;	
    default:
        break;
    }
}
BleStateHandler_t BleStateTable[BLE_STATE_MAX] = {
    [BLE_STATE_IDLE]        = BleState_Idle,
    [BLE_STATE_ADVERTISING] = BleState_Advertising,
    [BLE_STATE_CONNECTED]   = BleState_Connected,
	
    // 如果有 CONNECTING、DISCONNECTING 状态也可以写各自的处理函数
};
void BleStateMachine_Run(BleEvent_t evt)
{   
	   // 创建互斥锁（首次使用）
    if (bleStateMutex == NULL) {
        osMutexAttr_t mutex_attr = {
            .name = "bleStateMutex",
            .attr_bits = osMutexRecursive
        };
        bleStateMutex = osMutexNew(&mutex_attr);
    }
    
    osMutexAcquire(bleStateMutex, osWaitForever);
	log_debug("BleStateMachine_Run:%d,%d,%d\r\n",currentState,evt,userSetState);
    if ((currentState < BLE_STATE_MAX) && (BleStateTable[currentState] != NULL)) {
        BleStateTable[currentState](evt);
    }
	   
    osMutexRelease(bleStateMutex);
}

uint8_t* app_get_local_addr(void)
{
	return systerm_set_data;
}

ob_gap_addr_t* app_get_peer_addr(void)
{
	
	return &peer_addr;
}
void set_address_init(void)
{
    uint8_t buffer[6] = {0};	// addr;
    drv_rng_read(buffer,6);
    buffer[5] |= (3<<6);

    app_set_addr(buffer);
}

osTimerId_t deviceAuth_Timeout_ID;
osTimerType_t deviceAuth_Timeout_type = osTimerOnce;
osTimerAttr_t deviceAuth_Timeout_attr = {
	.name = "deviceAuth_Timer",
};
void TimerCallback_startCAT1(void *argument)
{
	if (ble_auth_state == BLE_AUTH_STATE_WAITING) 
	{
        if (currentState == BLE_STATE_CONNECTED) {
        // 断开连接
			userSetState = BLE_STATE_ADVERTISING;
			ob_gap_disconnect(0, 0x13);
		}
        ble_auth_state = BLE_AUTH_STATE_IDLE;
    }
}
void deviceAuthStartTimer(void)
{
	if (deviceAuth_Timeout_ID == NULL) {
		deviceAuth_Timeout_ID = osTimerNew(TimerCallback_startCAT1,deviceAuth_Timeout_type,NULL,&deviceAuth_Timeout_attr);
		if(!deviceAuth_Timeout_ID)
			LOG_LOC();
	}
	ble_auth_state = BLE_AUTH_STATE_WAITING;
	osTimerStart(deviceAuth_Timeout_ID, AUTH_START_TIMEOUT);
}

void deviceAuthStopTimer(void)
{
	if(deviceAuth_Timeout_ID != NULL && osTimerIsRunning(deviceAuth_Timeout_ID))
	{
		osTimerStop(deviceAuth_Timeout_ID);
	}
}

/**
 *******************************************************************************
 * @brief  BLE event process callback
 * @param[in] evt_id  event id
 * @param[in] evt     event parameters
 *******************************************************************************
 */
static void app_adv_event_cb(uint16_t evt_id, const omble_evt_t *evt)
{
    if (evt_id == OB_GAP_EVT_CONNECTED) {
        log_debug("OB_GAP_EVT_CONNECTED(%d): %d\n", evt->gap.conn_idx, evt->gap.connected.adv_idx);
        memcpy(&last_conn, &evt->gap.connected.peer_addr, sizeof(ob_gap_addr_t));
		deviceAuthStartTimer();
		ble_auth_state = BLE_AUTH_STATE_WAITING;
		 BleStateMachine_Run(BLE_EVT_CONNECT);
    } else if (evt_id == OB_GAP_EVT_DISCONNECTED) {
        log_debug("OB_GAP_EVT_DISCONNECTED(%d): 0x%02X\n", evt->gap.conn_idx, evt->gap.disconnected.reason);
         BleStateMachine_Run(BLE_EVT_DISCONNECT);
		deviceAuthStopTimer();
		ble_auth_state = BLE_AUTH_STATE_IDLE;
       // ob_gap_adv_start(0, &adv_param, &adv_data, &scan_rsp_data);
    } else if (evt_id == OB_GAP_EVT_ADV_STATE_CHANGED) {
        log_debug("OB_GAP_EVT_ADV_STATE_CHANGED(%d), reason:%d\n", evt->gap.adv_state_changed.adv_idx,
               evt->gap.adv_state_changed.state);
		
    } else if (evt_id == OB_GAP_EVT_SCAN_REQ_RECV) {
        log_debug("OB_GAP_EVT_SCAN_REQ_RECV %d:  %d %02X:%02X:%02X:%02X:%02X:%02X\n",
               evt->gap.scan_req_recv.adv_idx,
               evt->gap.scan_req_recv.addr.addr_type,
               evt->gap.scan_req_recv.addr.addr[0],
               evt->gap.scan_req_recv.addr.addr[1],
               evt->gap.scan_req_recv.addr.addr[2],
               evt->gap.scan_req_recv.addr.addr[3],
               evt->gap.scan_req_recv.addr.addr[4],
               evt->gap.scan_req_recv.addr.addr[5]);
    } else {
    }
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief  Init advertising module
 *******************************************************************************
 */

void app_adv_init(void)
{
    ob_event_callback_reg(app_adv_event_cb);

    adv_param.own_addr_type = OB_ADV_ADDR_TYPE_RANDOM;
    adv_param.prim_phy = OB_ADV_PHY_1M;
    adv_param.secd_phy = OB_ADV_PHY_1M;
    adv_param.tx_pwr = 0;
    adv_param.filter_policy = OB_ADV_FILTER_NONE;
    adv_param.prim_ch_map = OB_ADV_CH_ALL;
    adv_param.prim_intv_min = 0x640;
    adv_param.prim_intv_max = 0x640;
    adv_param.local_addr = app_get_local_addr();//local_addr;
    adv_param.peer_addr = app_get_peer_addr();//&peer_addr;
    // adv_param.adv_properties = OB_ADV_PROP_EXT_CONN_NONSCAN;
    // adv_param.adv_properties = OB_ADV_PROP_LEGACY_DIRECT_IND_HIGH;
    // adv_param.adv_properties = OB_ADV_PROP_EXT_NONCONN_SCAN;
    adv_param.adv_properties = OB_ADV_PROP_LEGACY_IND;
  //  ob_gap_adv_start(0, &adv_param, &adv_data, NULL);
//	evt_app_adv_start();
}



void evt_app_adv_dissconn(void)
{
	ob_gap_disconnect(0, 0x13);
}
/**
 *******************************************************************************
 * @brief  Start advertising
 *******************************************************************************
 */
void evt_app_adv_start(void)
{

    userSetState=BLE_STATE_ADVERTISING;
	log_debug("evt_app_adv_start  = %d\r\n",userSetState);
    BleStateMachine_Run(BLE_EVT_START_ADV);
}
/**
 *******************************************************************************
 * @brief  Start advertising
 *******************************************************************************
 */
void evt_app_high_speed_adv_start(void)
{
	adv_param.prim_intv_min = 0xA0;
    adv_param.prim_intv_max = 0xA0;
    userSetState=BLE_STATE_ADVERTISING;
	log_debug("evt_app_adv_start  = %d\r\n",userSetState);
    BleStateMachine_Run(BLE_EVT_START_ADV);
}
/**
 *******************************************************************************
 * @brief  stop advertising
 *******************************************************************************
 */
void evt_app_adv_stop(void)
{
	log_debug("evt_app_adv_stop:%d\r\n",currentState);
    userSetState = BLE_STATE_IDLE;  // 更新用户意图
    
    // 根据当前状态执行不同的停止逻辑
    if (currentState == BLE_STATE_ADVERTISING) {
        // 直接停止广播
        ob_gap_adv_stop(0);
        currentState = BLE_STATE_IDLE;
    } else if (currentState == BLE_STATE_CONNECTED) {
        // 断开连接
        ob_gap_disconnect(0, 0x13);
    } else {
        // 已经是IDLE状态，无需操作
        log_debug("Already in IDLE state\r\n");
    }
}
/** @} */
