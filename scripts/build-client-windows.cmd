@echo off
rem Builds the Windows client in the current git worktree with the CMake presets: scripts\build-client-windows.cmd [x64|x86]
rem Run it from the worktree's root. The first build in a worktree takes 15-20 minutes, later ones only recompile what changed.
rem Install the result into a client folder with: scripts\update-live-client.ps1 -Target <client folder> -LocalBuild out\build\windows-x64\src\Release
setlocal
set "ARCH=%~1"
if "%ARCH%"=="" set "ARCH=x64"

rem Find Visual Studio with the C++ tools (any edition) and activate its build environment.
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" ( echo Visual Studio is not installed ^(vswhere.exe is missing^). & exit /b 1 )
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%i"
if "%VSINSTALL%"=="" ( echo Visual Studio with the "Desktop development with C++" workload was not found. & exit /b 1 )
call "%VSINSTALL%\VC\Auxiliary\Build\vcvarsall.bat" %ARCH% >nul
if errorlevel 1 exit /b 1

rem vcpkg: use VCPKG_ROOT if it is set, otherwise the copy that comes with Visual Studio.
if "%VCPKG_ROOT%"=="" set "VCPKG_ROOT=%VSINSTALL%\VC\vcpkg"

echo === configure windows-%ARCH% ===
cmake --preset windows-%ARCH% -DBUILD_TESTING=OFF
if errorlevel 1 exit /b 1
echo === build ===
cmake --build --preset windows-%ARCH%-release
if errorlevel 1 exit /b 1
echo === BUILD DONE: out\build\windows-%ARCH%\src\Release\Main.exe ===
