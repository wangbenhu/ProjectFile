/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */
/*******************************************************************************
 * INCLUDES
 */
#include <string.h>
#include "omble.h"
#include "service_common.h"
#include "common_def.h"

/*********************************************************************
 * LOCAL VARIABLES
 */
#if SERVICE_BATTARY
static uint8_t val_bat_lev = 90;
static uint8_t val_chg_status = 0x01;
#endif /* SERVICE_BATTARY */

// Start handle
static uint16_t m_start_handle;

extern CHARGE_STATUS_T PM_GetChargeStatus(void);
extern uint8_t PM_GetBatteryCapacity(void);
/*********************************************************************
 * TYPEDEFS
 */
enum {
    #if defined(GAP_DEVICE_NAME) || defined(GAP_APPEARANCE)
    IDX_GAP_SVC,
    #if defined(GAP_DEVICE_NAME)
    IDX_DNAME_CHAR,
    IDX_DNAME_VAL,
    #endif
    #if defined(GAP_APPEARANCE)
    IDX_APPE_CHAR,
    IDX_APPE_VAL,
    #endif
    #endif /*defined(GAP_DEVICE_NAME) || defined(GAP_APPEARANCE) */

    #if (defined(DIS_SYSTEM_ID) || defined(DIS_HARD_VERSION) || defined(DIS_SOFT_VERSION) || defined(DIS_MANU_NAME_STR) || defined(DIS_PNP_ID) || defined(DIS_FIRM_REV_STR))
    IDX_DIS_SVC,
    #if defined(DIS_SYSTEM_ID)
    IDX_SYSID_CHAR,
    IDX_SYSID_VAL,
    #endif
    #if defined(DIS_HARD_VERSION)
    IDX_H_REV_CHAR,
    IDX_H_REV_VAL,
    #endif
    #if defined(DIS_SOFT_VERSION)
    IDX_S_REV_CHAR,
    IDX_S_REV_VAL,
    #endif
	#if defined(DIS_FIRM_REV_STR)
	IDX_FIRM_REV_CHAR,
    IDX_FIRM_REV_VAL,
	#endif
    #if defined(DIS_MANU_NAME_STR)
    IDX_MANU_CHAR,
    IDX_MANU_VAL,
    #endif
    #if defined(DIS_PNP_ID)
    IDX_PNPID_CHAR,
    IDX_PNPID_VAL,
    #endif
    #endif /* DIS_DEFINED  */

    #if SERVICE_BATTARY
    IDX_BATT_SVC,
    IDX_BATVAL_CHAR,
    IDX_BATVAL_VAL,
    IDX_BATVAL_DESC,
	IDX_CHGSTAT_CHAR,    // 新增：充电状态特征声明
    IDX_CHGSTAT_VAL,     // 新增：充电状态特征值  
    IDX_CHGSTAT_DESC,    // 新增：充电状态CCCD
    #endif /* SERVICE_BATTARY */
};
/*********************************************************************
 * LOCAL FUNCTIONS
 */
#if SERVICE_BATTARY
static uint8_t convert_charge_status_to_ble(CHARGE_STATUS_T charge_status)
{
    uint8_t ble_status = 0;
    
    switch (charge_status) {
        case CHARGE_STATUS_NO_CHARGE:
            // 未充电状态：电池存在(1), 能够放电(1), 能够充电(1), 电量未知(111)
            // 设备不在充电，但具备充放电能力
            ble_status = (1 << 0) | (1 << 1) | (1 << 2) | (0x07 << 3);
            break;
            
        case CHARGE_STATUS_CHARGING:
            // 充电中状态：电池存在(1), 能够放电(1), 能够充电(1), 电量未知(111)
            // 设备正在充电，但仍然具备放电能力（比如充电时也能使用设备）
            ble_status = (1 << 0) | (1 << 1) | (1 << 2) | (0x07 << 3);
            break;
            
        case CHARGE_STATUS_FULL:
            // 充满状态：电池存在(1), 能够放电(1), 能够充电(1), 电量良好(000)
            ble_status = (1 << 0) | (1 << 1) | (1 << 2) | (0x00 << 3);
            break;
            
        case CHARGE_STATUS_INVALID:
        default:
            // 无效/未知状态：所有状态未知
            ble_status = (0 << 0) | (0 << 1) | (0 << 2) | (0x07 << 3);
            break;
    }
    
    return ble_status;
}

static void batt_level_report(void)
{
    uint16_t len = 1;
    ob_gatts_hvx_t hvx = {
        OB_HANDLE_VALUE_NTF,
        m_start_handle + IDX_BATVAL_VAL, // handle
        &val_bat_lev,
        len,
    };
    ob_gatts_send_hvx(0, &hvx);
}

static void chg_status_report(void)
{
    uint16_t len = 1;
    ob_gatts_hvx_t hvx = {
        OB_HANDLE_VALUE_NTF,
        m_start_handle + IDX_CHGSTAT_VAL, // handle
        &val_chg_status,
        len,
    };
    ob_gatts_send_hvx(0, &hvx);
}

void batt_level_change(uint8_t val)
{
    val_bat_lev = val;
    batt_level_report();
}

void chg_status_change(uint8_t status)
{
    val_chg_status = status;
    chg_status_report();
}
void update_charging_status(CHARGE_STATUS_T charge_status)
{
    uint8_t ble_status = convert_charge_status_to_ble(charge_status);
    chg_status_change(ble_status);
}
#endif /* SERVICE_BATTARY */

static void service_discovery_event_cb(uint16_t evt_id, const omble_evt_t *evt)
{
    if (evt_id == OB_GATTS_EVT_READ_REQ) {
        const ob_gatts_evt_read_req_t *req = &evt->gatt.read_req;
        if (req->att_hdl == m_start_handle + IDX_DNAME_VAL) {
            if (req->offset < sizeof(GAP_DEVICE_NAME) - 1) {
                ob_gatts_read_response(evt->gatt.conn_idx, OB_GATT_ERR_NO_ERROR, (uint8_t *)GAP_DEVICE_NAME + req->offset,
                                       sizeof(GAP_DEVICE_NAME) - 1 - req->offset);
            } else {
                ob_gatts_read_response(evt->gatt.conn_idx, OB_GATT_ERR_NO_ERROR, (uint8_t *)"", 0);
            }
            #if defined(GAP_APPEARANCE)
        } else if (req->att_hdl == m_start_handle + IDX_APPE_VAL) {
            ob_gatts_read_response(evt->gatt.conn_idx, OB_GATT_ERR_NO_ERROR, (uint8_t *)GAP_APPEARANCE,
                                   sizeof(GAP_APPEARANCE) - 1);
            #endif
            #if SERVICE_BATTARY
        } else if (req->att_hdl == m_start_handle + IDX_BATVAL_VAL) {
            ob_gatts_read_response(evt->gatt.conn_idx, OB_GATT_ERR_NO_ERROR, &val_bat_lev,
                                   sizeof(val_bat_lev));
		// 新增：充电状态读取处理
		} else if (req->att_hdl == m_start_handle + IDX_CHGSTAT_VAL) {
		ob_gatts_read_response(evt->gatt.conn_idx, OB_GATT_ERR_NO_ERROR, &val_chg_status,
							   sizeof(val_chg_status));
            #endif
            #if defined(DIS_SYSTEM_ID)
        } else if (req->att_hdl == m_start_handle + IDX_SYSID_VAL) {
            ob_gatts_read_response(evt->gatt.conn_idx, OB_GATT_ERR_NO_ERROR, (uint8_t *)DIS_SYSTEM_ID, sizeof(DIS_SYSTEM_ID) - 1);
            #endif
            #if defined(DIS_HARD_VERSION)
        } else if (req->att_hdl == m_start_handle + IDX_H_REV_VAL) {
            ob_gatts_read_response(evt->gatt.conn_idx, OB_GATT_ERR_NO_ERROR, (uint8_t *)DIS_HARD_VERSION,
                                   sizeof(DIS_HARD_VERSION) - 1);
            #endif
            #if defined(DIS_SOFT_VERSION)
        } else if (req->att_hdl == m_start_handle + IDX_S_REV_VAL) {
            ob_gatts_read_response(evt->gatt.conn_idx, OB_GATT_ERR_NO_ERROR, (uint8_t *)DIS_SOFT_VERSION,
                                   sizeof(DIS_SOFT_VERSION) - 1);
            #endif
			#if defined(DIS_FIRM_REV_STR)
        } else if (req->att_hdl == m_start_handle + IDX_FIRM_REV_VAL) {
            ob_gatts_read_response(evt->gatt.conn_idx, OB_GATT_ERR_NO_ERROR, (uint8_t *)DIS_FIRM_REV_STR,
                                   sizeof(DIS_FIRM_REV_STR) - 1);
            #endif
            #if defined(DIS_MANU_NAME_STR)
        } else if (req->att_hdl == m_start_handle + IDX_MANU_VAL) {
            if (req->offset < sizeof(DIS_MANU_NAME_STR) - 1) {
                ob_gatts_read_response(evt->gatt.conn_idx, OB_GATT_ERR_NO_ERROR, (uint8_t *)DIS_MANU_NAME_STR + req->offset,
                                       sizeof(DIS_MANU_NAME_STR) - 1 - req->offset);
            } else {
                ob_gatts_read_response(evt->gatt.conn_idx, OB_GATT_ERR_NO_ERROR, (uint8_t *)"", 0);
            }
            #endif
            #if defined(DIS_PNP_ID)
        } else if (req->att_hdl == m_start_handle + IDX_PNPID_VAL) {
            ob_gatts_read_response(evt->gatt.conn_idx, OB_GATT_ERR_NO_ERROR, (uint8_t *)DIS_PNP_ID, sizeof(DIS_PNP_ID) - 1);
            #endif
        } else {
        }
    } else if (evt_id == OB_GATTS_EVT_WRITE_REQ) {
        #if SERVICE_BATTARY
        if (evt->gatt.write_req.att_hdl == m_start_handle + IDX_BATVAL_DESC) {
            if (*evt->gatt.write_req.data) {
				batt_level_change(PM_GetBatteryCapacity());
//                batt_level_report();
            }
        }
		// 新增：充电状态CCCD处理
        else if (evt->gatt.write_req.att_hdl == m_start_handle + IDX_CHGSTAT_DESC) {
            if (*evt->gatt.write_req.data) {
				update_charging_status(PM_GetChargeStatus());
//                chg_status_report();
            }
        }
        #endif /* SERVICE_BATTARY */
    }
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
void gatt_service_init(void)
{
    uint16_t gatt_handle;
    static const uint8_t serv_gap[2]  = {0x00, 0x18};
    static const uint8_t char_name[2] = {0x00, 0x2a};
    static const uint8_t char_2a01[2] = {0x01, 0x2a};
    static const ob_gatt_item_t atts_gap[] = {
        { ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
        { char_name,       OB_UUID_16BIT, OB_ATT_PROP_READ },
        { ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
        { char_2a01,       OB_UUID_16BIT, OB_ATT_PROP_READ },
    };
    static const ob_gatt_serv_t att_serv_gap = {
        serv_gap, OB_UUID_16BIT,
        sizeof(atts_gap) / sizeof(atts_gap[0]), atts_gap
    };
    ob_gatts_add_service(&att_serv_gap, &m_start_handle);

    #if (defined(DIS_SYSTEM_ID) || defined(DIS_HARD_VERSION) || defined(DIS_SOFT_VERSION) || defined(DIS_MANU_NAME_STR) || defined(DIS_PNP_ID) || defined(DIS_FIRM_REV_STR))
    static const uint8_t serv_dev[2]  = {0x0A, 0x18};
    #if defined(DIS_SYSTEM_ID)
    static const uint8_t char_sys_id[2] = {0x23, 0x2a};
    #endif
    #if defined(DIS_HARD_VERSION)
    static const uint8_t char_hard_v[2] = {0x27, 0x2a};
    #endif
    #if defined(DIS_SOFT_VERSION)
    static const uint8_t char_soft_v[2] = {0x28, 0x2a};
    #endif
	#if defined(DIS_FIRM_REV_STR)
    static const uint8_t char_firm_v[2] = {0x26, 0x2a};
    #endif
    #if defined(DIS_MANU_NAME_STR)
    static const uint8_t char_m_name[2] = {0x29, 0x2a};
    #endif
    #if defined(DIS_PNP_ID)
    static const uint8_t char_pnp_id[2] = {0x50, 0x2a};
    #endif
    static const ob_gatt_item_t atts_dev[] = {
        #if defined(DIS_SYSTEM_ID)
        { ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
        { char_sys_id,     OB_UUID_16BIT, OB_ATT_PROP_READ },
        #endif
        #if defined(DIS_HARD_VERSION)
        { ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
        { char_hard_v,     OB_UUID_16BIT, OB_ATT_PROP_READ },
        #endif
        #if defined(DIS_SOFT_VERSION)
        { ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
        { char_soft_v,     OB_UUID_16BIT, OB_ATT_PROP_READ },
        #endif
		#if defined(DIS_FIRM_REV_STR)
        { ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
        { char_firm_v,     OB_UUID_16BIT, OB_ATT_PROP_READ },
        #endif
        #if defined(DIS_MANU_NAME_STR)
        { ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
        { char_m_name,     OB_UUID_16BIT, OB_ATT_PROP_READ },
        #endif
        #if defined(DIS_PNP_ID)
        { ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
        { char_pnp_id,     OB_UUID_16BIT, OB_ATT_PROP_READ },
        #endif
    };
    static const ob_gatt_serv_t att_serv_dev = {
        serv_dev, OB_UUID_16BIT,
        sizeof(atts_dev) / sizeof(atts_dev[0]), atts_dev
    };
    ob_gatts_add_service(&att_serv_dev, &gatt_handle);
    #endif
    #if SERVICE_BATTARY
    static const uint8_t serv_bat[2]
        = {0x0F, 0x18};
    static const uint8_t char_bat[2] = {0x19, 0x2a};
	 static const uint8_t char_power_state[2] = {0x1A, 0x2A};
    static const ob_gatt_item_t atts_bat[] = {
        { ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
        { char_bat,        OB_UUID_16BIT, OB_ATT_PROP_READ | OB_ATT_PROP_NTF, 0 },
        { ob_att_cccd_def, OB_UUID_16BIT, OB_ATT_PROP_READ | OB_ATT_PROP_WRITE },
		
		{ ob_att_char_def, OB_UUID_16BIT, OB_ATT_PROP_READ },
        { char_power_state, OB_UUID_16BIT, OB_ATT_PROP_READ | OB_ATT_PROP_NTF, 0 },
        { ob_att_cccd_def, OB_UUID_16BIT, OB_ATT_PROP_READ | OB_ATT_PROP_WRITE },
    };
    static const ob_gatt_serv_t att_serv_bat = {
        serv_bat, OB_UUID_16BIT,
        sizeof(atts_bat) / sizeof(atts_bat[0]), atts_bat
    };
    ob_gatts_add_service(&att_serv_bat, &gatt_handle);
    #endif
}

void service_common_init(void)
{
    gatt_service_init();
    ob_event_callback_reg(service_discovery_event_cb);
}

/** @} */
