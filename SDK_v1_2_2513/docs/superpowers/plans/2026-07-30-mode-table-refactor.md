# M1-M5 Mode Table Refactor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace direct repeated M1-M5 mode operations with ordered static operation tables and one shared dispatcher while preserving every existing call, parameter, order, asynchronous handshake, and production-test behavior.

**Architecture:** Keep all new types, tables, and dispatcher functions inside `entry_task.c`. Existing `ModeM1Handler()` through `ModeM5Handler()` remain the externally visible entry points; each keeps its current logging and one-off logic, then runs an ordered table without state-diffing, deduplication, retries, or new waits.

**Tech Stack:** C, CMSIS-RTOS2, FreeRTOS, Keil MDK project `projects/Dog_Collar_Prj/keil5/ble_app_simple.uvprojx`, PowerShell source-characterization checks.

## Global Constraints

- Modify only `projects/Dog_Collar_Prj/Source/entry_task.c` as business source.
- Keep all new mode-table code in `entry_task.c`; do not create `mode_manager.c/.h`.
- Preserve operation order, call count, parameters, message fields, return-value handling, and blocking behavior exactly.
- Do not add state caching, operation deduplication, delays, retries, semaphores, event flags, or asynchronous waits.
- Do not alter `TaskManager_SetMode()`, `EntryTask_HandleMessageQueue()`, `m4_to_production_config()`, CAT1/GNSS STOP replies, M4 LTE restart, or Test Task startup.
- Preserve the existing uncommitted `set_entry_low_sleep_flag()` validation change and do not include it in the mode-refactor commit.
- Preserve the unrelated `projects/Dog_Collar_Prj/keil5/.vscode/keil-assistant.log` modification.
- Do not modify `Test_Task.c`, `CAT1_UART_Task.c`, `GNSS_UART_Task.c`, `Sensor_Task.c`, `pm_Task.c`, or `Comm_Task.c`.

---

## File Structure

- Modify: `projects/Dog_Collar_Prj/Source/entry_task.c`
  - Owns `ModeOperation_t`, `ModeStep_t`, five `static const` operation tables, the shared dispatcher, and the existing mode handlers.
- Reference only: `docs/superpowers/specs/2026-07-30-mode-table-refactor-design.md`
  - Defines the approved behavior-preserving architecture and acceptance criteria.
- Reference only: `projects/Dog_Collar_Prj/Source/Sensor_Task.c`
  - Confirms that `sensor_task_stop()` is not equivalent to sending a raw STOP message.
- Reference only: `projects/Dog_Collar_Prj/keil5/ble_app_simple.uvprojx`
  - Keil build target.

### Task 1: Implement the ordered operation tables and shared dispatcher

**Files:**
- Modify: `projects/Dog_Collar_Prj/Source/entry_task.c:611-928`
- Reference: `docs/superpowers/specs/2026-07-30-mode-table-refactor-design.md`

**Interfaces:**
- Consumes:
  - `void COMM_MODE_REPORT(SystemMode_t mode)`
  - `uint8_t Message_Cmd_Put(TASK_ID_T source_id, TASK_ID_T dest_id, TASK_CMD_T command, void *data, uint16_t data_length)`
  - `bool PM_SetTaskTimer(uint8_t value)`
  - Existing BLE, GNSS, Sensor, PM, filesystem, sleep, and shutdown functions already visible to `entry_task.c`
- Produces:
  - `static void ModeManager_ExecuteOperation(const ModeStep_t *step)`
  - `static void ModeManager_ExecuteSteps(const ModeStep_t *steps)`
  - `static const ModeStep_t mode_m1_steps[]` through `mode_m5_steps[]`
  - Unchanged public signatures for `ModeM1Handler()` through `ModeM5Handler()`

- [ ] **Step 1: Capture and protect the pre-existing user changes**

Run:

```powershell
git status --short
git diff -- projects/Dog_Collar_Prj/Source/entry_task.c
```

Expected:

```text
M projects/Dog_Collar_Prj/Source/entry_task.c
M projects/Dog_Collar_Prj/keil5/.vscode/keil-assistant.log
```

The `entry_task.c` diff must include the pre-existing validation correction:

```c
if (state > SYSTEM_POWER_STATE_SHUTDOWN ||
    state < SYSTEM_POWER_STATE_DEFAULT) {
```

Do not revert, rewrite, or stage this hunk as part of the mode-table commit.

- [ ] **Step 2: Run the source-characterization check and verify it fails before implementation**

Run:

```powershell
$modeSource = Get-Content -Raw 'projects\Dog_Collar_Prj\Source\entry_task.c'
$requiredSymbols = @(
    'ModeOperation_t',
    'ModeStep_t',
    'mode_m1_steps',
    'mode_m2_steps',
    'mode_m3_steps',
    'mode_m4_steps',
    'mode_m5_steps',
    'ModeManager_ExecuteOperation',
    'ModeManager_ExecuteSteps'
)
$missingSymbols = $requiredSymbols | Where-Object { $modeSource -notmatch [regex]::Escape($_) }
if ($missingSymbols.Count -ne 0) {
    throw "Missing mode-table symbols: $($missingSymbols -join ', ')"
}
```

Expected: FAIL with `Missing mode-table symbols`.

- [ ] **Step 3: Add the operation types and all five ordered tables**

Immediately before `ModeM1Handler()`, add the following definitions. Keep
`MODE_OP_SENSOR_SAFE_STOP` and `MODE_OP_SENSOR_RAW_STOP_MESSAGE` separate.

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

typedef struct {
    ModeOperation_t operation;
    uint32_t argument;
} ModeStep_t;

static void ModeManager_ExecuteSteps(const ModeStep_t *steps);

static const ModeStep_t mode_m1_steps[] = {
    { MODE_OP_END, 0 },
};

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

- [ ] **Step 4: Add the complete shared dispatcher**

Place the dispatcher after `board_deinit_test()` so every directly called
function has been declared or defined. Use exactly these mappings:

```c
static void ModeManager_ExecuteOperation(const ModeStep_t *step)
{
    switch (step->operation)
    {
        case MODE_OP_MODE_REPORT:
            COMM_MODE_REPORT((SystemMode_t)step->argument);
            break;

        case MODE_OP_BLE_ADV_STOP:
            evt_app_adv_stop();
            break;

        case MODE_OP_BLE_NORMAL_ADV_START:
            evt_app_adv_start();
            break;

        case MODE_OP_BLE_HIGH_SPEED_ADV_START:
            evt_app_high_speed_adv_start();
            break;

        case MODE_OP_PM_STOP_MESSAGE:
            Message_Cmd_Put(ENTRY_TASK_ID,
                            PM_TASK_ID,
                            TASK_CMD_STOP,
                            NULL,
                            0);
            break;

        case MODE_OP_PM_SET_INTERVAL:
            PM_SetTaskTimer((uint8_t)step->argument);
            break;

        case MODE_OP_SENSOR_SAFE_START:
            sensor_task_start();
            break;

        case MODE_OP_SENSOR_SAFE_STOP:
            sensor_task_stop();
            break;

        case MODE_OP_SENSOR_RAW_STOP_MESSAGE:
            Message_Cmd_Put(ENTRY_TASK_ID,
                            SENSOR_TASK_ID,
                            TASK_CMD_STOP,
                            NULL,
                            0);
            break;

        case MODE_OP_LED_STOP_MESSAGE:
            Message_Cmd_Put(ENTRY_TASK_ID,
                            LED_TASK_ID,
                            TASK_CMD_STOP,
                            NULL,
                            0);
            break;

        case MODE_OP_CAT1_START_MESSAGE:
            Message_Cmd_Put(ENTRY_TASK_ID,
                            CAT1_UART_TASK_ID,
                            TASK_CMD_START,
                            NULL,
                            0);
            break;

        case MODE_OP_CAT1_STOP_MESSAGE:
            Message_Cmd_Put(ENTRY_TASK_ID,
                            CAT1_UART_TASK_ID,
                            TASK_CMD_STOP,
                            NULL,
                            0);
            break;

        case MODE_OP_GNSS_POWER_ON:
            Entry_Control_GPS_Start_Power();
            break;

        case MODE_OP_GNSS_POWER_OFF:
            Entry_Control_GPS_Stop_Power();
            break;

        case MODE_OP_GNSS_ENTER_BACKUP:
            ENTRY_Control_GPS_Start();
            break;

        case MODE_OP_MONITOR_START:
            Message_Cmd_Put(ENTRY_TASK_ID,
                            COMM_TASK_ID,
                            TASK_STATE_MONITOR_START,
                            NULL,
                            0);
            break;

        case MODE_OP_MONITOR_STOP:
            Message_Cmd_Put(ENTRY_TASK_ID,
                            COMM_TASK_ID,
                            TASK_STATE_MONITOR_STOP,
                            NULL,
                            0);
            break;

        case MODE_OP_SLEEP_PREVENT:
            pm_sleep_prevent(PM_ID_ENTRY_SLEEP);
            break;

        case MODE_OP_BOARD_DEINIT_TEST:
            board_deinit_test();
            break;

        case MODE_OP_LFS_UNMOUNT:
            lfs_unmount_safe();
            break;

        case MODE_OP_M2_POWER_BRANCH:
            if (low_power_off != 1)
            {
                Message_Cmd_Put(ENTRY_TASK_ID,
                                CAT1_UART_TASK_ID,
                                TASK_CMD_STOP,
                                NULL,
                                0);
                Entry_Control_GPS_Stop_Power();
            }
            else
            {
                PowerOffSystem();
            }
            break;

        case MODE_OP_END:
        default:
            break;
    }
}

static void ModeManager_ExecuteSteps(const ModeStep_t *steps)
{
    uint32_t index = 0;

    while (steps[index].operation != MODE_OP_END)
    {
        ModeManager_ExecuteOperation(&steps[index]);
        index++;
    }
}
```

Do not add return-value checks, retries, logging, delays, bounds state, or
operation deduplication to this dispatcher.

- [ ] **Step 5: Convert all five handlers without moving special logic**

Replace the handler bodies with:

```c
void ModeM1Handler(void)
{
    log_debug("...Mode M1 running...\r\n");
    ModeManager_ExecuteSteps(mode_m1_steps);
}

void ModeM2Handler(void)
{
    log_debug("...Mode M2 running...:%d\r\n", low_power_off);
    ModeManager_ExecuteSteps(mode_m2_steps);
}

void ModeM3Handler(void)
{
    log_debug("...Mode M3 running...:%d\r\n", PM_GetBatteryCapacity());
    ModeManager_ExecuteSteps(mode_m3_steps);
}

void ModeM4Handler(void)
{
    log_debug("...Mode M4 running...:%d\r\n", littlefs_create_flag_get());

    drv_pmu_retention_reg2_set(0);
    ModeManager_ExecuteSteps(mode_m4_steps);
}

void ModeM5Handler(void)
{
    log_debug("...Mode M5 running...\r\n");
    ModeManager_ExecuteSteps(mode_m5_steps);
}
```

Keep `ModeErrorHandler()` unchanged. Do not modify the later
`TaskManager_SetMode()` switch.

- [ ] **Step 6: Run the exact operation-table characterization check**

Run:

```powershell
$modeSource = Get-Content -Raw 'projects\Dog_Collar_Prj\Source\entry_task.c'
$expectedModeOps = [ordered]@{
    mode_m1_steps = @(
        'MODE_OP_END'
    )
    mode_m2_steps = @(
        'MODE_OP_MODE_REPORT',
        'MODE_OP_BLE_ADV_STOP',
        'MODE_OP_PM_STOP_MESSAGE',
        'MODE_OP_SENSOR_SAFE_STOP',
        'MODE_OP_LED_STOP_MESSAGE',
        'MODE_OP_M2_POWER_BRANCH',
        'MODE_OP_MONITOR_STOP',
        'MODE_OP_END'
    )
    mode_m3_steps = @(
        'MODE_OP_MODE_REPORT',
        'MODE_OP_CAT1_STOP_MESSAGE',
        'MODE_OP_GNSS_POWER_OFF',
        'MODE_OP_BLE_NORMAL_ADV_START',
        'MODE_OP_PM_SET_INTERVAL',
        'MODE_OP_SENSOR_SAFE_STOP',
        'MODE_OP_BOARD_DEINIT_TEST',
        'MODE_OP_LFS_UNMOUNT',
        'MODE_OP_MONITOR_STOP',
        'MODE_OP_END'
    )
    mode_m4_steps = @(
        'MODE_OP_MODE_REPORT',
        'MODE_OP_CAT1_STOP_MESSAGE',
        'MODE_OP_GNSS_ENTER_BACKUP',
        'MODE_OP_BLE_HIGH_SPEED_ADV_START',
        'MODE_OP_PM_SET_INTERVAL',
        'MODE_OP_SENSOR_RAW_STOP_MESSAGE',
        'MODE_OP_MONITOR_START',
        'MODE_OP_END'
    )
    mode_m5_steps = @(
        'MODE_OP_SLEEP_PREVENT',
        'MODE_OP_MODE_REPORT',
        'MODE_OP_CAT1_START_MESSAGE',
        'MODE_OP_GNSS_POWER_ON',
        'MODE_OP_BLE_NORMAL_ADV_START',
        'MODE_OP_SENSOR_SAFE_START',
        'MODE_OP_PM_SET_INTERVAL',
        'MODE_OP_MONITOR_START',
        'MODE_OP_END'
    )
}

foreach ($tableName in $expectedModeOps.Keys) {
    $tablePattern = "static const ModeStep_t\s+$tableName\[\]\s*=\s*\{(?<body>.*?)\};"
    $tableMatch = [regex]::Match(
        $modeSource,
        $tablePattern,
        [System.Text.RegularExpressions.RegexOptions]::Singleline
    )
    if (-not $tableMatch.Success) {
        throw "Missing table: $tableName"
    }

    $actualOps = [regex]::Matches(
        $tableMatch.Groups['body'].Value,
        'MODE_OP_[A-Z0-9_]+'
    ) | ForEach-Object { $_.Value }

    $difference = Compare-Object $expectedModeOps[$tableName] $actualOps -SyncWindow 0
    if ($difference) {
        throw "Operation order mismatch in $tableName`n$($difference | Out-String)"
    }
}

'All mode operation tables match the approved order.'
```

Expected:

```text
All mode operation tables match the approved order.
```

- [ ] **Step 7: Verify protected functions and files were not changed by this task**

Run:

```powershell
rg -n 'm4_to_production_config|EntryTask_HandleMessageQueue|TaskManager_SetMode|TASK_STOP_REPLY' projects\Dog_Collar_Prj\Source\entry_task.c
git diff --name-only
git diff -- projects/Dog_Collar_Prj/Source/Test_Task.c projects/Dog_Collar_Prj/Source/CAT1_UART_Task.c projects/Dog_Collar_Prj/Source/GNSS_UART_Task.c projects/Dog_Collar_Prj/Source/Sensor_Task.c projects/Dog_Collar_Prj/Source/pm_Task.c projects/Dog_Collar_Prj/Source/Comm_Task.c
git diff --check
```

Expected:

- `m4_to_production_config()`, `EntryTask_HandleMessageQueue()`,
  `TaskManager_SetMode()`, and CAT1/GNSS reply references are still present.
- The protected task-file diff is empty.
- `git diff --check` exits successfully.
- `git diff --name-only` contains the pre-existing Keil log and
  `entry_task.c`; it must not contain newly modified business files.

- [ ] **Step 8: Build the Keil target and inspect the generated build log**

The current shell does not expose `UV4.exe` on `PATH`. Use the configured
Keil MDK/Keil Assistant environment to perform a full rebuild of target
`ble_app_simple` from:

```text
projects/Dog_Collar_Prj/keil5/ble_app_simple.uvprojx
```

If `UV4.exe` is made available on `PATH`, run:

```powershell
$uv4Command = Get-Command UV4.exe -ErrorAction Stop
& $uv4Command.Source `
    -b 'projects\Dog_Collar_Prj\keil5\ble_app_simple.uvprojx' `
    -j0 `
    -o 'projects\Dog_Collar_Prj\keil5\mode_table_build.log'
if ($LASTEXITCODE -ne 0) {
    throw "Keil build failed with exit code $LASTEXITCODE"
}
Get-Content 'projects\Dog_Collar_Prj\keil5\mode_table_build.log'
```

Otherwise, open the project in the configured Keil IDE, select
`ble_app_simple`, run **Rebuild all target files**, and inspect the newest
`projects/Dog_Collar_Prj/keil5/out/*.build_log.htm`.

Expected final summary:

```text
0 Error(s), 0 Warning(s).
```

Do not treat the pre-existing output files as proof of a new successful
build; the log timestamp must be newer than the source edit.

- [ ] **Step 9: Review the final source diff against the approved design**

Run:

```powershell
git diff -- projects/Dog_Collar_Prj/Source/entry_task.c
```

Check line by line:

- The pre-existing `set_entry_low_sleep_flag()` change remains intact.
- New mode-table changes are limited to the mode-handler area.
- M2 still performs its conditional branch between LED STOP and monitor STOP.
- M4 still clears retention before executing its table.
- M4 uses raw Sensor STOP; M2/M3 use `sensor_task_stop()`.
- M5 sleep prevention remains the first operation after its log.
- No protected asynchronous or production-test code changed.

- [ ] **Step 10: Stage only the mode-table hunks and commit**

Because `entry_task.c` already contains a user-owned hunk, stage
interactively:

```powershell
git add -p -- projects/Dog_Collar_Prj/Source/entry_task.c
```

At the validation-condition hunk near `set_entry_low_sleep_flag()`, answer
`n`. At the mode-table and Handler hunks, answer `y`. Then verify:

```powershell
git diff --cached --check
git diff --cached --name-only
git diff --cached -- projects/Dog_Collar_Prj/Source/entry_task.c
git diff -- projects/Dog_Collar_Prj/Source/entry_task.c
```

Expected:

- Cached diff contains only the mode-table refactor.
- Uncached diff still contains the user-owned validation change.
- Cached file list contains only
  `SDK_v1_2_2513/projects/Dog_Collar_Prj/Source/entry_task.c`.

Commit:

```powershell
git commit -m "refactor: table-drive system mode operations"
```

### Task 2: Run behavior-equivalence regression checks

**Files:**
- Test only: `projects/Dog_Collar_Prj/Source/entry_task.c`
- Test only: flashed firmware built from `projects/Dog_Collar_Prj/keil5/ble_app_simple.uvprojx`

**Interfaces:**
- Consumes:
  - The operation tables and handlers produced by Task 1.
  - Existing Entry, CAT1, GNSS, PM, Comm, and Test Task logs.
- Produces:
  - Evidence that mode selection, call order, LTE restart, and production-test entry remain unchanged.

- [ ] **Step 1: Re-run automated source and build verification from a clean index**

Run:

```powershell
git status --short
git show --stat --oneline HEAD
git show --check --oneline HEAD
```

Expected:

- HEAD contains only the mode-table refactor commit from Task 1.
- The user-owned `entry_task.c` validation hunk and Keil log remain
  uncommitted.
- `git show --check` reports no whitespace errors.

Repeat the exact operation-table characterization command from Task 1,
Step 6. Expected:

```text
All mode operation tables match the approved order.
```

- [ ] **Step 2: Verify M2 and M3 on hardware**

Test M2 twice:

1. Enter M2 with `low_power_off == 0`.
2. Enter M2 with `low_power_off == 1`.

Expected ordered evidence:

```text
Mode M2 log
mode report
BLE advertising stop
PM STOP
Sensor safe STOP
LED STOP
CAT1/GNSS STOP or PowerOffSystem according to low_power_off
Comm monitor STOP
```

Test M3 with a bound user and low battery.

Expected ordered evidence:

```text
Mode M3 log
mode report
CAT1 STOP
GNSS power off
normal BLE advertising start
PM interval 10
Sensor safe STOP
board_deinit_test
LittleFS unmount
Comm monitor STOP
```

- [ ] **Step 3: Verify M4 entry and LTE restart on hardware**

Run both scenarios:

1. Boot while charging.
2. Enter M4 by plugging in while M5 is running.

Expected:

```text
Mode M4 log
retention register 2 cleared
mode report
CAT1 STOP
GNSS backup request
high-speed BLE advertising start
PM interval 10
raw Sensor STOP message
Comm monitor START
CAT1 TASK_STOP_REPLY
CAT1 UART/task unblock
CAT1 TASK_CMD_START
```

Confirm that `FORE_MODE_STATUS` remains M4 and that the LTE restart trigger
is still CAT1 `TASK_STOP_REPLY`, not the operation-table executor.

- [ ] **Step 4: Verify both production-test timing orders**

Scenario A:

```text
Enter M4
wait for CAT1 and GNSS STOP replies
send productStart once
```

Scenario B:

```text
Enter M4
send productStart before both STOP replies arrive
wait for CAT1 and GNSS STOP replies
```

For both scenarios, expected:

```text
Test Task queue/thread created once
entry_low_sleep_flag reaches SYSTEM_POWER_STATE_SHUTDOWN_READY
ENTRY sends one TASK_CMD_START to TEST_TASK_ID
Test Task sets production_start_flag = 1
PRODUCTION_TASK_EXAMPLE_START is processed
REPORT_OK is produced
```

Do not send `productStart` twice to make the flow pass.

- [ ] **Step 5: Verify M5 and unplug behavior**

Enter M5 with a bound user and sufficient battery.

Expected ordered evidence:

```text
Mode M5 log
sleep prevention
mode report
CAT1 START
GNSS power on
normal BLE advertising start
Sensor safe START
PM interval 10
Comm monitor START
```

While in M4, unplug the charger.

Expected: the existing charger-removal reboot path remains unchanged; the
mode-table executor does not intercept or replace it.

- [ ] **Step 6: Record any unavailable hardware verification explicitly**

If no board, charger fixture, BLE production tool, or UART log capture is
available, do not report the corresponding scenario as passed. Report:

```text
Automated source characterization: PASS/FAIL
Keil full build: PASS/FAIL
M2/M3 board verification: PASS/NOT RUN
M4 LTE restart verification: PASS/NOT RUN
Production timing A/B: PASS/NOT RUN
M5/unplug verification: PASS/NOT RUN
```

No additional source commit is required for this verification-only task.
