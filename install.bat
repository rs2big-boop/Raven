@echo off
setlocal
title Raven Programming Language Installer

echo ================================================================
echo   🐦 Raven Programming Language — Windows Installer
echo ================================================================
echo.

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0install.ps1"
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Installation encountered an issue.
    echo You can install manually by running in PowerShell:
    echo   irm https://raw.githubusercontent.com/rs2big-boop/Raven/main/install.ps1 ^| iex
    echo.
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo Press any key to exit installer...
pause >nul
