#include "sha3_test_io.h"
#include <driver/hw_sha3.h>
#include <stdio.h>
#include <stdlib.h>

#define sha3_ctx_t reference_ctx_t
#include "sha3.h"
#undef sha3_ctx_t

static union {
	uint64_t lanes[25];
	uint8_t bytes[200];
} state;
static uint8_t block[200];
static sha3_mode_t mode;
static int stalled;
static int accepting;
static FILE *trace;

void secure_wipe(void *ptr, size_t len)
{
	volatile uint8_t *p = ptr;
	while (len-- != 0) {
		*p++ = 0;
	}
}

void sha3_test_trace(const char *path)
{
	if (trace != NULL) {
		assert(fclose(trace) == 0);
		trace = NULL;
	}
	if (path != NULL) {
		trace = fopen(path, "w");
		assert(trace != NULL);
	}
}

void sha3_test_stall(int stall)
{
	stalled = stall;
}

uint32_t sha3_test_read(uint32_t address)
{
	if (address == TK1_THSM_SHA3_STATUS) {
		if (accepting && !stalled) {
			accepting = 0;
			return 0;
		}
		return stalled ? 0 : 3;
	}
	if (address == TK1_THSM_SHA3_CTRL) {
		return 0;
	}
	assert(address >= TK1_THSM_SHA3_DIGEST0 &&
	       address < TK1_THSM_SHA3_DIGEST0 + 200);
	size_t n = address - TK1_THSM_SHA3_DIGEST0;
	uint32_t value = ((uint32_t)state.bytes[n] << 24) |
			 ((uint32_t)state.bytes[n + 1] << 16) |
			 ((uint32_t)state.bytes[n + 2] << 8) |
			 state.bytes[n + 3];
	if (trace != NULL) {
		assert(fprintf(trace, "0 %02x %08x\n",
			       (address - TK1_THSM_SHA3_BASE) / 4, value) > 0);
	}
	return value;
}

void sha3_test_write(uint32_t address, uint32_t value)
{
	if (trace != NULL) {
		assert(fprintf(trace, "1 %02x %08x\n",
			       (address - TK1_THSM_SHA3_BASE) / 4, value) > 0);
	}
	if (address == TK1_THSM_SHA3_MODE) {
		assert(value <= SHAKE_256);
		mode = value;
		return;
	}
	if (address == TK1_THSM_SHA3_CTRL) {
		assert(value == 1 || value == 2 || value == 4);
		accepting = 1;
		if (value == 1) {
			memset(&state, 0, sizeof(state));
		}
		if (value != 4) {
			size_t rate = mode == SHAKE_128	 ? 168
				      : mode == SHA3_512 ? 72
							 : 136;
			for (size_t i = 0; i < rate; i++) {
				state.bytes[i] ^= block[i];
			}
		}
		sha3_keccakf(state.lanes);
		return;
	}
	assert(address >= TK1_THSM_SHA3_BLOCK0 &&
	       address < TK1_THSM_SHA3_BLOCK0 + 200);
	size_t n = address - TK1_THSM_SHA3_BLOCK0;
	for (size_t i = 0; i < 4; i++) {
		block[n + i] = (uint8_t)(value >> (24 - i * 8));
	}
}
