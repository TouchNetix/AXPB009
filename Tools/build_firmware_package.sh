#!/usr/bin/env bash
set -euo pipefail

TOOLS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${TOOLS_DIR}/.." && pwd)"
BUILD_DIR="${BUILD_DIRECTORY:-${ROOT}/build/firmware}"
OUT_DIR="${OUTPUT_DIRECTORY:-${ROOT}/FirmwarePackage}"
VERSION_HEADER="${ROOT}/Inc/USB/usbd_desc.h"

if (($# == 0)); then
    targets=(STM32F070CB STM32F072CB)
else
    targets=("$@")
fi

major_hex="$(sed -n 's/.*USB_DEVICE_MAJOR_VERSION[[:space:]]*(0x\([0-9A-Fa-f]*\)).*/\1/p' "${VERSION_HEADER}")"
minor_hex="$(sed -n 's/.*USB_DEVICE_MINOR_VERSION[[:space:]]*(0x\([0-9A-Fa-f]*\)).*/\1/p' "${VERSION_HEADER}")"
if [[ -z "${major_hex}" || -z "${minor_hex}" ]]; then
    echo "Could not read the firmware version from ${VERSION_HEADER}" >&2
    exit 1
fi

major=$((16#${major_hex}))
minor=$((16#${minor_hex}))
revision="$(printf '%02d%02d' "${major}" "${minor}")"
version_bcd="$(printf '0x%02X%02X' "${major}" "${minor}")"
cmake_targets="$(IFS=';'; echo "${targets[*]}")"

mkdir -p "${BUILD_DIR}" "${OUT_DIR}"

cmake -S "${ROOT}" -B "${BUILD_DIR}" -G Ninja \
    -DCMAKE_BUILD_TYPE= \
    -DCMAKE_TOOLCHAIN_FILE="${ROOT}/cmake/arm-none-eabi-toolchain.cmake" \
    -DAXPB009_TARGETS="${cmake_targets}"
cmake --build "${BUILD_DIR}" --target clean
cmake --build "${BUILD_DIR}" --parallel

for target in "${targets[@]}"; do
    hex_file="${BUILD_DIR}/${target}/axpb009.hex"
    dfu_file="${OUT_DIR}/${target}_AXPB009_${revision}.dfu"
    packaged_hex_file="${OUT_DIR}/${target}_AXPB009_${revision}.hex"

    rm -f "${dfu_file}" "${packaged_hex_file}" \
        "${OUT_DIR}/${target}_${revision}.dfu" \
        "${OUT_DIR}/${target}_${revision}.hex"

    if [[ ! -f "${hex_file}" ]]; then
        echo "Build completed without expected HEX file: ${hex_file}" >&2
        exit 1
    fi

    python3 "${TOOLS_DIR}/dfuse_pack.py" \
        "${hex_file}" "${dfu_file}" \
        --name "${target} AXPB009 v${major}.${minor}" \
        --version "${version_bcd}"
    cp "${hex_file}" "${packaged_hex_file}"
    echo "PACKAGED ${target}: ${dfu_file}, ${packaged_hex_file}"
done

echo "Firmware package version: ${major}.${minor}"
echo "Compiler: $(arm-none-eabi-gcc --version | head -n 1)"
echo "Packaged (${#targets[@]}): ${targets[*]}"
