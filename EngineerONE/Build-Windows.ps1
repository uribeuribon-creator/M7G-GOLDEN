param([string]$BuildRoot='')
$ErrorActionPreference='Stop'
$sourceRoot=Split-Path $PSScriptRoot -Parent
if(-not $BuildRoot){$BuildRoot=$sourceRoot}
Set-Location $BuildRoot
function Checked([scriptblock]$Command){&$Command;if($LASTEXITCODE -ne 0){throw "Command failed: $LASTEXITCODE"}}
$juce=Join-Path $sourceRoot 'JUCE7'
if(-not(Test-Path "$juce/CMakeLists.txt")){Checked{git clone --depth 1 --branch 7.0.12 https://github.com/juce-framework/JUCE.git $juce}}
if(Test-Path "$juce/.git"){$revision=(git -C $juce rev-parse HEAD).Trim()}else{$revision=(Get-Content (Join-Path $sourceRoot 'JUCE7_REVISION.txt') -Raw).Trim()}
if($revision -ne '4f43011b96eb0636104cb3e433894cda98243626'){throw 'Unexpected JUCE 7 revision'}
$ara=Join-Path $sourceRoot 'EngineerONE/ThirdParty/ARA_SDK'
if(-not(Test-Path "$ara/ARA_API/ARAInterface.h")){Checked{git clone --depth 1 --branch releases/2.2.0 --recurse-submodules --shallow-submodules https://github.com/Celemony/ARA_SDK.git $ara}}
if(Test-Path "$ara/.git"){if((git -C $ara rev-parse HEAD).Trim() -ne 'e723ce48a917b1ce2d7851c78e468f36b51ea151'){throw 'Unexpected ARA SDK revision'}}
Checked{cmake -S "$sourceRoot/EngineerONE" -B build-one-windows -A x64 -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded}
Checked{cmake --build build-one-windows --config Release -j 4}
$licenseDirectory=Join-Path $env:APPDATA 'M7G';New-Item -ItemType Directory -Force $licenseDirectory | Out-Null
if(-not(Test-Path "$licenseDirectory/M7G.license")){Copy-Item "$sourceRoot/Licensing/M7G.license" $licenseDirectory}
Checked{ctest --test-dir build-one-windows -C Release --output-on-failure --verbose}
$artifacts=Join-Path $BuildRoot 'build-one-windows/EngineerONE_artefacts/Release'
$binary=Join-Path $artifacts 'VST3/Makkine Engineer ONE - Studio One Edition.vst3/Contents/x86_64-win/Makkine Engineer ONE - Studio One Edition.vst3'
$bytes=[IO.File]::ReadAllBytes($binary);$at=[BitConverter]::ToInt32($bytes,0x3c)
if($bytes[0]-ne 0x4d -or $bytes[1]-ne 0x5a -or [BitConverter]::ToUInt16($bytes,$at+4)-ne 0x8664){throw 'VST3 is not AMD64 PE'}
$nsis=(Get-Command makensis.exe -ErrorAction SilentlyContinue).Source
if(-not $nsis){$nsis="${env:ProgramFiles(x86)}/NSIS/makensis.exe"}
New-Item -ItemType Directory -Force EngineerONE/dist | Out-Null
$output=Join-Path $BuildRoot 'EngineerONE/dist/Makkine-Engineer-ONE-Studio-One-Edition-Windows-x64.exe'
Checked{&$nsis "/DARTIFACT_DIR=$artifacts" "/DOUTPUT_FILE=$output" "$sourceRoot/EngineerONE/Installer/Windows.nsi"}
Get-FileHash -Algorithm SHA256 $output | Format-List
Write-Host "Created: $output"
