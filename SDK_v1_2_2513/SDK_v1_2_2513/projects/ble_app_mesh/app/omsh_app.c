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
 * @brief    mesh app
 * @details  mesh app
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

#if !RTE_ECDSA
#error "Mesh APP dependency ECDSA driver"
#endif

#if defined ( __ICCARM__ )
#pragma diag_suppress=Pe188
#endif


/*********************************************************************
 * LOCAL VARIABLES
 */
static osSemaphoreId_t xSemBluetooth = NULL;


/*******************************************************************************
 * STATIC FUNCTIONS
 */
static void vEvtEventHandler(void)
{
    if (xSemBluetooth) {
        osSemaphoreRelease(xSemBluetooth);
    }
}

static void msh_app_init(void)
{
    // Enable Rand Function
    ecdsa_config_t cfg = {
        .random_seed = 0xAC67BF78
    };
    drv_ecdsa_init(&cfg);

#if APP_MESH_LOG
    msh_api_log_init(0xFFFF, LOG_LEVEL_DBG2);
#endif

#if APP_MESH_FIRMS
    msh_app_firms_init();
#elif APP_MESH_TMALL
    msh_app_tmall_init();
#endif

    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Mesh APP Present.\r\n");
}

static void mesh_thread(void *arguments)
{
    // Evt and evt timer initialization
    evt_init();
    evt_timer_init();

    // Init RF
    drv_rf_init();

    // BLE initialization
    struct ob_stack_param param = {
        .max_connection         = OB_LE_HOST_CONNECTION_NB,
        .max_ext_adv_set        = OB_LE_HOST_ADV_SET_NUM,
        .max_att_mtu            = OB_LE_HOST_ATT_MTU,
        .max_gatt_serv_num      = OB_LE_HOST_MAX_GATT_SERV_NUM,
        .max_gatt_write_cache   = OB_LE_HOST_ATT_WRITE_CACHE_SIZE,
        .smp_sc_support         = OB_LE_HOST_SC_PAIRING,
    };
    omble_init(&param);

    // Enable mesh feature
    msh_api_set_stack_feature(APP_FEAT_RELAY, APP_FEAT_PROXY, APP_FEAT_FRND, APP_FEAT_LPN);

    // Mesh app initialization
    msh_app_init();

    // Mesh stack initialization
    msh_api_stack_init();

    // Mesh load params
    msh_api_storage_load();

    // Mesh stack start
    msh_api_stack_enable();

    // Create semaphore
    xSemBluetooth = osSemaphoreNew(1, 0, NULL);

    // Set evt callback
    evt_schedule_trigger_callback_set(vEvtEventHandler);

    while(1)
    {
        // schedule for handle evt
        evt_schedule();

        // wait os semaphore
        (void)osSemaphoreAcquire(xSemBluetooth, osWaitForever);
    }
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 * @brief Start ble mesh task
 **/
void vStartBleMeshTask(void)
{
    const osThreadAttr_t MeshThreadAttr = {
        .name           = "ble mesh",
        .stack_size     = 2048U,
        .priority       = osPriorityRealtime,
    };

    // Create Mesh Task
    osThreadNew(mesh_thread, NULL, &MeshThreadAttr);
}

/**
 * @brief  Start ble mesh app
 **/
void msh_app_start(bool prov_state)
{
#if APP_MESH_FIRMS
    msh_app_firms_start(prov_state);
#elif APP_MESH_TMALL
    msh_app_tmall_start(prov_state);
#endif
}

#if defined ( __ICCARM__ )
#pragma diag_default=Pe188
#endif
/** @} */
