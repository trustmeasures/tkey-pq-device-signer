# Hardware ML-DSA Local Tests

## FIPS202 driver and simulator validation

Run `make test-fips202` from the repository root. Compilation and execution use
`localhost/tkey-builder-local`, matching tillitis-key1's local Podman builder.
The image must provide GCC with ASan/UBSan and Icarus Verilog. Override `IMAGE`
when using an equivalent builder image.

This target does not access or flash a physical device. It checks:

- SHA3-256/512 and SHAKE128/256 empty-input known-answer vectors.
- 288 differential cases against the vendored tiny_sha3 reference, including
  rate boundaries, multi-block input/output, incremental absorption and partial
  squeezing, and zero-length SHAKE output.
- Lifecycle, exclusive ownership, software wiping and timeout lockout.
- Three fixed-seed ML-DSA-44 keypairs and 18 fixed-randomness signatures against
  the original software backend, including external-MU signing,
  cross-verification and tampered signatures/messages.
- Driver-generated MMIO transaction replay against the actual SHA3 wrapper/core
  RTL, including checks of all 25 public state lanes after release.

Host tests use a software MMIO model; only the subsequent RTL replay validates
the real core logic. Neither is an on-device execution or timing measurement.
Generated binaries, transaction traces and link maps are under `.build/`.

For timing comparisons, build both backends with `TKEY_DEBUG=1` and use the
existing device-side signing timer, excluding touch and transport latency.
Obtain deployment approval first, then use the hardware harness below. Record
key-generation/signing time, app size and RAM/stack measurements for both builds.
ELF BSS alone does not measure peak RAM: ML-DSA contexts also occupy the stack.
ASan differential checks do not replace NIST ACVP conformance testing.

Signer key/signature randomness is provided directly by `hw_rng`'s
`rng_get_word()` API. The signer no longer has its own `rng.c` DRBG or the
SHA-256 implementation previously used by that DRBG. `rng_get_word()` reads
the RNG core's conditioned output buffer, not the raw oscillator entropy
registers; applications must not use raw TRNG entropy words as random output.

This folder contains a local hardware validation harness for the TKey ML-DSA signer.

The goal is to verify end-to-end behavior on real hardware:

- Public key retrieval from device
- Sign and verify across boundary message sizes
- Randomized sign and verify runs
- Negative verification checks (tampered message and tampered signature)
- Long-running stress loops

## Prerequisites

1. Build the signer app in this repository.
2. Build the host CLI in the neighboring repository:

   cd ../tkey-sign-cli-pq
   make

3. Connect a TKey device.

Depending on firmware build flags, touch confirmation may be required for every signature operation.

## Run

From this repository root:

make test-hw-mldsa

Optional direct invocation:

bash tests/run_mldsa_hw_local.sh --port /dev/ttyACM0 --speed 62500

Result artifact:

- By default, a timestamped JSON file is written to tests/results/
- Override output path with --result-json PATH

## Destructive protocol negative tests

There is a separate script for protocol/state-machine negative checks that
intentionally put the signer into failed state.

From repository root:

make test-hw-mldsa-protocol-negative

or directly:

bash tests/run_mldsa_hw_protocol_negative.sh --port /dev/ttyACM0 --speed 62500

What it does:

- Attempts to sign an empty message (size 0), expects rejection
- Attempts to sign an oversize message (>4096), expects rejection
- Verifies the signer no longer responds until power-cycle (repeated getkey attempts)
- Prompts you to unplug/replug between destructive cases

Useful option for this script:

- --failed-retries N
- --result-json PATH

Example:

bash tests/run_mldsa_hw_protocol_negative.sh --failed-retries 5

Result artifact:

- By default, a timestamped JSON file is written to tests/results/
- Override output path with --result-json PATH

Use this script when you want to validate failed-state behavior and recovery,
not for normal fast sign/verify coverage.

## Useful options

- --random N
- --stress N
- --skip-stress
- --keep-workdir
- --bin /path/to/tkey-sign-pq
- --result-json /tmp/mldsa-local-result.json

Example quick run:

bash tests/run_mldsa_hw_local.sh --random 20 --stress 100

Example with explicit result path:

bash tests/run_mldsa_hw_local.sh --random 20 --stress 100 --result-json /tmp/mldsa-local-result.json

## Expected behavior

- Valid signatures always verify successfully.
- Tampered messages and signatures always fail verification.
- Results are judged by verification correctness, not byte-identical signatures.

ML-DSA signing is randomized, so signature bytes are expected to differ between runs.

## Scope and limits

This harness covers practical end-to-end hardware correctness. It does not implement a full ACVP-on-device harness.

For algorithm conformance vectors, keep using:

cd mldsa-native
make run_acvp
