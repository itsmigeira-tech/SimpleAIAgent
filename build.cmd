@echo off
setlocal EnableDelayedExpansion

echo.
echo === Simple AI Agent Build ===
echo.

:: Must run from folder that contains CMakeLists.txt
if not exist "CMakeLists.txt" (
    echo ERROR: CMakeLists.txt not found.
    echo Run this script from the project root folder.
    echo Example: C:\Users\Migeira\Downloads\SimpleAIAgent-main\SimpleAIAgent-main
    pause
    exit /b 1
)

:: Check admin for installs
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Requesting Administrator rights for installs...
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

echo [1/4] Checking winget...
where winget >nul 2>&1
if %errorlevel% neq 0 (
    echo winget not found. Install App Installer from Microsoft Store, then run this again.
    pause
    exit /b 1
)

echo [2/4] Installing CMake if needed...
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    winget install -e --id Kitware.CMake --accept-package-agreements --accept-source-agreements
    set "PATH=%PATH%;%ProgramFiles%\CMake\bin"
) else (
    echo CMake already installed.
)

echo [3/4] Installing Visual Studio Build Tools if needed...
where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo Installing VS 2022 Build Tools. This takes several minutes...
    winget install -e --id Microsoft.VisualStudio.2022.BuildTools --accept-package-agreements --accept-source-agreements --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
) else (
    echo C++ compiler already available.
)

:: Refresh PATH for this session
set "PATH=%PATH%;%ProgramFiles%\CMake\bin"
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VSINSTALL=%%i"
if defined VSINSTALL (
    call "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
)

echo [4/4] Building...
if not exist build mkdir build
cd build

cmake .. -G "Visual Studio 17 2022" -A x64
if %errorlevel% neq 0 (
    echo CMake configure failed.
    echo Try opening "Developer Command Prompt for VS 2022" and run build.cmd from there.
    pause
    exit /b 1
)

cmake --build . --config Release
if %errorlevel% neq 0 (
    echo Build failed.
    pause
    exit /b 1
)

echo.
echo === Done ===
echo EXE: %cd%\Release\SimpleAIAgent.exe
echo.
if exist "Release\SimpleAIAgent.exe" (
    explorer /select,"%cd%\Release\SimpleAIAgent.exe"
)
pause
