@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem ============================================================
rem NwDev build script
rem
rem Expected layout:
rem   NwOS/
rem     compiler/
rem       builded/nwc.exe
rem     NwDev/
rem       build.bat
rem       *.nw
rem       *.nwo       <- generated beside the .nw source
rem
rem Usage:
rem   build.bat              -> compile every *.nw in NwDev
rem   build.bat file.nw      -> compile only file.nw
rem   build.bat clean        -> delete generated *.nwo files
rem ============================================================

pushd "%~dp0"

set "NwDEV_ROOT=%CD%"
set "NWC=%NwDEV_ROOT%\..\compiler\builded\nwc.exe"

if not exist "%NWC%" (
    echo.
    echo [NwDev] ERROR: NwC compiler was not found:
    echo         %NWC%
    echo.
    echo Build the compiler first:
    echo         ..\compiler\build.bat
    echo.
    popd
    exit /b 1
)

if /I "%~1"=="clean" goto :clean

if not "%~1"=="" goto :single

goto :all

:single
set "INPUT=%~1"

if not exist "%INPUT%" (
    echo.
    echo [NwDev] ERROR: source file not found:
    echo         %INPUT%
    echo.
    popd
    exit /b 1
)

for %%A in ("%INPUT%") do (
    set "INPUT_FULL=%%~fA"
    set "OUTPUT=%%~dpnA.nwo"
)

echo.
echo ========================================
echo              NwDev compiler
echo ========================================
echo Input : !INPUT_FULL!
echo Output: !OUTPUT!
echo ========================================
echo.

"%NWC%" "!INPUT_FULL!" -o "!OUTPUT!"
set "RESULT=!ERRORLEVEL!"

if not "!RESULT!"=="0" (
    echo.
    echo [NwDev] Compilation FAILED.
    popd
    exit /b !RESULT!
)

echo.
echo [NwDev] Compilation successful.
echo.

popd
exit /b 0

:all
set /a TOTAL=0
set /a FAILED=0

for %%F in (*.nw) do (
    set /a TOTAL+=1
    set "INPUT=%%~fF"
    set "OUTPUT=%%~dpnF.nwo"

    echo.
    echo ----------------------------------------
    echo Compiling: %%~nxF
    echo ----------------------------------------

    "%NWC%" "!INPUT!" -o "!OUTPUT!"

    if errorlevel 1 (
        set /a FAILED+=1
        echo [NwDev] FAILED: %%~nxF
    ) else (
        echo [NwDev] OK: %%~nxF
    )
)

if %TOTAL%==0 (
    echo.
    echo [NwDev] No .nw files found in:
    echo         %NwDEV_ROOT%
    echo.
    popd
    exit /b 1
)

echo.
echo ========================================
echo              NwDev summary
echo ========================================
echo Total : %TOTAL%
echo Failed: %FAILED%
echo ========================================
echo.

if %FAILED% GTR 0 (
    popd
    exit /b 1
)

popd
exit /b 0

:clean
echo.
echo ========================================
echo             Cleaning NwDev
echo ========================================

set /a REMOVED=0
for %%F in (*.nwo) do (
    del /q "%%~fF"
    if not exist "%%~fF" set /a REMOVED+=1
)

echo.
echo Removed: %REMOVED% .nwo file(s).
echo.

popd
exit /b 0
