#ifndef SHA3_TEST_IO_H
#define SHA3_TEST_IO_H

#include <assert.h>
#include <stdint.h>
#include <string.h>

uint32_t sha3_test_read(uint32_t address);
void sha3_test_write(uint32_t address, uint32_t value);
void secure_wipe(void *ptr, size_t len);
void sha3_test_stall(int stall);
void sha3_test_trace(const char *path);

#endif
