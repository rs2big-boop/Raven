#!/usr/bin/env bash
# run.bash - Runs a script with the Raven interpreter (like Go's run.bash)
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "$DIR/build/raven" "$@"
