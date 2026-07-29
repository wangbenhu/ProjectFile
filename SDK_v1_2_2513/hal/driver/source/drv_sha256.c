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
 * @brief    SHA256 driver source file
 * @details  SHA256 driver source file, implemeted by hardware.
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


/*******************************************************************************
 * INCLUDES
 */
#include "RTE_driver.h"
#if (RTE_SHA256)
#include <stddef.h>
#include "om_device.h"
#include "om_driver.h"


/*******************************************************************************
 * MACROS
 */
#define SHA256_BLOCK_SIZE       64

#define SHA256_WAIT_FOR_VALID()                                                \
    do {                                                                       \
        while (!(OM_SHA256->STATUS & SHA256_STATUS_VALID_MASK));               \
    } while (0)


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct {
    uint32_t    data_byte_cnt;
    drv_state_t state;
    uint8_t     wait_hash_data_len;
} sha256_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
static sha256_env_t sha256_env = {
    .data_byte_cnt      = 0,
    .wait_hash_data_len = 0,
    .state              = DRV_STATE_STOP,
};


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
__STATIC_INLINE uint32_t endian_convert(uint32_t val)
{
    return (val & 0x000000FFU) << 24 |
           (val & 0x0000FF00U) << 8 |
           (val & 0x00FF0000U) >> 8 |
           (val & 0xFF000000U) >> 24;
}

static void sha256_start_block(void)
{
    if (sha256_env.state == DRV_STATE_START) {
        // Calculate First Block
        OM_SHA256->CTRL |= SHA256_CTRL_INIT_MASK;
        sha256_env.state = DRV_STATE_CONTINUE;
    } else {
        // Calculate Other Blocks
        OM_SHA256->CTRL |= SHA256_CTRL_NEXT_MASK;
    }
    SHA256_WAIT_FOR_VALID();
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Start SHA256 calculation.
 *
 * @param[in] om_sha256     Pointer to SHA256 peripheral
 *******************************************************************************
 */
void drv_sha256_start(void)
{
    sha256_env.data_byte_cnt = 0;
    sha256_env.state = DRV_STATE_START;
    sha256_env.wait_hash_data_len = 0;
    DRV_RCC_RESET((uint32_t)OM_SHA256);
}

/**
 *******************************************************************************
 * @brief Update SHA256 calculation.
 *
 * @param[in] om_sha256     Pointer to SHA256 peripheral
 * @param[in] data          Data to be hashed
 * @param[in] len           Length of data to be hashed
 *******************************************************************************
 */
void drv_sha256_update(const uint8_t *data, uint32_t len)
{
    if (len == 0) {
        return;
    }

    uint32_t total_len = sha256_env.wait_hash_data_len + len;
    // Read the last tail data
    uint32_t tail_data = OM_SHA256->MSG[sha256_env.wait_hash_data_len / 4];
    // The index into input data
    uint32_t data_index = 0;
    // The cumulative length of data written into the message register
    uint32_t bytes_written = sha256_env.wait_hash_data_len;
    // The index of the the message register array
    uint8_t msg_index = sha256_env.wait_hash_data_len / 4;

    while (bytes_written < total_len) {
        if (((bytes_written % 4) == 0) && ((bytes_written + 4) <= total_len)) {
            // Write 4-byte aligned block
            OM_SHA256->MSG[msg_index] = *(((uint32_t *)(data + data_index)));
            bytes_written += 4;
            data_index += 4;
        } else {
            // Handle byte-by-byte tail data
            tail_data |= (data[data_index] << (8 * (bytes_written % 4)));
            bytes_written++;
            data_index++;
            // When aligned to word, write tail data to message register
            if ((bytes_written % 4) == 0) {
                OM_SHA256->MSG[msg_index] = tail_data;
                tail_data = 0;
            }
        }

        msg_index = (bytes_written / 4) % 16;

        // Start block if aligned to SHA256_BLOCK_SIZE
        if ((bytes_written != 0) && (bytes_written % SHA256_BLOCK_SIZE == 0)) {
            sha256_env.data_byte_cnt += SHA256_BLOCK_SIZE;
            sha256_start_block();
        }
    }

    // Write remaining tail data if any
    if (bytes_written % 4) {
        OM_SHA256->MSG[msg_index] = tail_data;
    }

    // Update the current data length that still requires the hash
    sha256_env.wait_hash_data_len = total_len % SHA256_BLOCK_SIZE;
}
/**
 *******************************************************************************
 * @brief Finish SHA256 calculation.
 *
 * @param[in]  om_sha256     Pointer to the SHA256 peripheral
 * @param[out] hash          Hash result
 *******************************************************************************
 */
void drv_sha256_finish(uint8_t hash[32])
{
    if (sha256_env.wait_hash_data_len == 0) {
        OM_SHA256->MSG[0] = 0x80U;
        OM_SHA256->MSG[15] = endian_convert(sha256_env.data_byte_cnt << 3);
        sha256_start_block();
    } else {
        sha256_env.data_byte_cnt += sha256_env.wait_hash_data_len;
        // add 1 at the end of data
        OM_SHA256->MSG[sha256_env.wait_hash_data_len / 4] |= 0x80U << ((sha256_env.wait_hash_data_len & 0x03U) << 3);

        // 2. check if a new block with only length information is required
        if (sha256_env.wait_hash_data_len < 56) {
            // length information can be write in this block
            // add data length at the end of block
            OM_SHA256->MSG[15] = endian_convert(sha256_env.data_byte_cnt << 3);
        } else {
            // length information cannot be write in this block
            // start previous block
            sha256_start_block();
            // add data length at the end of block
            OM_SHA256->MSG[15] = endian_convert(sha256_env.data_byte_cnt << 3);
        }
        // 3. start last block
        sha256_start_block();
    }

    for (uint8_t i = 0; i < 8; i++) {
        #if (RTE_SHA256_USING_BIG_ENDIAN)
        ((uint32_t *)hash)[i] = OM_SHA256->DIG[i];
        #else
        ((uint32_t *)hash)[i] = endian_convert(OM_SHA256->DIG[7 - i]);
        #endif
    }

    DRV_RCC_CLOCK_ENABLE((uint32_t)OM_SHA256, 0);
}


#endif  /* (RTE_SHA256) */

/** @} */