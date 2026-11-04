# Install the Raven editor extension into VS Code and/or Cursor (Windows).
$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SrcDir = Join-Path $RepoRoot "editors\vscode"

if (!(Test-Path (Join-Path $SrcDir "package.json"))) {
    Write-Error "Extension sources not found at $SrcDir"
    exit 1
}

if (!(Test-Path (Join-Path $SrcDir "out\extension.js"))) {
    Write-Host "Compiling TypeScript extension..." -ForegroundColor Yellow
    Push-Location $SrcDir
    try {
        if (!(Test-Path "node_modules")) {
            npm install --no-fund --no-audit
        }
        npm run compile
    } finally {
        Pop-Location
    }
}

$targets = @(
    (Join-Path $env:USERPROFILE ".vscode\extensions\raven-lang"),
    (Join-Path $env:USERPROFILE ".cursor\extensions\raven-lang"),
    (Join-Path $env:USERPROFILE ".vscode-insiders\extensions\raven-lang")
)

$installed = 0
foreach ($target in $targets) {
    $parent = Split-Path -Parent $target
    if (!(Test-Path $parent)) {
        continue
    }

    Write-Host "Installing Raven extension -> $target" -ForegroundColor Cyan
    if (Test-Path $target) {
        Remove-Item $target -Recurse -Force
    }
    New-Item -ItemType Directory -Path $target -Force | Out-Null

    Copy-Item (Join-Path $SrcDir "package.json") $target -Force
    Copy-Item (Join-Path $SrcDir "language-configuration.json") $target -Force
    Copy-Item (Join-Path $SrcDir "LICENSE") $target -Force -ErrorAction SilentlyContinue
    Copy-Item (Join-Path $SrcDir "README.md") $target -Force -ErrorAction SilentlyContinue
    Copy-Item (Join-Path $SrcDir "out") $target -Recurse -Force
    Copy-Item (Join-Path $SrcDir "snippets") $target -Recurse -Force
    Copy-Item (Join-Path $SrcDir "syntaxes") $target -Recurse -Force
    if (Test-Path (Join-Path $SrcDir "media")) {
        Copy-Item (Join-Path $SrcDir "media") $target -Recurse -Force
    }
    $installed++
}

if ($installed -eq 0) {
    Write-Host "No VS Code/Cursor extensions directory found yet." -ForegroundColor Yellow
    Write-Host "Open VS Code or Cursor once, then re-run scripts\install_vscode_ext.ps1" -ForegroundColor Yellow
    exit 0
}

Write-Host "Done. Reload the editor window to activate Raven IntelliSense." -ForegroundColor Green
