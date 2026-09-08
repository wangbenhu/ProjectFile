/* Copyright 2014, Kenneth MacKay. Licensed under the BSD 2-clause license. */
#include "RTE_driver.h"
#if (RTE_ECDSA)
#include <stdlib.h>
#include "uECC.h"
#include "uECC_vli.h"
#include "om_driver.h"

#ifndef uECC_RNG_MAX_TRIES
    #define uECC_RNG_MAX_TRIES 64
#endif

#if OM_UECC_ENABLE_VLI_API
    #define uECC_VLI_API
#else
    #define uECC_VLI_API static
#endif

#if (OM_UECC_PLATFORM == OM_UECC_AVR) || \
    (OM_UECC_PLATFORM == OM_UECC_ARM) || \
    (OM_UECC_PLATFORM == OM_UECC_ARM_THUMB) || \
    (OM_UECC_PLATFORM == OM_UECC_ARM_THUMB2)
    #define CONCATX(a, ...) a ## __VA_ARGS__
    #define CONCAT(a, ...) CONCATX(a, __VA_ARGS__)

    #define STRX(a) #a
    #define STR(a) STRX(a)

    #define EVAL(...)  EVAL1(EVAL1(EVAL1(EVAL1(__VA_ARGS__))))
    #define EVAL1(...) EVAL2(EVAL2(EVAL2(EVAL2(__VA_ARGS__))))
    #define EVAL2(...) EVAL3(EVAL3(EVAL3(EVAL3(__VA_ARGS__))))
    #define EVAL3(...) EVAL4(EVAL4(EVAL4(EVAL4(__VA_ARGS__))))
    #define EVAL4(...) __VA_ARGS__

    #define DEC_1  0
    #define DEC_2  1
    #define DEC_3  2
    #define DEC_4  3
    #define DEC_5  4
    #define DEC_6  5
    #define DEC_7  6
    #define DEC_8  7
    #define DEC_9  8
    #define DEC_10 9
    #define DEC_11 10
    #define DEC_12 11
    #define DEC_13 12
    #define DEC_14 13
    #define DEC_15 14
    #define DEC_16 15
    #define DEC_17 16
    #define DEC_18 17
    #define DEC_19 18
    #define DEC_20 19
    #define DEC_21 20
    #define DEC_22 21
    #define DEC_23 22
    #define DEC_24 23
    #define DEC_25 24
    #define DEC_26 25
    #define DEC_27 26
    #define DEC_28 27
    #define DEC_29 28
    #define DEC_30 29
    #define DEC_31 30
    #define DEC_32 31

    #define DEC(N) CONCAT(DEC_, N)

    #define SECOND_ARG(_, val, ...) val
    #define SOME_CHECK_0 ~, 0
    #define GET_SECOND_ARG(...) SECOND_ARG(__VA_ARGS__, SOME,)
    #define SOME_OR_0(N) GET_SECOND_ARG(CONCAT(SOME_CHECK_, N))

    #define EMPTY(...)
    #define DEFER(...) __VA_ARGS__ EMPTY()

    #define REPEAT_NAME_0() REPEAT_0
    #define REPEAT_NAME_SOME() REPEAT_SOME
    #define REPEAT_0(...)
    #define REPEAT_SOME(N, stuff) DEFER(CONCAT(REPEAT_NAME_, SOME_OR_0(DEC(N))))()(DEC(N), stuff) stuff
    #define REPEAT(N, stuff) EVAL(REPEAT_SOME(N, stuff))

    #define REPEATM_NAME_0() REPEATM_0
    #define REPEATM_NAME_SOME() REPEATM_SOME
    #define REPEATM_0(...)
    #define REPEATM_SOME(N, macro) macro(N) \
        DEFER(CONCAT(REPEATM_NAME_, SOME_OR_0(DEC(N))))()(DEC(N), macro)
    #define REPEATM(N, macro) EVAL(REPEATM_SOME(N, macro))
#endif

#include "platform-specific.inc"

#if (OM_UECC_WORD_SIZE == 1)
    #if OM_UECC_SUPPORTS_SECP160R1
        #define uECC_MAX_WORDS 21 /* Due to the size of curve_n. */
    #endif
    #if OM_UECC_SUPPORTS_SECP192R1
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 24
    #endif
    #if OM_UECC_SUPPORTS_SECP224R1
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 28
    #endif
    #if (OM_UECC_SUPPORTS_SECP256R1 || OM_UECC_SUPPORTS_SECP256K1)
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 32
    #endif
#elif (OM_UECC_WORD_SIZE == 4)
    #if OM_UECC_SUPPORTS_SECP160R1
        #define uECC_MAX_WORDS 6 /* Due to the size of curve_n. */
    #endif
    #if OM_UECC_SUPPORTS_SECP192R1
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 6
    #endif
    #if OM_UECC_SUPPORTS_SECP224R1
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 7
    #endif
    #if (OM_UECC_SUPPORTS_SECP256R1 || OM_UECC_SUPPORTS_SECP256K1)
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 8
    #endif
#elif (OM_UECC_WORD_SIZE == 8)
    #if OM_UECC_SUPPORTS_SECP160R1
        #define uECC_MAX_WORDS 3
    #endif
    #if OM_UECC_SUPPORTS_SECP192R1
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 3
    #endif
    #if OM_UECC_SUPPORTS_SECP224R1
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 4
    #endif
    #if (OM_UECC_SUPPORTS_SECP256R1 || OM_UECC_SUPPORTS_SECP256K1)
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 4
    #endif
#endif /* OM_UECC_WORD_SIZE */

#define BITS_TO_WORDS(num_bits) ((num_bits + ((OM_UECC_WORD_SIZE * 8) - 1)) / (OM_UECC_WORD_SIZE * 8))
#define BITS_TO_BYTES(num_bits) ((num_bits + 7) / 8)

struct om_uecc_curve_t {
    wordcount_t num_words;
    wordcount_t num_bytes;
    bitcount_t num_n_bits;
    om_uecc_word_t p[uECC_MAX_WORDS];
    om_uecc_word_t n[uECC_MAX_WORDS];
    om_uecc_word_t G[uECC_MAX_WORDS * 2];
    om_uecc_word_t b[uECC_MAX_WORDS];
    void (*double_jacobian)(om_uecc_word_t * X1,
                            om_uecc_word_t * Y1,
                            om_uecc_word_t * Z1,
                            om_uecc_curve curve);
#if OM_UECC_SUPPORT_COMPRESSED_POINT
    void (*mod_sqrt)(om_uecc_word_t *a, om_uecc_curve curve);
#endif
    void (*x_side)(om_uecc_word_t *result, const om_uecc_word_t *x, om_uecc_curve curve);
#if (OM_UECC_OPTIMIZATION_LEVEL > 0)
    void (*mmod_fast)(om_uecc_word_t *result, om_uecc_word_t *product);
#endif
};

#if OM_UECC_VLI_NATIVE_LITTLE_ENDIAN
static void ecc_copy(uint8_t *dst,
                  const uint8_t *src,
                  unsigned num_bytes) {
    while (0 != num_bytes) {
        num_bytes--;
        dst[num_bytes] = src[num_bytes];
    }
}
#endif

static cmpresult_t uECC_vli_cmp_unsafe(const om_uecc_word_t *left,
                                       const om_uecc_word_t *right,
                                       wordcount_t num_words);

#if (OM_UECC_PLATFORM == OM_UECC_ARM || OM_UECC_PLATFORM == OM_UECC_ARM_THUMB || \
        OM_UECC_PLATFORM == OM_UECC_ARM_THUMB2)
    #include "asm_arm.inc"
#endif

#if (OM_UECC_PLATFORM == OM_UECC_AVR)
    #include "asm_avr.inc"
#endif

#define default_RNG_defined     0

#if default_RNG_defined
__WEAK int om_default_RNG(uint8_t *dest, unsigned size)
{
    unsigned i;

    for (i = 0; i < size; i++) {
        dest[i] = rand() & 0xFF;
    }

    return 1;
}

static om_uecc_rng_function g_rng_function = &om_default_RNG;
#else
static om_uecc_rng_function g_rng_function = 0;
#endif

void om_uecc_set_rng(om_uecc_rng_function rng_function) {
    g_rng_function = rng_function;
}

om_uecc_rng_function om_uecc_get_rng(void) {
    return g_rng_function;
}

int om_uecc_curve_private_key_size(om_uecc_curve curve) {
    return BITS_TO_BYTES(curve->num_n_bits);
}

int om_uecc_curve_public_key_size(om_uecc_curve curve) {
    return 2 * curve->num_bytes;
}

#if !asm_clear
uECC_VLI_API void om_uecc_vli_clear(om_uecc_word_t *vli, wordcount_t num_words) {
    wordcount_t i;
    for (i = 0; i < num_words; ++i) {
        vli[i] = 0;
    }
}
#endif /* !asm_clear */

/* Constant-time comparison to zero - secure way to compare long integers */
/* Returns 1 if vli == 0, 0 otherwise. */
uECC_VLI_API om_uecc_word_t om_uecc_vli_iszero(const om_uecc_word_t *vli, wordcount_t num_words) {
    om_uecc_word_t bits = 0;
    wordcount_t i;
    for (i = 0; i < num_words; ++i) {
        bits |= vli[i];
    }
    return (bits == 0);
}

/* Returns nonzero if bit 'bit' of vli is set. */
uECC_VLI_API om_uecc_word_t om_uecc_vli_testbit(const om_uecc_word_t *vli, bitcount_t bit) {
    return (vli[bit >> OM_UECC_WORD_BITS_SHIFT] & ((om_uecc_word_t)1 << (bit & OM_UECC_WORD_BITS_MASK)));
}

/* Counts the number of words in vli. */
static wordcount_t vli_numDigits(const om_uecc_word_t *vli, const wordcount_t max_words) {
    wordcount_t i;
    /* Search from the end until we find a non-zero digit.
       We do it in reverse because we expect that most digits will be nonzero. */
    for (i = max_words - 1; i >= 0 && vli[i] == 0; --i) {
    }

    return (i + 1);
}

/* Counts the number of bits required to represent vli. */
uECC_VLI_API bitcount_t om_uecc_vli_numbits(const om_uecc_word_t *vli, const wordcount_t max_words) {
    om_uecc_word_t i;
    om_uecc_word_t digit;

    wordcount_t num_digits = vli_numDigits(vli, max_words);
    if (num_digits == 0) {
        return 0;
    }

    digit = vli[num_digits - 1];
    for (i = 0; digit; ++i) {
        digit >>= 1;
    }

    return (((bitcount_t)(num_digits - 1) << OM_UECC_WORD_BITS_SHIFT) + i);
}

/* Sets dest = src. */
#if !asm_set
uECC_VLI_API void om_uecc_vli_set(om_uecc_word_t *dest, const om_uecc_word_t *src, wordcount_t num_words) {
    wordcount_t i;
    for (i = 0; i < num_words; ++i) {
        dest[i] = src[i];
    }
}
#endif /* !asm_set */

/* Returns sign of left - right. */
static cmpresult_t uECC_vli_cmp_unsafe(const om_uecc_word_t *left,
                                       const om_uecc_word_t *right,
                                       wordcount_t num_words) {
    wordcount_t i;
    for (i = num_words - 1; i >= 0; --i) {
        if (left[i] > right[i]) {
            return 1;
        } else if (left[i] < right[i]) {
            return -1;
        }
    }
    return 0;
}

/* Constant-time comparison function - secure way to compare long integers */
/* Returns one if left == right, zero otherwise. */
uECC_VLI_API om_uecc_word_t om_uecc_vli_equal(const om_uecc_word_t *left,
                                        const om_uecc_word_t *right,
                                        wordcount_t num_words) {
    om_uecc_word_t diff = 0;
    wordcount_t i;
    for (i = num_words - 1; i >= 0; --i) {
        diff |= (left[i] ^ right[i]);
    }
    return (diff == 0);
}

uECC_VLI_API om_uecc_word_t om_uecc_vli_sub(om_uecc_word_t *result,
                                      const om_uecc_word_t *left,
                                      const om_uecc_word_t *right,
                                      wordcount_t num_words);

/* Returns sign of left - right, in constant time. */
uECC_VLI_API cmpresult_t om_uecc_vli_cmp(const om_uecc_word_t *left,
                                      const om_uecc_word_t *right,
                                      wordcount_t num_words) {
    om_uecc_word_t tmp[uECC_MAX_WORDS];
    om_uecc_word_t neg = !!om_uecc_vli_sub(tmp, left, right, num_words);
    om_uecc_word_t equal = om_uecc_vli_iszero(tmp, num_words);
    return (!equal - 2 * neg);
}

/* Computes vli = vli >> 1. */
#if !asm_rshift1
uECC_VLI_API void om_uecc_vli_rshift1(om_uecc_word_t *vli, wordcount_t num_words) {
    om_uecc_word_t *end = vli;
    om_uecc_word_t carry = 0;

    vli += num_words;
    while (vli-- > end) {
        om_uecc_word_t temp = *vli;
        *vli = (temp >> 1) | carry;
        carry = temp << (OM_UECC_WORD_BITS - 1);
    }
}
#endif /* !asm_rshift1 */

/* Computes result = left + right, returning carry. Can modify in place. */
#if !asm_add
uECC_VLI_API om_uecc_word_t om_uecc_vli_add(om_uecc_word_t *result,
                                      const om_uecc_word_t *left,
                                      const om_uecc_word_t *right,
                                      wordcount_t num_words) {
    om_uecc_word_t carry = 0;
    wordcount_t i;
    for (i = 0; i < num_words; ++i) {
        om_uecc_word_t sum = left[i] + right[i] + carry;
        if (sum != left[i]) {
            carry = (sum < left[i]);
        }
        result[i] = sum;
    }
    return carry;
}
#endif /* !asm_add */

/* Computes result = left - right, returning borrow. Can modify in place. */
#if !asm_sub
uECC_VLI_API om_uecc_word_t om_uecc_vli_sub(om_uecc_word_t *result,
                                      const om_uecc_word_t *left,
                                      const om_uecc_word_t *right,
                                      wordcount_t num_words) {
    om_uecc_word_t borrow = 0;
    wordcount_t i;
    for (i = 0; i < num_words; ++i) {
        om_uecc_word_t diff = left[i] - right[i] - borrow;
        if (diff != left[i]) {
            borrow = (diff > left[i]);
        }
        result[i] = diff;
    }
    return borrow;
}
#endif /* !asm_sub */

#if !asm_mult || (OM_UECC_SQUARE_FUNC && !asm_square) || \
    (OM_UECC_SUPPORTS_SECP256K1 && (OM_UECC_OPTIMIZATION_LEVEL > 0) && \
        ((OM_UECC_WORD_SIZE == 1) || (OM_UECC_WORD_SIZE == 8)))
static void muladd(om_uecc_word_t a,
                   om_uecc_word_t b,
                   om_uecc_word_t *r0,
                   om_uecc_word_t *r1,
                   om_uecc_word_t *r2) {
#if OM_UECC_WORD_SIZE == 8 && !SUPPORTS_INT128
    uint64_t a0 = a & 0xffffffffull;
    uint64_t a1 = a >> 32;
    uint64_t b0 = b & 0xffffffffull;
    uint64_t b1 = b >> 32;

    uint64_t i0 = a0 * b0;
    uint64_t i1 = a0 * b1;
    uint64_t i2 = a1 * b0;
    uint64_t i3 = a1 * b1;

    uint64_t p0, p1;

    i2 += (i0 >> 32);
    i2 += i1;
    if (i2 < i1) { /* overflow */
        i3 += 0x100000000ull;
    }

    p0 = (i0 & 0xffffffffull) | (i2 << 32);
    p1 = i3 + (i2 >> 32);

    *r0 += p0;
    *r1 += (p1 + (*r0 < p0));
    *r2 += ((*r1 < p1) || (*r1 == p1 && *r0 < p0));
#else
    om_uecc_dword_t p = (om_uecc_dword_t)a * b;
    om_uecc_dword_t r01 = ((om_uecc_dword_t)(*r1) << OM_UECC_WORD_BITS) | *r0;
    r01 += p;
    *r2 += (r01 < p);
    *r1 = r01 >> OM_UECC_WORD_BITS;
    *r0 = (om_uecc_word_t)r01;
#endif
}
#endif /* muladd needed */

#if !asm_mult
uECC_VLI_API void om_uecc_vli_mult(om_uecc_word_t *result,
                                const om_uecc_word_t *left,
                                const om_uecc_word_t *right,
                                wordcount_t num_words) {
    om_uecc_word_t r0 = 0;
    om_uecc_word_t r1 = 0;
    om_uecc_word_t r2 = 0;
    wordcount_t i, k;

    /* Compute each digit of result in sequence, maintaining the carries. */
    for (k = 0; k < num_words; ++k) {
        for (i = 0; i <= k; ++i) {
            muladd(left[i], right[k - i], &r0, &r1, &r2);
        }
        result[k] = r0;
        r0 = r1;
        r1 = r2;
        r2 = 0;
    }
    for (k = num_words; k < num_words * 2 - 1; ++k) {
        for (i = (k + 1) - num_words; i < num_words; ++i) {
            muladd(left[i], right[k - i], &r0, &r1, &r2);
        }
        result[k] = r0;
        r0 = r1;
        r1 = r2;
        r2 = 0;
    }
    result[num_words * 2 - 1] = r0;
}
#endif /* !asm_mult */

#if OM_UECC_SQUARE_FUNC

#if !asm_square
static void mul2add(om_uecc_word_t a,
                    om_uecc_word_t b,
                    om_uecc_word_t *r0,
                    om_uecc_word_t *r1,
                    om_uecc_word_t *r2) {
#if OM_UECC_WORD_SIZE == 8 && !SUPPORTS_INT128
    uint64_t a0 = a & 0xffffffffull;
    uint64_t a1 = a >> 32;
    uint64_t b0 = b & 0xffffffffull;
    uint64_t b1 = b >> 32;

    uint64_t i0 = a0 * b0;
    uint64_t i1 = a0 * b1;
    uint64_t i2 = a1 * b0;
    uint64_t i3 = a1 * b1;

    uint64_t p0, p1;

    i2 += (i0 >> 32);
    i2 += i1;
    if (i2 < i1)
    { /* overflow */
        i3 += 0x100000000ull;
    }

    p0 = (i0 & 0xffffffffull) | (i2 << 32);
    p1 = i3 + (i2 >> 32);

    *r2 += (p1 >> 63);
    p1 = (p1 << 1) | (p0 >> 63);
    p0 <<= 1;

    *r0 += p0;
    *r1 += (p1 + (*r0 < p0));
    *r2 += ((*r1 < p1) || (*r1 == p1 && *r0 < p0));
#else
    om_uecc_dword_t p = (om_uecc_dword_t)a * b;
    om_uecc_dword_t r01 = ((om_uecc_dword_t)(*r1) << OM_UECC_WORD_BITS) | *r0;
    *r2 += (p >> (OM_UECC_WORD_BITS * 2 - 1));
    p *= 2;
    r01 += p;
    *r2 += (r01 < p);
    *r1 = r01 >> OM_UECC_WORD_BITS;
    *r0 = (om_uecc_word_t)r01;
#endif
}

uECC_VLI_API void om_uecc_vli_square(om_uecc_word_t *result,
                                  const om_uecc_word_t *left,
                                  wordcount_t num_words) {
    om_uecc_word_t r0 = 0;
    om_uecc_word_t r1 = 0;
    om_uecc_word_t r2 = 0;

    wordcount_t i, k;

    for (k = 0; k < num_words * 2 - 1; ++k) {
        om_uecc_word_t min = (k < num_words ? 0 : (k + 1) - num_words);
        for (i = min; i <= k && i <= k - i; ++i) {
            if (i < k-i) {
                mul2add(left[i], left[k - i], &r0, &r1, &r2);
            } else {
                muladd(left[i], left[k - i], &r0, &r1, &r2);
            }
        }
        result[k] = r0;
        r0 = r1;
        r1 = r2;
        r2 = 0;
    }

    result[num_words * 2 - 1] = r0;
}
#endif /* !asm_square */

#else /* OM_UECC_SQUARE_FUNC */

#if OM_UECC_ENABLE_VLI_API
uECC_VLI_API void om_uecc_vli_square(om_uecc_word_t *result,
                                  const om_uecc_word_t *left,
                                  wordcount_t num_words) {
    om_uecc_vli_mult(result, left, left, num_words);
}
#endif /* OM_UECC_ENABLE_VLI_API */

#endif /* OM_UECC_SQUARE_FUNC */

/* Computes result = (left + right) % mod.
   Assumes that left < mod and right < mod, and that result does not overlap mod. */
uECC_VLI_API void om_uecc_vli_modadd(om_uecc_word_t *result,
                                  const om_uecc_word_t *left,
                                  const om_uecc_word_t *right,
                                  const om_uecc_word_t *mod,
                                  wordcount_t num_words) {
    om_uecc_word_t carry = om_uecc_vli_add(result, left, right, num_words);
    if (carry || uECC_vli_cmp_unsafe(mod, result, num_words) != 1) {
        /* result > mod (result = mod + remainder), so subtract mod to get remainder. */
        om_uecc_vli_sub(result, result, mod, num_words);
    }
}

/* Computes result = (left - right) % mod.
   Assumes that left < mod and right < mod, and that result does not overlap mod. */
uECC_VLI_API void om_uecc_vli_modsub(om_uecc_word_t *result,
                                  const om_uecc_word_t *left,
                                  const om_uecc_word_t *right,
                                  const om_uecc_word_t *mod,
                                  wordcount_t num_words) {
    om_uecc_word_t l_borrow = om_uecc_vli_sub(result, left, right, num_words);
    if (l_borrow) {
        /* In this case, result == -diff == (max int) - diff. Since -x % d == d - x,
           we can get the correct result from result + mod (with overflow). */
        om_uecc_vli_add(result, result, mod, num_words);
    }
}

/* Computes result = product % mod, where product is 2N words long. */
/* Currently only designed to work for curve_p or curve_n. */
uECC_VLI_API void om_uecc_vli_mmod(om_uecc_word_t *result,
                                om_uecc_word_t *product,
                                const om_uecc_word_t *mod,
                                wordcount_t num_words) {
    om_uecc_word_t mod_multiple[2 * uECC_MAX_WORDS];
    om_uecc_word_t tmp[2 * uECC_MAX_WORDS];
    om_uecc_word_t *v[2] = {tmp, product};
    om_uecc_word_t index;

    /* Shift mod so its highest set bit is at the maximum position. */
    bitcount_t shift = (num_words * 2 * OM_UECC_WORD_BITS) - om_uecc_vli_numbits(mod, num_words);
    wordcount_t word_shift = shift / OM_UECC_WORD_BITS;
    wordcount_t bit_shift = shift % OM_UECC_WORD_BITS;
    om_uecc_word_t carry = 0;
    om_uecc_vli_clear(mod_multiple, word_shift);
    if (bit_shift > 0) {
        for(index = 0; index < (om_uecc_word_t)num_words; ++index) {
            mod_multiple[word_shift + index] = (mod[index] << bit_shift) | carry;
            carry = mod[index] >> (OM_UECC_WORD_BITS - bit_shift);
        }
    } else {
        om_uecc_vli_set(mod_multiple + word_shift, mod, num_words);
    }

    for (index = 1; shift >= 0; --shift) {
        om_uecc_word_t borrow = 0;
        wordcount_t i;
        for (i = 0; i < num_words * 2; ++i) {
            om_uecc_word_t diff = v[index][i] - mod_multiple[i] - borrow;
            if (diff != v[index][i]) {
                borrow = (diff > v[index][i]);
            }
            v[1 - index][i] = diff;
        }
        index = !(index ^ borrow); /* Swap the index if there was no borrow */
        om_uecc_vli_rshift1(mod_multiple, num_words);
        mod_multiple[num_words - 1] |= mod_multiple[num_words] << (OM_UECC_WORD_BITS - 1);
        om_uecc_vli_rshift1(mod_multiple + num_words, num_words);
    }
    om_uecc_vli_set(result, v[index], num_words);
}

/* Computes result = (left * right) % mod. */
uECC_VLI_API void om_uecc_vli_modmult(om_uecc_word_t *result,
                                   const om_uecc_word_t *left,
                                   const om_uecc_word_t *right,
                                   const om_uecc_word_t *mod,
                                   wordcount_t num_words) {
    om_uecc_word_t product[2 * uECC_MAX_WORDS];
    om_uecc_vli_mult(product, left, right, num_words);
    om_uecc_vli_mmod(result, product, mod, num_words);
}

uECC_VLI_API void om_uecc_vli_modmult_fast(om_uecc_word_t *result,
                                        const om_uecc_word_t *left,
                                        const om_uecc_word_t *right,
                                        om_uecc_curve curve) {
    om_uecc_word_t product[2 * uECC_MAX_WORDS];
    om_uecc_vli_mult(product, left, right, curve->num_words);
#if (OM_UECC_OPTIMIZATION_LEVEL > 0)
    curve->mmod_fast(result, product);
#else
    om_uecc_vli_mmod(result, product, curve->p, curve->num_words);
#endif
}

#if OM_UECC_SQUARE_FUNC

#if OM_UECC_ENABLE_VLI_API
/* Computes result = left^2 % mod. */
uECC_VLI_API void om_uecc_vli_modsquare(om_uecc_word_t *result,
                                     const om_uecc_word_t *left,
                                     const om_uecc_word_t *mod,
                                     wordcount_t num_words) {
    om_uecc_word_t product[2 * uECC_MAX_WORDS];
    om_uecc_vli_square(product, left, num_words);
    om_uecc_vli_mmod(result, product, mod, num_words);
}
#endif /* OM_UECC_ENABLE_VLI_API */

uECC_VLI_API void om_uecc_vli_modsquare_fast(om_uecc_word_t *result,
                                          const om_uecc_word_t *left,
                                          om_uecc_curve curve) {
    om_uecc_word_t product[2 * uECC_MAX_WORDS];
    om_uecc_vli_square(product, left, curve->num_words);
#if (OM_UECC_OPTIMIZATION_LEVEL > 0)
    curve->mmod_fast(result, product);
#else
    om_uecc_vli_mmod(result, product, curve->p, curve->num_words);
#endif
}

#else /* OM_UECC_SQUARE_FUNC */

#if OM_UECC_ENABLE_VLI_API
uECC_VLI_API void om_uecc_vli_modsquare(om_uecc_word_t *result,
                                     const om_uecc_word_t *left,
                                     const om_uecc_word_t *mod,
                                     wordcount_t num_words) {
    om_uecc_vli_modmult(result, left, left, mod, num_words);
}
#endif /* OM_UECC_ENABLE_VLI_API */

uECC_VLI_API void om_uecc_vli_modsquare_fast(om_uecc_word_t *result,
                                          const om_uecc_word_t *left,
                                          om_uecc_curve curve) {
    om_uecc_vli_modmult_fast(result, left, left, curve);
}

#endif /* OM_UECC_SQUARE_FUNC */

#define EVEN(vli) (!(vli[0] & 1))
static void vli_modInv_update(om_uecc_word_t *uv,
                              const om_uecc_word_t *mod,
                              wordcount_t num_words) {
    om_uecc_word_t carry = 0;
    if (!EVEN(uv)) {
        carry = om_uecc_vli_add(uv, uv, mod, num_words);
    }
    om_uecc_vli_rshift1(uv, num_words);
    if (carry) {
        uv[num_words - 1] |= OM_HIGH_BIT_SET;
    }
}

/* Computes result = (1 / input) % mod. All VLIs are the same size.
   See "From Euclid's GCD to Montgomery Multiplication to the Great Divide" */
uECC_VLI_API void om_uecc_vli_modInv(om_uecc_word_t *result,
                                  const om_uecc_word_t *input,
                                  const om_uecc_word_t *mod,
                                  wordcount_t num_words) {
    om_uecc_word_t a[uECC_MAX_WORDS], b[uECC_MAX_WORDS], u[uECC_MAX_WORDS], v[uECC_MAX_WORDS];
    cmpresult_t cmpResult;

    if (om_uecc_vli_iszero(input, num_words)) {
        om_uecc_vli_clear(result, num_words);
        return;
    }

    om_uecc_vli_set(a, input, num_words);
    om_uecc_vli_set(b, mod, num_words);
    om_uecc_vli_clear(u, num_words);
    u[0] = 1;
    om_uecc_vli_clear(v, num_words);
    while ((cmpResult = uECC_vli_cmp_unsafe(a, b, num_words)) != 0) {
        if (EVEN(a)) {
            om_uecc_vli_rshift1(a, num_words);
            vli_modInv_update(u, mod, num_words);
        } else if (EVEN(b)) {
            om_uecc_vli_rshift1(b, num_words);
            vli_modInv_update(v, mod, num_words);
        } else if (cmpResult > 0) {
            om_uecc_vli_sub(a, a, b, num_words);
            om_uecc_vli_rshift1(a, num_words);
            if (uECC_vli_cmp_unsafe(u, v, num_words) < 0) {
                om_uecc_vli_add(u, u, mod, num_words);
            }
            om_uecc_vli_sub(u, u, v, num_words);
            vli_modInv_update(u, mod, num_words);
        } else {
            om_uecc_vli_sub(b, b, a, num_words);
            om_uecc_vli_rshift1(b, num_words);
            if (uECC_vli_cmp_unsafe(v, u, num_words) < 0) {
                om_uecc_vli_add(v, v, mod, num_words);
            }
            om_uecc_vli_sub(v, v, u, num_words);
            vli_modInv_update(v, mod, num_words);
        }
    }
    om_uecc_vli_set(result, u, num_words);
}

/* ------ Point operations ------ */

#include "curve-specific.inc"

/* Returns 1 if 'point' is the point at infinity, 0 otherwise. */
#define EccPoint_isZero(point, curve) om_uecc_vli_iszero((point), (curve)->num_words * 2)

/* Point multiplication algorithm using Montgomery's ladder with co-Z coordinates.
From http://eprint.iacr.org/2011/338.pdf
*/

/* Modify (x1, y1) => (x1 * z^2, y1 * z^3) */
static void apply_z(om_uecc_word_t * X1,
                    om_uecc_word_t * Y1,
                    const om_uecc_word_t * const Z,
                    om_uecc_curve curve) {
    om_uecc_word_t t1[uECC_MAX_WORDS];

    om_uecc_vli_modsquare_fast(t1, Z, curve);    /* z^2 */
    om_uecc_vli_modmult_fast(X1, X1, t1, curve); /* x1 * z^2 */
    om_uecc_vli_modmult_fast(t1, t1, Z, curve);  /* z^3 */
    om_uecc_vli_modmult_fast(Y1, Y1, t1, curve); /* y1 * z^3 */
}

/* P = (x1, y1) => 2P, (x2, y2) => P' */
static void XYcZ_initial_double(om_uecc_word_t * X1,
                                om_uecc_word_t * Y1,
                                om_uecc_word_t * X2,
                                om_uecc_word_t * Y2,
                                const om_uecc_word_t * const initial_Z,
                                om_uecc_curve curve) {
    om_uecc_word_t z[uECC_MAX_WORDS];
    wordcount_t num_words = curve->num_words;
    if (initial_Z) {
        om_uecc_vli_set(z, initial_Z, num_words);
    } else {
        om_uecc_vli_clear(z, num_words);
        z[0] = 1;
    }

    om_uecc_vli_set(X2, X1, num_words);
    om_uecc_vli_set(Y2, Y1, num_words);

    apply_z(X1, Y1, z, curve);
    curve->double_jacobian(X1, Y1, z, curve);
    apply_z(X2, Y2, z, curve);
}

/* Input P = (x1, y1, Z), Q = (x2, y2, Z)
   Output P' = (x1', y1', Z3), P + Q = (x3, y3, Z3)
   or P => P', Q => P + Q
*/
static void XYcZ_add(om_uecc_word_t * X1,
                     om_uecc_word_t * Y1,
                     om_uecc_word_t * X2,
                     om_uecc_word_t * Y2,
                     om_uecc_curve curve) {
    /* t1 = X1, t2 = Y1, t3 = X2, t4 = Y2 */
    om_uecc_word_t t5[uECC_MAX_WORDS];
    wordcount_t num_words = curve->num_words;

    om_uecc_vli_modsub(t5, X2, X1, curve->p, num_words); /* t5 = x2 - x1 */
    om_uecc_vli_modsquare_fast(t5, t5, curve);                  /* t5 = (x2 - x1)^2 = A */
    om_uecc_vli_modmult_fast(X1, X1, t5, curve);                /* t1 = x1*A = B */
    om_uecc_vli_modmult_fast(X2, X2, t5, curve);                /* t3 = x2*A = C */
    om_uecc_vli_modsub(Y2, Y2, Y1, curve->p, num_words); /* t4 = y2 - y1 */
    om_uecc_vli_modsquare_fast(t5, Y2, curve);                  /* t5 = (y2 - y1)^2 = D */

    om_uecc_vli_modsub(t5, t5, X1, curve->p, num_words); /* t5 = D - B */
    om_uecc_vli_modsub(t5, t5, X2, curve->p, num_words); /* t5 = D - B - C = x3 */
    om_uecc_vli_modsub(X2, X2, X1, curve->p, num_words); /* t3 = C - B */
    om_uecc_vli_modmult_fast(Y1, Y1, X2, curve);                /* t2 = y1*(C - B) */
    om_uecc_vli_modsub(X2, X1, t5, curve->p, num_words); /* t3 = B - x3 */
    om_uecc_vli_modmult_fast(Y2, Y2, X2, curve);                /* t4 = (y2 - y1)*(B - x3) */
    om_uecc_vli_modsub(Y2, Y2, Y1, curve->p, num_words); /* t4 = y3 */

    om_uecc_vli_set(X2, t5, num_words);
}

/* Input P = (x1, y1, Z), Q = (x2, y2, Z)
   Output P + Q = (x3, y3, Z3), P - Q = (x3', y3', Z3)
   or P => P - Q, Q => P + Q
*/
static void XYcZ_addC(om_uecc_word_t * X1,
                      om_uecc_word_t * Y1,
                      om_uecc_word_t * X2,
                      om_uecc_word_t * Y2,
                      om_uecc_curve curve) {
    /* t1 = X1, t2 = Y1, t3 = X2, t4 = Y2 */
    om_uecc_word_t t5[uECC_MAX_WORDS];
    om_uecc_word_t t6[uECC_MAX_WORDS];
    om_uecc_word_t t7[uECC_MAX_WORDS];
    wordcount_t num_words = curve->num_words;

    om_uecc_vli_modsub(t5, X2, X1, curve->p, num_words); /* t5 = x2 - x1 */
    om_uecc_vli_modsquare_fast(t5, t5, curve);                  /* t5 = (x2 - x1)^2 = A */
    om_uecc_vli_modmult_fast(X1, X1, t5, curve);                /* t1 = x1*A = B */
    om_uecc_vli_modmult_fast(X2, X2, t5, curve);                /* t3 = x2*A = C */
    om_uecc_vli_modadd(t5, Y2, Y1, curve->p, num_words); /* t5 = y2 + y1 */
    om_uecc_vli_modsub(Y2, Y2, Y1, curve->p, num_words); /* t4 = y2 - y1 */

    om_uecc_vli_modsub(t6, X2, X1, curve->p, num_words); /* t6 = C - B */
    om_uecc_vli_modmult_fast(Y1, Y1, t6, curve);                /* t2 = y1 * (C - B) = E */
    om_uecc_vli_modadd(t6, X1, X2, curve->p, num_words); /* t6 = B + C */
    om_uecc_vli_modsquare_fast(X2, Y2, curve);                  /* t3 = (y2 - y1)^2 = D */
    om_uecc_vli_modsub(X2, X2, t6, curve->p, num_words); /* t3 = D - (B + C) = x3 */

    om_uecc_vli_modsub(t7, X1, X2, curve->p, num_words); /* t7 = B - x3 */
    om_uecc_vli_modmult_fast(Y2, Y2, t7, curve);                /* t4 = (y2 - y1)*(B - x3) */
    om_uecc_vli_modsub(Y2, Y2, Y1, curve->p, num_words); /* t4 = (y2 - y1)*(B - x3) - E = y3 */

    om_uecc_vli_modsquare_fast(t7, t5, curve);                  /* t7 = (y2 + y1)^2 = F */
    om_uecc_vli_modsub(t7, t7, t6, curve->p, num_words); /* t7 = F - (B + C) = x3' */
    om_uecc_vli_modsub(t6, t7, X1, curve->p, num_words); /* t6 = x3' - B */
    om_uecc_vli_modmult_fast(t6, t6, t5, curve);                /* t6 = (y2+y1)*(x3' - B) */
    om_uecc_vli_modsub(Y1, t6, Y1, curve->p, num_words); /* t2 = (y2+y1)*(x3' - B) - E = y3' */

    om_uecc_vli_set(X1, t7, num_words);
}

/* result may overlap point. */
static void EccPoint_mult(om_uecc_word_t * result,
                          const om_uecc_word_t * point,
                          const om_uecc_word_t * scalar,
                          const om_uecc_word_t * initial_Z,
                          bitcount_t num_bits,
                          om_uecc_curve curve) {
    /* R0 and R1 */
    om_uecc_word_t Rx[2][uECC_MAX_WORDS];
    om_uecc_word_t Ry[2][uECC_MAX_WORDS];
    om_uecc_word_t z[uECC_MAX_WORDS];
    bitcount_t i;
    om_uecc_word_t nb;
    wordcount_t num_words = curve->num_words;

    om_uecc_vli_set(Rx[1], point, num_words);
    om_uecc_vli_set(Ry[1], point + num_words, num_words);

    XYcZ_initial_double(Rx[1], Ry[1], Rx[0], Ry[0], initial_Z, curve);

    for (i = num_bits - 2; i > 0; --i) {
        nb = !om_uecc_vli_testbit(scalar, i);
        XYcZ_addC(Rx[1 - nb], Ry[1 - nb], Rx[nb], Ry[nb], curve);
        XYcZ_add(Rx[nb], Ry[nb], Rx[1 - nb], Ry[1 - nb], curve);
    }

    nb = !om_uecc_vli_testbit(scalar, 0);
    XYcZ_addC(Rx[1 - nb], Ry[1 - nb], Rx[nb], Ry[nb], curve);

    /* Find final 1/Z value. */
    om_uecc_vli_modsub(z, Rx[1], Rx[0], curve->p, num_words); /* X1 - X0 */
    om_uecc_vli_modmult_fast(z, z, Ry[1 - nb], curve);               /* Yb * (X1 - X0) */
    om_uecc_vli_modmult_fast(z, z, point, curve);                    /* xP * Yb * (X1 - X0) */
    om_uecc_vli_modInv(z, z, curve->p, num_words);            /* 1 / (xP * Yb * (X1 - X0)) */
    /* yP / (xP * Yb * (X1 - X0)) */
    om_uecc_vli_modmult_fast(z, z, point + num_words, curve);
    om_uecc_vli_modmult_fast(z, z, Rx[1 - nb], curve); /* Xb * yP / (xP * Yb * (X1 - X0)) */
    /* End 1/Z calculation */

    XYcZ_add(Rx[nb], Ry[nb], Rx[1 - nb], Ry[1 - nb], curve);
    apply_z(Rx[0], Ry[0], z, curve);

    om_uecc_vli_set(result, Rx[0], num_words);
    om_uecc_vli_set(result + num_words, Ry[0], num_words);
}

static om_uecc_word_t regularize_k(const om_uecc_word_t * const k,
                                om_uecc_word_t *k0,
                                om_uecc_word_t *k1,
                                om_uecc_curve curve) {
    wordcount_t num_n_words = BITS_TO_WORDS(curve->num_n_bits);
    bitcount_t num_n_bits = curve->num_n_bits;
    om_uecc_word_t carry = om_uecc_vli_add(k0, k, curve->n, num_n_words) ||
        (num_n_bits < ((bitcount_t)num_n_words * OM_UECC_WORD_SIZE * 8) &&
         om_uecc_vli_testbit(k0, num_n_bits));
    om_uecc_vli_add(k1, k0, curve->n, num_n_words);
    return carry;
}

/* Generates a random integer in the range 0 < random < top.
   Both random and top have num_words words. */
uECC_VLI_API int om_uecc_generate_random_int(om_uecc_word_t *random,
                                          const om_uecc_word_t *top,
                                          wordcount_t num_words) {
    om_uecc_word_t mask = (om_uecc_word_t)-1;
    om_uecc_word_t tries;
    bitcount_t num_bits = om_uecc_vli_numbits(top, num_words);

    if (!g_rng_function) {
        return 0;
    }

    for (tries = 0; tries < uECC_RNG_MAX_TRIES; ++tries) {
        if (!g_rng_function((uint8_t *)random, num_words * OM_UECC_WORD_SIZE)) {
            return 0;
        }
        random[num_words - 1] &= mask >> ((bitcount_t)(num_words * OM_UECC_WORD_SIZE * 8 - num_bits));
        if (!om_uecc_vli_iszero(random, num_words) &&
                om_uecc_vli_cmp(top, random, num_words) == 1) {
            return 1;
        }
    }
    return 0;
}

static om_uecc_word_t EccPoint_compute_public_key(om_uecc_word_t *result,
                                               om_uecc_word_t *private_key,
                                               om_uecc_curve curve) {
    om_uecc_word_t tmp1[uECC_MAX_WORDS];
    om_uecc_word_t tmp2[uECC_MAX_WORDS];
    om_uecc_word_t *p2[2] = {tmp1, tmp2};
    om_uecc_word_t *initial_Z = 0;
    om_uecc_word_t carry;

    /* Regularize the bitcount for the private key so that attackers cannot use a side channel
       attack to learn the number of leading zeros. */
    carry = regularize_k(private_key, tmp1, tmp2, curve);

    /* If an RNG function was specified, try to get a random initial Z value to improve
       protection against side-channel attacks. */
    if (g_rng_function) {
        if (!om_uecc_generate_random_int(p2[carry], curve->p, curve->num_words)) {
            return 0;
        }
        initial_Z = p2[carry];
    }
    EccPoint_mult(result, curve->G, p2[!carry], initial_Z, curve->num_n_bits + 1, curve);

    if (EccPoint_isZero(result, curve)) {
        return 0;
    }
    return 1;
}

#if OM_UECC_WORD_SIZE == 1

uECC_VLI_API void om_uecc_vli_nativetobytes(uint8_t *bytes,
                                         int num_bytes,
                                         const uint8_t *native) {
    wordcount_t i;
    for (i = 0; i < num_bytes; ++i) {
        bytes[i] = native[(num_bytes - 1) - i];
    }
}

uECC_VLI_API void om_uecc_vli_bytestonative(uint8_t *native,
                                         const uint8_t *bytes,
                                         int num_bytes) {
    om_uecc_vli_nativetobytes(native, num_bytes, bytes);
}

#else
#if !OM_UECC_VLI_NATIVE_LITTLE_ENDIAN
uECC_VLI_API void om_uecc_vli_nativetobytes(uint8_t *bytes,
                                         int num_bytes,
                                         const om_uecc_word_t *native) {
    int i;
    for (i = 0; i < num_bytes; ++i) {
        unsigned b = num_bytes - 1 - i;
        bytes[i] = native[b / OM_UECC_WORD_SIZE] >> (8 * (b % OM_UECC_WORD_SIZE));
    }
}

uECC_VLI_API void om_uecc_vli_bytestonative(om_uecc_word_t *native,
                                         const uint8_t *bytes,
                                         int num_bytes) {
    int i;
    om_uecc_vli_clear(native, (num_bytes + (OM_UECC_WORD_SIZE - 1)) / OM_UECC_WORD_SIZE);
    for (i = 0; i < num_bytes; ++i) {
        unsigned b = num_bytes - 1 - i;
        native[b / OM_UECC_WORD_SIZE] |=
            (om_uecc_word_t)bytes[i] << (8 * (b % OM_UECC_WORD_SIZE));
    }
}
#endif

#endif /* OM_UECC_WORD_SIZE */

int om_uecc_make_key(uint8_t *public_key,
                  uint8_t *private_key,
                  om_uecc_curve curve) {
    OM_ASSERT(OM_IS_ALIGN((uint32_t)private_key, 4));
    OM_ASSERT(OM_IS_ALIGN((uint32_t)public_key, 4));

#if OM_UECC_VLI_NATIVE_LITTLE_ENDIAN
    om_uecc_word_t *_private = (om_uecc_word_t *)private_key;
    om_uecc_word_t *_public = (om_uecc_word_t *)public_key;
#else
    om_uecc_word_t _private[uECC_MAX_WORDS];
    om_uecc_word_t _public[uECC_MAX_WORDS * 2];
#endif
    om_uecc_word_t tries;

    for (tries = 0; tries < uECC_RNG_MAX_TRIES; ++tries) {
        if (!om_uecc_generate_random_int(_private, curve->n, BITS_TO_WORDS(curve->num_n_bits))) {
            return 0;
        }

        if (EccPoint_compute_public_key(_public, _private, curve)) {
#if OM_UECC_VLI_NATIVE_LITTLE_ENDIAN == 0
            om_uecc_vli_nativetobytes(private_key, BITS_TO_BYTES(curve->num_n_bits), _private);
            om_uecc_vli_nativetobytes(public_key, curve->num_bytes, _public);
            om_uecc_vli_nativetobytes(
                public_key + curve->num_bytes, curve->num_bytes, _public + curve->num_words);
#endif
            return 1;
        }
    }
    return 0;
}

int om_uecc_shared_secret(const uint8_t *public_key,
                       const uint8_t *private_key,
                       uint8_t *secret,
                       om_uecc_curve curve) {
    OM_ASSERT(OM_IS_ALIGN((uint32_t)private_key, 4));
    OM_ASSERT(OM_IS_ALIGN((uint32_t)public_key, 4));

    om_uecc_word_t _public[uECC_MAX_WORDS * 2];
    om_uecc_word_t _private[uECC_MAX_WORDS];

    om_uecc_word_t tmp[uECC_MAX_WORDS];
    om_uecc_word_t *p2[2] = {_private, tmp};
    om_uecc_word_t *initial_Z = 0;
    om_uecc_word_t carry;
    wordcount_t num_words = curve->num_words;
    wordcount_t num_bytes = curve->num_bytes;

#if OM_UECC_VLI_NATIVE_LITTLE_ENDIAN
    ecc_copy((uint8_t *) _private, private_key, num_bytes);
    ecc_copy((uint8_t *) _public, public_key, num_bytes*2);
#else
    om_uecc_vli_bytestonative(_private, private_key, BITS_TO_BYTES(curve->num_n_bits));
    om_uecc_vli_bytestonative(_public, public_key, num_bytes);
    om_uecc_vli_bytestonative(_public + num_words, public_key + num_bytes, num_bytes);
#endif

    /* Regularize the bitcount for the private key so that attackers cannot use a side channel
       attack to learn the number of leading zeros. */
    carry = regularize_k(_private, _private, tmp, curve);

    /* If an RNG function was specified, try to get a random initial Z value to improve
       protection against side-channel attacks. */
    if (g_rng_function) {
        if (!om_uecc_generate_random_int(p2[carry], curve->p, num_words)) {
            return 0;
        }
        initial_Z = p2[carry];
    }

    EccPoint_mult(_public, _public, p2[!carry], initial_Z, curve->num_n_bits + 1, curve);
#if OM_UECC_VLI_NATIVE_LITTLE_ENDIAN
    ecc_copy((uint8_t *) secret, (uint8_t *) _public, num_bytes);
#else
    om_uecc_vli_nativetobytes(secret, num_bytes, _public);
#endif
    return !EccPoint_isZero(_public, curve);
}

#if OM_UECC_SUPPORT_COMPRESSED_POINT
void om_uecc_compress(const uint8_t *public_key, uint8_t *compressed, om_uecc_curve curve) {
    OM_ASSERT(OM_IS_ALIGN((uint32_t)public_key, 4));

    wordcount_t i;
    for (i = 0; i < curve->num_bytes; ++i) {
        compressed[i+1] = public_key[i];
    }
#if OM_UECC_VLI_NATIVE_LITTLE_ENDIAN
    compressed[0] = 2 + (public_key[curve->num_bytes] & 0x01);
#else
    compressed[0] = 2 + (public_key[curve->num_bytes * 2 - 1] & 0x01);
#endif
}

void om_uecc_decompress(const uint8_t *compressed, uint8_t *public_key, om_uecc_curve curve) {
    OM_ASSERT(OM_IS_ALIGN((uint32_t)public_key, 4));

#if OM_UECC_VLI_NATIVE_LITTLE_ENDIAN
    om_uecc_word_t *point = (om_uecc_word_t *)public_key;
#else
    om_uecc_word_t point[uECC_MAX_WORDS * 2];
#endif
    om_uecc_word_t *y = point + curve->num_words;
#if OM_UECC_VLI_NATIVE_LITTLE_ENDIAN
    ecc_copy(public_key, compressed+1, curve->num_bytes);
#else
    om_uecc_vli_bytestonative(point, compressed + 1, curve->num_bytes);
#endif
    curve->x_side(y, point, curve);
    curve->mod_sqrt(y, curve);

    if ((y[0] & 0x01) != (compressed[0] & 0x01)) {
        om_uecc_vli_sub(y, curve->p, y, curve->num_words);
    }

#if OM_UECC_VLI_NATIVE_LITTLE_ENDIAN == 0
    om_uecc_vli_nativetobytes(public_key, curve->num_bytes, point);
    om_uecc_vli_nativetobytes(public_key + curve->num_bytes, curve->num_bytes, y);
#endif
}
#endif /* OM_UECC_SUPPORT_COMPRESSED_POINT */

uECC_VLI_API int om_uecc_valid_point(const om_uecc_word_t *point, om_uecc_curve curve) {
    om_uecc_word_t tmp1[uECC_MAX_WORDS];
    om_uecc_word_t tmp2[uECC_MAX_WORDS];
    wordcount_t num_words = curve->num_words;

    /* The point at infinity is invalid. */
    if (EccPoint_isZero(point, curve)) {
        return 0;
    }

    /* x and y must be smaller than p. */
    if (uECC_vli_cmp_unsafe(curve->p, point, num_words) != 1 ||
            uECC_vli_cmp_unsafe(curve->p, point + num_words, num_words) != 1) {
        return 0;
    }

    om_uecc_vli_modsquare_fast(tmp1, point + num_words, curve);
    curve->x_side(tmp2, point, curve); /* tmp2 = x^3 + ax + b */

    /* Make sure that y^2 == x^3 + ax + b */
    return (int)(om_uecc_vli_equal(tmp1, tmp2, num_words));
}

int om_uecc_valid_public_key(const uint8_t *public_key, om_uecc_curve curve) {
    OM_ASSERT(OM_IS_ALIGN((uint32_t)public_key, 4));

#if OM_UECC_VLI_NATIVE_LITTLE_ENDIAN
    om_uecc_word_t *_public = (om_uecc_word_t *)public_key;
#else
    om_uecc_word_t _public[uECC_MAX_WORDS * 2];
#endif

#if OM_UECC_VLI_NATIVE_LITTLE_ENDIAN == 0
    om_uecc_vli_bytestonative(_public, public_key, curve->num_bytes);
    om_uecc_vli_bytestonative(
        _public + curve->num_words, public_key + curve->num_bytes, curve->num_bytes);
#endif
    return om_uecc_valid_point(_public, curve);
}

int om_uecc_compute_public_key(const uint8_t *private_key, uint8_t *public_key, om_uecc_curve curve) {
    OM_ASSERT(OM_IS_ALIGN((uint32_t)private_key, 4));
    OM_ASSERT(OM_IS_ALIGN((uint32_t)public_key, 4));

#if OM_UECC_VLI_NATIVE_LITTLE_ENDIAN
    om_uecc_word_t *_private = (om_uecc_word_t *)private_key;
    om_uecc_word_t *_public = (om_uecc_word_t *)public_key;
#else
    om_uecc_word_t _private[uECC_MAX_WORDS];
    om_uecc_word_t _public[uECC_MAX_WORDS * 2];
#endif

#if OM_UECC_VLI_NATIVE_LITTLE_ENDIAN == 0
    om_uecc_vli_bytestonative(_private, private_key, BITS_TO_BYTES(curve->num_n_bits));
#endif

    /* Make sure the private key is in the range [1, n-1]. */
    if (om_uecc_vli_iszero(_private, BITS_TO_WORDS(curve->num_n_bits))) {
        return 0;
    }

    if (om_uecc_vli_cmp(curve->n, _private, BITS_TO_WORDS(curve->num_n_bits)) != 1) {
        return 0;
    }

    /* Compute public key. */
    if (!EccPoint_compute_public_key(_public, _private, curve)) {
        return 0;
    }

#if OM_UECC_VLI_NATIVE_LITTLE_ENDIAN == 0
    om_uecc_vli_nativetobytes(public_key, curve->num_bytes, _public);
    om_uecc_vli_nativetobytes(
        public_key + curve->num_bytes, curve->num_bytes, _public + curve->num_words);
#endif
    return 1;
}


/* -------- ECDSA code -------- */

static void bits2int(om_uecc_word_t *native,
                     const uint8_t *bits,
                     unsigned bits_size,
                     om_uecc_curve curve) {
    unsigned num_n_bytes = BITS_TO_BYTES(curve->num_n_bits);
    unsigned num_n_words = BITS_TO_WORDS(curve->num_n_bits);
    int shift;
    om_uecc_word_t carry;
    om_uecc_word_t *ptr;

    if (bits_size > num_n_bytes) {
        bits_size = num_n_bytes;
    }

    om_uecc_vli_clear(native, num_n_words);
#if OM_UECC_VLI_NATIVE_LITTLE_ENDIAN
    ecc_copy((uint8_t *) native, bits, bits_size);
#else
    om_uecc_vli_bytestonative(native, bits, bits_size);
#endif
    if (bits_size * 8 <= (unsigned)curve->num_n_bits) {
        return;
    }
    shift = bits_size * 8 - curve->num_n_bits;
    carry = 0;
    ptr = native + num_n_words;
    while (ptr-- > native) {
        om_uecc_word_t temp = *ptr;
        *ptr = (temp >> shift) | carry;
        carry = temp << (OM_UECC_WORD_BITS - shift);
    }

    /* Reduce mod curve_n */
    if (uECC_vli_cmp_unsafe(curve->n, native, num_n_words) != 1) {
        om_uecc_vli_sub(native, native, curve->n, num_n_words);
    }
}

static int uECC_sign_with_k_internal(const uint8_t *private_key,
                            const uint8_t *message_hash,
                            unsigned hash_size,
                            om_uecc_word_t *k,
                            uint8_t *signature,
                            om_uecc_curve curve) {

    om_uecc_word_t tmp[uECC_MAX_WORDS];
    om_uecc_word_t s[uECC_MAX_WORDS];
    om_uecc_word_t *k2[2] = {tmp, s};
    om_uecc_word_t *initial_Z = 0;
#if OM_UECC_VLI_NATIVE_LITTLE_ENDIAN
    om_uecc_word_t *p = (om_uecc_word_t *)signature;
#else
    om_uecc_word_t p[uECC_MAX_WORDS * 2];
#endif
    om_uecc_word_t carry;
    wordcount_t num_words = curve->num_words;
    wordcount_t num_n_words = BITS_TO_WORDS(curve->num_n_bits);
    bitcount_t num_n_bits = curve->num_n_bits;

    /* Make sure 0 < k < curve_n */
    if (om_uecc_vli_iszero(k, num_words) || om_uecc_vli_cmp(curve->n, k, num_n_words) != 1) {
        return 0;
    }

    carry = regularize_k(k, tmp, s, curve);
    /* If an RNG function was specified, try to get a random initial Z value to improve
       protection against side-channel attacks. */
    if (g_rng_function) {
        if (!om_uecc_generate_random_int(k2[carry], curve->p, num_words)) {
            return 0;
        }
        initial_Z = k2[carry];
    }
    EccPoint_mult(p, curve->G, k2[!carry], initial_Z, num_n_bits + 1, curve);
    if (om_uecc_vli_iszero(p, num_words)) {
        return 0;
    }

    /* If an RNG function was specified, get a random number
       to prevent side channel analysis of k. */
    if (!g_rng_function) {
        om_uecc_vli_clear(tmp, num_n_words);
        tmp[0] = 1;
    } else if (!om_uecc_generate_random_int(tmp, curve->n, num_n_words)) {
        return 0;
    }

    /* Prevent side channel analysis of om_uecc_vli_modInv() to determine
       bits of k / the private key by premultiplying by a random number */
    om_uecc_vli_modmult(k, k, tmp, curve->n, num_n_words); /* k' = rand * k */
    om_uecc_vli_modInv(k, k, curve->n, num_n_words);       /* k = 1 / k' */
    om_uecc_vli_modmult(k, k, tmp, curve->n, num_n_words); /* k = 1 / k */

#if OM_UECC_VLI_NATIVE_LITTLE_ENDIAN == 0
    om_uecc_vli_nativetobytes(signature, curve->num_bytes, p); /* store r */
#endif

#if OM_UECC_VLI_NATIVE_LITTLE_ENDIAN
    ecc_copy((uint8_t *) tmp, private_key, BITS_TO_BYTES(curve->num_n_bits));
#else
    om_uecc_vli_bytestonative(tmp, private_key, BITS_TO_BYTES(curve->num_n_bits)); /* tmp = d */
#endif

    s[num_n_words - 1] = 0;
    om_uecc_vli_set(s, p, num_words);
    om_uecc_vli_modmult(s, tmp, s, curve->n, num_n_words); /* s = r*d */

    bits2int(tmp, message_hash, hash_size, curve);
    om_uecc_vli_modadd(s, tmp, s, curve->n, num_n_words); /* s = e + r*d */
    om_uecc_vli_modmult(s, s, k, curve->n, num_n_words);  /* s = (e + r*d) / k */
    if (om_uecc_vli_numbits(s, num_n_words) > (bitcount_t)curve->num_bytes * 8) {
        return 0;
    }
#if OM_UECC_VLI_NATIVE_LITTLE_ENDIAN
    ecc_copy((uint8_t *) signature + curve->num_bytes, (uint8_t *) s, curve->num_bytes);
#else
    om_uecc_vli_nativetobytes(signature + curve->num_bytes, curve->num_bytes, s);
#endif
    return 1;
}

/* For testing - sign with an explicitly specified k value */
int om_uecc_sign_with_k(const uint8_t *private_key,
                            const uint8_t *message_hash,
                            unsigned hash_size,
                            const uint8_t *k,
                            uint8_t *signature,
                            om_uecc_curve curve) {
    om_uecc_word_t k2[uECC_MAX_WORDS];
    bits2int(k2, k, BITS_TO_BYTES(curve->num_n_bits), curve);
    return uECC_sign_with_k_internal(private_key, message_hash, hash_size, k2, signature, curve);
}

int om_uecc_sign(const uint8_t *private_key,
              const uint8_t *message_hash,
              unsigned hash_size,
              uint8_t *signature,
              om_uecc_curve curve) {
    OM_ASSERT(OM_IS_ALIGN((uint32_t)private_key, 4));

    om_uecc_word_t k[uECC_MAX_WORDS];
    om_uecc_word_t tries;

    for (tries = 0; tries < uECC_RNG_MAX_TRIES; ++tries) {
        if (!om_uecc_generate_random_int(k, curve->n, BITS_TO_WORDS(curve->num_n_bits))) {
            return 0;
        }

        if (uECC_sign_with_k_internal(private_key, message_hash, hash_size, k, signature, curve)) {
            return 1;
        }
    }
    return 0;
}

/* Compute an HMAC using K as a key (as in RFC 6979). Note that K is always
   the same size as the hash result size. */
static void HMAC_init(const om_uecc_hashcontext *hash_context, const uint8_t *K) {
    uint8_t *pad = hash_context->tmp + 2 * hash_context->result_size;
    unsigned i;
    for (i = 0; i < hash_context->result_size; ++i)
        pad[i] = K[i] ^ 0x36;
    for (; i < hash_context->block_size; ++i)
        pad[i] = 0x36;

    hash_context->init_hash(hash_context);
    hash_context->update_hash(hash_context, pad, hash_context->block_size);
}

static void HMAC_update(const om_uecc_hashcontext *hash_context,
                        const uint8_t *message,
                        unsigned message_size) {
    hash_context->update_hash(hash_context, message, message_size);
}

static void HMAC_finish(const om_uecc_hashcontext *hash_context,
                        const uint8_t *K,
                        uint8_t *result) {
    uint8_t *pad = hash_context->tmp + 2 * hash_context->result_size;
    unsigned i;
    for (i = 0; i < hash_context->result_size; ++i)
        pad[i] = K[i] ^ 0x5c;
    for (; i < hash_context->block_size; ++i)
        pad[i] = 0x5c;

    hash_context->finish_hash(hash_context, result);

    hash_context->init_hash(hash_context);
    hash_context->update_hash(hash_context, pad, hash_context->block_size);
    hash_context->update_hash(hash_context, result, hash_context->result_size);
    hash_context->finish_hash(hash_context, result);
}

/* V = HMAC_K(V) */
static void update_V(const om_uecc_hashcontext *hash_context, uint8_t *K, uint8_t *V) {
    HMAC_init(hash_context, K);
    HMAC_update(hash_context, V, hash_context->result_size);
    HMAC_finish(hash_context, K, V);
}

/* Deterministic signing, similar to RFC 6979. Differences are:
    * We just use H(m) directly rather than bits2octets(H(m))
      (it is not reduced modulo curve_n).
    * We generate a value for k (aka T) directly rather than converting endianness.

   Layout of hash_context->tmp: <K> | <V> | (1 byte overlapped 0x00 or 0x01) / <HMAC pad> */
int om_uecc_sign_deterministic(const uint8_t *private_key,
                            const uint8_t *message_hash,
                            unsigned hash_size,
                            const om_uecc_hashcontext *hash_context,
                            uint8_t *signature,
                            om_uecc_curve curve) {
    OM_ASSERT(OM_IS_ALIGN((uint32_t)private_key, 4));

    uint8_t *K = hash_context->tmp;
    uint8_t *V = K + hash_context->result_size;
    wordcount_t num_bytes = curve->num_bytes;
    wordcount_t num_n_words = BITS_TO_WORDS(curve->num_n_bits);
    bitcount_t num_n_bits = curve->num_n_bits;
    om_uecc_word_t tries;
    unsigned i;
    for (i = 0; i < hash_context->result_size; ++i) {
        V[i] = 0x01;
        K[i] = 0;
    }

    /* K = HMAC_K(V || 0x00 || int2octets(x) || h(m)) */
    HMAC_init(hash_context, K);
    V[hash_context->result_size] = 0x00;
    HMAC_update(hash_context, V, hash_context->result_size + 1);
    HMAC_update(hash_context, private_key, num_bytes);
    HMAC_update(hash_context, message_hash, hash_size);
    HMAC_finish(hash_context, K, K);

    update_V(hash_context, K, V);

    /* K = HMAC_K(V || 0x01 || int2octets(x) || h(m)) */
    HMAC_init(hash_context, K);
    V[hash_context->result_size] = 0x01;
    HMAC_update(hash_context, V, hash_context->result_size + 1);
    HMAC_update(hash_context, private_key, num_bytes);
    HMAC_update(hash_context, message_hash, hash_size);
    HMAC_finish(hash_context, K, K);

    update_V(hash_context, K, V);

    for (tries = 0; tries < uECC_RNG_MAX_TRIES; ++tries) {
        om_uecc_word_t T[uECC_MAX_WORDS];
        uint8_t *T_ptr = (uint8_t *)T;
        wordcount_t T_bytes = 0;
        for (;;) {
            update_V(hash_context, K, V);
            for (i = 0; i < hash_context->result_size; ++i) {
                T_ptr[T_bytes++] = V[i];
                if (T_bytes >= num_n_words * OM_UECC_WORD_SIZE) {
                    goto filled;
                }
            }
        }
    filled:
        if ((bitcount_t)num_n_words * OM_UECC_WORD_SIZE * 8 > num_n_bits) {
            om_uecc_word_t mask = (om_uecc_word_t)-1;
            T[num_n_words - 1] &=
                mask >> ((bitcount_t)(num_n_words * OM_UECC_WORD_SIZE * 8 - num_n_bits));
        }

        if (uECC_sign_with_k_internal(private_key, message_hash, hash_size, T, signature, curve)) {
            return 1;
        }

        /* K = HMAC_K(V || 0x00) */
        HMAC_init(hash_context, K);
        V[hash_context->result_size] = 0x00;
        HMAC_update(hash_context, V, hash_context->result_size + 1);
        HMAC_finish(hash_context, K, K);

        update_V(hash_context, K, V);
    }
    return 0;
}

static bitcount_t smax(bitcount_t a, bitcount_t b) {
    return (a > b ? a : b);
}

int om_uecc_verify(const uint8_t *public_key,
                const uint8_t *message_hash,
                unsigned hash_size,
                const uint8_t *signature,
                om_uecc_curve curve) {
    OM_ASSERT(OM_IS_ALIGN((uint32_t)public_key, 4));

    om_uecc_word_t u1[uECC_MAX_WORDS], u2[uECC_MAX_WORDS];
    om_uecc_word_t z[uECC_MAX_WORDS];
    om_uecc_word_t sum[uECC_MAX_WORDS * 2];
    om_uecc_word_t rx[uECC_MAX_WORDS];
    om_uecc_word_t ry[uECC_MAX_WORDS];
    om_uecc_word_t tx[uECC_MAX_WORDS];
    om_uecc_word_t ty[uECC_MAX_WORDS];
    om_uecc_word_t tz[uECC_MAX_WORDS];
    const om_uecc_word_t *points[4];
    const om_uecc_word_t *point;
    bitcount_t num_bits;
    bitcount_t i;
#if OM_UECC_VLI_NATIVE_LITTLE_ENDIAN
    om_uecc_word_t *_public = (om_uecc_word_t *)public_key;
#else
    om_uecc_word_t _public[uECC_MAX_WORDS * 2];
#endif
    om_uecc_word_t r[uECC_MAX_WORDS], s[uECC_MAX_WORDS];
    wordcount_t num_words = curve->num_words;
    wordcount_t num_n_words = BITS_TO_WORDS(curve->num_n_bits);

    rx[num_n_words - 1] = 0;
    r[num_n_words - 1] = 0;
    s[num_n_words - 1] = 0;

#if OM_UECC_VLI_NATIVE_LITTLE_ENDIAN
    ecc_copy((uint8_t *) r, signature, curve->num_bytes);
    ecc_copy((uint8_t *) s, signature + curve->num_bytes, curve->num_bytes);
#else
    om_uecc_vli_bytestonative(_public, public_key, curve->num_bytes);
    om_uecc_vli_bytestonative(
        _public + num_words, public_key + curve->num_bytes, curve->num_bytes);
    om_uecc_vli_bytestonative(r, signature, curve->num_bytes);
    om_uecc_vli_bytestonative(s, signature + curve->num_bytes, curve->num_bytes);
#endif

    /* r, s must not be 0. */
    if (om_uecc_vli_iszero(r, num_words) || om_uecc_vli_iszero(s, num_words)) {
        return 0;
    }

    /* r, s must be < n. */
    if (uECC_vli_cmp_unsafe(curve->n, r, num_n_words) != 1 ||
            uECC_vli_cmp_unsafe(curve->n, s, num_n_words) != 1) {
        return 0;
    }

    /* Calculate u1 and u2. */
    om_uecc_vli_modInv(z, s, curve->n, num_n_words); /* z = 1/s */
    u1[num_n_words - 1] = 0;
    bits2int(u1, message_hash, hash_size, curve);
    om_uecc_vli_modmult(u1, u1, z, curve->n, num_n_words); /* u1 = e/s */
    om_uecc_vli_modmult(u2, r, z, curve->n, num_n_words); /* u2 = r/s */

    /* Calculate sum = G + Q. */
    om_uecc_vli_set(sum, _public, num_words);
    om_uecc_vli_set(sum + num_words, _public + num_words, num_words);
    om_uecc_vli_set(tx, curve->G, num_words);
    om_uecc_vli_set(ty, curve->G + num_words, num_words);
    om_uecc_vli_modsub(z, sum, tx, curve->p, num_words); /* z = x2 - x1 */
    XYcZ_add(tx, ty, sum, sum + num_words, curve);
    om_uecc_vli_modInv(z, z, curve->p, num_words); /* z = 1/z */
    apply_z(sum, sum + num_words, z, curve);

    /* Use Shamir's trick to calculate u1*G + u2*Q */
    points[0] = 0;
    points[1] = curve->G;
    points[2] = _public;
    points[3] = sum;
    num_bits = smax(om_uecc_vli_numbits(u1, num_n_words),
                    om_uecc_vli_numbits(u2, num_n_words));

    point = points[(!!om_uecc_vli_testbit(u1, num_bits - 1)) |
                   ((!!om_uecc_vli_testbit(u2, num_bits - 1)) << 1)];
    om_uecc_vli_set(rx, point, num_words);
    om_uecc_vli_set(ry, point + num_words, num_words);
    om_uecc_vli_clear(z, num_words);
    z[0] = 1;

    for (i = num_bits - 2; i >= 0; --i) {
        om_uecc_word_t index;
        curve->double_jacobian(rx, ry, z, curve);

        index = (!!om_uecc_vli_testbit(u1, i)) | ((!!om_uecc_vli_testbit(u2, i)) << 1);
        point = points[index];
        if (point) {
            om_uecc_vli_set(tx, point, num_words);
            om_uecc_vli_set(ty, point + num_words, num_words);
            apply_z(tx, ty, z, curve);
            om_uecc_vli_modsub(tz, rx, tx, curve->p, num_words); /* Z = x2 - x1 */
            XYcZ_add(tx, ty, rx, ry, curve);
            om_uecc_vli_modmult_fast(z, z, tz, curve);
        }
    }

    om_uecc_vli_modInv(z, z, curve->p, num_words); /* Z = 1/Z */
    apply_z(rx, ry, z, curve);

    /* v = x1 (mod n) */
    if (uECC_vli_cmp_unsafe(curve->n, rx, num_n_words) != 1) {
        om_uecc_vli_sub(rx, rx, curve->n, num_n_words);
    }

    /* Accept only if v == r. */
    return (int)(om_uecc_vli_equal(rx, r, num_words));
}

#if OM_UECC_ENABLE_VLI_API

unsigned om_uecc_curve_num_words(om_uecc_curve curve) {
    return curve->num_words;
}

unsigned om_uecc_curve_num_bytes(om_uecc_curve curve) {
    return curve->num_bytes;
}

unsigned om_uecc_curve_num_bits(om_uecc_curve curve) {
    return curve->num_bytes * 8;
}

unsigned om_uecc_curve_num_n_words(om_uecc_curve curve) {
    return BITS_TO_WORDS(curve->num_n_bits);
}

unsigned om_uecc_curve_num_n_bytes(om_uecc_curve curve) {
    return BITS_TO_BYTES(curve->num_n_bits);
}

unsigned om_uecc_curve_num_n_bits(om_uecc_curve curve) {
    return curve->num_n_bits;
}

const om_uecc_word_t *om_uecc_curve_p(om_uecc_curve curve) {
    return curve->p;
}

const om_uecc_word_t *om_uecc_curve_n(om_uecc_curve curve) {
    return curve->n;
}

const om_uecc_word_t *om_uecc_curve_g(om_uecc_curve curve) {
    return curve->G;
}

const om_uecc_word_t *om_uecc_curve_b(om_uecc_curve curve) {
    return curve->b;
}

#if OM_UECC_SUPPORT_COMPRESSED_POINT
void om_uecc_vli_mod_sqrt(om_uecc_word_t *a, om_uecc_curve curve) {
    curve->mod_sqrt(a, curve);
}
#endif

void om_uecc_vli_mmod_fast(om_uecc_word_t *result, om_uecc_word_t *product, om_uecc_curve curve) {
#if (OM_UECC_OPTIMIZATION_LEVEL > 0)
    curve->mmod_fast(result, product);
#else
    om_uecc_vli_mmod(result, product, curve->p, curve->num_words);
#endif
}

void om_uecc_point_mult(om_uecc_word_t *result,
                     const om_uecc_word_t *point,
                     const om_uecc_word_t *scalar,
                     om_uecc_curve curve) {
    om_uecc_word_t tmp1[uECC_MAX_WORDS];
    om_uecc_word_t tmp2[uECC_MAX_WORDS];
    om_uecc_word_t *p2[2] = {tmp1, tmp2};
    om_uecc_word_t carry = regularize_k(scalar, tmp1, tmp2, curve);

    EccPoint_mult(result, point, p2[!carry], 0, curve->num_n_bits + 1, curve);
}

#endif /* OM_UECC_ENABLE_VLI_API */

#endif /* RTE_ECDSA */