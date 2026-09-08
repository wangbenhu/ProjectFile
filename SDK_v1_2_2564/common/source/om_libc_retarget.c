/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/*
 * NOTE: When GNU Arm compiler version greater equal than *11.3.Rel1*, there is a linker issue that
 * some system calls are not implemented, such as _close, _fstat and _getpid etc. So add this file
 * including stub functions of system calls to avoid the above linker issue.
 */

#include <stddef.h>
#include <stdint.h>

__attribute__((weak))
void _close(void)
{
}

__attribute__((weak))
void _fstat(void)
{
}

__attribute__((weak))
void _getpid(void)
{
}

__attribute__((weak))
void _isatty(void)
{
}

__attribute__((weak))
void _kill(void)
{
}

__attribute__((weak))
void _lseek(void)
{
}

__attribute__((weak))
void _read(void)
{
}

__attribute__((weak))
void _write(void)
{
}

__attribute__((weak))
size_t __write(int handle, const unsigned char *buf, size_t buf_size)
{
    __attribute__((weak)) void om_putchar(char character);

    for (size_t i=0; i<buf_size; i++) {
        om_putchar(buf[i]);
    }

    return buf_size;
}

#if defined(__GNUC__)

#include <assert.h>
#include "om_device.h"

__attribute__((weak))
void _exit(int status)
{
    while(1);
}

__attribute__((weak))
void __assert_func (const char *fil, int line, const char *func, const char *e)
{
    while(1);
}

__attribute__((weak))
char* _sbrk(int incr)   /* caddr_t is defined char* in <sys/types.h> */
{
    #if defined (__ARMCC_VERSION)
    extern unsigned Image$$ARM_LIB_STACK$$Base;
    uint8_t * const stack_base = (uint8_t *)&Image$$ARM_LIB_STACK$$Base;
    #else
    extern unsigned __StackLimit;
    uint8_t * const stack_base = (uint8_t *) &__StackLimit;
    #endif
    uint8_t * const stack_limit = (uint8_t *)__get_MSP();

    static uint8_t *psbrk = stack_base;
    uint8_t *       prev = psbrk;
    uint8_t *       next = psbrk + incr;

    // Not use OM_ASSERT, it may trigger next this fault
    while(next >= stack_limit);
    psbrk = next;

    return (char *)prev;
}

#endif /* defined(__GNUC__) */


/** @} */
