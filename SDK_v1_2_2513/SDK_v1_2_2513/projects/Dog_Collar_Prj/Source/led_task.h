#ifndef _LED_TASK_H
#define _LED_TASK_H

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
#include "obc.h"
#include "cmsis_os2.h"
#include "timers.h"
#include "m_i2c.h"
#include "om_list.h"

#define LED_TASK_PRIORITY       (osPriorityNormal)
#define LED_TASK_STACK_SIZE     (1024U * 2)
#define CUSTOM_STACK_CONTROL    0

// 呼吸灯参数
#define BREATH_PERIOD_TOTAL_MS   2000 	// 呼吸灯周期
#define BREATH_UNIT_MS           25		// 每个呼吸灯周期内一个小单元的时间间隔
#define BREATH_UNIT_COUNT        (BREATH_PERIOD_TOTAL_MS / BREATH_UNIT_MS)
#define BREATH_STEP_INTERVAL     1		// 刷新电平间隔
#define MAX_RUNNING_LENGTH       8      // 最大跑马灯序列长度
#define FIXED_TIMER_BASE         50		// 固定时间基准(ms)

#define RED_START_OVER_TIME (5000)
#if CUSTOM_STACK_CONTROL
#define STACK_MAX_DEPTH    8   // 栈最大深度
#define STACK_ITEM_MAX_LEN 64  // 单个栈元素最大字节数
#endif

// CH423位操作宏
#define CH423_SET_BIT(mask)  do { \
    uint8_t cmd_mask = (mask) & 0xFF; \
    g_ch423_sys_cmd |= cmd_mask; \
} while(0)

#define CH423_CLR_BIT(mask)  do { \
    uint8_t cmd_mask = (mask) & 0xFF; \
    g_ch423_sys_cmd &= ~cmd_mask; \
} while(0)

/* CH423配置位定义 */
typedef enum {
	CH423_BIT_SLEEP   = (1U << 7U),
	CH423_BIT_INTENSH = (1U << 6U),
	CH423_BIT_INTENSL = (1U << 5U),
	CH423_BIT_OD_EN	  = (1U << 4U),
	CH423_BIT_X_INT   = (1U << 3U),
	CH423_BIT_DEC_H	  = (1U << 2U),
	CH423_BIT_DEC_L   = (1U << 1U),
	CH423_BIT_IO_OE   = (1U << 0U),
}ch423_reg_cfg_t;

/* LED掩码定义 */
typedef enum {
	// red
	LED_RED_0   = (1U << 0U),
	LED_RED_1 	= (1U << 3U),
	LED_RED_2 	= (1U << 5U),
	LED_RED_3 	= (1U << 6U),
	// white
	LED_WHITE_0 = (1U << 1U),
	LED_WHITE_1 = (1U << 2U),
	LED_WHITE_2 = (1U << 4U),
	LED_WHITE_3 = (1U << 7U),
}led_color_id_t;

typedef enum {
	_RED = 0x11,
	_WHITE,
}led_color_t;

/* LED状态 */
// typedef enum {
//     LED_STATE_NONE = 0,         // 无状态
//     LED_STATE_BLINK,            // 闪烁
//     LED_STATE_BREATH,           // 呼吸
// 	LED_STATE_RUNNING,          // 跑马灯
//     RESERVED,
// } led_state_t;

#if CUSTOM_STACK_CONTROL
typedef struct {
    uint8_t  buffer[STACK_MAX_DEPTH][STACK_ITEM_MAX_LEN]; // 栈存储缓冲区
    uint16_t item_len[STACK_MAX_DEPTH];                   // 每个元素的实际长度
    int8_t   top;                                         // 栈顶index（-1为空）
} generic_stack_t;
#endif

/* 呼吸参数结构体 */
typedef struct {
    uint32_t breath_period_total_ms; // 
    uint32_t period;           // 周期计数器
    uint32_t count_h;          // 高电平计数阈值
    bool is_breathing_down;    // 呼吸方向标识
    uint32_t interval;         // 呼吸间隔（控制快慢）
} breath_state_t;


/* LED参数配置结构体 */
typedef struct {
    // 状态位
    uint8_t is_forever : 1;             // 常亮使能
    uint8_t is_blink : 1;               // 闪烁使能
    uint8_t is_breath : 1;              // 呼吸使能
    uint8_t is_running : 1;             // 跑马灯使能
    
    uint32_t count;                     // 全局计数器（用于多效果时间同步）
    
    uint8_t forever_mask;               // 常亮掩码
    uint32_t turn_on_time;              // led常亮模式定时时长，0表示不会灭灯

    breath_state_t custom_breath;       // 自定义呼吸参数

    bool blink_on;                      // 用于闪烁的翻转
    uint8_t blink_mask;                 // 闪烁掩码
    uint32_t blink_interval;            // 闪烁间隔
    uint8_t breath_mask;                // 呼吸掩码

    uint8_t running_buffer[MAX_RUNNING_LENGTH];	// 跑马灯序列
    uint8_t current_led_idx;            // 跑马灯当前索引
    uint8_t running_count;  			// 跑马灯序列长度
    uint32_t running_interval;          // 跑马灯间隔
} led_single_params_t;

typedef struct {
    osTimerId_t state_timer;            // 主定时器
	uint32_t state_interval_ms;         // 主定时器周期
    bool is_red_run;                 // 是否有红灯在运行
    bool is_white_run;               // 是否有白灯在运行
    led_single_params_t red;            // 红灯参数
    led_single_params_t white;          // 白灯参数
} led_parrms_t;

// 红色LED工作状态枚举
typedef enum {
    LED_STATE_PAIRING = 0,      // 待配对状态
    R_LED_STATE_IDLE,         // 空闲状态  
    R_LED_STATE_FINDING_PET,  // 寻宠状态
    R_LED_STATE_FINDING_DEVICE, // 寻找设备状态
	R_LED_STATE_LOW_POWER,			// 低电量显示1颗闪烁
} LedState;
// 白色LED电池状态枚举
typedef enum {
    LED_STATE_LOW_BATTERY = 0,   	// 低电量状态
    W_LED_STATE_IDLE,          		// 空闲状态
    W_LED_STATE_FULLY_CHARGED, 		// 充满状态
    W_LED_STATE_CHARGING,      		// 充电中状态电量百分比
	W_LED_STATE_CHARGING_ALL,     	// 充电全部闪烁
	W_LED_STATE_LOW_POWER,			// 低电量显示1颗闪烁
	W_LED_STATE_RUNING,				// 设备在运行中
	W_LED_STATE_FINDING,			// 寻宠模式
} WhiteLedState;

void ch423_init(void);
void ch423_set_output(uint8_t value);

// LED功能函数
void led_init(void);
void led_turn_on_all(void);
void led_turn_off_all(void);
void led_turn_on_specific(led_color_id_t led_mask);
void led_turn_off_specific(led_color_id_t led_mask);

/**
 * @brief : led常亮控制
 * 
 * @param  : num - 点亮的数量
 * @param  : color - led颜色
 * @param  : on - 使能
 * @param  : time_count - 超时关闭时间，单位ms；传入0时led常亮
 */
void led_control(uint8_t num, led_color_t color, bool on, uint32_t time_count);

/**
 * @brief : led闪烁控制
 * 
 * @param  : num - 点亮的数量
 * @param  : color - led颜色
 * @param  : on - 使能
 */
void led_blink_control(uint8_t num, led_color_t color, bool on);

/**
 * @brief : led呼吸控制
 * 
 * @param  : num - 点亮的数量
 * @param  : color - led颜色
 * @param  : on - 使能
 * @param  : speed - 呼吸速率；0为缓慢呼吸，1为快速呼吸
 */
void led_breath_control(uint8_t num, led_color_t color, bool on, bool speed);

/**
 * @brief : led跑马控制
 * 
 * @param  : color - led颜色
 * @param  : on - 使能
 */
void led_running_control(led_color_t color, bool on);


// 任务相关函数
void led_force_stop_all(void);
// void led_stop_current_state(void);
osThreadId_t vStartLedTask(void);

void white_led_start(WhiteLedState mode,uint8_t vlue);
void white_led_status_reset(void);
//红色LED
void red_led_status_reset(void);
void red_led_start(LedState mode);

#endif