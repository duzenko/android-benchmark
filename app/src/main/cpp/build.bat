@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
cd /d "%~dp0"
if not exist build mkdir build
cl /EHsc /std:c++17 /O2 /Fe:build\benchmark.exe main.cpp
