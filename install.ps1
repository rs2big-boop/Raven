# ==============================================================================
# Raven Programming Language — Official Windows Installer (PowerShell)
# Usage:
#   irm https://raw.githubusercontent.com/rs2big-boop/Raven/main/install.ps1 | iex
# or run locally:
#   powershell -ExecutionPolicy Bypass -File install.ps1
# ==============================================================================

$ErrorActionPreference = "Stop"

$RavenVersion = "1.0.1"
$RepoOwner    = "rs2big-boop"
$RepoName     = "Raven"
$InstallDir   = Join-Path $HOME ".raven"
$BinDir       = Join-Path $InstallDir "bin"
$StdlibDir    = Join-Path $InstallDir "stdlib"

function Install-FromPackageRoot {
    param(
        [Parameter(Mandatory = $true)][string]$PackageRoot
    )

    $pkgBin = Join-Path $PackageRoot "bin"
    $pkgStd = Join-Path $PackageRoot "stdlib"

    if (Test-Path (Join-Path $pkgBin "raven.exe")) {
        Copy-Item (Join-Path $pkgBin "*.exe") -Destination $BinDir -Force
    } elseif (Test-Path (Join-Path $PackageRoot "raven.exe")) {
        Copy-Item (Join-Path $PackageRoot "*.exe") -Destination $BinDir -Force
    } else {
        throw "Package root '$PackageRoot' does not contain Raven binaries."
    }

    if (Test-Path $pkgStd) {
        Copy-Item (Join-Path $pkgStd "*") -Destination $StdlibDir -Recurse -Force
    }
}

function Find-PackageRoot {
    param(
        [Parameter(Mandatory = $true)][string]$ExtractRoot
    )

    $direct = Join-Path $ExtractRoot "bin\raven.exe"
    if (Test-Path $direct) {
        return $ExtractRoot
    }

    $nested = Get-ChildItem -Path $ExtractRoot -Directory -ErrorAction SilentlyContinue |
        Where-Object { Test-Path (Join-Path $_.FullName "bin\raven.exe") } |
        Select-Object -First 1

    if ($nested) {
        return $nested.FullName
    }

    $looseExe = Get-ChildItem -Path $ExtractRoot -Filter "raven.exe" -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($looseExe) {
        return $looseExe.Directory.FullName
    }

    return $null
}

Write-Host "🐦 Installing Raven Programming Language v$RavenVersion for Windows..." -ForegroundColor Cyan

# 1. Create target directories
New-Item -ItemType Directory -Path $BinDir -Force | Out-Null
New-Item -ItemType Directory -Path $StdlibDir -Force | Out-Null

# Resolve script directory at top-level only.
# Under `irm ... | iex`, there is no script path — leave $ScriptDir empty and download.
$ScriptDir = $null
if ($PSScriptRoot) {
    $ScriptDir = $PSScriptRoot
} elseif ($MyInvocation.MyCommand.Path) {
    $ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
}
$LocalBinFound = $false

# 2. Prefer binaries next to this script (cloned repo or extracted release zip)
if ($ScriptDir) {
    $candidates = @(
        (Join-Path $ScriptDir "build\Release"),
        (Join-Path $ScriptDir "build"),
        (Join-Path $ScriptDir "bin")
    )
    foreach ($candidate in $candidates) {
        if (Test-Path (Join-Path $candidate "raven.exe")) {
            Write-Host "📦 Copying binaries from $candidate..." -ForegroundColor Green
            Copy-Item (Join-Path $candidate "*.exe") -Destination $BinDir -Force
            $LocalBinFound = $true
            break
        }
    }

    $localStdlib = Join-Path $ScriptDir "stdlib"
    if ($LocalBinFound -and (Test-Path $localStdlib)) {
        Copy-Item (Join-Path $localStdlib "*") -Destination $StdlibDir -Recurse -Force
    }
}

# 3. If no local binaries, download the pre-built Windows release package
if (-not $LocalBinFound) {
    $ReleaseUrl  = "https://github.com/$RepoOwner/$RepoName/releases/download/v$RavenVersion/raven-v$RavenVersion-windows-x86_64.zip"
    $FallbackUrl = "https://github.com/$RepoOwner/$RepoName/releases/latest/download/raven-v$RavenVersion-windows-x86_64.zip"
    $ZipPath     = Join-Path $env:TEMP "raven-windows.zip"
    $ExtractRoot = Join-Path $env:TEMP "raven_extract"

    Write-Host "🌐 Downloading pre-built Windows binaries from GitHub Releases..." -ForegroundColor Yellow
    $downloaded = $false
    foreach ($url in @($ReleaseUrl, $FallbackUrl)) {
        try {
            Invoke-WebRequest -Uri $url -OutFile $ZipPath -UseBasicParsing
            $downloaded = $true
            break
        } catch {
            Write-Host "Download failed from $url" -ForegroundColor Yellow
        }
    }

    if (-not $downloaded) {
        Write-Error "Could not download pre-built binaries. Please visit: https://github.com/$RepoOwner/$RepoName/releases"
        exit 1
    }

    if (Test-Path $ExtractRoot) {
        Remove-Item $ExtractRoot -Recurse -Force -ErrorAction SilentlyContinue
    }

    Write-Host "📂 Extracting binaries..." -ForegroundColor Green
    Expand-Archive -Path $ZipPath -DestinationPath $ExtractRoot -Force

    # Release zips contain a top-level folder: raven-vX.Y.Z-windows-x86_64/{bin,stdlib}
    $PackageRoot = Find-PackageRoot -ExtractRoot $ExtractRoot
    if (-not $PackageRoot) {
        Write-Error "Downloaded archive did not contain Raven binaries. Please re-download from GitHub Releases."
        exit 1
    }

    Install-FromPackageRoot -PackageRoot $PackageRoot

    Remove-Item $ZipPath -Force -ErrorAction SilentlyContinue
    Remove-Item $ExtractRoot -Recurse -Force -ErrorAction SilentlyContinue
}

# 4. Final verification — never report success with an empty install
$RavenExe = Join-Path $BinDir "raven.exe"
if (-not (Test-Path $RavenExe)) {
    Write-Error "Installation failed: raven.exe was not installed to $BinDir"
    exit 1
}

# 5. Add to User PATH Environment Variable
$UserPath = [Environment]::GetEnvironmentVariable("Path", "User")
if ([string]::IsNullOrEmpty($UserPath) -or ($UserPath -notlike "*$BinDir*")) {
    Write-Host "🔧 Adding $BinDir to your User PATH..." -ForegroundColor Cyan
    $NewPath = if ($UserPath) { "$BinDir;$UserPath" } else { $BinDir }
    [Environment]::SetEnvironmentVariable("Path", $NewPath, "User")
    $env:Path = "$BinDir;$env:Path"
} else {
    Write-Host "✓ $BinDir is already in your PATH" -ForegroundColor Green
}

# 6. Success
Write-Host ""
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host "  🐦 Raven Programming Language successfully installed!" -ForegroundColor Green
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host "  Location : $BinDir"
Write-Host "  Standard Library : $StdlibDir"
Write-Host ""
Write-Host "  Quick Start Commands:" -ForegroundColor Yellow
Write-Host "    raven run <script.rvn>      # Run with tree-walk interpreter"
Write-Host "    ravenc <script.rvn>         # Compile to Corvus bytecode (.rvc)"
Write-Host "    rvm <script.rvc>            # Execute Corvus bytecode VM"
Write-Host "    raven --help                # View full toolchain commands"
Write-Host ""
Write-Host "Note: Restart PowerShell (or open a new terminal), then run:" -ForegroundColor Gray
Write-Host "  raven --version" -ForegroundColor Gray
Write-Host ""
