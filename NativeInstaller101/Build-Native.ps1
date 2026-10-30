param([switch]$SkipInstallValidation)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if ([Environment]::OSVersion.Platform -ne [PlatformID]::Win32NT -or
    -not [Environment]::Is64BitOperatingSystem -or
    -not [Environment]::Is64BitProcess) {
    throw 'This build requires native Windows x64 and 64-bit PowerShell.'
}

$root = $PSScriptRoot
$dist = Join-Path $root 'deliverables'
$evidence = Join-Path $root 'validation'
New-Item -ItemType Directory -Force $dist, $evidence | Out-Null
$inputs = Get-Content (Join-Path $root 'BuildInputs.json') -Raw | ConvertFrom-Json
$zip = Join-Path $root 'Inputs.zip'
if ((Get-FileHash $zip -Algorithm SHA256).Hash.ToLowerInvariant() -ne $inputs.inputs_zip_sha256) {
    throw 'Build input archive checksum mismatch.'
}
Expand-Archive -LiteralPath $zip -DestinationPath $root -Force

$compilerZip = Join-Path $env:RUNNER_TEMP 'InnoSetup-6.2.2.zip'
Invoke-WebRequest -Uri 'https://api.nuget.org/v3-flatcontainer/tools.innosetup/6.2.2/tools.innosetup.6.2.2.nupkg' -OutFile $compilerZip
if ((Get-FileHash $compilerZip -Algorithm SHA256).Hash.ToLowerInvariant() -ne $inputs.compiler_package_sha256) {
    throw 'Inno Setup 6.2.2 package checksum mismatch.'
}
$compilerDir = Join-Path $env:RUNNER_TEMP 'InnoSetup-6.2.2'
Expand-Archive -LiteralPath $compilerZip -DestinationPath $compilerDir -Force
$iscc = Join-Path $compilerDir 'tools\ISCC.exe'
$compilerVersion = '6.2.2'

$bundle = 'M7G Fresh Vocal Pro Beast.vst3'
$moduleRelative = "Contents\x86_64-win\$bundle"
$original = Join-Path $root "Payload\$bundle\$moduleRelative"
if ((Get-FileHash $original -Algorithm SHA256).Hash.ToLowerInvariant() -ne $inputs.vst3_sha256) {
    throw 'The original compiled VST3 checksum changed.'
}

$filename = 'M7G-Fresh-Vocal-Pro-Beast-Installer-v1.0.1-Windows-x64.exe'
$iss = Join-Path $root 'M7G-Fresh-Vocal-Pro-Beast-Installer-v1.0.1-Windows-x64.iss'
& $iscc "/DOutputRoot=$dist" $iss 2>&1 | Tee-Object (Join-Path $evidence 'iscc.log')
if ($LASTEXITCODE -ne 0) { throw "ISCC failed with exit code $LASTEXITCODE" }
$compileLog = Get-Content (Join-Path $evidence 'iscc.log') -Raw
if ($compileLog -notmatch 'Compiler engine version: Inno Setup 6\.2\.2(?:\r?\n|$)') {
    throw 'The compiler engine did not identify itself as Inno Setup 6.2.2.'
}
$installer = Join-Path $dist $filename
$versionInfo = (Get-Item $installer).VersionInfo
$metadata = [ordered]@{ FileDescription=$versionInfo.FileDescription; CompanyName=$versionInfo.CompanyName; FileVersion=$versionInfo.FileVersion; Language=$versionInfo.Language }
$metadata | ConvertTo-Json | Tee-Object (Join-Path $evidence 'file-properties.json')
# Inno Setup pads PE version strings with spaces; validate their displayed values.
$description = $versionInfo.FileDescription.TrimEnd()
$company = $versionInfo.CompanyName.TrimEnd()
$fileVersion = $versionInfo.FileVersion.TrimEnd()
if ($description -cne 'Instalador VST3 Windows x64') { throw "Wrong FileDescription: '$description'" }
if ($company -cne 'M7G Music') { throw 'Wrong CompanyName.' }
if ($fileVersion -notmatch '^1\.0\.1(?:\.0)?$') { throw 'Wrong FileVersion.' }
if ((Get-Item $installer).Length -lt 2MB -or (Get-Item $installer).Length -gt 4MB) { throw 'Unexpected installer size.' }

$report = [ordered]@{
    NativeWindows = $true
    OperatingSystem = (Get-CimInstance Win32_OperatingSystem).Caption
    OSVersion = [Environment]::OSVersion.Version.ToString()
    Architecture = 'x64'
    Compiler = "Inno Setup $compilerVersion"
    Filename = $filename
    FileDescription = $description
    FileVersion = $fileVersion
    CompanyName = $company
    SizeBytes = (Get-Item $installer).Length
    InstallerSHA256 = (Get-FileHash $installer -Algorithm SHA256).Hash.ToLowerInvariant()
    OriginalVST3SHA256 = $inputs.vst3_sha256
    ARA = $false
}

function Run-Installer([string]$exe, [string]$log) {
    $arguments = "/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /SP- /LOG=`"$log`""
    $process = Start-Process -FilePath $exe -ArgumentList $arguments -Wait -PassThru
    if ($process.ExitCode -ne 0) { throw "$exe failed with exit code $($process.ExitCode)" }
}

if (-not $SkipInstallValidation) {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = [Security.Principal.WindowsPrincipal]::new($identity)
    if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        throw 'The disposable native Windows validation runner must already be elevated.'
    }
    $vstRoot = Join-Path ${env:CommonProgramFiles} 'VST3'
    $installedBundle = Join-Path $vstRoot $bundle
    $installedModule = Join-Path $installedBundle $moduleRelative
    $regPath = 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{D6065C24-A5D1-5C79-8251-B591811DCF28}_is1'
    $uninstaller = Join-Path ${env:ProgramFiles} 'M7G Music\M7G Fresh Vocal Pro Beast\unins000.exe'
    $neighbor = Join-Path $vstRoot 'M7G-Installer-Native-Validation-Neighbor.vst3'
    if ((Test-Path $installedBundle) -or (Test-Path $regPath) -or (Test-Path $neighbor)) {
        throw 'The isolated Windows runner must not contain an existing M7G installation or test fixture.'
    }
    New-Item -ItemType Directory $neighbor -Force | Out-Null
    $marker = Join-Path $neighbor 'preserve.txt'
    'Preserve neighboring VST3 plugins.' | Set-Content $marker
    $markerHash = (Get-FileHash $marker -Algorithm SHA256).Hash

    Run-Installer $installer (Join-Path $evidence 'install.log')
    if ((Get-FileHash $installedModule -Algorithm SHA256).Hash.ToLowerInvariant() -ne $inputs.vst3_sha256) {
        throw 'The installed VST3 differs from the original compiled module.'
    }
    foreach ($relative in @('Contents\Resources\M7G.ico', 'Contents\Resources\Licenses\License-GPL3.txt')) {
        if (-not (Test-Path (Join-Path $installedBundle $relative))) { throw "Missing $relative" }
    }
    $uninstallEntry = Get-ItemProperty $regPath
    if ($uninstallEntry.DisplayVersion -ne '1.0.1' -or $uninstallEntry.Publisher -ne 'M7G Music') {
        throw 'Wrong Windows uninstall registration.'
    }
    if (-not (Test-Path $uninstaller)) { throw 'Uninstaller is missing.' }
    $uninstallEntry | ConvertTo-Json | Set-Content (Join-Path $evidence 'uninstall-registration.json')
    $installLog = Get-Content (Join-Path $evidence 'install.log') -Raw
    if ($installLog -notmatch '64-bit install mode: Yes') { throw 'Not installed in 64-bit mode.' }
    if ($installLog -notmatch 'Administrative install mode: Yes') { throw 'Not installed in the required common-files mode.' }

    Run-Installer $installer (Join-Path $evidence 'reinstall.log')
    if ((Get-FileHash $installedModule -Algorithm SHA256).Hash.ToLowerInvariant() -ne $inputs.vst3_sha256) {
        throw 'Reinstallation changed the module.'
    }
    if ((Get-FileHash $marker -Algorithm SHA256).Hash -ne $markerHash) { throw 'Neighboring plugin changed.' }

    Run-Installer $uninstaller (Join-Path $evidence 'uninstall.log')
    $deadline = [DateTime]::UtcNow.AddSeconds(30)
    while (((Test-Path $installedModule) -or (Test-Path $regPath)) -and [DateTime]::UtcNow -lt $deadline) {
        Start-Sleep -Milliseconds 250
    }
    if ((Test-Path $installedModule) -or (Test-Path $regPath)) { throw 'Uninstall did not finish.' }
    if ((Get-FileHash $marker -Algorithm SHA256).Hash -ne $markerHash) { throw 'Uninstall touched a neighboring plugin.' }
    Remove-Item $neighbor -Recurse -Force
    $report['Install'] = 'PASS'
    $report['Reinstall'] = 'PASS'
    $report['Uninstall'] = 'PASS'
    $report['ControlPanelRegistration'] = 'PASS'
    $report['OriginalVST3Preserved'] = 'PASS'
    $report['NeighboringPluginsPreserved'] = 'PASS'
    $report['InstallDestination'] = $vstRoot
}

$report | ConvertTo-Json | Tee-Object (Join-Path $evidence 'native-windows-verification.json')
if (@(Get-ChildItem $dist -File).Count -ne 1) { throw 'Deliverables must contain only the Windows EXE.' }
