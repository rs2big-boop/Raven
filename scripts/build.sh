#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

BUILD_DIR="${ROOT_DIR}/build"
BUILD_TYPE="${1:-Debug}"

echo "==> Configuring Raven (${BUILD_TYPE})..."
cmake -B "${BUILD_DIR}" -S "${ROOT_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

echo "==> Building Raven..."
cmake --build "${BUILD_DIR}" -j"$(nproc 2>/dev/null || echo 4)"

echo "==> Raven build successful!"
