#include "sha3_test_io.h"
#include <driver/hw_sha3.h>
#include <stdio.h>

#define sha3_ctx_t reference_ctx_t
#include "sha3.h"
#undef sha3_ctx_t

static void reference(sha3_mode_t mode, const uint8_t *in, size_t len,
		      uint8_t *out, size_t outlen)
{
	reference_ctx_t ctx;
	int mdlen = mode == SHAKE_128 ? 16 : mode == SHA3_512 ? 64 : 32;
	assert(sha3_init(&ctx, mdlen) == 1);
	assert(sha3_update(&ctx, in, len) == 1);
	if (mode == SHAKE_128 || mode == SHAKE_256) {
		shake_xof(&ctx);
		shake_out(&ctx, out, outlen);
	} else {
		assert(sha3_final(out, &ctx) == 1);
	}
}

static void check_hex(const uint8_t *bytes, const char *hex)
{
	for (size_t i = 0; hex[i * 2] != '\0'; i++) {
		unsigned int expected;
		assert(sscanf(hex + i * 2, "%2x", &expected) == 1);
		assert(bytes[i] == expected);
	}
}

int main(int argc, char **argv)
{
	uint8_t input[600], actual[600], expected[600];
	sha3_ctx_t ctx, other;
	for (size_t i = 0; i < sizeof(input); i++) {
		input[i] = (uint8_t)(i * 37 + 11);
	}
	if (argc == 2) {
		sha3_test_trace(argv[1]);
	}
	size_t cases = 0;
	for (int m = SHA3_256; m <= SHAKE_256; m++) {
		sha3_mode_t mode = m;
		size_t rate = mode == SHAKE_128	 ? 168
			      : mode == SHA3_512 ? 72
						 : 136;
		size_t lengths[] = {0,
				    1,
				    3,
				    rate - 1,
				    rate,
				    rate + 1,
				    2 * rate - 1,
				    2 * rate,
				    2 * rate + 1};
		size_t outputs[] = {0,	  1,	    3,	      rate - 1,
				    rate, rate + 1, 2 * rate, 2 * rate + 7};
		for (size_t l = 0; l < sizeof(lengths) / sizeof(lengths[0]);
		     l++) {
			for (size_t o = 0;
			     o < sizeof(outputs) / sizeof(outputs[0]); o++) {
				size_t outlen = mode == SHA3_256   ? 32
						: mode == SHA3_512 ? 64
								   : outputs[o];
				reference(mode, input, lengths[l], expected,
					  outlen);
				assert(sha3_hash_mode(input, lengths[l], actual,
						      outlen, mode) == SHA3_OK);
				assert(memcmp(actual, expected, outlen) == 0);
				assert(sha3_stream_init(&ctx, mode) == SHA3_OK);
				assert(sha3_stream_absorb(&ctx, NULL, 0) ==
				       SHA3_OK);
				assert(sha3_stream_squeeze(&ctx, actual, 0) ==
				       SHA3_ERR_STATE);
				for (size_t p = 0; p < lengths[l];) {
					size_t n = (p % 23) + 1;
					if (n > lengths[l] - p) {
						n = lengths[l] - p;
					}
					assert(
					    sha3_stream_absorb(&ctx, input + p,
							       n) == SHA3_OK);
					p += n;
				}
				assert(sha3_stream_finalize(&ctx) == SHA3_OK);
				assert(sha3_stream_finalize(&ctx) ==
				       SHA3_ERR_STATE);
				assert(sha3_stream_absorb(&ctx, input, 1) ==
				       SHA3_ERR_STATE);
				assert(sha3_stream_squeeze(&ctx, NULL, 0) ==
				       SHA3_OK);
				for (size_t p = 0; p < outlen;) {
					size_t n = (p % 19) + 1;
					if (n > outlen - p) {
						n = outlen - p;
					}
					assert(sha3_stream_squeeze(
						   &ctx, actual + p, n) ==
					       SHA3_OK);
					p += n;
				}
				assert(memcmp(actual, expected, outlen) == 0);
				if (mode == SHA3_256 || mode == SHA3_512) {
					assert(sha3_stream_squeeze(&ctx, actual,
								   1) ==
					       SHA3_ERR_ARGUMENT);
				}
				assert(sha3_stream_release(&ctx) == SHA3_OK);
				uint64_t cleared[25] = {0};
				sha3_keccakf(cleared);
				const uint8_t *public_state =
				    (const uint8_t *)cleared;
				for (size_t i = 0; i < 50; i++) {
					uint32_t word =
					    ((uint32_t)public_state[i * 4]
					     << 24) |
					    ((uint32_t)public_state[i * 4 + 1]
					     << 16) |
					    ((uint32_t)public_state[i * 4 + 2]
					     << 8) |
					    public_state[i * 4 + 3];
					assert(sha3_test_read(
						   TK1_THSM_SHA3_DIGEST0 +
						   i * 4) == word);
				}
				for (size_t i = 0; i < sizeof(ctx); i++) {
					assert(((uint8_t *)&ctx)[i] == 0);
				}
				cases++;
			}
		}
	}
	assert(sha3_hash_mode(NULL, 0, actual, 32, SHA3_256) == SHA3_OK);
	check_hex(
	    actual,
	    "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a");
	assert(sha3_hash_mode(NULL, 0, actual, 64, SHA3_512) == SHA3_OK);
	check_hex(
	    actual,
	    "a69f73cca23a9ac5c8b567dc185a756e97c982164fe25859e0d1dcc1475c80a61"
	    "5b2123af1f5f94c11e3e9402c3ac558f500199d95b6d3e301758586281dcd26");
	assert(sha3_hash_mode(NULL, 0, actual, 32, SHAKE_128) == SHA3_OK);
	check_hex(
	    actual,
	    "7f9c2ba4e88f827d616045507605853ed73b8093f6efbc88eb1a6eacfa66ef26");
	assert(sha3_hash_mode(NULL, 0, actual, 32, SHAKE_256) == SHA3_OK);
	check_hex(
	    actual,
	    "46b9dd2b0ba88d13233b3feb743eeb243fcd52ea62b81b82b50c27646ed5762f");
	sha3_test_trace(NULL);

	assert(sha3_stream_init(NULL, SHAKE_128) == SHA3_ERR_ARGUMENT);
	assert(sha3_stream_init(&ctx, (sha3_mode_t)99) == SHA3_ERR_ARGUMENT);
	assert(sha3_stream_init(&ctx, SHAKE_128) == SHA3_OK);
	assert(sha3_stream_init(&ctx, SHAKE_256) == SHA3_ERR_BUSY);
	assert(sha3_stream_init(&other, SHAKE_128) == SHA3_ERR_BUSY);
	assert(sha3_hash_mode(input, 1, actual, 32, SHAKE_256) ==
	       SHA3_ERR_BUSY);
	assert(sha3_stream_release(&other) == SHA3_ERR_STATE);
	assert(sha3_stream_absorb(&ctx, NULL, 1) == SHA3_ERR_ARGUMENT);
	assert(sha3_stream_release(&ctx) == SHA3_OK);
	assert(sha3_stream_release(&ctx) == SHA3_ERR_STATE);
	assert(sha3_hash_mode(input, 1, actual, 31, SHA3_256) ==
	       SHA3_ERR_ARGUMENT);
	assert(sha3_stream_init(&ctx, SHAKE_256) == SHA3_OK);
	assert(sha3_stream_finalize(&ctx) == SHA3_OK);
	assert(sha3_stream_squeeze(&ctx, NULL, 1) == SHA3_ERR_ARGUMENT);
	sha3_test_stall(1);
	assert(sha3_stream_squeeze(&ctx, actual, 136) == SHA3_OK);
	assert(sha3_stream_squeeze(&ctx, actual, 1) == SHA3_ERR_TIMEOUT);
	assert(sha3_stream_release(&ctx) == SHA3_ERR_TIMEOUT);
	sha3_test_stall(0);
	assert(sha3_stream_init(&ctx, SHAKE_128) == SHA3_ERR_TIMEOUT);
	printf("PASS: %zu differential cases, four empty-input KATs, "
	       "lifecycle/ownership/timeout\n",
	       cases);
	return 0;
}
