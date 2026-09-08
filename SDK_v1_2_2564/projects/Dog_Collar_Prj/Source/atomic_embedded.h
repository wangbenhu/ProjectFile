#ifndef ATOMIC_EMBEDDED_H
#define ATOMIC_EMBEDDED_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef volatile uint8_t atomic_u8_t;

uint8_t atomic_load_u8(atomic_u8_t *v);
void atomic_store_u8(atomic_u8_t *v, uint8_t val);
bool atomic_cas_u8(atomic_u8_t *v, uint8_t old_val, uint8_t new_val);

#ifdef __cplusplus
}
#endif

#endif