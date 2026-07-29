#include "atomic_embedded.h"
#include "cmsis_gcc.h"   // 或 cmsis_armclang.h

uint8_t atomic_load_u8(atomic_u8_t *v)
{
	 return *v;
}

void atomic_store_u8(atomic_u8_t *v, uint8_t val)
{
	*v = val;
    __DMB();
}

bool atomic_cas_u8(atomic_u8_t *v, uint8_t old_val, uint8_t new_val)
{
	 uint32_t tmp;

    do {
        tmp = __LDREXB(v);
        if (tmp != old_val) {
            __CLREX();
            return false;
        }
    } while (__STREXB(new_val, v));

    __DMB();
    return true;
}
