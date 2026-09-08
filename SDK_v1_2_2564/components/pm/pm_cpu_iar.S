/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup System
 * @ingroup  Hal
 * @brief    System Store/Restore for enter sleep mode
 * @details  System Store/Restore for enter sleep mode
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#include "autoconf.h"

#if (CONFIG_PM)
/**
 * System sleep status:
 *
 * [0:31]  wakeup resume address
 * */
#define __SYSTEM_CPU_STATUS     0x400E00F0
/* SCR */
#define __SYSTEM_CPU_SCR        0xE000ED10

    EXTERN pm_cpu_context

    RSEG    .textrw:CODE:REORDER:NOROOT(2)
    thumb

    PUBLIC pm_cpu_context_store

pm_cpu_context_store
    push {r0-r12, lr}

    /* store CPU registers */
    ldr r0, =pm_cpu_context
    mrs r1, control
    str r1, [r0, #0]
    /* store MSP & PSP register */
    mrs r1, msp
    mrs r2, psp
    strd r1, r2, [r0, #4]

    /* save resume address */
    ldr r0, =__SYSTEM_CPU_STATUS
    ldr r1, =__SYSTEM_RESUME
    str r1, [r0]

    /* set deep sleep flag */
    Ldr r0, =__SYSTEM_CPU_SCR
    mov r1, #4
    str r1, [r0]

    wfi

__SYSTEM_RESUME:
    /* clear resume address */
    ldr r1, =__SYSTEM_CPU_STATUS
    mov r0, #0
    str r0, [r1]

    /* clear deep sleep flag */
    ldr r1, =__SYSTEM_CPU_SCR
    str r0, [r1]

    /* restore CPU registers */
    ldr r0, =pm_cpu_context
    ldr r1, [r0]
    msr control, r1
    /* restore MSP & PSP register */
    ldrd r1, r2, [r0, #4]
    msr msp, r1
    msr psp, r2

__SYSTEM_EXIT:
    pop {r0-r12, pc}

    END

#endif  /* (CONFIG_PM) */
