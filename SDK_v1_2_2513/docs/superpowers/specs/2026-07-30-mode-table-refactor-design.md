# M1-M5 模式有序操作表等价重构设计

日期：2026-07-30

## 1. 背景

`projects/Dog_Collar_Prj/Source/entry_task.c` 当前通过
`ModeM1Handler()` 至 `ModeM5Handler()` 直接调用 BLE、CAT1、GNSS、PM、
Sensor、LED 和 Comm 等接口。各模式之间存在大量重复的 START、STOP 和模式
上报代码，增加或调整操作时需要在多个 Handler 中分别修改，不便于核对各模式
的完整行为。

本次只重构调用组织方式，不改变现有模式行为、接口调用次数、调用顺序、消息
参数、异步回执、阻塞条件或产测逻辑。

## 2. 目标

- 在 `entry_task.c` 内用有序操作表描述 M1-M5 的现有调用序列。
- 通过一个共用执行器逐项调用现有接口。
- 保留 `ModeM1Handler()` 至 `ModeM5Handler()` 作为现有模式入口。
- 让各模式的操作顺序可以集中阅读和对比。
- 减少 Handler 内重复的 `Message_Cmd_Put()` 和外设控制调用。
- 通过静态检查、构建和板端场景证明重构前后行为等价。

## 3. 非目标

本次不实施以下改动：

- 不将代码拆分到新的 `mode_manager.c/.h`。
- 不根据当前外设状态计算模式差异。
- 不跳过或合并重复的 START、STOP。
- 不调整任何操作顺序。
- 不增加新的延时、重试、信号量、事件标志或异步等待。
- 不改变 `TaskManager_SetMode()` 的模式判定、赋值时机或 `switch`。
- 不修改 CAT1、GNSS、Sensor、PM、Comm 或 Test Task 的内部状态机。
- 不改变 M4 中 CAT1 的 `STOP_REPLY -> START` 重启逻辑。
- 不改变 M4 产测任务的创建、启动和回执同步逻辑。
- 不修复本次重构范围以外的现有问题。

## 4. 方案比较

### 4.1 仅使用模式函数指针表

用函数指针数组替换 `TaskManager_SetMode()` 中的 `switch`。

优点是改动最小；缺点是各 Handler 内的重复调用仍然存在，不能解决主要维护
问题。本次不采用。

### 4.2 有序操作表和共用执行器

每个模式用按顺序排列的操作项描述，共用执行器逐项调用当前接口。

该方案能够保留原有接口层级、调用顺序、参数和次数，同时集中展示各模式行为。
本次采用该方案。

### 4.3 目标资源状态表

每个模式只描述 BLE、LTE、GNSS 等资源的目标状态，由管理器比较当前状态并
执行差异。

该方案最简洁，但会引入状态缓存、操作去重和新的转换语义，可能改变现有调用
顺序及次数，不符合本次等价重构要求。本次不采用。

## 5. 总体架构

所有新增类型、操作表和执行函数均保留在 `entry_task.c` 内，并声明为
`static`。模式入口和外部接口保持不变。

### 5.1 操作类型

操作枚举按当前接口语义区分，不把行为相近但入口不同的操作强制合并。

```c
typedef enum {
    MODE_OP_END = 0,
    MODE_OP_MODE_REPORT,
    MODE_OP_BLE_ADV_STOP,
    MODE_OP_BLE_NORMAL_ADV_START,
    MODE_OP_BLE_HIGH_SPEED_ADV_START,
    MODE_OP_PM_STOP_MESSAGE,
    MODE_OP_PM_SET_INTERVAL,
    MODE_OP_SENSOR_SAFE_START,
    MODE_OP_SENSOR_SAFE_STOP,
    MODE_OP_SENSOR_RAW_STOP_MESSAGE,
    MODE_OP_LED_STOP_MESSAGE,
    MODE_OP_CAT1_START_MESSAGE,
    MODE_OP_CAT1_STOP_MESSAGE,
    MODE_OP_GNSS_POWER_ON,
    MODE_OP_GNSS_POWER_OFF,
    MODE_OP_GNSS_ENTER_BACKUP,
    MODE_OP_MONITOR_START,
    MODE_OP_MONITOR_STOP,
    MODE_OP_SLEEP_PREVENT,
    MODE_OP_BOARD_DEINIT_TEST,
    MODE_OP_LFS_UNMOUNT,
    MODE_OP_M2_POWER_BRANCH,
} ModeOperation_t;
```

其中 Sensor 操作需要明确区分：

- `MODE_OP_SENSOR_SAFE_START` 调用 `sensor_task_start()`。
- `MODE_OP_SENSOR_SAFE_STOP` 调用 `sensor_task_stop()`，保留其状态检查和信号量
  处理。
- `MODE_OP_SENSOR_RAW_STOP_MESSAGE` 直接向 Sensor 消息队列发送
  `TASK_CMD_STOP`，保留 M4 当前行为。

### 5.2 操作步骤

```c
typedef struct {
    ModeOperation_t operation;
    uint32_t argument;
} ModeStep_t;
```

操作表使用 `static const`，避免占用可写 RAM。每张表必须以
`MODE_OP_END` 结束。

### 5.3 共用执行器

执行器只做顺序分发，不承担状态管理：

```c
static void ModeManager_ExecuteOperation(const ModeStep_t *step);
static void ModeManager_ExecuteSteps(const ModeStep_t *steps);
```

`ModeManager_ExecuteSteps()` 从表首执行到 `MODE_OP_END`。
`ModeManager_ExecuteOperation()` 使用 `switch` 将操作项映射到当前接口。

执行器必须满足：

- 严格按表中顺序调用。
- 不缓存外设状态。
- 不合并相邻操作。
- 不增加延时或异步等待。
- 不因某个接口失败而跳过后续操作。
- 保持当前被忽略的返回值仍然被忽略。
- 只在对应操作分支中解释 `argument`。

## 6. M1-M5 等价操作映射

### 6.1 M1

M1 当前只有日志，不调用 `COMM_MODE_REPORT()` 或任务控制接口，因此使用空操作
表。不得为了形式统一增加新的模式上报。

```c
static const ModeStep_t mode_m1_steps[] = {
    { MODE_OP_END, 0 },
};
```

### 6.2 M2

```c
static const ModeStep_t mode_m2_steps[] = {
    { MODE_OP_MODE_REPORT,      MODE_M2 },
    { MODE_OP_BLE_ADV_STOP,     0 },
    { MODE_OP_PM_STOP_MESSAGE,  0 },
    { MODE_OP_SENSOR_SAFE_STOP, 0 },
    { MODE_OP_LED_STOP_MESSAGE, 0 },
    { MODE_OP_M2_POWER_BRANCH,  0 },
    { MODE_OP_MONITOR_STOP,     0 },
    { MODE_OP_END,              0 },
};
```

`MODE_OP_M2_POWER_BRANCH` 保留当前条件和调用位置：

```text
如果 low_power_off != 1：
    CAT1 <- TASK_CMD_STOP
    GNSS 完全关机
否则：
    PowerOffSystem()
```

该分支仍位于 LED STOP 和 Comm Monitor STOP 之间。

### 6.3 M3

```c
static const ModeStep_t mode_m3_steps[] = {
    { MODE_OP_MODE_REPORT,          MODE_M3 },
    { MODE_OP_CAT1_STOP_MESSAGE,    0 },
    { MODE_OP_GNSS_POWER_OFF,       0 },
    { MODE_OP_BLE_NORMAL_ADV_START, 0 },
    { MODE_OP_PM_SET_INTERVAL,      10 },
    { MODE_OP_SENSOR_SAFE_STOP,     0 },
    { MODE_OP_BOARD_DEINIT_TEST,    0 },
    { MODE_OP_LFS_UNMOUNT,          0 },
    { MODE_OP_MONITOR_STOP,         0 },
    { MODE_OP_END,                  0 },
};
```

### 6.4 M4

M4 Handler 保留原日志及 `drv_pmu_retention_reg2_set(0)`，并确保 retention 清除
仍发生在模式操作表执行前。

```c
static const ModeStep_t mode_m4_steps[] = {
    { MODE_OP_MODE_REPORT,              MODE_M4 },
    { MODE_OP_CAT1_STOP_MESSAGE,        0 },
    { MODE_OP_GNSS_ENTER_BACKUP,        0 },
    { MODE_OP_BLE_HIGH_SPEED_ADV_START, 0 },
    { MODE_OP_PM_SET_INTERVAL,          10 },
    { MODE_OP_SENSOR_RAW_STOP_MESSAGE,  0 },
    { MODE_OP_MONITOR_START,            0 },
    { MODE_OP_END,                      0 },
};
```

M4 的 CAT1 STOP 回执处理、LTE 重启、GNSS/CAT1 就绪事件和
`m4_to_production_config()` 均不进入操作表，也不做修改。

### 6.5 M5

```c
static const ModeStep_t mode_m5_steps[] = {
    { MODE_OP_SLEEP_PREVENT,        0 },
    { MODE_OP_MODE_REPORT,          MODE_M5 },
    { MODE_OP_CAT1_START_MESSAGE,   0 },
    { MODE_OP_GNSS_POWER_ON,        0 },
    { MODE_OP_BLE_NORMAL_ADV_START, 0 },
    { MODE_OP_SENSOR_SAFE_START,    0 },
    { MODE_OP_PM_SET_INTERVAL,      10 },
    { MODE_OP_MONITOR_START,        0 },
    { MODE_OP_END,                  0 },
};
```

`MODE_OP_SLEEP_PREVENT` 必须保持为第一项。

## 7. Handler 保留策略

现有 Handler 名称和调用关系保持不变：

```c
void ModeM4Handler(void)
{
    log_debug("...Mode M4 running...:%d\r\n",
              littlefs_create_flag_get());

    drv_pmu_retention_reg2_set(0);
    ModeManager_ExecuteSteps(mode_m4_steps);
}
```

其他 Handler 按相同方式保留原日志，再执行对应操作表。M2 日志中的
`low_power_off`、M3 日志中的当前电量以及 M4 日志中的文件系统状态均保持
不变。

第一阶段不使用描述符表替换 `TaskManager_SetMode()` 的 `switch`，以限制改动
范围并降低模式分发行为变化风险。

## 8. 错误处理

本次不引入新的运行时错误策略：

- `Message_Cmd_Put()` 的返回值继续按当前 Handler 行为处理。
- `PM_SetTaskTimer()` 和 `lfs_unmount_safe()` 等返回值不增加新的分支。
- 执行器不提前终止后续操作。
- 不增加重试和回滚。

操作表属于编译期静态数据，使用 `MODE_OP_END` 作为终止项。实现时应检查每张
表均包含终止项，并确保操作参数不会隐式截断。

## 9. 验证方案

### 9.1 调用基线

以重构前代码建立 M1-M5 的有序接口调用清单。重构后的每张操作表必须逐项
匹配对应清单，包括：

- 接口名称。
- 调用先后顺序。
- 调用次数。
- 消息 source、dest、command、data 和长度。
- 传入的模式及定时参数。

### 9.2 静态检查

确认：

- 所有原有 Handler 调用均能在对应操作表或保留的 Handler 特殊逻辑中找到。
- M2 条件分支位置及内容不变。
- M4 retention 清除顺序不变。
- M4 的 `TASK_STOP_REPLY -> CAT1 START` 逻辑不变。
- `m4_to_production_config()` 不变。
- `EntryTask_HandleMessageQueue()` 不变。
- `TaskManager_SetMode()` 的判定、赋值及 `switch` 不变。
- `Test_Task.c`、`CAT1_UART_Task.c`、`GNSS_UART_Task.c` 不修改。

### 9.3 构建检查

- 执行工程全量编译。
- 不引入新的编译 warning。
- 确认操作表为 `static const`。
- 确认所有表包含 `MODE_OP_END`。
- 确认枚举和 `argument` 不发生隐式截断。

### 9.4 板端场景

至少覆盖：

1. 未绑定、未充电进入 M2。
2. 已绑定、低电量进入 M3。
3. M5 运行中插电进入 M4。
4. 上电时已经插电，直接进入 M4。
5. M4 中 CAT1 返回 `TASK_STOP_REPLY`，LTE 按当前逻辑重启。
6. M4 中发送一次 `productStart`，Test Task 正常创建并 START。
7. `productStart` 先于 CAT1/GNSS STOP 回执到达。
8. CAT1/GNSS STOP 回执先于 `productStart` 到达。
9. M4 拔掉充电器，仍走当前重启流程。
10. M2 的 `low_power_off == 0` 和 `low_power_off == 1` 分支。

## 10. 验收标准

以下条件必须全部满足：

- 模式判定结果和 Handler 调用次数不变。
- 每个 Handler 内的接口调用顺序和次数不变。
- 所有消息字段和接口参数不变。
- M4 两路 STOP 回执条件不变。
- LTE 重启触发点不变。
- Test Task 创建和 START 触发点不变。
- 除 `entry_task.c` 外不修改业务源文件。
- 全量构建通过且无新增 warning。
- 板端场景的行为和关键日志顺序与重构前一致。

## 11. 实施边界

实施阶段预计只修改：

- `projects/Dog_Collar_Prj/Source/entry_task.c`

设计文档和后续实施计划除外。若实现过程中发现必须修改其他业务源文件，应停止
实施并重新确认范围，不得自行扩大改动。
