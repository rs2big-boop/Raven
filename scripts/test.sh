#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

BUILD_DIR="${ROOT_DIR}/build"

if [[ ! -d "${BUILD_DIR}" ]]; then
    "${SCRIPT_DIR}/build.sh"
fi

echo "==> Running Raven CTest Suite..."
ctest --test-dir "${BUILD_DIR}" --output-on-failure
