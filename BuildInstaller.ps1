$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
function Checked([scriptblock]$Command) {
    & $Command
    if ($LASTEXITCODE -ne 0) { throw "Error de compilacion: $LASTEXITCODE" }
}
if (-not (Test-Path JUCE/CMakeLists.txt)) {
    Checked { git clone --depth 1 --branch 8.0.6 https://github.com/juce-framework/JUCE.git JUCE }
}
Checked { cmake -S . -B build-windows -A x64 -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded }
Checked { cmake --build build-windows --config Release -j 4 }
Checked { ctest --test-dir build-windows -C Release --output-on-failure --verbose }
$artefacts = Join-Path $PSScriptRoot 'build-windows/M7G_Golden_Voice_AI_artefacts/Release'
$binary = Join-Path $artefacts 'VST3/M7G ALIEN VOICE V10 TRIPLE GOD ORIGIN.vst3/Contents/x86_64-win/M7G ALIEN VOICE V10 TRIPLE GOD ORIGIN.vst3'
if (-not (Test-Path $binary)) { throw "Falta el binario VST3 Windows x64: $binary" }
# Validate the PE signature and target machine before packaging.
$bytes = [IO.File]::ReadAllBytes($binary)
$peOffset = [BitConverter]::ToInt32($bytes, 0x3c)
if ($bytes[0] -ne 0x4d -or $bytes[1] -ne 0x5a -or [BitConverter]::ToUInt32($bytes,$peOffset) -ne 0x4550 -or [BitConverter]::ToUInt16($bytes,$peOffset+4) -ne 0x8664) { throw 'El plugin no es un PE x64 valido' }
$nsis = (Get-Command makensis.exe -ErrorAction SilentlyContinue).Source
if (-not $nsis) { $nsis = "${env:ProgramFiles(x86)}/NSIS/makensis.exe" }
if (-not (Test-Path $nsis)) { throw 'Instala NSIS 3 para generar el instalador.' }
New-Item -ItemType Directory -Force dist | Out-Null
$output = Join-Path $PSScriptRoot 'dist/M7G-ALIEN-VOICE-V10-TRIPLE-GOD-ORIGIN-Windows-x64.exe'
Checked { & $nsis "/DARTIFACT_DIR=$artefacts" "/DOUTPUT_FILE=$output" Installer/Windows.nsi }
Get-FileHash -Algorithm SHA256 $output | Format-List
Write-Host "Instalador creado: $output"
