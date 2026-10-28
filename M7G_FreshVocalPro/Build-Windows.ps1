param([string]$BuildRoot='', [switch]$AlsoBuildProjucer)
$ErrorActionPreference='Stop'
$sourceRoot=Split-Path $PSScriptRoot -Parent
if(-not $BuildRoot){$BuildRoot=$sourceRoot}
Set-Location $BuildRoot
function Checked([scriptblock]$Command){&$Command;if($LASTEXITCODE -ne 0){throw "Command failed: $LASTEXITCODE"}}
$juce=Join-Path $sourceRoot 'JUCE7'
if(-not(Test-Path "$juce/CMakeLists.txt")){Checked{git clone --depth 1 --branch 7.0.12 https://github.com/juce-framework/JUCE.git $juce}}
if(Test-Path "$juce/.git"){$revision=(git -C $juce rev-parse HEAD).Trim()}else{$revision=(Get-Content (Join-Path $sourceRoot 'JUCE7_REVISION.txt') -Raw).Trim()}
if($revision -ne '4f43011b96eb0636104cb3e433894cda98243626'){throw 'Unexpected JUCE 7 revision'}
$ara=Join-Path $sourceRoot 'M7G_FreshVocalPro/ThirdParty/ARA_SDK'
if(-not(Test-Path "$ara/ARA_API/ARAInterface.h")){Checked{git clone --depth 1 --branch releases/2.2.0 --recurse-submodules --shallow-submodules https://github.com/Celemony/ARA_SDK.git $ara}}
Checked{cmake -S "$sourceRoot/M7G_FreshVocalPro" -B build-fresh-windows -A x64 -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded}
Checked{cmake --build build-fresh-windows --config Release -j 4}
$licenseDirectory=Join-Path $env:APPDATA 'M7G';New-Item -ItemType Directory -Force $licenseDirectory | Out-Null
if(-not(Test-Path "$licenseDirectory/M7G.license")){Copy-Item "$sourceRoot/Licensing/M7G.license" $licenseDirectory}
$env:M7G_DISABLE_OPENGL='1'
Checked{ctest --test-dir build-fresh-windows -C Release --output-on-failure --verbose}
if($AlsoBuildProjucer){
 $solution=Join-Path $sourceRoot 'M7G_FreshVocalPro/Builds/VisualStudio2022/M7G Fresh Vocal Pro Beast.sln'
 if(-not(Test-Path $solution)){Checked{python "$sourceRoot/M7G_FreshVocalPro/Tools/prepare-projucer.py"}}
 Checked{MSBuild $solution /p:Configuration=Release /p:Platform=x64 /m:4 /nologo /verbosity:minimal}
 $jucerBinary=Join-Path (Split-Path $solution -Parent) 'x64/Release/VST3/M7G Fresh Vocal Pro Beast.vst3/Contents/x86_64-win/M7G Fresh Vocal Pro Beast.vst3'
 if(-not(Test-Path $jucerBinary)){throw "Projucer VST3 missing: $jucerBinary"}
 Checked{& ./build-fresh-windows/Release/FreshVST3Load.exe $jucerBinary}
}
$artifacts=Join-Path $BuildRoot 'build-fresh-windows/FreshVocal_artefacts/Release'
$binary=Join-Path $artifacts 'VST3/M7G Fresh Vocal Pro Beast.vst3/Contents/x86_64-win/M7G Fresh Vocal Pro Beast.vst3'
$bytes=[IO.File]::ReadAllBytes($binary);$at=[BitConverter]::ToInt32($bytes,0x3c)
if($bytes[0]-ne 0x4d -or $bytes[1]-ne 0x5a -or [BitConverter]::ToUInt16($bytes,$at+4)-ne 0x8664){throw 'VST3 is not AMD64 PE'}
$nsis=(Get-Command makensis.exe -ErrorAction SilentlyContinue).Source
if(-not $nsis){$nsis="${env:ProgramFiles(x86)}/NSIS/makensis.exe"}
New-Item -ItemType Directory -Force M7G_FreshVocalPro/dist | Out-Null
$output=Join-Path $BuildRoot 'M7G_FreshVocalPro/dist/M7G-Fresh-Vocal-Pro-Beast-Windows-x64.exe'
Checked{&$nsis "/DARTIFACT_DIR=$artifacts" "/DOUTPUT_FILE=$output" "$sourceRoot/M7G_FreshVocalPro/Installer/Windows.nsi"}
Get-FileHash -Algorithm SHA256 $output | Format-List
Write-Host "Created: $output"
