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
 * @brief    SM2 algorithm source file
 * @details  SM2 algorithm source file. Implemeted by software.
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
#if (RTE_SM2)
#include <stdint.h>
#include <string.h>
#include "om_device.h"
#include "om_driver.h"


/*******************************************************************************
 * MACROS
 */
#define DRV_SM2_EXCH_DEFINE(SM2_EXCH_n, sm2_exch_n)     \
static sm2_exch_env_t sm2_exch_n##_env = {              \
    .sm2_exch_id = (uint32_t)OM_##SM2_EXCH_n,           \
}


/*******************************************************************************
 * TYPEDEFS
 */
typedef uint64_t sm2_z256_t[4];
typedef uint64_t sm2_z512_t[8];

typedef struct {
    sm2_z256_t t;                      // tA or tB
    unsigned char xy[64];           // 临时公钥(x1, y1) or (x2, y2)
    unsigned char z[32];            // ZA or ZB
    unsigned char isInitiator;      // 1为发起方，否则为响应方
} sm2_exch_context_t;

typedef struct {
    uint32_t sm2_exch_id;
    sm2_exch_context_t ctx;
} sm2_exch_env_t;

typedef struct {
	sm2_z256_t X;
	sm2_z256_t Y;
	sm2_z256_t Z;
} SM2_Z256_POINT;

typedef struct {
	sm2_z256_t x;
	sm2_z256_t y;
} SM2_Z256_AFFINE_POINT;


/*******************************************************************************
 * CONST & VARIABLES
 */
DRV_SM2_EXCH_DEFINE(SM2_EXCH_0, sm2_exch_0);
DRV_SM2_EXCH_DEFINE(SM2_EXCH_1, sm2_exch_1);

/*
SM2 parameters

p = 0xfffffffeffffffffffffffffffffffffffffffff00000000ffffffffffffffff
a = 0xfffffffeffffffffffffffffffffffffffffffff00000000fffffffffffffffc
b = 0x28e9fa9e9d9f5e344d5a9e4bcf6509a7f39789f515ab8f92ddbcbd414d940e93
x = 0x32c4ae2c1f1981195f9904466a39c9948fe30bbff2660be1715a4589334c74c7
y = 0xbc3736a2f4f6779c59bdcee36b692153d0a9877cc62a474002df32e52139f0a0
n = 0xfffffffeffffffffffffffffffffffff7203df6b21c6052b53bbf40939d54123
h = 0x1
*/

// SM2 a
static const unsigned char GM_ECC_A[] = {
        0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC
};

// SM2 b
static const unsigned char GM_ECC_B[] = {
        0x28, 0xE9, 0xFA, 0x9E, 0x9D, 0x9F, 0x5E, 0x34,
        0x4D, 0x5A, 0x9E, 0x4B, 0xCF, 0x65, 0x09, 0xA7,
        0xF3, 0x97, 0x89, 0xF5, 0x15, 0xAB, 0x8F, 0x92,
        0xDD, 0xBC, 0xBD, 0x41, 0x4D, 0x94, 0x0E, 0x93
};

// SM2 Gx
static const unsigned char GM_ECC_G_X[] = {
        0x32, 0xC4, 0xAE, 0x2C, 0x1F, 0x19, 0x81, 0x19,
        0x5F, 0x99, 0x04, 0x46, 0x6A, 0x39, 0xC9, 0x94,
        0x8F, 0xE3, 0x0B, 0xBF, 0xF2, 0x66, 0x0B, 0xE1,
        0x71, 0x5A, 0x45, 0x89, 0x33, 0x4C, 0x74, 0xC7
};

// SM2 Gy
static const unsigned char GM_ECC_G_Y[] = {
        0xBC, 0x37, 0x36, 0xA2, 0xF4, 0xF6, 0x77, 0x9C,
        0x59, 0xBD, 0xCE, 0xE3, 0x6B, 0x69, 0x21, 0x53,
        0xD0, 0xA9, 0x87, 0x7C, 0xC6, 0x2A, 0x47, 0x40,
        0x02, 0xDF, 0x32, 0xE5, 0x21, 0x39, 0xF0, 0xA0
};

// SM2 G
SM2_Z256_POINT MONT_G = {
	{0x61328990f418029e, 0x3e7981eddca6c050, 0xd6a1ed99ac24c3c3, 0x91167a5ee1c13b05},
	{0xc1354e593c2d0ddd, 0xc1f5e5788d3295fa, 0x8d4cfb066e2a48f8, 0x63cd65d481d735bd},
    {0x0000000000000001, 0x00000000FFFFFFFF, 0x0000000000000000, 0x0000000100000000}
};

static const uint64_t GM_BN_2W[] = {
    0x0000000000000000, 0x8000000000000000, 0x0000000000000000, 0x0000000000000000,
};

static const uint64_t GM_BN_2W_SUB_ONE[] = {
    0xFFFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF, 0x0000000000000000, 0x0000000000000000,
};

const sm2_z256_t SM2_Z256_ONE = { 1,0,0,0 };



/*******************************************************************************
 * LOCAL FUNCTIONS
 */
int sm2_z256_cmp(const sm2_z256_t a, const sm2_z256_t b);
void sm2_z256_modn_to_mont(const sm2_z256_t a, uint64_t r[4]);
void sm2_z256_modn_from_mont(sm2_z256_t r, const sm2_z256_t a);
void sm2_z256_point_add_affine(SM2_Z256_POINT *r, const SM2_Z256_POINT *a, const SM2_Z256_AFFINE_POINT *b);

static sm2_exch_env_t *sm2_exch_get_env(uint32_t om_sm2_exch) {
    if (sm2_exch_0_env.sm2_exch_id == om_sm2_exch) {
        return &sm2_exch_0_env;
    } else if (sm2_exch_1_env.sm2_exch_id == om_sm2_exch) {
        return &sm2_exch_1_env;
    }

    return NULL;
}

const uint64_t *sm2_z256_one(void) {
	return &SM2_Z256_ONE[0];
}

void sm2_z256_set_one(sm2_z256_t r)
{
	r[0] = 1;
	r[1] = 0;
	r[2] = 0;
	r[3] = 0;
}

void sm2_z256_set_zero(sm2_z256_t r)
{
	r[0] = 0;
	r[1] = 0;
	r[2] = 0;
	r[3] = 0;
}

int sm2_z256_rand_range(sm2_z256_t r, const sm2_z256_t range)
{
	unsigned int tries = 100;

	do {
		if (!tries) {
			// caller call this function again if return zero
			return 0;
		}
		randombytes((uint8_t *)r, 32);
		tries--;

	} while (sm2_z256_cmp(r, range) >= 0);

	return 1;
}

void sm2_z256_from_bytes(sm2_z256_t r, const uint8_t in[32])
{
	r[3] = GM_GETU64(in);
	r[2] = GM_GETU64(in + 8);
	r[1] = GM_GETU64(in + 16);
	r[0] = GM_GETU64(in + 24);
}

void sm2_z256_to_bytes(const sm2_z256_t a, uint8_t out[32])
{
	GM_PUTU64(out, a[3]);
	GM_PUTU64(out + 8, a[2]);
	GM_PUTU64(out + 16, a[1]);
	GM_PUTU64(out + 24, a[0]);
}

void sm2_z256_copy(sm2_z256_t r, const sm2_z256_t a)
{
	r[3] = a[3];
	r[2] = a[2];
	r[1] = a[1];
	r[0] = a[0];
}

void sm2_z256_copy_conditional(sm2_z256_t dst, const sm2_z256_t src, uint64_t move)
{
	uint64_t mask1 = 0-move;
	uint64_t mask2 = ~mask1;

	dst[0] = (src[0] & mask1) ^ (dst[0] & mask2);
	dst[1] = (src[1] & mask1) ^ (dst[1] & mask2);
	dst[2] = (src[2] & mask1) ^ (dst[2] & mask2);
	dst[3] = (src[3] & mask1) ^ (dst[3] & mask2);
}

static uint64_t is_zero(uint64_t in)
{
	in |= (0 - in);
	in = ~in;
	in >>= 63;
	return in;
}

uint64_t sm2_z256_equ(const sm2_z256_t a, const sm2_z256_t b)
{
	uint64_t res;

	res = a[0] ^ b[0];
	res |= a[1] ^ b[1];
	res |= a[2] ^ b[2];
	res |= a[3] ^ b[3];

	return is_zero(res);
}

int sm2_z256_cmp(const sm2_z256_t a, const sm2_z256_t b)
{
	if (a[3] > b[3]) return 1;
	else if (a[3] < b[3]) return -1;
	if (a[2] > b[2]) return 1;
	else if (a[2] < b[2]) return -1;
	if (a[1] > b[1]) return 1;
	else if (a[1] < b[1]) return -1;
	if (a[0] > b[0]) return 1;
	else if (a[0] < b[0]) return -1;
	return 0;
}

uint64_t sm2_z256_is_zero(const sm2_z256_t a)
{
	return
		is_zero(a[0]) &
		is_zero(a[1]) &
		is_zero(a[2]) &
		is_zero(a[3]);
}

uint64_t sm2_z256_add(sm2_z256_t r, const sm2_z256_t a, const sm2_z256_t b)
{
	uint64_t t, c = 0;

	t = a[0] + b[0];
	c = t < a[0];
	r[0] = t;

	t = a[1] + c;
	c = t < a[1];
	r[1] = t + b[1];
	c += r[1] < t;

	t = a[2] + c;
	c = t < a[2];
	r[2] = t + b[2];
	c += r[2] < t;

	t = a[3] + c;
	c = t < a[3];
	r[3] = t + b[3];
	c += r[3] < t;

	return c;
}

uint64_t sm2_z256_sub(sm2_z256_t r, const sm2_z256_t a, const sm2_z256_t b)
{
	uint64_t t, c = 0;

	t = a[0] - b[0];
	c = t > a[0];
	r[0] = t;

	t = a[1] - c;
	c = t > a[1];
	r[1] = t - b[1];
	c += r[1] > t;

	t = a[2] - c;
	c = t > a[2];
	r[2] = t - b[2];
	c += r[2] > t;

	t = a[3] - c;
	c = t > a[3];
	r[3] = t - b[3];
	c += r[3] > t;

	return c;
}

void sm2_z256_mul(sm2_z512_t r, const sm2_z256_t a, const sm2_z256_t b)
{
	uint64_t a_[8];
	uint64_t b_[8];
	uint64_t s[16] = {0};
	uint64_t u;
	int i, j;

	for (i = 0; i < 4; i++) {
		a_[2 * i] = a[i] & 0xffffffff;
		b_[2 * i] = b[i] & 0xffffffff;
		a_[2 * i + 1] = a[i] >> 32;
		b_[2 * i + 1] = b[i] >> 32;
	}

	for (i = 0; i < 8; i++) {
		u = 0;
		for (j = 0; j < 8; j++) {
			u = s[i + j] + a_[i] * b_[j] + u;
			s[i + j] = u & 0xffffffff;
			u >>= 32;
		}
		s[i + 8] = u;
	}

	for (i = 0; i < 8; i++) {
		r[i] = (s[2 * i + 1] << 32) | s[2 * i];
	}
}

static uint64_t sm2_z512_add(sm2_z512_t r, const sm2_z512_t a, const sm2_z512_t b)
{
	uint64_t t, c = 0;

	t = a[0] + b[0];
	c = t < a[0];
	r[0] = t;

	t = a[1] + c;
	c = t < a[1];
	r[1] = t + b[1];
	c += r[1] < t;

	t = a[2] + c;
	c = t < a[2];
	r[2] = t + b[2];
	c += r[2] < t;

	t = a[3] + c;
	c = t < a[3];
	r[3] = t + b[3];
	c += r[3] < t;

	t = a[4] + c;
	c = t < a[4];
	r[4] = t + b[4];
	c += r[4] < t;

	t = a[5] + c;
	c = t < a[5];
	r[5] = t + b[5];
	c += r[5] < t;

	t = a[6] + c;
	c = t < a[6];
	r[6] = t + b[6];
	c += r[6] < t;

	t = a[7] + c;
	c = t < a[7];
	r[7] = t + b[7];
	c += r[7] < t;

	return c;
}

int sm2_z256_get_booth(const sm2_z256_t a, unsigned int window_size, int i)
{
	uint64_t mask = (1 << window_size) - 1;
	uint64_t wbits;
	int n, j;

	if (i == 0) {
		return (int)((a[0] << 1) & mask) - (int)(a[0] & mask);
	}

	j = i * window_size - 1;
	n = j / 64;
	j = j % 64;

	wbits = a[n] >> j;
	if ((64 - j) < (int)(window_size + 1) && n < 3) {
		wbits |= a[n + 1] << (64 - j);
	}
	return (int)(wbits & mask) - (int)((wbits >> 1) & mask);
}

// GF(p)

// p = 2^256 - 2^224 - 2^96 + 2^64 - 1
//   = 0xfffffffeffffffffffffffffffffffffffffffff00000000ffffffffffffffff
const uint64_t SM2_Z256_P[4] = {
	0xffffffffffffffff, 0xffffffff00000000, 0xffffffffffffffff, 0xfffffffeffffffff,
};
// TODO: SM2_Z256_P[0] and SM2_Z256_P[2] are special values (fff...f), use this to optimize the ASM code


const uint64_t *sm2_z256_prime(void) {
	return &SM2_Z256_P[0];
}


// 2^256 - p = 2^224 + 2^96 - 2^64 + 1
const uint64_t SM2_Z256_NEG_P[4] = {
	1, ((uint64_t)1 << 32) - 1, 0, ((uint64_t)1 << 32),
};

void sm2_z256_modp_add(sm2_z256_t r, const sm2_z256_t a, const sm2_z256_t b)
{
	uint64_t c;

	c = sm2_z256_add(r, a, b);

	if (c) {
		// a + b - p = (a + b - 2^256) + (2^256 - p)
		(void)sm2_z256_add(r, r, SM2_Z256_NEG_P);
		return;
	}

	if (sm2_z256_cmp(r, SM2_Z256_P) >= 0) {

		(void)sm2_z256_sub(r, r, SM2_Z256_P);
	}
}

void sm2_z256_modp_sub(sm2_z256_t r, const sm2_z256_t a, const sm2_z256_t b)
{
	uint64_t c;

	c = sm2_z256_sub(r, a, b);

	if (c) {
		// a - b + p = (a - b + 2^256) - (2^256 - p)
		(void)sm2_z256_sub(r, r, SM2_Z256_NEG_P);
	}
}

void sm2_z256_modp_dbl(sm2_z256_t r, const sm2_z256_t a)
{
	sm2_z256_modp_add(r, a, a);
}

void sm2_z256_modp_tri(sm2_z256_t r, const sm2_z256_t a)
{
	sm2_z256_t t;
	sm2_z256_modp_add(t, a, a);
	sm2_z256_modp_add(r, t, a);
}

void sm2_z256_modp_neg(sm2_z256_t r, const sm2_z256_t a)
{
	(void)sm2_z256_sub(r, SM2_Z256_P, a);
}

void sm2_z256_modp_haf(sm2_z256_t r, const sm2_z256_t a)
{
	uint64_t c = 0;

	if (a[0] & 1) {
		c = sm2_z256_add(r, a, SM2_Z256_P);
	} else {
		r[0] = a[0];
		r[1] = a[1];
		r[2] = a[2];
		r[3] = a[3];
	}

	r[0] = (r[0] >> 1) | ((r[1] & 1) << 63);
	r[1] = (r[1] >> 1) | ((r[2] & 1) << 63);
	r[2] = (r[2] >> 1) | ((r[3] & 1) << 63);
	r[3] = (r[3] >> 1) | ((c & 1) << 63);
}

// p' * p = -1 mod 2^256

// p' = -p^(-1) mod 2^256
//    = fffffffc00000001fffffffe00000000ffffffff000000010000000000000001
// sage: -(IntegerModRing(2^256)(p))^-1
const uint64_t SM2_Z256_P_PRIME[4] = {
	0x0000000000000001, 0xffffffff00000001, 0xfffffffe00000000, 0xfffffffc00000001,
};


// mont(1) (mod p) = 2^256 mod p = 2^256 - p
const uint64_t *SM2_Z256_MODP_MONT_ONE = SM2_Z256_NEG_P;

// z = a*b
// c = (z + (z * p' mod 2^256) * p)/2^256
void sm2_z256_modp_mont_mul(sm2_z256_t r, const sm2_z256_t a, const sm2_z256_t b)
{
	sm2_z512_t z;
	sm2_z512_t t;
	uint64_t c;

	//sm2_z256_print(stderr, 0, 0, "a", a);
	//sm2_z256_print(stderr, 0, 0, "b", b);

	// z = a * b
	sm2_z256_mul(z, a, b);
	//sm2_z512_print(stderr, 0, 0, "z", z);

	// t = low(z) * p'
	sm2_z256_mul(t, z, SM2_Z256_P_PRIME);
	//sm2_z256_print(stderr, 0, 0, "z * p' mod 2^256", t);

	// t = low(t) * p
	sm2_z256_mul(t, t, SM2_Z256_P);
	//sm2_z512_print(stderr, 0, 0, "(z * p' mod 2^256) * p", t);

	// z = z + t
	c = sm2_z512_add(z, z, t);
	//sm2_z512_print(stderr, 0, 0, "z", z);

	// r = high(r)
	sm2_z256_copy(r, z + 4);
	//sm2_z256_print(stderr, 0, 0, "r", r);

	if (c) {
		sm2_z256_add(r, r, SM2_Z256_MODP_MONT_ONE);
		//sm2_z256_print(stderr, 0, 0, "r1", r);

	} else if (sm2_z256_cmp(r, SM2_Z256_P) >= 0) {
		(void)sm2_z256_sub(r, r, SM2_Z256_P);
		//sm2_z256_print(stderr, 0, 0, "r2", r);
	}
}

void sm2_z256_modp_mont_sqr(sm2_z256_t r, const sm2_z256_t a)
{
	sm2_z256_modp_mont_mul(r, a, a);
}

// mont(mont(a), 1) = aR * 1 * R^-1 (mod p) = a (mod p)
void sm2_z256_modp_from_mont(sm2_z256_t r, const sm2_z256_t a)
{
	sm2_z256_modp_mont_mul(r, a, SM2_Z256_ONE);
}

// 2^512 (mod p)
const uint64_t SM2_Z256_2e512modp[4] = {
	0x0000000200000003, 0x00000002ffffffff, 0x0000000100000001, 0x0000000400000002
};

// mont(a) = a * 2^256 (mod p) = mont_mul(a, 2^512 mod p)
void sm2_z256_modp_to_mont(const sm2_z256_t a, uint64_t r[4])
{
	sm2_z256_modp_mont_mul(r, a, SM2_Z256_2e512modp);
}

void sm2_z256_modp_mont_exp(sm2_z256_t r, const sm2_z256_t a, const sm2_z256_t e)
{
	sm2_z256_t t;
	uint64_t w;
	int i, j;

	// t = mont(1) (mod p)
	sm2_z256_copy(t, SM2_Z256_MODP_MONT_ONE);

	for (i = 3; i >= 0; i--) {
		w = e[i];
		for (j = 0; j < 64; j++) {
			sm2_z256_modp_mont_sqr(t, t);
			if (w & 0x8000000000000000) {
				sm2_z256_modp_mont_mul(t, t, a);
			}
			w <<= 1;
		}
	}

	sm2_z256_copy(r, t);
}

// caller should check a != 0
void sm2_z256_modp_mont_inv(sm2_z256_t r, const sm2_z256_t a)
{
	sm2_z256_t a1;
	sm2_z256_t a2;
	sm2_z256_t a3;
	sm2_z256_t a4;
	sm2_z256_t a5;
	int i;

	sm2_z256_modp_mont_sqr(a1, a);
	sm2_z256_modp_mont_mul(a2, a1, a);
	sm2_z256_modp_mont_sqr(a3, a2);
	sm2_z256_modp_mont_sqr(a3, a3);
	sm2_z256_modp_mont_mul(a3, a3, a2);
	sm2_z256_modp_mont_sqr(a4, a3);
	sm2_z256_modp_mont_sqr(a4, a4);
	sm2_z256_modp_mont_sqr(a4, a4);
	sm2_z256_modp_mont_sqr(a4, a4);
	sm2_z256_modp_mont_mul(a4, a4, a3);
	sm2_z256_modp_mont_sqr(a5, a4);
	for (i = 1; i < 8; i++) {
		sm2_z256_modp_mont_sqr(a5, a5);
	}
	sm2_z256_modp_mont_mul(a5, a5, a4);
	for (i = 0; i < 8; i++) {
		sm2_z256_modp_mont_sqr(a5, a5);
	}
	sm2_z256_modp_mont_mul(a5, a5, a4);
	for (i = 0; i < 4; i++) {
		sm2_z256_modp_mont_sqr(a5, a5);
	}
	sm2_z256_modp_mont_mul(a5, a5, a3);
	sm2_z256_modp_mont_sqr(a5, a5);
	sm2_z256_modp_mont_sqr(a5, a5);
	sm2_z256_modp_mont_mul(a5, a5, a2);
	sm2_z256_modp_mont_sqr(a5, a5);
	sm2_z256_modp_mont_mul(a5, a5, a);
	sm2_z256_modp_mont_sqr(a4, a5);
	sm2_z256_modp_mont_mul(a3, a4, a1);
	sm2_z256_modp_mont_sqr(a5, a4);
	for (i = 1; i< 31; i++) {
		sm2_z256_modp_mont_sqr(a5, a5);
	}
	sm2_z256_modp_mont_mul(a4, a5, a4);
	sm2_z256_modp_mont_sqr(a4, a4);
	sm2_z256_modp_mont_mul(a4, a4, a);
	sm2_z256_modp_mont_mul(a3, a4, a2);
	for (i = 0; i < 33; i++) {
		sm2_z256_modp_mont_sqr(a5, a5);
	}
	sm2_z256_modp_mont_mul(a2, a5, a3);
	sm2_z256_modp_mont_mul(a3, a2, a3);
	for (i = 0; i < 32; i++) {
		sm2_z256_modp_mont_sqr(a5, a5);
	}
	sm2_z256_modp_mont_mul(a2, a5, a3);
	sm2_z256_modp_mont_mul(a3, a2, a3);
	sm2_z256_modp_mont_mul(a4, a2, a4);
	for (i = 0; i < 32; i++) {
		sm2_z256_modp_mont_sqr(a5, a5);
	}
	sm2_z256_modp_mont_mul(a2, a5, a3);
	sm2_z256_modp_mont_mul(a3, a2, a3);
	sm2_z256_modp_mont_mul(a4, a2, a4);
	for (i = 0; i < 32; i++) {
		sm2_z256_modp_mont_sqr(a5, a5);
	}
	sm2_z256_modp_mont_mul(a2, a5, a3);
	sm2_z256_modp_mont_mul(a3, a2, a3);
	sm2_z256_modp_mont_mul(a4, a2, a4);
	for (i = 0; i < 32; i++) {
		sm2_z256_modp_mont_sqr(a5, a5);
	}
	sm2_z256_modp_mont_mul(a2, a5, a3);
	sm2_z256_modp_mont_mul(a3, a2, a3);
	sm2_z256_modp_mont_mul(a4, a2, a4);
	for (i = 0; i < 32; i++) {
		sm2_z256_modp_mont_sqr(a5, a5);
	}
	sm2_z256_modp_mont_mul(r, a4, a5);
}

// (p+1)/4 = 3fffffffbfffffffffffffffffffffffffffffffc00000004000000000000000
const uint64_t SM2_Z256_SQRT_EXP[4] = {
	0x4000000000000000, 0xffffffffc0000000, 0xffffffffffffffff, 0x3fffffffbfffffff,
};

// -r (mod p), i.e. (p - r) is also a square root of a
int sm2_z256_modp_mont_sqrt(sm2_z256_t r, const sm2_z256_t a)
{
	sm2_z256_t a_;
	sm2_z256_t r_; // temp result, prevent call sm2_fp_sqrt(a, a)

	// r = a^((p + 1)/4) when p = 3 (mod 4)
	sm2_z256_modp_mont_exp(r_, a, SM2_Z256_SQRT_EXP);

	// check r^2 == a
	sm2_z256_modp_mont_sqr(a_, r_);
	if (sm2_z256_cmp(a_, a) != 0) {
		// not every number has a square root, so it is not an error
		// `sm2_z256_point_from_hash` need a non-negative return value
		return 0;
	}

	sm2_z256_copy(r, r_);
	return 1;
}

// GF(n)

// n = 0xfffffffeffffffffffffffffffffffff7203df6b21c6052b53bbf40939d54123
const uint64_t SM2_Z256_N[4] = {
	0x53bbf40939d54123, 0x7203df6b21c6052b, 0xffffffffffffffff, 0xfffffffeffffffff,
};

const uint64_t SM2_Z256_N_MINUS_ONE[4] = {
	0x53bbf40939d54122, 0x7203df6b21c6052b, 0xffffffffffffffff, 0xfffffffeffffffff,
};


// 2^256 - n = 0x10000000000000000000000008dfc2094de39fad4ac440bf6c62abedd
const uint64_t SM2_Z256_NEG_N[4] = {
	0xac440bf6c62abedd, 0x8dfc2094de39fad4, 0x0000000000000000, 0x0000000100000000,
};

void sm2_z256_modn_add(sm2_z256_t r, const sm2_z256_t a, const sm2_z256_t b)
{
	uint64_t c;

	c = sm2_z256_add(r, a, b);

	if (c) {
		// a + b - n = (a + b - 2^256) + (2^256 - n)
		(void)sm2_z256_add(r, r, SM2_Z256_NEG_N);
		return;
	}

	if (sm2_z256_cmp(r, SM2_Z256_N) >= 0) {
		(void)sm2_z256_sub(r, r, SM2_Z256_N);
	}
}

void sm2_z256_modn_sub(sm2_z256_t r, const sm2_z256_t a, const sm2_z256_t b)
{
	uint64_t c;

	c = sm2_z256_sub(r, a, b);

	if (c) {
		// a - b + n = (a - b + 2^256) - (2^256 - n)
		(void)sm2_z256_sub(r, r, SM2_Z256_NEG_N);
	}
}

void sm2_z256_modn_neg(sm2_z256_t r, const sm2_z256_t a)
{
	(void)sm2_z256_sub(r, SM2_Z256_N, a);
}

// n' = -n^(-1) mod 2^256
//    = 0x6f39132f82e4c7bc2b0068d3b08941d4df1e8d34fc8319a5327f9e8872350975
// sage: -(IntegerModRing(2^256)(n))^-1
const uint64_t SM2_Z256_N_PRIME[4] = {
	0x327f9e8872350975, 0xdf1e8d34fc8319a5, 0x2b0068d3b08941d4, 0x6f39132f82e4c7bc,
};

const uint64_t *sm2_z256_order(void) {
	return &SM2_Z256_N[0];
}

const uint64_t *sm2_z256_order_minus_one(void) {
	return &SM2_Z256_N_MINUS_ONE[0];
}


// mont(1) (mod n) = 2^256 - n
const uint64_t *SM2_Z256_MODN_MONT_ONE = SM2_Z256_NEG_N;


void sm2_z256_modn_mont_mul(sm2_z256_t r, const sm2_z256_t a, const sm2_z256_t b)
{
	sm2_z512_t z;
	sm2_z512_t t;
	uint64_t c;

	// z = a * b
	sm2_z256_mul(z, a, b);
	//sm2_z512_print(stderr, 0, 0, "z", z);

	// t = low(z) * n'
	sm2_z256_mul(t, z, SM2_Z256_N_PRIME);
	//sm2_z256_print(stderr, 0, 0, "z * n' mod 2^256", t);

	// t = low(t) * n
	sm2_z256_mul(t, t, SM2_Z256_N);
	//sm2_z512_print(stderr, 0, 0, "(z * n' mod 2^256) * n", t);

	// z = z + t
	c = sm2_z512_add(z, z, t);
	//sm2_z512_print(stderr, 0, 0, "z", z);

	// r = high(r)
	sm2_z256_copy(r, z + 4);
	//sm2_z256_print(stderr, 0, 0, "r", r);

	if (c) {
		sm2_z256_add(r, r, SM2_Z256_MODN_MONT_ONE);
		//sm2_z256_print(stderr, 0, 0, "r1", r);

	} else if (sm2_z256_cmp(r, SM2_Z256_N) >= 0) {
		(void)sm2_z256_sub(r, r, SM2_Z256_N);
		//sm2_z256_print(stderr, 0, 0, "r2", r);
	}
}

void sm2_z256_modn_mul(sm2_z256_t r, const sm2_z256_t a, const sm2_z256_t b)
{
	sm2_z256_t mont_a;
	sm2_z256_t mont_b;

	sm2_z256_modn_to_mont(a, mont_a);
	sm2_z256_modn_to_mont(b, mont_b);
	sm2_z256_modn_mont_mul(r, mont_a, mont_b);
	sm2_z256_modn_from_mont(r, r);
}

void sm2_z256_modn_mont_sqr(sm2_z256_t r, const sm2_z256_t a)
{
	sm2_z256_modn_mont_mul(r, a, a);
}

void sm2_z256_modn_sqr(sm2_z256_t r, const sm2_z256_t a)
{
	sm2_z256_t mont_a;

	sm2_z256_modn_to_mont(a, mont_a);
	sm2_z256_modn_mont_sqr(r, mont_a);
	sm2_z256_modn_from_mont(r, r);
}

void sm2_z256_modn_mont_exp(sm2_z256_t r, const sm2_z256_t a, const sm2_z256_t e)
{
	sm2_z256_t t;
	uint64_t w;
	int i, j;

	// t = mont(1)
	sm2_z256_copy(t, SM2_Z256_MODN_MONT_ONE);

	for (i = 3; i >= 0; i--) {
		w = e[i];
		for (j = 0; j < 64; j++) {
			sm2_z256_modn_mont_sqr(t, t);
			if (w & 0x8000000000000000) {
				sm2_z256_modn_mont_mul(t, t, a);
			}
			w <<= 1;
		}
	}

	sm2_z256_copy(r, t);
}

void sm2_z256_modn_exp(sm2_z256_t r, const sm2_z256_t a, const sm2_z256_t e)
{
	sm2_z256_t mont_a;

	sm2_z256_modn_to_mont(a, mont_a);
	sm2_z256_modn_mont_exp(r, mont_a, e);
	sm2_z256_modn_from_mont(r, r);
}

// n - 2 = 0xfffffffeffffffffffffffffffffffff7203df6b21c6052b53bbf40939d54121
const uint64_t SM2_Z256_N_MINUS_TWO[4] = {
	0x53bbf40939d54121, 0x7203df6b21c6052b, 0xffffffffffffffff, 0xfffffffeffffffff,
};
// TODO: use the special form of SM2_Z256_N_MINUS_TWO[2, 3]

void sm2_z256_modn_mont_inv(sm2_z256_t r, const sm2_z256_t a)
{
	// expand sm2_z256_modn_mont_exp(r, a, SM2_Z256_N_MINUS_TWO)
	sm2_z256_t t;
	uint64_t w;
	int i;

	sm2_z256_copy(t, a);

	for (i = 0; i < 30; i++) {
		sm2_z256_modn_mont_sqr(t, t);
		sm2_z256_modn_mont_mul(t, t, a);
	}
	sm2_z256_modn_mont_sqr(t, t);
	for (i = 0; i < 96; i++) {
		sm2_z256_modn_mont_sqr(t, t);
		sm2_z256_modn_mont_mul(t, t, a);
	}
	w = SM2_Z256_N_MINUS_TWO[1];
	for (i = 0; i < 64; i++) {
		sm2_z256_modn_mont_sqr(t, t);
		if (w & 0x8000000000000000) {
			sm2_z256_modn_mont_mul(t, t, a);
		}
		w <<= 1;
	}
	w = SM2_Z256_N_MINUS_TWO[0];
	for (i = 0; i < 64; i++) {
		sm2_z256_modn_mont_sqr(t, t);
		if (w & 0x8000000000000000) {
			sm2_z256_modn_mont_mul(t, t, a);
		}
		w <<= 1;
	}

	sm2_z256_copy(r, t);
}

void sm2_z256_modn_inv(sm2_z256_t r, const sm2_z256_t a)
{
	sm2_z256_t mont_a;

	sm2_z256_modn_to_mont(a, mont_a);
	sm2_z256_modn_mont_inv(r, mont_a);
	sm2_z256_modn_from_mont(r, r);
}


// mont(mont(a), 1) = aR * 1 * R^-1 (mod n) = a (mod p)
void sm2_z256_modn_from_mont(sm2_z256_t r, const sm2_z256_t a)
{
	sm2_z256_modn_mont_mul(r, a, SM2_Z256_ONE);
}

// 2^512 (mod n) = 0x1eb5e412a22b3d3b620fc84c3affe0d43464504ade6fa2fa901192af7c114f20
const uint64_t SM2_Z256_2e512modn[4] = {
	0x901192af7c114f20, 0x3464504ade6fa2fa, 0x620fc84c3affe0d4, 0x1eb5e412a22b3d3b,
};

// mont(a) = a * 2^256 (mod n) = mont_mul(a, 2^512 mod n)
void sm2_z256_modn_to_mont(const sm2_z256_t a, uint64_t r[4])
{
	sm2_z256_modn_mont_mul(r, a, SM2_Z256_2e512modn);
}


// Jacobian Point with Montgomery coordinates

void sm2_z256_point_set_infinity(SM2_Z256_POINT *P)
{
	sm2_z256_copy(P->X, SM2_Z256_MODP_MONT_ONE);
	sm2_z256_copy(P->Y, SM2_Z256_MODP_MONT_ONE);
	sm2_z256_set_zero(P->Z);
}

// point at infinity should be like (k^2 : k^3 : 0), k in [0, p-1]
int sm2_z256_point_is_at_infinity(const SM2_Z256_POINT *P)
{
	if (sm2_z256_is_zero(P->Z)) {
		sm2_z256_t X_cub;
		sm2_z256_t Y_sqr;

		sm2_z256_modp_mont_sqr(X_cub, P->X);
		sm2_z256_modp_mont_mul(X_cub, X_cub, P->X);
		sm2_z256_modp_mont_sqr(Y_sqr, P->Y);

		if (sm2_z256_cmp(X_cub, Y_sqr) != 0) {
			return 0;
		}

		return 1;
	} else {
		return 0;
	}
}

// mont(b), b = 0x28e9fa9e9d9f5e344d5a9e4bcf6509a7f39789f515ab8f92ddbcbd414d940e93
const uint64_t SM2_Z256_MODP_MONT_B[4] = {
	0x90d230632bc0dd42, 0x71cf379ae9b537ab, 0x527981505ea51c3c, 0x240fe188ba20e2c8,
};

int sm2_z256_point_is_on_curve(const SM2_Z256_POINT *P)
{
	sm2_z256_t t0;
	sm2_z256_t t1;
	sm2_z256_t t2;

	if (sm2_z256_cmp(P->Z, SM2_Z256_MODP_MONT_ONE) == 0) {
		// if Z == 1, check y^2 + 3*x == x^3 + b
		sm2_z256_modp_mont_sqr(t0, P->Y);
		sm2_z256_modp_add(t0, t0, P->X);
		sm2_z256_modp_add(t0, t0, P->X);
		sm2_z256_modp_add(t0, t0, P->X);
		sm2_z256_modp_mont_sqr(t1, P->X);
		sm2_z256_modp_mont_mul(t1, t1, P->X);
		sm2_z256_modp_add(t1, t1, SM2_Z256_MODP_MONT_B);
	} else {
		// check Y^2 + 3 * X * Z^4 == X^3 + b * Z^6
		// if Z == 0, Y^2 == X^3, i.e. Y == X is checked
		sm2_z256_modp_mont_sqr(t0, P->Y);
		sm2_z256_modp_mont_sqr(t1, P->Z);
		sm2_z256_modp_mont_sqr(t2, t1);
		sm2_z256_modp_mont_mul(t1, t1, t2);
		sm2_z256_modp_mont_mul(t1, t1, SM2_Z256_MODP_MONT_B);
		sm2_z256_modp_mont_mul(t2, t2, P->X);
		sm2_z256_modp_add(t0, t0, t2);
		sm2_z256_modp_add(t0, t0, t2);
		sm2_z256_modp_add(t0, t0, t2);
		sm2_z256_modp_mont_sqr(t2, P->X);
		sm2_z256_modp_mont_mul(t2, t2, P->X);
		sm2_z256_modp_add(t1, t1, t2);
	}

	if (sm2_z256_cmp(t0, t1) != 0) {
		return 0;
	}
	return 1;
}

int sm2_z256_point_get_xy(const SM2_Z256_POINT *P, uint64_t x[4], uint64_t y[4])
{
	if (sm2_z256_point_is_at_infinity(P) == 1) {
		sm2_z256_set_zero(x);
		if (y) {
			sm2_z256_set_zero(y);
		}
		return 0;
	}

	if (sm2_z256_cmp(P->Z, SM2_Z256_MODP_MONT_ONE) == 0) {
		sm2_z256_modp_from_mont(x, P->X);
		if (y) {
			sm2_z256_modp_from_mont(y, P->Y);
		}
	} else {
		sm2_z256_t z_inv;

		sm2_z256_modp_mont_inv(z_inv, P->Z);
		if (y) {
			sm2_z256_modp_mont_mul(y, P->Y, z_inv);
		}
		sm2_z256_modp_mont_sqr(z_inv, z_inv);
		sm2_z256_modp_mont_mul(x, P->X, z_inv);
		sm2_z256_modp_from_mont(x, x);
		if (y) {
			sm2_z256_modp_mont_mul(y, y, z_inv);
			sm2_z256_modp_from_mont(y, y);
		}
	}

	return 1;
}

void sm2_z256_point_dbl(SM2_Z256_POINT *R, const SM2_Z256_POINT *A)
{
	const uint64_t *X1 = A->X;
	const uint64_t *Y1 = A->Y;
	const uint64_t *Z1 = A->Z;
	uint64_t *X3 = R->X;
	uint64_t *Y3 = R->Y;
	uint64_t *Z3 = R->Z;
	sm2_z256_t S;
	sm2_z256_t M;
	sm2_z256_t Zsqr;
	sm2_z256_t tmp0;

	// 1. S = 2Y
	sm2_z256_modp_dbl(S, Y1);

	// 2. Zsqr = Z^2
	sm2_z256_modp_mont_sqr(Zsqr, Z1);

	// 3. S = S^2 = 4Y^2
	sm2_z256_modp_mont_sqr(S, S);

	// 4. Z = Z*Y
	sm2_z256_modp_mont_mul(Z3, Z1, Y1);

	// 5. Z = 2*Z = 2*Y*Z ===> Z3
	sm2_z256_modp_dbl(Z3, Z3);

	// 6. M = X + Zsqr = X + Z^2
	sm2_z256_modp_add(M, X1, Zsqr);

	// 7. Zsqr = X - Zsqr = X - Z^2
	sm2_z256_modp_sub(Zsqr, X1, Zsqr);

	// 8. Y = S^2 = 16Y^4
	sm2_z256_modp_mont_sqr(Y3, S);

	// 9. Y = Y/2 = 8Y^4
	sm2_z256_modp_haf(Y3, Y3);

	// 10. M = M * Zsqr = (X + Z^2)*(X - Z^2) = X^2 - Z^4
	sm2_z256_modp_mont_mul(M, M, Zsqr);

	// 11. M = 3M = 3X^2 - 3Z^4
	sm2_z256_modp_tri(M, M);

	// 12. S = S * X = 4X*Y^2
	sm2_z256_modp_mont_mul(S, S, X1);

	// 13. tmp0 = 2 * S = 8X*Y^2
	sm2_z256_modp_dbl(tmp0, S);

	// 14. X = M^2 = (3X^2 - 3Z^4)^2
	sm2_z256_modp_mont_sqr(X3, M);

	// 15. X = X - tmp0 = (3X^2 - 3Z^4)^2 - 8X*Y^2 ===> X3
	sm2_z256_modp_sub(X3, X3, tmp0);

	// 16. S = S - X3 = 4X*Y^2 - X3
	sm2_z256_modp_sub(S, S, X3);

	// 17. S = S * M = (3X^2 - 3Z^4)*(4X*Y^2 - X3)
	sm2_z256_modp_mont_mul(S, S, M);

	// 18. Y = S - Y = (3X^2 - 3Z^4)*(4X*Y^2 - X3) - 8Y^4 ===> Y3
	sm2_z256_modp_sub(Y3, S, Y3);
}

/*
  (X1:Y1:Z1) + (X2:Y2:Z2) => (X3:Y3:Z3)

	A = Y2 * Z1^3 - Y1 * Z2^3
	B = X2 * Z1^2 - X1 * Z2^2

	X3 = A^2 - B^2 * (X2 * Z1^2 + X1 * Z2^2)
	   = A^2 - B^3 - 2 * B^2 * X1 * Z2^2
	Y3 = A * (X1 * B^2 * Z2^2 - X3) - Y1 * B^3 * Z2^3
	Z3 = B * Z1 * Z2

  P + (-P) = (X:Y:Z) + (k^2*X : k^3*Y : k*Z) => (0:0:0)
*/
void sm2_z256_point_add(SM2_Z256_POINT *r, const SM2_Z256_POINT *a, const SM2_Z256_POINT *b)
{
	sm2_z256_t U2, S2;
	sm2_z256_t U1, S1;
	sm2_z256_t Z1sqr;
	sm2_z256_t Z2sqr;
	sm2_z256_t H, R;
	sm2_z256_t Hsqr;
	sm2_z256_t Rsqr;
	sm2_z256_t Hcub;

	sm2_z256_t res_x;
	sm2_z256_t res_y;
	sm2_z256_t res_z;

	uint64_t in1infty, in2infty;

	const uint64_t *in1_x = a->X;
	const uint64_t *in1_y = a->Y;
	const uint64_t *in1_z = a->Z;

	const uint64_t *in2_x = b->X;
	const uint64_t *in2_y = b->Y;
	const uint64_t *in2_z = b->Z;

	/*
	* Infinity in encoded as (,,0)
	*/
	in1infty = (in1_z[0] | in1_z[1] | in1_z[2] | in1_z[3]);

	in2infty = (in2_z[0] | in2_z[1] | in2_z[2] | in2_z[3]);

	in1infty = is_zero(in1infty);
	in2infty = is_zero(in2infty);

	// TODO: can we parallel on the following code?
	sm2_z256_modp_mont_sqr(Z2sqr, in2_z);        /* Z2^2 */
	sm2_z256_modp_mont_sqr(Z1sqr, in1_z);        /* Z1^2 */

	sm2_z256_modp_mont_mul(S1, Z2sqr, in2_z);    /* S1 = Z2^3 */
	sm2_z256_modp_mont_mul(S2, Z1sqr, in1_z);    /* S2 = Z1^3 */

	sm2_z256_modp_mont_mul(S1, S1, in1_y);       /* S1 = Y1*Z2^3 */
	sm2_z256_modp_mont_mul(S2, S2, in2_y);       /* S2 = Y2*Z1^3 */
	sm2_z256_modp_sub(R, S2, S1);                /* R = S2 - S1 */

	sm2_z256_modp_mont_mul(U1, in1_x, Z2sqr);    /* U1 = X1*Z2^2 */
	sm2_z256_modp_mont_mul(U2, in2_x, Z1sqr);    /* U2 = X2*Z1^2 */
	sm2_z256_modp_sub(H, U2, U1);                /* H = U2 - U1 */

	/*
	* This should not happen during sign/ecdh, so no constant time violation
	*/
	if (sm2_z256_equ(U1, U2) && !in1infty && !in2infty) {
		if (sm2_z256_equ(S1, S2)) {
			sm2_z256_point_dbl(r, a);
			return;
		} else {
			memset(r, 0, sizeof(*r));
			return;
		}
	}

	sm2_z256_modp_mont_sqr(Rsqr, R);             /* R^2 */
	sm2_z256_modp_mont_mul(res_z, H, in1_z);     /* Z3 = H*Z1*Z2 */

	sm2_z256_modp_mont_sqr(Hsqr, H);             /* H^2 */
	sm2_z256_modp_mont_mul(res_z, res_z, in2_z); /* Z3 = H*Z1*Z2 */

	sm2_z256_modp_mont_mul(Hcub, Hsqr, H);       /* H^3 */
	sm2_z256_modp_mont_mul(U2, U1, Hsqr);        /* U1*H^2 */

	sm2_z256_modp_dbl(Hsqr, U2);            /* 2*U1*H^2 */

	sm2_z256_modp_sub(res_x, Rsqr, Hsqr);
	sm2_z256_modp_sub(res_x, res_x, Hcub);

	sm2_z256_modp_sub(res_y, U2, res_x);

	sm2_z256_modp_mont_mul(S2, S1, Hcub);
	sm2_z256_modp_mont_mul(res_y, R, res_y);

	sm2_z256_modp_sub(res_y, res_y, S2);

	sm2_z256_copy_conditional(res_x, in2_x, in1infty);
	sm2_z256_copy_conditional(res_y, in2_y, in1infty);
	sm2_z256_copy_conditional(res_z, in2_z, in1infty);

	sm2_z256_copy_conditional(res_x, in1_x, in2infty);
	sm2_z256_copy_conditional(res_y, in1_y, in2infty);
	sm2_z256_copy_conditional(res_z, in1_z, in2infty);

	memcpy(r->X, res_x, sizeof(res_x));
	memcpy(r->Y, res_y, sizeof(res_y));
	memcpy(r->Z, res_z, sizeof(res_z));
}

void sm2_z256_point_neg(SM2_Z256_POINT *R, const SM2_Z256_POINT *P)
{
	sm2_z256_copy(R->X, P->X);
	sm2_z256_modp_neg(R->Y, P->Y);
	sm2_z256_copy(R->Z, P->Z);
}

void sm2_z256_point_sub(SM2_Z256_POINT *R, const SM2_Z256_POINT *A, const SM2_Z256_POINT *B)
{
	SM2_Z256_POINT neg_B;
	sm2_z256_point_neg(&neg_B, B);
	sm2_z256_point_add(R, A, &neg_B);
}

void sm2_z256_point_mul_pre_compute(const SM2_Z256_POINT *P, SM2_Z256_POINT T[16])
{
	memcpy(&T[0], P, sizeof(SM2_Z256_POINT));

	if (sm2_z256_equ(P->Z, SM2_Z256_MODP_MONT_ONE) == 1) {
		const SM2_Z256_AFFINE_POINT *P_ = (const SM2_Z256_AFFINE_POINT *)P;
		sm2_z256_point_dbl(&T[ 1], &T[ 0]);
		sm2_z256_point_add_affine(&T[ 2], &T[ 1], P_);
		sm2_z256_point_dbl(&T[ 3], &T[ 1]);
		sm2_z256_point_add_affine(&T[ 4], &T[ 3], P_);
		sm2_z256_point_dbl(&T[ 5], &T[ 2]);
		sm2_z256_point_add_affine(&T[ 6], &T[ 5], P_);
		sm2_z256_point_dbl(&T[ 7], &T[ 3]);
		sm2_z256_point_add_affine(&T[ 8], &T[ 7], P_);
		sm2_z256_point_dbl(&T[ 9], &T[ 4]);
		sm2_z256_point_add_affine(&T[10], &T[ 9], P_);
		sm2_z256_point_dbl(&T[11], &T[ 5]);
		sm2_z256_point_add_affine(&T[12], &T[11], P_);
		sm2_z256_point_dbl(&T[13], &T[ 6]);
		sm2_z256_point_add_affine(&T[14], &T[13], P_);
		sm2_z256_point_dbl(&T[15], &T[ 7]);
	} else {
		sm2_z256_point_dbl(&T[2-1], &T[1-1]);
		sm2_z256_point_dbl(&T[4-1], &T[2-1]);
		sm2_z256_point_dbl(&T[8-1], &T[4-1]);
		sm2_z256_point_dbl(&T[16-1], &T[8-1]);
		sm2_z256_point_add(&T[3-1], &T[2-1], P);
		sm2_z256_point_dbl(&T[6-1], &T[3-1]);
		sm2_z256_point_dbl(&T[12-1], &T[6-1]);
		sm2_z256_point_add(&T[5-1], &T[3-1], &T[2-1]);
		sm2_z256_point_dbl(&T[10-1], &T[5-1]);
		sm2_z256_point_add(&T[7-1], &T[4-1], &T[3-1]);
		sm2_z256_point_dbl(&T[14-1], &T[7-1]);
		sm2_z256_point_add(&T[9-1], &T[4-1], &T[5-1]);
		sm2_z256_point_add(&T[11-1], &T[6-1], &T[5-1]);
		sm2_z256_point_add(&T[13-1], &T[7-1], &T[6-1]);
		sm2_z256_point_add(&T[15-1], &T[8-1], &T[7-1]);
	}
}

void sm2_z256_point_mul_ex(SM2_Z256_POINT *R, const uint64_t k[4], const SM2_Z256_POINT *T)
{
	int window_size = 5;
	int R_infinity = 1;
	int n = (256 + window_size - 1)/window_size;
	int i;

	for (i = n - 1; i >= 0; i--) {
		int booth = sm2_z256_get_booth(k, window_size, i);

		if (R_infinity) {
			if (booth != 0) {
				*R = T[booth - 1];
				R_infinity = 0;
			}
		} else {
			sm2_z256_point_dbl(R, R);
			sm2_z256_point_dbl(R, R);
			sm2_z256_point_dbl(R, R);
			sm2_z256_point_dbl(R, R);
			sm2_z256_point_dbl(R, R);

			if (booth > 0) {
				sm2_z256_point_add(R, R, &T[booth - 1]);
			} else if (booth < 0) {
				sm2_z256_point_sub(R, R, &T[-booth - 1]);
			}
		}
	}

	if (R_infinity) {
		memset(R, 0, sizeof(*R));
	}

}

void sm2_z256_point_mul(SM2_Z256_POINT *R, const sm2_z256_t k, const SM2_Z256_POINT *P)
{
	int window_size = 5;
	SM2_Z256_POINT T[16];
	int R_infinity = 1;
	int n = (256 + window_size - 1)/window_size;
	int i;

	sm2_z256_point_mul_pre_compute(P, T);

	for (i = n - 1; i >= 0; i--) {
		int booth = sm2_z256_get_booth(k, window_size, i);

		if (R_infinity) {
			if (booth != 0) {
				*R = T[booth - 1];
				R_infinity = 0;
			}
		} else {
			sm2_z256_point_dbl(R, R);
			sm2_z256_point_dbl(R, R);
			sm2_z256_point_dbl(R, R);
			sm2_z256_point_dbl(R, R);
			sm2_z256_point_dbl(R, R);

			if (booth > 0) {
				sm2_z256_point_add(R, R, &T[booth - 1]);
			} else if (booth < 0) {
				sm2_z256_point_sub(R, R, &T[-booth - 1]);
			}
		}
	}

	if (R_infinity) {
		memset(R, 0, sizeof(*R));
	}
}


void sm2_z256_point_copy_affine(SM2_Z256_POINT *R, const SM2_Z256_AFFINE_POINT *P)
{
	memcpy(R, P, sizeof(SM2_Z256_AFFINE_POINT));
	sm2_z256_copy(R->Z, SM2_Z256_MODP_MONT_ONE);
}

void sm2_z256_point_add_affine(SM2_Z256_POINT *r, const SM2_Z256_POINT *a, const SM2_Z256_AFFINE_POINT *b)
{
	sm2_z256_t U2, S2;
	sm2_z256_t Z1sqr;
	sm2_z256_t H, R;
	sm2_z256_t Hsqr;
	sm2_z256_t Rsqr;
	sm2_z256_t Hcub;

	sm2_z256_t res_x;
	sm2_z256_t res_y;
	sm2_z256_t res_z;

	uint64_t in1infty, in2infty;

	const uint64_t *in1_x = a->X;
	const uint64_t *in1_y = a->Y;
	const uint64_t *in1_z = a->Z;

	const uint64_t *in2_x = b->x;
	const uint64_t *in2_y = b->y;

	/*
	* Infinity in encoded as (,,0)
	*/
	in1infty = (in1_z[0] | in1_z[1] | in1_z[2] | in1_z[3]);

	/*
	* In affine representation we encode infinity as (0,0), which is
	* not on the curve, so it is OK
	*/
	in2infty = (in2_x[0] | in2_x[1] | in2_x[2] | in2_x[3] | in2_y[0] | in2_y[1] | in2_y[2] | in2_y[3]);

	in1infty = is_zero(in1infty);
	in2infty = is_zero(in2infty);


	/* Z1^2 */
	sm2_z256_modp_mont_sqr(Z1sqr, in1_z);

	/* U2 = X2*Z1^2 */
	sm2_z256_modp_mont_mul(U2, in2_x, Z1sqr);
	/* H = U2 - U1 */
	sm2_z256_modp_sub(H, U2, in1_x);

	sm2_z256_modp_mont_mul(S2, Z1sqr, in1_z);    /* S2 = Z1^3 */

	sm2_z256_modp_mont_mul(res_z, H, in1_z);     /* Z3 = H*Z1*Z2 */

	sm2_z256_modp_mont_mul(S2, S2, in2_y);       /* S2 = Y2*Z1^3 */
	sm2_z256_modp_sub(R, S2, in1_y);             /* R = S2 - S1 */

	sm2_z256_modp_mont_sqr(Hsqr, H);             /* H^2 */
	sm2_z256_modp_mont_sqr(Rsqr, R);             /* R^2 */
	sm2_z256_modp_mont_mul(Hcub, Hsqr, H);       /* H^3 */

	sm2_z256_modp_mont_mul(U2, in1_x, Hsqr);     /* U1*H^2 */
	sm2_z256_modp_dbl(Hsqr, U2);            /* 2*U1*H^2 */

	sm2_z256_modp_sub(res_x, Rsqr, Hsqr);
	sm2_z256_modp_sub(res_x, res_x, Hcub);
	sm2_z256_modp_sub(H, U2, res_x);

	sm2_z256_modp_mont_mul(S2, in1_y, Hcub);
	sm2_z256_modp_mont_mul(H, H, R);
	sm2_z256_modp_sub(res_y, H, S2);

	sm2_z256_copy_conditional(res_x, in2_x, in1infty);
	sm2_z256_copy_conditional(res_x, in1_x, in2infty);

	sm2_z256_copy_conditional(res_y, in2_y, in1infty);
	sm2_z256_copy_conditional(res_y, in1_y, in2infty);

	sm2_z256_copy_conditional(res_z, SM2_Z256_MODP_MONT_ONE, in1infty);
	sm2_z256_copy_conditional(res_z, in1_z, in2infty);

	memcpy(r->X, res_x, sizeof(res_x));
	memcpy(r->Y, res_y, sizeof(res_y));
	memcpy(r->Z, res_z, sizeof(res_z));
}

void sm2_z256_point_sub_affine(SM2_Z256_POINT *R,
	const SM2_Z256_POINT *A, const SM2_Z256_AFFINE_POINT *B)
{
	SM2_Z256_AFFINE_POINT neg_B;

	sm2_z256_copy(neg_B.x, B->x);
	sm2_z256_modp_neg(neg_B.y, B->y);

	sm2_z256_point_add_affine(R, A, &neg_B);
}

// point_at_infinity can not be encoded/decoded to/from bytes
int sm2_z256_point_from_bytes(SM2_Z256_POINT *P, const uint8_t in[64])
{
	sm2_z256_from_bytes(P->X, in);
	if (sm2_z256_cmp(P->X, sm2_z256_prime()) >= 0) {
		return -1;
	}
	sm2_z256_from_bytes(P->Y, in + 32);
	if (sm2_z256_cmp(P->Y, sm2_z256_prime()) >= 0) {
		return -1;
	}

	// point_at_infinity
	if (sm2_z256_is_zero(P->X) == 1 && sm2_z256_is_zero(P->Y) == 1) {
		sm2_z256_point_set_infinity(P);
		return 0;
	}

	sm2_z256_modp_to_mont(P->X, P->X);
	sm2_z256_modp_to_mont(P->Y, P->Y);
	sm2_z256_copy(P->Z, SM2_Z256_MODP_MONT_ONE);

	if (sm2_z256_point_is_on_curve(P) != 1) {
		return -1;
	}
	return 1;
}

int sm2_z256_point_set_xy(SM2_Z256_POINT *R, const sm2_z256_t x, const sm2_z256_t y)
{
	if (sm2_z256_cmp(x, sm2_z256_prime()) >= 0) {
		return -1;
	}
	if (sm2_z256_cmp(y, sm2_z256_prime()) >= 0) {
		return -1;
	}

	sm2_z256_modp_to_mont(x, R->X);
	sm2_z256_modp_to_mont(y, R->Y);
	sm2_z256_copy(R->Z, SM2_Z256_MODP_MONT_ONE);

	if (sm2_z256_point_is_on_curve(R) != 1) {
		return -1;
	}
	return 1;
}

// point_at_infinity should not to_bytes
int sm2_z256_point_to_bytes(const SM2_Z256_POINT *P, uint8_t out[64])
{
	sm2_z256_t x;
	sm2_z256_t y;
	int ret;

	if ((ret = sm2_z256_point_get_xy(P, x, y)) < 0) {
		return -1;
	}
	sm2_z256_to_bytes(x, out);
	sm2_z256_to_bytes(y, out + 32);
	return ret;
}

int sm2_z256_point_equ(const SM2_Z256_POINT *P, const SM2_Z256_POINT *Q)
{
	sm2_z256_t Z1;
	sm2_z256_t Z2;
	sm2_z256_t V1;
	sm2_z256_t V2;

	// X1 * Z2^2 == X2 * Z1^2
	sm2_z256_modp_mont_sqr(Z1, P->Z);
	sm2_z256_modp_mont_sqr(Z2, Q->Z);
	sm2_z256_modp_mont_mul(V1, P->X, Z2);
	sm2_z256_modp_mont_mul(V2, Q->X, Z1);
	if (sm2_z256_cmp(V1, V2) != 0) {
		return 0;
	}

	// Y1 * Z2^3 == Y2 * Z1^3
	sm2_z256_modp_mont_mul(Z1, Z1, P->Z);
	sm2_z256_modp_mont_mul(Z2, Z2, Q->Z);
	sm2_z256_modp_mont_mul(V1, P->Y, Z2);
	sm2_z256_modp_mont_mul(V2, Q->Y, Z1);
	if (sm2_z256_cmp(V1, V2) != 0) {
		return 0;
	}

	return 1;
}

int sm2_z256_is_odd(const sm2_z256_t a)
{
	return a[0] & 0x01;
}

// return 0 if no point for given x coordinate
int sm2_z256_point_from_x_bytes(SM2_Z256_POINT *P, const uint8_t x_bytes[32], int y_is_odd)
{
	// mont(3), i.e. mont(-b)
	const uint64_t SM2_Z256_MODP_MONT_THREE[4] = {
		0x0000000000000003, 0x00000002fffffffd, 0x0000000000000000, 0x0000000300000000
	};

	sm2_z256_t x;
	sm2_z256_t y;
	sm2_z256_t y_sqr;
	int ret;

	sm2_z256_from_bytes(x, x_bytes);
	if (sm2_z256_cmp(x, SM2_Z256_P) >= 0) {
		return -1;
	}
	sm2_z256_modp_to_mont(x, x);
	sm2_z256_copy(P->X, x);

	// y^2 = x^3 - 3x + b = (x^2 - 3)*x + b
	sm2_z256_modp_mont_sqr(y_sqr, x);
	sm2_z256_modp_sub(y_sqr, y_sqr, SM2_Z256_MODP_MONT_THREE);
	sm2_z256_modp_mont_mul(y_sqr, y_sqr, x);
	sm2_z256_modp_add(y_sqr, y_sqr, SM2_Z256_MODP_MONT_B);

	// y = sqrt(y^2)
	if ((ret = sm2_z256_modp_mont_sqrt(y, y_sqr)) != 1) {
		return ret;
	}

	sm2_z256_copy(P->Y , y); // mont(y)

	sm2_z256_modp_from_mont(y, y);
	if (y_is_odd) {
		if (!sm2_z256_is_odd(y)) {
			sm2_z256_modp_neg(P->Y, P->Y);
		}
	} else {
		if (sm2_z256_is_odd(y)) {
			sm2_z256_modp_neg(P->Y, P->Y);
		}
	}

	sm2_z256_copy(P->Z, SM2_Z256_MODP_MONT_ONE);

	return 1;
}

static void sm2_compute_z_digest(const uint8_t * id_bytes, uint32_t idLen,
                                 const SM2_Z256_POINT *pub_key, uint8_t output[32]) {
    sm3_ctx_t ctx;
	sm2_z256_t x, y;

    drv_sm3_start(&ctx);

    // update idlen，这里的idlen是比特长度，所以移位注意一下
    uint8_t idbits[2];
	idbits[0] = (uint8_t)(idLen >> 5);
	idbits[1] = (uint8_t)(idLen << 3);
	drv_sm3_update(&ctx, idbits, 2);

    // update id
    drv_sm3_update(&ctx, id_bytes, idLen);

    // update a
    drv_sm3_update(&ctx, GM_ECC_A, 32);

    // update b
    drv_sm3_update(&ctx, GM_ECC_B, 32);

    // update Gx
    drv_sm3_update(&ctx, GM_ECC_G_X, 32);

    // update Gy
    drv_sm3_update(&ctx, GM_ECC_G_Y, 32);

    sm2_z256_point_get_xy(pub_key, x, y);

    // update Px
    sm2_z256_to_bytes(x, output);// 借用output当缓冲区
    drv_sm3_update(&ctx, output, 32);

    // update Py
    sm2_z256_to_bytes(y, output); // 借用output当缓冲区
    drv_sm3_update(&ctx, output, 32);

    drv_sm3_stop(&ctx, output);
}

// 2^w + ( x & ( 2^w − 1 ) )
static void sm2_exch_reduce(sm2_z256_t x) {
    for(uint32_t i = 0; i < 4; i++) {
        x[i] &= GM_BN_2W_SUB_ONE[i];
        x[i] += GM_BN_2W[i];
    }
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief Generate sm2 public key from private key
 *
 * @param[in] private_key private key
 * @param[out] public_key public key
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_sm2_generate_public_key(const uint8_t private_key[32], uint8_t public_key[64])
{
    SM2_Z256_POINT public_key_tmp;
    sm2_z256_t private_key_tmp;

    if (private_key == NULL || public_key == NULL) {
        return OM_ERROR_PARAMETER;
    }

    sm2_z256_from_bytes(private_key_tmp, private_key);
    sm2_z256_point_mul(&public_key_tmp, private_key_tmp, &MONT_G);
    sm2_z256_point_to_bytes(&public_key_tmp, public_key);

    return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief Generate sm2 key pair
 *
 * @param[out] private_key private key
 * @param[out] public_key public key
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_sm2_key_generate(uint8_t private_key[32], uint8_t public_key[64])
{
    if (private_key == NULL || public_key == NULL) {
        return OM_ERROR_PARAMETER;
    }

    sm2_z256_t private_key_tmp;
	// rand sk in [1, n-2]
	do {
		if (sm2_z256_rand_range(private_key_tmp, sm2_z256_order_minus_one()) != 1) {
			return OM_ERROR_OUT_OF_RANGE;
		}
	} while (sm2_z256_is_zero(private_key_tmp));

    sm2_z256_to_bytes(private_key_tmp, private_key);
    drv_sm2_generate_public_key(private_key, public_key);

	return OM_ERROR_OK;
}

/**
 *******************************************************************************
 * @brief key exchange initialization
 *
 * @param[in] om_sm2_exch sm2 exch ID
 * @param[in] private_key private key
 * @param[in] public_key public key
 * @param[in] rand_private_key random private key
 * @param[in] rand_public_key random public key
 * @param[in] isInitiator 1:requester, 0:responder
 * @param[in] id_bytes  user id
 * @param[in] idLen user id length
 *******************************************************************************
 */
void drv_sm2_exch_init(uint32_t om_sm2_exch,
                       uint8_t private_key[32], uint8_t public_key[64],
                       uint8_t rand_private_key[32], uint8_t rand_public_key[64],
                       unsigned char isInitiator,
                       const unsigned char *id_bytes, unsigned int idLen)
{
    sm2_exch_env_t *env;

    sm2_z256_t r, x, y, res, private_key_tmp;
    SM2_Z256_POINT pr, public_key_tmp;

    env = sm2_exch_get_env(om_sm2_exch);
    if (env == NULL) {
        OM_ASSERT(0);
        return;
    }

    sm2_z256_from_bytes(private_key_tmp, private_key);
    sm2_z256_point_from_bytes(&public_key_tmp, public_key);
    sm2_z256_from_bytes(r, rand_private_key);
    sm2_z256_point_from_bytes(&pr, rand_public_key);

    sm2_z256_point_get_xy(&pr, x, y);

    // 2^w + ( x & ( 2^w − 1 ) )
    sm2_exch_reduce(x);

    // t = (d + x · r) mod n
    sm2_z256_modn_mul(res, x, r);
    sm2_z256_modn_add(env->ctx.t, res, private_key_tmp);

    // compute z digest
    sm2_compute_z_digest(id_bytes, idLen, &public_key_tmp, env->ctx.z);

    env->ctx.isInitiator = isInitiator;

    sm2_z256_point_to_bytes(&pr, env->ctx.xy);
}

/**
 *******************************************************************************
 * @brief Calculate exchange key and hash
 *
 * @param[in] om_sm2_exch sm2 exch ID
 * @param[in] peer_p peer public key
 * @param[in] peer_rp peer random public key
 * @param[in] peer_id_bytes peer user id
 * @param[in] idLen peer user id length
 * @param[in] kLen key length
 * @param[out] output key || S1/SB || S2/SA，Len is kLen + 64
 *******************************************************************************
 */
void drv_sm2_exch_calculate(uint32_t om_sm2_exch,
                            const uint8_t *peer_p, const uint8_t *peer_rp,
                            const uint8_t *peer_id_bytes, uint32_t idLen,
                            uint32_t kLen,
                            uint8_t *output)
{
    sm2_exch_env_t *env;
    unsigned char buf[100] = {0};
    unsigned char peerZ[32] = {0};
    int i, ki, kn, ct;

    sm2_z256_t peerTmpPubK_X;
    SM2_Z256_POINT peerPubK, peerTmpPubK, res, res1;
    sm3_ctx_t sm3_ctx;

    env = sm2_exch_get_env(om_sm2_exch);
    if (env == NULL) {
        OM_ASSERT(0);
    }

    sm2_z256_from_bytes(peerTmpPubK_X, peer_rp); // 取出随机公钥中的x
    sm2_z256_point_from_bytes(&peerPubK, peer_p);
    sm2_z256_point_from_bytes(&peerTmpPubK, peer_rp);

    // compute peer z digest
    sm2_compute_z_digest(peer_id_bytes, idLen, &peerPubK, peerZ);

    // 2^w + ( peerTmpX & ( 2^w − 1 ) )
    sm2_exch_reduce(peerTmpPubK_X);

    // U = t * (peerPubK + peerTmpPubK_X · peerTmpPubK)
    sm2_z256_point_mul(&res, peerTmpPubK_X, &peerTmpPubK);
    sm2_z256_point_add(&res1, &peerPubK, &res);
    sm2_z256_point_mul(&res, env->ctx.t, &res1);
    sm2_z256_point_to_bytes(&res, buf); // U

    // KDF(x_u || y_u || Z_A || Z_B)
    kn = (kLen + 31) / 32;
    ki = 0;
    for(i = 0, ct = 1; i < kn; i++, ct++) {

        drv_sm3_start(&sm3_ctx);
        drv_sm3_update(&sm3_ctx, buf, 64);

        if(env->ctx.isInitiator) {
            drv_sm3_update(&sm3_ctx, env->ctx.z, 32);
            drv_sm3_update(&sm3_ctx, peerZ, 32);
        }else {
            drv_sm3_update(&sm3_ctx, peerZ, 32);
            drv_sm3_update(&sm3_ctx, env->ctx.z, 32);
        }

        GM_PUT_UINT32_BE(ct, buf + 64, 0);
        drv_sm3_update(&sm3_ctx, buf + 64, 4);
        drv_sm3_stop(&sm3_ctx, buf + 68);

        // output kA or kB
        if(i == (kn - 1)) {
            memcpy(output + ki, buf + 68, (kLen - ki));
        }else {
            memcpy(output + ki, buf + 68, 32);
            ki += 32;
        }
    }

    // Hash(0x02 || y_u || Hash(x_u || Z_A || Z_B || x_1 || y_1 || x_2 || y_2))
    drv_sm3_start(&sm3_ctx);
    drv_sm3_update(&sm3_ctx, buf, 32);
    if(env->ctx.isInitiator) {
        drv_sm3_update(&sm3_ctx, env->ctx.z, 32);
        drv_sm3_update(&sm3_ctx, peerZ, 32);
        drv_sm3_update(&sm3_ctx, env->ctx.xy, 64);
        drv_sm3_update(&sm3_ctx, peer_rp, 64);
    }else {
        drv_sm3_update(&sm3_ctx, peerZ, 32);
        drv_sm3_update(&sm3_ctx, env->ctx.z, 32);
        drv_sm3_update(&sm3_ctx, peer_rp, 64);
        drv_sm3_update(&sm3_ctx, env->ctx.xy, 64);
    }
    drv_sm3_stop(&sm3_ctx, buf + 68);

    drv_sm3_start(&sm3_ctx);
    buf[31] = 0x02;
    drv_sm3_update(&sm3_ctx, buf + 31, 33);
    drv_sm3_update(&sm3_ctx, buf + 68, 32);

    // ouput s1 or sB
    drv_sm3_stop(&sm3_ctx, output + kLen);

    // Hash(0x03 || y_u || Hash(x_u || Z_A || Z_B || x_1 || y_1 || x_2 || y_2))
    drv_sm3_start(&sm3_ctx);
    buf[31] = 0x03;
    drv_sm3_update(&sm3_ctx, buf + 31, 33);
    drv_sm3_update(&sm3_ctx, buf + 68, 32);

    // ouput s2 or sA
    drv_sm3_stop(&sm3_ctx, output + kLen + 32);
}

/**
 *******************************************************************************
 * @brief Calculate SM2 shared secret
 *
 * @param[in] peer_public_key peer public key
 * @param[in] private_key private key
 * @param[out] shared_secret shared secret
 *
 * @return om_error
 *******************************************************************************
 */
om_error_t drv_sm2_shared_secret(const uint8_t peer_public_key[64], const uint8_t private_key[32], uint8_t shared_secret[64])
{
    sm2_z256_t private_key_tmp;
    SM2_Z256_POINT peer_public_key_tmp, res;

    if (peer_public_key == NULL || private_key == NULL || shared_secret == NULL) {
        return OM_ERROR_PARAMETER;
    }

    sm2_z256_from_bytes(private_key_tmp, private_key);
    sm2_z256_point_from_bytes(&peer_public_key_tmp, peer_public_key);
    sm2_z256_point_mul(&res,private_key_tmp, &peer_public_key_tmp);
    sm2_z256_point_to_bytes(&res, shared_secret);

    return OM_ERROR_OK;
}

#endif /* RTE_SM2 */

/** @} */