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
 * @brief    Mesh app hook functions source file
 * @details  Mesh app hook functions source file
 * @version
 *
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


/*******************************************************************************
 * INCLUDES
 */
#include "omsh_app.h"

#if defined ( __ICCARM__ )
#pragma diag_suppress=Pe188
#endif
/*******************************************************************************
 * DEFINES
 */
/// see @om_tb_store_upd_type
/// 0: Network key updated
/// 1: Network key deleted
/// 2: Application key updated
/// 3: Application key deleted
/// 4: Model publication parameters updated
/// 5: Model subscription list updated
/// 6: Model/application key binding updated
/// 7: State updated
/// 8: Friendship with LPN updated
/// 9: Friendship with LPN lost
/// 10: Friendship with Friend updated
/// 11: Friendship with Friend lost
#define OMESH_APP_IND_APPKEY_UPDATED        (2)

/// 0: Configuration Server SIG Model ID
/// 1: Configuration Client SIG Model ID
/// 2: Health Server SIG Model ID
/// 3: Health Client SIG Model ID
#define OMESH_MODEL_ID_HLTHS                (0x0002)


/*******************************************************************************
 * TYPEDEF
 */
typedef struct {
    /// Command code
    uint32_t cmd_code;
    /// Status of the command execution
    uint16_t status;
} omesh_base_cmp_hook_t;

typedef struct {
    /// Command code
    uint32_t cmd_code;
    /// Status of the command execution
    uint16_t status;

    /// Model local index
    uint8_t model_lid;
} omesh_mdl_reg_cmp_hook_t;

typedef struct {
    /// Command code
    uint32_t cmd_code;
    /// Status of the command execution
    uint16_t status;

    /// X coordinate of public key
    uint8_t pub_key_x[32];
    /// Y coordinate of public key
    uint8_t pub_key_y[32];
} omesh_pub_key_read_cmp_hook_t;

typedef struct {
    /// Model local identifier
    uint8_t model_lid;
    /// Mesh message operation code
    uint32_t opcode;
} omesh_mdl_op_req_ind_hook_t;

typedef struct {
    /// Update type
    uint8_t upd_type;
    /// Entry length
    uint8_t length;
    /// Entry value pointer
    uint8_t *data;
} omesh_update_ind_hook_t;

typedef struct {
    /// Provisioning procedure state
    uint8_t state;
    /// Relevant only for provisioning failed
    uint16_t status;
} omesh_prov_state_ind_hook_t;

/// Inform about new publication period for Current Health state of primary element
typedef struct {
    /// Publication period in milliseconds when no fault is known
    uint32_t period_ms;
    /// Publication period in milliseconds when one or several fault are known
    uint32_t period_fault_ms;
} omesh_fault_period_ind_hook_t;

/// Request to start a test procedure of primary element
typedef struct {
    /// Company ID
    uint16_t comp_id;
    /// Test ID
    uint8_t  test_id;
    /// Indicate if MESH_FAULT_CFM message is expected
    bool cfm_needed;
} omesh_fault_test_req_ind_t;

typedef struct {
    /// Address of Friend node that sent the friend offer message
    uint16_t addr;
    /// Receive window value supported by the friend node
    uint8_t rx_window;
    /// Queue size available on the friend node
    uint8_t queue_size;
    /// Size of the subscription list that can be supported by the friend node
    uint8_t subs_list_size;
    /// RSSI measured by the friend node
    int8_t rssi;
} omesh_lpn_offer_ind_hook_t;

typedef struct {
    /// Status
    uint16_t status;
    /// Friend address
    uint16_t friend_addr;
} omesh_lpn_status_ind_hook_t;

typedef struct {
    /// Proxy connectable advertising state update types
    /// 0: MESH_PROXY_ADV_NODE_STOP, Advertising with Node Identity stopped
    /// 1: MESH_PROXY_ADV_NODE_START, Advertising with Node Identity started
    /// 2: MESH_PROXY_ADV_NET_STOP, Advertising with Network ID stopped
    /// 3: MESH_PROXY_ADV_NET_START, Advertising with Network ID started
    uint8_t state;
    /// Proxy connectable advertising state update reasons
    /// 0: MESH_PROXY_ADV_UPD_REASON_TIMEOUT, Stopped due to timeout(Default 60s)
    /// 1: MESH_PROXY_ADV_UPD_REASON_STATE, Stopped due to state update
    /// 2: MESH_PROXY_ADV_UPD_REASON_USER, User request
    /// 3: MESH_PROXY_ADV_UPD_REASON_PEER, Peer request
    /// 4: MESH_PROXY_ADV_UPD_REASON_PROV, Started due to provisioning using PB-GATT
    /// 5: MESH_PROXY_ADV_UPD_REASON_DISC, Disconnection
    uint8_t reason;
} omesh_proxy_adv_update_ind_hook_t;


/*******************************************************************************
 * PRIVATE VARIABLES
 */
static evt_timer_t omesh_fault_timer;
static uint8_t hlths_test_id = 0;
static uint8_t hlths_err_len = 1;
static uint8_t hlths_err_buf[4] = {0x5,0,0,0};


/*******************************************************************************
 * STATIC FUNCTION DEFINITIONS
 */
static void omesh_fault_period_timer_cb(evt_timer_t *timer, void *params)
{
    uint16_t status;

    uint8_t mdl_lid = 0;
    /// Destination address of the publish message
    uint16_t dst;
    /// Virtual Address Local Index
    uint8_t va_lid;
    /// Application key local index
    uint8_t app_lid;
    // Publication parameters
    uint8_t period,retx_param,friend_cred,ttl;

    if(MSH_ERROR_NO_ERROR == msh_api_get_model_lid_by_id(OMESH_MODEL_ID_HLTHS, &mdl_lid)) {
        status = msh_api_get_publi_param(mdl_lid, &dst, &va_lid, &app_lid, &ttl, &period, &retx_param, &friend_cred);

        if(MSH_ERROR_NO_ERROR == status) {
            // See at MshPRFv1.0.1 section 4.2.2.5,If the Publish TTL state is set to 1, the outgoing messages are published to local elements only
            if(ttl != 1) {
                msh_api_publish_health_status(hlths_err_len, hlths_err_buf);
            }
        }
    }
}


/*******************************************************************************
 * EXPORTED FUNCTION DEFINITIONS
 */
void msh_stk_enable_cmp_hook(void *p_evt)
{
    omesh_base_cmp_hook_t *p_hook = (omesh_base_cmp_hook_t *)p_evt;

    if(MSH_ERROR_NO_ERROR == p_hook->status) {
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Mesh stack enabled hook.\r\n");

        msh_app_start(msh_api_get_if_proved());
    }
}

void msh_stk_disable_cmp_hook(void *p_evt)
{
    omesh_base_cmp_hook_t *p_hook = (omesh_base_cmp_hook_t *)p_evt;

    if(MSH_ERROR_NO_ERROR == p_hook->status) {
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Mesh stack disabled hook.\r\n");
    }
}

void msh_mdl_register_cmp_hook(void *p_evt)
{
    omesh_mdl_reg_cmp_hook_t *p_hook = (omesh_mdl_reg_cmp_hook_t *)p_evt;

    if(MSH_ERROR_NO_ERROR == p_hook->status) {
        __LOG(LOG_SRC_APP, LOG_LEVEL_DBG3, "Registered a model hook. Model local index: %d.\r\n", p_hook->model_lid);
    }
}

void msh_mdl_publish_cmp_hook(void *p_evt)
{
    omesh_base_cmp_hook_t *p_hook = (omesh_base_cmp_hook_t *)p_evt;

    if(MSH_ERROR_NO_ERROR == p_hook->status) {
        __LOG(LOG_SRC_APP, LOG_LEVEL_DBG3, "Published a message hook.\r\n");
    }
}

void msh_mdl_opcode_req_ind_hook(void *p_ind)
{
    omesh_mdl_op_req_ind_hook_t *p_hook = (omesh_mdl_op_req_ind_hook_t *)p_ind;

    __LOG(LOG_SRC_APP, LOG_LEVEL_DBG3, "Model LID: 0x%x, support opcode: 0x%x.\r\n", p_hook->model_lid, p_hook->opcode);

    p_hook = p_hook;
}

void msh_mdl_rsp_send_cmp_hook(void *p_evt)
{
    omesh_base_cmp_hook_t *p_hook = (omesh_base_cmp_hook_t *)p_evt;

    if(MSH_ERROR_NO_ERROR == p_hook->status) {
        __LOG(LOG_SRC_APP, LOG_LEVEL_DBG3, "Responsed a message hook.\r\n");
    }
}

void msh_health_status_send_cmp_hook(void *p_evt)
{
    omesh_base_cmp_hook_t *p_hook = (omesh_base_cmp_hook_t *)p_evt;

    if(MSH_ERROR_NO_ERROR == p_hook->status) {
        __LOG(LOG_SRC_APP, LOG_LEVEL_DBG3, "Sent a health status message hook.\r\n");
    }
}

void msh_pub_key_read_cmp_hook(void *p_evt)
{
    omesh_pub_key_read_cmp_hook_t *p_hook = (omesh_pub_key_read_cmp_hook_t *)p_evt;

    if(MSH_ERROR_NO_ERROR == p_hook->status) {
        __LOG(LOG_SRC_APP, LOG_LEVEL_DBG3, "Read public key hook.\r\n");
    }
}

void msh_iv_upd_test_cmp_hook(void *p_evt)
{
    omesh_base_cmp_hook_t *p_hook = (omesh_base_cmp_hook_t *)p_evt;

    if(MSH_ERROR_NO_ERROR == p_hook->status) {
        __LOG(LOG_SRC_APP, LOG_LEVEL_DBG3, "IV updated by test mode hook.\r\n");
    }
}

void msh_lpn_start_cmp_hook(void *p_evt)
{
    omesh_base_cmp_hook_t *p_hook = (omesh_base_cmp_hook_t *)p_evt;

    if(MSH_ERROR_NO_ERROR == p_hook->status) {
        __LOG(LOG_SRC_APP, LOG_LEVEL_DBG2, "LPN started hook.\r\n");
    }
}

void msh_lpn_stop_cmp_hook(void *p_evt)
{
    omesh_base_cmp_hook_t *p_hook = (omesh_base_cmp_hook_t *)p_evt;

    if(MSH_ERROR_NO_ERROR == p_hook->status) {
        __LOG(LOG_SRC_APP, LOG_LEVEL_DBG2, "LPN stopped hook.\r\n");
    }
}

void msh_lpn_select_friend_cmp_hook(void *p_evt)
{
    omesh_base_cmp_hook_t *p_hook = (omesh_base_cmp_hook_t *)p_evt;

    if(MSH_ERROR_NO_ERROR == p_hook->status) {
        __LOG(LOG_SRC_APP, LOG_LEVEL_DBG2, "LPN selected friend hook.\r\n");
    }
}

void msh_lpn_offer_ind_hook(void *p_ind)
{
    omesh_lpn_offer_ind_hook_t *p_hook = (omesh_lpn_offer_ind_hook_t *)p_ind;

    __LOG(LOG_SRC_APP, LOG_LEVEL_DBG2, "LPN Receive Offer, addr=0x%x, rssi=%d.\r\n", p_hook->addr, p_hook->rssi);

    msh_api_lpn_select_friend(p_hook->addr);
}

void msh_lpn_status_ind_hook(void *p_ind)
{
    omesh_lpn_status_ind_hook_t *p_hook = (omesh_lpn_status_ind_hook_t *)p_ind;

    // 0x0183:  MESH_ERR_LPN_ESTAB_FAILED, Establishment failed after several attempts
    // 0x0283:  MESH_ERR_LPN_ESTAB_FAILED_KEY, Establishment failed due to failure during generation of friend keys
    // 0x0383:  MESH_ERR_LPN_ESTAB_FAILED_UPD, Establishment failed because Friend Update message not received after transmission of Friend Poll
    // 0x0483:  MESH_ERR_LPN_FRIEND_LOST_LOCAL, Friendship stopped due to local request
    // 0x0583:  MESH_ERR_LPN_FRIEND_LOST_TIMEOUT, Friendship lost due to request timeout
    if(false != p_hook->status) {
        msh_api_lpn_stop();
        __LOG(LOG_SRC_APP, LOG_LEVEL_DBG2, "LPN FriendShip error: 0x%x\r\n", p_hook->status);
    } else {
        __LOG(LOG_SRC_APP, LOG_LEVEL_DBG2, "LPN setup friendship success, Friend address: 0x%x\r\n", p_hook->friend_addr);
    }
}

void msh_proxy_ctrl_cmp_hook(void *p_evt)
{
    omesh_base_cmp_hook_t *p_hook = (omesh_base_cmp_hook_t *)p_evt;

    if(MSH_ERROR_NO_ERROR == p_hook->status) {
        __LOG(LOG_SRC_APP, LOG_LEVEL_DBG3, "Proxy control hook.\r\n");
    }
}

void msh_proxy_adv_update_ind_hook(void *p_ind)
{
    omesh_proxy_adv_update_ind_hook_t *p_hook = (omesh_proxy_adv_update_ind_hook_t *)p_ind;

    __LOG(LOG_SRC_APP, LOG_LEVEL_DBG2, "Proxy update state: %d, reason: %d.\r\n", p_hook->state, p_hook->reason);
}

void msh_storage_load_cmp_hook(void *p_evt)
{
    omesh_base_cmp_hook_t *p_hook = (omesh_base_cmp_hook_t *)p_evt;

    uint16_t node_addr = msh_api_get_prim_addr();

    if(MSH_ERROR_NO_ERROR == p_hook->status) {
        if((0x0000 != node_addr) && (0xFFFF != node_addr)) {
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Already provisioned, Node address : 0x%04x\r\n", node_addr);

            msh_api_set_relay_state(APP_FEAT_RELAY);
            msh_api_set_proxy_state(APP_FEAT_PROXY);
            msh_api_set_friend_state(APP_FEAT_FRND);
            msh_api_set_lpn_state(APP_FEAT_LPN);

            if(APP_FEAT_LPN) {
                msh_api_lpn_start();
            }

            if(APP_FEAT_PROXY) {
                msh_api_proxy_con_adv_ctrl(MSH_PROXY_ADV_CTL_START_NET);
            }
        }
        else {
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Unprovisioned state.\r\n");
        }
    }
}

void msh_storage_save_cmp_hook(void *p_evt)
{
    omesh_base_cmp_hook_t *p_hook = (omesh_base_cmp_hook_t *)p_evt;

    if(MSH_ERROR_NO_ERROR == p_hook->status) {
        __LOG(LOG_SRC_APP, LOG_LEVEL_DBG3, "Storage saved complete hook.\r\n");
    }
}

void msh_storage_config_cmp_hook(void *p_evt)
{
    omesh_base_cmp_hook_t *p_hook = (omesh_base_cmp_hook_t *)p_evt;

    if(MSH_ERROR_NO_ERROR == p_hook->status) {
        __LOG(LOG_SRC_APP, LOG_LEVEL_DBG3, "Storage config complete hook.\r\n");
    }
}

void msh_node_reset_ind_hook(void *p_ind)
{
    (void)p_ind;

    // Get first nvds tag value
    uint8_t tag = MESH_TB_GET_NVDS_TAG(0);

    // Clean bt address
    nvds_del(NVDS_TAG_BD_ADDRESS);

    // Clean all NVDS tags related of mesh
    for(; tag < NVDS_TAG_MESH_LAST; tag++) {
        nvds_del(tag);
    }

    __LOG(LOG_SRC_APP, LOG_LEVEL_DBG2, "Successfully Unbind and Reset.\r\n");

    drv_pmu_reset(PMU_REBOOT_FROM_SOFT_RESET);
}

void msh_update_ind_hook(void *p_ind)
{
    omesh_update_ind_hook_t *p_hook = (omesh_update_ind_hook_t *)p_ind;

    if(OMESH_APP_IND_APPKEY_UPDATED == p_hook->upd_type) {
#if APP_MESH_TMALL
        // Self bind for Vendor Tmall Model, regardless UUID version is 0 or 1
        //omesh_app_tmall_self_bind(p_hook->data[0]);
#endif
    }

    // NVDS save params
    msh_api_storage_save();
}

void msh_fault_get_req_ind_hook(void *p_ind)
{
    (void)p_ind;
    msh_api_publish_health_fault_status(hlths_err_len, hlths_err_buf);
}

void msh_fault_test_req_ind_hook(void *p_ind)
{
    omesh_fault_test_req_ind_t *p_hook = (omesh_fault_test_req_ind_t *)p_ind;
    uint16_t cid,pid,vid,feat,loc;

    msh_api_get_comp_info(&cid, &pid, &vid, &feat, &loc);

    if(hlths_test_id != p_hook->test_id || cid != p_hook->comp_id) {
        return;
    }

    if(p_hook->cfm_needed) {
        msh_api_publish_health_fault_status(hlths_err_len, hlths_err_buf);
    }
}

void msh_fault_period_ind_hook(void *p_ind)
{
    omesh_fault_period_ind_hook_t *p_hook = (omesh_fault_period_ind_hook_t *)p_ind;

    if(p_hook->period_fault_ms != 0) {
        __LOG(LOG_SRC_APP, LOG_LEVEL_DBG2, "Hlths period fault %d ms.\r\n", p_hook->period_fault_ms);
        evt_timer_set(&omesh_fault_timer, hlths_err_len != 0 ? p_hook->period_fault_ms : p_hook->period_ms, EVT_TIMER_REPEAT, omesh_fault_period_timer_cb, NULL);
    }
}

void msh_fault_clear_ind_hook(void *p_ind)
{
    (void)p_ind;
    // Clear current fault
    hlths_err_len = 0;
    memset(hlths_err_buf, 0, sizeof(hlths_err_buf));
}

void msh_prov_state_ind_hook(void *p_ind)
{
    omesh_prov_state_ind_hook_t *p_hook = (omesh_prov_state_ind_hook_t *)p_ind;

    if(MSH_PROV_SUCCED == p_hook->state) {
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Successfully provisioned, Node address: 0x%02x\r\n", msh_api_get_prim_addr());

        msh_api_set_relay_state(APP_FEAT_RELAY);
        msh_api_set_proxy_state(APP_FEAT_PROXY);
        msh_api_set_friend_state(APP_FEAT_FRND);
        msh_api_set_lpn_state(APP_FEAT_LPN);

        if(APP_FEAT_LPN) {
            msh_api_lpn_start();
        }

        if(APP_FEAT_PROXY) {
            msh_api_proxy_con_adv_ctrl(MSH_PROXY_ADV_CTL_START_NET);
        }
    }
}

void msh_prov_param_req_ind_hook(void *p_ind)
{
    (void)p_ind;

    // UUID pointer
    uint8_t *p_uuid;
    // Prepare GAPC_PARAM_UPDAET_CMD message
    msh_api_prov_param_cfm_t cfm;

    p_uuid = msh_api_get_uuid();
    memcpy(cfm.dev_uuid, p_uuid, MESH_DEV_UUID_LEN);

    /// URI hash
    cfm.uri_hash        = 0;
    /// OOB information
    cfm.oob_info        = 0;

#if defined(APP_MESH_TMALL)
    /// Publish key OOB information available
    cfm.pub_key_oob     = 1;
    /// Static OOB information available
    cfm.static_oob      = 1;
#else
    /// Public key OOB information available
    cfm.pub_key_oob     = 0;
    cfm.static_oob      = 0;
#endif

    /// Maximum size of output OOB supported
    cfm.out_oob_size    = 0;
    /// Supported output OOB actions
    cfm.out_oob_action  = 0;
    /// Maximum size in octets of input OOB supported
    cfm.in_oob_size     = 0;
    /// Supported input OOB actions
    cfm.in_oob_action   = 0;
    /// Number of element support
    cfm.nb_elt          = 1;
    /// Bit field providing additional information
    cfm.info            = 0;

    msh_api_prov_param_req_ind_handle(&cfm);
}

void msh_prov_auth_req_ind_hook(void *p_ind)
{
    (void)p_ind;
    msh_api_prov_auth_data_cfm_t cfm;

    cfm.accept          = 1;
#if defined(APP_MESH_TMALL)
    cfm.auth_size       = MESH_OOB_AUTH_DATA_LEN;
    memcpy(cfm.auth_data, msh_api_get_prov_auth_data(), MESH_OOB_AUTH_DATA_LEN);
#else
    cfm.auth_size       = 0;
#endif

    msh_api_prov_auth_data_req_ind_handle(&cfm);
}

#if defined ( __ICCARM__ )
#pragma diag_default=Pe188
#endif

/** @} */