#!/usr/bin/env bash
# make.bash - Builds the Raven compiler and toolchain (like Go's make.bash)
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

echo "==> Building Raven Toolchain..."
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc 2>/dev/null || echo 4)
echo "==> Raven build completed successfully."
