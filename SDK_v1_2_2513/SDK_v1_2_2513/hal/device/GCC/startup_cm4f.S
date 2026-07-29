/**************************************************************************//**
 * @file     startup_ARMCM33.S
 * @brief    CMSIS-Core(M) Device Startup File for Cortex-M33 Device
 * @version  V2.0.1
 * @date     23. July 2019
 ******************************************************************************/
/*
 * Copyright (c) 2009-2019 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

                .syntax  unified
                .arch    armv7-m
                .eabi_attribute Tag_ABI_align_preserved, 1

                #if defined(__ARMCC_VERSION)
                #ifndef __INITIAL_SP
                #define __INITIAL_SP     Image$$ARM_LIB_STACK$$ZI$$Limit
                #endif
                .section RESET, "a"
                #elif defined(__GNUC__)
                #ifndef __INITIAL_SP
                #define __INITIAL_SP     __StackTop
                #endif
                .section .vectors, "a"
                #endif

                .align   2
                .weak    __Vectors
                .weak    __Vectors_End
                .weak    __Vectors_Size

__Vectors:
                .long    __INITIAL_SP                       /*     Top of Stack */
                .long    Reset_Handler                      /*     Reset Handler */
                .long    NMI_Handler                        /* -14 NMI Handler */
                .long    HardFault_Handler                  /* -13 Hard Fault Handler */
                .long    MemManage_Handler                  /* -12 MPU Fault Handler */
                .long    BusFault_Handler                   /* -11 Bus Fault Handler */
                .long    UsageFault_Handler                 /* -10 Usage Fault Handler */
                .long    0                                  /*     Reserved */
                .long    0                                  /*     Reserved */
                .long    0                                  /*     Reserved */
                .long    0                                  /*     Reserved */
                .long    SVC_Handler                        /*  -5 SVCall Handler */
                .long    DebugMon_Handler                   /*  -4 Debug Monitor Handler */
                .long    0                                  /*     Reserved */
                .long    PendSV_Handler                     /*  -2 PendSV Handler */
                .long    SysTick_Handler                    /*  -1 SysTick Handler */

                /* Interrupts */
                .long    BLE_IRQHandler                     /*  0   */
                .long    BLE_WAKEUP_IRQHandler              /*  1   */
                .long    GPDMA_IRQHandler                   /*  2   */
                .long    PMU_PIN_WAKEUP_IRQHandler          /*  3   */
                .long    TIM_COMB_IRQHandler                /*  4   */
                .long    OM24G_RF_IRQHandler                /*  5   */
                .long    PMU_POF_IRQHandler                 /*  6   */
                .long    PMU_TIMER_IRQHandler               /*  7   */
                .long    WDT_IRQHandler                     /*  8   */
                .long    UART1_IRQHandler                   /*  9   */
                .long    EFUSE_IRQHandler                   /* 10   */
                .long    GPIO_IRQHandler                    /* 11   */
                .long    GPADC_IRQHandler                   /* 12   */
                .long    I2C0_IRQHandler                    /* 13   */
                .long    SF0_IRQHandler                     /* 14   */
                .long    SOFT0_IRQHandler                   /* 15   */
                .long    SOFT1_IRQHandler                   /* 16   */
                .long    SOFT2_IRQHandler                   /* 17   */
                .long    SOFT3_IRQHandler                   /* 18   */
                .long    SOFT4_IRQHandler                   /* 19   */
                .long    SOFT5_IRQHandler                   /* 20   */
                .long    SOFT6_IRQHandler                   /* 21   */
                .long    SOFT7_IRQHandler                   /* 22   */
                .long    0x6627                             /* 23   */
                .long    CRY32M_RDY_IRQHandler              /* 24   */
                .long    UART0_IRQHandler                   /* 25   */
                .long    SPI0_IRQHandler                    /* 26   */
                .long    CC_INTR_IRQHandler                 /* 27   */
                .long    TIM0_IRQHandler                    /* 28   */
                .long    TIM1_IRQHandler                    /* 29   */
                .long    TIM2_IRQHandler                    /* 30   */
                .long    SPI1_IRQHandler                    /* 31   */
                .long    RTC_SECOND_IRQHandler              /* 32   */
                .long    RTC_ALARM_IRQHandler               /* 33   */
                .long    LPTIM_IRQHandler                   /* 34   */
                .long    CRY32M_DECT_IRQHandler             /* 35   */
                .long    I2S_IRQHandler                     /* 36   */
                .long    UART2_IRQHandler                   /* 37   */
                .long    OM24G_TIM_WAKEUP_IRQHandler        /* 38   */
                .long    RGB_IRQHandler                     /* 39   */
                .long    I2C1_IRQHandler                    /* 40   */
                .long    SHA256_IRQHandler                  /* 41   */
                .long    USB_IRQHandler                     /* 42   */
                .long    USB_BUSACT_IRQHandler              /* 43   */
                .long    USB_DMA_IRQHandler                 /* 44   */
                .long    IRTX_IRQHandler                    /* 45   */
                .long    QDEC_IRQHandler                    /* 46   */
                .long    OSPI1_IRQHandler                   /* 47   */
                .long    LCD_IRQHandler                     /* 48   */
                .long    AUDIO_IRQHandler                   /* 49   */

__Vectors_End:
                .equ     __Vectors_Size, __Vectors_End - __Vectors
                .size    __Vectors, . - __Vectors


                .thumb
                .section .text
                .align   2

                .thumb_func
                .type    Reset_Handler, %function
                .weak    Reset_Handler
                .fnstart
Reset_Handler:
                bl       SystemInit

                #if defined(__ARMCC_VERSION)
                bl       __main
                #elif defined(__GNUC__)
                ldr      r4, =__copy_table_start__
                ldr      r5, =__copy_table_end__

.L_loop0:
                cmp      r4, r5
                bge      .L_loop0_done
                ldr      r1, [r4]              /* source address */
                ldr      r2, [r4, #4]          /* destination address */
                ldr      r3, [r4, #8]          /* word count */
                lsls     r3, r3, #2            /* byte count */

.L_loop0_0:
                subs     r3, #4                /* decrement byte count */
                ittt     ge
                ldrge    r0, [r1, r3]
                strge    r0, [r2, r3]
                bge      .L_loop0_0

                adds     r4, #12
                b        .L_loop0
.L_loop0_done:

                ldr      r3, =__zero_table_start__
                ldr      r4, =__zero_table_end__

.L_loop2:
                cmp      r3, r4
                bge      .L_loop2_done
                ldr      r1, [r3]                /* destination address */
                ldr      r2, [r3, #4]            /* word count */
                lsls     r2, r2, #2              /* byte count */
                movs     r0, 0

.L_loop2_0:
                subs     r2, #4                  /* decrement byte count */
                itt      ge
                strge    r0, [r1, r2]
                bge      .L_loop2_0

                adds     r3, #8
                b        .L_loop2
.L_loop2_done:
                bl       SystemInitPost
                bl       main
                #endif

                .fnend
                .size    Reset_Handler, . - Reset_Handler

                #if defined(__ARMCC_VERSION)
/* From __main */
                .thumb_func
                .type    $Sub$$main, %function
                .globl   $Sub$$main
                .fnstart
$Sub$$main:
                bl       SystemInitPost
                bl       $Super$$main
                .fnend
                #endif

/* The default macro is not used for NMI_Handler
 * because this results in a poor debug illusion.
 * void NMI_Handler_C(uint32_t exc_return_code, uint32_t sp)
 */
                .thumb_func
                .type    NMI_Handler_C, %function
                .weak    NMI_Handler_C
                .fnstart
NMI_Handler_C:
                b    .
                .fnend
                .size    NMI_Handler_C, . - NMI_Handler_C

                .thumb_func
                .type    NMI_Handler, %function
                .weak    NMI_Handler
                .fnstart
NMI_Handler:
                tst   lr, #4
                ite   eq
                mrseq r1, msp
                mrsne r1, psp
                mov   r0, lr
                b     NMI_Handler_C
                .fnend
                .size    NMI_Handler, . - NMI_Handler

/* The default macro is not used for HardFault_Handler
 * because this results in a poor debug illusion.
 * We can using fault_handler_c function to display Fault status:
 * void fault_handler_c(uint32_t exc_return_code, uint32_t sp)
 */
                .thumb_func
                .type    fault_handler_c, %function
                .weak    fault_handler_c
                .fnstart
fault_handler_c:
                b    .
                .fnend
                .size    fault_handler_c, . - fault_handler_c

 /* We can using fault_handler_c_pre function to store regsiters before display Fault status:
  * void fault_handler_c_pre(uint32_t exc_return_code, uint32_t sp)
  */
                .thumb_func
                .type    fault_handler_c_pre, %function
                .weak    fault_handler_c_pre
                .fnstart
fault_handler_c_pre:
                bx   lr
                .fnend
                .size    fault_handler_c_pre, . - fault_handler_c_pre

                .thumb_func
                .type    HardFault_Handler, %function
                .weak    HardFault_Handler
                .fnstart
HardFault_Handler:
                tst   lr, #4
                ite   eq
                mrseq r1, msp
                mrsne r1, psp
                mov   r0, lr
                push  {r0, r1}
                bl    fault_handler_c_pre
                pop   {r0, r1}
                b     fault_handler_c
                .fnend
                .size    HardFault_Handler, . - HardFault_Handler

/* Macro to define default fault handlers.
 * Default handler are weak symbols.
 * They can be overwritten by real handlers.
 */
                .macro   Set_Default_Fault_Handler  Handler_Name
                .weak    \Handler_Name
                .set     \Handler_Name, HardFault_Handler
                .endm

                .thumb_func
                .type    Default_Handler, %function
                .weak    Default_Handler
                .fnstart
Default_Handler:
                b        .
                .fnend
                .size    Default_Handler, . - Default_Handler

/* Macro to define default exception/interrupt handlers.
 * Default handler are weak symbols with an endless loop.
 * They can be overwritten by real handlers.
 */
                .macro   Set_Default_Handler  Handler_Name
                .weak    \Handler_Name
                .set     \Handler_Name, Default_Handler
                .endm


/* Default exception/interrupt handler */
                Set_Default_Fault_Handler  MemManage_Handler
                Set_Default_Fault_Handler  BusFault_Handler
                Set_Default_Fault_Handler  UsageFault_Handler
                Set_Default_Handler  SVC_Handler
                Set_Default_Handler  DebugMon_Handler
                Set_Default_Handler  PendSV_Handler
                Set_Default_Handler  SysTick_Handler

                Set_Default_Handler  BLE_IRQHandler
                Set_Default_Handler  BLE_WAKEUP_IRQHandler
                Set_Default_Handler  GPDMA_IRQHandler
                Set_Default_Handler  PMU_PIN_WAKEUP_IRQHandler
                Set_Default_Handler  TIM_COMB_IRQHandler
                Set_Default_Handler  OM24G_RF_IRQHandler
                Set_Default_Handler  PMU_POF_IRQHandler
                Set_Default_Handler  PMU_TIMER_IRQHandler
                Set_Default_Handler  WDT_IRQHandler
                Set_Default_Handler  UART1_IRQHandler
                Set_Default_Handler  EFUSE_IRQHandler
                Set_Default_Handler  GPIO_IRQHandler
                Set_Default_Handler  GPADC_IRQHandler
                Set_Default_Handler  I2C0_IRQHandler
                Set_Default_Handler  SF0_IRQHandler
                Set_Default_Handler  SOFT0_IRQHandler
                Set_Default_Handler  SOFT1_IRQHandler
                Set_Default_Handler  SOFT2_IRQHandler
                Set_Default_Handler  SOFT3_IRQHandler
                Set_Default_Handler  SOFT4_IRQHandler
                Set_Default_Handler  SOFT5_IRQHandler
                Set_Default_Handler  SOFT6_IRQHandler
                Set_Default_Handler  SOFT7_IRQHandler
                Set_Default_Handler  CRY32M_RDY_IRQHandler
                Set_Default_Handler  UART0_IRQHandler
                Set_Default_Handler  SPI0_IRQHandler
                Set_Default_Handler  CC_INTR_IRQHandler
                Set_Default_Handler  TIM0_IRQHandler
                Set_Default_Handler  TIM1_IRQHandler
                Set_Default_Handler  TIM2_IRQHandler
                Set_Default_Handler  SPI1_IRQHandler
                Set_Default_Handler  RTC_SECOND_IRQHandler
                Set_Default_Handler  RTC_ALARM_IRQHandler
                Set_Default_Handler  LPTIM_IRQHandler
                Set_Default_Handler  CRY32M_DECT_IRQHandler
                Set_Default_Handler  I2S_IRQHandler
                Set_Default_Handler  UART2_IRQHandler
                Set_Default_Handler  OM24G_TIM_WAKEUP_IRQHandler
                Set_Default_Handler  RGB_IRQHandler
                Set_Default_Handler  I2C1_IRQHandler
                Set_Default_Handler  SHA256_IRQHandler
                Set_Default_Handler  USB_IRQHandler
                Set_Default_Handler  USB_BUSACT_IRQHandler
                Set_Default_Handler  USB_DMA_IRQHandler
                Set_Default_Handler  IRTX_IRQHandler
                Set_Default_Handler  QDEC_IRQHandler
                Set_Default_Handler  OSPI1_IRQHandler
                Set_Default_Handler  LCD_IRQHandler
                Set_Default_Handler  AUDIO_IRQHandler

                .end
