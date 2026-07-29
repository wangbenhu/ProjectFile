/* Copyright 2015, Kenneth MacKay. Licensed under the BSD 2-clause license. */

#ifndef _UECC_VLI_H_
#define _UECC_VLI_H_

#include "uECC.h"
#include "types.h"

/* Functions for raw large-integer manipulation. These are only available
   if uECC.c is compiled with OM_UECC_ENABLE_VLI_API defined to 1. */
#ifndef OM_UECC_ENABLE_VLI_API
    #define OM_UECC_ENABLE_VLI_API 0
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#if OM_UECC_ENABLE_VLI_API

void om_uecc_vli_clear(om_uecc_word_t *vli, wordcount_t num_words);

/* Constant-time comparison to zero - secure way to compare long integers */
/* Returns 1 if vli == 0, 0 otherwise. */
om_uecc_word_t om_uecc_vli_iszero(const om_uecc_word_t *vli, wordcount_t num_words);

/* Returns nonzero if bit 'bit' of vli is set. */
om_uecc_word_t om_uecc_vli_testbit(const om_uecc_word_t *vli, bitcount_t bit);

/* Counts the number of bits required to represent vli. */
bitcount_t om_uecc_vli_numbits(const om_uecc_word_t *vli, const wordcount_t max_words);

/* Sets dest = src. */
void om_uecc_vli_set(om_uecc_word_t *dest, const om_uecc_word_t *src, wordcount_t num_words);

/* Constant-time comparison function - secure way to compare long integers */
/* Returns one if left == right, zero otherwise */
om_uecc_word_t om_uecc_vli_equal(const om_uecc_word_t *left,
                              const om_uecc_word_t *right,
                              wordcount_t num_words);

/* Constant-time comparison function - secure way to compare long integers */
/* Returns sign of left - right, in constant time. */
cmpresult_t om_uecc_vli_cmp(const om_uecc_word_t *left, const om_uecc_word_t *right, wordcount_t num_words);

/* Computes vli = vli >> 1. */
void om_uecc_vli_rshift1(om_uecc_word_t *vli, wordcount_t num_words);

/* Computes result = left + right, returning carry. Can modify in place. */
om_uecc_word_t om_uecc_vli_add(om_uecc_word_t *result,
                         const om_uecc_word_t *left,
                         const om_uecc_word_t *right,
                         wordcount_t num_words);

/* Computes result = left - right, returning borrow. Can modify in place. */
om_uecc_word_t om_uecc_vli_sub(om_uecc_word_t *result,
                         const om_uecc_word_t *left,
                         const om_uecc_word_t *right,
                         wordcount_t num_words);

/* Computes result = left * right. Result must be 2 * num_words long. */
void om_uecc_vli_mult(om_uecc_word_t *result,
                   const om_uecc_word_t *left,
                   const om_uecc_word_t *right,
                   wordcount_t num_words);

/* Computes result = left^2. Result must be 2 * num_words long. */
void om_uecc_vli_square(om_uecc_word_t *result, const om_uecc_word_t *left, wordcount_t num_words);

/* Computes result = (left + right) % mod.
   Assumes that left < mod and right < mod, and that result does not overlap mod. */
void om_uecc_vli_modadd(om_uecc_word_t *result,
                     const om_uecc_word_t *left,
                     const om_uecc_word_t *right,
                     const om_uecc_word_t *mod,
                     wordcount_t num_words);

/* Computes result = (left - right) % mod.
   Assumes that left < mod and right < mod, and that result does not overlap mod. */
void om_uecc_vli_modsub(om_uecc_word_t *result,
                     const om_uecc_word_t *left,
                     const om_uecc_word_t *right,
                     const om_uecc_word_t *mod,
                     wordcount_t num_words);

/* Computes result = product % mod, where product is 2N words long.
   Currently only designed to work for mod == curve->p or curve_n. */
void om_uecc_vli_mmod(om_uecc_word_t *result,
                   om_uecc_word_t *product,
                   const om_uecc_word_t *mod,
                   wordcount_t num_words);

/* Calculates result = product (mod curve->p), where product is up to
   2 * curve->num_words long. */
void om_uecc_vli_mmod_fast(om_uecc_word_t *result, om_uecc_word_t *product, om_uecc_curve curve);

/* Computes result = (left * right) % mod.
   Currently only designed to work for mod == curve->p or curve_n. */
void om_uecc_vli_modmult(om_uecc_word_t *result,
                      const om_uecc_word_t *left,
                      const om_uecc_word_t *right,
                      const om_uecc_word_t *mod,
                      wordcount_t num_words);

/* Computes result = (left * right) % curve->p. */
void om_uecc_vli_modmult_fast(om_uecc_word_t *result,
                           const om_uecc_word_t *left,
                           const om_uecc_word_t *right,
                           om_uecc_curve curve);

/* Computes result = left^2 % mod.
   Currently only designed to work for mod == curve->p or curve_n. */
void om_uecc_vli_modsquare(om_uecc_word_t *result,
                        const om_uecc_word_t *left,
                        const om_uecc_word_t *mod,
                        wordcount_t num_words);

/* Computes result = left^2 % curve->p. */
void om_uecc_vli_modsquare_fast(om_uecc_word_t *result, const om_uecc_word_t *left, om_uecc_curve curve);

/* Computes result = (1 / input) % mod.*/
void om_uecc_vli_modInv(om_uecc_word_t *result,
                     const om_uecc_word_t *input,
                     const om_uecc_word_t *mod,
                     wordcount_t num_words);

#if OM_UECC_SUPPORT_COMPRESSED_POINT
/* Calculates a = sqrt(a) (mod curve->p) */
void om_uecc_vli_mod_sqrt(om_uecc_word_t *a, om_uecc_curve curve);
#endif

/* Converts an integer in uECC native format to big-endian bytes. */
void om_uecc_vli_nativetobytes(uint8_t *bytes, int num_bytes, const om_uecc_word_t *native);
/* Converts big-endian bytes to an integer in uECC native format. */
void om_uecc_vli_bytestonative(om_uecc_word_t *native, const uint8_t *bytes, int num_bytes);

unsigned om_uecc_curve_num_words(om_uecc_curve curve);
unsigned om_uecc_curve_num_bytes(om_uecc_curve curve);
unsigned om_uecc_curve_num_bits(om_uecc_curve curve);
unsigned om_uecc_curve_num_n_words(om_uecc_curve curve);
unsigned om_uecc_curve_num_n_bytes(om_uecc_curve curve);
unsigned om_uecc_curve_num_n_bits(om_uecc_curve curve);

const om_uecc_word_t *om_uecc_curve_p(om_uecc_curve curve);
const om_uecc_word_t *om_uecc_curve_n(om_uecc_curve curve);
const om_uecc_word_t *om_uecc_curve_g(om_uecc_curve curve);
const om_uecc_word_t *om_uecc_curve_b(om_uecc_curve curve);

int om_uecc_valid_point(const om_uecc_word_t *point, om_uecc_curve curve);

/* Multiplies a point by a scalar. Points are represented by the X coordinate followed by
   the Y coordinate in the same array, both coordinates are curve->num_words long. Note
   that scalar must be curve->num_n_words long (NOT curve->num_words). */
void om_uecc_point_mult(om_uecc_word_t *result,
                     const om_uecc_word_t *point,
                     const om_uecc_word_t *scalar,
                     om_uecc_curve curve);

/* Generates a random integer in the range 0 < random < top.
   Both random and top have num_words words. */
int om_uecc_generate_random_int(om_uecc_word_t *random,
                             const om_uecc_word_t *top,
                             wordcount_t num_words);

#endif /* OM_UECC_ENABLE_VLI_API */

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* _UECC_VLI_H_ */
