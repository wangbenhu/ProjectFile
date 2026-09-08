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
#include "app_adv.h"

// 当前状态
static BleState_t currentState = BLE_STATE_IDLE;
typedef void (*BleStateHandler_t)(BleEvent_t evt);

static BleState_t userSetState = BLE_STATE_IDLE;
ble_auth_state_t ble_auth_state = BLE_AUTH_STATE_IDLE;

uint8_t systerm_set_data[6];
uint8_t Systerm_data_Addr[6];

// 1. 添加状态机锁，防止重入
static osMutexId_t bleStateMutex = NULL;
static BleStateMachine_t bleStateMachine;
/*****
**************************************************************************
 * MACROS
 */
#define hexdump(d, l) do{for(int i=0;i<l;i++)log_debug("%02X ", ((uint8_t*)d)[i]);log_debug("\n");}while(0);
#define AUTH_START_TIMEOUT (30000)
#define APP_ADV_FAST_INTV_MIN 0x40U
#define APP_ADV_FAST_INTV_MAX 0x80U
#define APP_ADV_SLOW_INTV_MIN 0x640U
#define APP_ADV_SLOW_INTV_MAX 0x680U

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
static uint8_t s_conn_idx;
static bool s_connected;
static ble_adv_mode_t s_adv_mode = BLE_ADV_MODE_FAST;
/*******************************************************************************
 * LOCAL FUNCTIONS
 */
void evt_app_adv_dissconn(void);
/*******************************************************************************
 * LOCAL FUNCTIONS
 */

BleState_t get_ble_status(void)
{
	   BleState_t state = BleStateMachine_GetState(&bleStateMachine);

    if (bleStateMutex != NULL &&
        osMutexAcquire(bleStateMutex, osWaitForever) == osOK) {
        state = BleStateMachine_GetState(&bleStateMachine);
        (void)osMutexRelease(bleStateMutex);
    }

    return state;
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
    //    log_debug("update_adv_data_with_mac: NULL pointer\r\n");
        return;
    }
    
    // 确保sdata有足够空间
    if (sizeof(sdata) < 11) {
    //    log_debug("update_adv_data_with_mac: sdata too small\r\n");
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

	log_debug("address : %02X:%02X:%02X:%02X:%02X:%02X\r\n", 
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

	update_adv_data_with_mac(local_addr);
    nvds_addr_save(local_addr);
}
static uint32_t BleDriver_StartAdvertising(void *context,
                                           BleAdvertisingMode_t mode)
{
    uint32_t result;

    (void)context;
    if (mode == BLE_ADV_MODE_HIGH_SPEED) {
        adv_param.prim_intv_min = APP_ADV_FAST_INTV_MIN;
        adv_param.prim_intv_max = APP_ADV_FAST_INTV_MAX;
    } else {
        adv_param.prim_intv_min = APP_ADV_SLOW_INTV_MIN;
        adv_param.prim_intv_max = APP_ADV_SLOW_INTV_MAX;
    }

    result = ob_gap_adv_start(0, &adv_param, &adv_data, NULL);
    if (result != OB_ERROR_NO_ERR) {
        log_debug("BLE start request failed: state=%d mode=%d error=%lu\r\n",
                  BleStateMachine_GetState(&bleStateMachine),
                  mode,
                  (unsigned long)result);
    }
    return result;
}

static uint32_t BleDriver_StopAdvertising(void *context)
{
    uint32_t result;

    (void)context;
    result = ob_gap_adv_stop(0);
    if (result != OB_ERROR_NO_ERR) {
        log_debug("BLE stop request failed: state=%d error=%lu\r\n",
                  BleStateMachine_GetState(&bleStateMachine),
                  (unsigned long)result);
    }
    return result;
}

static uint32_t BleDriver_Disconnect(void *context)
{
    uint32_t result;

    (void)context;
    result = ob_gap_disconnect(0, 0x13);
    if (result != OB_ERROR_NO_ERR) {
        log_debug("BLE disconnect request failed: state=%d error=%lu\r\n",
                  BleStateMachine_GetState(&bleStateMachine),
                  (unsigned long)result);
    }
    return result;
}

static void BleDriver_StartRestartTimer(void *context)
{
    (void)context;
    if (Disconn_Timeout_ID != NULL) {
        (void)osTimerStart(Disconn_Timeout_ID, 100U);
    }
}

static void BleDriver_CancelRestartTimer(void *context)
{
    (void)context;
    if (Disconn_Timeout_ID != NULL &&
        osTimerIsRunning(Disconn_Timeout_ID)) {
        (void)osTimerStop(Disconn_Timeout_ID);
    }
}

static const BleStateMachineOps_t bleStateMachineOps = {
    BleDriver_StartAdvertising,
    BleDriver_StopAdvertising,
    BleDriver_Disconnect,
    BleDriver_StartRestartTimer,
    BleDriver_CancelRestartTimer
};

void BleStateMachine_Run(BleEvent_t event)
{
    if (bleStateMutex == NULL ||
        osMutexAcquire(bleStateMutex, osWaitForever) != osOK) {
        log_debug("BLE state lock failed, event=%d\r\n", event);
        return;
    }

    log_debug("BLE state event: state=%d event=%d target=%d\r\n",
              BleStateMachine_GetState(&bleStateMachine),
              event,
              BleStateMachine_IsTargetEnabled(&bleStateMachine));
    BleStateMachine_Dispatch(&bleStateMachine, event);
    (void)osMutexRelease(bleStateMutex);
}

void DisconnTimerAdvCallback(void *argument)
{
	  (void)argument;
    BleStateMachine_Run(BLE_EVT_RESTART_TIMEOUT);
	  //evt_app_adv_start();
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
        if (get_ble_status() == BLE_STATE_CONNECTED) {
        // 断开连接
			evt_app_adv_dissconn();
//			userSetState = BLE_STATE_ADVERTISING;
//			ob_gap_disconnect(0, 0x13);
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
		s_conn_idx = evt->gap.conn_idx;
        s_connected = true;
      //  log_debug("OB_GAP_EVT_CONNECTED(%d): %d\n", evt->gap.conn_idx, evt->gap.connected.adv_idx);
        memcpy(&last_conn, &evt->gap.connected.peer_addr, sizeof(ob_gap_addr_t));
		deviceAuthStartTimer();
		ble_auth_state = BLE_AUTH_STATE_WAITING;
		 BleStateMachine_Run(BLE_EVT_CONNECTED_CONFIRMED);
    } else if (evt_id == OB_GAP_EVT_DISCONNECTED) {
		s_connected = false;
       // log_debug("OB_GAP_EVT_DISCONNECTED(%d): 0x%02X\n", evt->gap.conn_idx, evt->gap.disconnected.reason);
         BleStateMachine_Run(BLE_EVT_DISCONNECTED_CONFIRMED);
		deviceAuthStopTimer();
		ble_auth_state = BLE_AUTH_STATE_IDLE;
       // ob_gap_adv_start(0, &adv_param, &adv_data, &scan_rsp_data);
    } else if (evt_id == OB_GAP_EVT_ADV_STATE_CHANGED) {
        log_debug("OB_GAP_EVT_ADV_STATE_CHANGED(%d), reason:%d\n", evt->gap.adv_state_changed.adv_idx,
               evt->gap.adv_state_changed.state);
		switch (evt->gap.adv_state_changed.state) {
			case OB_GAP_ADV_ST_STARTED:
				BleStateMachine_Run(BLE_EVT_ADV_STARTED_CONFIRMED);
				break;
			case OB_GAP_ADV_ST_STOPPED_BY_USER:
				BleStateMachine_Run(BLE_EVT_ADV_STOPPED_BY_USER);
				break;
			case OB_GAP_ADV_ST_STOPPED_BY_CONNECTED:
				BleStateMachine_Run(BLE_EVT_ADV_STOPPED_BY_CONNECTED);
				break;
			case OB_GAP_ADV_ST_STOPPED_BY_TIMEOUT:
			case OB_GAP_ADV_ST_STOPPED_BY_EVENT:
			case OB_GAP_ADV_ST_STOPPED_UNEXPECTED:
				BleStateMachine_Run(BLE_EVT_ADV_STOPPED_UNEXPECTED);
				break;
			default:
				break;
        }
    } else if (evt_id == OB_GAP_EVT_SCAN_REQ_RECV) {
//        log_debug("OB_GAP_EVT_SCAN_REQ_RECV %d:  %d %02X:%02X:%02X:%02X:%02X:%02X\n",
//               evt->gap.scan_req_recv.adv_idx,
//               evt->gap.scan_req_recv.addr.addr_type,
//               evt->gap.scan_req_recv.addr.addr[0],
//               evt->gap.scan_req_recv.addr.addr[1],
//               evt->gap.scan_req_recv.addr.addr[2],
//               evt->gap.scan_req_recv.addr.addr[3],
//               evt->gap.scan_req_recv.addr.addr[4],
//               evt->gap.scan_req_recv.addr.addr[5]);
    } else {
    }
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */

bool app_adv_get_conn_idx(uint8_t *conn_idx)
{
    if ((conn_idx == NULL) || !s_connected) {
        return false;
    }
    *conn_idx = s_conn_idx;
    return true;
}
static bool app_adv_apply_mode(ble_adv_mode_t mode)
{
    if (mode == BLE_ADV_MODE_FAST) {
        adv_param.prim_intv_min = APP_ADV_FAST_INTV_MIN;
        adv_param.prim_intv_max = APP_ADV_FAST_INTV_MAX;
    } else if (mode == BLE_ADV_MODE_SLOW) {
        adv_param.prim_intv_min = APP_ADV_SLOW_INTV_MIN;
        adv_param.prim_intv_max = APP_ADV_SLOW_INTV_MAX;
    } else {
        return false;
    }

    s_adv_mode = mode;
    return true;
}
/**
 *******************************************************************************
 * @brief  Init advertising module
 *******************************************************************************
 */
void app_adv_init(void)
{
	    static const osMutexAttr_t mutex_attr = {
        .name = "bleStateMutex",
        .attr_bits = osMutexRecursive
    };

    bleStateMutex = osMutexNew(&mutex_attr);
    Disconn_Timeout_ID = osTimerNew(DisconnTimerAdvCallback, osTimerOnce, NULL, NULL);
    BleStateMachine_Init(&bleStateMachine, &bleStateMachineOps, NULL);
    if (bleStateMutex == NULL || Disconn_Timeout_ID == NULL) {
        LOG_LOC();
    }

    ob_event_callback_reg(app_adv_event_cb);

    adv_param.own_addr_type = OB_ADV_ADDR_TYPE_RANDOM;
    adv_param.prim_phy = OB_ADV_PHY_1M;
    adv_param.secd_phy = OB_ADV_PHY_1M;
    adv_param.tx_pwr = 0;
    adv_param.filter_policy = OB_ADV_FILTER_NONE;
    adv_param.prim_ch_map = OB_ADV_CH_ALL;
    adv_param.prim_intv_min = APP_ADV_SLOW_INTV_MIN;
    adv_param.prim_intv_max = APP_ADV_SLOW_INTV_MAX;
    adv_param.local_addr = app_get_local_addr();//local_addr;
    adv_param.peer_addr = app_get_peer_addr();//&peer_addr;
    // adv_param.adv_properties = OB_ADV_PROP_EXT_CONN_NONSCAN;
    // adv_param.adv_properties = OB_ADV_PROP_LEGACY_DIRECT_IND_HIGH;
    // adv_param.adv_properties = OB_ADV_PROP_EXT_NONCONN_SCAN;
    adv_param.adv_properties = OB_ADV_PROP_LEGACY_IND;
}

void evt_app_adv_dissconn(void)
{
    BleStateMachine_Run(BLE_EVT_DISCONNECT_REQUEST);
}
/**
 *******************************************************************************
 * @brief  Start advertising
 *******************************************************************************
 */
void evt_app_adv_start(void)
{
    BleStateMachine_Run(BLE_EVT_ENABLE_REQUEST);
}

void evt_app_high_speed_adv_start(void)
{
    BleStateMachine_Run(BLE_EVT_ENABLE_HIGH_SPEED_REQUEST);
}

/**
 *******************************************************************************
 * @brief  stop advertising
 *******************************************************************************
 */
void evt_app_adv_stop(void)
{
    BleStateMachine_Run(BLE_EVT_DISABLE_REQUEST);
}


uint32_t app_adv_start(ble_adv_mode_t mode)
{
//    if (!app_adv_apply_mode(mode)) {
//        return OB_ERROR_INVALID_PARAM;
//    }

//    return ob_gap_adv_start(0, &adv_param, &adv_data, &scan_rsp_data);
	
	 if (mode == BLE_ADV_MODE_FAST) {
		evt_app_high_speed_adv_start();
    } else if (mode == BLE_ADV_MODE_SLOW) {
		evt_app_adv_start();
    } else {
        return OB_ERROR_INVALID_PARAM;
    }
	return OB_ERROR_NO_ERR;
}

uint32_t app_adv_stop(void)
{
	
	evt_app_adv_stop();
    return OB_ERROR_NO_ERR;
}
uint32_t app_adv_disconnect(void)
{
	evt_app_adv_dissconn();
    return OB_ERROR_NO_ERR;
}
/** @} */
