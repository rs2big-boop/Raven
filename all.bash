#!/usr/bin/env bash
# all.bash - Builds Raven and runs full test suite (like Go's all.bash)
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

./make.bash

echo "==> Running Test Suite..."
ctest --test-dir build --output-on-failure

echo "==> Running Go Standard Library Showcase..."
./build/raven examples/go_stdlib_tour.rvn

echo "==> All Raven verification passed!"
