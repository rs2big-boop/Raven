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
"${REPO_ROOT}/make.bash"

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

# 4. Tarball
cd "${DIST_DIR}"
tar -czf "${PKG_NAME}.tar.gz" "${PKG_NAME}"
sha256sum "${PKG_NAME}.tar.gz" > "${PKG_NAME}.tar.gz.sha256"

echo "✅ Distribution tarball successfully created:"
echo "   ${DIST_DIR}/${PKG_NAME}.tar.gz"
echo "   Checksum: $(cat ${DIST_DIR}/${PKG_NAME}.tar.gz.sha256)"
