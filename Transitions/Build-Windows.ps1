$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
Set-Location $repo
function Checked([scriptblock]$Command) { & $Command; if ($LASTEXITCODE -ne 0) { throw "Build failed: $LASTEXITCODE" } }
if (-not (Test-Path JUCE7/CMakeLists.txt)) { Checked { git clone --depth 1 --branch 7.0.12 https://github.com/juce-framework/JUCE.git JUCE7 } }
$revision = (git -C JUCE7 rev-parse HEAD).Trim()
if ($revision -ne '4f43011b96eb0636104cb3e433894cda98243626') { throw "Unexpected JUCE 7 revision: $revision" }
Checked { cmake -S Transitions -B build-transitions-windows -A x64 -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded }
Checked { cmake --build build-transitions-windows --config Release -j 4 }
$licenseDirectory = Join-Path $env:APPDATA 'M7G'
New-Item -ItemType Directory -Force $licenseDirectory | Out-Null
$licence = Join-Path $licenseDirectory 'M7G.license'
if (-not (Test-Path $licence)) { Copy-Item Licensing/M7G.license $licence }
Checked { ctest --test-dir build-transitions-windows -C Release --output-on-failure --verbose }
$artifacts = Join-Path $repo 'build-transitions-windows/M7G_Transitions_artefacts/Release'
$binary = Join-Path $artifacts 'VST3/M7G ALIEN TRANSITIONS GOD V1.vst3/Contents/x86_64-win/M7G ALIEN TRANSITIONS GOD V1.vst3'
$bytes = [IO.File]::ReadAllBytes($binary)
$offset = [BitConverter]::ToInt32($bytes,0x3c)
if ($bytes[0] -ne 0x4d -or $bytes[1] -ne 0x5a -or [BitConverter]::ToUInt32($bytes,$offset) -ne 0x4550 -or [BitConverter]::ToUInt16($bytes,$offset+4) -ne 0x8664) { throw 'VST3 is not Windows AMD64 PE' }
$nsis = (Get-Command makensis.exe -ErrorAction SilentlyContinue).Source
if (-not $nsis) { $nsis = "${env:ProgramFiles(x86)}/NSIS/makensis.exe" }
New-Item -ItemType Directory -Force Transitions/dist | Out-Null
$output = Join-Path $repo 'Transitions/dist/M7G-ALIEN-TRANSITIONS-GOD-V1-Windows-x64.exe'
Checked { & $nsis "/DARTIFACT_DIR=$artifacts" "/DOUTPUT_FILE=$output" "$PSScriptRoot/Installer/Windows.nsi" }
Get-FileHash -Algorithm SHA256 $output | Format-List
Write-Host "Created: $output"
