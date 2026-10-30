; M7G Fresh Vocal Pro Beast 1.0.1 - Inno Setup 6
; Guardar este archivo en UTF-8. Compilador usado: Inno Setup 6.2.2.
; Entradas editables: PayloadRoot, AssetRoot y OutputRoot.
; PayloadRoot contiene la carpeta completa "M7G Fresh Vocal Pro Beast.vst3".
; AssetRoot contiene M7G.ico y los textos de licencia incluidos en el proyecto.
; El instalador solicita únicamente la elevación UAC normal para Common Files.

#define AppName "M7G Fresh Vocal Pro Beast"
#define AppVersion "1.0.1"
#define AppPublisher "M7G Music"
#define BundleName "M7G Fresh Vocal Pro Beast.vst3"
#ifndef PayloadRoot
  #define PayloadRoot SourcePath + "Payload"
#endif
#ifndef AssetRoot
  #define AssetRoot SourcePath + "Assets"
#endif
#ifndef OutputRoot
  #define OutputRoot SourcePath + "Output"
#endif

[Setup]
AppId={{D6065C24-A5D1-5C79-8251-B591811DCF28}
AppName={#AppName}
AppVersion=1.0.1
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppComments=Procesador de mezcla vocal VST3 para Windows x64. Sin ARA.
DefaultDirName={commoncf}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
AppendDefaultDirName=no
UsePreviousAppDir=no
CreateAppDir=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
MinVersion=10.0
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=
LicenseFile={#AssetRoot}\License-GPL3.txt
SetupIconFile={#AssetRoot}\M7G.ico
WizardStyle=modern
WizardSizePercent=110
ShowLanguageDialog=no
LanguageDetectionMethod=none
OutputDir={#OutputRoot}
OutputBaseFilename=M7G-Fresh-Vocal-Pro-Beast-Installer-v1.0.1-Windows-x64
Compression=lzma2/max
SolidCompression=yes
DiskSpanning=no
Uninstallable=yes
UninstallFilesDir={commonpf64}\M7G Music\M7G Fresh Vocal Pro Beast
UninstallDisplayName={#AppName}
UninstallDisplayIcon={app}\{#BundleName}\Contents\Resources\M7G.ico
CloseApplications=no
RestartApplications=no
SetupLogging=yes
VersionInfoVersion=1.0.1
VersionInfoProductVersion=1.0.1
VersionInfoCompany=M7G Music
VersionInfoDescription=Instalador VST3 Windows x64
VersionInfoProductName={#AppName}
VersionInfoCopyright=M7G Music. Ver licencias incluidas.

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Files]
; Mantiene Contents\x86_64-win\<plugin>.vst3 dentro del bundle.
Source: "{#PayloadRoot}\{#BundleName}\*"; DestDir: "{app}\{#BundleName}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#AssetRoot}\M7G.ico"; DestDir: "{app}\{#BundleName}\Contents\Resources"; Flags: ignoreversion
Source: "{#AssetRoot}\License-GPL3.txt"; DestDir: "{app}\{#BundleName}\Contents\Resources\Licenses"; Flags: ignoreversion
Source: "{#AssetRoot}\NOTICES.md"; DestDir: "{app}\{#BundleName}\Contents\Resources\Licenses"; Flags: ignoreversion
Source: "{#AssetRoot}\INTER-OFL.txt"; DestDir: "{app}\{#BundleName}\Contents\Resources\Licenses"; Flags: ignoreversion
Source: "{#AssetRoot}\JUCE7-LICENSE.md"; DestDir: "{app}\{#BundleName}\Contents\Resources\Licenses"; Flags: ignoreversion
Source: "{#AssetRoot}\VST3-LICENSE.txt"; DestDir: "{app}\{#BundleName}\Contents\Resources\Licenses"; Flags: ignoreversion

[UninstallDelete]
; Solo retira el directorio del fabricante si ha quedado vacío.
Type: dirifempty; Name: "{commonpf64}\M7G Music"

[Messages]
WelcomeLabel2=Este asistente instalará [name/ver] como plugin VST3 de Windows x64.%n%nCierra tu DAW antes de continuar.
FinishedLabel=Se ha instalado [name].%n%nAbre tu DAW y actualiza la lista de plugins VST3.
