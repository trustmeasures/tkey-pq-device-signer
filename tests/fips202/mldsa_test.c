#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define DECLARE(prefix)                                                        \
	int prefix##_keypair_internal(uint8_t *, uint8_t *, const uint8_t *);  \
	int prefix##_signature_internal(                                       \
	    uint8_t *, size_t *, const uint8_t *, size_t, const uint8_t *,     \
	    size_t, const uint8_t *, const uint8_t *, int);                    \
	int prefix##_verify_internal(const uint8_t *, size_t, const uint8_t *, \
				     size_t, const uint8_t *, size_t,          \
				     const uint8_t *, int)
DECLARE(hw);
DECLARE(sw);

int main(void)
{
	uint8_t seed[32], rnd[32], msg[400];
	uint8_t hpk[1312], spk[1312], hsk[2560], ssk[2560];
	uint8_t hsig[2420], ssig[2420];
	for (size_t i = 0; i < sizeof(msg); i++) {
		msg[i] = (uint8_t)i;
	}
	for (size_t run = 0; run < 3; run++) {
		memset(seed, (int)run, sizeof(seed));
		memset(rnd, (int)(run + 17), sizeof(rnd));
		assert(hw_keypair_internal(hpk, hsk, seed) == 0);
		assert(sw_keypair_internal(spk, ssk, seed) == 0);
		assert(memcmp(hpk, spk, sizeof(hpk)) == 0);
		assert(memcmp(hsk, ssk, sizeof(hsk)) == 0);
		size_t lengths[] = {0, 135, 136, 137, 400, 64};
		for (size_t i = 0; i < sizeof(lengths) / sizeof(lengths[0]);
		     i++) {
			size_t hn = 0, sn = 0;
			int externalmu = i == 5;
			assert(hw_signature_internal(hsig, &hn, msg, lengths[i],
						     NULL, 0, rnd, hsk,
						     externalmu) == 0);
			assert(sw_signature_internal(ssig, &sn, msg, lengths[i],
						     NULL, 0, rnd, ssk,
						     externalmu) == 0);
			assert(hn == sizeof(hsig) && sn == hn);
			assert(memcmp(hsig, ssig, hn) == 0);
			assert(sw_verify_internal(hsig, hn, msg, lengths[i],
						  NULL, 0, spk,
						  externalmu) == 0);
			assert(hw_verify_internal(ssig, sn, msg, lengths[i],
						  NULL, 0, hpk,
						  externalmu) == 0);
			hsig[100] ^= 1;
			assert(hw_verify_internal(hsig, hn, msg, lengths[i],
						  NULL, 0, hpk,
						  externalmu) != 0);
			if (lengths[i] != 0) {
				msg[0] ^= 1;
				assert(hw_verify_internal(
					   ssig, sn, msg, lengths[i], NULL, 0,
					   hpk, externalmu) != 0);
				msg[0] ^= 1;
			}
		}
	}
	puts("PASS: ML-DSA-44 fixed-seed keys, 18 identical signatures, "
	     "cross-verification and tampering");
	return 0;
}
