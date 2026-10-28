$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
Set-Location $repo
function Checked([scriptblock]$Command){&$Command;if($LASTEXITCODE -ne 0){throw "Command failed: $LASTEXITCODE"}}
if(-not(Test-Path JUCE7/CMakeLists.txt)){Checked{git clone --depth 1 --branch 7.0.12 https://github.com/juce-framework/JUCE.git JUCE7}}
if((git -C JUCE7 rev-parse HEAD).Trim() -ne '4f43011b96eb0636104cb3e433894cda98243626'){throw 'Unexpected JUCE 7 revision'}
& "$PSScriptRoot/Tools/Fetch-Dependencies.ps1"
Checked{cmake -S Tune -B build-tune-windows -A x64 -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded}
Checked{cmake --build build-tune-windows --config Release -j 4}
# ORT requires the VC++ runtime. Distribute Microsoft's app-local redistributable
# DLLs from the licensed VS toolchain; no runtime installer or Internet needed.
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=(& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath).Trim()
$crt=Get-ChildItem "$vs/VC/Redist/MSVC" -Directory | Sort-Object FullName -Descending | ForEach-Object { Join-Path $_.FullName 'x64/Microsoft.VC143.CRT' } | Where-Object { Test-Path $_ } | Select-Object -First 1
if(-not $crt){throw 'Visual C++ app-local redistributables not found'}
$payload=Join-Path $repo 'build-tune-windows/M7G_Tune_artefacts/Release/VST3/M7G ALIEN TUNE GOD V1.vst3/Contents/x86_64-win'
Copy-Item "$crt/*.dll" $payload
Copy-Item "$crt/*.dll" 'build-tune-windows/Release'
$licenseDirectory=Join-Path $env:APPDATA 'M7G';New-Item -ItemType Directory -Force $licenseDirectory | Out-Null
if(-not(Test-Path (Join-Path $licenseDirectory 'M7G.license'))){Copy-Item Licensing/M7G.license $licenseDirectory}
Checked{ctest --test-dir build-tune-windows -C Release --output-on-failure --verbose}
$artifacts=Join-Path $repo 'build-tune-windows/M7G_Tune_artefacts/Release'
foreach($binary in @((Join-Path $payload 'M7G ALIEN TUNE GOD V1.vst3'),(Join-Path $payload 'onnxruntime.dll'))){
 $bytes=[IO.File]::ReadAllBytes($binary);$at=[BitConverter]::ToInt32($bytes,0x3c)
 if($bytes[0]-ne 0x4d -or $bytes[1]-ne 0x5a -or [BitConverter]::ToUInt16($bytes,$at+4)-ne 0x8664){throw "Not AMD64 PE: $binary"}
}
$nsis=(Get-Command makensis.exe -ErrorAction SilentlyContinue).Source
if(-not $nsis){$nsis="${env:ProgramFiles(x86)}/NSIS/makensis.exe"}
New-Item -ItemType Directory -Force Tune/dist | Out-Null
$output=Join-Path $repo 'Tune/dist/M7G-ALIEN-TUNE-GOD-V1-Windows-x64.exe'
Checked{&$nsis "/DARTIFACT_DIR=$artifacts" "/DOUTPUT_FILE=$output" "$PSScriptRoot/Installer/Windows.nsi"}
Get-FileHash -Algorithm SHA256 $output | Format-List
Write-Host "Created: $output"
