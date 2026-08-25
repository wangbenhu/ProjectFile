/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup DOC DOC
 * @ingroup  DOCUMENT
 * @brief    fault handle
 * @details  fault handle
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __FAULT_HANDLE_H
#define __FAULT_HANDLE_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>

#ifdef  __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * TYPEDEFS
 */
typedef enum {
    FAULT_ID_LOW_VOLTAGE,       /* low voltage detect */
    FAULT_ID_WDT,               /* WDT interrupt */
    FAULT_ID_HARD_FAULT,        /* hard fault */
    FAULT_ID_USER,              /* user check fault */

    FAULT_ID_MAX  = 0xFFFFFFFFU,
} fault_id_t;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Fault Callback. The function will be called when system fault.
 *        Hardfault（MemManage_Handler/BusFault_Handler/UsageFault_Handler）,
 *        OM_ASSERT, WDT（NMI）Handler，call it. OM_ASSERT call BKPT instument,
 *        will trig DebugException_Handler, then Hardfault.
 *        User may call this function for self-check failed, It need User added.
 *
 *        The processing as follows:
 *        1. The disable global interrupt
 *        2. keep alive for WDT
 *        3. saved RTC second counter
 *        4. store fault context. call fault_context_store()
 *        5. reset system
 *
 *        Context is different by the fault_id. As follows:


 *        FAULT_ID_LOW_VOLTAGE:
 *        FAULT_ID_WDT:
 *        FAULT_ID_HARD_FAULT:
 *                             context is pointer to stacked regs, as follows:
 *                             struct stacked_reg_tag {
 *                                  uint32_t stacked_r0;
 *                                  uint32_t stacked_r1;
 *                                  uint32_t stacked_r2;
 *                                  uint32_t stacked_r3;
 *                                  uint32_t stacked_r12;
 *                                  uint32_t stacked_lr;
 *                                  uint32_t stacked_pc;
 *                              } stacked_regs;
 *                              and context_len is fixed 28 Byte;
 *                              param is exception return code(EXC_RETURN).
 *
 * @param fault_id      fault id
 * @param context       the context for the fault_id
 * @param context_len   valid length in bytes for context
 * @param param         extra parameter for fault_id
 *
 *******************************************************************************
 **/
extern void fault_callback(fault_id_t fault_id, uint8_t *context, uint32_t context_len, uint32_t param);

/**
 *******************************************************************************
 * @brief Fault context store. It is a weak function. The default context store is
 *        endless loop. the fault_context_store is called by fault_callback.
 *        User may define own callback in user/project file, not modify it in the
 *        isr driver. the parameter is the same as fault_callback function.
 *
 *        Context is different by the fault_id. As follows:
 *        FAULT_ID_LOW_VOLTAGE:
 *        FAULT_ID_WDT:
 *        FAULT_ID_HARD_FAULT:
 *                              context is pointer to stacked regs, as follows:
 *                              struct stacked_reg_tag {
 *                                  uint32_t stacked_r0;
 *                                  uint32_t stacked_r1;
 *                                  uint32_t stacked_r2;
 *                                  uint32_t stacked_r3;
 *                                  uint32_t stacked_r12;
 *                                  uint32_t stacked_lr;
 *                                  uint32_t stacked_pc;
 *                              } stacked_regs;
 *                              and context_len is fixed 28 Byte;
 *                              param is exception return code(EXC_RETURN).
 *
 * @param fault_id      fault id
 * @param context       the context for the fault_id
 * @param context_len   valid length in bytes for context
 * @param param         extra parameter for fault_id
 *
 *******************************************************************************
 **/
extern void fault_context_store(fault_id_t fault_id, uint8_t *context, uint32_t context_len, uint32_t param);

#ifdef  __cplusplus
}
#endif

#endif /* __FAULT_HANDLE_H */

/** @} */
