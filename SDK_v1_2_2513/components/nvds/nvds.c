/*********************************************************************
 * @file nvds.c
 * @brief
 * @version 1.0
 * @date 10 Dec 2020
 * @author wangyc
 *
 * @note
 */

#include <string.h> // String definitions
#include <stddef.h> // Standard definitions
#include <limits.h> // Limits definitions
#include "om_driver.h"
#include "nvds.h"   // NVDS definitions


/*
 * DEFINES
 ****************************************************************************************
 */

/// Enable debug mode, it is only used for internal debug, default 0
#define NVDS_DEBUG_MODE                 0
/// Size of one flash page
//#define FLASH_PAGE_SIZE                 0x100
/// Size of one flash sector
//#define FLASH_SECTOR_SIZE               0x1000
/// Magic length
#define NVDS_MAGIC_NUMBER_LENGTH        0x04
/// Tag not exists
#define NVDS_TAG_NIL                    0xFF
/// Tag flash address is not exist
#define NVDS_NO_TAG_ADDRESS             0x00
/// Size of crc
#define NVDS_CRC_SIZE                   0x01
/// Max storage for the NVDS device which can be used for tags
#define NVDS_MAX_STORAGE_SIZE           4096
/// Number of pages in nvds sector (first page is used for magic number,ignore it)
#define NVDS_NUM_OF_PAGES ((FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE) - 1)
/// NVDS maximum tag length
#define NVDS_MAX_TAG_LENGTH (2 * FLASH_PAGE_SIZE)
/// Invalid serial number
#define NVDS_SN_INVALID                 0xFFFFFFFF
/// Invalid flash address
#define NVDS_INVALID_FLASH_ADDRESS      0xFFFF
/// Invalid tag length
#define NVDS_INVALID_TAG_LENGTH         0xFFFF
/// Maximum crc check size
#define NVDS_MAX_CRC_CHECK_SIZE         32
/// Maximum retry times for CRC check
#define NVDS_MAX_RETRY                  3

/*
 * MACROS
 ****************************************************************************************
 */
/// Check if tag is the last one
#define NVDS_IS_TAG_LAST(h) ((h).tag == NVDS_TAG_NIL)

/// Check if tag id is valid
#define NVDS_IS_TAG_VALID(h) ((h).tag < NVDS_MAX_NUM_OF_TAGS)

/// Check if tag sn is valid
#define NVDS_IS_SN_VALID(h, cur_sn) (((h)->sn > cur_sn) && ((h)->sn != NVDS_SN_INVALID))

/// Length of tag header
#define NVDS_TAG_HEADER_LENGTH (sizeof(struct nvds_tag_header))

/// Length of config tag header
#define NVDS_CONF_TAG_HEADER_LENGTH (sizeof(struct conf_tag_header))

/// Length of tag data
#define NVDS_TAG_DATA_LENGTH(h) ((h).length)

/// NVDS data maximum length
#define NVDS_DATA_MAX_LENGTH (NVDS_MAX_TAG_LENGTH - NVDS_TAG_HEADER_LENGTH)

/// Check if it is the start address of current page
#define NVDS_IS_PAGE_START_ADDRESS(addr) (!((addr) % FLASH_PAGE_SIZE))

/// Next page start address of current flash address
#define NVDS_NEXT_PAGE_START_ADDRESS(addr) \
    ((addr) + (FLASH_PAGE_SIZE - ((addr) % FLASH_PAGE_SIZE)))

/// Update current sector address to a new page address
#define NVDS_UPDATE_SECTOR_ADDRESS(addr) \
    (NVDS_IS_PAGE_START_ADDRESS(addr) ? (addr) : NVDS_NEXT_PAGE_START_ADDRESS(addr))

/// Complete tag length (header + data)
#define NVDS_TAG_COMPLETE_LENGTH (NVDS_TAG_HEADER_LENGTH + NVDS_DATA_MAX_LENGTH)

/// The maximum address of sector
#define NVDS_SECTOR_MAX_ADDRESS(start) (start + NVDS_MAX_STORAGE_SIZE)

/// Get page start address mask bit
#define NVDS_GET_PAGE_START_ADDRESS_MASK(base, addr) (((addr) - (base)) / FLASH_PAGE_SIZE)

/// Get sector start address base current flash address
#define NVDS_GET_SECTOR(addr, base) base + FLASH_SECTOR_SIZE *((address - base) / FLASH_SECTOR_SIZE)

/// Set page start address mask bit
#define NVDS_SET_PAGE_START_ADDRESS_MASK(mask, offset) (mask | (0x01 << offset))

#define NVDS_READ(address, buf, length)       drv_flash_read(OM_FLASH0, address, buf, length)

#define NVDS_WRITE(address, buf, length)      drv_flash_write(OM_FLASH0, address, buf, length)

#define NVDS_ERASE(address, length)           nvds_erase_wrap(address, length)

#define NVDS_SF_SECTOR_SHIFT                  12

#define NVDS_SF_CAPACITY                      (1024*1024) //drv_flash_capacity(OM_FLASH0)

/*
 * ENUMERATION DEFINITIONS
 ****************************************************************************************
 */
enum SECTOR_MAGIC
{
    NVDS_SECTOR = 0,
    BKUP_SECTOR = 1,
    CONF_SECTOR = 2,
    TOTAL_NUM_OF_SECTORS
};
/*
 * STRUCT DEFINITIONS
 ****************************************************************************************
 */

struct nvds_tag_header
{
    /* Note: Do not change order of the following members!*/

    /// CRC, including tag,len,sn,payload
    uint8_t crc;
    /// Current TAG identifier
    nvds_tag_id_t tag;
    /// Length of the TAG
    nvds_tag_len_t length;
    /// Serial number
    uint32_t sn;
};

struct conf_tag_header
{
    /// Current TAG identifier
    nvds_tag_id_t tag;
    /// Length of the TAG
    nvds_tag_len_t length;
} __attribute__((packed));

/// Environment structure of the NVDS module
struct nvds_env_tag
{
    /// The NVDS's base address in flash
    nvds_addr_len_t flash_base;

    /// NVDS sector start address
    nvds_addr_len_t nvds_addr;

    /// The spare address of current NVDS sector
    nvds_addr_len_t nvds_spare_addr;

    /// NVDS sector page start address mask
    uint16_t nvds_mask;

    /// Total used size of current NVDS sector
    uint16_t nvds_used_size;

    /// Backup sector start address
    nvds_addr_len_t bkup_addr;

    /// The spare address of current backup sector
    nvds_addr_len_t bkup_spare_addr;

    /// The spare size of current backup sector
    uint16_t bkup_spare_size;

    /// Backup sector page start address mask
    uint16_t bkup_mask;

    /// Config sector start address
    nvds_addr_len_t conf_addr;

    /// Record all valid tag_id in NVDS sectors
    uint8_t nvds_tag_record[NVDS_NUM_OF_PAGES];

#if CFG_FILE_SIMULATION
    /// The spare address of current config sector
    nvds_addr_len_t conf_spare_addr;
#endif
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/// NVDS environment
static struct nvds_env_tag nvds_env;

/// magic number keyword of each sector
/// NOTE: flash controller DMA doesn't support ROM address
const static int32_t nvds_magic_number[TOTAL_NUM_OF_SECTORS] = {0x5344564e, /* NVDS */
                                                          0x50554b42, /* BKUP */
                                                          0x464e4f43  /* CONF */};

/* Table for CRC-7 (polynomial x^7 + x^3 + 1) */
static const uint8_t crc7_syndrome_table[256] = {
    0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f,
    0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77,
    0x19, 0x10, 0x0b, 0x02, 0x3d, 0x34, 0x2f, 0x26,
    0x51, 0x58, 0x43, 0x4a, 0x75, 0x7c, 0x67, 0x6e,
    0x32, 0x3b, 0x20, 0x29, 0x16, 0x1f, 0x04, 0x0d,
    0x7a, 0x73, 0x68, 0x61, 0x5e, 0x57, 0x4c, 0x45,
    0x2b, 0x22, 0x39, 0x30, 0x0f, 0x06, 0x1d, 0x14,
    0x63, 0x6a, 0x71, 0x78, 0x47, 0x4e, 0x55, 0x5c,
    0x64, 0x6d, 0x76, 0x7f, 0x40, 0x49, 0x52, 0x5b,
    0x2c, 0x25, 0x3e, 0x37, 0x08, 0x01, 0x1a, 0x13,
    0x7d, 0x74, 0x6f, 0x66, 0x59, 0x50, 0x4b, 0x42,
    0x35, 0x3c, 0x27, 0x2e, 0x11, 0x18, 0x03, 0x0a,
    0x56, 0x5f, 0x44, 0x4d, 0x72, 0x7b, 0x60, 0x69,
    0x1e, 0x17, 0x0c, 0x05, 0x3a, 0x33, 0x28, 0x21,
    0x4f, 0x46, 0x5d, 0x54, 0x6b, 0x62, 0x79, 0x70,
    0x07, 0x0e, 0x15, 0x1c, 0x23, 0x2a, 0x31, 0x38,
    0x41, 0x48, 0x53, 0x5a, 0x65, 0x6c, 0x77, 0x7e,
    0x09, 0x00, 0x1b, 0x12, 0x2d, 0x24, 0x3f, 0x36,
    0x58, 0x51, 0x4a, 0x43, 0x7c, 0x75, 0x6e, 0x67,
    0x10, 0x19, 0x02, 0x0b, 0x34, 0x3d, 0x26, 0x2f,
    0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c,
    0x3b, 0x32, 0x29, 0x20, 0x1f, 0x16, 0x0d, 0x04,
    0x6a, 0x63, 0x78, 0x71, 0x4e, 0x47, 0x5c, 0x55,
    0x22, 0x2b, 0x30, 0x39, 0x06, 0x0f, 0x14, 0x1d,
    0x25, 0x2c, 0x37, 0x3e, 0x01, 0x08, 0x13, 0x1a,
    0x6d, 0x64, 0x7f, 0x76, 0x49, 0x40, 0x5b, 0x52,
    0x3c, 0x35, 0x2e, 0x27, 0x18, 0x11, 0x0a, 0x03,
    0x74, 0x7d, 0x66, 0x6f, 0x50, 0x59, 0x42, 0x4b,
    0x17, 0x1e, 0x05, 0x0c, 0x33, 0x3a, 0x21, 0x28,
    0x5f, 0x56, 0x4d, 0x44, 0x7b, 0x72, 0x69, 0x60,
    0x0e, 0x07, 0x1c, 0x15, 0x2a, 0x23, 0x38, 0x31,
    0x46, 0x4f, 0x54, 0x5d, 0x62, 0x6b, 0x70, 0x79};

/*
 * LOCAL FUNCTION DECLARATION
 ****************************************************************************************
 */
static void nvds_erase_wrap(uint32_t address, uint32_t length)
{
    for (uint32_t i = 0; i < length; i += FLASH_SECTOR_SIZE) {
        drv_flash_erase(OM_FLASH0, address + i, FLASH_ERASE_4K);
    }
}

static void _nvds_read(uint32_t address, uint32_t length, uint8_t *buf)
{
#if CFG_FILE_SIMULATION
    cfg_sim_read(buf, (void *)((intptr_t)address), length);
#else
    NVDS_READ(address, buf, length);
#endif
}

static void _nvds_write(uint32_t address, uint32_t length, uint8_t *buf)
{
#if CFG_FILE_SIMULATION
    cfg_sim_write(buf, (void *)((intptr_t)address), length);
#else
    NVDS_WRITE(address, buf, length);
#endif
}

static void _nvds_erase(uint32_t address, uint32_t length)
{
#if CFG_FILE_SIMULATION
    cfg_sim_erase((void *)((intptr_t)address), FLASH_SECTOR_SIZE);
#else
    NVDS_ERASE(address, length);
#endif
}

static inline uint8_t _nvds_crc7_byte(uint8_t crc, uint8_t data)
{
    return crc7_syndrome_table[(crc << 0x01) ^ data];
}

static uint8_t _nvds_crc7(uint8_t crc, const uint8_t *buffer, uint32_t length)
{
    while (length--)
    {
        crc = _nvds_crc7_byte(crc, *buffer++);
    }
    return crc;
}

/**
 * @brief check if crc is correct
 *
 * @param[in]  crc        target crc value
 * @param[in]  address    target address
 * @param[in]  length     data length
 *
 * @return errno
 **/
static uint8_t _nvds_check_crc(uint8_t crc, uint32_t address, nvds_tag_len_t length)
{
    uint8_t cal;
    uint8_t status = NVDS_CORRUPT;
    uint8_t retry = NVDS_MAX_RETRY;
    uint16_t len_leftover, cur_len, offset;
    uint32_t buf_ptr[NVDS_MAX_CRC_CHECK_SIZE / sizeof(uint32_t)];

    if (length > (NVDS_TAG_HEADER_LENGTH + NVDS_DATA_MAX_LENGTH - NVDS_CRC_SIZE))
    {
        status = NVDS_CORRUPT;
        goto exit;
    }

    do
    {
        memset(buf_ptr, 0x00, NVDS_MAX_CRC_CHECK_SIZE);

        if (length > NVDS_MAX_CRC_CHECK_SIZE)
        {
            offset = 0;
            cal = 0;
            len_leftover = length;

            while (len_leftover)
            {
                cur_len = len_leftover > NVDS_MAX_CRC_CHECK_SIZE ? NVDS_MAX_CRC_CHECK_SIZE : len_leftover;
                _nvds_read(address + offset, cur_len, (uint8_t *)buf_ptr);
                cal = _nvds_crc7(cal, (uint8_t *)buf_ptr, cur_len);
                len_leftover -= cur_len;
                if (len_leftover == 0)
                {
                    break;
                }
                offset += cur_len;
                memset((uint8_t *)buf_ptr, 0, NVDS_MAX_CRC_CHECK_SIZE);
            }
        }
        else
        {
            _nvds_read(address, length, (uint8_t *)buf_ptr);
            cal = _nvds_crc7(0, (uint8_t *)buf_ptr, length);
        }

        if (crc != cal)
        {
            retry--;
        }
        else
        {
            status = NVDS_OK;
            break;
        }

    } while (retry);

exit:
    return status;
}

static uint8_t _nvds_generate_crc(uint8_t crc, uint8_t *buf, uint32_t len)
{
    return _nvds_crc7(crc, buf, len);
}

static uint8_t _nvds_verify_write(uint8_t* expected_data, uint32_t address, uint16_t length)
{
    uint8_t actual_data[FLASH_PAGE_SIZE];

    if (length > FLASH_PAGE_SIZE)
    {
        return NVDS_LENGTH_OUT_OF_RANGE;
    }

    NVDS_READ(address, actual_data, length);
    if (memcmp(actual_data, expected_data, length) != 0)
    {
        return NVDS_FAIL;
    }
    return NVDS_OK;
}

/**
 * @brief check if specific sector's magic is valid
 *
 * @param[in]  sector_addr        sector address
 * @param[in]  sector_magic       sector type
 *
 * @return true or false
 **/
static bool _nvds_is_magic_number_ok(uint32_t sector_addr, enum SECTOR_MAGIC sector_magic)
{
    bool is_magic_number_ok = false;
    uint32_t read_magic_number = 0;

    // Look for the magic number
    _nvds_read(sector_addr, NVDS_MAGIC_NUMBER_LENGTH, (uint8_t *)&read_magic_number);

    // Compare the read magic number with the correct value
    if (read_magic_number == nvds_magic_number[sector_magic])
    {
        is_magic_number_ok = true;
    }

    return is_magic_number_ok;
}

/**
 * @brief format specific sector
 *
 * @param[in]  sector_addr        sector address
 * @param[in]  sector_magic       sector type
 *
 * @return none
 **/
static void _nvds_format_sector(uint32_t sector_addr, enum SECTOR_MAGIC sector_magic)
{
    int32_t magic_num;

#if !CFG_FILE_SIMULATION
    // Shall not format the config sector
    if (sector_magic == CONF_SECTOR)
        return;
#endif
retry:
    // Format the sector
    _nvds_erase(sector_addr, NVDS_MAX_STORAGE_SIZE);

    // Write correct magic number
    magic_num = nvds_magic_number[sector_magic];
    _nvds_write(sector_addr,
                (uint32_t)NVDS_MAGIC_NUMBER_LENGTH,
                (uint8_t *)&magic_num);
    if(_nvds_verify_write((uint8_t *)&magic_num, sector_addr, NVDS_MAGIC_NUMBER_LENGTH)) {
        goto retry;
    }
}

/**
 * @brief read current tag header and get next tag address
 *
 * @param[in]  cur_tag_addr        current tag address
 * @param[in]  tag_header_len      length of tag header
 * @param[out] tag_header_ptr      pointer to the buffer containing the header part of the tag
 * @param[out] nxt_tag_addr_ptr    pointer to next tag address
 *
 * @return true or false
 **/
static uint8_t _nvds_walk_tag(nvds_addr_len_t cur_tag_addr,
                              uint8_t tag_header_len,
                              uint8_t *tag_header_ptr,
                              nvds_addr_len_t *nxt_tag_addr_ptr)
{
    uint8_t status = NVDS_OK;
    struct conf_tag_header tag_hdr;
    nvds_tag_len_t com_tag_len = 0;

    // Read the current parameter header
    _nvds_read((uint32_t)cur_tag_addr, (uint32_t)tag_header_len, tag_header_ptr);

    if (tag_header_len == NVDS_TAG_HEADER_LENGTH)
    {
        tag_hdr.tag = ((struct nvds_tag_header *)tag_header_ptr)->tag;
        tag_hdr.length = ((struct nvds_tag_header *)tag_header_ptr)->length;
        com_tag_len += tag_hdr.length + NVDS_TAG_HEADER_LENGTH;
        if (_nvds_check_crc(((struct nvds_tag_header *)tag_header_ptr)->crc, cur_tag_addr + NVDS_CRC_SIZE, com_tag_len - NVDS_CRC_SIZE) != NVDS_OK || ((struct nvds_tag_header *)tag_header_ptr)->sn == 0)
        {
            // Check if the nvds sector is corrupted
            if (NVDS_IS_PAGE_START_ADDRESS(cur_tag_addr)) {
                uint32_t tag_buf[FLASH_PAGE_SIZE / sizeof(uint32_t)];
                _nvds_read((uint32_t)cur_tag_addr, FLASH_PAGE_SIZE, (uint8_t *)tag_buf);
                for (uint16_t i = 0; i < FLASH_PAGE_SIZE; i++) {
                    if (((uint8_t *)tag_buf)[i] != 0xFF) {
                        return NVDS_CORRUPT;
                    }
                }
            }
            // this is beyond the last TAG
            status = NVDS_TAG_NOT_DEFINED;
        }
    }
    else
    {
        tag_hdr = *((struct conf_tag_header *)tag_header_ptr);
    }

    // Check if the read operation completed successfully
    if (!NVDS_IS_TAG_LAST(tag_hdr) && NVDS_IS_TAG_VALID(tag_hdr) && status == NVDS_OK)
    {
        // Calculate the address of the next tag
        *nxt_tag_addr_ptr = cur_tag_addr + tag_header_len + tag_hdr.length;
    }
    else
    {
        // this is beyond the last TAG
        status = NVDS_TAG_NOT_DEFINED;
    }
    return (status);
}

/**
 * @brief check if specific address is in CONF sector range
 *
 * @param[in]  address        flash address
 *
 * @return true or false
 **/
static bool _nvds_is_conf_tag(uint32_t address)
{
    if (nvds_env.conf_addr != NVDS_INVALID_FLASH_ADDRESS && address > nvds_env.conf_addr && address < (nvds_env.conf_addr + NVDS_MAX_STORAGE_SIZE))
    {
        return true;
    }
    return false;
}

/**
 * @brief read specific tag from CONF sector
 *
 * @param[in]  tag        tag index
 * @param[in]  length     buffer length, if it is null, ignore it.
 * @param[out] length'    real readed length, if length(in) > length(real),
 * @param[out] buf        pointer to the buffer containing the data part of the tag
 *
 * @return errno
 **/
static uint8_t _nvds_read_conf_tag(uint8_t tag, uint16_t *length, uint8_t *buf)
{
    uint8_t status = NVDS_OK;
    struct conf_tag_header tag_hdr;
    nvds_addr_len_t cur_tag_addr, nxt_tag_addr;
    uint32_t max_access_addr;
    uint16_t tag_len;

    if (nvds_env.conf_addr == NVDS_INVALID_FLASH_ADDRESS)
    {
        return NVDS_TAG_NOT_DEFINED;
    }

    if (length == NULL || buf == NULL)
    {
        return NVDS_NO_SPACE_AVAILABLE;
    }

    if (*length == 0 || *length > NVDS_DATA_MAX_LENGTH)
    {
        return NVDS_LENGTH_OUT_OF_RANGE;
    }

    nxt_tag_addr = NVDS_NEXT_PAGE_START_ADDRESS(nvds_env.conf_addr);
    max_access_addr = NVDS_SECTOR_MAX_ADDRESS(nvds_env.conf_addr);

    do
    {
        // go to the next tag
        cur_tag_addr = nxt_tag_addr;
        // retrieve the tag header
        status = _nvds_walk_tag(cur_tag_addr, NVDS_CONF_TAG_HEADER_LENGTH, (uint8_t *)&tag_hdr, &nxt_tag_addr);
        // Check if tag status is ok and tag id is valid
        if (status == NVDS_OK && tag_hdr.tag == tag)
        {
            tag_len = tag_hdr.length >= *length ? *length : tag_hdr.length;
            _nvds_read(cur_tag_addr + NVDS_CONF_TAG_HEADER_LENGTH, tag_len, buf);
            *length = tag_len;
            break;
        }

    } while (status == NVDS_OK && nxt_tag_addr < (max_access_addr - NVDS_CONF_TAG_HEADER_LENGTH));

    return status;
}

/**
 * @brief update current NVDS sector used size of all valid tag
 *
 * @return none
 **/
static void _nvds_update_used_size(void)
{
    uint8_t idx, temp;
    nvds_addr_len_t cur_tag_addr;
    uint16_t com_tag_len;
    uint32_t tag_hdr[NVDS_TAG_HEADER_LENGTH / sizeof(uint32_t)] = {0};

    memset(nvds_env.nvds_tag_record, NVDS_TAG_NIL, NVDS_NUM_OF_PAGES);
    nvds_env.nvds_used_size = 0;

    idx = NVDS_GET_PAGE_START_ADDRESS_MASK(nvds_env.nvds_addr, nvds_env.nvds_spare_addr) - 1;

    while (idx != 0)
    {
        if (nvds_env.nvds_mask & (0x01 << idx))
        {
            cur_tag_addr = nvds_env.nvds_addr + FLASH_PAGE_SIZE * idx;
            _nvds_read(cur_tag_addr, NVDS_TAG_HEADER_LENGTH, (uint8_t *)tag_hdr);
            com_tag_len = NVDS_TAG_HEADER_LENGTH + ((struct nvds_tag_header *)tag_hdr)->length;

            if (_nvds_check_crc(((struct nvds_tag_header *)tag_hdr)->crc, cur_tag_addr + NVDS_CRC_SIZE, com_tag_len - NVDS_CRC_SIZE) == NVDS_OK)
            {
                for (temp = 0; temp < NVDS_NUM_OF_PAGES; temp++)
                {
                    if (nvds_env.nvds_tag_record[temp] == ((struct nvds_tag_header *)tag_hdr)->tag)
                    {
                        break;
                    }

                    if (nvds_env.nvds_tag_record[temp] == NVDS_TAG_NIL)
                    {
                        nvds_env.nvds_tag_record[temp] = ((struct nvds_tag_header *)tag_hdr)->tag;
                        nvds_env.nvds_used_size += com_tag_len;
                        break;
                    }
                }
            }
        }
        idx--;
    }
}

/**
 * @brief traversal CONF sector and get specific tag address
 *
 * @param[in]  tag        tag index
 * @param[out] tag_hdr    tag header buffer
 *
 * @return valid tag address or NVDS_NO_TAG_ADDRESS
 **/
#if NVDS_DEBUG_MODE
// define print function
#define NVDS_PRINTF_PORT             (void*)om_printf
#define NVDS_PRINTF(fmt, args...)    om_printf(fmt, ##args)
// function declaration
static void _nvds_print_data(void *printf_dump_func, uint16_t data_len, uint8_t *data);
// used for storing complete tag data
uint32_t tag_com_data[FLASH_PAGE_SIZE * 2 / sizeof(uint32_t)] = {0};
// switch of debug mode
bool sw_flag = false;
#endif
static uint32_t _nvds_browse_conf_sector(uint8_t tag, struct conf_tag_header *tag_header)
{
    uint8_t status = NVDS_OK;
    nvds_addr_len_t cur_tag_addr, nxt_tag_addr;
    uint32_t max_access_addr;
    uint32_t tag_hdr = 0;

    if (nvds_env.conf_addr == NVDS_INVALID_FLASH_ADDRESS)
    {
        return NVDS_NO_TAG_ADDRESS;
    }

    nxt_tag_addr = NVDS_NEXT_PAGE_START_ADDRESS(nvds_env.conf_addr);
    max_access_addr = NVDS_SECTOR_MAX_ADDRESS(nvds_env.conf_addr);

    do
    {
        // go to the next tag
        cur_tag_addr = nxt_tag_addr;
        // retrieve the tag header
        status = _nvds_walk_tag(cur_tag_addr, NVDS_CONF_TAG_HEADER_LENGTH, (uint8_t *)&tag_hdr, &nxt_tag_addr);
#if NVDS_DEBUG_MODE
        if (status == NVDS_OK && sw_flag)
        {
            NVDS_PRINTF("[CONF] Tag ID:%3d,Length:%3d,CRC:---,SN:------,", ((struct conf_tag_header *)&tag_hdr)->tag, ((struct conf_tag_header *)&tag_hdr)->length);
            if (((struct conf_tag_header *)&tag_hdr)->length == 0)
            {
                NVDS_PRINTF("Data(Hex):Deleted\r\n");
            }
            else
            {
                _nvds_read(cur_tag_addr + NVDS_CONF_TAG_HEADER_LENGTH, ((struct conf_tag_header *)&tag_hdr)->length, (uint8_t *)tag_com_data);
                NVDS_PRINTF("Data(Hex):");
                _nvds_print_data(NVDS_PRINTF_PORT, ((struct conf_tag_header *)&tag_hdr)->length, (uint8_t *)tag_com_data);
                NVDS_PRINTF("\r\n");
            }
        }
#endif
        // Check if tag status is ok and tag id is valid
        if (status == NVDS_OK && tag == ((struct conf_tag_header *)&tag_hdr)->tag && tag_header != NULL)
        {
            memcpy(tag_header, &tag_hdr, NVDS_CONF_TAG_HEADER_LENGTH);
            return cur_tag_addr;
        }

    } while (status == NVDS_OK && nxt_tag_addr < (max_access_addr - NVDS_CONF_TAG_HEADER_LENGTH));

#if CFG_FILE_SIMULATION
    nvds_env.conf_spare_addr = nxt_tag_addr;
#endif

    return NVDS_NO_TAG_ADDRESS;
}

/**
 * @brief traversal BKUP sector and get bkup spare address and spare size
 *
 * @return none
 **/
static void _nvds_browse_bkup_sector(uint8_t *tag_map)
{
    uint8_t status = NVDS_OK;
    nvds_addr_len_t cur_tag_addr, nxt_tag_addr, max_access_addr;
    uint32_t tag_hdr[NVDS_TAG_HEADER_LENGTH / sizeof(uint32_t)] = {0x00};

    nxt_tag_addr = NVDS_NEXT_PAGE_START_ADDRESS(nvds_env.bkup_addr);
    max_access_addr = NVDS_SECTOR_MAX_ADDRESS(nvds_env.bkup_addr);

    do
    {
        // go to the next tag
        cur_tag_addr = nxt_tag_addr;
        // retrieve the tag header
        status = _nvds_walk_tag(cur_tag_addr, NVDS_TAG_HEADER_LENGTH, (uint8_t *)tag_hdr, &nxt_tag_addr);
#if NVDS_DEBUG_MODE
        if (status == NVDS_OK && sw_flag)
        {
            NVDS_PRINTF("[BKUP] Tag ID:%3d,Length:%3d,CRC:%3d,SN:%6d,", ((struct nvds_tag_header *)&tag_hdr)->tag, ((struct nvds_tag_header *)&tag_hdr)->length,
                        ((struct nvds_tag_header *)&tag_hdr)->crc, ((struct nvds_tag_header *)&tag_hdr)->sn);
            if (((struct nvds_tag_header *)&tag_hdr)->length == 0)
            {
                NVDS_PRINTF("Data(Hex):Deleted\r\n");
            }
            else
            {
                _nvds_read(cur_tag_addr + NVDS_TAG_HEADER_LENGTH, ((struct nvds_tag_header *)&tag_hdr)->length, (uint8_t *)tag_com_data);
                NVDS_PRINTF("Data(Hex):");
                _nvds_print_data(NVDS_PRINTF_PORT, ((struct nvds_tag_header *)&tag_hdr)->length, (uint8_t *)tag_com_data);
                NVDS_PRINTF("\r\n");
            }
            continue;
        }
#endif
        // update tag map
        if (status == NVDS_OK && tag_map != NULL)
        {
            tag_map[((struct nvds_tag_header *)tag_hdr)->tag / 8] |= (0x01 << (((struct nvds_tag_header *)tag_hdr)->tag % 8));
            continue;
        }

        // Check if tag status is ok and it is start page address
        if (status == NVDS_OK && NVDS_IS_PAGE_START_ADDRESS(cur_tag_addr))
        {
            if (tag_map == NULL)
            {
                nvds_env.bkup_mask = NVDS_SET_PAGE_START_ADDRESS_MASK(nvds_env.bkup_mask, NVDS_GET_PAGE_START_ADDRESS_MASK(nvds_env.bkup_addr, cur_tag_addr));
            }
        }
        // Check if it is start address of current page and status is not ok.
        else if (NVDS_IS_PAGE_START_ADDRESS(cur_tag_addr) && status != NVDS_OK && ((struct nvds_tag_header *)tag_hdr)->tag == NVDS_TAG_NIL)
        {
            break;
        }
        // There mey be no tag in current page or it is an invalid tag
        else
        {
            // Point to next start page address
            if (NVDS_IS_PAGE_START_ADDRESS(cur_tag_addr))
            {
                nxt_tag_addr = NVDS_UPDATE_SECTOR_ADDRESS(cur_tag_addr) + FLASH_PAGE_SIZE;
            }
            else if (status != NVDS_OK)
            {
                nxt_tag_addr = NVDS_UPDATE_SECTOR_ADDRESS(cur_tag_addr);
            }
        }

    } while (nxt_tag_addr < (max_access_addr - NVDS_TAG_HEADER_LENGTH));

    if (tag_map == NULL)
    {
        // Calculate the start spare address of backup sector
        nvds_env.bkup_spare_addr = nxt_tag_addr > max_access_addr ? max_access_addr : nxt_tag_addr;
        // Calculate the spare size of backup sector
        nvds_env.bkup_spare_size = max_access_addr - nvds_env.bkup_spare_addr;
    }
}

/**
 * @brief traversal NVDS sector and get nvds spare address
 *
 * @return none
 **/
static uint8_t _nvds_browse_nvds_sector(uint8_t *tag_map)
{
    uint8_t status = NVDS_OK;
    nvds_addr_len_t cur_tag_addr, nxt_tag_addr, max_access_addr;
    uint32_t tag_hdr[NVDS_TAG_HEADER_LENGTH / sizeof(uint32_t)] = {0};

    nxt_tag_addr = NVDS_NEXT_PAGE_START_ADDRESS(nvds_env.nvds_addr);
    max_access_addr = NVDS_SECTOR_MAX_ADDRESS(nvds_env.nvds_addr);
    nvds_env.nvds_used_size = 0;

    do
    {
        // go to the next tag
        cur_tag_addr = nxt_tag_addr;
        // retrieve the tag header
        status = _nvds_walk_tag(cur_tag_addr, NVDS_TAG_HEADER_LENGTH, (uint8_t *)tag_hdr, &nxt_tag_addr);
        // If status is not ok, that means this page is unused, exit loop
        if (status != NVDS_OK)
        {
            break;
        }
#if NVDS_DEBUG_MODE
        if (sw_flag)
        {
            NVDS_PRINTF("[NVDS] Tag ID:%3d,Length:%3d,CRC:%3d,SN:%6d,", ((struct nvds_tag_header *)&tag_hdr)->tag, ((struct nvds_tag_header *)&tag_hdr)->length,
                        ((struct nvds_tag_header *)&tag_hdr)->crc, ((struct nvds_tag_header *)&tag_hdr)->sn);
            if (((struct nvds_tag_header *)&tag_hdr)->length == 0)
            {
                NVDS_PRINTF("Data(Hex):Deleted\r\n");
            }
            else
            {
                _nvds_read(cur_tag_addr + NVDS_TAG_HEADER_LENGTH, ((struct nvds_tag_header *)&tag_hdr)->length, (uint8_t *)tag_com_data);
                NVDS_PRINTF("Data(Hex):");
                _nvds_print_data(NVDS_PRINTF_PORT, ((struct nvds_tag_header *)&tag_hdr)->length, (uint8_t *)tag_com_data);
                NVDS_PRINTF("\r\n");
            }
        }
#endif
        // update tag map
        if (tag_map != NULL)
        {
            tag_map[((struct nvds_tag_header *)tag_hdr)->tag / 8] |= (0x01 << (((struct nvds_tag_header *)tag_hdr)->tag % 8));
        }
        // If this address is page start address, set page start address mask
        else if (NVDS_IS_PAGE_START_ADDRESS(cur_tag_addr))
        {
            nvds_env.nvds_mask = NVDS_SET_PAGE_START_ADDRESS_MASK(nvds_env.nvds_mask, NVDS_GET_PAGE_START_ADDRESS_MASK(nvds_env.nvds_addr, cur_tag_addr));
        }

        // Point to next start page address
        nxt_tag_addr = NVDS_UPDATE_SECTOR_ADDRESS(nxt_tag_addr);

    } while (nxt_tag_addr < (max_access_addr - NVDS_TAG_HEADER_LENGTH));

    if (tag_map == NULL)
    {
        nvds_env.nvds_spare_addr = nxt_tag_addr;
    }
    return status;
}

/**
 * @brief get the specific tag from NVDS sector tail
 *
 * @param[in]  tag        tag index
 * @param[out] tag_hdr    tag header buffer
 *
 * @return valid tag address or NVDS_NO_TAG_ADDRESS
 **/
static nvds_addr_len_t _nvds_tail_browse_nvds_sector(uint8_t tag, struct nvds_tag_header *tag_header)
{
    uint8_t idx;
    nvds_addr_len_t cur_tag_addr;
    nvds_tag_len_t com_tag_len;
    uint32_t tag_hdr[NVDS_TAG_HEADER_LENGTH / sizeof(uint32_t)] = {0};

    idx = NVDS_GET_PAGE_START_ADDRESS_MASK(nvds_env.nvds_addr, nvds_env.nvds_spare_addr) - 1;

    while (idx != 0)
    {
        if (nvds_env.nvds_mask & (0x01 << idx))
        {
            cur_tag_addr = nvds_env.nvds_addr + FLASH_PAGE_SIZE * idx;
            _nvds_read(cur_tag_addr, NVDS_TAG_HEADER_LENGTH, (uint8_t *)tag_hdr);
            com_tag_len = NVDS_TAG_HEADER_LENGTH + ((struct nvds_tag_header *)tag_hdr)->length;

            if (tag == ((struct nvds_tag_header *)tag_hdr)->tag &&
                _nvds_check_crc(((struct nvds_tag_header *)tag_hdr)->crc, cur_tag_addr + NVDS_CRC_SIZE, com_tag_len - NVDS_CRC_SIZE) == NVDS_OK)
            {
                memcpy((uint8_t *)tag_header, (uint8_t *)tag_hdr, NVDS_TAG_HEADER_LENGTH);
                return cur_tag_addr;
            }
        }
        idx--;
    }

    return NVDS_NO_TAG_ADDRESS;
}

/**
 * @brief get the specific tag from BKUP sector tail
 *
 * @param[in]  tag        tag index
 * @param[out] tag_hdr    tag header buffer
 *
 * @return valid tag address or NVDS_NO_TAG_ADDRESS
 **/
static nvds_addr_len_t _nvds_tail_browse_bkup_sector(uint8_t tag, struct nvds_tag_header *tag_header)
{
    uint8_t idx, status = NVDS_FAIL;
    nvds_addr_len_t tag_addr = NVDS_NO_TAG_ADDRESS, cur_tag_addr, nxt_tag_addr, end;
    nvds_tag_len_t com_tag_len;
    uint32_t tag_hdr[NVDS_TAG_HEADER_LENGTH / sizeof(uint32_t)] = {0};

    idx = NVDS_GET_PAGE_START_ADDRESS_MASK(nvds_env.bkup_addr, nvds_env.bkup_spare_addr) - 1;
    end = nvds_env.bkup_spare_addr;

    while (idx != 0)
    {
        if (nvds_env.bkup_mask & (0x01 << idx))
        {
            nxt_tag_addr = nvds_env.bkup_addr + FLASH_PAGE_SIZE * idx;
            do
            {
                cur_tag_addr = nxt_tag_addr;
                status = _nvds_walk_tag(cur_tag_addr, NVDS_TAG_HEADER_LENGTH, (uint8_t *)tag_hdr, &nxt_tag_addr);
                if (status != NVDS_OK)
                {
                    break;
                }
                com_tag_len = NVDS_TAG_HEADER_LENGTH + ((struct nvds_tag_header *)tag_hdr)->length;
                if (tag == ((struct nvds_tag_header *)tag_hdr)->tag &&
                    _nvds_check_crc(((struct nvds_tag_header *)tag_hdr)->crc, cur_tag_addr + NVDS_CRC_SIZE, com_tag_len - NVDS_CRC_SIZE) == NVDS_OK)
                {
                    memcpy((uint8_t *)tag_header, (uint8_t *)tag_hdr, NVDS_TAG_HEADER_LENGTH);
                    tag_addr = cur_tag_addr;
                }

            } while (nxt_tag_addr < (end - NVDS_TAG_HEADER_LENGTH));
            end = nvds_env.bkup_addr + FLASH_PAGE_SIZE * idx;
        }

        if (tag_addr != NVDS_NO_TAG_ADDRESS)
        {
            return tag_addr;
        }

        idx--;
    }

    return NVDS_NO_TAG_ADDRESS;
}

/**
 * @brief get the specific tag address
 *
 * @param[in]  tag        tag index
 * @param[out] tag_hdr    tag header buffer
 *
 * @return valid tag address or NVDS_NO_TAG_ADDRESS
 **/
static nvds_addr_len_t _nvds_get_tag_addr(uint8_t tag, struct nvds_tag_header *tag_hdr)
{
    nvds_addr_len_t tag_addr = NVDS_NO_TAG_ADDRESS;

    if (tag_hdr == NULL)
    {
        return tag_addr;
    }

    tag_addr = _nvds_tail_browse_nvds_sector(tag, tag_hdr);
    if (!tag_addr)
    {
        tag_addr = _nvds_tail_browse_bkup_sector(tag, tag_hdr);
        if (!tag_addr)
        {
            tag_addr = _nvds_browse_conf_sector(tag, (struct conf_tag_header *)tag_hdr);
        }
    }

    return tag_addr;
}

/**
 * @brief check if current address is align
 *
 * @param[in]  addr    flash address
 *
 * @return 0x00~0x03
 **/
static uint8_t _nvds_check_align(uint32_t addr)
{
    uint8_t addr_offset;

    addr_offset = (addr & 0x000F) % 0x04;

    if (addr_offset != 0)
    {
        return (0x04 - addr_offset);
    }

    return addr_offset;
}

/**
 * @brief align read data
 *
 * @param[in]  tag_addr    the flash address of tag
 * @param[in]  addr_offset the offset of current tag address
 * @param[in]  data_len    buffer length
 * @param[out] buf         A pointer to the buffer allocated by the caller to be filled with data
 *
 * @return none
 **/
static void _nvds_align_read(uint32_t tag_addr, uint8_t addr_offset, uint16_t data_len, uint8_t *buf)
{
    uint32_t temp = 0;

    if (addr_offset != 0 && data_len > addr_offset)
    {
        _nvds_read(tag_addr, addr_offset, (uint8_t *)&temp);
        tag_addr += addr_offset;
        memcpy(buf, (uint8_t *)&temp, addr_offset);
        _nvds_read(tag_addr, data_len - addr_offset, buf + addr_offset);
    }
    else if (addr_offset != 0 && data_len <= addr_offset)
    {
        _nvds_read(tag_addr, data_len, (uint8_t *)&temp);
        memcpy(buf, (uint8_t *)&temp, data_len);
    }
    else
    {
        _nvds_read(tag_addr, data_len, buf);
    }
}

/**
 * @brief get all valid data from NVDS and BKUP sector
 *
 * @param[in]  start        start tag index
 * @param[out] start        after buffer be filled, current start tag index
 * @param[in]  end          the last tag index
 * @param[in]  tag_table    tag list. if it is null, read tag from start to end
 * @param[out] tag_offset   current start tag offset
 * @param[out] total_len    after buffer be filled, the buffer length will be callback
 * @param[out] prev_crc     current start tag crc
 * @param[out] buf          A pointer to the buffer allocated by the caller to be filled with valid data
 *                          Note: the maximum length is 256
 * @param[in] tag_map       mapping which tag is valid
 * @return errno
 **/
static uint8_t _nvds_get_all_tags_data(uint8_t *start, uint8_t end, uint8_t *tag_table, uint16_t *tag_offset, uint16_t *total_len, uint8_t *prev_crc, uint8_t *buf, uint8_t *tag_map)
{
    uint8_t idx, tag, crc, status = NVDS_OK, hdr_len = 0;
    nvds_tag_len_t offset = 0, data_offset, nxt_buf_offset = 0, com_tag_len, data_len;
    nvds_addr_len_t tag_addr, addr_offset;
    uint32_t tag_hdr_buf[NVDS_TAG_HEADER_LENGTH / sizeof(uint32_t)] = {0};

    if (*start > end)
    {
        return NVDS_FAIL;
    }

    if (tag_offset == NULL || total_len == NULL || buf == NULL)
    {
        return NVDS_NO_SPACE_AVAILABLE;
    }

    *total_len = FLASH_PAGE_SIZE;

    for (idx = *start; idx < end; idx++)
    {
        if (tag_table == NULL)
        {
            tag = idx;
            // Check if this is a valid tag
            if (tag_map != NULL && (tag_map[tag / 8] & (0x01 << (tag % 8))) == 0)
            {
                continue;
            }
        }
        else if (tag_table[idx] == NVDS_TAG_NIL)
        {
            *start = end;
            break;
        }
        else
        {
            tag = tag_table[idx];
        }

        if (tag >= NVDS_MAX_NUM_OF_TAGS)
        {
            return NVDS_TAG_NOT_DEFINED;
        }

        tag_addr = _nvds_get_tag_addr(tag, (struct nvds_tag_header *)tag_hdr_buf);

        if (tag_addr != NVDS_NO_TAG_ADDRESS && !_nvds_is_conf_tag(tag_addr))
        {
            com_tag_len = NVDS_TAG_HEADER_LENGTH + ((struct nvds_tag_header *)tag_hdr_buf)->length;

            // If there is a deleted tag in BKUP sector, it will be ignored
            if (tag_table == NULL && ((struct nvds_tag_header *)tag_hdr_buf)->length == 0)
            {
                memset((uint8_t *)tag_hdr_buf, 0, NVDS_TAG_HEADER_LENGTH);
                continue;
            }
            // Check if there is enough space to save the tag header
            if ((offset + NVDS_TAG_HEADER_LENGTH) >= FLASH_PAGE_SIZE)
            {
                // remain length of current page
                data_len = FLASH_PAGE_SIZE - offset;
                memcpy(buf + offset, (uint8_t *)tag_hdr_buf, data_len);
                // update start tag id
                *start = idx;
                // update tag offset
                *tag_offset = data_len;
                // if length is greater than crc size
                if (data_len > NVDS_CRC_SIZE)
                {
                    // update current crc
                    *prev_crc = _nvds_crc7(0, ((uint8_t *)tag_hdr_buf) + NVDS_CRC_SIZE, data_len - NVDS_CRC_SIZE);
                }
                else
                {
                    *prev_crc = 0;
                }
                return status;
            }
            else
            {
                // Check if this is unfinished tag
                if (*tag_offset != 0)
                {
                    // check if tag header was readed completely
                    if (*tag_offset < NVDS_TAG_HEADER_LENGTH)
                    {
                        hdr_len = NVDS_TAG_HEADER_LENGTH - *tag_offset;
                        memcpy(buf, ((uint8_t *)tag_hdr_buf) + *tag_offset, hdr_len);
                        crc = _nvds_crc7(*prev_crc, ((uint8_t *)tag_hdr_buf) + *tag_offset, hdr_len);
                    }
                    else
                    {
                        crc = *prev_crc;
                        hdr_len = 0;
                    }
                }
                else
                {
                    memcpy(buf + offset, (uint8_t *)tag_hdr_buf, NVDS_TAG_HEADER_LENGTH);
                    hdr_len = NVDS_TAG_HEADER_LENGTH;
                    crc = _nvds_crc7(0, ((uint8_t *)tag_hdr_buf) + NVDS_CRC_SIZE, hdr_len - NVDS_CRC_SIZE);
                }
            }
            // Check if tag was deleted
            if (((struct nvds_tag_header *)tag_hdr_buf)->length != 0)
            {
                // check if previous tag has not been read finished and tag offset is not less than tag header length
                if (*tag_offset != 0 && *tag_offset >= NVDS_TAG_HEADER_LENGTH)
                {
                    // current tag data offset
                    data_offset = *tag_offset - NVDS_TAG_HEADER_LENGTH;
                }
                else
                {
                    // data header
                    data_offset = 0;
                }
                // get the buffer offset for next tag
                nxt_buf_offset = offset + hdr_len + (uint32_t)(((struct nvds_tag_header *)tag_hdr_buf)->length) - data_offset;

                // offset tag address
                tag_addr += NVDS_TAG_HEADER_LENGTH + data_offset;

                // since tag address may not 4-bytes align, get address offset
                addr_offset = _nvds_check_align((uint32_t)(buf + offset + hdr_len));
                // check if remain space is enough for storing complete tag data
                if (nxt_buf_offset > FLASH_PAGE_SIZE)
                {
                    // get remain length in current page
                    data_len = FLASH_PAGE_SIZE - offset - hdr_len;
                    // storing to buffer
                    _nvds_align_read(tag_addr, addr_offset, data_len, buf + offset + hdr_len);
                    // update start tag id
                    *start = idx;
                    // current wrote tag data length + tag header + previous wrote tag data length
                    *tag_offset = data_len + NVDS_TAG_HEADER_LENGTH + data_offset;
                    // update crc
                    *prev_crc = _nvds_crc7(crc, buf + offset + hdr_len, data_len);
                    return status;
                }
                else
                {
                    // get remain length of current tag data
                    data_len = ((struct nvds_tag_header *)tag_hdr_buf)->length - data_offset;
                    _nvds_align_read(tag_addr, addr_offset, data_len, buf + offset + hdr_len);
                    crc = _nvds_crc7(crc, buf + offset + hdr_len, data_len);
                }
            }
            // Check if crc is valid
            if (((struct nvds_tag_header *)tag_hdr_buf)->crc != crc)
            {
                return NVDS_CRC_ERROR;
            }

            // If equal to flash page size, that means this tag is finished
            if (nxt_buf_offset == FLASH_PAGE_SIZE)
            {
                // update start tag id
                *start = ++idx;
                // clear tag offset
                *tag_offset = 0;
                return status;
            }
            // clear tag header buffer
            memset((uint8_t *)tag_hdr_buf, 0, NVDS_TAG_HEADER_LENGTH);
            // if tag offset is not 0, clear it and update the offset of current page
            if (*tag_offset != 0)
            {
                offset += com_tag_len - *tag_offset;
                *tag_offset = 0;
            }
            else
            {
                offset += com_tag_len;
            }
        }
        // update start tag id
        *start = idx + 1;
    }

    // check if current tag is the last one
    if (*start >= end || idx >= end)
    {
        // update total readed length
        *total_len = offset;
    }

    return status;
}
/**
 * @brief purge the data from current BKUP sector and copy valid data to new BKUP sector
 *
 * @param[out] buf_ptr    A pointer to the buffer allocated by the caller to be filled with all valid tag
 *
 * @return errno
 **/
static uint8_t nvds_purge_get_data_err = 0;
static uint8_t _nvds_purge(uint8_t *buf_ptr)
{
    nvds_tag_len_t total_len, next_tag_offset, read_len;
    uint8_t status = NVDS_OK, idx, crc;
    uint8_t start, end = NVDS_MAX_NUM_OF_TAGS;
    uint8_t tag_map[32] = {0};
    int32_t magic_num;

retry:
    total_len = start = next_tag_offset = crc = 0;
    idx = 1;
    // Erase nvds sector
    _nvds_erase(nvds_env.nvds_addr, NVDS_MAX_STORAGE_SIZE);

    // Get valid tag map
    _nvds_browse_nvds_sector(tag_map);
    _nvds_browse_bkup_sector(tag_map);

    do
    {
        // Clear read_len
        read_len = 0;
        // CLear buffer
        memset((uint8_t *)buf_ptr, 0, FLASH_PAGE_SIZE);
        // Get data of all valid tags
        status = _nvds_get_all_tags_data(&start, end, NULL, &next_tag_offset, &read_len, &crc, (uint8_t *)buf_ptr, tag_map);
        if (status != NVDS_OK)
        {
            // Erase nvds sector
            _nvds_format_sector(nvds_env.nvds_addr, NVDS_SECTOR);
            nvds_purge_get_data_err++;
            return status;
        }
        if (read_len == 0)
        {
            break;
        }
        //  Write tag data to new backup sector, if total_len equal to 0, do nothing
        _nvds_write(nvds_env.nvds_addr + FLASH_PAGE_SIZE * idx, (uint32_t)read_len, (uint8_t *)buf_ptr);
        // If write error occurs, erase nvds sector and re-purge
        if (_nvds_verify_write((uint8_t *)buf_ptr, nvds_env.nvds_addr + FLASH_PAGE_SIZE * idx, read_len)){
            goto retry;
        }
        total_len += read_len;
        idx++;
    } while (start < end && read_len == FLASH_PAGE_SIZE);

    // Since data purge finished, write magic to this sector
    magic_num = nvds_magic_number[BKUP_SECTOR];
    _nvds_write(nvds_env.nvds_addr,
                (uint32_t)NVDS_MAGIC_NUMBER_LENGTH,
                (uint8_t *)&magic_num);
    if (_nvds_verify_write((uint8_t *)&magic_num, nvds_env.nvds_addr, NVDS_MAGIC_NUMBER_LENGTH)){
        goto retry;
    }

    // Erase backup sector
    _nvds_format_sector(nvds_env.bkup_addr, NVDS_SECTOR);

    if (nvds_env.nvds_addr > nvds_env.bkup_addr)
    {
        nvds_env.nvds_addr -= FLASH_SECTOR_SIZE;
        nvds_env.bkup_addr += FLASH_SECTOR_SIZE;
    }
    else
    {
        nvds_env.nvds_addr += FLASH_SECTOR_SIZE;
        nvds_env.bkup_addr -= FLASH_SECTOR_SIZE;
    }

    nvds_env.nvds_spare_addr = NVDS_NEXT_PAGE_START_ADDRESS(nvds_env.nvds_addr);
    nvds_env.nvds_used_size = 0;
    nvds_env.nvds_mask = 0;
    memset(nvds_env.nvds_tag_record, NVDS_TAG_NIL, NVDS_NUM_OF_PAGES);

    nvds_env.bkup_spare_addr = NVDS_UPDATE_SECTOR_ADDRESS(nvds_env.bkup_addr + FLASH_PAGE_SIZE + total_len);
    nvds_env.bkup_mask = 0;
    nvds_env.bkup_mask = NVDS_SET_PAGE_START_ADDRESS_MASK(nvds_env.bkup_mask, NVDS_GET_PAGE_START_ADDRESS_MASK(nvds_env.bkup_addr, nvds_env.bkup_addr + FLASH_PAGE_SIZE));
    nvds_env.bkup_spare_size = nvds_env.bkup_addr + NVDS_MAX_STORAGE_SIZE - nvds_env.bkup_spare_addr;

    return status;
}

/**
 * @brief reclaim the data current NVDS sector into BKUP sector
 *
 * @param[out] buf_ptr    A pointer to the buffer allocated by the caller to be filled with all valid tag
 *
 * @return errno
 **/
static uint8_t nvds_reclaim_used_size_err = 0;
static uint8_t nvds_reclaim_get_data_err = 0;

static uint8_t _nvds_reclaim(uint8_t *buf_ptr)
{
    uint8_t status = NVDS_OK, idx = 0, crc = 0, write_status = NVDS_OK;
    uint8_t start = 0, end = NVDS_NUM_OF_PAGES;
    uint16_t read_len, total_len = 0, next_tag_offset = 0;

    if (nvds_env.nvds_used_size > nvds_env.bkup_spare_size || nvds_env.nvds_used_size == 0)
    {
        _nvds_update_used_size();
        if (nvds_env.nvds_used_size > nvds_env.bkup_spare_size || nvds_env.nvds_used_size == 0)
        {
            // Erase nvds sector
            _nvds_format_sector(nvds_env.nvds_addr, NVDS_SECTOR);
            nvds_reclaim_used_size_err++;
            return NVDS_OK;
        }
    }
retry:
    do
    {
        read_len = 0;
        memset(buf_ptr, 0, FLASH_PAGE_SIZE);
        status = _nvds_get_all_tags_data(&start, end, nvds_env.nvds_tag_record, &next_tag_offset, &read_len, &crc, buf_ptr, NULL);
        if (status != NVDS_OK)
        {
            status = _nvds_purge(buf_ptr);
            nvds_reclaim_get_data_err++;
            return status;
        }

        if (read_len == 0)
        {
            break;
        }
        // Reclaim tag data to backup sector
        _nvds_write(nvds_env.bkup_spare_addr + FLASH_PAGE_SIZE * idx, (uint32_t)read_len, buf_ptr);
        if (_nvds_verify_write(buf_ptr, nvds_env.bkup_spare_addr + FLASH_PAGE_SIZE * idx, read_len)){
            write_status = NVDS_WRITE_ERROR;
        }
        idx++;
        total_len += read_len;
    } while (start < end && read_len == FLASH_PAGE_SIZE);

    nvds_env.bkup_mask = NVDS_SET_PAGE_START_ADDRESS_MASK(nvds_env.bkup_mask, NVDS_GET_PAGE_START_ADDRESS_MASK(nvds_env.bkup_addr, nvds_env.bkup_spare_addr));

    // update the spare address for backup sector
    nvds_env.bkup_spare_addr = NVDS_UPDATE_SECTOR_ADDRESS(nvds_env.bkup_spare_addr + total_len);
    // update the spare size for backup sector
    nvds_env.bkup_spare_size = nvds_env.bkup_addr + NVDS_MAX_STORAGE_SIZE - nvds_env.bkup_spare_addr;
    // If a write error occurs and the remaining space in the BAUP area still allows reclaim, return an error
    if (write_status && (nvds_env.nvds_used_size <= nvds_env.bkup_spare_size)) {
        idx = crc = start = total_len = next_tag_offset = 0;
        goto retry;
    }
    // Clear used size
    nvds_env.nvds_used_size = 0;
    // Clear nvds mask
    nvds_env.nvds_mask = 0;
    // Clear spare address (First page is used for magic number)
    nvds_env.nvds_spare_addr = NVDS_NEXT_PAGE_START_ADDRESS(nvds_env.nvds_addr);
    // Since data was reclaimed, clear tag record
    memset(nvds_env.nvds_tag_record, NVDS_TAG_NIL, NVDS_NUM_OF_PAGES);

    return NVDS_OK;
}

/**
 * @brief initialize flash address for all sectors
 *
 * @return none
 **/
static void _nvds_init_each_sector_addr(void)
{
    // Check if config sector is valid
    if (_nvds_is_magic_number_ok(nvds_env.flash_base + 2 * FLASH_SECTOR_SIZE, CONF_SECTOR))
    {
        nvds_env.conf_addr = nvds_env.flash_base + 2 * FLASH_SECTOR_SIZE;
    }
    else
    {
#if CFG_FILE_SIMULATION
        nvds_env.conf_addr = nvds_env.flash_base + 2 * FLASH_SECTOR_SIZE;
        _nvds_format_sector(nvds_env.conf_addr, CONF_SECTOR);
#else
        nvds_env.conf_addr = NVDS_INVALID_FLASH_ADDRESS;
#endif
    }

    if (_nvds_is_magic_number_ok(nvds_env.flash_base, NVDS_SECTOR))
    {
        if (!_nvds_is_magic_number_ok(nvds_env.flash_base + FLASH_SECTOR_SIZE, BKUP_SECTOR))
        {
            _nvds_format_sector(nvds_env.flash_base + FLASH_SECTOR_SIZE, BKUP_SECTOR);
        }
        nvds_env.nvds_addr = nvds_env.flash_base;
    }
    else if (_nvds_is_magic_number_ok(nvds_env.flash_base, BKUP_SECTOR))
    {
        if (!_nvds_is_magic_number_ok(nvds_env.flash_base + FLASH_SECTOR_SIZE, NVDS_SECTOR))
        {
            _nvds_format_sector(nvds_env.flash_base + FLASH_SECTOR_SIZE, NVDS_SECTOR);
        }
        nvds_env.nvds_addr = nvds_env.flash_base + FLASH_SECTOR_SIZE;
    }
    else
    {
        if (_nvds_is_magic_number_ok(nvds_env.flash_base + FLASH_SECTOR_SIZE, NVDS_SECTOR))
        {
            _nvds_format_sector(nvds_env.flash_base, BKUP_SECTOR);
            nvds_env.nvds_addr = nvds_env.flash_base + FLASH_SECTOR_SIZE;
        }
        else if (_nvds_is_magic_number_ok(nvds_env.flash_base + FLASH_SECTOR_SIZE, BKUP_SECTOR))
        {
            _nvds_format_sector(nvds_env.flash_base, NVDS_SECTOR);
            nvds_env.nvds_addr = nvds_env.flash_base;
        }
        else
        {
            _nvds_format_sector(nvds_env.flash_base, NVDS_SECTOR);
            _nvds_format_sector(nvds_env.flash_base + FLASH_SECTOR_SIZE, BKUP_SECTOR);
            nvds_env.nvds_addr = nvds_env.flash_base;
        }
    }

    if (nvds_env.nvds_addr == nvds_env.flash_base)
    {
        nvds_env.bkup_addr = nvds_env.flash_base + FLASH_SECTOR_SIZE;
    }
    else
    {
        nvds_env.bkup_addr = nvds_env.flash_base;
    }
}

static int nvds_sf_detect(uint32_t *p_sector_shift)
{
    uint32_t start;
    flash_id_t flash_id;
    uint32_t capacity;

    drv_flash_read_id(OM_FLASH0, &flash_id);
    capacity = FLASH_ID2CAP(flash_id);

    if(p_sector_shift != NULL)
    {
        *p_sector_shift = NVDS_SF_SECTOR_SHIFT;
    }

    if (capacity == 0)
    {
        return -1;
    }

    /* the last sector is reserved for trace id & GPADC etc; then
       the 3 sectors of flash are used for cfg partition in default */
    start = capacity - ((1 + 3) << NVDS_SF_SECTOR_SHIFT);

    return start /*sector_start*/;
}

/*
 * EXPORTED FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 * @brief NVDS initialize
 *
 * @param[in] flash_base  The NVDS's base address in flash
 *
 * @return errno
 **/
uint8_t nvds_init(nvds_addr_len_t flash_base)
{
    uint8_t status;
    if (flash_base == 0)
    {
        int ret = 0;
#if CFG_FILE_SIMULATION
        ret = cfg_sim_init(FLASH_SECTOR_SIZE * (2 + 1));
        if (ret)
        {
            return NVDS_FAIL;
        }
#else
        // Get flash start address
        ret = nvds_sf_detect(NULL);
        if (ret >= 0)
        {
            flash_base = ret;
        }
        else
        {
            return NVDS_FAIL;
        }
#endif
    }

    // Setting NVDS base address
    nvds_env.flash_base = flash_base;

    // Initialize all sectors address
    _nvds_init_each_sector_addr();

#if CFG_FILE_SIMULATION
    // Get the flash address of each tag from config sector
    _nvds_browse_conf_sector(0xFF, NULL);
#endif
    // Get the flash address of each tag from backup sector
    _nvds_browse_bkup_sector(NULL);

    // Get the flash address of each tag from nvds sector
    status = _nvds_browse_nvds_sector(NULL);

    // Get current used size for NVDS sector
    _nvds_update_used_size();

    // If the NVDS sector is corrupted, reclaim it
    if (status == NVDS_CORRUPT) {
        uint32_t tag_buf[FLASH_PAGE_SIZE / sizeof(uint32_t)];
        _nvds_reclaim((uint8_t *)tag_buf);
        _nvds_format_sector(nvds_env.nvds_addr, NVDS_SECTOR);
    }

    return NVDS_OK;
}

/**
 * @brief get the newest tag
 *
 * @param[in] tag       tag index
 * @param[in] length    pdata buffer length, if it is null, ignore it.
 * @param[out]length'   real readed length, if length(in) > length(real),
 * @param[out]buf       A pointer to the buffer allocated by the caller to be filled with
 *                      the DATA part of the TAG
 *
 * @return errno
 **/
uint8_t nvds_get(nvds_tag_id_t tag, nvds_tag_len_t *length, void *buf)
{
    uint8_t status = NVDS_OK;
    uint32_t tag_hdr_buf[NVDS_TAG_HEADER_LENGTH / sizeof(uint32_t)] = {0};
    nvds_addr_len_t tag_addr = 0;
    nvds_tag_len_t tag_len = 0;

    if (nvds_env.flash_base == 0)
        return NVDS_NOINIT_ERROR;

    if (buf == NULL || length == NULL)
    {
        return NVDS_NO_SPACE_AVAILABLE;
    }

    if (*length > NVDS_DATA_MAX_LENGTH || *length == 0)
    {
        return NVDS_LENGTH_OUT_OF_RANGE;
    }

    tag_addr = _nvds_get_tag_addr(tag, (struct nvds_tag_header *)tag_hdr_buf);

    if (tag_addr != NVDS_NO_TAG_ADDRESS && !_nvds_is_conf_tag(tag_addr))
    {
        if (((struct nvds_tag_header *)tag_hdr_buf)->length != 0)
        {
            tag_len = ((struct nvds_tag_header *)tag_hdr_buf)->length >= *length ? *length : ((struct nvds_tag_header *)tag_hdr_buf)->length;
            _nvds_read(tag_addr + NVDS_TAG_HEADER_LENGTH, tag_len, buf);
            *length = tag_len;
            return status;
        }
    }

    if (_nvds_is_conf_tag(tag_addr))
    {
        _nvds_read(tag_addr, NVDS_CONF_TAG_HEADER_LENGTH, (uint8_t *)tag_hdr_buf);
        tag_len = ((struct conf_tag_header *)tag_hdr_buf)->length >= *length ? *length : ((struct conf_tag_header *)tag_hdr_buf)->length;
        _nvds_read(tag_addr + NVDS_CONF_TAG_HEADER_LENGTH, tag_len, buf);
        *length = tag_len;
    }
    else
    {
        status = _nvds_read_conf_tag(tag, length, buf);
    }

    return status;
}

/**
 * @brief delete tag
 *
 * @param[in] tag  tag index
 *
 * @return errno
 **/
uint8_t nvds_del(nvds_tag_id_t tag)
{
    if (nvds_env.flash_base == 0)
        return NVDS_NOINIT_ERROR;

    return nvds_put(tag, 0, NULL);
}

#if CFG_FILE_SIMULATION
uint8_t nvds_conf_put(nvds_tag_id_t tag, nvds_tag_len_t length, void *buf)
{
    uint32_t temp_tag_hdr = 0;
    uint32_t com_tag_buf[FLASH_PAGE_SIZE / sizeof(uint32_t)] = {0};
    nvds_tag_len_t com_tag_len, data_leftover, data_len;

    // Check if length is valid
    if (length > NVDS_DATA_MAX_LENGTH || length == 0)
    {
        return NVDS_LENGTH_OUT_OF_RANGE;
    }

    // The total length of current tag
    com_tag_len = NVDS_CONF_TAG_HEADER_LENGTH + length;

    // Check if buffer is valid
    if (buf == NULL || (nvds_env.conf_spare_addr + com_tag_len) >= (nvds_env.conf_addr + FLASH_SECTOR_SIZE))
    {
        return NVDS_NO_SPACE_AVAILABLE;
    }

    ((struct conf_tag_header *)&temp_tag_hdr)->tag = tag;
    ((struct conf_tag_header *)&temp_tag_hdr)->length = length;

    memcpy((uint8_t *)com_tag_buf, (uint8_t *)&temp_tag_hdr, NVDS_CONF_TAG_HEADER_LENGTH);

    data_len = length > (FLASH_PAGE_SIZE - NVDS_CONF_TAG_HEADER_LENGTH) ? FLASH_PAGE_SIZE - NVDS_CONF_TAG_HEADER_LENGTH : length;
    data_leftover = length - data_len;

    memcpy(((uint8_t *)com_tag_buf) + NVDS_CONF_TAG_HEADER_LENGTH, buf, data_len);
    _nvds_write(nvds_env.conf_spare_addr, com_tag_len - data_leftover, (uint8_t *)com_tag_buf);

    if (data_leftover)
    {
        memset(com_tag_buf, 0, FLASH_PAGE_SIZE);
        memcpy(com_tag_buf, buf + data_leftover, data_leftover);
        _nvds_write(nvds_env.conf_spare_addr + FLASH_PAGE_SIZE, data_leftover, (uint8_t *)com_tag_buf);
    }

    nvds_env.conf_spare_addr += com_tag_len;

    return NVDS_OK;
}

#endif

/**
 * @brief put tag to NVDS sector
 *
 * @param[in] tag       tag index
 * @param[in] length    data length, excluding tag node size, range in [1, 504].
 * @param[in] buf       Pointer to the buffer containing the DATA part of the TAG to add to
 *                      the NVDS
 *
 * @return errno
 **/
uint8_t nvds_put(nvds_tag_id_t tag, nvds_tag_len_t length, const void *buf)
{
    uint8_t status = NVDS_FAIL;
    struct nvds_tag_header tag_hdr;
    nvds_tag_len_t com_tag_len, data_len = 0, data_leftover = 0;
    uint32_t temp_tag_hdr[NVDS_TAG_HEADER_LENGTH / sizeof(uint32_t)] = {0};
    nvds_addr_len_t tag_addr, nvds_max_access_addr = NVDS_SECTOR_MAX_ADDRESS(nvds_env.nvds_addr);
    uint8_t *buf_ptr = (uint8_t *)buf;

    if (nvds_env.flash_base == 0)
        return NVDS_NOINIT_ERROR;

    // Check if length is valid
    if (length > NVDS_DATA_MAX_LENGTH)
    {
        return NVDS_LENGTH_OUT_OF_RANGE;
    }

    // Check if buffer is valid
    if (length != 0 && buf == NULL)
    {
        return NVDS_NO_SPACE_AVAILABLE;
    }

    uint32_t com_tag_buf[FLASH_PAGE_SIZE / sizeof(uint32_t)] = {0};

    // Set tag id
    tag_hdr.tag = tag;
    tag_addr = _nvds_get_tag_addr(tag, (struct nvds_tag_header *)temp_tag_hdr);
    // Check if tag is existed or it is config tag
    if (tag_addr == NVDS_NO_TAG_ADDRESS || _nvds_is_conf_tag(tag_addr))
    {
        // If this is delete process, do nothing
        if (length == 0)
        {
            return NVDS_OK;
        }
        tag_hdr.sn = 1;
    }
    else
    {
        // Check if this is a delete process and the tag was deleted
        if (((struct nvds_tag_header *)temp_tag_hdr)->length == 0 && length == 0)
        {
            return NVDS_OK;
        }
        tag_hdr.sn = ((struct nvds_tag_header *)temp_tag_hdr)->sn + 1;
    }
    // Set tag data length
    if (buf == NULL)
    {
        tag_hdr.length = 0;
    }
    else
    {
        tag_hdr.length = length;
    }

    // The total length of current tag
    com_tag_len = NVDS_TAG_HEADER_LENGTH + tag_hdr.length;

    // Check if nvds sector will be filled
    if (nvds_env.nvds_spare_addr >= nvds_max_access_addr ||
        (nvds_env.nvds_spare_addr + com_tag_len) >= nvds_max_access_addr)
    {
        // Reclaim nvds sector data to backup sector
        if (_nvds_reclaim((uint8_t *)com_tag_buf) != NVDS_OK)
        {
            goto exit;
        }
        // Format nvds sector
        _nvds_format_sector(nvds_env.nvds_addr, NVDS_SECTOR);
    }

    // Check if backup sector will be filled
    if ((nvds_env.nvds_used_size + com_tag_len) >= nvds_env.bkup_spare_size)
    {
        // Reclaim nvds sector data to backup sector
        if (nvds_env.nvds_used_size != 0)
        {
            status = _nvds_reclaim((uint8_t *)com_tag_buf);
            if (status != NVDS_OK)
            {
                return status;
            }
        }
        // Purge tag
        if (_nvds_purge((uint8_t *)com_tag_buf) != NVDS_OK)
        {
            goto exit;
        }
        // If still no enough space, return error
        if (com_tag_len >= nvds_env.bkup_spare_size)
        {
            status = NVDS_NO_SPACE_AVAILABLE;
            goto exit;
        }
    }

    memcpy((uint8_t *)com_tag_buf, &tag_hdr, NVDS_TAG_HEADER_LENGTH);

    if (tag_hdr.length != 0)
    {
        data_len = tag_hdr.length > (FLASH_PAGE_SIZE - NVDS_TAG_HEADER_LENGTH) ? FLASH_PAGE_SIZE - NVDS_TAG_HEADER_LENGTH : tag_hdr.length;
        data_leftover = tag_hdr.length - data_len;
        memcpy(((uint8_t *)com_tag_buf) + NVDS_TAG_HEADER_LENGTH, buf_ptr, data_len);
    }

    // Generate crc
    *((uint8_t *)com_tag_buf) = _nvds_generate_crc(0, ((uint8_t *)com_tag_buf) + NVDS_CRC_SIZE, com_tag_len - NVDS_CRC_SIZE - data_leftover);
    if (data_leftover != 0)
    {
        *((uint8_t *)com_tag_buf) = _nvds_generate_crc(*((uint8_t *)com_tag_buf), buf_ptr + data_len, data_leftover);
    }

    // Write tag
    _nvds_write(nvds_env.nvds_spare_addr, com_tag_len - data_leftover, (uint8_t *)com_tag_buf);
    if (_nvds_verify_write((uint8_t*)com_tag_buf, nvds_env.nvds_spare_addr, com_tag_len - data_leftover)) {
        status = NVDS_WRITE_ERROR;
    }
    // If the data length exceeds one page and no write error occurs
    if (data_leftover != 0)
    {
        _nvds_write(nvds_env.nvds_spare_addr + FLASH_PAGE_SIZE, data_leftover, buf_ptr + data_len);
        if (_nvds_verify_write(buf_ptr + data_len, nvds_env.nvds_spare_addr + FLASH_PAGE_SIZE, data_leftover)) {
            status = NVDS_WRITE_ERROR;
        }
    }
    // If a write error occurs, data reclamation is required
    if (status != NVDS_WRITE_ERROR) {
        nvds_env.nvds_mask = NVDS_SET_PAGE_START_ADDRESS_MASK(nvds_env.nvds_mask, NVDS_GET_PAGE_START_ADDRESS_MASK(nvds_env.nvds_addr, nvds_env.nvds_spare_addr));
        nvds_env.nvds_spare_addr = NVDS_UPDATE_SECTOR_ADDRESS(nvds_env.nvds_spare_addr + com_tag_len);
        _nvds_update_used_size();
        status = NVDS_OK;
    } else {
        _nvds_reclaim((uint8_t *)com_tag_buf);
        _nvds_format_sector(nvds_env.nvds_addr, NVDS_SECTOR);
    }

exit:

    return status;
}

static void _nvds_print_data(void *printf_dump_func, uint16_t data_len, uint8_t *data)
{
    void (*__printf)(const char *format, ...) = (void (*)(const char *format, ...))printf_dump_func;    //lint !e894

    for (uint16_t cnt = 0; cnt < data_len; cnt++)
    {
        __printf("%02x", *(data + cnt));
        if (cnt != (data_len - 1))
            __printf(",");
    }
}

/**
 * @brief Print debug information
 *
 * @param[in] printf_dump_func
 *
 * @return None
 **/
void nvds_dump(void *printf_dump_func)
{
    uint8_t idx, tag_hdr_len, status = NVDS_OK;
    nvds_addr_len_t tag_addr = 0;
    uint32_t temp_tag_hdr[NVDS_TAG_HEADER_LENGTH / sizeof(uint32_t)] = {0};
    uint32_t tag_data[FLASH_PAGE_SIZE / sizeof(uint32_t)] = {0};
    uint16_t data_len;
    uint8_t tag_map[32] = {0};
    uint32_t cur_time = 0;

    // Get valid tag map
    _nvds_browse_nvds_sector(tag_map);
    _nvds_browse_bkup_sector(tag_map);

    void (*__printf)(const char *format, ...) = (void (*)(const char *format, ...))printf_dump_func;    //lint !e611

    __printf("Err info: reclaim used size:%2d,reclaim get data:%2d,purge get data:%2d\r\n\r\n", nvds_reclaim_used_size_err, nvds_reclaim_get_data_err, nvds_purge_get_data_err);

    for (idx = 0; idx < NVDS_MAX_NUM_OF_TAGS; idx++)
    {
        // Check if this is a valid tag
        if ((tag_map[idx / 8] & (0x01 << (idx % 8))) == 0)
        {
            continue;
        }

        tag_addr = _nvds_get_tag_addr(idx, (struct nvds_tag_header *)temp_tag_hdr);
        if (status != NVDS_OK)
        {
            return;
        }

        if (tag_addr != 0)
        {
            // get current time
            cur_time = 0; //om_time();
            cur_time = ((uint32_t)((((uint64_t)(cur_time)) * 125) >> 12)); // tick->ms(pmu_timer 32768hz)

            if (_nvds_is_conf_tag(tag_addr))
            {
                tag_hdr_len = NVDS_CONF_TAG_HEADER_LENGTH;
                data_len = ((struct conf_tag_header *)temp_tag_hdr)->length;
                __printf("[%010d][NVDS] Tag ID:%3d,Length:%3d,CRC:---,SN:------,", cur_time,
                         ((struct conf_tag_header *)temp_tag_hdr)->tag,
                         ((struct conf_tag_header *)temp_tag_hdr)->length);
            }
            else
            {
                tag_hdr_len = NVDS_TAG_HEADER_LENGTH;
                data_len = ((struct nvds_tag_header *)temp_tag_hdr)->length;
                __printf("[%010d][NVDS] Tag ID:%3d,Length:%3d,CRC:%3d,SN:%6d,", cur_time, ((struct nvds_tag_header *)temp_tag_hdr)->tag,
                         ((struct nvds_tag_header *)temp_tag_hdr)->length, ((struct nvds_tag_header *)temp_tag_hdr)->crc, ((struct nvds_tag_header *)temp_tag_hdr)->sn);
            }

            if (data_len == 0)
            {
                __printf("Data(Hex):Deleted\r\n");
                continue;
            }

            __printf("Data(Hex):");

            if (data_len <= FLASH_PAGE_SIZE)
            {
                _nvds_read(tag_addr + tag_hdr_len, data_len, (uint8_t *)tag_data);
                _nvds_print_data(printf_dump_func, data_len, (uint8_t *)tag_data);
            }
            else
            {
                _nvds_read(tag_addr + tag_hdr_len, FLASH_PAGE_SIZE, (uint8_t *)tag_data);
                _nvds_print_data(printf_dump_func, FLASH_PAGE_SIZE, (uint8_t *)tag_data);
                memset((uint8_t *)tag_data, 0, FLASH_PAGE_SIZE);
                _nvds_read(tag_addr + tag_hdr_len + FLASH_PAGE_SIZE, data_len - FLASH_PAGE_SIZE, ((uint8_t *)tag_data));
                _nvds_print_data(printf_dump_func, data_len - FLASH_PAGE_SIZE, (uint8_t *)tag_data);
            }
            __printf("\r\n");
            memset((uint8_t *)tag_data, 0, FLASH_PAGE_SIZE);
        }
    }
#if NVDS_DEBUG_MODE
    sw_flag = true;
    __printf("Show all data in CONF sector\r\n");
    _nvds_browse_conf_sector(NVDS_TAG_NIL, NULL);
    __printf("Show all data in NVDS sector\r\n");
    _nvds_browse_nvds_sector(NULL);
    __printf("Show all data in BKUP sector\r\n");
    _nvds_browse_bkup_sector(NULL);
    sw_flag = false;
#endif
}

uint8_t nvds_reset(void)
{
    if (nvds_env.flash_base == 0)
        return NVDS_NOINIT_ERROR;

    _nvds_format_sector(nvds_env.nvds_addr, NVDS_SECTOR);
    _nvds_format_sector(nvds_env.bkup_addr, BKUP_SECTOR);

    return NVDS_OK;
}

/// @} NVDS
