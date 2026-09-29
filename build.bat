@echo off

setlocal

set "MSYS2=C:\msys64"
set "PATH=%MSYS2%\ucrt64\bin;%MSYS2%\usr\bin;%PATH%"

cd /d "%~dp0"

echo ========================================
echo MSYS2 environment
echo ========================================
echo.

echo PATH:
echo %PATH%

echo.
echo ========================================
echo GCC location
echo ========================================
echo.

where g++.exe

echo.
echo ========================================
echo GCC version
echo ========================================
echo.

g++ --version

if errorlevel 1 (
    echo.
    echo ERROR: g++ could not be started.
    exit /b 1
)

echo.
echo ========================================
echo Building emulator
echo ========================================
echo.

:: ADD ANY NEW .CPP FILES TO THE G++ INCLUDE BATCH OR THE EMULATOR WILL NOT BUILD CORRECTLY WHEN PACKAGED

g++ ^
    -g ^
    "src\main.cpp" ^
    "src\cart.cpp" ^
    "src\cpu.cpp" ^
    "src\cpu_instructions.cpp" ^
    "src\emulator_core.cpp" ^
    "src\memory_bus.cpp" ^
    "src\timer.cpp" ^
    "src\interrupts.cpp" ^
    "src\debug_log.cpp" ^
    "src\serial_test.cpp" ^
    "src\MBC\no_mbc.cpp" ^
    "src\MBC\mbc1.cpp" ^
    "src\MBC\mbc2.cpp" ^
    "src\MBC\mbc3.cpp" ^
    "src\MBC\mbc5.cpp" ^
    "src\MBC\mbc6.cpp" ^
    "src\MBC\mbc7.cpp" ^
    "src\MBC\huc1.cpp" ^
    "src\MBC\huc3.cpp" ^
    "src\MBC\mmm01.cpp" ^
    "src\MBC\m161.cpp" ^
    -I"include" ^
    -I"include\MBC" ^
    -I"%MSYS2%\ucrt64\include" ^
    -L"%MSYS2%\ucrt64\lib" ^
    -lSDL3 ^
    -lcomdlg32 ^
    -o "my_emulator.exe"

if errorlevel 1 (
    echo.
    echo ========================================
    echo BUILD FAILED
    echo ========================================
    exit /b 1
)

echo.
echo ========================================
echo BUILD SUCCESSFUL
echo ========================================

exit /b 0