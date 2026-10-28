#!/usr/bin/env bash
# Raven Programming Language Installer
set -e

echo "🐦‍⬛ Installing Raven Programming Language..."

# Build if not already built
if [ ! -f "build/raven" ]; then
    echo "🔨 Building Raven with CMake..."
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j$(nproc 2>/dev/null || echo 2)
fi

echo "📦 Installing binaries to /usr/local/bin..."
sudo install -m 755 build/raven /usr/local/bin/raven
sudo install -m 755 build/ravenc /usr/local/bin/ravenc
sudo install -m 755 build/rvm /usr/local/bin/rvm

echo "📚 Installing Standard Library to /usr/local/share/raven/stdlib..."
sudo mkdir -p /usr/local/share/raven/stdlib
sudo cp -r stdlib/* /usr/local/share/raven/stdlib/

echo "✅ Raven successfully installed!"
echo "Run 'raven --help' to get started."
