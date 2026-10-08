@echo off
REM Convenience build script: sets up the MSVC environment, configures with
REM CMake + Ninja, and builds. Run from the project root:  build.bat
setlocal
set VS=C:\Program Files\Microsoft Visual Studio\18\Community
call "%VS%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
set CMAKE=%VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe
set NINJA=%VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe

if not exist build (
    "%CMAKE%" -S . -B build -G Ninja -DCMAKE_MAKE_PROGRAM="%NINJA%" -DCMAKE_BUILD_TYPE=Release
    if errorlevel 1 exit /b 1
)
"%CMAKE%" --build build %*
exit /b %errorlevel%
