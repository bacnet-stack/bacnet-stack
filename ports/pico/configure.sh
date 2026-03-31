#!/bin/bash
# configure.sh - Clone external dependencies required to build the Pico port

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXTERNAL_DIR="${SCRIPT_DIR}/external"

mkdir -p "${EXTERNAL_DIR}"

PICO_SDK_DIR="${EXTERNAL_DIR}/pico-sdk"
PICO_SDK_URL="${PICO_SDK_URL:-https://github.com/raspberrypi/pico-sdk.git}"
PICO_SDK_REF="${PICO_SDK_REF:-2.2.0}"

if [ -d "${PICO_SDK_DIR}/.git" ]; then
    echo "Pico SDK already present at ${PICO_SDK_DIR}."
    echo "Requested SDK ref: ${PICO_SDK_REF}"
else
    echo "Cloning Raspberry Pi Pico SDK ${PICO_SDK_REF} ..."
    git clone --branch "${PICO_SDK_REF}" --depth 1 "${PICO_SDK_URL}" \
        "${PICO_SDK_DIR}"
    git -C "${PICO_SDK_DIR}" submodule update --init --depth 1
fi

echo ""
echo "External dependencies are ready in ${EXTERNAL_DIR}/"
echo ""
echo "To build:"
echo "  cmake -S . -B build -DPICO_SDK_PATH=${PICO_SDK_DIR}"
echo "  # Optional: override the default Pico 2 board selection"
echo "  # cmake -S . -B build -DPICO_SDK_PATH=${PICO_SDK_DIR} -DPICO_BOARD=pico2_w"
echo "  cmake --build build -- -j$(nproc)"
