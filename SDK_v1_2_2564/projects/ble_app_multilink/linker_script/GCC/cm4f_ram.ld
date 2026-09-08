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
 * @brief    GNU Linker Script for Cortex-M based device
 * @details  GNU Linker Script for Cortex-M based device
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


/*
 *-------- <<< Use Configuration Wizard in Context Menu >>> -------------------
 */
/*--------------------- Stack / Heap Configuration ----------------------------
  <h> Stack / Heap Configuration
    <o0> Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
    <o1> Heap Size (in Bytes) <0x0-0xFFFFFFFF:8>
  </h>
-----------------------------------------------------------------------------*/
__STACK_SIZE = 0x00000400;
__HEAP_SIZE  = 0x00000C00;

/*
 *-------------------- <<< end of configuration section >>> -------------------
 */
MEMORY
{
    ROM (rx)  : ORIGIN = 0x0200000, LENGTH = 256K
    RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 256K
}

INCLUDE cm4f_ram.sections

/** @} */
