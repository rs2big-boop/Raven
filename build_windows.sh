#!/usr/bin/env bash
# ==============================================================================
# build_windows.sh — Cross-compile Raven for Windows on Linux
# ==============================================================================
set -e

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION="1.0.0"

echo "🪟 Cross-compiling Raven v${VERSION} for Windows (x86_64)..."

# 1. Check for MinGW cross-compiler
if ! command -v x86_64-w64-mingw32-g++ &>/dev/null; then
    echo ""
    echo "❌ MinGW-w64 cross-compiler not found!"
    echo "   To cross-compile for Windows on Linux, install MinGW:"
    echo ""
    echo "   • Arch Linux / Manjaro:"
    echo "       sudo pacman -S mingw-w64-gcc"
    echo ""
    echo "   • Ubuntu / Debian / Mint:"
    echo "       sudo apt install mingw-w64"
    echo ""
    echo "   • Fedora:"
    echo "       sudo dnf install mingw64-gcc-c++"
    echo ""
    echo "💡 Note: You don't HAVE to build locally for Windows!"
    echo "   GitHub Actions already builds Windows .exe files automatically"
    echo "   on every tag release using our .github/workflows/release.yml workflow."
    exit 1
fi

# 2. Configure and build
BUILD_DIR="${REPO_ROOT}/build-win"
echo "⚙️  Configuring CMake with MinGW toolchain..."
cmake -B "${BUILD_DIR}" \
      -DCMAKE_TOOLCHAIN_FILE="${REPO_ROOT}/cmake/mingw-w64.cmake" \
      -DCMAKE_BUILD_TYPE=Release \
      "${REPO_ROOT}"

echo "🔨 Compiling Windows executables (raven.exe, ravenc.exe, rvm.exe)..."
cmake --build "${BUILD_DIR}" --config Release -j"$(nproc 2>/dev/null || echo 2)"

echo "✅ Windows executables built successfully in ${BUILD_DIR}:"
ls -la "${BUILD_DIR}"/*.exe

# 3. Test with WINE if available
if command -v wine &>/dev/null; then
    echo ""
    echo "🍷 Testing Windows binary on Linux using Wine..."
    wine "${BUILD_DIR}/raven.exe" --version || true
fi

# 4. Package Windows release ZIP
DIST_DIR="${REPO_ROOT}/dist"
PKG_NAME="raven-v${VERSION}-windows-x86_64"
TARGET_DIR="${DIST_DIR}/${PKG_NAME}"

rm -rf "${TARGET_DIR}" "${DIST_DIR}/${PKG_NAME}.zip"
mkdir -p "${TARGET_DIR}/bin" "${TARGET_DIR}/stdlib"

cp "${BUILD_DIR}"/*.exe "${TARGET_DIR}/bin/"
cp -r "${REPO_ROOT}/stdlib/"* "${TARGET_DIR}/stdlib/"
cp "${REPO_ROOT}/README.md" "${TARGET_DIR}/"
cp "${REPO_ROOT}/LICENSE" "${TARGET_DIR}/" 2>/dev/null || true
cp "${REPO_ROOT}/install.ps1" "${TARGET_DIR}/"
cp "${REPO_ROOT}/install.bat" "${TARGET_DIR}/"

cd "${DIST_DIR}"
if command -v zip &>/dev/null; then
    zip -rq "${PKG_NAME}.zip" "${PKG_NAME}"
    sha256sum "${PKG_NAME}.zip" > "${PKG_NAME}.zip.sha256"
    echo ""
    echo "📦 Windows release package created:"
    echo "   ${DIST_DIR}/${PKG_NAME}.zip"
    echo "   Checksum: $(cat ${DIST_DIR}/${PKG_NAME}.zip.sha256)"
fi

echo ""
echo "🎉 Done! You can now transfer ${PKG_NAME}.zip or the .exe files to any Windows machine."
