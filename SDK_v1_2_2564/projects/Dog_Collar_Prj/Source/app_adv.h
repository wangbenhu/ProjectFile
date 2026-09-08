#ifndef APP_ADV_H
#define APP_ADV_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    BLE_ADV_MODE_FAST = 0,
    BLE_ADV_MODE_SLOW
} ble_adv_mode_t;

void app_adv_init(void);
uint32_t app_adv_start(ble_adv_mode_t mode);
uint32_t app_adv_stop(void);
bool app_adv_get_conn_idx(uint8_t *conn_idx);
uint32_t app_adv_disconnect(void);
#endif
