/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup PMU PMU
 * @ingroup  DRIVER
 * @brief    PMU driver
 * @details  PMU driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_PMU_H
#define __DRV_PMU_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_PMU)
#include <stdint.h>
#include <stdbool.h>
#include "om_device.h"
#include "om_driver.h"
#include "om_common.h"


#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    bool dcdc_vout_is_set;
    int8_t dcdc_vout;
} pmu_dcdc_info_t;

/// PMU analog power type
typedef enum {
    /// PMU analog power type: RF for calib/test/carrier...
    PMU_ANA_RF          = (1U << 0),
    /// PMU analog power type: RF only for 24G MAC
    PMU_ANA_RF_24G      = (1U << 1),
    /// PMU analog power type: RF only for BLE MAC
    PMU_ANA_RF_BLE      = (1U << 2),
    /// PMU analog power type: GPADC
    PMU_ANA_GPADC       = (1U << 3),
    /// PMU analog power type: Calibration RC32K
    PMU_ANA_CALIB_RC32K = (1U << 4),
    /// PMU analog power type: Audio
    PMU_ANA_AUDIO       = (1U << 5),
    /// PMU analog power type: I2S
    PMU_ANA_I2S         = (1U << 6),
} pmu_ana_type_t;

/// 32k select, work with RTC, baseband in sleep
typedef enum {
    /// Select RC32K
    PMU_32K_SEL_RC           = 0,
    /// Select XTAL32K
    PMU_32K_SEL_32768HZ_XTAL = 1,
    /// Select DIV32K (Div by Xtal32M)
    PMU_32K_SEL_DIV          = 2,
} pmu_32k_sel_t;

/// Mode
typedef enum {
    /// Float (Input)
    PMU_PIN_MODE_FLOAT,
    /// Push pull (Output)
    PMU_PIN_MODE_PP,
    /// Pull up, @ref drv_pmu_pin_pullup_set()
    PMU_PIN_MODE_PU,
    /// Pull down (141kOHM~303kOHM)
    PMU_PIN_MODE_PD,
    /// Open drain
    PMU_PIN_MODE_OD,
    /// Open drain, pull up
    PMU_PIN_MODE_OD_PU,
} pmu_pin_mode_t;

/// Interrupt trigger type
typedef enum {
    /// Pin wakeup trigger by low level
    PMU_PIN_WAKEUP_LOW_LEVEL,
    /// Pin wakeup trigger by high level
    PMU_PIN_WAKEUP_HIGH_LEVEL,
    /// Pin wakeup trigger type is falling edge
    /// NOTE: It cannot be used in deep sleep mode with 32k off (@ref drv_pmu_32k_enable_in_deep_sleep)
    /// NOTE: If this mode is used, there can be one and only one IO as the wake-up source
    PMU_PIN_WAKEUP_FALLING_EDGE,
    /// Pin wakeup trigger type is rising edge
    /// NOTE: It cannot be used in deep sleep mode with 32k off (@ref drv_pmu_32k_enable_in_deep_sleep)
    /// NOTE: If this mode is used, there can be one and only one IO as the wake-up source
    PMU_PIN_WAKEUP_RISING_EDGE,
    /// Pin wakeup trigger type  is both edge
    PMU_PIN_WAKEUP_RISING_FAILING_EDGE,
    /// Pin wakeup trigger disable
    PMU_PIN_WAKEUP_DISABLE,
} pmu_pin_wakeup_type_t;

/// Charge status
typedef enum {
    /// Charger extract
    PMU_CHARGE_EXTRACT,
    /// Charger insert and charging
    PMU_CHARGE_CHARGING,
    /// Charger insert and charge complete
    PMU_CHARGE_COMPLETE,
} pmu_charge_status_t;

/// Reboot reason
typedef enum {
    /// Reboot from power on
    PMU_REBOOT_FROM_POWER_ON             = 0U,
    /// Reboot from ultra deep sleep
    PMU_REBOOT_FROM_ULTRA_DEEP_SLEEP     = 1U,
    /// Reboot from inside flash low voltage reset
    PMU_REBOOT_FROM_IFLASH_LOW_V         = 2U,
    /// Reboot from watch dog
    PMU_REBOOT_FROM_WDT                  = 3U,
    /// soft reset with unknown reason
    PMU_REBOOT_FROM_SOFT_RESET           = (1U << 2),
    /// Reboot from ISP
    PMU_REBOOT_FROM_SOFT_RESET_ISP       = (1U << 2) | 1U,
    /// Users take the reason to reboot
    PMU_REBOOT_FROM_SOFT_RESET_USER      = (1U << 2) | 2U,
} pmu_reboot_reason_t;

/// @cond
/// PIN driver current
typedef enum {
    /// Pin driver current: Normal
    PMU_PIN_DRIVER_CURRENT_NORMAL = 0,
    PMU_PIN_DRIVER_CURRENT_MAX    = 1,
} pmu_pin_driver_current_t;

/* pullup impedance seletion */
typedef enum {
    PMU_PIN_PULLUP_4K    = 0U,    /* 4K ohm pullup */
    PMU_PIN_PULLUP_10K   = 1U,    /* 10K ohm pullup */
    PMU_PIN_PULLUP_300K  = 2U,    /* 300K ohm pullup */
    PMU_PIN_PULLUP_4M    = 3U,    /* 4M ohm pullup */
} pmu_pin_pullup_t;

/// Sleep leave step
typedef enum {
    /// PMU sleep leave step1: on rc32m
    PMU_SLEEP_LEAVE_STEP1_ON_RC32M      = 0x01,
    /// PMU sleep leave step2: wait xtal32m
    PMU_SLEEP_LEAVE_STEP2_WAIT_XTAL32M  = 0x02,
    /// PMU sleep leave step3: finish
    PMU_SLEEP_LEAVE_STEP3_FINISH        = 0x04,
    /// PMU sleep leave step all
    PMU_SLEEP_LEAVE_STEP_ALL            = 0xFF,
} pmu_sleep_leave_step_t;

/// Iflash auto enter sleep mode wait time
typedef enum {
    PMU_IFLASH_MANU_POWER_UP                 = 0U,  /* iflash power up immediately */
    PMU_IFLASH_MANU_POWER_DOWN               = 1U,  /* iflash power down immediately */
    PMU_IFLASH_AUTO_SLEEP_POWER_DOWN_ENABLE  = 2U,  /* iflash enable automatic power down when sleep */
    PMU_IFLASH_AUTO_SLEEP_POWER_DOWN_DISABLE = 3U,  /* iflash disable automatic power down when sleep */
} pmu_iflash_power_mode_t;

typedef struct {
    uint32_t pmu_ram_block_sram0  : 1U;   /* SRAM 16 KiB, [0x20000000 + 0K,  0x20000000 + 16K) */
    uint32_t pmu_ram_block_sram1  : 1U;   /* SRAM 16 KiB, [0x20000000 + 16K, 0x20000000 + 32K) */
    uint32_t pmu_ram_block_sram2  : 1U;   /* SRAM 32 KiB, [0x20000000 + 32K, 0x20000000 + 64K) */
    uint32_t pmu_ram_block_sram3  : 1U;   /* SRAM 64 KiB, [0x20000000 + 64K, 0x20000000 + 128K) */
    uint32_t pmu_ram_block_sram4  : 1U;   /* SRAM 64 KiB, [0x20000000 + 128K, 0x20000000 + 192K) */
    uint32_t pmu_ram_block_sram5  : 1U;   /* SRAM 64 KiB, [0x20000000 + 192K, 0x20000000 + 256K) */
    uint32_t pmu_ram_block_icache : 1U;   /* ICACHE RAM */
    uint32_t pmu_ram_block_ble    : 1U;   /* BLE RAM */
    uint32_t pmu_ram_block_pso    : 1U;   /* RF RAM */
    uint32_t pmu_ram_block_rom    : 1U;   /* ROM */
} pmu_ram_block_t;
/// @endcond

typedef enum {
    PMU_POF_VOLTAGE_1P8V   = 0U,  /* 1.8V */
    PMU_POF_VOLTAGE_1P9V   = 1U,  /* 1.9V */
    PMU_POF_VOLTAGE_2P0V   = 2U,  /* 2.0V */
    PMU_POF_VOLTAGE_2P1V   = 3U,  /* 2.1V */
    PMU_POF_VOLTAGE_2P2V   = 4U,  /* 2.2V */
    PMU_POF_VOLTAGE_2P3V   = 5U,  /* 2.3V */
    PMU_POF_VOLTAGE_2P4V   = 6U,  /* 2.4V */
    PMU_POF_VOLTAGE_2P5V   = 7U,  /* 2.5V */
} pmu_pof_voltage_t;

typedef enum {
    PMU_POF_INT_BOTH_EDGE  = 0U,  /* both edge */
    PMU_POF_INT_POS_EDGE   = 1U,  /* positive edge */
    PMU_POF_INT_NEG_EDGE   = 2U,  /* negative edge */
} pmu_pof_int_edge_t;

typedef enum {
    PMU_DVDD_VOLTAGE_0P80V   = 0xFC, /* 0.80V */
    PMU_DVDD_VOLTAGE_0P85V   = 0xFD, /* 0.85V */
    PMU_DVDD_VOLTAGE_0P90V   = 0xFE, /* 0.90V */
    PMU_DVDD_VOLTAGE_0P95V   = 0xFF, /* 0.95V */
    PMU_DVDD_VOLTAGE_1P00V   = 0x00, /* 1.00V */
    PMU_DVDD_VOLTAGE_1P05V   = 0x01, /* 1.05V */
    PMU_DVDD_VOLTAGE_1P10V   = 0x02, /* 1.10V */
    PMU_DVDD_VOLTAGE_1P15V   = 0x03, /* 1.15V */
    PMU_DVDD_VOLTAGE_1P20V   = 0x04, /* 1.20V */
    PMU_DVDD_VOLTAGE_1P25V   = 0x05, /* 1.25V */
} pmu_dvdd_voltage_t;


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Set dvdd voltage
 *
 * @param[in] voltage  dvdd voltage
 *******************************************************************************
 */
extern void drv_pmu_dvdd_voltage_set(pmu_dvdd_voltage_t voltage);

/**
 *******************************************************************************
 * @brief pmu select xtal32m as top clock, call by system
 *
 * @note  This function only be called in system init.
 *******************************************************************************
 **/
extern void drv_pmu_xtal32m_startup(void);

/**
 *******************************************************************************
 * @brief pmu select 32k
 *
 * @param[in] clk32k  32k clock select
 *
 * @note
 *   If selecting PMU_32K_SEL_32768HZ_XTAL clock and using BLE,
 *   the `obcc.lpclk_drift` in obc_cc.h should be changed to correct ppm (may be 50 ppm)
 *
 *******************************************************************************
 **/
extern void drv_pmu_select_32k(pmu_32k_sel_t clk32k);

/**
 *******************************************************************************
 * @brief pmu get 32k select
 *
 * @return 32k select
 *******************************************************************************
 **/
extern pmu_32k_sel_t drv_pmu_select_32k_get(void);

/**
 *******************************************************************************
 * @brief control ram power state, It will take effect immediately. all ram
 *        (defined by pmu_ram_block_t) is power on from startup (ROM SystemInit()
 *        and SDK SystemInit()). If the corresponding struct member setting as 1
 *        indicate the blocks power down else power up immediately.
 *        sram0 is always on; sram1 and sram2 share a common power switch.
 *
 * @param[in] blocks  RAM block.
 *******************************************************************************
 **/
extern void drv_pmu_ram_power_down(const pmu_ram_block_t blocks);

/**
 *******************************************************************************
 * @brief control ram power state in sleep, It will work when the system enter
 *        sleep/deep sleep state. all ram (defined by pmu_ram_block_t) is power
 *        on from startup (ROM SystemInit() and SDK SystemInit()). the corresponding
 *        struct member setting as 1 indicate the blocks power down.
 *
 * @param[in] blocks  RAM block.
 *******************************************************************************
 **/
extern void drv_pmu_ram_power_down_in_sleep(const pmu_ram_block_t blocks);

/**
 *******************************************************************************
 * @brief  register step set, use: drv_pmu_register_step_set(...)
 *
 * @param[in] reg  reg
 * @param[in] mask  mask
 * @param[in] pos  pos
 * @param[in] value  value
 * @param[in] should_update  should update
 * @param[in] delay_us  delay us
 *******************************************************************************
 */
extern void drv_pmu_register_step_set(volatile uint32_t *reg, uint32_t mask, uint32_t pos, uint32_t value, bool should_update, uint32_t delay_us);

/**
 *******************************************************************************
 * @brief 32k enable in deep sleep
 *
 * @note
 *  The xtal 32768Hz crystal startup is very very slow (0.5s~2s),
 *  Its current consumption is about 0.3uA,
 *  Enable it in deepsleep, can make the wakeup faster
 *
 * @param[in] enable  enable or disable
 *******************************************************************************
 **/
extern void drv_pmu_32k_enable_in_deep_sleep(bool enable);

/**
 * @brief Change xtal 32k params
 *
 * @param[in] load_capacitance  load capacitance, range:0~3, default:0(0pF), step:1.5pF, max:4.5pF
 * @param[in] drive_current  drive current, range:0-3, default:0
 *
 * @note load_capacitance will effect xtal 32k startup time and precision,
 *       appropriate value can speed up startup time.
 **/
__STATIC_INLINE void drv_pmu_xtal32k_change_param(int load_capacitance, int drive_current)
{
    register_set(&OM_PMU->XTAL32K_CTRL, MASK_1REG(PMU_XTAL32K_CTRL_XTAL32K_CTUNE, load_capacitance));
    register_set(&OM_PMU->CLK_CTRL_2, MASK_1REG(PMU_CLK_CTRL_2_XTAL32K_ISEL, drive_current));
}

/**
 *******************************************************************************
 * @brief Change xtal 32m params
 *
 * @param[in] load_capacitance  load capacitance, range:0~63, default:32, step:0.38pF(<20ppm), max:23.94pF
 *
 * @note load_capacitance will effect xtal 32m precision and frequency offset.
 *******************************************************************************
 **/
__STATIC_INLINE void drv_pmu_xtal32m_change_param(uint32_t load_capacitance)
{
    register_set(&OM_PMU->CLK_CTRL_2, PMU_CLK_CTRL_2_CT_XTAL32M_MASK, (load_capacitance << PMU_CLK_CTRL_2_CT_XTAL32M_POS)); // TX
    register_set(&OM_PMU->OM26B_CFG0, PMU_OM26B_CFG0_XTAL32M_CT_RX_MASK,(load_capacitance << PMU_OM26B_CFG0_XTAL32M_CT_RX_POS)); // RX
}

/**
 *******************************************************************************
 * @brief  pmu xtal32m get param
 *******************************************************************************
 **/
__STATIC_INLINE uint32_t drv_pmu_xtal32m_get_param(void)
{
    return (OM_PMU->CLK_CTRL_2 & PMU_CLK_CTRL_2_CT_XTAL32M_MASK) >> PMU_CLK_CTRL_2_CT_XTAL32M_POS;
}

/**
 *******************************************************************************
 * @brief pmu gpio wakeup pin setup
 *
 * @param[in] pin_idx  pin index
 * @param[in] trigger_type  wakeup trigger type
 *******************************************************************************
 **/
extern void drv_pmu_wakeup_pin_set(uint8_t pin_idx, pmu_pin_wakeup_type_t trigger_type);

#if (RTE_PMU_PIN_WAKEUP_REGISTER_CALLBACK)
/**
 *******************************************************************************
 * @brief pmu wakeup pin register callback
 *
 * @param[in] isr_callback  callback
 *
 * @note
 *  When deepsleep, xtal32k startup is very very slow (0.5s-2s),
 *  So the deepsleep pin wakeup irq may don't debounce
 *******************************************************************************
 **/
extern void drv_pmu_wakeup_pin_register_callback(drv_isr_callback_t isr_callback);
#endif

/**
 *******************************************************************************
 * @brief The interrupt callback for PMU pin wakeup. It is a weak function. User should define
 *        their own callback in user file, other than modify it in the PMU driver.
 *
 * @param om_pmu      PMU device address
 * @param event       The driver event
 *                      - DRV_EVENT_PMU_PIN_WAKEUP_GPIO0
 *                      - DRV_EVENT_PMU_PIN_WAKEUP_GPIO1
 * @param int_status  interrupt status
 *******************************************************************************
 */
extern void drv_pmu_pin_wakeup_isr_callback(OM_PMU_Type *om_pmu, drv_event_t event, uint32_t int_status);

/**
 *******************************************************************************
 * @brief check pmu(pin wakeup) is allow enter deep_sleep
 *
 * @return true: allow enter sleep/deep_sleep
 *         false: not allow
 *******************************************************************************
 **/
extern bool drv_pmu_deep_sleep_is_allow(void);

/**
 *******************************************************************************
 * @brief  pmu sleep enter
 *
 * @param[in] is_deep_sleep  deep sleep
 * @param[in] reboot_req  reboot req
 *******************************************************************************
 */
extern void drv_pmu_sleep_enter(uint8_t is_deep_sleep, bool reboot_req);

/**
 *******************************************************************************
 * @brief pmu leave lowpower status, call by system
 *
 * @param step  step
 *******************************************************************************
 **/
extern void drv_pmu_sleep_leave(pmu_sleep_leave_step_t step);

/**
 *******************************************************************************
 * @brief Set pin input enable/disable
 *
 * @param[in] pin_idx  pin index
 * @param[in] ie  pin enable/disable
 *******************************************************************************
 **/
extern void drv_pmu_pin_input_enable(uint8_t pin_idx, uint8_t ie);

/**
 *******************************************************************************
 * @brief Set pin mode
 *
 * @param[in] pin_idx  pin index
 * @param[in] mode  pin mode
 *******************************************************************************
 **/
extern void drv_pmu_pin_mode_set(uint8_t pin_idx, pmu_pin_mode_t mode);

/**
 *******************************************************************************
 * @brief Set gpio driven current
 *
 * @param[in] pin_idx  pin index
 * @param[in] driven  current driven (Large driven current should be push-pull output)
 *******************************************************************************
 **/
extern void drv_pmu_pin_driven_current_set(uint8_t pin_idx, pmu_pin_driver_current_t driven);

/**
 *******************************************************************************
 * @brief Set pad pullup impedance
 *
 * @param[in] pin_idx  pin index
 * @param[in] pullup   pullup impedance
 *******************************************************************************
 **/
extern void drv_pmu_pin_pullup_set(uint8_t pin_idx, pmu_pin_pullup_t pullup);

/**
 *******************************************************************************
 * @brief pmu analog power enable, call by system
 *
 * @param[in] enable  enable/disable
 * @param[in] ana  analog type
 *******************************************************************************
 **/
extern void drv_pmu_ana_enable(bool enable, pmu_ana_type_t ana);

/**
 *******************************************************************************
 * @brief analog is enabled
 *
 * @param[in] ana  analog module
 *
 * @return enabled?
 *******************************************************************************
 **/
extern bool drv_pmu_ana_is_enabled(pmu_ana_type_t ana);

/**
 *******************************************************************************
 * @brief Force system to reset by the reason
 *******************************************************************************
 **/
extern void drv_pmu_reset(pmu_reboot_reason_t reason);

/**
 *******************************************************************************
 * @brief Force system to reboot by PMU_REBOOT_FROM_SOFT_RESET reason
 *******************************************************************************
 **/
__STATIC_INLINE CC_DEPRECATED void drv_pmu_force_reboot(void)
{
    drv_pmu_reset(PMU_REBOOT_FROM_SOFT_RESET);
}

/**
 *******************************************************************************
 * @brief  drv pmu reboot reason
 *
 * @return reason
 *******************************************************************************
 */
__STATIC_FORCEINLINE pmu_reboot_reason_t drv_pmu_reboot_reason(void)
{
    return (pmu_reboot_reason_t)((OM_PMU->RSVD_SW_REG[0] & PMU_RSVD_SW_REG_REBOOT_REASON_MASK) >> PMU_RSVD_SW_REG_REBOOT_REASON_POS);
}

/**
 *******************************************************************************
 * @brief Get charge status
 *
 * @return status
 *******************************************************************************
 **/
extern pmu_charge_status_t drv_pmu_charge_status(void);

/**
 *******************************************************************************
 * @brief get retention reg value
 *
 * @note This reg value will lost only after power down. default is 0x0000
 *
 * @return retention reg value
 *******************************************************************************
 **/
__STATIC_INLINE uint16_t drv_pmu_retention_reg_get(void)
{
    return (OM_PMU->SW_STATUS & PMU_SW_STATUS_USER_RETENTION_MASK) >> PMU_SW_STATUS_USER_RETENTION_POS;
}

/**
 *******************************************************************************
 * @brief set retention reg
 *
 * @note This reg value will lost only after power down. default is 0x0000
 *
 * @param[in] value  reg value
 *******************************************************************************
 **/
__STATIC_INLINE void drv_pmu_retention_reg_set(uint16_t value)
{
    OM_CRITICAL_BEGIN();
    REGW(&OM_PMU->SW_STATUS, MASK_1REG(PMU_SW_STATUS_USER_RETENTION, value));
    OM_CRITICAL_END();
}

/**
 * @brief  pmu 32k switch to rc
 *
 * @param[in] calib  calib
 * @param[in] pd_others  pd others
 **/
extern void drv_pmu_32k_switch_to_rc(bool calib, bool pd_others);

/// @cond
/**
 *******************************************************************************
 * @brief  drv pmu pin wakeup out of date
 *
 * @note:
 * PIN_WAKEUP_IRQ will be generated not only from pin-wakeup but also each GPIO irq(not sleep)
 * So do this to let PIN_WAKEUP_IRQ only generate from pin-wakeup
 *
 * @note: PIN_WAKEUP_IRQn must less then GPIO0_IRQn/GPIO1_IRQn
 *******************************************************************************
 */
extern void drv_pmu_pin_wakeup_out_of_date(void);

/**
 *******************************************************************************
 * @brief  pmu recalib sysclk
 *******************************************************************************
 **/
extern void drv_pmu_topclk_recalib(void);
/// @endcond

/**
 *******************************************************************************
 * @brief  enable INSIDE DCDC
 *
 * @param[in] enable  enable
 *******************************************************************************
 */
extern void drv_pmu_dcdc_enable(bool enable);

/**
 *******************************************************************************
 * @brief  pmu dcdc is enabled
 *
 * @return  is enabled
 *******************************************************************************
 **/
__STATIC_INLINE bool drv_pmu_dcdc_is_enabled(void)
{
    return (OM_PMU->SW_STATUS & PMU_SW_STATUS_DCDC_ENABLED_MASK) ? true : false;
}

/**
 *******************************************************************************
 * @brief  drv pmu pin wakeup isr
 *******************************************************************************
 */
extern void drv_pmu_pin_wakeup_isr(void);

/**
 *******************************************************************************
 * @brief  pmu topclk double preset
 *******************************************************************************
 **/
extern void drv_pmu_clk64m_enable(bool enable);

/**
 *******************************************************************************
 * @brief  pmu rc32m power enable NOTE: Make sure peripheral clock source is not
 * rc32m before power down rc32m.
 *
 * @return is_last_enabled ?
 *******************************************************************************
 **/
extern bool drv_pmu_rc32m_enable(bool enable);

/**
 *******************************************************************************
 * @brief  pmu topclk xtal32m power enable
 *
 * @param[in] enable
 *******************************************************************************
 **/
extern void drv_pmu_xtal32m_enable(bool enable);

/**
 *******************************************************************************
 * @brief  pmu topclk xtal32m is enabled
 *******************************************************************************
 **/
extern bool drv_pmu_xtal32m_is_enabled(void);

/**
 *******************************************************************************
 * @brief  pmu topclk xtal32m is enabled
 *******************************************************************************
 **/
extern bool drv_pmu_clk64m_is_enabled(void);

/**
 *******************************************************************************
 * @brief  pmu jtag enable
 *
 * @param[in] enable  enable
 *******************************************************************************
 */
__STATIC_FORCEINLINE void drv_pmu_jtag_enable(bool enable)
{
    REGW(&OM_PMU->MISC_CTRL, MASK_1REG(PMU_MISC_CTRL_O_JTAG_ENABLE, enable ? 1 : 0));
}

/**
 *******************************************************************************
 * @brief  Enable the chip pof voltage, set trigger threshold. Accuracy is positive
 *         or negative 5% of the threshold.
 *
 * @param[in] enable  true or false
 * @param[in] voltage Set trigger threshold, see @ref pmu_pof_voltage_t
 * @param[in] mode    Set interrupt edge, see @ref pmu_pof_int_edge_t
 *
 *******************************************************************************
 */
extern void drv_pmu_pof_enable(bool enable, pmu_pof_voltage_t voltage, pmu_pof_int_edge_t mode);

#if (RTE_PMU_POF_REGISTER_CALLBACK)
/**
 *******************************************************************************
 * @brief pmu pof register callback
 *
 * @param[in] isr_callback  callback
 *******************************************************************************
 **/
extern void drv_pmu_pof_register_callback(drv_isr_callback_t isr_callback);
#endif

/**
 *******************************************************************************
 * @brief The interrupt callback for PMU POF. It is a weak function. User should define
 *        their own callback in user file, other than modify it in the PMU driver.
 *
 * @param om_pmu      PMU device address
 * @param event       The driver event: DRV_EVENT_PMU_POF
 *******************************************************************************
 */
extern void drv_pmu_pof_isr_callback(OM_PMU_Type *om_pmu, drv_event_t event);

/**
 *******************************************************************************
 * @brief  drv pmu pof isr
 *******************************************************************************
 */
extern void drv_pmu_pof_isr(void);

/**
 *******************************************************************************
 * @brief  syspll power enable (pll96M/48M)
 *
 * @param[in] enable  enable
 *******************************************************************************
 */
extern void drv_pmu_syspll_power_enable(uint8_t enable);

/**
 *******************************************************************************
 * @brief  iflash get iflash ready state
 *******************************************************************************
 */
__STATIC_FORCEINLINE bool drv_pmu_iflash_ready_state_get(void)
{
    return (OM_PMU->FLASH_LOW_VOL_CTRL_0 & PMU_FLASH_LOW_VOL_CTRL_0_FLASH_POWER_READY_SYNC_MASK) ? true : false;
}

/**
 *******************************************************************************
 * @brief  iflash set power mode
 *******************************************************************************
 */
__STATIC_FORCEINLINE void drv_pmu_iflash_power_mode_set(pmu_iflash_power_mode_t mode)
{
    OM_CRITICAL_BEGIN();
    switch (mode) {
        case PMU_IFLASH_MANU_POWER_UP:
            REGW(&OM_PMU->ANA_PD, MASK_2REG(PMU_ANA_PD_PD_FLASH_ME, 1, PMU_ANA_PD_PD_FLASH_REG, 0));
            while(!drv_pmu_iflash_ready_state_get());
            break;
        case PMU_IFLASH_MANU_POWER_DOWN:
            REGW(&OM_PMU->ANA_PD, MASK_2REG(PMU_ANA_PD_PD_FLASH_ME, 1, PMU_ANA_PD_PD_FLASH_REG, 1));
            break;
        case PMU_IFLASH_AUTO_SLEEP_POWER_DOWN_ENABLE:
            REGW(&OM_PMU->ANA_PD, MASK_2REG(PMU_ANA_PD_PD_FLASH_ME, 0, PMU_ANA_PD_PD_FLASH_REG, 1));
            break;
        case PMU_IFLASH_AUTO_SLEEP_POWER_DOWN_DISABLE:
            REGW(&OM_PMU->ANA_PD, MASK_2REG(PMU_ANA_PD_PD_FLASH_ME, 0, PMU_ANA_PD_PD_FLASH_REG, 0));
            break;
        default:
            break;
    }
    OM_CRITICAL_END();
}

/**
 *******************************************************************************
 * @brief  iflash auto send deep powerdown command when sleep
 *
 * @param[in] enable  enable
 * @param[in] exit_wait inner flash exit sleep waitting time
 * @param[in] enter_wait inner flash enter sleep waitting time
 *******************************************************************************
 */
__STATIC_FORCEINLINE void drv_pmu_iflash_sleep_auto_send_power_cmd_disable(void)

{
    REGWA(&OM_PMU->SLEEP_WAKE_CTRL,
          MASK_4REG(PMU_SLEEP_WAKE_CTRL_RETENTION_RESTORE_EN, 1,
                    PMU_SLEEP_WAKE_CTRL_RETENTION_SAVE_EN, 1,
                    PMU_SLEEP_WAKE_CTRL_SF_RDI_EN, 0,
                    PMU_SLEEP_WAKE_CTRL_SF_DP_EN, 0));
}

/**
 *******************************************************************************
 * @brief  iflash low voltage detection enable, the system will reset if detect flash low power
 *
 *******************************************************************************
 */
__STATIC_FORCEINLINE void drv_pmu_iflash_low_voltage_detection_enable(void)
{
    // Enable low voltage reset
    OM_PMU->FLASH_LOW_VOL_CTRL_0 = 0x6666U;
}

/**
 *******************************************************************************
 * @brief  iflash low voltage detection disable
 *
 *******************************************************************************
 */
__STATIC_FORCEINLINE void drv_pmu_iflash_low_voltage_detection_disable(void)
{
    OM_PMU->FLASH_LOW_VOL_CTRL_1 = 0x9999U;
}

/**
 *******************************************************************************
 * @brief  ship mode sample enable check. If SM_CTRL(GPIO10) signal is 0 and call
 *         the function with enable = 1, then the system enter ship mdoe; If
 *         SM_CTRL(GPIO10) signal is 1 and call the function with enable = 0, then
 *         the system exit ship mode.
 *         Only sample one times at execute the funciton.
 *
 * @param[in] enable  0-disable  1-enable
 *******************************************************************************
 */
extern void drv_pmu_shelf_mode_enable(uint8_t enable);

/**
 *******************************************************************************
 * @brief  write PMU Basic register. This register use 32K clock to sync,
 *         the write operation is not atomic. So the function will recheck
 *         until the write operation is successful.
 *
 * @param[in] mask         the mask of the register field
 * @param[in] mask_value   the value of the register field
 *******************************************************************************
 */
extern void drv_pmu_basic_reg_set(uint32_t mask, uint32_t mask_value);

/**
 *******************************************************************************
 * @brief  dump PMU register
 *
 * @param[in] printf_dump_func  printf dump function
 *******************************************************************************
 */
extern void drv_pmu_dump(void *printf_dump_func);


#ifdef __cplusplus
}
#endif

#endif  /* (RTE_PMU) */

#endif  /* __DRV_PMU_H */


/** @} */
