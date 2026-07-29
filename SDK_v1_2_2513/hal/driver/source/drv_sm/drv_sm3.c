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
 * @brief    SM3 algorithm source file
 * @details  SM3 algorithm source file. Implemeted by software.
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
#if (RTE_SM3)
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include "om_driver.h"
#include "sm_endian.h"


/*******************************************************************************
 * MACROS
 */
#define P0(x) ((x) ^ ROL32((x), 9) ^ ROL32((x),17))
#define P1(x) ((x) ^ ROL32((x),15) ^ ROL32((x),23))

#define FF00(x,y,z)  ((x) ^ (y) ^ (z))
#define FF16(x,y,z)  (((x)&(y)) | ((x)&(z)) | ((y)&(z)))
#define GG00(x,y,z)  ((x) ^ (y) ^ (z))
#define GG16(x,y,z)  ((((y)^(z)) & (x)) ^ (z))

#define DRV_SM3_HMAC_DEFINE(SM3_HMAC_n, sm3_hmac_n)         \
static sm3_hmac_env_t sm3_hmac_n##_env = {                  \
    .sm3_hmac_id = (uint32_t)OM_##SM3_HMAC_n,               \
    .state = DRV_STATE_STOP,                                \
}


/*******************************************************************************
 * TYPEDEFS
 */


typedef struct {
    uint32_t    sm3_hmac_id;
    drv_state_t state;
    sm3_ctx_t   sm3_ctx;
    uint8_t     key[SM3_BLOCK_SIZE];
} sm3_hmac_env_t;


/*******************************************************************************
 * CONST & VARIABLES
 */
static uint32_t K[64] = {
	0x79cc4519U, 0xf3988a32U, 0xe7311465U, 0xce6228cbU,
	0x9cc45197U, 0x3988a32fU, 0x7311465eU, 0xe6228cbcU,
	0xcc451979U, 0x988a32f3U, 0x311465e7U, 0x6228cbceU,
	0xc451979cU, 0x88a32f39U, 0x11465e73U, 0x228cbce6U,
	0x9d8a7a87U, 0x3b14f50fU, 0x7629ea1eU, 0xec53d43cU,
	0xd8a7a879U, 0xb14f50f3U, 0x629ea1e7U, 0xc53d43ceU,
	0x8a7a879dU, 0x14f50f3bU, 0x29ea1e76U, 0x53d43cecU,
	0xa7a879d8U, 0x4f50f3b1U, 0x9ea1e762U, 0x3d43cec5U,
	0x7a879d8aU, 0xf50f3b14U, 0xea1e7629U, 0xd43cec53U,
	0xa879d8a7U, 0x50f3b14fU, 0xa1e7629eU, 0x43cec53dU,
	0x879d8a7aU, 0x0f3b14f5U, 0x1e7629eaU, 0x3cec53d4U,
	0x79d8a7a8U, 0xf3b14f50U, 0xe7629ea1U, 0xcec53d43U,
	0x9d8a7a87U, 0x3b14f50fU, 0x7629ea1eU, 0xec53d43cU,
	0xd8a7a879U, 0xb14f50f3U, 0x629ea1e7U, 0xc53d43ceU,
	0x8a7a879dU, 0x14f50f3bU, 0x29ea1e76U, 0x53d43cecU,
	0xa7a879d8U, 0x4f50f3b1U, 0x9ea1e762U, 0x3d43cec5U,
};

DRV_SM3_HMAC_DEFINE(SM3_HMAC_0, sm3_hmac_0);


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
static sm3_hmac_env_t *sm3_hmac_get_env(uint32_t om_sm3_hmac)
{
    if (sm3_hmac_0_env.sm3_hmac_id == om_sm3_hmac) {
        return &sm3_hmac_0_env;
    }

    OM_ASSERT(0);
    return NULL;
}


#if ENABLE_SMALL_FOOTPRINT
static void sm3_compress_blocks(uint32_t digest[8], const uint8_t *data, size_t blocks)
{
	uint32_t A;
	uint32_t B;
	uint32_t C;
	uint32_t D;
	uint32_t E;
	uint32_t F;
	uint32_t G;
	uint32_t H;
	uint32_t W[68];
	uint32_t SS1, SS2, TT1, TT2;
	int j;

	while (blocks--) {

		A = digest[0];
		B = digest[1];
		C = digest[2];
		D = digest[3];
		E = digest[4];
		F = digest[5];
		G = digest[6];
		H = digest[7];

		for (j = 0; j < 16; j++) {
			W[j] = GETU32(data + j*4);
		}

		for (; j < 68; j++) {
			W[j] = P1(W[j - 16] ^ W[j - 9] ^ ROL32(W[j - 3], 15))
				^ ROL32(W[j - 13], 7) ^ W[j - 6];
		}

		for (j = 0; j < 16; j++) {
			SS1 = ROL32((ROL32(A, 12) + E + K[j]), 7);
			SS2 = SS1 ^ ROL32(A, 12);
			TT1 = FF00(A, B, C) + D + SS2 + (W[j] ^ W[j + 4]);
			TT2 = GG00(E, F, G) + H + SS1 + W[j];
			D = C;
			C = ROL32(B, 9);
			B = A;
			A = TT1;
			H = G;
			G = ROL32(F, 19);
			F = E;
			E = P0(TT2);
		}

		for (; j < 64; j++) {
			SS1 = ROL32((ROL32(A, 12) + E + K[j]), 7);
			SS2 = SS1 ^ ROL32(A, 12);
			TT1 = FF16(A, B, C) + D + SS2 + (W[j] ^ W[j + 4]);
			TT2 = GG16(E, F, G) + H + SS1 + W[j];
			D = C;
			C = ROL32(B, 9);
			B = A;
			A = TT1;
			H = G;
			G = ROL32(F, 19);
			F = E;
			E = P0(TT2);
		}

		digest[0] ^= A;
		digest[1] ^= B;
		digest[2] ^= C;
		digest[3] ^= D;
		digest[4] ^= E;
		digest[5] ^= F;
		digest[6] ^= G;
		digest[7] ^= H;

		data += 64;
	}
}
#else

#define SM3_ROUND_0(j,A,B,C,D,E,F,G,H)			                               \
	SS0 = ROL32(A, 12);				                                           \
	SS1 = ROL32(SS0 + E + K[j], 7);			                                   \
	SS2 = SS1 ^ SS0;				                                           \
	D += FF00(A, B, C) + SS2 + (W[j] ^ W[j + 4]);	                           \
	SS1 += GG00(E, F, G) + H + W[j];		                                   \
	B = ROL32(B, 9);				                                           \
	H = P0(SS1);					                                           \
	F = ROL32(F, 19);				                                           \
	W[j+16] = P1(W[j] ^ W[j+7] ^ ROL32(W[j+13], 15)) ^ ROL32(W[j+3], 7) ^ W[j+10];

#define SM3_ROUND_1(j,A,B,C,D,E,F,G,H)			                               \
	SS0 = ROL32(A, 12);				                                           \
	SS1 = ROL32(SS0 + E + K[j], 7);			                                   \
	SS2 = SS1 ^ SS0;				                                           \
	D += FF16(A, B, C) + SS2 + (W[j] ^ W[j + 4]);	                           \
	SS1 += GG16(E, F, G) + H + W[j];		                                   \
	B = ROL32(B, 9);					                                       \
	H = P0(SS1);					                                           \
	F = ROL32(F, 19);				                                           \
	W[j+16] = P1(W[j] ^ W[j+7] ^ ROL32(W[j+13], 15)) ^ ROL32(W[j+3], 7) ^ W[j+10];


#define SM3_ROUND_2(j,A,B,C,D,E,F,G,H)			                               \
	SS0 = ROL32(A, 12);				                                           \
	SS1 = ROL32(SS0 + E + K[j], 7);			                                   \
	SS2 = SS1 ^ SS0;				                                           \
	D += FF16(A, B, C) + SS2 + (W[j] ^ W[j + 4]);	                           \
	SS1 += GG16(E, F, G) + H + W[j];		                                   \
	B = ROL32(B, 9);				                                           \
	H = P0(SS1);					                                           \
	F = ROL32(F, 19);

static void sm3_compress_blocks(uint32_t digest[8], const uint8_t *data, size_t blocks)
{
	uint32_t A;
	uint32_t B;
	uint32_t C;
	uint32_t D;
	uint32_t E;
	uint32_t F;
	uint32_t G;
	uint32_t H;
	uint32_t W[68];
	uint32_t SS0, SS1, SS2;
	int j;

	while (blocks--) {

		A = digest[0];
		B = digest[1];
		C = digest[2];
		D = digest[3];
		E = digest[4];
		F = digest[5];
		G = digest[6];
		H = digest[7];

		for (j = 0; j < 16; j++) {
			W[j] = GETU32(data + j*4);
		}

		SM3_ROUND_0( 0, A,B,C,D, E,F,G,H);
		SM3_ROUND_0( 1, D,A,B,C, H,E,F,G);
		SM3_ROUND_0( 2, C,D,A,B, G,H,E,F);
		SM3_ROUND_0( 3, B,C,D,A, F,G,H,E);
		SM3_ROUND_0( 4, A,B,C,D, E,F,G,H);
		SM3_ROUND_0( 5, D,A,B,C, H,E,F,G);
		SM3_ROUND_0( 6, C,D,A,B, G,H,E,F);
		SM3_ROUND_0( 7, B,C,D,A, F,G,H,E);
		SM3_ROUND_0( 8, A,B,C,D, E,F,G,H);
		SM3_ROUND_0( 9, D,A,B,C, H,E,F,G);
		SM3_ROUND_0(10, C,D,A,B, G,H,E,F);
		SM3_ROUND_0(11, B,C,D,A, F,G,H,E);
		SM3_ROUND_0(12, A,B,C,D, E,F,G,H);
		SM3_ROUND_0(13, D,A,B,C, H,E,F,G);
		SM3_ROUND_0(14, C,D,A,B, G,H,E,F);
		SM3_ROUND_0(15, B,C,D,A, F,G,H,E);
		SM3_ROUND_1(16, A,B,C,D, E,F,G,H);
		SM3_ROUND_1(17, D,A,B,C, H,E,F,G);
		SM3_ROUND_1(18, C,D,A,B, G,H,E,F);
		SM3_ROUND_1(19, B,C,D,A, F,G,H,E);
		SM3_ROUND_1(20, A,B,C,D, E,F,G,H);
		SM3_ROUND_1(21, D,A,B,C, H,E,F,G);
		SM3_ROUND_1(22, C,D,A,B, G,H,E,F);
		SM3_ROUND_1(23, B,C,D,A, F,G,H,E);
		SM3_ROUND_1(24, A,B,C,D, E,F,G,H);
		SM3_ROUND_1(25, D,A,B,C, H,E,F,G);
		SM3_ROUND_1(26, C,D,A,B, G,H,E,F);
		SM3_ROUND_1(27, B,C,D,A, F,G,H,E);
		SM3_ROUND_1(28, A,B,C,D, E,F,G,H);
		SM3_ROUND_1(29, D,A,B,C, H,E,F,G);
		SM3_ROUND_1(30, C,D,A,B, G,H,E,F);
		SM3_ROUND_1(31, B,C,D,A, F,G,H,E);
		SM3_ROUND_1(32, A,B,C,D, E,F,G,H);
		SM3_ROUND_1(33, D,A,B,C, H,E,F,G);
		SM3_ROUND_1(34, C,D,A,B, G,H,E,F);
		SM3_ROUND_1(35, B,C,D,A, F,G,H,E);
		SM3_ROUND_1(36, A,B,C,D, E,F,G,H);
		SM3_ROUND_1(37, D,A,B,C, H,E,F,G);
		SM3_ROUND_1(38, C,D,A,B, G,H,E,F);
		SM3_ROUND_1(39, B,C,D,A, F,G,H,E);
		SM3_ROUND_1(40, A,B,C,D, E,F,G,H);
		SM3_ROUND_1(41, D,A,B,C, H,E,F,G);
		SM3_ROUND_1(42, C,D,A,B, G,H,E,F);
		SM3_ROUND_1(43, B,C,D,A, F,G,H,E);
		SM3_ROUND_1(44, A,B,C,D, E,F,G,H);
		SM3_ROUND_1(45, D,A,B,C, H,E,F,G);
		SM3_ROUND_1(46, C,D,A,B, G,H,E,F);
		SM3_ROUND_1(47, B,C,D,A, F,G,H,E);
		SM3_ROUND_1(48, A,B,C,D, E,F,G,H);
		SM3_ROUND_1(49, D,A,B,C, H,E,F,G);
		SM3_ROUND_1(50, C,D,A,B, G,H,E,F);
		SM3_ROUND_1(51, B,C,D,A, F,G,H,E);
		SM3_ROUND_2(52, A,B,C,D, E,F,G,H);
		SM3_ROUND_2(53, D,A,B,C, H,E,F,G);
		SM3_ROUND_2(54, C,D,A,B, G,H,E,F);
		SM3_ROUND_2(55, B,C,D,A, F,G,H,E);
		SM3_ROUND_2(56, A,B,C,D, E,F,G,H);
		SM3_ROUND_2(57, D,A,B,C, H,E,F,G);
		SM3_ROUND_2(58, C,D,A,B, G,H,E,F);
		SM3_ROUND_2(59, B,C,D,A, F,G,H,E);
		SM3_ROUND_2(60, A,B,C,D, E,F,G,H);
		SM3_ROUND_2(61, D,A,B,C, H,E,F,G);
		SM3_ROUND_2(62, C,D,A,B, G,H,E,F);
		SM3_ROUND_2(63, B,C,D,A, F,G,H,E);

		digest[0] ^= A;
		digest[1] ^= B;
		digest[2] ^= C;
		digest[3] ^= D;
		digest[4] ^= E;
		digest[5] ^= F;
		digest[6] ^= G;
		digest[7] ^= H;

		data += 64;
	}
}
#endif



/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
void drv_sm3_start(sm3_ctx_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
	ctx->digest[0] = 0x7380166F;
	ctx->digest[1] = 0x4914B2B9;
	ctx->digest[2] = 0x172442D7;
	ctx->digest[3] = 0xDA8A0600;
	ctx->digest[4] = 0xA96F30BC;
	ctx->digest[5] = 0x163138AA;
	ctx->digest[6] = 0xE38DEE4D;
	ctx->digest[7] = 0xB0FB0E4E;
}

void drv_sm3_update(sm3_ctx_t *ctx, const uint8_t *data, uint32_t data_len)
{
    uint32_t blocks;

	ctx->num &= 0x3f;
	if (ctx->num) {
		uint32_t left = SM3_BLOCK_SIZE - ctx->num;
		if (data_len < left) {
			memcpy(ctx->block + ctx->num, data, data_len);
			ctx->num += data_len;
			return;
		} else {
			memcpy(ctx->block + ctx->num, data, left);
			sm3_compress_blocks(ctx->digest, ctx->block, 1);
			ctx->nblocks++;
			data += left;
			data_len -= left;
		}
	}

	blocks = data_len / SM3_BLOCK_SIZE;
	if (blocks) {
		sm3_compress_blocks(ctx->digest, data, blocks);
		ctx->nblocks += blocks;
		data += SM3_BLOCK_SIZE * blocks;
		data_len -= SM3_BLOCK_SIZE * blocks;
	}

	ctx->num = data_len;
	if (data_len) {
		memcpy(ctx->block, data, data_len);
	}
}

void drv_sm3_stop(sm3_ctx_t *ctx, uint8_t digest[SM3_DIGEST_SIZE])
{
	ctx->num &= 0x3f;
	ctx->block[ctx->num] = 0x80;

	if (ctx->num <= SM3_BLOCK_SIZE - 9) {
		memset(ctx->block + ctx->num + 1, 0, SM3_BLOCK_SIZE - ctx->num - 9);
	} else {
		memset(ctx->block + ctx->num + 1, 0, SM3_BLOCK_SIZE - ctx->num - 1);
		sm3_compress_blocks(ctx->digest, ctx->block, 1);
		memset(ctx->block, 0, SM3_BLOCK_SIZE - 8);
	}

	PUTU32(ctx->block + 56, ctx->nblocks >> 23);
	PUTU32(ctx->block + 60, (ctx->nblocks << 9) + (ctx->num << 3));
	sm3_compress_blocks(ctx->digest, ctx->block, 1);

	for (uint8_t i = 0; i < 8; i++) {
		PUTU32(digest + i*4, ctx->digest[i]);
	}
}

/*************************************HMAC*************************************/
/**
 * HMAC_k(m) = H((k ^ opad) || H((k ^ ipad) || m))
 * pseudo-code:
 * function hmac(key, message)
 *	opad = [0x5c * blocksize]
 *	ipad = [0x36 * blocksize]
 *	if (length(key) > blocksize) then
 *		key = hash(key)
 *	end if
 *	for i from 0 to length(key) - 1 step 1
 *		ipad[i] = ipad[i] XOR key[i]
 *		opad[i] = opad[i] XOR key[i]
 *	end for
 *	return hash(opad || hash(ipad || message))
 * end function
 */

#define IPAD	            0x36
#define OPAD	            0x5C

om_error_t drv_sm3_hmac_start(uint32_t om_sm3_hmac, const uint8_t *key, uint32_t key_len)
{
    sm3_hmac_env_t *env;

    env = sm3_hmac_get_env(om_sm3_hmac);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    OM_ASSERT(env->state == DRV_STATE_STOP);

	if (key_len <= SM3_BLOCK_SIZE) {
		memcpy(env->key, key, key_len);
		memset(env->key + key_len, 0, SM3_BLOCK_SIZE - key_len);
	} else {
		drv_sm3_start(&env->sm3_ctx);
		drv_sm3_update(&env->sm3_ctx, key, key_len);
		drv_sm3_stop(&env->sm3_ctx, env->key);
		memset(env->key + SM3_DIGEST_SIZE, 0, SM3_BLOCK_SIZE-SM3_DIGEST_SIZE);
	}
	for (uint8_t i = 0; i < SM3_BLOCK_SIZE; i++) {
		env->key[i] ^= IPAD;
	}

	drv_sm3_start(&env->sm3_ctx);
	drv_sm3_update(&env->sm3_ctx, env->key, SM3_BLOCK_SIZE);

    env->state = DRV_STATE_START;

    return OM_ERROR_OK;
}

om_error_t drv_sm3_hmac_update(uint32_t om_sm3_hmac, const uint8_t *data, uint32_t data_len)
{
    sm3_hmac_env_t *env;

    env = sm3_hmac_get_env(om_sm3_hmac);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    OM_ASSERT(env->state == DRV_STATE_START || env->state == DRV_STATE_CONTINUE);

	drv_sm3_update(&env->sm3_ctx, data, data_len);

    env->state = DRV_STATE_CONTINUE;

    return OM_ERROR_OK;
}

om_error_t drv_sm3_hmac_stop(uint32_t om_sm3_hmac, uint8_t mac[SM3_DIGEST_SIZE])
{
    sm3_hmac_env_t *env;

    env = sm3_hmac_get_env(om_sm3_hmac);
    if (env == NULL) {
        return OM_ERROR_PARAMETER;
    }

    OM_ASSERT(env->state == DRV_STATE_CONTINUE);

	for (uint8_t i = 0; i < SM3_BLOCK_SIZE; i++) {
		env->key[i] ^= (IPAD ^ OPAD);
	}
	drv_sm3_stop(&env->sm3_ctx, mac);
	drv_sm3_start(&env->sm3_ctx);
	drv_sm3_update(&env->sm3_ctx, env->key, SM3_BLOCK_SIZE);
	drv_sm3_update(&env->sm3_ctx, mac, SM3_DIGEST_SIZE);
	drv_sm3_stop(&env->sm3_ctx, mac);

    memset(&env->sm3_ctx, 0x0, sizeof(sm3_hmac_env_t) - offsetof(sm3_hmac_env_t, sm3_ctx));
    env->state = DRV_STATE_STOP;

    return OM_ERROR_OK;
}

#endif

/** @} */