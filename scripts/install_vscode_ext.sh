#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${REPO_ROOT}/editors/vscode"

if [ ! -f "${SRC_DIR}/package.json" ]; then
    echo "Extension sources not found at ${SRC_DIR}" >&2
    exit 1
fi

if [ ! -f "${SRC_DIR}/out/extension.js" ]; then
    echo "Compiling TypeScript extension..."
    (
        cd "${SRC_DIR}"
        if [ ! -d node_modules ]; then
            npm install --no-fund --no-audit
        fi
        npm run compile
    )
fi

install_into() {
    local target="$1"
    local parent
    parent="$(dirname "${target}")"
    if [ ! -d "${parent}" ]; then
        return 1
    fi

    echo "Installing Raven extension -> ${target}"
    rm -rf "${target}"
    mkdir -p "${target}"
    cp "${SRC_DIR}/package.json" "${SRC_DIR}/language-configuration.json" "${target}/"
    cp "${SRC_DIR}/LICENSE" "${target}/" 2>/dev/null || true
    cp "${SRC_DIR}/README.md" "${target}/" 2>/dev/null || true
    cp -R "${SRC_DIR}/out" "${SRC_DIR}/snippets" "${SRC_DIR}/syntaxes" "${target}/"
    if [ -d "${SRC_DIR}/media" ]; then
        cp -R "${SRC_DIR}/media" "${target}/"
    fi
    return 0
}

INSTALLED=0
for target in \
    "${HOME}/.vscode/extensions/raven-lang" \
    "${HOME}/.cursor/extensions/raven-lang" \
    "${HOME}/.vscode-insiders/extensions/raven-lang"
do
    if install_into "${target}"; then
        INSTALLED=$((INSTALLED + 1))
    fi
done

if [ "${INSTALLED}" -eq 0 ]; then
    echo "No VS Code/Cursor extensions directory found yet."
    echo "Open the editor once, then re-run ./scripts/install_vscode_ext.sh"
    exit 0
fi

echo "Done. Reload the editor window to activate Raven IntelliSense."
