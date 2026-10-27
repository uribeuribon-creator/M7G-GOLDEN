$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
$setup = Join-Path $PWD 'dist/M7G-ALIEN-VOICE-V11-ALIEN-INTELLIGENCE-Windows-x64.exe'
$module = 'M7G ALIEN VOICE V11 - ALIEN INTELLIGENCE.vst3/Contents/x86_64-win/M7G ALIEN VOICE V11 - ALIEN INTELLIGENCE.vst3'
$builtModule = Join-Path $PWD "build-windows/M7G_Golden_Voice_AI_artefacts/Release/VST3/$module"
$vstRoot = Join-Path $env:CommonProgramFiles 'VST3'
$installedModule = Join-Path $vstRoot $module
$entry = 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\M7GAlienVoiceV11'
# Disposable CI fixtures exercise the two previous product names.
$legacy = @('M7G Golden Voice AI.vst3', 'MAKKINE VOICE 7.vst3', 'M7G ALIEN VOICE.vst3', 'M7G ALIEN VOICE V10 TRIPLE GOD ORIGIN.vst3')
foreach ($name in $legacy) {
    $path = Join-Path $vstRoot $name
    if (Test-Path $path) { throw "Unexpected pre-existing fixture: $path" }
    New-Item -ItemType Directory -Force $path | Out-Null
    Set-Content (Join-Path $path 'migration-test.txt') 'CI migration fixture'
}
$process = Start-Process -FilePath $setup -ArgumentList '/S' -PassThru
if (-not $process.WaitForExit(60000)) { $process.Kill(); throw 'Installer did not finish within 60 seconds' }
if ($process.ExitCode -ne 0) { throw "Installer exit code: $($process.ExitCode)" }
if (-not (Test-Path $installedModule)) { throw "Missing installed VST3: $installedModule" }
if ((Get-FileHash $builtModule).Hash -ne (Get-FileHash $installedModule).Hash) { throw 'Installed VST3 differs from the tested build' }
foreach ($name in $legacy) { if (-not (Test-Path (Join-Path $vstRoot $name))) { throw "Earlier product was removed: $name" } }
$globalLicense = Join-Path $env:ProgramData 'M7G/M7G.license'
if (-not (Test-Path $globalLicense)) { throw 'Installer did not provide the global licence' }
# A valid existing file (extra JSON whitespace) must survive reinstall.
Add-Content -Path $globalLicense -Value ' '
$preservedHash = (Get-FileHash $globalLicense).Hash
$again = Start-Process -FilePath $setup -ArgumentList '/S' -PassThru
if (-not $again.WaitForExit(60000) -or $again.ExitCode -ne 0) { throw 'Reinstall failed' }
if ((Get-FileHash $globalLicense).Hash -ne $preservedHash) { throw 'Existing global licence was overwritten' }
$registration = Get-ItemProperty $entry
if ($registration.DisplayName -ne 'M7G ALIEN VOICE V11 - ALIEN INTELLIGENCE' -or $registration.DisplayVersion -ne '11.0.0') { throw 'Unexpected uninstall registration' }
& ./build-windows/Release/M7GVST3Load.exe $installedModule
if ($LASTEXITCODE -ne 0) { throw "Installed VST3 load failed: $LASTEXITCODE" }
$uninstaller = $registration.UninstallString.Trim('"')
if (-not (Test-Path $uninstaller)) { throw "Uninstaller not found: $uninstaller" }
$process = Start-Process -FilePath $uninstaller -ArgumentList '/S' -PassThru
if (-not $process.WaitForExit(60000)) { $process.Kill(); throw 'Uninstaller did not finish within 60 seconds' }
for ($attempt=0; $attempt -lt 75 -and ((Test-Path $installedModule) -or (Test-Path $entry)); $attempt++) { Start-Sleep -Milliseconds 200 }
if ((Test-Path $installedModule) -or (Test-Path $entry)) { throw 'Uninstall did not remove the VST3 and registration' }
if (-not (Test-Path $globalLicense)) { throw 'Uninstall removed the perpetual licence' }
foreach ($name in $legacy) { Remove-Item -Recurse -Force (Join-Path $vstRoot $name) }
Write-Host 'PASS: native V11 installation, previous products preserved, installed VST3 load and uninstall'
