#!/usr/bin/env bash
# ==============================================================================
# Raven Programming Language — Universal Linux & macOS Installer
#
# Quick Install:
#   curl -fsSL https://raw.githubusercontent.com/rs2big-boop/Raven/main/install.sh | bash
# Local Install:
#   ./install.sh
# ==============================================================================
set -e

VERSION="1.0.1"
REPO_OWNER="rs2big-boop"
REPO_NAME="Raven"

echo "🐦 Installing Raven Programming Language v${VERSION}..."

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" 2>/dev/null && pwd || echo "")"

# Decide install prefix: prefer ~/.local if non-root or /usr/local if writable/root
USE_SYSTEM=false
if [ "$(id -u)" -eq 0 ]; then
    USE_SYSTEM=true
elif [ -w "/usr/local/bin" ]; then
    USE_SYSTEM=true
fi

if [ "$USE_SYSTEM" = true ]; then
    BIN_DIR="/usr/local/bin"
    STDLIB_DIR="/usr/local/share/raven/stdlib"
else
    BIN_DIR="${HOME}/.local/bin"
    STDLIB_DIR="${HOME}/.local/share/raven/stdlib"
fi

mkdir -p "${BIN_DIR}"
mkdir -p "${STDLIB_DIR}"

# 1. Locate binaries
BIN_SOURCE=""
if [ -n "${SCRIPT_DIR}" ] && [ -f "${SCRIPT_DIR}/build/raven" ]; then
    BIN_SOURCE="${SCRIPT_DIR}/build"
elif [ -n "${SCRIPT_DIR}" ] && [ -f "${SCRIPT_DIR}/bin/raven" ]; then
    BIN_SOURCE="${SCRIPT_DIR}/bin"
elif [ -f "./build/raven" ]; then
    BIN_SOURCE="./build"
fi

# 2. Build if running in source repo without build
if [ -z "${BIN_SOURCE}" ] && [ -n "${SCRIPT_DIR}" ] && [ -f "${SCRIPT_DIR}/CMakeLists.txt" ]; then
    echo "🔨 Building Raven with CMake..."
    cmake -B "${SCRIPT_DIR}/build" -DCMAKE_BUILD_TYPE=Release
    cmake --build "${SCRIPT_DIR}/build" -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)"
    BIN_SOURCE="${SCRIPT_DIR}/build"
fi

# 3. If still no binaries (e.g. piped from curl), download pre-built release tarball
if [ -z "${BIN_SOURCE}" ]; then
    OS="$(uname -s | tr '[:upper:]' '[:lower:]')"
    ARCH="$(uname -m)"
    if [ "${ARCH}" = "x86_64" ]; then
        ARCH="x86_64"
    elif [ "${ARCH}" = "aarch64" ] || [ "${ARCH}" = "arm64" ]; then
        ARCH="arm64"
    fi

    TAR_NAME="raven-v${VERSION}-${OS}-${ARCH}.tar.gz"
    DOWNLOAD_URL="https://github.com/${REPO_OWNER}/${REPO_NAME}/releases/download/v${VERSION}/${TAR_NAME}"
    TMP_DIR="$(mktemp -d)"

    echo "🌐 Downloading pre-built binary for ${OS}-${ARCH} from GitHub Releases..."
    if curl -fsSL "${DOWNLOAD_URL}" -o "${TMP_DIR}/${TAR_NAME}" 2>/dev/null; then
        tar -xzf "${TMP_DIR}/${TAR_NAME}" -C "${TMP_DIR}"
        EXTRACTED_DIR="$(find "${TMP_DIR}" -maxdepth 1 -type d -name "raven-*" | head -n1)"
        if [ -d "${EXTRACTED_DIR}/bin" ]; then
            BIN_SOURCE="${EXTRACTED_DIR}/bin"
            cp -r "${EXTRACTED_DIR}/stdlib/"* "${STDLIB_DIR}/" 2>/dev/null || true
        fi
    else
        echo "⚠️  Pre-built release archive not yet uploaded for ${OS}-${ARCH}."
        echo "   Falling back to building from source..."
        git clone --depth 1 "https://github.com/${REPO_OWNER}/${REPO_NAME}.git" "${TMP_DIR}/Raven"
        cmake -B "${TMP_DIR}/Raven/build" -DCMAKE_BUILD_TYPE=Release
        cmake --build "${TMP_DIR}/Raven/build" -j"$(nproc 2>/dev/null || echo 2)"
        BIN_SOURCE="${TMP_DIR}/Raven/build"
        cp -r "${TMP_DIR}/Raven/stdlib/"* "${STDLIB_DIR}/" 2>/dev/null || true
    fi
fi

# 4. Copy binaries
echo "📦 Installing binaries to ${BIN_DIR}..."
cp "${BIN_SOURCE}/raven"  "${BIN_DIR}/raven"
cp "${BIN_SOURCE}/ravenc" "${BIN_DIR}/ravenc"
cp "${BIN_SOURCE}/rvm"    "${BIN_DIR}/rvm"
chmod +x "${BIN_DIR}/raven" "${BIN_DIR}/ravenc" "${BIN_DIR}/rvm"

# 5. Copy stdlib if available locally
if [ -n "${SCRIPT_DIR}" ] && [ -d "${SCRIPT_DIR}/stdlib" ]; then
    echo "📚 Installing standard library to ${STDLIB_DIR}..."
    cp -r "${SCRIPT_DIR}/stdlib/"* "${STDLIB_DIR}/"
fi

# 6. Ensure PATH contains BIN_DIR
PATH_ADDED=false
case ":${PATH}:" in
    *":${BIN_DIR}:"*) ;;
    *)
        for RC in "${HOME}/.bashrc" "${HOME}/.zshrc" "${HOME}/.profile"; do
            if [ -f "${RC}" ] && ! grep -q "${BIN_DIR}" "${RC}"; then
                echo "export PATH=\"${BIN_DIR}:\$PATH\"" >> "${RC}"
                PATH_ADDED=true
            fi
        done
        ;;
esac

# 7. Install VS Code / Cursor extension when possible
EXT_INSTALLED=false
if [ -n "${SCRIPT_DIR}" ] && [ -f "${SCRIPT_DIR}/scripts/install_vscode_ext.sh" ]; then
    echo "🧩 Installing Raven editor extension..."
    if bash "${SCRIPT_DIR}/scripts/install_vscode_ext.sh"; then
        EXT_INSTALLED=true
    fi
elif [ -f "${SCRIPT_DIR}/editors/vscode/out/extension.js" ]; then
    echo "🧩 Installing Raven editor extension..."
    if bash "${SCRIPT_DIR}/scripts/install_vscode_ext.sh" 2>/dev/null; then
        EXT_INSTALLED=true
    fi
fi

echo ""
echo "================================================================"
echo "  🐦 Raven Programming Language successfully installed!"
echo "================================================================"
echo "  Binaries : ${BIN_DIR}"
echo "  Stdlib   : ${STDLIB_DIR}"
if [ "${EXT_INSTALLED}" = true ]; then
    echo "  Editor   : VS Code / Cursor extension installed (reload window)"
fi
echo ""
echo "  Tools installed:"
echo "    • raven   - Driver, REPL, Package Manager & Toolchain"
echo "    • ravenc  - Corvus Bytecode Compiler & Native Packager"
echo "    • rvm     - Ultra-fast Virtual Machine Execution Engine"
echo ""
if [ "$PATH_ADDED" = true ]; then
    echo "  💡 Added ${BIN_DIR} to your shell profile."
    echo "     Run 'source ~/.bashrc' (or restart your terminal) to begin."
else
    echo "  Verify installation with:"
    echo "    raven --version"
fi
echo ""
