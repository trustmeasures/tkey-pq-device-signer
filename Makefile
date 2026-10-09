# Check for OS, if not macos assume linux
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	shasum = shasum -a 512
else
	shasum = sha512sum
endif

IMAGE ?= localhost/tkey-builder-local
PODMAN_TARGET ?= all

OBJCOPY ?= llvm-objcopy

P := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
LIBDIR ?= $(P)/../tillitis-key1/hw/application_fpga/tkey-libs
FIPS202_BACKEND ?= hardware
ifeq ($(filter $(FIPS202_BACKEND),hardware software),)
$(error FIPS202_BACKEND must be hardware or software)
endif
BUILD_DIR := .build/$(FIPS202_BACKEND)

CC = clang

INCLUDE_DIRS = $(LIBDIR)/include \
	$(P)/mldsa-native/mldsa \
	$(P)/signer

INCLUDES = $(addprefix -I , $(INCLUDE_DIRS))

# If you want libcommon's debug_puts() et cetera to output something
# on the QEMU debug port, use -DQEMU_DEBUG, or -DTKEY_DEBUG if you
# want it on the TKey HID debug endpoint
CFLAGS = -target riscv32-unknown-none-elf -march=rv32iczmmul -mabi=ilp32 -mcmodel=medany \
   -static -std=gnu99 -O2 -ffast-math -fno-common -fno-builtin-printf \
   -fno-builtin-putchar -nostdlib -mno-relax -flto -g \
   -Wall -Werror=implicit-function-declaration \
   -DMLD_CONFIG_FILE=\"mldsa_config.h\" \
   $(INCLUDES) -I $(LIBDIR) #-DQEMU_DEBUG

ifeq ($(FIPS202_BACKEND),hardware)
CFLAGS += -DTKEY_FIPS202_HARDWARE
endif
ifneq ($(TKEY_DEBUG),)
CFLAGS := $(CFLAGS) -DTKEY_DEBUG
endif

ifneq ($(TKEY_SIGNER_APP_NO_TOUCH),)
CFLAGS := $(CFLAGS) -DTKEY_SIGNER_APP_NO_TOUCH
endif

AS = clang
ASFLAGS = -target riscv32-unknown-none-elf -march=rv32iczmmul -mabi=ilp32 -mcmodel=medany -mno-relax

LDLIBS = -L $(LIBDIR) -lhw -lmonocypher -lcommon -lcrt0
LDFLAGS = -T $(LIBDIR)/app.lds


.PHONY: all
all: signer/app.bin check-signer-hash

# Create compile_commands.json for clangd and LSP
.PHONY: clangd
clangd: compile_commands.json
compile_commands.json:
	$(MAKE) clean
	bear -- make signer/app.bin

# Turn elf into bin for device
%.bin: %.elf
	$(OBJCOPY) --input-target=elf32-littleriscv --output-target=binary $^ $@
	chmod a-x $@

show-%-hash: %/app.bin
	@echo "Device app digest:"
	@$(shasum) $$(dirname $^)/app.bin

check-signer-hash: signer/app.bin show-signer-hash
	@echo "Expected device app digest: "
	@cat signer/app.bin.sha512
	$(shasum) -c signer/app.bin.sha512

CLANG_TIDY = clang-tidy

.PHONY: check
check:
	$(CLANG_TIDY) -header-filter=.* -checks=cert-* signer/*.[ch] -- $(CFLAGS)

# Simple mldsa signer app
SIGNERSRCS = signer/main.c signer/app_proto.c
SIGNEROBJS = $(addprefix $(BUILD_DIR)/,$(SIGNERSRCS:.c=.o)) $(BUILD_DIR)/signer/mldsa.o
LIBARCHIVES = $(addprefix $(LIBDIR)/,libhw.a libmonocypher.a libcommon.a libcrt0.a)

.PHONY: FORCE
FORCE:

$(BUILD_DIR)/config: FORCE
	@mkdir -p $(@D)
	@printf '%s\n' '$(CC)' '$(CFLAGS)' '$(LDFLAGS)' '$(LDLIBS)' > $@.tmp
	@cmp -s $@.tmp $@ || mv $@.tmp $@
	@rm -f $@.tmp

$(BUILD_DIR)/libs: FORCE
	$(MAKE) -C $(LIBDIR) all
	@mkdir -p $(@D)
	@$(shasum) $(LIBARCHIVES) > $@.tmp
	@cmp -s $@.tmp $@ || mv $@.tmp $@
	@rm -f $@.tmp

$(BUILD_DIR)/signer/app.elf: $(SIGNEROBJS) $(BUILD_DIR)/libs $(LIBDIR)/app.lds
	$(CC) $(CFLAGS) $(SIGNEROBJS) $(LDFLAGS) $(LDLIBS) -Wl,-Map,$@.map -o $@

signer/app.elf: $(BUILD_DIR)/signer/app.elf FORCE
	cp $< $@

signer/app.bin: $(BUILD_DIR)/signer/app.bin signer/app.elf FORCE
	cp $< $@

$(SIGNEROBJS): $(LIBDIR)/include/driver/hw_rng.h $(LIBDIR)/include/tkey/tk1_mem.h signer/app_proto.h
$(SIGNEROBJS): $(BUILD_DIR)/config

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/signer/mldsa.o: mldsa-native/mldsa/mldsa_native.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(SIGNEROBJS:.o=.d)

.PHONY: clean
clean:
	rm -f signer/app.bin signer/app.elf
	rm -rf .build/hardware .build/software

# Uses ../.clang-format
FMTFILES=signer/*.[ch]

.PHONY: fmt
fmt:
	clang-format --dry-run --ferror-limit=0 $(FMTFILES)
	clang-format --verbose -i $(FMTFILES)
.PHONY: checkfmt
checkfmt:
	clang-format --dry-run --ferror-limit=0 --Werror $(FMTFILES)

.PHONY: podman
podman:
	podman run --arch=amd64 --rm --mount type=bind,source=$(CURDIR),target=/src --mount type=bind,source=$(LIBDIR),target=/tkey-libs -w /src $(IMAGE) make -j LIBDIR=/tkey-libs FIPS202_BACKEND=$(FIPS202_BACKEND) $(if $(TKEY_DEBUG),TKEY_DEBUG=$(TKEY_DEBUG)) $(if $(TKEY_SIGNER_APP_NO_TOUCH),TKEY_SIGNER_APP_NO_TOUCH=$(TKEY_SIGNER_APP_NO_TOUCH)) $(PODMAN_TARGET)

.PHONY: test-hw-mldsa
test-hw-mldsa:
	bash tests/run_mldsa_hw_local.sh

.PHONY: test-fips202
test-fips202:
	podman run --arch=amd64 --rm --mount type=bind,source=$(CURDIR),target=/src --mount type=bind,source=$(LIBDIR),target=/tkey-libs --mount type=bind,source=$(LIBDIR)/../core/sha3,target=/sha3-core,ro -w /src $(IMAGE) make -C tests/fips202 LIBDIR=/tkey-libs CORE=/sha3-core

.PHONY: test-hw-mldsa-protocol-negative
test-hw-mldsa-protocol-negative:
	bash tests/run_mldsa_hw_protocol_negative.sh
