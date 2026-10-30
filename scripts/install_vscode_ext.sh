#!/usr/bin/env bash
set -e

EXT_DIR="$HOME/.vscode/extensions/raven-lang"
SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/editors/vscode"

echo "🐦 Installing Raven VS Code extension..."
mkdir -p "$EXT_DIR"
cp -r "$SRC_DIR"/* "$EXT_DIR/"

echo "✅ Raven extension installed to: $EXT_DIR"
echo "Restart VS Code or press Ctrl+Shift+P -> 'Developer: Reload Window' to apply."
