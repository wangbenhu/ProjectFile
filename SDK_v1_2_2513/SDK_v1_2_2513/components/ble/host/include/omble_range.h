/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @addtogroup OMBLE_RANGE
 * @brief RANGE
 * @version
 * Version 1.0
 *  - Initial release
 *
 */
/// @{

#ifndef __OMBLE_RANGE_H__
#define __OMBLE_RANGE_H__
#include <stdint.h>


///  event base value of each modules
enum OB_EVENTS_BASE {
    /// GAP Event base value
    OB_GAP_EVTS_BASE = 0x0100,
    /// GATT Event base value
    OB_GATT_EVTS_BASE = 0x0200,
};

#endif /* __OMBLE_RANGE_H__ */

/// @}
