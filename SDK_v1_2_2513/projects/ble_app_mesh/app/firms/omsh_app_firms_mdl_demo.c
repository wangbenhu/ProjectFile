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
 * @brief    Mesh firms app source file
 * @details  Mesh firms app source file
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
#define MM_FIRMS_DEMO_MODEL_ID              0X1000

#define MM_FIRMS_DEMO_MODEL_OPC_GET         0x0182
#define MM_FIRMS_DEMO_MODEL_OPC_SET         0x0282
#define MM_FIRMS_DEMO_MODEL_OPC_SET_UNACK   0x0382
#define MM_FIRMS_DEMO_MODEL_OPC_STATUS      0x0482


/*******************************************************************************
 * TYPEDEFS
 */
/// Firms mesh model layer environment structure
typedef struct mm_firms_demo_env {
    /// List of buffers containing message to process
    msh_api_list_t process_queue;

    /// Delay job
    msh_api_djob_t djob;

    /// Local model index
    uint8_t model_lid;
} mm_firms_demo_env_t;


/*******************************************************************************
 * STATIC VARIABLES
 */
/// Firms mesh demo model context
static mm_firms_demo_env_t *p_firms_demo_env;


/*******************************************************************************
 * STATIC FUNCTIONS
 */
/// TODO: Process received message
static void mm_firms_demo_process_receive_handler(void *params)
{
    msh_api_mdl_rec_data_t m_plain_rec;

    // Mesh stack process receive
    msh_api_model_pick_rx_packet(&p_firms_demo_env->process_queue, &m_plain_rec);

    /// TODO: Debug log
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "RX Msg: OP %x, SRC %x, DST %x, NET_LID %x, APP_LID %x, SEQ %x, TTL %x\r\n",
          m_plain_rec.opcode,
          m_plain_rec.src,
          m_plain_rec.dst,
          m_plain_rec.net_lid,
          m_plain_rec.app_lid,
          m_plain_rec.seq,
          m_plain_rec.ttl_ctl);

    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "RX Dat: Len %d, [%x,%x,%x,%x]\r\n",
          m_plain_rec.p_data_len,
          m_plain_rec.p_data[0],
          m_plain_rec.p_data[1],
          m_plain_rec.p_data[2],
          m_plain_rec.p_data[3]);

    /// TODO: Send ACK to Client
    uint8_t out_dat = m_plain_rec.p_data[0];

    msh_api_mdl_send_ack_t plain_ack = {
        .opcode         = MM_FIRMS_DEMO_MODEL_OPC_STATUS,
        .model_lid      = p_firms_demo_env->model_lid,
        .vendor         = false,
        .p_data_len     = sizeof(out_dat),
        .p_data         = &out_dat
    };

    msh_api_model_send_ack(&p_firms_demo_env->process_queue, &plain_ack);

    // Whatever the result,you must skip to next message
    msh_api_model_process_next(&p_firms_demo_env->process_queue, &p_firms_demo_env->djob);
}

/// Model receive data callback hanlder
static void mm_firms_demo_cb_rx(uint8_t mdl_lid, uint32_t opcode, msh_api_buf_t *p_api_buf, uint8_t app_key_lid, uint16_t src, int8_t rssi, bool not_relayed)
{
    msh_api_model_rx_handle(&p_firms_demo_env->process_queue, p_api_buf, &p_firms_demo_env->djob);
}

/// Model have sent data callback handler
static void mm_firms_demo_cb_sent(uint8_t mdl_lid, uint8_t tx_hdl, msh_api_buf_t *p_api_buf, uint16_t status)
{
    msh_api_model_sent_handle(p_api_buf);
}

/// Model receive satify callback handler
static void mm_firms_demo_cb_opcode_check(uint8_t mdl_lid, uint32_t opcode)
{
    uint16_t status = MSH_ERROR_INVALID_PARAMS;

    /// TODO: Check opcode valid, Whether it should be handled by this model
    if((opcode >= MM_FIRMS_DEMO_MODEL_OPC_GET) && (opcode <= MM_FIRMS_DEMO_MODEL_OPC_SET_UNACK)) {
        status = MSH_ERROR_NO_ERROR;
    }

    msh_api_model_opcode_check_handle(mdl_lid, opcode, status);
}

/// Inform the appilication about the new pulication period
static void mm_firms_demo_cb_publish_period(uint8_t mdl_lid, uint32_t period_ms)
{
    msh_api_model_publish_period_handle(mdl_lid, period_ms);
}


/*********************************************************************
 * CONST VARIABLES
 */
/// Firms mesh demo model callback table
static const msh_api_mdl_cb_t mm_firms_demo_cb = {
    .cb_rx              = mm_firms_demo_cb_rx,
    .cb_sent            = mm_firms_demo_cb_sent,
    .cb_opcode_check    = mm_firms_demo_cb_opcode_check,
    .cb_publish_period  = mm_firms_demo_cb_publish_period
};


/*******************************************************************************
 * EXPORTED FUNCTION DEFINITIONS
 */
uint16_t mm_firms_demo_init(bool reset, void *p_env, void *p_cfg)
{
    // TODO: Modify base on actual model id
    bool is_vendor = false;
    // TODO: Modify base on actual element id
    uint8_t elemt_id = 0;

    if(!reset) {
        // Get environment for mesh model
        p_firms_demo_env = (mm_firms_demo_env_t *)p_env;

        // Register the model
        msh_api_register_model(MM_FIRMS_DEMO_MODEL_ID, elemt_id, is_vendor, &mm_firms_demo_cb, &p_firms_demo_env->model_lid);

        // Initialize delayed job
        p_firms_demo_env->djob.cb = mm_firms_demo_process_receive_handler;
    } else {
        p_firms_demo_env = NULL;
    }

    // Return environment size
    return (sizeof(mm_firms_demo_env_t));
}

uint16_t mm_firms_demo_get_env_size(void *p_cfg)
{
    // Return environment size
    return (sizeof(mm_firms_demo_env_t));
}

#if defined ( __ICCARM__ )
#pragma diag_default=Pe188
#endif
/** @} */