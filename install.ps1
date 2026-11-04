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
$InstallDir   = "$HOME\.raven"
$BinDir       = "$InstallDir\bin"
$StdlibDir    = "$InstallDir\stdlib"

Write-Host "🐦 Installing Raven Programming Language v$RavenVersion for Windows..." -ForegroundColor Cyan

# 1. Create target directories
if (!(Test-Path $BinDir))    { New-Item -ItemType Directory -Path $BinDir -Force | Out-Null }
if (!(Test-Path $StdlibDir)) { New-Item -ItemType Directory -Path $StdlibDir -Force | Out-Null }

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

# 2. Check if local build/binaries exist (running from cloned repository)
$LocalBinFound = $false
if ($ScriptDir -and (Test-Path "$ScriptDir\build\Release\raven.exe")) {
    Write-Host "📦 Copying locally built binaries from $ScriptDir\build\Release..." -ForegroundColor Green
    Copy-Item "$ScriptDir\build\Release\*.exe" -Destination $BinDir -Force
    $LocalBinFound = $true
} elseif ($ScriptDir -and (Test-Path "$ScriptDir\build\raven.exe")) {
    Write-Host "📦 Copying locally built binaries from $ScriptDir\build..." -ForegroundColor Green
    Copy-Item "$ScriptDir\build\*.exe" -Destination $BinDir -Force
    $LocalBinFound = $true
} elseif ($ScriptDir -and (Test-Path "$ScriptDir\bin\raven.exe")) {
    Write-Host "📦 Copying binaries from release archive..." -ForegroundColor Green
    Copy-Item "$ScriptDir\bin\*.exe" -Destination $BinDir -Force
    $LocalBinFound = $true
}

# 3. If no local binaries, download pre-built release package from GitHub
if (!$LocalBinFound) {
    $ReleaseUrl = "https://github.com/$RepoOwner/$RepoName/releases/download/v$RavenVersion/raven-v$RavenVersion-windows-x86_64.zip"
    $FallbackUrl = "https://github.com/$RepoOwner/$RepoName/releases/latest/download/raven-v$RavenVersion-windows-x86_64.zip"
    $ZipPath = "$env:TEMP\raven-windows.zip"

    Write-Host "🌐 Downloading pre-built Windows binaries from GitHub Releases..." -ForegroundColor Yellow
    try {
        Invoke-WebRequest -Uri $ReleaseUrl -OutFile $ZipPath -UseBasicParsing
    } catch {
        Write-Host "Trying fallback release URL..." -ForegroundColor Yellow
        try {
            Invoke-WebRequest -Uri $FallbackUrl -OutFile $ZipPath -UseBasicParsing
        } catch {
            # If download fails (e.g. offline or repo just tagged), try compiling with CMake if available
            if (Get-Command cmake -ErrorAction SilentlyContinue) {
                Write-Host "⚙️  Building locally with CMake..." -ForegroundColor Yellow
                cmake -B "$InstallDir\build" -DCMAKE_BUILD_TYPE=Release
                cmake --build "$InstallDir\build" --config Release
                Copy-Item "$InstallDir\build\Release\*.exe" -Destination $BinDir -Force
                $LocalBinFound = $true
            } else {
                Write-Error "Could not download pre-built binaries and CMake is not installed. Please visit: https://github.com/$RepoOwner/$RepoName/releases"
                exit 1
            }
        }
    }

    if (Test-Path $ZipPath) {
        Write-Host "📂 Extracting binaries..." -ForegroundColor Green
        Expand-Archive -Path $ZipPath -DestinationPath "$env:TEMP\raven_extract" -Force
        if (Test-Path "$env:TEMP\raven_extract\bin") {
            Copy-Item "$env:TEMP\raven_extract\bin\*.exe" -Destination $BinDir -Force
            if (Test-Path "$env:TEMP\raven_extract\stdlib") {
                Copy-Item "$env:TEMP\raven_extract\stdlib\*" -Destination $StdlibDir -Recurse -Force
            }
        } else {
            Copy-Item "$env:TEMP\raven_extract\*.exe" -Destination $BinDir -Force
        }
        Remove-Item $ZipPath -Force -ErrorAction SilentlyContinue
        Remove-Item "$env:TEMP\raven_extract" -Recurse -Force -ErrorAction SilentlyContinue
    }
}

# 4. Copy standard library if local repository is present
if ($ScriptDir -and (Test-Path "$ScriptDir\stdlib")) {
    Copy-Item "$ScriptDir\stdlib\*" -Destination $StdlibDir -Recurse -Force
}

# 5. Add to User PATH Environment Variable
$UserPath = [Environment]::GetEnvironmentVariable("Path", "User")
if ($UserPath -notlike "*$BinDir*") {
    Write-Host "🔧 Adding $BinDir to your User PATH..." -ForegroundColor Cyan
    $NewPath = if ($UserPath) { "$BinDir;$UserPath" } else { $BinDir }
    [Environment]::SetEnvironmentVariable("Path", $NewPath, "User")
    $env:Path = "$BinDir;$env:Path"
} else {
    Write-Host "✓ $BinDir is already in your PATH" -ForegroundColor Green
}

# 6. Verify installation
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
Write-Host "Note: If running in a new terminal, restart PowerShell or run:" -ForegroundColor Gray
Write-Host "  `$env:Path = `"$BinDir;`$env:Path`"" -ForegroundColor Gray
Write-Host ""
