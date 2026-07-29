#ifndef BLE_STATE_MACHINE_H
#define BLE_STATE_MACHINE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    BLE_STATE_IDLE = 0,
    BLE_STATE_STARTING,
    BLE_STATE_ADVERTISING,
    BLE_STATE_CONNECTING,
    BLE_STATE_CONNECTED,
    BLE_STATE_STOPPING,
    BLE_STATE_DISCONNECTING,
    BLE_STATE_MAX
} BleState_t;

typedef enum {
    BLE_ADV_MODE_NORMAL = 0,
    BLE_ADV_MODE_HIGH_SPEED,
    BLE_ADV_MODE_MAX
} BleAdvertisingMode_t;

typedef enum {
    BLE_EVT_ENABLE_REQUEST = 0,
    BLE_EVT_ENABLE_HIGH_SPEED_REQUEST,
    BLE_EVT_DISABLE_REQUEST,
    BLE_EVT_DISCONNECT_REQUEST,
    BLE_EVT_ADV_STARTED_CONFIRMED,
    BLE_EVT_ADV_STOPPED_BY_USER,
    BLE_EVT_ADV_STOPPED_BY_CONNECTED,
    BLE_EVT_ADV_STOPPED_UNEXPECTED,
    BLE_EVT_CONNECTED_CONFIRMED,
    BLE_EVT_DISCONNECTED_CONFIRMED,
    BLE_EVT_RESTART_TIMEOUT,
    BLE_EVT_MAX
} BleEvent_t;

typedef uint32_t (*BleStateMachineStartOperation_t)(
    void *context,
    BleAdvertisingMode_t mode);
typedef uint32_t (*BleStateMachineOperation_t)(void *context);
typedef void (*BleStateMachineTimerOperation_t)(void *context);

typedef struct {
    BleStateMachineStartOperation_t start_advertising;
    BleStateMachineOperation_t stop_advertising;
    BleStateMachineOperation_t disconnect;
    BleStateMachineTimerOperation_t start_restart_timer;
    BleStateMachineTimerOperation_t cancel_restart_timer;
} BleStateMachineOps_t;

typedef struct {
    BleState_t state;
    bool target_enabled;
    bool restart_pending;
    bool disconnect_requested;
    BleAdvertisingMode_t target_mode;
    BleAdvertisingMode_t pending_mode;
    BleAdvertisingMode_t active_mode;
    BleStateMachineOps_t ops;
    void *operation_context;
} BleStateMachine_t;

void BleStateMachine_Init(BleStateMachine_t *machine,
                          const BleStateMachineOps_t *ops,
                          void *operation_context);
void BleStateMachine_Dispatch(BleStateMachine_t *machine, BleEvent_t event);
BleState_t BleStateMachine_GetState(const BleStateMachine_t *machine);
bool BleStateMachine_IsTargetEnabled(const BleStateMachine_t *machine);
BleAdvertisingMode_t BleStateMachine_GetTargetMode(
    const BleStateMachine_t *machine);
BleAdvertisingMode_t BleStateMachine_GetActiveMode(
    const BleStateMachine_t *machine);

#endif
