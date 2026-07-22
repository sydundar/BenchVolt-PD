@echo off
REM ============================================================
REM BenchVolt-PD - Windows build script
REM Place this file in the SAME folder as BenchVolt-PD.py,
REM benchvolt_remote.py and BenchVolt-PD.spec, then double-click
REM it, or run it from a cmd window.
REM ============================================================

echo [1/3] Checking Python...
python --version
if %errorlevel% neq 0 (
    echo ERROR: Python not found. Install it from https://www.python.org/downloads/
    echo Make sure to check "Add Python to PATH" during installation.
    pause
    exit /b 1
)

echo [2/3] Installing required packages...
python -m pip install --upgrade pip
python -m pip install pyinstaller customtkinter numpy matplotlib pyserial

echo [3/3] Building the EXE...
python -m PyInstaller --noconfirm BenchVolt-PD.spec

echo.
echo ============================================================
echo Done. Your EXE is here:
echo   dist\BenchVolt-PD.exe
echo ============================================================
pause
