#! /bin/sh

# SPDX-FileCopyrightText: 2023 Tillitis AB <tillitis.se>
# SPDX-License-Identifier: BSD-2-Clause

set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
libdir=${LIBDIR:-"$project_dir/../tillitis-key1/hw/application_fpga/tkey-libs"}

image=${IMAGE:-localhost/tkey-builder-local}
make -j -C "$libdir" podman IMAGE="$image"
make -j -C "$project_dir" podman LIBDIR="$libdir" IMAGE="$image" \
    FIPS202_BACKEND="${FIPS202_BACKEND:-hardware}" \
    PODMAN_TARGET="${PODMAN_TARGET:-all}"
