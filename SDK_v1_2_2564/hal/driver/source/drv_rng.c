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
 * @brief    TRNG driver source file
 * @details  TRNG driver source file
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
#if (RTE_RNG)
#include <stddef.h>
#include "om_device.h"
#include "om_driver.h"


/*******************************************************************************
 * MACROS
 */
#define USE_TRNG                    1U

#if USE_TRNG
#define RNG_CLOCK_ENABLE(en)        DRV_RCC_CLOCK_ENABLE(RCC_CLK_TRNG, en)
#else
#define RNG_CLOCK_ENABLE(en)        DRV_RCC_CLOCK_ENABLE(RCC_CLK_RNG, en)
#endif


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
#if USE_TRNG
static void drv_trng_init(void)
{
    om_error_t err;

    if (OM_TRNG->ISR & TRNG_ISR_AFC_DONE_MASK) {
        return;
    }

    // calib 1Ghz
    OM_TRNG->CSR0 &= ~TRNG_CSR0_PD_3P3_MASK;
    DRV_DELAY_US(8);

    DRV_RCC_ANA_CLK_ENABLE();
    DRV_RCC_CLOCK_ENABLE(RCC_CLK_DAIF, 1U);
    uint8_t cko16m_st = OM_DAIF->CLK_CFG & DAIF_XTAL32M_EN_CKO16M_DIG_MASK;
    REGW1(&OM_DAIF->CLK_CFG, DAIF_XTAL32M_EN_CKO16M_DIG_MASK);

    OM_TRNG->CSR1 |= TRNG_CSR1_AFC_EN_MASK;
    DRV_DWT_WAIT_MS_UNTIL_TO(!(OM_TRNG->ISR & TRNG_ISR_AFC_DONE_MASK), 1000, err);

    // Restore
    if (cko16m_st) {
        REGW1(&OM_DAIF->CLK_CFG, DAIF_XTAL32M_EN_CKO16M_DIG_MASK);
    } else {
        REGW0(&OM_DAIF->CLK_CFG, DAIF_XTAL32M_EN_CKO16M_DIG_MASK);
    }
    DRV_RCC_ANA_CLK_RESTORE();

    (void)err;
}
#endif

/**
 *******************************************************************************
 * @brief Get random numbers
 *
 * @return Random numbers
 *******************************************************************************
 */
static uint32_t drv_rng_get(void)
{
    om_error_t err;
    uint32_t random_val = 0;

    #if USE_TRNG
    #define REPEAT_NUM      3U

    drv_trng_init();
    OM_CRITICAL_BEGIN();
    OM_TRNG->CSR0 &= ~TRNG_CSR0_PD_3P3_MASK;

    for (uint8_t i = 0; i < REPEAT_NUM; i++) {
        OM_TRNG->CSR1 |= TRNG_CSR1_GEN_EN_MASK;
        DRV_DWT_WAIT_MS_UNTIL_TO(!(OM_TRNG->ISR & TRNG_ISR_RND_VLD_MASK), 1000, err);
        OM_TRNG->ISR = TRNG_ISR_RND_VLD_MASK;
        random_val ^= (err == OM_ERROR_OK) ? OM_TRNG->RNDDR : 0x0;
    }
    OM_TRNG->CSR0 |= TRNG_CSR0_PD_3P3_MASK;
    OM_CRITICAL_END();
    #else
    DRV_DWT_WAIT_MS_UNTIL_TO(!(OM_RNG->RDY & RNG_READY_MASK), 1000, err);
    random_val = (err == OM_ERROR_OK) ? OM_RNG->RANDOM : 0x0;
    OM_RNG->RDY |= RNG_READY_CLR_MASK;
    #endif

    return random_val;
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief read random numbers
 *
 * @param[out] random   Pointer to buffer that stores random numbers
 * @param[in]  num      Number of random numbers
 *******************************************************************************
 */
void drv_rng_read(uint8_t *random, uint32_t num)
{
    RNG_CLOCK_ENABLE(1);

    for (uint32_t i = 0; i < num / 4; i++) {
        uint32_t tmp_val = drv_rng_get();
        memcpy(random + i * 4, &tmp_val, 4);
    }

    if (num % 4) {
        uint32_t val = drv_rng_get();
        memcpy(((uint8_t *)random) + num - num % 4, &val, num % 4);
    }

    RNG_CLOCK_ENABLE(0);
}


#endif  /* RTE_RNG */

/** @} */
