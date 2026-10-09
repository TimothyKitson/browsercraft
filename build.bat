@echo off
REM Convenience build script: finds MSVC, configures with CMake + Ninja, and
REM builds. Run from the project root:  build.bat   (.\build.bat in PowerShell)
setlocal enabledelayedexpansion

REM vswhere ships with every Visual Studio 2017 and later installer and
REM always sits at this path, whatever edition or version is installed.
set "VS="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "!VSWHERE!" for /f "usebackq tokens=*" %%i in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS=%%i"

if not defined VS set "VS=C:\Program Files\Microsoft Visual Studio\18\Community"

if not exist "!VS!\VC\Auxiliary\Build\vcvars64.bat" goto :nogit
echo.
echo Could not find git, which CMake needs to fetch SDL2.
echo.
echo Install it with:  winget install --id Git.Git -e
echo then open a new terminal so git is on PATH.
echo.
exit /b 1

:nocompiler

REM CMake fetches SDL2, glm and stb by cloning them, so git has to be on
REM PATH before any of the rest of this is worth trying.
where git >nul 2>&1
if errorlevel 1 goto :nogit

call "!VS!\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

set "CMAKE=!VS!\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "NINJA=!VS!\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
if not exist "!CMAKE!" set "CMAKE=cmake.exe"
if not exist "!NINJA!" set "NINJA=ninja.exe"

REM Checking for the generated build file rather than the folder: a
REM configure that failed half way still leaves the folder behind, and
REM testing for that alone would skip straight to a build with nothing
REM to build.
if not exist build\build.ninja "!CMAKE!" -S . -B build -G Ninja -DCMAKE_MAKE_PROGRAM="!NINJA!" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1

"!CMAKE!" --build build %*
exit /b %errorlevel%

:nogit
echo.
echo Could not find git, which CMake needs to fetch SDL2.
echo.
echo Install it with:  winget install --id Git.Git -e
echo then open a new terminal so git is on PATH.
echo.
exit /b 1

:nocompiler
echo.
echo Could not find Visual Studio with the C++ tools.
echo.
echo Install Visual Studio Community from
echo   https://visualstudio.microsoft.com/downloads/
echo and tick "Desktop development with C++" in the installer.
echo.
exit /b 1
