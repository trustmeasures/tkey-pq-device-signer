LIBDIR ?= ../../../tillitis-key1/hw/application_fpga/tkey-libs
ROOT := ../..
CORE ?= $(LIBDIR)/../core/sha3
BUILD := $(ROOT)/.build/fips202-tests
REFERENCE := $(ROOT)/mldsa-native/examples/bring_your_own_fips202/custom_fips202/tiny_sha3
CC = cc
CFLAGS = -std=c99 -O1 -g -Wall -Wextra -Werror -MMD -MP -fsanitize=address,undefined \
	-I . -I $(LIBDIR)/include -I $(REFERENCE) -iquote $(ROOT)/signer
MLD_FLAGS = -I $(ROOT)/mldsa-native/mldsa \
	-DMLD_CONFIG_FILE=\"test_config.h\"

.PHONY: all
all: $(BUILD)/driver-test $(BUILD)/mldsa-test $(BUILD)/replay
	$(BUILD)/driver-test $(BUILD)/trace.txt
	$(BUILD)/mldsa-test
	vvp $(BUILD)/replay +trace=$(BUILD)/trace.txt

$(BUILD):
	mkdir -p $@

$(BUILD)/reference.o: $(REFERENCE)/sha3.c Makefile | $(BUILD)
	$(CC) $(CFLAGS) -Dsha3_ctx_t=reference_ctx_t -c $< -o $@

$(BUILD)/driver.o: $(LIBDIR)/libhw/hw_sha3.c $(LIBDIR)/include/driver/hw_sha3.h sha3_test_io.h Makefile | $(BUILD)
	$(CC) $(CFLAGS) -DSHA3_HW_TEST -c $< -o $@

$(BUILD)/model.o: model.c sha3_test_io.h Makefile | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/driver-test: driver_test.c $(BUILD)/model.o $(BUILD)/driver.o $(BUILD)/reference.o
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD)/hw.o: $(ROOT)/mldsa-native/mldsa/mldsa_native.c test_config.h $(ROOT)/signer/fips202_hw.h Makefile | $(BUILD)
	$(CC) $(CFLAGS) $(MLD_FLAGS) -DTKEY_FIPS202_HARDWARE -DMLD_CONFIG_NAMESPACE_PREFIX=hw -c $< -o $@

$(BUILD)/sw.o: $(ROOT)/mldsa-native/mldsa/mldsa_native.c test_config.h Makefile | $(BUILD)
	$(CC) $(CFLAGS) $(MLD_FLAGS) -DMLD_CONFIG_NAMESPACE_PREFIX=sw -c $< -o $@

$(BUILD)/mldsa-test: mldsa_test.c $(BUILD)/hw.o $(BUILD)/sw.o $(BUILD)/model.o $(BUILD)/driver.o $(BUILD)/reference.o
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD)/replay: replay.v $(CORE)/rtl/sha3.v $(CORE)/rtl/sha3_core.v | $(BUILD)
	iverilog -g2012 -s replay -o $@ $^

-include $(wildcard $(BUILD)/*.d)
