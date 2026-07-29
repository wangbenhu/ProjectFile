/******************************************************************************
 * @file     cm33_rom.ld
 * @brief    GNU Linker Script for Cortex-M based device
 * @version  V1.0.0
 * @date     26. Feb. 2024
 ******************************************************************************/
/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/*
 *-------- <<< Use Configuration Wizard in Context Menu >>> -------------------
 */
/*--------------------- Stack Configuration ----------------------------
  <h> Stack Configuration
    <o0> Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
  </h>
-----------------------------------------------------------------------------*/
__STACK_SIZE = 0x00002000;

/*
 *-------------------- <<< end of configuration section >>> -------------------
 */
MEMORY
{
    ROM (rx)  : ORIGIN = 0x00100000, LENGTH = 256K
    RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 256K
}

/* Define the entry point of the output file. */
ENTRY(Reset_Handler)

SECTIONS
{
    .vectors :
    {
        . = ALIGN(512);
        KEEP(*(.vectors))
    } > ROM

    .text :
    {
        __code_start__ = .;
        *(.text*)
        *(.ram_code)
        *(.ram_rodata)
        *(.rodata*)
        . = ALIGN(32);
        __code_end__ = .;
    } > ROM

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > ROM

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > ROM
    __exidx_end = .;

    .copy.table :
    {
        . = ALIGN(4);
        __copy_table_start__ = .;
        /* ram data */
        LONG (LOADADDR(.ram_data))
        LONG (__ram_data_start__)
        LONG ((__ram_data_end__ - __ram_data_start__ + 3) >> 2)

        __copy_table_end__ = .;
    } > ROM

    .zero.table :
    {
        . = ALIGN(4);
        __zero_table_start__ = .;
        /* ram bss */
        LONG (__ram_bss_start__)
        LONG ((__ram_bss_end__ - __ram_bss_start__ + 3) >> 2)
        __zero_table_end__ = .;
    } > ROM

    /**
    * Location counter can end up 2byte aligned with narrow Thumb code but
    * __etext is assumed by startup code to be the LMA of a section in RAM
    * which must be 4byte aligned
    */
    __etext = ALIGN (4);

    .ram_data : ALIGN(4)
    {
        . = ALIGN(4);
        __ram_data_start__ = .;
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        /* All data end */
        __ram_data_end__ = .;
    } > RAM AT > ROM

    .ram_bss :
    {
        . = ALIGN(4);
        __ram_bss_start__ = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        __ram_bss_end__ = .;
    } > RAM

    .ram_data_non_init (NOLOAD):
    {
        *(.ram_data_non_init)
    } > RAM

    .stack (NOLOAD):
    {
        . = ALIGN(8);
        __StackLimit = .;
        . = . + __STACK_SIZE - 8;
        . = ALIGN(8);
        __StackTop = .;
    } > RAM

    PROVIDE(__stack = __StackTop);
}
