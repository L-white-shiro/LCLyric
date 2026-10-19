@echo off
setlocal enabledelayedexpansion

rem ============================================================
rem  LCLyric (untitled1) one-click Release deploy script
rem
rem  Usage:
rem    deploy.bat [output_dir]
rem      output_dir defaults to D:\deploy\LCLyric
rem
rem  Requirements:
rem    Run from "x64 Native Tools Command Prompt for VS 2022"
rem    or from Qt Creator's MSVC kit terminal (needs cl.exe).
rem
rem  Override Qt path if needed:
rem    set QT_ROOT=D:\Software\CS\QT\6.12.0\msvc2022_64
rem ============================================================

if "%QT_ROOT%"=="" set "QT_ROOT=D:\Software\CS\QT\6.12.0\msvc2022_64"
set "QT_TOOLS=%QT_ROOT%\..\Tools"
set "WINDEVEVT=%QT_ROOT%\bin\windeployqt.exe"
set "NINJA=%QT_TOOLS%\Ninja\ninja.exe"
set "PROJ_DIR=%~dp0"
set "BUILD_DIR=%PROJ_DIR%build\release"
set "DEPLOY_DIR=%~1"
if "%DEPLOY_DIR%"=="" set "DEPLOY_DIR=D:\deploy\LCLyric"

rem ---------- sanity checks ----------
where cl.exe >nul 2>nul
if errorlevel 1 (
    echo [ERROR] cl.exe not found on PATH.
    echo         Run this from "x64 Native Tools Command Prompt for VS 2022"
    echo         or from Qt Creator's MSVC kit terminal.
    exit /b 1
)
if not exist "%WINDEVEVT%" (
    echo [ERROR] windeployqt not found: %WINDEVEVT%
    exit /b 1
)
if not exist "%NINJA%" (
    echo [ERROR] ninja not found: %NINJA%
    exit /b 1
)

rem ---------- 1. configure + build Release ----------
echo [1/4] Configuring Release build in %BUILD_DIR%
cmake -B "%BUILD_DIR%" -S "%PROJ_DIR%" -G Ninja -DCMAKE_MAKE_PROGRAM="%NINJA%" -DCMAKE_PREFIX_PATH="%QT_ROOT%" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo [ERROR] cmake configure failed.
    exit /b 1
)

echo [2/4] Building Release ...
cmake --build "%BUILD_DIR%" --config Release
if errorlevel 1 (
    echo [ERROR] Release build failed.
    exit /b 1
)
if not exist "%BUILD_DIR%\untitled1.exe" (echo [ERROR] Build output missing: %BUILD_DIR%\untitled1.exe & exit /b 1)
if not exist "%BUILD_DIR%\smtc_probe.exe" (echo [ERROR] Build output missing: %BUILD_DIR%\smtc_probe.exe & exit /b 1)

rem ---------- 2. prepare deploy directory ----------
echo [3/4] Preparing deploy directory: %DEPLOY_DIR%
if exist "%DEPLOY_DIR%" rmdir /s /q "%DEPLOY_DIR%"
mkdir "%DEPLOY_DIR%"
if errorlevel 1 (
    echo [ERROR] cannot create %DEPLOY_DIR%
    exit /b 1
)
copy /y "%BUILD_DIR%\untitled1.exe" "%DEPLOY_DIR%\" >nul
copy /y "%BUILD_DIR%\smtc_probe.exe" "%DEPLOY_DIR%\" >nul

rem ---------- 3. windeployqt ----------
echo [4/4] Running windeployqt ...
"%WINDEVEVT%" --release --compiler-runtime "%DEPLOY_DIR%\untitled1.exe"
if errorlevel 1 (
    echo [ERROR] windeployqt failed on untitled1.exe
    exit /b 1
)
"%WINDEVEVT%" --release --compiler-runtime "%DEPLOY_DIR%\smtc_probe.exe"
if errorlevel 1 (
    echo [ERROR] windeployqt failed on smtc_probe.exe
    exit /b 1
)

rem ---------- optional: ship vcredist installer instead ----------
rem If you prefer users to run the VC++ redist installer instead of
rem app-local runtime DLLs: remove --compiler-runtime from the two
rem windeployqt lines above, then uncomment the next line:
rem copy /y "D:\Software\CS\QT\vcredist\vc14.50.35719_VC_redist.x64.exe" "%DEPLOY_DIR%\"

echo.
echo Done. Deploy directory:
echo   %DEPLOY_DIR%
echo Test on a machine without Qt before shipping.
pause
