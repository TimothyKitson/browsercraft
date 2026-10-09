@echo off
REM Convenience build script: finds MSVC, configures with CMake + Ninja, and
REM builds. Run from the project root:  build.bat   (.\build.bat in PowerShell)
setlocal enabledelayedexpansion

REM --- Visual Studio -------------------------------------------------------
REM vswhere ships with every Visual Studio 2017 and later installer and
REM always sits at this path, whatever edition or version is installed.
set "VS="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "!VSWHERE!" for /f "usebackq tokens=*" %%i in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS=%%i"

if not defined VS set "VS=C:\Program Files\Microsoft Visual Studio\18\Community"
if not exist "!VS!\VC\Auxiliary\Build\vcvars64.bat" goto :nocompiler

REM --- git -----------------------------------------------------------------
REM CMake fetches SDL2, glm and stb by cloning them, so git has to be
REM reachable. Looking only at PATH gets this wrong: the Windows installer
REM does not always put git there, and CMake finds it anyway by searching
REM the usual folders. So those are searched here too, and only a machine
REM with no git at all is turned away.
where git >nul 2>&1
if not errorlevel 1 goto :gotgit

call :trygit "%ProgramFiles%\Git\cmd"
if defined GITDIR goto :addgit
call :trygit "%ProgramFiles(x86)%\Git\cmd"
if defined GITDIR goto :addgit
call :trygit "%LOCALAPPDATA%\Programs\Git\cmd"
if defined GITDIR goto :addgit
call :trygit "%ProgramW6432%\Git\cmd"
if defined GITDIR goto :addgit
goto :nogit

:addgit
set "PATH=!GITDIR!;!PATH!"
echo Using git from !GITDIR!

:gotgit

REM --- configure and build -------------------------------------------------
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

:trygit
set "GITDIR="
if exist "%~1\git.exe" set "GITDIR=%~1"
exit /b 0

:nogit
echo.
echo Could not find git, which CMake needs to fetch SDL2.
echo.
echo Looked on PATH and in:
echo   %ProgramFiles%\Git\cmd
echo   %ProgramFiles(x86)%\Git\cmd
echo   %LOCALAPPDATA%\Programs\Git\cmd
echo.
echo Install it with:  winget install --id Git.Git -e
echo then open a new terminal.
echo.
echo If git is installed somewhere else, put that folder on PATH first:
echo   $env:Path += ";C:\path\to\Git\cmd"
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
