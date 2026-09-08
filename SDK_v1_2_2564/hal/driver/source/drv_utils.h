/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup UTILS UTILS
 * @ingroup  DRIVER
 * @brief    UTILS driver
 * @details  UTILS driver apis and typedefs header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_UTILS_H
#define __DRV_UTILS_H


/*******************************************************************************
 * INCLUDES
 */
#include "cmsis_compiler.h"

#ifdef  __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
/**
 *******************************************************************************
 *  @brief Align val on the multiple of 2 equal or nearest higher.
 *
 *  @param[in]   val    Value to align.
 *
 *  @return             Value aligned, range in [2, 4, 8, 16, ... 2**30]
 *******************************************************************************
 */
#define DRV_ALIGN_HI(val, align)   ((((unsigned)(val) + (align) - 1U)) & (~((align) - 1U)))

/**
 *******************************************************************************
 *  @brief Align val on the multiple of 2 equal or nearest lower.
 *
 *  @param[in]   val    Value to align.
 *
 *  @return             Value aligned, range in [2, 4, 8, 16, ... 2**30]
 *******************************************************************************
 */
#define DRV_ALIGN_LO(val, align)   ((unsigned)(val) & (~((align) - 1U)))

// check val is align to 2**n (n range in [0, 32], align 0->2**32)
/**
 *******************************************************************************
 *  @brief Check val is align to the power of 2
 *
 *  @param[in]   val     Value to align.
 *  @param[in]   align   Align value, align range in [2, 4, 8, ...2**31]
 *
 *  @return              is aligned.
 *******************************************************************************
 */
#define DRV_IS_ALIGN(val, align)   (!((unsigned)(val) & ((unsigned)(align) - 1U)))


// Mask value pair
#define MASK_1REG(name1, value1) \
    name1##_MASK, \
    ((((uint32_t)(value1)) << (name1##_POS)) & (name1##_MASK))

#define MASK_2REG(name1, value1, \
                  name2, value2) \
    name1##_MASK | \
    name2##_MASK,  \
    (((((uint32_t)(value1)) << (name1##_POS)) & (name1##_MASK))) | \
    (((((uint32_t)(value2)) << (name2##_POS)) & (name2##_MASK)))

#define MASK_3REG(name1, value1, \
                  name2, value2, \
                  name3, value3) \
    name1##_MASK | \
    name2##_MASK | \
    name3##_MASK,  \
    (((((uint32_t)(value1)) << (name1##_POS)) & (name1##_MASK))) |   \
    (((((uint32_t)(value2)) << (name2##_POS)) & (name2##_MASK))) |   \
    (((((uint32_t)(value3)) << (name3##_POS)) & (name3##_MASK)))

#define MASK_4REG(name1, value1, \
                  name2, value2, \
                  name3, value3, \
                  name4, value4) \
    name1##_MASK | \
    name2##_MASK | \
    name3##_MASK | \
    name4##_MASK,  \
    (((((uint32_t)(value1)) << (name1##_POS)) & (name1##_MASK))) | \
    (((((uint32_t)(value2)) << (name2##_POS)) & (name2##_MASK))) | \
    (((((uint32_t)(value3)) << (name3##_POS)) & (name3##_MASK))) | \
    (((((uint32_t)(value4)) << (name4##_POS)) & (name4##_MASK)))

#define MASK_5REG(name1, value1, \
                  name2, value2, \
                  name3, value3, \
                  name4, value4, \
                  name5, value5) \
    name1##_MASK | \
    name2##_MASK | \
    name3##_MASK | \
    name4##_MASK | \
    name5##_MASK,  \
    (((((uint32_t)(value1)) << (name1##_POS)) & (name1##_MASK))) | \
    (((((uint32_t)(value2)) << (name2##_POS)) & (name2##_MASK))) | \
    (((((uint32_t)(value3)) << (name3##_POS)) & (name3##_MASK))) | \
    (((((uint32_t)(value4)) << (name4##_POS)) & (name4##_MASK))) | \
    (((((uint32_t)(value5)) << (name5##_POS)) & (name5##_MASK)))

#define MASK_6REG(name1, value1, \
                  name2, value2, \
                  name3, value3, \
                  name4, value4, \
                  name5, value5, \
                  name6, value6) \
    name1##_MASK | \
    name2##_MASK | \
    name3##_MASK | \
    name4##_MASK | \
    name5##_MASK | \
    name6##_MASK,  \
    (((((uint32_t)(value1)) << (name1##_POS)) & (name1##_MASK))) | \
    (((((uint32_t)(value2)) << (name2##_POS)) & (name2##_MASK))) | \
    (((((uint32_t)(value3)) << (name3##_POS)) & (name3##_MASK))) | \
    (((((uint32_t)(value4)) << (name4##_POS)) & (name4##_MASK))) | \
    (((((uint32_t)(value5)) << (name5##_POS)) & (name5##_MASK))) | \
    (((((uint32_t)(value6)) << (name6##_POS)) & (name6##_MASK)))

#define MASK_7REG(name1, value1, \
                  name2, value2, \
                  name3, value3, \
                  name4, value4, \
                  name5, value5, \
                  name6, value6, \
                  name7, value7) \
    name1##_MASK | \
    name2##_MASK | \
    name3##_MASK | \
    name4##_MASK | \
    name5##_MASK | \
    name6##_MASK | \
    name7##_MASK,  \
    (((((uint32_t)(value1)) << (name1##_POS)) & (name1##_MASK))) | \
    (((((uint32_t)(value2)) << (name2##_POS)) & (name2##_MASK))) | \
    (((((uint32_t)(value3)) << (name3##_POS)) & (name3##_MASK))) | \
    (((((uint32_t)(value4)) << (name4##_POS)) & (name4##_MASK))) | \
    (((((uint32_t)(value5)) << (name5##_POS)) & (name5##_MASK))) | \
    (((((uint32_t)(value6)) << (name6##_POS)) & (name6##_MASK))) | \
    (((((uint32_t)(value7)) << (name7##_POS)) & (name7##_MASK)))

#define MASK_8REG(name1, value1, \
                  name2, value2, \
                  name3, value3, \
                  name4, value4, \
                  name5, value5, \
                  name6, value6, \
                  name7, value7, \
                  name8, value8) \
    name1##_MASK | \
    name2##_MASK | \
    name3##_MASK | \
    name4##_MASK | \
    name5##_MASK | \
    name6##_MASK | \
    name7##_MASK | \
    name8##_MASK,  \
    (((((uint32_t)(value1)) << (name1##_POS)) & (name1##_MASK))) | \
    (((((uint32_t)(value2)) << (name2##_POS)) & (name2##_MASK))) | \
    (((((uint32_t)(value3)) << (name3##_POS)) & (name3##_MASK))) | \
    (((((uint32_t)(value4)) << (name4##_POS)) & (name4##_MASK))) | \
    (((((uint32_t)(value5)) << (name5##_POS)) & (name5##_MASK))) | \
    (((((uint32_t)(value6)) << (name6##_POS)) & (name6##_MASK))) | \
    (((((uint32_t)(value7)) << (name7##_POS)) & (name7##_MASK))) | \
    (((((uint32_t)(value8)) << (name8##_POS)) & (name8##_MASK)))

#define MASK_9REG(name1, value1, \
                  name2, value2, \
                  name3, value3, \
                  name4, value4, \
                  name5, value5, \
                  name6, value6, \
                  name7, value7, \
                  name8, value8, \
                  name9, value9) \
    name1##_MASK | \
    name2##_MASK | \
    name3##_MASK | \
    name4##_MASK | \
    name5##_MASK | \
    name6##_MASK | \
    name7##_MASK | \
    name8##_MASK | \
    name9##_MASK,  \
    (((((uint32_t)(value1)) << (name1##_POS)) & (name1##_MASK))) | \
    (((((uint32_t)(value2)) << (name2##_POS)) & (name2##_MASK))) | \
    (((((uint32_t)(value3)) << (name3##_POS)) & (name3##_MASK))) | \
    (((((uint32_t)(value4)) << (name4##_POS)) & (name4##_MASK))) | \
    (((((uint32_t)(value5)) << (name5##_POS)) & (name5##_MASK))) | \
    (((((uint32_t)(value6)) << (name6##_POS)) & (name6##_MASK))) | \
    (((((uint32_t)(value7)) << (name7##_POS)) & (name7##_MASK))) | \
    (((((uint32_t)(value8)) << (name8##_POS)) & (name8##_MASK))) | \
    (((((uint32_t)(value9)) << (name9##_POS)) & (name9##_MASK)))

#define MASK_10REG(name1,  value1,  \
                   name2,  value2,  \
                   name3,  value3,  \
                   name4,  value4,  \
                   name5,  value5,  \
                   name6,  value6,  \
                   name7,  value7,  \
                   name8,  value8,  \
                   name9,  value9,  \
                   name10, value10) \
    name1##_MASK  | \
    name2##_MASK  | \
    name3##_MASK  | \
    name4##_MASK  | \
    name5##_MASK  | \
    name6##_MASK  | \
    name7##_MASK  | \
    name8##_MASK  | \
    name9##_MASK  | \
    name10##_MASK,  \
    (((((uint32_t)(value1)) << (name1##_POS)) & (name1##_MASK))) | \
    (((((uint32_t)(value2)) << (name2##_POS)) & (name2##_MASK))) | \
    (((((uint32_t)(value3)) << (name3##_POS)) & (name3##_MASK))) | \
    (((((uint32_t)(value4)) << (name4##_POS)) & (name4##_MASK))) | \
    (((((uint32_t)(value5)) << (name5##_POS)) & (name5##_MASK))) | \
    (((((uint32_t)(value6)) << (name6##_POS)) & (name6##_MASK))) | \
    (((((uint32_t)(value7)) << (name7##_POS)) & (name7##_MASK))) | \
    (((((uint32_t)(value8)) << (name8##_POS)) & (name8##_MASK))) | \
    (((((uint32_t)(value9)) << (name9##_POS)) & (name9##_MASK))) | \
    (((((uint32_t)(value10)) << (name10##_POS)) & (name10##_MASK)))

#define MASK_11REG(name1, value1,   \
                   name2, value2,   \
                   name3, value3,   \
                   name4, value4,   \
                   name5, value5,   \
                   name6, value6,   \
                   name7, value7,   \
                   name8, value8,   \
                   name9, value9,   \
                   name10, value10, \
                   name11, value11) \
    name1##_MASK  | \
    name2##_MASK  | \
    name3##_MASK  | \
    name4##_MASK  | \
    name5##_MASK  | \
    name6##_MASK  | \
    name7##_MASK  | \
    name8##_MASK  | \
    name9##_MASK  | \
    name10##_MASK | \
    name11##_MASK,  \
    (((((uint32_t)(value1)) << (name1##_POS)) & (name1##_MASK)))    | \
    (((((uint32_t)(value2)) << (name2##_POS)) & (name2##_MASK)))    | \
    (((((uint32_t)(value3)) << (name3##_POS)) & (name3##_MASK)))    | \
    (((((uint32_t)(value4)) << (name4##_POS)) & (name4##_MASK)))    | \
    (((((uint32_t)(value5)) << (name5##_POS)) & (name5##_MASK)))    | \
    (((((uint32_t)(value6)) << (name6##_POS)) & (name6##_MASK)))    | \
    (((((uint32_t)(value7)) << (name7##_POS)) & (name7##_MASK)))    | \
    (((((uint32_t)(value8)) << (name8##_POS)) & (name8##_MASK)))    | \
    (((((uint32_t)(value9)) << (name9##_POS)) & (name9##_MASK)))    | \
    (((((uint32_t)(value10)) << (name10##_POS)) & (name10##_MASK))) | \
    (((((uint32_t)(value11)) << (name11##_POS)) & (name11##_MASK)))

#define MASK_12REG(name1, value1,   \
                   name2, value2,   \
                   name3, value3,   \
                   name4, value4,   \
                   name5, value5,   \
                   name6, value6,   \
                   name7, value7,   \
                   name8, value8,   \
                   name9, value9,   \
                   name10, value10, \
                   name11, value11, \
                   name12, value12) \
    name1##_MASK  | \
    name2##_MASK  | \
    name3##_MASK  | \
    name4##_MASK  | \
    name5##_MASK  | \
    name6##_MASK  | \
    name7##_MASK  | \
    name8##_MASK  | \
    name9##_MASK  | \
    name10##_MASK | \
    name11##_MASK | \
    name12##_MASK,  \
    (((((uint32_t)(value1)) << (name1##_POS)) & (name1##_MASK)))    | \
    (((((uint32_t)(value2)) << (name2##_POS)) & (name2##_MASK)))    | \
    (((((uint32_t)(value3)) << (name3##_POS)) & (name3##_MASK)))    | \
    (((((uint32_t)(value4)) << (name4##_POS)) & (name4##_MASK)))    | \
    (((((uint32_t)(value5)) << (name5##_POS)) & (name5##_MASK)))    | \
    (((((uint32_t)(value6)) << (name6##_POS)) & (name6##_MASK)))    | \
    (((((uint32_t)(value7)) << (name7##_POS)) & (name7##_MASK)))    | \
    (((((uint32_t)(value8)) << (name8##_POS)) & (name8##_MASK)))    | \
    (((((uint32_t)(value9)) << (name9##_POS)) & (name9##_MASK)))    | \
    (((((uint32_t)(value10)) << (name10##_POS)) & (name10##_MASK))) | \
    (((((uint32_t)(value11)) << (name11##_POS)) & (name11##_MASK))) | \
    (((((uint32_t)(value12)) << (name12##_POS)) & (name12##_MASK)))

#define MASK_13REG(name1, value1,   \
                   name2, value2,   \
                   name3, value3,   \
                   name4, value4,   \
                   name5, value5,   \
                   name6, value6,   \
                   name7, value7,   \
                   name8, value8,   \
                   name9, value9,   \
                   name10, value10, \
                   name11, value11, \
                   name12, value12, \
                   name13, value13) \
    name1##_MASK  | \
    name2##_MASK  | \
    name3##_MASK  | \
    name4##_MASK  | \
    name5##_MASK  | \
    name6##_MASK  | \
    name7##_MASK  | \
    name8##_MASK  | \
    name9##_MASK  | \
    name10##_MASK | \
    name11##_MASK | \
    name12##_MASK | \
    name13##_MASK,  \
    (((((uint32_t)(value1)) << (name1##_POS)) & (name1##_MASK)))    | \
    (((((uint32_t)(value2)) << (name2##_POS)) & (name2##_MASK)))    | \
    (((((uint32_t)(value3)) << (name3##_POS)) & (name3##_MASK)))    | \
    (((((uint32_t)(value4)) << (name4##_POS)) & (name4##_MASK)))    | \
    (((((uint32_t)(value5)) << (name5##_POS)) & (name5##_MASK)))    | \
    (((((uint32_t)(value6)) << (name6##_POS)) & (name6##_MASK)))    | \
    (((((uint32_t)(value7)) << (name7##_POS)) & (name7##_MASK)))    | \
    (((((uint32_t)(value8)) << (name8##_POS)) & (name8##_MASK)))    | \
    (((((uint32_t)(value9)) << (name9##_POS)) & (name9##_MASK)))    | \
    (((((uint32_t)(value10)) << (name10##_POS)) & (name10##_MASK))) | \
    (((((uint32_t)(value11)) << (name11##_POS)) & (name11##_MASK))) | \
    (((((uint32_t)(value12)) << (name12##_POS)) & (name12##_MASK))) | \
    (((((uint32_t)(value13)) << (name13##_POS)) & (name13##_MASK)))

#define MASK_14REG(name1,  value1,  \
                   name2,  value2,  \
                   name3,  value3,  \
                   name4,  value4,  \
                   name5,  value5,  \
                   name6,  value6,  \
                   name7,  value7,  \
                   name8,  value8,  \
                   name9,  value9,  \
                   name10, value10, \
                   name11, value11, \
                   name12, value12, \
                   name13, value13, \
                   name14, value14) \
    name1##_MASK  | \
    name2##_MASK  | \
    name3##_MASK  | \
    name4##_MASK  | \
    name5##_MASK  | \
    name6##_MASK  | \
    name7##_MASK  | \
    name8##_MASK  | \
    name9##_MASK  | \
    name10##_MASK | \
    name11##_MASK | \
    name12##_MASK | \
    name13##_MASK | \
    name14##_MASK,  \
    (((((uint32_t)(value1)) << (name1##_POS)) & (name1##_MASK)))    | \
    (((((uint32_t)(value2)) << (name2##_POS)) & (name2##_MASK)))    | \
    (((((uint32_t)(value3)) << (name3##_POS)) & (name3##_MASK)))    | \
    (((((uint32_t)(value4)) << (name4##_POS)) & (name4##_MASK)))    | \
    (((((uint32_t)(value5)) << (name5##_POS)) & (name5##_MASK)))    | \
    (((((uint32_t)(value6)) << (name6##_POS)) & (name6##_MASK)))    | \
    (((((uint32_t)(value7)) << (name7##_POS)) & (name7##_MASK)))    | \
    (((((uint32_t)(value8)) << (name8##_POS)) & (name8##_MASK)))    | \
    (((((uint32_t)(value9)) << (name9##_POS)) & (name9##_MASK)))    | \
    (((((uint32_t)(value10)) << (name10##_POS)) & (name10##_MASK))) | \
    (((((uint32_t)(value11)) << (name11##_POS)) & (name11##_MASK))) | \
    (((((uint32_t)(value12)) << (name12##_POS)) & (name12##_MASK))) | \
    (((((uint32_t)(value13)) << (name13##_POS)) & (name13##_MASK))) | \
    (((((uint32_t)(value14)) << (name14##_POS)) & (name14##_MASK)))

#define MASK_15REG(name1,  value1,  \
                   name2,  value2,  \
                   name3,  value3,  \
                   name4,  value4,  \
                   name5,  value5,  \
                   name6,  value6,  \
                   name7,  value7,  \
                   name8,  value8,  \
                   name9,  value9,  \
                   name10, value10, \
                   name11, value11, \
                   name12, value12, \
                   name13, value13, \
                   name14, value14, \
                   name15, value15) \
    name1##_MASK  | \
    name2##_MASK  | \
    name3##_MASK  | \
    name4##_MASK  | \
    name5##_MASK  | \
    name6##_MASK  | \
    name7##_MASK  | \
    name8##_MASK  | \
    name9##_MASK  | \
    name10##_MASK | \
    name11##_MASK | \
    name12##_MASK | \
    name13##_MASK | \
    name14##_MASK | \
    name15##_MASK,  \
    (((((uint32_t)(value1)) << (name1##_POS)) & (name1##_MASK)))    | \
    (((((uint32_t)(value2)) << (name2##_POS)) & (name2##_MASK)))    | \
    (((((uint32_t)(value3)) << (name3##_POS)) & (name3##_MASK)))    | \
    (((((uint32_t)(value4)) << (name4##_POS)) & (name4##_MASK)))    | \
    (((((uint32_t)(value5)) << (name5##_POS)) & (name5##_MASK)))    | \
    (((((uint32_t)(value6)) << (name6##_POS)) & (name6##_MASK)))    | \
    (((((uint32_t)(value7)) << (name7##_POS)) & (name7##_MASK)))    | \
    (((((uint32_t)(value8)) << (name8##_POS)) & (name8##_MASK)))    | \
    (((((uint32_t)(value9)) << (name9##_POS)) & (name9##_MASK)))    | \
    (((((uint32_t)(value10)) << (name10##_POS)) & (name10##_MASK))) | \
    (((((uint32_t)(value11)) << (name11##_POS)) & (name11##_MASK))) | \
    (((((uint32_t)(value12)) << (name12##_POS)) & (name12##_MASK))) | \
    (((((uint32_t)(value13)) << (name13##_POS)) & (name13##_MASK))) | \
    (((((uint32_t)(value14)) << (name14##_POS)) & (name14##_MASK))) | \
    (((((uint32_t)(value15)) << (name15##_POS)) & (name15##_MASK)))

#define MASK_16REG(name1, value1,     \
                   name2, value2,     \
                   name3, value3,     \
                   name4, value4,     \
                   name5, value5,     \
                   name6, value6,     \
                   name7, value7,     \
                   name8, value8,     \
                   name9, value9,     \
                   name10, value10,   \
                   name11, value11,   \
                   name12, value12,   \
                   name13, value13,   \
                   name14, value14,   \
                   name15, value15,   \
                   name16, value16)   \
    name1##_MASK  |     \
    name2##_MASK  |     \
    name3##_MASK  |     \
    name4##_MASK  |     \
    name5##_MASK  |     \
    name6##_MASK  |     \
    name7##_MASK  |     \
    name8##_MASK  |     \
    name9##_MASK  |     \
    name10##_MASK |     \
    name11##_MASK |     \
    name12##_MASK |     \
    name13##_MASK |     \
    name14##_MASK |     \
    name15##_MASK |     \
    name16##_MASK,      \
    (((((uint32_t)(value1)) << (name1##_POS)) & (name1##_MASK)))    | \
    (((((uint32_t)(value2)) << (name2##_POS)) & (name2##_MASK)))    | \
    (((((uint32_t)(value3)) << (name3##_POS)) & (name3##_MASK)))    | \
    (((((uint32_t)(value4)) << (name4##_POS)) & (name4##_MASK)))    | \
    (((((uint32_t)(value5)) << (name5##_POS)) & (name5##_MASK)))    | \
    (((((uint32_t)(value6)) << (name6##_POS)) & (name6##_MASK)))    | \
    (((((uint32_t)(value7)) << (name7##_POS)) & (name7##_MASK)))    | \
    (((((uint32_t)(value8)) << (name8##_POS)) & (name8##_MASK)))    | \
    (((((uint32_t)(value9)) << (name9##_POS)) & (name9##_MASK)))    | \
    (((((uint32_t)(value10)) << (name10##_POS)) & (name10##_MASK))) | \
    (((((uint32_t)(value11)) << (name11##_POS)) & (name11##_MASK))) | \
    (((((uint32_t)(value12)) << (name12##_POS)) & (name12##_MASK))) | \
    (((((uint32_t)(value13)) << (name13##_POS)) & (name13##_MASK))) | \
    (((((uint32_t)(value14)) << (name14##_POS)) & (name14##_MASK))) | \
    (((((uint32_t)(value15)) << (name15##_POS)) & (name15##_MASK))) | \
    (((((uint32_t)(value16)) << (name16##_POS)) & (name16##_MASK)))


#define MASK_POS(name) \
    name##_MASK, name##_POS

#define MASK_STEP(name, value) \
    name##_MASK, name##_POS, ((uint32_t)(value))

#define REG_SSAT(name, value) \
    register_saturate((value), 0, (name##_MASK)>>(name##_POS))

#define MASK_STEP_SSAT(name, value) \
    name##_MASK, name##_POS, register_saturate((value), 0, (name##_MASK)>>(name##_POS))

#define MASK_1REG_SSAT(name, value) \
    name##_MASK, \
    (((register_saturate((value), 0, (name##_MASK)>>(name##_POS))) << (name##_POS)) & (name##_MASK))

// for compatibility
#define REGW   register_set
#define REGWA  register_set_raw
#define REGW1  register_set1
#define REGW0  register_set0
#define REGR   register_get


/*******************************************************************************
 * EXTERN FUNCTIONS
 */

/**
 *******************************************************************************
 * @brief  register set
 *
 * @param[in] reg  reg
 * @param[in] mask  mask
 * @param[in] value  value
 *******************************************************************************
 */
__STATIC_FORCEINLINE void register_set(volatile uint32_t *reg, uint32_t mask, uint32_t value)
{
    register uint32_t reg_prev;

    reg_prev = *reg;
    reg_prev &= ~mask;
    reg_prev |= mask & value;
    *reg = reg_prev;
}

/**
 *******************************************************************************
 * @brief  register set raw
 *
 * @param[in] reg  reg
 * @param[in] mask  mask
 * @param[in] value  value
 *******************************************************************************
 */
__STATIC_FORCEINLINE void register_set_raw(volatile uint32_t *reg, uint32_t mask, uint32_t value)
{
    *reg = mask & value;
}

/**
 *******************************************************************************
 * @brief  register set1
 *
 * @param[in] reg  reg
 * @param[in] mask  mask
 *******************************************************************************
 */
__STATIC_FORCEINLINE void register_set1(volatile uint32_t *reg, uint32_t mask)
{
    *reg |= mask;
}

/**
 *******************************************************************************
 * @brief  register set0
 *
 * @param[in] reg  reg
 * @param[in] mask  mask
 *******************************************************************************
 */
__STATIC_FORCEINLINE void register_set0(volatile uint32_t *reg, uint32_t mask)
{
    *reg &= ~mask;
}

/**
 *******************************************************************************
 * @brief  register get
 *
 * @param[in] reg  reg
 * @param[in] mask  mask
 * @param[in] pos  pos
 *
 * @return
 *******************************************************************************
 */
__STATIC_FORCEINLINE uint32_t register_get(const volatile uint32_t *reg, uint32_t mask, uint32_t pos)
{
    return (*reg & mask) >> pos;
}

/**
 * @brief  calib repair value select
 *
 * @param[in] calc  calc
 * @param[in] min  min
 * @param[in] max  max
 *
 * @return select
 **/
__STATIC_FORCEINLINE int register_saturate(int calc, int min, int max)
{
    return (calc < min) ? min
         : (calc > max) ? max
         : calc;
}

#ifdef  __cplusplus
}
#endif

#endif  /* __DRIVER_UTILS_H */


/** @} */
