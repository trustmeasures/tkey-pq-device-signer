#ifndef TKEY_FIPS202_HW_H
#define TKEY_FIPS202_HW_H

#include <driver/hw_sha3.h>
#include <tkey/assert.h>

#define SHAKE128_RATE SHAKE_128_RATE_BYTES
#define SHAKE256_RATE SHAKE_256_RATE_BYTES
#define SHA3_256_RATE SHAKE_256_RATE_BYTES
#define SHA3_512_RATE SHA3_512_RATE_BYTES
#define SHA3_256_HASHBYTES SHA3_256_DIGEST_BYTES
#define SHA3_512_HASHBYTES SHA3_512_DIGEST_BYTES

typedef sha3_ctx_t mld_shake128ctx;
typedef sha3_ctx_t mld_shake256ctx;

#define mld_shake128_init MLD_NAMESPACE(shake128_init)
#define mld_shake128_absorb MLD_NAMESPACE(shake128_absorb)
#define mld_shake128_finalize MLD_NAMESPACE(shake128_finalize)
#define mld_shake128_squeeze MLD_NAMESPACE(shake128_squeeze)
#define mld_shake128_release MLD_NAMESPACE(shake128_release)
#define mld_shake256_init MLD_NAMESPACE(shake256_init)
#define mld_shake256_absorb MLD_NAMESPACE(shake256_absorb)
#define mld_shake256_finalize MLD_NAMESPACE(shake256_finalize)
#define mld_shake256_squeeze MLD_NAMESPACE(shake256_squeeze)
#define mld_shake256_release MLD_NAMESPACE(shake256_release)
#define mld_shake256 MLD_NAMESPACE(shake256)

static inline void mld_shake128_init(mld_shake128ctx *state)
{
	assert(sha3_stream_init(state, SHAKE_128) == SHA3_OK);
}

static inline void mld_shake128_absorb(mld_shake128ctx *state,
				       const uint8_t *in, size_t inlen)
{
	assert(sha3_stream_absorb(state, in, inlen) == SHA3_OK);
}

static inline void mld_shake128_finalize(mld_shake128ctx *state)
{
	assert(sha3_stream_finalize(state) == SHA3_OK);
}

static inline void mld_shake128_squeeze(uint8_t *out, size_t outlen,
					mld_shake128ctx *state)
{
	assert(sha3_stream_squeeze(state, out, outlen) == SHA3_OK);
}

static inline void mld_shake128_release(mld_shake128ctx *state)
{
	assert(sha3_stream_release(state) == SHA3_OK);
}

static inline void mld_shake256_init(mld_shake256ctx *state)
{
	assert(sha3_stream_init(state, SHAKE_256) == SHA3_OK);
}

static inline void mld_shake256_absorb(mld_shake256ctx *state,
				       const uint8_t *in, size_t inlen)
{
	assert(sha3_stream_absorb(state, in, inlen) == SHA3_OK);
}

static inline void mld_shake256_finalize(mld_shake256ctx *state)
{
	assert(sha3_stream_finalize(state) == SHA3_OK);
}

static inline void mld_shake256_squeeze(uint8_t *out, size_t outlen,
					mld_shake256ctx *state)
{
	assert(sha3_stream_squeeze(state, out, outlen) == SHA3_OK);
}

static inline void mld_shake256_release(mld_shake256ctx *state)
{
	assert(sha3_stream_release(state) == SHA3_OK);
}

static inline void mld_shake256(uint8_t *out, size_t outlen, const uint8_t *in,
				size_t inlen)
{
	assert(sha3_hash_mode(in, inlen, out, outlen, SHAKE_256) == SHA3_OK);
}

#endif
