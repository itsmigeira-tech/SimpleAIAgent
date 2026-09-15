@echo off
setlocal EnableDelayedExpansion

echo.
echo === Simple AI Agent Build ===
echo.

cd /d "%~dp0"

:: Find project root
if exist "CMakeLists.txt" (
    set "ROOT=%cd%"
) else if exist "SimpleAIAgent-main\CMakeLists.txt" (
    cd SimpleAIAgent-main
) else if exist "SimpleAIAgent\CMakeLists.txt" (
    cd SimpleAIAgent
) else (
    for /d %%D in (*) do (
        if exist "%%D\CMakeLists.txt" (
            cd "%%D"
            goto :found
        )
    )
    echo ERROR: CMakeLists.txt not found.
    echo Current: %cd%
    dir /b
    pause
    exit /b 1
)
:found
echo Project: %cd%

:: Detect tools without elevating first
set "NEED_CMAKE=0"
set "NEED_VS=0"
where cmake >nul 2>&1 || set "NEED_CMAKE=1"
where cl >nul 2>&1
if errorlevel 1 (
    if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
        for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VSINSTALL=%%i"
    )
    if not defined VSINSTALL set "NEED_VS=1"
)

:: Only elevate when an install is required
if "%NEED_CMAKE%"=="1" goto :need_admin
if "%NEED_VS%"=="1" goto :need_admin
goto :build

:need_admin
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Need Administrator to install missing tools...
    powershell -Command "Start-Process -FilePath '%~f0' -WorkingDirectory '%cd%' -Verb RunAs"
    exit /b
)

where winget >nul 2>&1
if errorlevel 1 (
    echo winget missing. Install App Installer from Microsoft Store.
    pause
    exit /b 1
)

if "%NEED_CMAKE%"=="1" (
    echo Installing CMake...
    winget install -e --id Kitware.CMake --accept-package-agreements --accept-source-agreements --disable-interactivity
    set "PATH=%PATH%;%ProgramFiles%\CMake\bin"
)

if "%NEED_VS%"=="1" (
    echo Installing VS 2022 Build Tools (long first run)...
    winget install -e --id Microsoft.VisualStudio.2022.BuildTools --accept-package-agreements --accept-source-agreements --disable-interactivity --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
)

:build
set "PATH=%PATH%;%ProgramFiles%\CMake\bin"
if not defined VSINSTALL (
    if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
        for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VSINSTALL=%%i"
    )
)
if defined VSINSTALL call "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

where cmake >nul 2>&1
if errorlevel 1 (
    echo CMake still not found. Open a new terminal and run build.cmd again.
    pause
    exit /b 1
)

if not exist build mkdir build
cd build

:: Configure only when needed
if not exist CMakeCache.txt (
    echo Configuring...
    cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_CONFIGURATION_TYPES=Release
    if errorlevel 1 (
        echo Configure failed.
        pause
        exit /b 1
    )
) else (
    echo Already configured. Skipping cmake configure.
)

echo Building Release (parallel)...
set CMAKE_BUILD_PARALLEL_LEVEL=%NUMBER_OF_PROCESSORS%
cmake --build . --config Release --parallel %NUMBER_OF_PROCESSORS%
if errorlevel 1 (
    echo Build failed.
    pause
    exit /b 1
)

echo.
echo === Done ===
echo EXE: %cd%\Release\SimpleAIAgent.exe
if exist "Release\SimpleAIAgent.exe" explorer /select,"%cd%\Release\SimpleAIAgent.exe"
echo.
pause
