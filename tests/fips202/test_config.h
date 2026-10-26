#ifndef FIPS202_TEST_CONFIG_H
#define FIPS202_TEST_CONFIG_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define MLD_CONFIG_PARAMETER_SET 44
#define MLD_CONFIG_REDUCE_RAM
#define MLD_CONFIG_NO_RANDOMIZED_API
#define MLD_CONFIG_CUSTOM_ZEROIZE
void secure_wipe(void *ptr, size_t len);
#define mld_zeroize secure_wipe

#if defined(TKEY_FIPS202_HARDWARE)
#define MLD_CONFIG_FIPS202_CUSTOM_HEADER "fips202_hw.h"
#define MLD_CONFIG_SERIAL_FIPS202_ONLY
#endif

#endif
