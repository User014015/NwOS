@echo off
setlocal EnableExtensions

rem ============================================================
rem NwC host compiler build script
rem
rem Expected layout:
rem   compiler/
rem     compilerMain.c
rem     lexer/lexer.c
rem     lexer/lexer.h
rem     parser/parser.c
rem     parser/parser.h
rem     codegen/codegen.c
rem     codegen/codegen.h
rem     nwc_kernel.c
rem     nwc_kernel.h
rem     nwc_runtime.c
rem     nwc_runtime.h
rem     builded/
rem       nwc.exe          <- generated here
rem
rem NOTE:
rem   nwc_kernel.* and nwc_runtime.* are kernel-side sources.
rem   They are NOT linked into the Windows NwC executable.
rem ============================================================

pushd "%~dp0"

set "ROOT=%CD%"
set "OUT=%ROOT%\builded"
set "NWC_OUT=%OUT%\nwc.exe"

if not exist "%OUT%" mkdir "%OUT%"

where clang >nul 2>nul
if errorlevel 1 (
    echo [NwC] ERROR: clang was not found in PATH.
    echo [NwC] Add LLVM/Clang to PATH and run this again.
    popd
    exit /b 1
)

where lld-link >nul 2>nul
if errorlevel 1 (
    where ld.lld >nul 2>nul
    if errorlevel 1 (
        echo [NwC] ERROR: LLVM LLD was not found.
        echo [NwC] Expected lld-link.exe or ld.lld.exe in PATH.
        popd
        exit /b 1
    )
)

echo.
echo ========================================
echo           Building NwC compiler
echo ========================================
echo.

echo [1/2] Compiling and linking...

clang -std=c11 -O2 -Wall -Wextra -I"%ROOT%" ^
    "%ROOT%\compilerMain.c" ^
    "%ROOT%\lexer\lexer.c" ^
    "%ROOT%\parser\parser.c" ^
    "%ROOT%\codegen\codegen.c" ^
    -fuse-ld=lld ^
    -o "%NWC_OUT%"

if errorlevel 1 (
    echo.
    echo [NwC] BUILD FAILED.
    popd
    exit /b 1
)

echo.
echo [2/2] Checking output...

if not exist "%NWC_OUT%" (
    echo [NwC] ERROR: nwc.exe was not created.
    popd
    exit /b 1
)

for %%A in ("%NWC_OUT%") do set "NWC_SIZE=%%~zA"

echo.
echo ========================================
echo NwC build successful.
echo Output: %NWC_OUT%
echo Size  : %NWC_SIZE% bytes
echo ========================================
echo.

popd
exit /b 0
