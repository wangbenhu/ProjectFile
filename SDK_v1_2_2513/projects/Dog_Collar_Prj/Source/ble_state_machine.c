#include "ble_state_machine.h"

#include <stddef.h>
#include <string.h>

#define BLE_DRIVER_REQUEST_ACCEPTED 0U

typedef void (*BleStateHandler_t)(BleStateMachine_t *machine,
                                  BleEvent_t event);

static bool request_start(BleStateMachine_t *machine)
{
    uint32_t result;

    if (machine->ops.start_advertising == NULL) {
        return false;
    }

    result = machine->ops.start_advertising(machine->operation_context,
                                            machine->target_mode);
    if (result == BLE_DRIVER_REQUEST_ACCEPTED) {
        machine->pending_mode = machine->target_mode;
        return true;
    }
    return false;
}

static bool request_stop(BleStateMachine_t *machine)
{
    return machine->ops.stop_advertising != NULL &&
           machine->ops.stop_advertising(machine->operation_context) ==
               BLE_DRIVER_REQUEST_ACCEPTED;
}

static bool request_disconnect(BleStateMachine_t *machine)
{
    return machine->ops.disconnect != NULL &&
           machine->ops.disconnect(machine->operation_context) ==
               BLE_DRIVER_REQUEST_ACCEPTED;
}

static void start_restart_timer(BleStateMachine_t *machine)
{
    if (!machine->restart_pending &&
        machine->ops.start_restart_timer != NULL) {
        machine->restart_pending = true;
        machine->ops.start_restart_timer(machine->operation_context);
    }
}

static void cancel_restart_timer(BleStateMachine_t *machine)
{
    if (machine->restart_pending &&
        machine->ops.cancel_restart_timer != NULL) {
        machine->ops.cancel_restart_timer(machine->operation_context);
    }
    machine->restart_pending = false;
}

static bool is_enable_event(BleEvent_t event)
{
    return event == BLE_EVT_ENABLE_REQUEST ||
           event == BLE_EVT_ENABLE_HIGH_SPEED_REQUEST;
}

static BleAdvertisingMode_t requested_mode(BleEvent_t event)
{
    return event == BLE_EVT_ENABLE_HIGH_SPEED_REQUEST
               ? BLE_ADV_MODE_HIGH_SPEED
               : BLE_ADV_MODE_NORMAL;
}

static void apply_enable_request(BleStateMachine_t *machine,
                                 BleEvent_t event)
{
    machine->target_enabled = true;
    machine->target_mode = requested_mode(event);
}

static void handle_idle(BleStateMachine_t *machine, BleEvent_t event)
{
    if (is_enable_event(event)) {
        apply_enable_request(machine, event);
    } else if (event == BLE_EVT_DISABLE_REQUEST) {
        machine->target_enabled = false;
        machine->disconnect_requested = false;
        cancel_restart_timer(machine);
        return;
    } else if (event != BLE_EVT_RESTART_TIMEOUT) {
        return;
    }

    cancel_restart_timer(machine);
    if (machine->target_enabled && request_start(machine)) {
        machine->state = BLE_STATE_STARTING;
    }
}

static void handle_starting(BleStateMachine_t *machine, BleEvent_t event)
{
    if (event == BLE_EVT_DISABLE_REQUEST) {
        machine->target_enabled = false;
        machine->disconnect_requested = false;
        cancel_restart_timer(machine);
    } else if (is_enable_event(event)) {
        apply_enable_request(machine, event);
    } else if (event == BLE_EVT_ADV_STARTED_CONFIRMED) {
        cancel_restart_timer(machine);
        machine->active_mode = machine->pending_mode;
        machine->state = BLE_STATE_ADVERTISING;
        if ((!machine->target_enabled ||
             machine->active_mode != machine->target_mode) &&
            request_stop(machine)) {
            machine->state = BLE_STATE_STOPPING;
        }
    }
}

static void handle_advertising(BleStateMachine_t *machine, BleEvent_t event)
{
    if (is_enable_event(event)) {
        apply_enable_request(machine, event);
        if (machine->active_mode != machine->target_mode &&
            request_stop(machine)) {
            machine->state = BLE_STATE_STOPPING;
        }
    } else if (event == BLE_EVT_DISABLE_REQUEST) {
        machine->target_enabled = false;
        machine->disconnect_requested = false;
        cancel_restart_timer(machine);
        if (request_stop(machine)) {
            machine->state = BLE_STATE_STOPPING;
        }
    }
}

static void handle_connecting(BleStateMachine_t *machine, BleEvent_t event)
{
    if (is_enable_event(event)) {
        apply_enable_request(machine, event);
    } else if (event == BLE_EVT_DISABLE_REQUEST) {
        machine->target_enabled = false;
        machine->disconnect_requested = false;
        cancel_restart_timer(machine);
    } else if (event == BLE_EVT_DISCONNECT_REQUEST) {
        machine->target_enabled = true;
        machine->disconnect_requested = true;
    }
}

static void handle_connected(BleStateMachine_t *machine, BleEvent_t event)
{
    if (is_enable_event(event)) {
        apply_enable_request(machine, event);
    } else if (event == BLE_EVT_DISABLE_REQUEST) {
        machine->target_enabled = false;
        machine->disconnect_requested = false;
        cancel_restart_timer(machine);
        if (request_disconnect(machine)) {
            machine->state = BLE_STATE_DISCONNECTING;
        }
    } else if (event == BLE_EVT_DISCONNECT_REQUEST) {
        machine->target_enabled = true;
        machine->disconnect_requested = true;
        if (request_disconnect(machine)) {
            machine->state = BLE_STATE_DISCONNECTING;
        }
    }
}

static void handle_stopping(BleStateMachine_t *machine, BleEvent_t event)
{
    if (is_enable_event(event)) {
        apply_enable_request(machine, event);
    } else if (event == BLE_EVT_DISABLE_REQUEST) {
        machine->target_enabled = false;
        machine->disconnect_requested = false;
        cancel_restart_timer(machine);
    } else if (event == BLE_EVT_ADV_STOPPED_BY_USER) {
        machine->state = BLE_STATE_IDLE;
        if (machine->target_enabled && request_start(machine)) {
            machine->state = BLE_STATE_STARTING;
        }
    }
}

static void handle_disconnecting(BleStateMachine_t *machine,
                                 BleEvent_t event)
{
    if (is_enable_event(event)) {
        apply_enable_request(machine, event);
    } else if (event == BLE_EVT_DISABLE_REQUEST) {
        machine->target_enabled = false;
        machine->disconnect_requested = false;
        cancel_restart_timer(machine);
    } else if (event == BLE_EVT_DISCONNECT_REQUEST) {
        machine->target_enabled = true;
        machine->disconnect_requested = true;
    }
}

static const BleStateHandler_t state_handlers[BLE_STATE_MAX] = {
    [BLE_STATE_IDLE] = handle_idle,
    [BLE_STATE_STARTING] = handle_starting,
    [BLE_STATE_ADVERTISING] = handle_advertising,
    [BLE_STATE_CONNECTING] = handle_connecting,
    [BLE_STATE_CONNECTED] = handle_connected,
    [BLE_STATE_STOPPING] = handle_stopping,
    [BLE_STATE_DISCONNECTING] = handle_disconnecting
};

void BleStateMachine_Init(BleStateMachine_t *machine,
                          const BleStateMachineOps_t *ops,
                          void *operation_context)
{
    if (machine == NULL || ops == NULL) {
        return;
    }

    memset(machine, 0, sizeof(*machine));
    machine->state = BLE_STATE_IDLE;
    machine->target_mode = BLE_ADV_MODE_NORMAL;
    machine->pending_mode = BLE_ADV_MODE_NORMAL;
    machine->active_mode = BLE_ADV_MODE_NORMAL;
    machine->ops = *ops;
    machine->operation_context = operation_context;
}

void BleStateMachine_Dispatch(BleStateMachine_t *machine, BleEvent_t event)
{
    if (machine == NULL || event >= BLE_EVT_MAX ||
        machine->state >= BLE_STATE_MAX) {
        return;
    }

    if (event == BLE_EVT_CONNECTED_CONFIRMED) {
        cancel_restart_timer(machine);
        machine->state = BLE_STATE_CONNECTED;
        if ((!machine->target_enabled || machine->disconnect_requested) &&
            request_disconnect(machine)) {
            machine->state = BLE_STATE_DISCONNECTING;
        }
        return;
    }

    if (event == BLE_EVT_DISCONNECTED_CONFIRMED) {
        machine->state = BLE_STATE_IDLE;
        machine->disconnect_requested = false;
        if (machine->target_enabled) {
            start_restart_timer(machine);
        } else {
            cancel_restart_timer(machine);
        }
        return;
    }

    if (event == BLE_EVT_ADV_STOPPED_BY_CONNECTED &&
        (machine->state == BLE_STATE_STARTING ||
         machine->state == BLE_STATE_ADVERTISING ||
         machine->state == BLE_STATE_STOPPING)) {
        machine->state = BLE_STATE_CONNECTING;
        return;
    }

    if (event == BLE_EVT_ADV_STOPPED_UNEXPECTED &&
        (machine->state == BLE_STATE_STARTING ||
         machine->state == BLE_STATE_ADVERTISING ||
         machine->state == BLE_STATE_STOPPING ||
         machine->state == BLE_STATE_CONNECTING)) {
        machine->state = BLE_STATE_IDLE;
        if (machine->target_enabled) {
            start_restart_timer(machine);
        } else {
            cancel_restart_timer(machine);
        }
        return;
    }

    state_handlers[machine->state](machine, event);
}

BleState_t BleStateMachine_GetState(const BleStateMachine_t *machine)
{
    return machine == NULL ? BLE_STATE_IDLE : machine->state;
}

bool BleStateMachine_IsTargetEnabled(const BleStateMachine_t *machine)
{
    return machine != NULL && machine->target_enabled;
}

BleAdvertisingMode_t BleStateMachine_GetTargetMode(
    const BleStateMachine_t *machine)
{
    return machine == NULL ? BLE_ADV_MODE_NORMAL : machine->target_mode;
}

BleAdvertisingMode_t BleStateMachine_GetActiveMode(
    const BleStateMachine_t *machine)
{
    return machine == NULL ? BLE_ADV_MODE_NORMAL : machine->active_mode;
}
