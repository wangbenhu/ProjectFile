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
 * @brief    IAR Linker Script for Cortex-M based device
 * @details  IAR Linker Script for Cortex-M based device
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

;
; The modules in this file are included in the libraries, and may be replaced
; by any user-defined modules that define the PUBLIC symbol _program_start or
; a user defined start symbol.
; To override the cstartup defined in the library, simply add your modified
; version to the workbench project.
;
; The vector table is normally located at address 0.
; When debugging in RAM, it can be located in RAM, aligned to at least 2^6.
; The name "__vector_table" has special meaning for C-SPY:
; it is where the SP start value is found, and the NVIC vector
; table register (VTOR) is initialized to this address if != 0.
;
; Cortex-M version
;

                MODULE   ?cstartup

                ;; Forward declaration of sections.
                SECTION  CSTACK:DATA:NOROOT(3)

                SECTION  .intvec:CODE:NOROOT(2)

                EXTERN   __iar_program_start
                EXTERN   SystemInit
                EXTERN   SystemInitPost

                PUBLIC   __vector_table
                PUBLIC   __Vectors
                PUBLIC   __Vectors_End
                PUBLIC   __Vectors_Size

                DATA

__vector_table
                DCD    sfe(CSTACK)                       ;     Top of Stack
                DCD    Reset_Handler                     ;     Reset Handler
                DCD    NMI_Handler                       ; -14 NMI Handler
                DCD    HardFault_Handler                 ; -13 Hard Fault Handler
                DCD    MemManage_Handler                 ; -12 MPU Fault Handler
                DCD    BusFault_Handler                  ; -11 Bus Fault Handler
                DCD    UsageFault_Handler                ; -10 Usage Fault Handler
                DCD    0                                 ;     Reserved */
                DCD    0                                 ;     Reserved */
                DCD    0                                 ;     Reserved */
                DCD    0                                 ;     Reserved */
                DCD    SVC_Handler                       ;  -5 SVCall Handler
                DCD    DebugMon_Handler                  ;  -4 Debug Monitor Handler
                DCD    0                                 ;     Reserved */
                DCD    PendSV_Handler                    ;  -2 PendSV Handler
                DCD    SysTick_Handler                   ;  -1 SysTick Handler

                /* Interrupts */
                DCD    BLE_IRQHandler                    ; 0
                DCD    BLE_WAKEUP_IRQHandler             ; 1
                DCD    GPDMA_IRQHandler                  ; 2
                DCD    PMU_PIN_WAKEUP_IRQHandler         ; 3
                DCD    TIM_COMB_IRQHandler               ; 4
                DCD    OM24G_RF_IRQHandler               ; 5
                DCD    PMU_POF_IRQHandler                ; 6
                DCD    PMU_TIMER_IRQHandler              ; 7
                DCD    WDT_IRQHandler                    ; 8
                DCD    UART1_IRQHandler                  ; 9
                DCD    EFUSE_IRQHandler                  ; 10
                DCD    GPIO_IRQHandler                   ; 11
                DCD    GPADC_IRQHandler                  ; 12
                DCD    I2C0_IRQHandler                   ; 13
                DCD    SF0_IRQHandler                    ; 14
                DCD    SOFT0_IRQHandler                  ; 15
                DCD    SOFT1_IRQHandler                  ; 16
                DCD    SOFT2_IRQHandler                  ; 17
                DCD    SOFT3_IRQHandler                  ; 18
                DCD    SOFT4_IRQHandler                  ; 19
                DCD    SOFT5_IRQHandler                  ; 20
                DCD    SOFT6_IRQHandler                  ; 21
                DCD    SOFT7_IRQHandler                  ; 22
                DCD    0x6627                            ; 23
                DCD    CRY32M_RDY_IRQHandler             ; 24
                DCD    UART0_IRQHandler                  ; 25
                DCD    SPI0_IRQHandler                   ; 26
                DCD    CC_INTR_IRQHandler                ; 27
                DCD    TIM0_IRQHandler                   ; 28
                DCD    TIM1_IRQHandler                   ; 29
                DCD    TIM2_IRQHandler                   ; 30
                DCD    SPI1_IRQHandler                   ; 31
                DCD    RTC_SECOND_IRQHandler             ; 32
                DCD    RTC_ALARM_IRQHandler              ; 33
                DCD    LPTIM_IRQHandler                  ; 34
                DCD    CRY32M_DECT_IRQHandler            ; 35
                DCD    I2S_IRQHandler                    ; 36
                DCD    UART2_IRQHandler                  ; 37
                DCD    OM24G_TIM_WAKEUP_IRQHandler       ; 38
                DCD    RGB_IRQHandler                    ; 39
                DCD    I2C1_IRQHandler                   ; 40
                DCD    SHA256_IRQHandler                 ; 41
                DCD    USB_IRQHandler                    ; 42
                DCD    USB_BUSACT_IRQHandler             ; 43
                DCD    USB_DMA_IRQHandler                ; 44
                DCD    IRTX_IRQHandler                   ; 45
                DCD    QDEC_IRQHandler                   ; 46
                DCD    OSPI1_IRQHandler                  ; 47
                DCD    LCD_IRQHandler                    ; 48
                DCD    AUDIO_IRQHandler                  ; 49

__Vectors_End

__Vectors       EQU      __vector_table
__Vectors_Size  EQU      __Vectors_End - __Vectors


                THUMB

; Reset Handler
                PUBWEAK  Reset_Handler
                SECTION  .text:CODE:REORDER:NOROOT(2)
Reset_Handler
                LDR R0, =SystemInit
                BLX R0
                LDR R0, =__iar_program_start
                BX  R0
; __low_level_init will auto called by IAR cstartup
                EXTERN  __iar_data_init3
                PUBLIC  __low_level_init
__low_level_init
                PUSH {LR}
                ; call IAR table copy function.
                BL  __iar_data_init3
                BL  SystemInitPost
                MOV R0, #0   ; return 0
                POP  {PC}


; NMI_Handler_C & fault_handler_c_pre & fault_handler_c
                PUBWEAK  NMI_Handler_C
                PUBWEAK  fault_handler_c_pre
                PUBWEAK  fault_handler_c
fault_handler_c
NMI_Handler_C
                B    .
fault_handler_c_pre
                BX   LR

; NMI_Handler
                PUBWEAK NMI_Handler
NMI_Handler
                TST   LR, #4
                ITE   EQ
                MRSEQ R1, MSP
                MRSNE R1, PSP
                MOV   R0, LR
                B     NMI_Handler_C

; HardFault_Handler
                PUBWEAK HardFault_Handler
                PUBWEAK MemManage_Handler
                PUBWEAK BusFault_Handler
                PUBWEAK UsageFault_Handler

HardFault_Handler
MemManage_Handler
BusFault_Handler
UsageFault_Handler
                TST   LR, #4
                ITE   EQ
                MRSEQ R1, MSP
                MRSNE R1, PSP
                MOV   R0, LR
                PUSH  {R0, R1}
                BL    fault_handler_c_pre
                POP   {R0, R1}
                B     fault_handler_c

                PUBWEAK SVC_Handler
                PUBWEAK DebugMon_Handler
                PUBWEAK PendSV_Handler
                PUBWEAK SysTick_Handler

                PUBWEAK BLE_IRQHandler
                PUBWEAK BLE_WAKEUP_IRQHandler
                PUBWEAK GPDMA_IRQHandler
                PUBWEAK PMU_PIN_WAKEUP_IRQHandler
                PUBWEAK TIM_COMB_IRQHandler
                PUBWEAK OM24G_RF_IRQHandler
                PUBWEAK PMU_POF_IRQHandler
                PUBWEAK PMU_TIMER_IRQHandler
                PUBWEAK WDT_IRQHandler
                PUBWEAK UART1_IRQHandler
                PUBWEAK EFUSE_IRQHandler
                PUBWEAK GPIO_IRQHandler
                PUBWEAK GPADC_IRQHandler
                PUBWEAK I2C0_IRQHandler
                PUBWEAK SF0_IRQHandler
                PUBWEAK SOFT0_IRQHandler
                PUBWEAK SOFT1_IRQHandler
                PUBWEAK SOFT2_IRQHandler
                PUBWEAK SOFT3_IRQHandler
                PUBWEAK SOFT4_IRQHandler
                PUBWEAK SOFT5_IRQHandler
                PUBWEAK SOFT6_IRQHandler
                PUBWEAK SOFT7_IRQHandler
                PUBWEAK VTRACK_IRQHandler
                PUBWEAK CRY32M_RDY_IRQHandler
                PUBWEAK UART0_IRQHandler
                PUBWEAK SPI0_IRQHandler
                PUBWEAK CC_INTR_IRQHandler
                PUBWEAK TIM0_IRQHandler
                PUBWEAK TIM1_IRQHandler
                PUBWEAK TIM2_IRQHandler
                PUBWEAK SPI1_IRQHandler
                PUBWEAK RTC_SECOND_IRQHandler
                PUBWEAK RTC_ALARM_IRQHandler
                PUBWEAK LPTIM_IRQHandler
                PUBWEAK CRY32M_DECT_IRQHandler
                PUBWEAK I2S_IRQHandler
                PUBWEAK UART2_IRQHandler
                PUBWEAK OM24G_TIM_WAKEUP_IRQHandler
                PUBWEAK RGB_IRQHandler
                PUBWEAK I2C1_IRQHandler
                PUBWEAK SHA256_IRQHandler
                PUBWEAK USB_IRQHandler
                PUBWEAK USB_BUSACT_IRQHandler
                PUBWEAK USB_DMA_IRQHandler
                PUBWEAK IRTX_IRQHandler
                PUBWEAK QDEC_IRQHandler
                PUBWEAK OSPI1_IRQHandler
                PUBWEAK LCD_IRQHandler
                PUBWEAK AUDIO_IRQHandler

                SECTION .text:CODE:REORDER:NOROOT(1)

HardFault_Handler
MemManage_Handler
BusFault_Handler
UsageFault_Handler
SVC_Handler
DebugMon_Handler
PendSV_Handler
SysTick_Handler

BLE_IRQHandler
BLE_WAKEUP_IRQHandler
GPDMA_IRQHandler
PMU_PIN_WAKEUP_IRQHandler
TIM_COMB_IRQHandler
OM24G_RF_IRQHandler
PMU_POF_IRQHandler
PMU_TIMER_IRQHandler
WDT_IRQHandler
UART1_IRQHandler
EFUSE_IRQHandler
GPIO_IRQHandler
GPADC_IRQHandler
I2C0_IRQHandler
SF0_IRQHandler
SOFT0_IRQHandler
SOFT1_IRQHandler
SOFT2_IRQHandler
SOFT3_IRQHandler
SOFT4_IRQHandler
SOFT5_IRQHandler
SOFT6_IRQHandler
SOFT7_IRQHandler
VTRACK_IRQHandler
CRY32M_RDY_IRQHandler
UART0_IRQHandler
SPI0_IRQHandler
CC_INTR_IRQHandler
TIM0_IRQHandler
TIM1_IRQHandler
TIM2_IRQHandler
SPI1_IRQHandler
RTC_SECOND_IRQHandler
RTC_ALARM_IRQHandler
LPTIM_IRQHandler
CRY32M_DECT_IRQHandler
I2S_IRQHandler
UART2_IRQHandler
OM24G_TIM_WAKEUP_IRQHandler
RGB_IRQHandler
I2C1_IRQHandler
SHA256_IRQHandler
USB_IRQHandler
USB_BUSACT_IRQHandler
USB_DMA_IRQHandler
IRTX_IRQHandler
QDEC_IRQHandler
OSPI1_IRQHandler
LCD_IRQHandler
AUDIO_IRQHandler
                B      .

                END
