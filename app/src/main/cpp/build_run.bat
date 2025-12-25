@echo off
setlocal

:: Try Visual Studio installations (18/Preview, 2022, 2019)
if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
) else (
    echo Error: Visual Studio not found
    exit /b 1
)

cd /d "%~dp0"

if not exist "build" mkdir build

echo Building benchmark...
cl /nologo /EHsc /std:c++17 /O2 /Fe:build\benchmark.exe main.cpp

if %ERRORLEVEL% neq 0 (
    echo Build failed
    exit /b 1
)

echo.
echo Running benchmark...
build\benchmark.exe --once
