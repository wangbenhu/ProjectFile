 /*********************************************************************
 * @file nvds_tags.h
 * @brief
 * @version 1.0
 * @date 10 Dec 2021
 * @author wangyc
 *
 * @defgroup NVDS
 * @ingroup HS662X
 * @brief NVDS system
 * @details
 * @note
 */

#ifndef __NVDS_TAGS_H__
#define __NVDS_TAGS_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */


/*********************************************************************
 * TYPEDEFS
 */
enum NVDS_TAGS
{
    /// SYSTEM reserved (1-99)
    NVDS_TAG_BD_ADDRESS                 = 1,
    NVDS_TAG_DEVICE_NAME                = 2,
    NVDS_TAG_SCA                        = 3,
    NVDS_TAG_XTAL32M_CTUNE              = 4,
    NVDS_TAG_XTAL32K_CTUNE              = 5,
    NVDS_TAG_FREQ_OFFSET                = 6,
    NVDS_TAG_DCDC_ENABLE                = 7,
    NVDS_TAG_PRE_WAKEUP_TIME            = 8,
    NVDS_TAG_SMP_BOND_INFO              = 9,

    /// User define begin
    NVDS_TAG_USER_BEGIN                 = 100,
	
	NVDS_TAG_LTE_POWER_STATUS			= 101,
	
	NVDS_USER_TAG_BD_ADDRESS			= 102,
    /// Tag value MUST be less than or equal to 254
    NVDS_TAG_MAX                        = 254,
};

enum NVDS_TAGS_LEN
{
    /// Definition of length associated to each parameters
    NVDS_LEN_BD_ADDRESS                 = 6,
    NVDS_LEN_DEVICE_NAME                = 32,
    NVDS_LEN_SCA                        = 1,
    NVDS_LEN_XTAL32M_CTUNE              = 1,
    NVDS_LEN_XTAL32K_CTUNE              = 1,
    NVDS_LEN_FREQ_OFFSET                = 1,
    NVDS_LEN_DCDC_ENABLE                = 1,
    NVDS_LEN_PRE_WAKEUP_TIME            = 1,

    /// Tag length MUST be less than or equal to 504
    NVDS_LEN_MAX                        = 504,
};

/*********************************************************************
 * MACROS
 */


/*********************************************************************
 * EXTERN VARIABLES
 */

/*********************************************************************
 * EXTERN FUNCTIONS
 */


#ifdef __cplusplus
}
#endif

#endif

/** @} */

