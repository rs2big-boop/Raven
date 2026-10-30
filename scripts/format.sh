#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

if command -v clang-format &> /dev/null; then
    echo "==> Formatting C++ code with clang-format..."
    find "${ROOT_DIR}/src" "${ROOT_DIR}/include" "${ROOT_DIR}/tests" -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
    echo "==> Done formatting!"
else
    echo "clang-format not found; skipping."
fi
