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

g++ ^
    -g ^
    "src\main.cpp" ^
    "src\cart.cpp" ^
    -I"include" ^
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