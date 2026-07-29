/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup FLASH FLASH
 * @ingroup  HAL_Driver
 * @brief    FLASH Driver for om66xx
 * @details  FLASH Driver for om66xx
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __DRV_FLASH_COMMON_H
#define __DRV_FLASH_COMMON_H


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_FLASH0 || RTE_FLASH1)
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
#define FLASH_SECTOR_SIZE                       (4 * 1024)
#define FLASH_BLOCK_32K_SIZE                    (32 * 1024)
#define FLASH_BLOCK_64K_SIZE                    (64 * 1024)
#define FLASH_ID2CAP(flash_id)                  (1U << flash_id.cap_id)

#define FLASH_STATUS_1_WIP_POS                   0
#define FLASH_STATUS_1_WIP_MASK                 (0x1U << 0)
#define FLASH_STATUS_1_WEL_POS                   1
#define FLASH_STATUS_1_WEL_MASK                 (0x1U << 1)
#define FLASH_STATUS_1_BP_POS                    2
#define FLASH_STATUS_1_BP_MASK                  (0x1FU << 2)
#define FLASH_STATUS_1_SRP0_POS                  7
#define FLASH_STATUS_1_SRP0_MASK                (0x1U << 7)

#define FLASH_STATUS_2_SRP1_POS                  0
#define FLASH_STATUS_2_SRP1_MASK                (0x1U << 0)
#define FLASH_STATUS_2_QE_POS                    1
#define FLASH_STATUS_2_QE_MASK                  (0x1U << 1)
#define FLASH_STATUS_2_CMP_POS                   6
#define FLASH_STATUS_2_CMP_MASK                 (0x1U << 6)

#define GD_FLASH_STATUS_2_DC_POS                 4
#define GD_FLASH_STATUS_2_DC_MASK               (0x1U << 4)

#define PUYA_FLASH_CONFIG_DC_POS                 1
#define PUYA_FLASH_CONFIG_DC_MASK               (0x1U << 1)
#define PUYA_FLASH_CONFIG_WPS_POS                2
#define PUYA_FLASH_CONFIG_WPS_MASK               (0x1U << 2)
#define PUYA_FLASH_CONFIG_HD_RST_POS             7
#define PUYA_FLASH_CONFIG_HD_RST_MASK            (0x1U << 7)

#define GT_FLASH_CONFIG_DRV0_POS                 5
#define GT_FLASH_CONFIG_DRV0_MASK                (0x1U << 5)
#define GT_FLASH_CONFIG_DRV1_POS                 6
#define GT_FLASH_CONFIG_DRV1_MASK                (0x1U << 6)

#define FLASH_SECURE_REG_ADDR_LOW_POS            0
#define FLASH_SECURE_REG_ADDR_LOW_MASK           (0x1FU << 0)
#define FLASH_SECURE_REG_ADDR_HIGH_POS           12
#define FLASH_SECURE_REG_ADDR_HIGH_MASK          (0xFU << 12)
#define FLASH_SECURE_REG_ADDR_HIGH(reg)          \
                  ((reg << FLASH_SECURE_REG_ADDR_HIGH_POS) & FLASH_SECURE_REG_ADDR_HIGH_MASK)
#define FLASH_SECURE_REG_ADDR(reg, off)          \
                  (((reg << FLASH_SECURE_REG_ADDR_HIGH_POS) & FLASH_SECURE_REG_ADDR_HIGH_MASK) | \
                   ((off << FLASH_SECURE_REG_ADDR_LOW_POS) & FLASH_SECURE_REG_ADDR_LOW_MASK))

#define FLASH_READ_PARA_DUMMY_CYCLE_POS          4
#define FLASH_READ_PARA_DUMMY_CYCLE_MASK        (0x03U << 4)
#define FLASH_READ_PARA_WRAP_DIS_POS             2
#define FLASH_READ_PARA_WRAP_DIS_MASK           (0x01U << 2)
#define FLASH_READ_PARA_WRAP_LEN_POS             0
#define FLASH_READ_PARA_WRAP_LEN_MASK           (0x03U << 0)

#define FLASH_READ_PARA_DUMMY_CYCLE_4            1U
#define FLASH_READ_PARA_DUMMY_CYCLE_6            2U
#define FLASH_READ_PARA_DUMMY_CYCLE_8            3U
#define FLASH_READ_PARA_WRAP_EN                  0U
#define FLASH_READ_PARA_WRAP_DIS                 1U
#define FLASH_READ_PARA_WRAP_LEN_8               0U
#define FLASH_READ_PARA_WRAP_LEN_16              1U
#define FLASH_READ_PARA_WRAP_LEN_32              2U
#define FLASH_READ_PARA_WRAP_LEN_64              3U

#define FLASH_PAGE_SIZE                          (256)

// flash default FREQ HZ (8M-DELAY2 is stable when DVDD is changed)
#define FLASH_FREQ_HZ_DEFAULT                   8000000
// flash default delay
#define FLASH_DELAY_DEFAULT                     2
// flash max delay
#define FLASH_DELAY_MAX                         0x0F
// flash auto detect delay
#define FLASH_DELAY_AUTO                        0xFF
// flash auto delay retry times
#define FLASH_AUTO_DLY_RETYR_CNT                3U
// flash default timeout, the value is depends on the page programming time
#define FLASH_TIMEOUT_MS_DEFAULT                2000


/*******************************************************************************
 * TYPEDEFS
 */
typedef union {
    struct {
        uint8_t man_id;              /*!< manufacturer id */
        uint8_t mem_id;              /*!< memory id */
        uint8_t cap_id;              /*!< capacity id */
    };
    uint32_t id;
} flash_id_t;

typedef enum {
    /* SPI mode */
    FLASH_READ                       = 0x00,
    FLASH_FAST_READ                  = 0x01,
    FLASH_FAST_READ_DO               = 0x02,
    FLASH_FAST_READ_DIO              = 0x03,
    FLASH_FAST_READ_QO               = 0x04,
    FLASH_FAST_READ_QIO              = 0x05,
    /* QPI mode, and for external flash only */
    FLASH_FAST_READ_QPI_4_DUMMY      = 0x10,
    FLASH_FAST_READ_QPI_6_DUMMY      = 0x11,
    FLASH_FAST_READ_QPI_8_DUMMY      = 0x12,
    FLASH_FAST_READ_QPI_10_DUMMY     = 0x13,
    FLASH_FAST_READ_QIO_QPI_4_DUMMY  = 0x14,
    FLASH_FAST_READ_QIO_QPI_6_DUMMY  = 0x15,
    FLASH_FAST_READ_QIO_QPI_8_DUMMY  = 0x16,
    FLASH_FAST_READ_QIO_QPI_10_DUMMY = 0x17,
} flash_read_t;

typedef enum {
    /* SPI mode */
    FLASH_PAGE_PROGRAM               = 0x00,
    FLASH_PAGE_PROGRAM_QI            = 0x01,
    /* QPI mode */
    FLASH_PAGE_PROGRAM_QPI           = 0x10,
} flash_write_t;

typedef enum {
    FLASH_ERASE_4K,
    FLASH_ERASE_32K,
    FLASH_ERASE_64K,
    FLASH_ERASE_CHIP,
} flash_erase_t;

/*
 *  Attention:
 *      The flash_protect_t support the following flash:
 *      GD25WQ16E, GD25WQ80E, GD25WQ40E,
 *      P25Q40SU, P25Q80SU, P25Q16SU,
 *      GT25Q40D, GT25Q80A
 */
typedef enum {
    FLASH_PROTECT_ALL                        = 0x00U << FLASH_STATUS_1_BP_POS,
    FLASH_PROTECT_UPPER_1_BLOCK_UNPROTECTED  = 0x01U << FLASH_STATUS_1_BP_POS,
    FLASH_PROTECT_UPPER_2_BLOCK_UNPROTECTED  = 0x02U << FLASH_STATUS_1_BP_POS,
    FLASH_PROTECT_UPPER_4_BLOCK_UNPROTECTED  = 0x03U << FLASH_STATUS_1_BP_POS,
    FLASH_PROTECT_UPPER_1_SECTOR_UNPROTECTED = 0x11U << FLASH_STATUS_1_BP_POS,
    FLASH_PROTECT_UPPER_2_SECTOR_UNPROTECTED = 0x12U << FLASH_STATUS_1_BP_POS,
    FLASH_PROTECT_UPPER_4_SECTOR_UNPROTECTED = 0x13U << FLASH_STATUS_1_BP_POS,
    FLASH_PROTECT_NONE                       = 0x1FU << FLASH_STATUS_1_BP_POS,
} flash_protect_t;

typedef enum {
    FLASH_MID_NONE              = 0,
    FLASH_MID_PUYA              = 0x85,    /* Puya */
    FLASH_MID_BOYA_0            = 0xE0,    /* Boya */
    FLASH_MID_BOYA_1            = 0x68,    /* Boya */
    FLASH_MID_TONGXIN           = 0xEB,    /* Tongxin */
    FLASH_MID_XTX               = 0x0B,    /* XTX */
    FLASH_MID_ALLIANCE          = 0x52,    /* Alliance Semiconductor */
    FLASH_MID_AMD               = 0x01,    /* AMD */
    FLASH_MID_AMIC              = 0x37,    /* AMIC */
    FLASH_MID_ATMEL             = 0x1F,    /* Atmel (now used by Adesto) */
    FLASH_MID_BRIGHT            = 0xAD,    /* Bright Microelectronics */
    FLASH_MID_CATALYST          = 0x31,    /* Catalyst */
    FLASH_MID_ESMT              = 0x8C,    /* Elite Semiconductor Memory Technology (ESMT)/EFST Elite Flash Storage */
    FLASH_MID_EON               = 0x1C,    /* EON, missing 0x7F prefix */
    FLASH_MID_EXCEL             = 0x4A,    /* ESI, missing 0x7F prefix */
    FLASH_MID_FIDELIX           = 0xF8,    /* Fidelix */
    FLASH_MID_FUJITSU           = 0x04,    /* Fujitsu */
    FLASH_MID_GIGADEVICE        = 0xC8,    /* GigaDevice */
    FLASH_MID_GIGADEVICE_XD     = 0x50,    /* GigaDevice */
    FLASH_MID_GIGADEVICE_MD     = 0x51,    /* GigaDevice */
    FLASH_MID_HYUNDAI           = 0xAD,    /* Hyundai */
    FLASH_MID_IMT               = 0x1F,    /* Integrated Memory Technologies */
    FLASH_MID_INTEL             = 0x89,    /* Intel */
    FLASH_MID_ISSI              = 0xD5,    /* ISSI Integrated Silicon Solutions, see also PMC. */
    FLASH_MID_MACRONIX          = 0xC2,    /* Macronix (MX) */
    FLASH_MID_NANTRONICS        = 0xD5,    /* Nantronics, missing prefix */
    FLASH_MID_PMC               = 0x9D,    /* PMC, missing 0x7F prefix */
    FLASH_MID_SANYO             = 0x62,    /* Sanyo */
    FLASH_MID_SHARP             = 0xB0,    /* Sharp */
    FLASH_MID_SPANSION          = 0x01,    /* Spansion, same ID as AMD */
    FLASH_MID_SST               = 0xBF,    /* SST */
    FLASH_MID_ST                = 0x20,    /* ST / SGS/Thomson / Numonyx (later acquired by Micron) */
    FLASH_MID_SYNCMOS_MVC       = 0x40,    /* SyncMOS (SM) and Mosel Vitelic Corporation (MVC) */
    FLASH_MID_TI                = 0x97,    /* Texas Instruments */
    FLASH_MID_WINBOND_NEX       = 0xEF,    /* Winbond (ex Nexcom) serial flashes */
    FLASH_MID_WINBOND           = 0xDA,    /* Winbond */
    FLASH_MID_GT                = 0xC4,    /* Giantec */
} flash_mid_t;

typedef enum {
    FLASH_SPI_MODE_0            = 0,       /*!< SPI mode 0, CPHA=0, CPOL=0 */
    FLASH_SPI_MODE_1            = 1,       /*!< SPI mode 1, CPHA=1, CPOL=0 */
    FLASH_SPI_MODE_2            = 2,       /*!< SPI mode 2, CPHA=0, CPOL=1 */
    FLASH_SPI_MODE_3            = 3,       /*!< SPI mode 3, CPHA=1, CPOL=1 */
} flash_spi_mode_t;

typedef enum {
    FLASH_STATE_UNINIT,
    FLASH_STATE_INIT,
    FLASH_STATE_ERASING,
    FLASH_STATE_WRITING,
    FLASH_STATE_READING,
} flash_state_t;

typedef enum {
    FLASH_TRANS_IDLE,                       /**< flash is not in transfer state */
    FLASH_TRANS_BUSY,                       /**< flash is in transfer state */
} flash_trans_state_t;

typedef struct {
    uint32_t cmd;                           /**< read or write opcode, only valid [0-7] bit */
    uint32_t frame_cfg[2];                  /**< frame config for cmd, see OSPI_FRAME_CONFIG define */
} flash_frame_t;

typedef struct __PACKED {
    uint8_t clk_div;                        /*!< Clock divider */
    uint8_t delay;                          /*!< Delay(2 ns for per step) */
    flash_read_t read_cmd;                  /*!< Command for reading, see@ref flash_read_t */
    flash_write_t write_cmd;                /*!< Command for writing, see@ref flash_write_t */
    flash_spi_mode_t spi_mode;              /*!< Standard SPI mode, see@ref flash_spi_mode_t */
} flash_config_t;

typedef struct {
    uint8_t auto_delay_en;                  /*!< Auto delay enable */
    uint8_t auto_delay;                     /*!< Auto delay value */
    uint8_t valid_delay_min;                /*!< Minimum valid delay value */
    uint8_t valid_delay_max;                /*!< Maximum valid delay value */
} flash_delay_info_t;

typedef enum {
    SR_WRITE_EN_NONE = 0,                   /*!< Not send write enable command */
    SR_WRITE_EN_VOLATILE,                   /*!< Send volatile write enable command (0x50) */
    SR_WRITE_EN_PERMANENT,                  /*!< Send permanent write enable command (0x06) */
} flash_reg_write_en_t;

#ifdef __cplusplus
}
#endif

#endif  /* (RTE_FLASH) */

#endif  /* __DRV_FLASH_COMMON_H */


/** @} */
