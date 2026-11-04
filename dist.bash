#!/usr/bin/env bash
# dist.bash - Generates release distribution archives for Raven v1.0.0
set -e

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION="1.0.0"
DIST_DIR="${REPO_ROOT}/dist"
PKG_NAME="raven-v${VERSION}-linux-x86_64"
TARGET_DIR="${DIST_DIR}/${PKG_NAME}"

echo "📦 Creating Raven ${VERSION} Production Release..."

# 1. Build
cd "${REPO_ROOT}"
cmake -B "${REPO_ROOT}/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "${REPO_ROOT}/build"

# 2. Test
ctest --test-dir "${REPO_ROOT}/build" --output-on-failure

# 3. Assemble distribution
rm -rf "${DIST_DIR}"
mkdir -p "${TARGET_DIR}/bin"
mkdir -p "${TARGET_DIR}/stdlib"

cp "${REPO_ROOT}/build/raven" "${TARGET_DIR}/bin/"
cp "${REPO_ROOT}/build/ravenc" "${TARGET_DIR}/bin/"
cp "${REPO_ROOT}/build/rvm" "${TARGET_DIR}/bin/"
cp -r "${REPO_ROOT}/stdlib/"* "${TARGET_DIR}/stdlib/"
cp "${REPO_ROOT}/README.md" "${TARGET_DIR}/"
cp "${REPO_ROOT}/LICENSE" "${TARGET_DIR}/" 2>/dev/null || true
cp "${REPO_ROOT}/install.sh" "${TARGET_DIR}/"
cp "${REPO_ROOT}/install.ps1" "${TARGET_DIR}/"
cp "${REPO_ROOT}/install.bat" "${TARGET_DIR}/"

# 4. Tarball
cd "${DIST_DIR}"
tar -czf "${PKG_NAME}.tar.gz" "${PKG_NAME}"
sha256sum "${PKG_NAME}.tar.gz" > "${PKG_NAME}.tar.gz.sha256"

echo "✅ Distribution tarball successfully created:"
echo "   ${DIST_DIR}/${PKG_NAME}.tar.gz"
echo "   Checksum: $(cat ${DIST_DIR}/${PKG_NAME}.tar.gz.sha256)"
# 5. Dist for Windows (if Windows binaries are present)
WIN_BIN_DIR=""
if [ -f "${REPO_ROOT}/build-win/raven.exe" ]; then
    WIN_BIN_DIR="${REPO_ROOT}/build-win"
elif [ -f "${REPO_ROOT}/build/raven.exe" ]; then
    WIN_BIN_DIR="${REPO_ROOT}/build"
fi

if [ -n "${WIN_BIN_DIR}" ]; then
    echo "📦 Creating Raven ${VERSION} Windows Release..."
    PKG_NAME_WIN="raven-v${VERSION}-windows-x86_64"
    TARGET_DIR_WIN="${DIST_DIR}/${PKG_NAME_WIN}"
    rm -rf "${TARGET_DIR_WIN}"
    mkdir -p "${TARGET_DIR_WIN}/bin" "${TARGET_DIR_WIN}/stdlib"
    cp "${WIN_BIN_DIR}/raven.exe" "${TARGET_DIR_WIN}/bin/"
    cp "${WIN_BIN_DIR}/ravenc.exe" "${TARGET_DIR_WIN}/bin/"
    cp "${WIN_BIN_DIR}/rvm.exe" "${TARGET_DIR_WIN}/bin/"
    [ -f "${WIN_BIN_DIR}/rvndb.exe" ] && cp "${WIN_BIN_DIR}/rvndb.exe" "${TARGET_DIR_WIN}/bin/"
    cp -r "${REPO_ROOT}/stdlib/"* "${TARGET_DIR_WIN}/stdlib/"
    cp "${REPO_ROOT}/README.md" "${TARGET_DIR_WIN}/"
    cp "${REPO_ROOT}/LICENSE" "${TARGET_DIR_WIN}/" 2>/dev/null || true
    cp "${REPO_ROOT}/install.ps1" "${TARGET_DIR_WIN}/"
    cp "${REPO_ROOT}/install.bat" "${TARGET_DIR_WIN}/"
    cd "${DIST_DIR}"
    zip -rq "${PKG_NAME_WIN}.zip" "${PKG_NAME_WIN}"
    sha256sum "${PKG_NAME_WIN}.zip" > "${PKG_NAME_WIN}.zip.sha256"
    echo "✅ Windows distribution zip successfully created:"
    echo "   ${DIST_DIR}/${PKG_NAME_WIN}.zip"
    echo "   Checksum: $(cat ${DIST_DIR}/${PKG_NAME_WIN}.zip.sha256)"
fi