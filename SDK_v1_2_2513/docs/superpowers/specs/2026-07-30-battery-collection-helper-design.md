# 电池采集与百分比更新函数设计

## 目标

将 `pm_Task.c` 中重复的原始 ADC 转换、电压滤波、SOC 更新和百分比持久化逻辑封装为一个文件内通用函数。调用方传入原始 ADC 值，函数返回最终电量百分比。

## 接口

```c
static uint8_t PM_BatteryCollectAndUpdate(uint32_t adc_value);
```

- 输入：电池原始 ADC 值。
- 输出：经过转换、滤波和 SOC 策略处理后的百分比，范围为 0～100。
- 可见范围：仅限 `pm_Task.c`，不增加公共头文件接口。

## 数据流

函数按以下固定顺序执行：

1. 使用 `adc_to_mv_generic()` 将原始 ADC 转换为毫伏。
2. 使用当前 `PM_GetChargeStatus()` 调用 `BatteryVoltage_AntiJump()` 完成电压滤波。
3. 使用同一次读取到的充电状态调用 `Battery_SocUpdate()` 计算最终百分比。
4. 更新 `g_PMBatStatus.BAT_Capacity`。
5. 调用 `reset_rom_battery_percent()` 写入掉电保持区。
6. 以统一的 `PM_BatteryCollectAndUpdate` 标签输出原始 ADC、充电状态、百分比和滤波电压日志。
7. 返回 `g_PMBatStatus.BAT_Capacity`。

为避免一次处理过程中充电状态发生变化，函数只读取一次 `PM_GetChargeStatus()`，并将该状态同时用于滤波和 SOC 更新。

## 调用点

- `PM_Charge_Battery_Init()` 传入 `r_PowerOn_BatteryLevel()`。
- `battery_voltage_state_update()` 传入其参数 `adv_vlue`。
- 原调用点中的重复日志移入通用函数，并统一日志标签；日志字段和值保持不变。

## 范围与约束

- 不修改 `Battery_SocUpdate()` 和 `Battery_FullDetect()` 的算法。
- 不增加 ADC 合法性判断，因为现有代码没有定义 ADC 错误值及处理策略。
- 不改变初始化时读取、清除掉电保持百分比的顺序。
- 删除封装后不再需要的重复局部变量和处理语句。

## 验证

- 静态检查两个调用点都只执行一次 SOC 更新和一次掉电保持写入。
- 编译 `Dog_Collar_Prj`，确认没有类型、声明顺序或未使用变量警告。
- 对比修改前后的初始化及周期采集数据流，确认输出值和日志字段保持一致。
