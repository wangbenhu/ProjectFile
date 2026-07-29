#include "led_task.h"

#define BATTERY_ONE_VALUE_MIN 	    0	//%
#define BATTERY_ONE_VALUE_MAX 	    25	//%
#define BATTERY_TWO_VALUE_MAX 	    50	//%
#define BATTERY_THREE_VALUE_MAX 	75	//%
#define BATTERY_FOUR_VALUE_MAX 	    100	//%

#define LED_LEVEL_HIGH				(1)
#define LED_LEVEL_LOW				(0)

#define LED_MASK_VALID_LEVEL		LED_LEVEL_LOW //
static uint8_t g_ch423_sys_cmd = 0x00;
static uint8_t g_current_led_output = 0x00;

static uint8_t led_task_cmd_status = TASK_CMD_END; 	

static uint8_t breath_direct_musk = 0;
static LedState red_led_control_status = R_LED_STATE_IDLE;
static WhiteLedState white_led_control_status = W_LED_STATE_IDLE;

static LedState red_led_control_mutual = R_LED_STATE_IDLE;
static WhiteLedState white_led_control_mutual = W_LED_STATE_IDLE;


static osSemaphoreId_t g_ledLowPowerSemaphore = NULL;//低功耗是否进入的标志
#if CUSTOM_STACK_CONTROL
static generic_stack_t g_common_stack = {.top = -1};
#endif

static led_parrms_t led_params = {
    .state_timer = NULL,
	.state_interval_ms = 0,
	.is_red_run = false,
	.is_white_run = false,

    .red = {
		// 红灯参数
        .is_forever = false,
        .is_blink = false,
        .is_breath = false, 
        .is_running = false,
        .count = 0,
        .turn_on_time = 0,
        .custom_breath = {
            .breath_period_total_ms = BREATH_PERIOD_TOTAL_MS,
            .period = 0,
            .count_h = 0,
            .is_breathing_down = false,
            .interval = BREATH_UNIT_MS
        },
		.blink_on = false,
        .blink_interval = 0,
        .running_buffer = {0},
        .running_count = 0,
        .running_interval = 0,
    },
    .white = {
        // 白灯参数
        .is_forever = false,
        .is_blink = false,
        .is_breath = false, 
        .is_running = false,
        .count = 0,
        .turn_on_time = 0,
        .custom_breath = {
            .breath_period_total_ms = BREATH_PERIOD_TOTAL_MS,
            .period = 0,
            .count_h = 0,
            .is_breathing_down = false,
            .interval = BREATH_UNIT_MS
        },
		.blink_on = false,
        .blink_interval = 0,
        .running_buffer = {0},
        .running_count = 0,
        .running_interval = 0,
    }
};


#if CUSTOM_STACK_CONTROL
int8_t stack_push(generic_stack_t *stack, const void *data, uint16_t data_len) 
{
    // 参数校验
    if (stack == NULL || data == NULL) return -2;
    if (data_len > STACK_ITEM_MAX_LEN) {
        printf("stack item too long (max: %d)\n", STACK_ITEM_MAX_LEN);
        return -2;
    }
    // 检查栈是否已满
    if (stack->top >= STACK_MAX_DEPTH - 1) {
        printf("stack is full (max depth: %d)\n", STACK_MAX_DEPTH);
        return -1;
    }

    // 栈顶指针上移，拷贝数据到栈缓冲区，记录数据长度
    stack->top++;
    memcpy(stack->buffer[stack->top], data, data_len);
    stack->item_len[stack->top] = data_len;

    return 0;
}

int16_t stack_pop(generic_stack_t *stack, void *out_buf, uint16_t buf_len) 
{
    // 参数校验
    if (stack == NULL || out_buf == NULL) return -2;
    // 检查栈是否为空
    if (stack->top < 0) {
        printf("stack is empty\n");
        return -1;
    }

    // 获取栈顶元素长度，检查缓冲区是否足够
    uint16_t data_len = stack->item_len[stack->top];
    if (buf_len < data_len) {
        printf("out buffer too small (need: %d, got: %d)\n", data_len, buf_len);
        return -3;
    }

    // 拷贝栈顶数据到输出缓冲区，栈顶指针下移
    memcpy(out_buf, stack->buffer[stack->top], data_len);
    stack->top--;

    return data_len;
}

int8_t stack_delete_by_index(generic_stack_t *stack, int8_t index) 
{
    // 参数校验
    if (stack == NULL) return -1;
    // 检查索引是否合法
    if (index < 0 || index > stack->top) {
        printf("stack index out of range (valid: 0~%d, input: %d)\n", stack->top, index);
        return -2;
    }

    // 从删除位置开始，后续元素整体前移一位
    for (int8_t i = index; i < stack->top; i++) {
        memcpy(stack->buffer[i], stack->buffer[i+1], STACK_ITEM_MAX_LEN);
        stack->item_len[i] = stack->item_len[i+1];
    }

    // 清空最后一位的残留数据，栈顶指针下移
    memset(stack->buffer[stack->top], 0, STACK_ITEM_MAX_LEN);
    stack->item_len[stack->top] = 0;
    stack->top--;

    printf("delete stack item at index %d success\n", index);
    return 0;
}

int8_t stack_delete_by_data(generic_stack_t *stack, const void *data, uint16_t data_len) 
{
    // 参数校验
    if (stack == NULL || data == NULL || data_len > STACK_ITEM_MAX_LEN) return -1;
    if (stack->top < 0) {
        printf("stack is empty, no data to delete\n");
        return -2;
    }

    // 遍历栈，找到第一个匹配的元素
    for (int8_t i = 0; i <= stack->top; i++) {
        // 先校验长度是否一致，再校验内容
        if (stack->item_len[i] == data_len && 
            memcmp(stack->buffer[i], data, data_len) == 0) {
            // 调用按索引删除函数
            return stack_delete_by_index(stack, i);
        }
    }

    printf("no matched data found in stack\n");
    return -2;
}

void stack_clear(generic_stack_t *stack) 
{
    if (stack == NULL) return;
    stack->top = -1;
    // 清空缓冲区
    memset(stack->buffer, 0, sizeof(stack->buffer));
    memset(stack->item_len, 0, sizeof(stack->item_len));
}

int8_t stack_get_depth(generic_stack_t *stack) 
{
    if (stack == NULL) return -1;
    return stack->top + 1; // top从-1开始，深度=top+1
}
#endif

// CH423初始化
void ch423_init(void) 
{
    I2C_Write(OM_I2C0, 0x48>>1, g_ch423_sys_cmd, NULL, 0);
}

// 设置CH423输出（直接控制硬件）
void ch423_set_output(uint8_t value) 
{
    uint8_t cmd = (value & 0xFF);
    I2C_Write(OM_I2C0, 0x60>>1, cmd, NULL, 0);
}

// LED全亮
void led_turn_on_all(void) 
{
#if (LED_MASK_VALID_LEVEL)
	 g_current_led_output = 0xFF;
#else
	 g_current_led_output = 0x00;
#endif 
	ch423_set_output(g_current_led_output);
}

// LED全灭
void led_turn_off_all(void) 
{
#if (LED_MASK_VALID_LEVEL)
    g_current_led_output = 0x00;
#else
	g_current_led_output = 0xFF;
#endif
    ch423_set_output(g_current_led_output);
}

// 点亮指定LED
void led_turn_off_specific(led_color_id_t led_mask) 
{
#if (LED_MASK_VALID_LEVEL)
    g_current_led_output &= ~led_mask;
#else
	 g_current_led_output |= led_mask;
#endif
    ch423_set_output(g_current_led_output);
}

// 熄灭指定LED
void led_turn_on_specific(led_color_id_t led_mask) 
{
#if (LED_MASK_VALID_LEVEL)
     g_current_led_output |= led_mask;
#else
	g_current_led_output &= ~led_mask;
#endif 
    ch423_set_output(g_current_led_output);
}

// 停止当前状态
void led_stop_current_state(void) 
{
    if (led_params.state_timer != NULL)
        osTimerStop(led_params.state_timer);

    led_turn_off_all();

    // 重置红灯参数
    memset(&led_params.red, 0, sizeof(led_single_params_t));
    // 重置白灯参数
    memset(&led_params.white, 0, sizeof(led_single_params_t));

	led_params.state_interval_ms = 0;
	led_params.is_red_run = false;
	led_params.is_white_run = false;
}

// 更新定时器基准
static void update_timer_base()
{
    uint32_t new_interval = 0; // 默认呼吸灯周期（最小）
    bool has_active_effect = false;

    // 检查是否有活跃的灯效
    bool red_active = led_params.red.is_forever || led_params.red.is_blink || 
                      led_params.red.is_breath || led_params.red.is_running;

    bool white_active = led_params.white.is_forever || led_params.white.is_blink || 
                        led_params.white.is_breath || led_params.white.is_running;

    has_active_effect = red_active || white_active;

	if(red_active)
		led_params.is_red_run = true;
	else
		led_params.is_red_run = false;
	if(white_active)
		led_params.is_white_run = true;
	else
		led_params.is_white_run = false;
		
    // 无活跃效果时，停止定时器
    if (!has_active_effect) {
        led_stop_current_state();
        return;
    }

    // 按优先级计算各灯效的有效时间间隔
    uint32_t valid_intervals[8] = {0}; 
    uint8_t interval_count = 0;

    // 呼吸灯
    if (led_params.red.is_breath || led_params.white.is_breath) {
        valid_intervals[interval_count++] = BREATH_STEP_INTERVAL;
    }

    // 跑马灯
    if (led_params.red.is_running && led_params.red.running_interval > 0) {
        valid_intervals[interval_count++] = led_params.red.running_interval;
    }
    if (led_params.white.is_running && led_params.white.running_interval > 0) {
        valid_intervals[interval_count++] = led_params.white.running_interval;
    }

    // 闪烁灯
    if (led_params.red.is_blink && led_params.red.blink_interval > 0) {
        valid_intervals[interval_count++] = led_params.red.blink_interval;
    }
    if (led_params.white.is_blink && led_params.white.blink_interval > 0) {
        valid_intervals[interval_count++] = led_params.white.blink_interval;
    }

    // 常亮灯
    if (led_params.red.is_forever && led_params.red.turn_on_time > 0) {
        valid_intervals[interval_count++] = led_params.red.turn_on_time;
    }
    if (led_params.white.is_forever && led_params.white.turn_on_time > 0) {
        valid_intervals[interval_count++] = led_params.white.turn_on_time;
    }

    // 选择最小的有效时间间隔
    if (interval_count > 0) {
        new_interval = valid_intervals[0];
        for (uint8_t i = 1; i < interval_count; i++) {
            if (valid_intervals[i] > 0 && valid_intervals[i] < new_interval) {
                new_interval = valid_intervals[i];
            }
        }
    }

    // 限制最小周期
    if (new_interval == 0) {
        // new_interval = BREATH_STEP_INTERVAL;
		return;
    }

    // 更新定时器时间基准
    if (led_params.state_interval_ms != new_interval) {
        led_params.state_interval_ms = new_interval;
    
        if (led_params.state_timer != NULL) {
            osTimerStop(led_params.state_timer);
			// log_debug("new_interval:%d\n", new_interval);
            osTimerStart(led_params.state_timer, pdMS_TO_TICKS(new_interval));
        }
    }
	
}

void led_control(uint8_t num, led_color_t color, bool on, uint32_t time_count)
{
	uint8_t led_mask;
	
	if(color == _RED) {
		switch(num)
		{
			case 1: led_mask = LED_RED_0; break;
			case 2: led_mask = LED_RED_0 | LED_RED_1; break;
			case 3: led_mask = LED_RED_0 | LED_RED_1 | LED_RED_2; break;
			case 4: led_mask = LED_RED_0 | LED_RED_1 | LED_RED_2 | LED_RED_3; break;
			default: return;
		}

		led_params.red.forever_mask = led_mask;
		led_params.red.is_forever = on;
		led_params.red.turn_on_time = time_count;
	}else if(color == _WHITE) {
		switch(num)
		{
			case 1: led_mask = LED_WHITE_0;break;
			case 2: led_mask = LED_WHITE_0 | LED_WHITE_1;break;
			case 3: led_mask = LED_WHITE_0 | LED_WHITE_1 | LED_WHITE_2;break;
			case 4: led_mask = LED_WHITE_0 | LED_WHITE_1 | LED_WHITE_2 | LED_WHITE_3;break;
			default: break;
		}
		
		led_params.white.forever_mask = led_mask;
		led_params.white.is_forever = on;
		led_params.white.turn_on_time = time_count;
	}
    
    if(on)
        led_turn_on_specific(led_mask);
    else
        led_turn_off_specific(led_mask);
    
    update_timer_base();
}
void led_blink_control(uint8_t num, led_color_t color, bool on)
{
    uint8_t led_mask;
	
	if(color == _RED) {
		switch(num)
		{
			case 1: led_mask = LED_RED_0; break;
			case 2: led_mask = LED_RED_0 | LED_RED_1; break;
			case 3: led_mask = LED_RED_0 | LED_RED_1 | LED_RED_2; break;
			case 4: led_mask = LED_RED_0 | LED_RED_1 | LED_RED_2 | LED_RED_3; break;
			default: return;
		}
		
		led_params.red.blink_mask = led_mask;
		led_params.red.blink_interval = 5000;
		led_params.red.is_blink = on;
	}else if(color == _WHITE) {
		switch(num)
		{
			case 1: led_mask = LED_WHITE_0;break;
			case 2: led_mask = LED_WHITE_0 | LED_WHITE_1;break;
			case 3: led_mask = LED_WHITE_0 | LED_WHITE_1 | LED_WHITE_2;break;
			case 4: led_mask = LED_WHITE_0 | LED_WHITE_1 | LED_WHITE_2 | LED_WHITE_3;break;
			default: break;
		}
		
		led_params.white.blink_mask = led_mask;
		led_params.white.blink_interval = 1000;
		led_params.white.is_blink = on;
	}
	
    update_timer_base();
}

void led_breath_control(uint8_t num, led_color_t color, bool on, bool speed)
{
    uint8_t led_mask;

	breath_state_t breath_params = {
        .interval = 25,        
        .period = 0,           
        .count_h = 0,         
        .is_breathing_down = false,
    };

	if(speed) {
		breath_params.breath_period_total_ms = 2000; // 快速呼吸灯
	} else {
		breath_params.breath_period_total_ms = 5000; // 缓慢呼吸灯
	}

	if(color == _RED) {
		switch(num)
		{
			case 1: led_mask = LED_RED_0;break;
			case 2: led_mask = LED_RED_0 | LED_RED_1;break;
			case 3: led_mask = LED_RED_0 | LED_RED_1 | LED_RED_2;break;
			case 4: led_mask = LED_RED_0 | LED_RED_1 | LED_RED_2 | LED_RED_3;break;
			default: break;
		}
		
		led_params.red.breath_mask = led_mask;
		led_params.red.is_breath = on;
		// memcpy(&led_params.red.custom_breath, &breath_params, sizeof(breath_state_t));
		if(led_params.red.is_breath)
			memcpy(&led_params.red.custom_breath, &breath_params, sizeof(breath_state_t));
		// else
		// 	memset(&led_params.red.custom_breath, 0, sizeof(breath_state_t));

	}else if(color == _WHITE) {
		switch(num)
		{
			case 1: led_mask = LED_WHITE_0;break;
			case 2: led_mask = LED_WHITE_0 | LED_WHITE_1;break;
			case 3: led_mask = LED_WHITE_0 | LED_WHITE_1 | LED_WHITE_2;break;
			case 4: led_mask = LED_WHITE_0 | LED_WHITE_1 | LED_WHITE_2 | LED_WHITE_3;break;
			default: break;
		}

		led_params.white.breath_mask = led_mask;
		led_params.white.is_breath = on;
		// memcpy(&led_params.white.custom_breath, &breath_params, sizeof(breath_state_t));
		if(led_params.white.is_breath)
			memcpy(&led_params.white.custom_breath, &breath_params, sizeof(breath_state_t));
	}

	update_timer_base();
}

void led_running_control(led_color_t color, bool on)
{
	if(color == _RED) {
		uint8_t running_sequence[4] = {
			LED_RED_0,
			LED_RED_1,
			LED_RED_2,
			LED_RED_3
		};
	
		led_params.red.is_running = on;
		memcpy(led_params.red.running_buffer, running_sequence, 4 * sizeof(uint8_t)); 
		led_params.red.running_count = 4;
		led_params.red.current_led_idx = 0;
		led_params.red.running_interval = 250;
	}else if(color ==_WHITE) {
		uint8_t running_sequence[4] = {
			LED_WHITE_0,
			LED_WHITE_1,
			LED_WHITE_2,
			LED_WHITE_3
		};

		led_params.white.is_running = on;
		memcpy(led_params.white.running_buffer, running_sequence, 4 * sizeof(uint8_t)); 
		led_params.white.running_count = 4;
		led_params.white.current_led_idx = 0;
		led_params.white.running_interval = 250;
	}
    update_timer_base();
}

static void process_led_effects(led_single_params_t *params, uint8_t *final_output)
{
    // 保存当前输出用于叠加处理
    uint8_t temp_output = *final_output;

    // 常亮效果
    if (params->is_forever) {  
		#if (LED_MASK_VALID_LEVEL)
        temp_output |= params->forever_mask;
		#else
		temp_output &= ~params->forever_mask;
		#endif


        // 定时关闭
        if (params->turn_on_time > 0) {
            uint32_t turn_off_div = params->turn_on_time / led_params.state_interval_ms;
            if (turn_off_div > 0 && (params->count % turn_off_div) == 0) {
                params->is_forever = false;

				#if (LED_MASK_VALID_LEVEL)
				temp_output &= ~params->forever_mask;
				#else
				temp_output |= params->forever_mask;
				#endif
            }
        }
    }

    // 呼吸灯效果
    if (params->is_breath) {
        breath_state_t *b = &params->custom_breath;
        b->period++;
        uint32_t current_unit = (b->period / b->interval) % (b->breath_period_total_ms / BREATH_UNIT_MS);

        if (!b->is_breathing_down) {
            b->count_h = (current_unit * b->interval) / ((b->breath_period_total_ms / BREATH_UNIT_MS) / 2);
            if (current_unit >= (b->breath_period_total_ms / BREATH_UNIT_MS) / 2 - 1) {
                b->is_breathing_down = true;
            }
        } else {
            b->count_h = b->interval - ((current_unit - (b->breath_period_total_ms / BREATH_UNIT_MS) / 2) * b->interval) / ((b->breath_period_total_ms / BREATH_UNIT_MS) / 2);
            if (current_unit >= (b->breath_period_total_ms / BREATH_UNIT_MS) - 1) {
                b->period = 0;
                b->is_breathing_down = false;
            }
        }

        // 应用呼吸灯效果
        if ((b->period % b->interval) < b->count_h) {
			#if (LED_MASK_VALID_LEVEL)
			temp_output |= params->breath_mask;
			#else
			temp_output &= ~params->breath_mask;
			#endif
        }
    }

    // 闪烁效果
    if (params->is_blink) {
        uint32_t blink_div = params->blink_interval / led_params.state_interval_ms;
        if (blink_div > 0 && (params->count % blink_div) == 0) {
            params->blink_on = !params->blink_on;
        }
        if (params->blink_on) {
			#if (LED_MASK_VALID_LEVEL)
			temp_output |= params->blink_mask;
			#else
            temp_output &= ~params->blink_mask;
			#endif
        }
    }

    // 跑马灯效果
    if (params->is_running && params->running_count > 0) {
        uint32_t running_div = params->running_interval / led_params.state_interval_ms;
        if (running_div > 0 && (params->count % running_div) == 0) {
            // 下一个LED
            params->current_led_idx = (params->current_led_idx + 1) % params->running_count;
        }
		#if (LED_MASK_VALID_LEVEL)
		temp_output |= params->running_buffer[params->current_led_idx];
		#else
		temp_output &= ~params->running_buffer[params->current_led_idx];
		#endif
    }

    *final_output = temp_output;
}

// 定时器回调函数（处理各种LED状态）
static void state_timer_callback(void *argument) 
{
#if (LED_MASK_VALID_LEVEL)
    uint8_t final_output = 0x00;
#else
    uint8_t final_output = 0xFF;
#endif

    // 处理红灯效果
	if(led_params.is_red_run) {
		led_params.red.count++;
    	process_led_effects(&led_params.red, &final_output);
	}
    // 处理白灯效果
	if(led_params.is_white_run) {
		led_params.white.count++;
    	process_led_effects(&led_params.white, &final_output);
	}

    g_current_led_output = final_output;
    ch423_set_output(g_current_led_output);
}

// LED初始化
void led_init(void) 
{
    I2C_Init(OM_I2C0);
	led_turn_off_all(); // 初始全灭 //低电平点亮时，需要先给寄存器写低电平，否则LED初始化完之后会一直亮
    CH423_SET_BIT(CH423_BIT_IO_OE); // 使能IO输出
    ch423_init();
	
	if (led_params.state_timer != NULL)
        osTimerStop(led_params.state_timer);
	
    // 创建状态定时器
    if (led_params.state_timer == NULL) {
        led_params.state_timer = osTimerNew(state_timer_callback, osTimerPeriodic, NULL, NULL);
        if (led_params.state_timer == NULL) {
            log_debug("LED state timer create failed!\r\n");
        }
    }
	//led_turn_on_all();
}

// 强制停止所有状态
void led_force_stop_all(void) 
{
	led_stop_current_state();
	red_led_control_status = R_LED_STATE_IDLE;
	white_led_control_status = W_LED_STATE_IDLE;
//    led_params.current_state = LED_STATE_NONE;
}

// 关闭4颗常亮的红灯
void led_red_full_off()
{
	led_control(4, _RED, false, 0);
}

// 关闭4颗常亮的白灯
void led_white_full_off()
{
	led_control(4, _WHITE, false, 0);
}

//亮4颗红灯
void pet_mode_red_full_on(void)
{
	// led_red_control(4,true,RED_START_OVER_TIME);
	led_control(4, _RED, true, 0);
}
void pet_mode_red_full_off(void)
{
	// led_red_control(4,false,RED_START_OVER_TIME);
	led_control(4, _RED, false, 0);
}
//满电全亮4 颗白灯
void led_battery_full(void)
{ 
	// led_white_control(4,true,0);
	led_control(4, _WHITE, true, 0);
}
//满电全亮4 颗白灯
void led_battery_full_stop(void)
{ 
	// led_white_control(4,true,0);
	led_control(4, _WHITE, false, 0);
}


//关闭四颗红色灯闪烁
void led_red_blink_full_off(void)
{
	//  led_red_blink_control(4,false);
	led_blink_control(4, _RED, false);
	//单独关闭4颗红色灯
}
//配对提示闪烁4颗红灯
void led_red_blink_full_on(void) {
	// led_red_blink_control(4,true);
	led_blink_control(4, _RED, true);
}

//关闭四颗红色灯闪烁
void low_power_red_blink_off(void)
{
	led_blink_control(1, _RED, false);
	//单独关闭4颗红色灯
}
//配对提示闪烁4颗红灯
void low_power_red_blink_on(void) {
	led_blink_control(1, _RED, true);
}

//关闭四颗红色灯闪烁
void low_power_white_blink_off(void)
{
	led_blink_control(1, _WHITE, false);
	//单独关闭4颗红色灯
}
//配对提示闪烁4颗红灯
void low_power_white_blink_on(void) {
	led_blink_control(1, _WHITE, true);
}
/*
* 20260310修改将充电电量百分比闪烁改为4颗同时闪烁
* 
*/

void led_breath_direct_all(void) {
	
	uint8_t mask=4;
	led_breath_control(mask, _WHITE, true, 0);
}
/*
* 根据电池电量百分比和充电状态设置呼吸灯
* @param value 电池电量百分比（0-100）
*/

void led_breath_direct(uint8_t value) {
	
	uint8_t mask=0;
	
	if(value > BATTERY_FOUR_VALUE_MAX)
		return;
	if((value>=BATTERY_ONE_VALUE_MIN)&& (value<=BATTERY_ONE_VALUE_MAX))
	{
		mask = 1;
	}
	else if((value>BATTERY_ONE_VALUE_MAX) && (value<=BATTERY_TWO_VALUE_MAX))
	{
		mask = 2;
	}
	else if((value>BATTERY_TWO_VALUE_MAX) && (value<=BATTERY_THREE_VALUE_MAX))
	{
		mask = 3 ;
	}
	else
	{
		mask = 4;
	}
	led_breath_control(mask, _WHITE, true, 1);
}
void led_breath_direct_stop(void)
{
	// led_white_breath_control(4,false,1);
	led_breath_control(4, _WHITE, false, 1);
}

//寻宠模式关闭
void find_pet_led_stop(void)
{
	// led_red_running_control(false);
	led_running_control(_RED, false);
}
//寻宠模式开启
void find_pet_led_start(void)
{
	// led_red_running_control(true);
	led_running_control(_RED, true);
}//寻宠模式关闭
void w_find_pet_led_stop(void)
{
	// led_red_running_control(false);
	led_running_control(_WHITE, false);
}
//寻宠模式开启
void w_find_pet_led_start(void)
{
	// led_red_running_control(true);
	led_running_control(_WHITE, true);
}
//低电量
void low_power_led_start(void)
{
	// led_white_breath_control(1,true,1);
	led_breath_control(1, _WHITE, true, 1);
}
void low_power_led_stop(void)
{
	// led_white_breath_control(1,false,1);
	led_breath_control(1, _WHITE, false, 1);
}

//低电量
void w_device_runing_led_start(void)
{
	led_breath_control(1, _WHITE, true, 0);
}
void w_device_runing_led_stop(void)
{
	led_breath_control(1, _WHITE, false, 0);
}
//白色LED
static uint8_t first_battery_vlue = 0;
void white_led_status_reset(void)
{
	if(white_led_control_status != W_LED_STATE_IDLE)
	{
		switch(white_led_control_status)
		{
			case LED_STATE_LOW_BATTERY:
				low_power_led_stop();//20251223取消M3模式下的低电量显示灯效，考虑功耗因素 平均2mA
				break;
			
			case W_LED_STATE_FULLY_CHARGED:
				led_white_full_off();//
				break;
			
			case W_LED_STATE_CHARGING:
				led_breath_direct_stop();//
				break;
			case W_LED_STATE_CHARGING_ALL:
				led_breath_direct_stop();//
				break;
			case W_LED_STATE_LOW_POWER:
				low_power_white_blink_off();//
				break;
			case W_LED_STATE_RUNING:
				w_device_runing_led_stop();
				break;
			case W_LED_STATE_FINDING:
				w_find_pet_led_stop();//寻宠跑马灯
				break;
			default:
				break;						
		}
		white_led_control_status = W_LED_STATE_IDLE;
	}
}

void white_led_start(WhiteLedState mode,uint8_t vlue)
{
	uint8_t battery_vlue = vlue;
	WhiteLedState mode_tmp = mode;
	if(white_led_control_status == mode)
	{
		if(mode == W_LED_STATE_CHARGING)
		{
			if(first_battery_vlue==vlue)
			{
				return ;
			}
			else
			{
				first_battery_vlue = vlue;
			}
		}
		else
		{
			return ;
		}
		
	}
	log_debug("white_led_start:%d,%d,%d\r\n",mode,vlue,mode_tmp);
	white_led_status_reset();
	if(mode_tmp != W_LED_STATE_IDLE)
	{
		switch(mode_tmp)
		{
			case LED_STATE_LOW_BATTERY:
				low_power_led_start();////20251223取消M3模式下的低电量显示灯效，考虑功耗因素 平均2mA		
				break;
			
			case W_LED_STATE_FULLY_CHARGED:
				led_battery_full();//
				break;
			
			case W_LED_STATE_CHARGING:
				led_breath_direct(battery_vlue);//
				break;
			case W_LED_STATE_CHARGING_ALL:
				led_breath_direct_all();//
				break;
			case W_LED_STATE_LOW_POWER:
				low_power_white_blink_on();//
				break;
			case W_LED_STATE_RUNING:
				w_device_runing_led_start();//
				break;
			case W_LED_STATE_FINDING:
				w_find_pet_led_start();//寻宠跑马灯
				break;
			
			default:
				mode_tmp = W_LED_STATE_IDLE;
				break;						
		}	
		white_led_control_status = mode_tmp;
		if(white_led_control_status!=W_LED_STATE_CHARGING)
		{
			first_battery_vlue=0;
		}
		
	}
}

//红色LED
void red_led_status_reset(void)
{
	if(red_led_control_status != R_LED_STATE_IDLE)
	{
		switch(red_led_control_status)
		{
			case LED_STATE_PAIRING:
				led_red_blink_full_off();//闪烁待配对
				break;
			
			case R_LED_STATE_FINDING_PET:
				find_pet_led_stop();//寻宠跑马灯
				break;
			
			case R_LED_STATE_FINDING_DEVICE:
				pet_mode_red_full_off();//找设备常量
				break;
			case R_LED_STATE_LOW_POWER:
				low_power_red_blink_off();
				break;
			default:
				break;						
		}
		red_led_control_status = R_LED_STATE_IDLE;
	}
}
void red_led_start(LedState mode)
{
	LedState mode_tmp = mode;
	
	if(red_led_control_status == mode)
	{
		return;
	}

	red_led_status_reset();	
	log_debug("red_led_start :%d,%d \r\n",red_led_control_status,mode);
	if(mode_tmp != R_LED_STATE_IDLE)
	{
		switch(mode_tmp)
		{
			case LED_STATE_PAIRING:
				led_red_blink_full_on();//闪烁待配对
				break;
			
			case R_LED_STATE_FINDING_PET:
				find_pet_led_start();//寻宠跑马灯
				break;
			
			case R_LED_STATE_FINDING_DEVICE:
				pet_mode_red_full_on();//找设备常量
				break;
			case R_LED_STATE_LOW_POWER:
				low_power_red_blink_on();
				break;
			default:
				mode_tmp = R_LED_STATE_IDLE;
				break;						
		}
		red_led_control_status = mode_tmp;	
	}
}
// LED任务处理函数
static void vLedTask(void *argument) 
{
    TaskInfo_t *my_task_info = GetTaskInfo(LED_TASK_ID);
    Message_t received_msg;

	if (g_ledLowPowerSemaphore == NULL) {
		g_ledLowPowerSemaphore = osSemaphoreNew(1, 0, NULL);
    } else {
        log_debug("vLedTask already exists, skip creation\r\n");
    }
	
    for (;;) {
		
		//log_debug("vLedTask\r\n");
		
		osSemaphoreRelease(g_ledLowPowerSemaphore);
		
        if (osOK == osMessageQueueGet(my_task_info->queue_handle, &received_msg, NULL, portMAX_DELAY)) {
			
            // 初始化LED
            if (received_msg.command == TASK_CMD_START) {
				if(led_task_cmd_status == TASK_CMD_START)//重入互斥
					continue ;
				led_task_cmd_status=TASK_CMD_START;
            }		
			if(received_msg.source_id == ENTRY_TASK_ID)
            { 
                if(received_msg.command == TASK_CMD_STOP) 
				{
					if(led_task_cmd_status == TASK_CMD_STOP)//重入互斥
						continue ;
					led_task_cmd_status=TASK_CMD_STOP;
					   // 强制停止所有
					led_force_stop_all();
					
					  // 尝试获取信号量，低功耗阻塞
					osSemaphoreAcquire(g_ledLowPowerSemaphore, osWaitForever);
					
				}
			}
		}
		  // 尝试获取信号量，低功耗阻塞
        osSemaphoreAcquire(g_ledLowPowerSemaphore, osWaitForever);
    }
}

// 启动LED任务
osThreadId_t vStartLedTask(void) 
{
    const osThreadAttr_t LedThreadAttr = {
        .name = "Led_Task",
        .stack_size = LED_TASK_STACK_SIZE,
        .priority = LED_TASK_PRIORITY,
    };
    return osThreadNew(vLedTask, NULL, &LedThreadAttr);
}
