#include "m_battery.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "common_def.h"
#include "board_define.h"
// ==================== 配置参数 ====================

#define ADC_COLLECT_NUM 		10 	// 充电检测消抖次数

// ==================== 数据类型定义 ====================
typedef enum {
    BATTERY_STATE_DISCHARGING,    // 放电状态
    BATTERY_STATE_CHARGING,       // 充电状态
    BATTERY_STATE_FULL,           // 满电状态
    BATTERY_STATE_ERROR           // 错误状态
} BatteryState;
typedef enum {
    CHARGE_EVENT_NONE,            // 无事件
    CHARGE_EVENT_PLUGGED,         // 充电器插入
    CHARGE_EVENT_UNPLUGGED,       // 充电器拔出
    CHARGE_EVENT_FULL             // 充满电
} ChargeEvent;

drv_gpadc_config_t config;

/// Read finish flag
static uint32_t adc_sum = 0;

static uint32_t adc_average = 0;
static uint8_t battery_updata_flag = 0;//1更新完成 0暂未更新
// ==================== 静态变量 ====================

/// Buffer that stores the data to be received
static int16_t adc_read_buffer[ADC_COLLECT_NUM];

typedef struct {
    uint16_t mv;   // 电压阈值（mV）
    uint8_t  soc;  // 对应 SOC（%）
} SOC_POINT_T;

/*  按电压从高到低排序 */
//static const SOC_POINT_T soc_table[] =
//{
//    { 4150,  99 },   // 接近满电（100% 由 FullDetect 决定）
//    { 4050,  95 },
//    { 3950,  90 },
//    { 3850,  80 },
//    { 3750,  70 },
//    { 3650,  55 },
//    { 3550,  35 },
//    { 3500,  20 },
//    { 3400,  10 },
//    { 3300,   0 },
//};
//static const SOC_POINT_T soc_table[] =
//{
//    { 4000,  99 },   // 显示满（100% 由 FullDetect 决定）
//    { 3950,  97 },
//    { 3900,  95 },
//    { 3800,  85 },
//    { 3700,  75 },
//    { 3600,  55 },
//    { 3550,  35 },
//    { 3500,  20 },   // 你指定的关键点
//    { 3400,  10 },
//    { 3300,   0 },   // 空电
//};
// static const SOC_POINT_T soc_table[] =
// {
// 	{ 4100,  99 },   // 新满电（最高只到99%）
// 	{ 4050,  92 },
// 	{ 4000,  88 },
// 	{ 3950,  80 },
// 	{ 3900,  70 },
// 	{ 3800,  60 },
// 	{ 3700,  45 },
// 	{ 3600,  30 },
// 	{ 3550,  20 },
// 	{ 3500,  10 },   // 保留你原来的关键点
// 	{ 3400,   5 },
// 	{ 3300,   0 },   // 空电
// };
// static const SOC_POINT_T soc_table[] =
// {
// 	{ 4100,  99 },   // 满电（最高只到99%）
// 	{ 4080,  98 },   // 高电压区：50mV仅下降2%
//     { 4050,  96 },   // 高电压区：50mV仅下降2%
// 	{ 4000,  94 },   // 高电压区：50mV仅下降3%
// 	{ 3950,  90 },   // 高电压区：50mV仅下降4%
// 	{ 3900,  85 },   // 高电压区：50mV仅下降5%
// 	{ 3850,  78 },   // 中高电压区
// 	{ 3800,  70 },   // 中电压区
// 	{ 3750,  60 },   // 中电压区
// 	{ 3700,  48 },   // 中低电压区
// 	{ 3650,  35 },   // 低电压区开始加快
// 	{ 3600,  28 },   // 低电压区
// 	{ 3550,  20 },   // 低电压区：50mV下降10%
// 	{ 3500,   12 },   // 接近放电终止
// 	{ 3400,   7 },   // 放电终止前
// 	{ 3300,   0 },   // 空电
// };
static const SOC_POINT_T soc_table[] =
{
    { 4100,  99 },
    { 4080,  99 },
    { 4050,  98 },
    { 4020,  97 },
    { 4000,  95 },  // 拔充电后的电压回落，最多只掉约 4%
    { 3950,  88 },
    { 3900,  72 },
    { 3850,  63 },
    { 3800,  58 },
    { 3750,  54 },
    { 3700,  50 },
    { 3650,  44 },
    { 3600,  32 },
    { 3550,  20 },  // 低电量提示
    { 3500,  12 },
    { 3450,   7 },
    { 3400,   4 },
    { 3320,   0 },
};

#define ARRAY_SIZE(arr)   (sizeof(arr) / sizeof((arr)[0]))

/**
 * 锂电池电量百分比计算
 * @param voltage 电池电压 (V)
 * @return 电量百分比 0-100%
 */

uint8_t VoltageToPercent(uint32_t mv)
{
	    uint32_t soc;
  /* 上限保护：绝不在这里给 100% */
    if (mv >= soc_table[0].mv)
        return soc_table[0].soc;

    /* 下限保护 */
    if (mv <= soc_table[ARRAY_SIZE(soc_table)-1].mv)
        return soc_table[ARRAY_SIZE(soc_table)-1].soc;

    /* 查找所在区间 */
    for (uint32_t i = 0; i < ARRAY_SIZE(soc_table) - 1; i++)
    {
        uint32_t mv_hi = soc_table[i].mv;
        uint32_t mv_lo = soc_table[i+1].mv;

        if (mv <= mv_hi && mv > mv_lo)
        {
            uint32_t soc_hi = soc_table[i].soc;
            uint32_t soc_lo = soc_table[i+1].soc;

            /* 线性插值 */
            soc = soc_lo +
                  (mv - mv_lo) * (soc_hi - soc_lo) /
                  (mv_hi - mv_lo);

            return (uint8_t)soc;
        }
    }

    return 0; // 理论不会到
}
uint32_t PercentToVoltage(uint8_t soc)
{
    uint32_t mv;

    /* 上限保护 */
    if (soc >= soc_table[0].soc)
        return soc_table[0].mv;

    /* 下限保护 */
    if (soc <= soc_table[ARRAY_SIZE(soc_table)-1].soc)
        return soc_table[ARRAY_SIZE(soc_table)-1].mv;

    /* 查找所在区间 */
    for (uint32_t i = 0; i < ARRAY_SIZE(soc_table) - 1; i++)
    {
        uint32_t soc_hi = soc_table[i].soc;
        uint32_t soc_lo = soc_table[i+1].soc;

        if (soc <= soc_hi && soc > soc_lo)
        {
            uint32_t mv_hi = soc_table[i].mv;
            uint32_t mv_lo = soc_table[i+1].mv;

            /* 线性插值（反向） */
            mv = mv_lo +
                 (uint32_t)(soc - soc_lo) * (mv_hi - mv_lo) /
                 (soc_hi - soc_lo);

            return mv;
        }
    }

    return soc_table[ARRAY_SIZE(soc_table)-1].mv; // fallback
}
//uint8_t VoltageToPercent(uint32_t voltage)
//{
//     // 电压范围检查
//    if (voltage >= BATTERY_VOLTAGE_FULL) return 100;
//    if (voltage <= BATTERY_VOLTAGE_EMPTY_M2) return 0;
//    uint32_t voltage_mV = voltage;
//    uint32_t percentage;
//    
//    // 区间1: 4200mV - 3700mV 对应 100% - 20%
//    if (voltage_mV >= BATTERY_VOLTAGE_NORMAL_M5) {
//        // 计算：20 + (电压差 / 500mV * 80)
//        // 使用乘法避免除法精度损失
//        percentage = 20 + ((voltage_mV - BATTERY_VOLTAGE_NORMAL_M5) * 80) / 
//                     (BATTERY_VOLTAGE_FULL - BATTERY_VOLTAGE_NORMAL_M5);
//    }
//    // 区间2: 3700mV - 3600mV 对应 20% - 0%
//    else if (voltage_mV >= BATTERY_VOLTAGE_LOW_M3) {
//        // 计算：10 + (电压差 / 100mV * 10)
//        percentage = ((voltage_mV - BATTERY_VOLTAGE_LOW_M3) * 10) / 
//                     (BATTERY_VOLTAGE_NORMAL_M5 - BATTERY_VOLTAGE_LOW_M3);
//    }
//    // 区间3: 3600mV - 3400mV 对应 10% - 0%
//    else {
//      
//    }
//    // 限制在0-100范围内
//    if (percentage > 100) return 100;
//    
//    return (uint8_t)percentage;
//}
#if (0)
/**
 * @brief 设置充电控制
 * @param enable true: 允许充电, false: 停止充电
 */
static void set_charge_control(bool enable) {
    // 实际实现：控制充电芯片
    // HAL_GPIO_WritePin(CHARGE_EN_GPIO_Port, CHARGE_EN_Pin, enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
}/**
 * @brief 一阶低通滤波器
 * @param new_value 新值
 * @param old_value 旧值
 * @param alpha 滤波系数 (0-1)
 * @return 滤波后的值
 */
static float low_pass_filter(float new_value, float old_value, float alpha) {
    return alpha * new_value + (1.0f - alpha) * old_value;
}
/**
 * @brief 发送充满电通知
 */
static void notify_full_charge(void) {
    // 实际实现：LED、蜂鸣器、消息等
    // printf("Battery fully charged!\n");
}
// ==================== 非易失存储接口 ====================
// 需要根据实际平台实现

/**
 * @brief 保存关键数据到非易失存储
 */
static void nv_store_battery_data(uint32_t address, const void* data, size_t size) {
    // 实际实现：写入EEPROM/Flash
    // eeprom_write(address, data, size);
}

/**
 * @brief 从非易失存储读取数据
 */
static void nv_read_battery_data(uint32_t address, void* data, size_t size) {
    // 实际实现：读取EEPROM/Flash
    // eeprom_read(address, data, size);
}

// ==================== 重启保护管理 ====================
typedef struct {
    // 重启相关
    uint32_t restart_count;        // 重启计数
    uint32_t last_restart_time;    // 上次重启时间
    bool is_restart_recovery;      // 重启恢复期标志
    uint32_t recovery_start_time;  // 恢复开始时间
    
    // 状态保存
    float last_stable_voltage;     // 上次稳定电压
    float last_stable_soc;         // 上次稳定电量
    BatteryState last_state;       // 上次状态
    uint32_t last_save_time;       // 上次保存时间
    
    // 重启检测
    bool power_loss_detected;      // 掉电检测
    uint32_t boot_voltage_samples; // 启动电压采样
    float boot_voltage;            // 启动时电压
} RestartManager;

// ==================== 改进的电池管理 ====================
typedef struct {
    BatteryManager battery;        // 基础电池管理
    RestartManager restart;        // 重启管理
    
    // 重启保护参数
    float pre_restart_voltage;     // 重启前电压
    float restart_voltage_threshold; // 重启电压阈值
    uint32_t min_operation_time;   // 最小运行时间
} EnhancedBatteryManager;

static EnhancedBatteryManager enhanced_battery;
/**
 * @brief 读取ADC原始值
 * @return ADC原始值 (0-4095)
 */
static uint16_t read_adc_value(void) {
    // 实际实现：读取ADC通道
    // return HAL_ADC_GetValue();
    return 0; // 示例
}
/**
 * @brief 获取系统时间(ms)
 * @return 当前时间戳
 */
static uint32_t get_system_time(void) {
    // 实际实现：获取系统滴答
    // return HAL_GetTick();
    return 0; // 示例
}
/**
 * @brief 检测充电器是否插入
 * @return true: 充电器插入, false: 充电器未插入
 */
static bool is_charger_connected(void) {
    // 实际实现：检测充电器插入GPIO
    // return HAL_GPIO_ReadPin(CHARGER_DETECT_GPIO_Port, CHARGER_DETECT_Pin);
    return false; // 示例
}
// ==================== 重启检测和恢复 ====================

/**
 * @brief 检测是否发生了重启
 * @return true: 检测到重启, false: 正常启动
 */
static bool detect_restart_event(void) {
    static uint32_t boot_count_addr = 0x1000;
    static uint32_t boot_count = 0;
    uint32_t stored_count = 0;
    
    // 读取存储的启动计数
    nv_read_battery_data(boot_count_addr, &stored_count, sizeof(stored_count));
    
    // 存储的计数加1
    uint32_t new_count = stored_count + 1;
    nv_store_battery_data(boot_count_addr, &new_count, sizeof(new_count));
    
    // 检测重启：当前电压与上次存储电压差异大
    float stored_voltage = 0;
    nv_read_battery_data(0x1010, &stored_voltage, sizeof(stored_voltage));
    
    float current_voltage = (read_adc_value() * ADC_REF_VOLTAGE / ADC_MAX_VALUE) * VOLTAGE_DIVIDER_RATIO;
    
    // 电压跳变过大（超过0.5V）且不是充电状态
    bool voltage_jump = fabsf(current_voltage - stored_voltage) > 0.5f;
    bool charger_connected = is_charger_connected();
    
    return (voltage_jump && !charger_connected);
}

/**
 * @brief 重启恢复期特殊处理
 */
static void handle_restart_recovery(void) {
    uint32_t current_time = get_system_time();
    
    if (!enhanced_battery.restart.is_restart_recovery) {
        return;
    }
    
    uint32_t recovery_elapsed = current_time - enhanced_battery.restart.recovery_start_time;
    
    if (recovery_elapsed < RESTART_RECOVERY_TIME) {
        // 恢复期内：使用保守参数
        enhanced_battery.battery.is_voltage_stable = false;
        
        // 恢复期内使用上次保存的状态
        static bool state_restored = false;
        if (!state_restored) {
            enhanced_battery.battery.filtered_soc = enhanced_battery.restart.last_stable_soc;
            enhanced_battery.battery.displayed_soc = enhanced_battery.restart.last_stable_soc;
            enhanced_battery.battery.state = enhanced_battery.restart.last_state;
            state_restored = true;
        }
        
        // 恢复期内限制更新频率
        if (recovery_elapsed % 500 < 100) { // 每500ms更新一次
            enhanced_battery.battery.last_update_time = current_time;
        }
    } else {
        // 恢复期结束
        enhanced_battery.restart.is_restart_recovery = false;
    }
}

/**
 * @brief 保存关键状态到非易失存储
 */
static void save_critical_states(void) {
    static uint32_t last_save = 0;
    uint32_t current_time = get_system_time();
    
    // 每30秒保存一次，或状态变化时保存
    if ((current_time - last_save > 30000) || 
        (enhanced_battery.battery.state != enhanced_battery.restart.last_state)) {
        
        // 保存电压
        float voltage = enhanced_battery.battery.filtered_voltage;
        nv_store_battery_data(0x1010, &voltage, sizeof(voltage));
        
        // 保存电量
        float soc = enhanced_battery.battery.displayed_soc;
        nv_store_battery_data(0x1020, &soc, sizeof(soc));
        
        // 保存状态
        BatteryState state = enhanced_battery.battery.state;
        nv_store_battery_data(0x1030, &state, sizeof(state));
        
        enhanced_battery.restart.last_stable_voltage = voltage;
        enhanced_battery.restart.last_stable_soc = soc;
        enhanced_battery.restart.last_state = state;
        enhanced_battery.restart.last_save_time = current_time;
        
        last_save = current_time;
    }
}
/**
 * @brief 处理充电器插入/拔出事件
 * @param charger_connected 当前充电器连接状态
 */
static void handle_charge_event(bool charger_connected) {
    static uint8_t debounce_count = 0;
    
    if (charger_connected != battery.was_charging) {
        debounce_count++;
        
        if (debounce_count >= CHARGE_DETECT_DEBOUNCE) {
            if (charger_connected) {
                battery.last_event = CHARGE_EVENT_PLUGGED;
                battery.is_soc_locked = false; // 解除电量锁定，允许上涨
            } else {
                battery.last_event = CHARGE_EVENT_UNPLUGGED;
                battery.is_soc_locked = false; // 解除电量锁定
            }
            debounce_count = 0;
            battery.was_charging = charger_connected;
        }
    } else {
        debounce_count = 0;
    }
}
// ==================== 改进的充电状态处理 ====================
/**
 * @brief 处理充电状态逻辑
 */
static void handle_charge_state(void) {
    bool charger_connected = is_charger_connected();
    
    // 处理充电器插入/拔出事件
    handle_charge_event(charger_connected);
    
    // 状态机逻辑
    switch (battery.state) {
        case BATTERY_STATE_DISCHARGING:
            if (charger_connected) {
                battery.state = BATTERY_STATE_CHARGING;
                set_charge_control(true);
            }
            break;
            
        case BATTERY_STATE_CHARGING:
            if (!charger_connected) {
                battery.state = BATTERY_STATE_DISCHARGING;
                set_charge_control(false);
            } else if (battery.filtered_voltage >= BATTERY_CHARGE_CUTOFF) {
                battery.state = BATTERY_STATE_FULL;
                battery.full_charge_time = get_system_time();
                battery.last_event = CHARGE_EVENT_FULL;
                notify_full_charge();
            }
            break;
            
        case BATTERY_STATE_FULL:
            if (!charger_connected) {
                battery.state = BATTERY_STATE_DISCHARGING;
                set_charge_control(false);
            } else if (battery.filtered_voltage < BATTERY_RECHARGE_VOLTAGE) {
                battery.state = BATTERY_STATE_CHARGING;
                set_charge_control(true);
            }
            break;
            
        default:
            break;
    }
}
/**
 * @brief 重启安全的充电状态处理
 */
static void handle_charge_state_restart_safe(void) {
    bool charger_connected = is_charger_connected();
    
    // 重启恢复期内，对充电器状态进行消抖处理
    static uint8_t charge_debounce = 0;
    static bool last_charge_state = false;
    
    if (charger_connected != last_charge_state) {
        charge_debounce++;
        if (charge_debounce >= 10) { // 更严格的消抖
            last_charge_state = charger_connected;
            charge_debounce = 0;
            
            // 充电器状态变化时，延迟处理以避免重启
            if (enhanced_battery.restart.is_restart_recovery) {
                // 恢复期内不立即响应充电器状态变化
                return;
            }
        }
    } else {
        charge_debounce = 0;
    }
    
    // 使用基础的状态处理
    handle_charge_state();
}

/**
 * @brief 重启安全的电压读取
 */
static float read_voltage_restart_safe(void) {
    uint16_t adc_value = read_adc_value();
    
    // 重启恢复期内使用多次平均
    if (enhanced_battery.restart.is_restart_recovery) {
        static uint16_t samples[5] = {0};
        static uint8_t idx = 0;
        
        samples[idx++ % 5] = adc_value;
        
        uint32_t sum = 0;
        for (int i = 0; i < 5; i++) {
            sum += samples[i];
        }
        adc_value = sum / 5;
    }
    
    return (adc_value * ADC_REF_VOLTAGE / ADC_MAX_VALUE) * VOLTAGE_DIVIDER_RATIO;
}

// ==================== 改进的初始化 ====================

/**
 * @brief 重启安全的电池管理初始化
 */
void battery_manager_init_restart_safe(void) {
    uint32_t current_time = get_system_time();
    
    // 延迟初始化，等待电源稳定
    if (current_time < RESTART_DELAY_MS) {
        return; // 延迟返回，由调用者重试
    }
    
    memset(&enhanced_battery, 0, sizeof(EnhancedBatteryManager));
    
    // 检测重启事件
    bool restart_detected = detect_restart_event();
    
    if (restart_detected) {
        enhanced_battery.restart.restart_count++;
        enhanced_battery.restart.is_restart_recovery = true;
        enhanced_battery.restart.recovery_start_time = current_time;
        
        // 从存储恢复状态
        nv_read_battery_data(0x1020, &enhanced_battery.restart.last_stable_soc, sizeof(float));
        nv_read_battery_data(0x1030, &enhanced_battery.restart.last_state, sizeof(BatteryState));
        
        // 设置恢复期初始状态
        enhanced_battery.battery.filtered_soc = enhanced_battery.restart.last_stable_soc;
        enhanced_battery.battery.displayed_soc = enhanced_battery.restart.last_stable_soc;
        enhanced_battery.battery.state = enhanced_battery.restart.last_state;
        
        // 恢复期内使用保守参数
        enhanced_battery.battery.is_soc_locked = true;
    } else {
        // 正常启动
        enhanced_battery.battery.state = BATTERY_STATE_DISCHARGING;
        enhanced_battery.battery.filtered_soc = 50.0f;
        enhanced_battery.battery.displayed_soc = 50.0f;
    }
    
    enhanced_battery.battery.is_initialized = true;
    set_charge_control(false);
}

// ==================== 改进的主更新循环 ====================
/**
 * @brief 处理电量百分比计算，保证充电时只上涨，放电时只下降
 */
static void handle_soc_logic(void) {
    // 计算原始电量
    battery.raw_soc = VoltageToPercent(battery.filtered_voltage);
    
    // 根据状态处理电量
    switch (battery.state) {
        case BATTERY_STATE_CHARGING:
            // 充电状态：电量只上涨不下降
            if (battery.raw_soc > battery.filtered_soc || !battery.is_soc_locked) {
                battery.filtered_soc = low_pass_filter(
                    battery.raw_soc, 
                    battery.filtered_soc, 
                    SOC_FILTER_ALPHA
                );
                battery.is_soc_locked = true; // 锁定，避免电压下降时电量下降
            }
            break;
            
        case BATTERY_STATE_DISCHARGING:
            // 放电状态：电量只下降不上涨
            if (battery.raw_soc < battery.filtered_soc || !battery.is_soc_locked) {
                battery.filtered_soc = low_pass_filter(
                    battery.raw_soc, 
                    battery.filtered_soc, 
                    SOC_FILTER_ALPHA
                );
                battery.is_soc_locked = true; // 锁定，避免电压回升时电量上涨
            }
            break;
            
        case BATTERY_STATE_FULL:
            // 满电状态：保持100%
            battery.filtered_soc = 100.0f;
            break;
            
        default:
            // 其他状态：直接使用滤波值
            battery.filtered_soc = low_pass_filter(
                battery.raw_soc, 
                battery.filtered_soc, 
                SOC_FILTER_ALPHA
            );
            break;
    }
    
    // 计算电量变化率
    battery.soc_derivative = battery.filtered_soc - battery.displayed_soc;
    
    // 平滑显示电量变化（防止跳变）
    if (fabsf(battery.soc_derivative) > 1.0f) {
        battery.displayed_soc += (battery.soc_derivative > 0 ? 1.0f : -1.0f);
    } else {
        battery.displayed_soc = battery.filtered_soc;
    }
}

/**
 * @brief 重启安全的电池管理更新
 */
void battery_manager_update_restart_safe(void) {
    if (!enhanced_battery.battery.is_initialized) {
        return;
    }
    
    uint32_t current_time = get_system_time();
    
    // 处理重启恢复期
    handle_restart_recovery();
    
    // 重启恢复期内降低更新频率
    if (enhanced_battery.restart.is_restart_recovery) {
        if (current_time - enhanced_battery.battery.last_update_time < 500) {
            return; // 恢复期内每500ms更新一次
        }
    } else {
        if (current_time - enhanced_battery.battery.last_update_time < 100) {
            return; // 正常每100ms更新一次
        }
    }
    
    // 保存上一次状态
    enhanced_battery.battery.prev_state = enhanced_battery.battery.state;
    
    // 读取电压（重启安全版本）
    float new_voltage = read_voltage_restart_safe();
    new_voltage += enhanced_battery.battery.voltage_offset;
    
    // 电压滤波（重启恢复期使用更强的滤波）
    float alpha = enhanced_battery.restart.is_restart_recovery ? 
                  VOLTAGE_FILTER_ALPHA * 0.5f : VOLTAGE_FILTER_ALPHA;
    
    enhanced_battery.battery.raw_voltage = new_voltage;
    enhanced_battery.battery.filtered_voltage = low_pass_filter(
        new_voltage, 
        enhanced_battery.battery.filtered_voltage, 
        alpha
    );
    
    // 处理充电状态（重启安全版本）
    handle_charge_state_restart_safe();
    
    // 处理电量逻辑
    handle_soc_logic();
    
    // 保存关键状态
    save_critical_states();
    
    // 更新时间和状态
    enhanced_battery.battery.last_update_time = current_time;
}
/**
 * @brief 获取电池电压
 * @return 滤波后的电池电压(V)
 */
float battery_get_voltage(void) {
    return battery.filtered_voltage;
}
// ==================== 电源故障检测 ====================

/**
 * @brief 检测电源故障并采取措施
 */
void check_power_fault(void) {
    float voltage = battery_get_voltage();
    
    // 电压过低保护
    if (voltage < BATTERY_EMPTY_VOLTAGE + 0.1f) {
        // 进入低功耗模式
      //  enter_low_power_mode();
    }
    
    // 电压过高保护
    if (voltage > BATTERY_FULL_VOLTAGE + 0.1f) {
        // 断开充电
        set_charge_control(false);
    }
    
    // 重启频率过高检测
    static uint32_t last_check_time = 0;
    uint32_t current_time = get_system_time();
    
    if (current_time - last_check_time > 60000) { // 每分钟检查一次
        if (enhanced_battery.restart.restart_count > RESTART_MAX_COUNT) {
            // 重启过于频繁，进入安全模式
//            enter_safe_mode();
        }
        last_check_time = current_time;
    }
}
#endif 





uint8_t battery_updata_flag_get(void)
{
	return battery_updata_flag;
}
static void battery_updata_flag_set(void)
{
	battery_updata_flag = 1;
}
void battery_updata_flag_clear(void)
{
	battery_updata_flag = 0;
}
/*
* 充电状态获取接口
*/
CHARGE_STATUS_T Get_ChargeIO_Status(void)
{
	uint8_t charge_flag = GPIO_STATUS_LOW;
	//log_debug("GPIO_READ = %#x \r\n",drv_gpio_read(OM_GPIO0,GPIO_MASK(PAD_CHAGE_CHECK)));
	if(drv_gpio_read(OM_GPIO0,GPIO_MASK(PAD_CHAGE_CHECK)))
	{
		charge_flag = GPIO_STATUS_HIGH;
	}
	
	switch(charge_flag)
	{
		case GPIO_STATUS_LOW:
			return CHARGE_STATUS_NO_CHARGE;
			break;
		case GPIO_STATUS_HIGH:
			return CHARGE_STATUS_CHARGING;
			break;
		default:
			break;
	}
	return CHARGE_STATUS_INVALID;
//	log_debug("g_PMBatStatus.charge_status == %d\r\n",g_PMBatStatus.charge_status);
}

//中断方式读取电池电压
void pm_vbattery_get_int(void)
{
    drv_gpadc_read_int(config.channel_p, &adc_read_buffer[0], ADC_COLLECT_NUM);
}
//获取ADC采集原始值
uint32_t get_gpadc_read_data(void)
{
	return adc_average;
}
static void gpadc_read_cb(void *om_gpadc, drv_event_t event, void *read_buf, void *read_cnt)
{
	
    if (event == DRV_EVENT_COMMON_READ_COMPLETED) {
		
		uint32_t adc_num = (uint32_t)read_cnt;   // 这里就是10
		uint16_t *pData =(uint16_t *)read_buf;
		for(int i=0;i<adc_num;i++)
		{
			 adc_sum += pData[i];
		} 
		adc_average = adc_sum / adc_num; // 求平均
		adc_sum = 0;
		battery_updata_flag_set();
    }
}
void pm_gpadc_int(void)
{
    config.channel_p = GPADC_CH_P_GPIO14;
    config.channel_n = GPADC_CH_N_AVSS;
    config.mode = GPADC_MODE_SINGLE;
    config.gain = GPADC_GAIN_1_3_INTERNAL_REF;
    config.sum_num = GPADC_SUM_NUM_256;
    config.sampling_cycles = GPADC_SAMPLING_CYCLES_256;
    drv_gpadc_init(&config);
    drv_gpadc_register_isr_callback(gpadc_read_cb);

}
//adc block read vlue 
uint32_t r_PowerOn_BatteryLevel(void)	
{
	uint32_t adc_sum_tmp = 0;
	uint32_t adc_average_tmp = 0;
	drv_gpadc_read(config.channel_p, &adc_read_buffer[0], ADC_COLLECT_NUM);
	adc_sum=0;
	for(int i=0;i<ADC_COLLECT_NUM;i++)
	{
		 adc_sum_tmp += adc_read_buffer[i];
	} 
	adc_average_tmp = adc_sum_tmp / ADC_COLLECT_NUM; // 求平均
	return adc_average_tmp;
}

