@echo off
setlocal
cd /d "%~dp0"
where cmake >nul 2>nul || (echo Install CMake 3.22+ and Visual Studio 2022 Desktop development with C++. & exit /b 1)
if not exist JUCE\CMakeLists.txt (
 git clone --depth 1 --branch 8.0.6 https://github.com/juce-framework/JUCE.git JUCE
 if errorlevel 1 exit /b 1
)
cmake -S . -B build -A x64
if errorlevel 1 exit /b 1
cmake --build build --config Release -j 4
if errorlevel 1 exit /b 1
ctest --test-dir build -C Release --output-on-failure
if errorlevel 1 exit /b 1
echo VST3: build\M7G_Golden_Voice_AI_artefacts\Release\VST3\
