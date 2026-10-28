#!/usr/bin/env bash
# clean.bash - Cleans build artifacts (like Go's clean.bash)
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
rm -rf "$DIR/build"
echo "==> Raven build artifacts cleaned."
