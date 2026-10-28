Unicode True
Target x86-unicode
!include "MUI2.nsh"
!include "x64.nsh"
!include "WinVer.nsh"
!ifndef ARTIFACT_DIR
 !error "ARTIFACT_DIR is required"
!endif
!ifndef OUTPUT_FILE
 !define OUTPUT_FILE "M7G-ALIEN-TUNE-GOD-V1-Windows-x64.exe"
!endif
!define PRODUCT "M7G ALIEN TUNE GOD V1"
Name "${PRODUCT}"
OutFile "${OUTPUT_FILE}"
InstallDir "$PROGRAMFILES64\M7G\Alien Tune V1"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
BrandingText "M7G GOLDEN / V1 TUNE GOD / 100 PRESETS"
VIProductVersion "1.0.0.0"
VIAddVersionKey /LANG=1034 "ProductName" "${PRODUCT}"
VIAddVersionKey /LANG=1034 "FileDescription" "${PRODUCT} - Instalador VST3 Windows x64"
VIAddVersionKey /LANG=1034 "LegalCopyright" "M7G. Included JUCE 7 and font licence notices apply."
VIAddVersionKey /LANG=1034 "FileVersion" "1.0.0"
VIAddVersionKey /LANG=1034 "ProductVersion" "1.0.0"
!define MUI_ABORTWARNING
!define MUI_WELCOMEPAGE_TEXT "Afinacion vocal, cuatro armonias y dos dobles.$\r$\n$\r$\nCierra Studio One antes de instalar. Despues vuelve a escanear VST3.$\r$\n$\r$\nESTUDIO: 42.67 ms. MONITOR: 10.67 ms a 48 kHz. El DAW compensa la latencia.$\r$\n$\r$\nWindows 10/11 de 64 bits. Conserva Alien Voice, Transitions y tu licencia global."
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_TEXT "Instalado en la carpeta VST3 de Windows.$\r$\n$\r$\nAbre Studio One, escanea VST3 y busca M7G ALIEN TUNE GOD V1.$\r$\n$\r$\nElige un perfil, tu tonalidad y un preset. Reproduce la voz; activa armonias o dobles."
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "Spanish"
Function .onInit
 ${IfNot} ${RunningX64}
  MessageBox MB_ICONSTOP "Requiere Windows de 64 bits."
  Abort
 ${EndIf}
 ${IfNot} ${AtLeastWin10}
  MessageBox MB_ICONSTOP "Requiere Windows 10 o posterior."
  Abort
 ${EndIf}
 SetRegView 64
FunctionEnd
Section "VST3 / 100 PRESETS"
 SetOutPath "$COMMONFILES64\VST3\M7G ALIEN TUNE GOD V1.vst3"
 ClearErrors
 File /r "${ARTIFACT_DIR}\VST3\M7G ALIEN TUNE GOD V1.vst3\*.*"
 IfErrors 0 +3
  MessageBox MB_ICONSTOP "Cierra el DAW y vuelve a intentar la instalacion."
  Abort
 SetOutPath "$INSTDIR"
 File /oname=LEEME.md "..\README.md"
 File /oname=MODELOS.json "..\Models\METRICS.json"
 File /oname=ORT-LICENSE.txt "..\Dependencies\onnxruntime-win-x64-1.20.1\LICENSE"
 File /oname=ORT-NOTICES.txt "..\Dependencies\onnxruntime-win-x64-1.20.1\ThirdPartyNotices.txt"
 File /oname=NOTICES.md "..\NOTICES.md"
 File /oname=JUCE7-LICENSE.md "..\..\JUCE7\LICENSE.md"
 File /oname=VST3-LICENSE.txt "..\..\JUCE7\modules\juce_audio_processors\format_types\VST3_SDK\LICENSE.txt"
 File /oname=DejaVu-LICENSE.txt "..\..\Assets\Fonts\DejaVu-LICENSE.txt"
 File /oname=Graffiti-LICENSE.txt "..\..\Assets\Fonts\RubikWetPaint-LICENSE.txt"
 SetShellVarContext all
 IfFileExists "$APPDATA\M7G\M7G.license" license_exists
 SetOutPath "$APPDATA\M7G"
 File "..\..\Licensing\M7G.license"
 license_exists:
 SetOutPath "$INSTDIR"
 WriteUninstaller "$INSTDIR\Uninstall.exe"
 WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\M7GAlienTuneV1" "DisplayName" "${PRODUCT}"
 WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\M7GAlienTuneV1" "DisplayVersion" "1.0.0"
 WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\M7GAlienTuneV1" "Publisher" "M7G"
 WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\M7GAlienTuneV1" "UninstallString" '"$INSTDIR\Uninstall.exe"'
 WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\M7GAlienTuneV1" "NoModify" 1
 WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\M7GAlienTuneV1" "NoRepair" 1
SectionEnd
Section "Uninstall"
 SetRegView 64
 RMDir /r "$COMMONFILES64\VST3\M7G ALIEN TUNE GOD V1.vst3"
 Delete "$INSTDIR\LEEME.md"
 Delete "$INSTDIR\MODELOS.json"
 Delete "$INSTDIR\ORT-LICENSE.txt"
 Delete "$INSTDIR\ORT-NOTICES.txt"
 Delete "$INSTDIR\NOTICES.md"
 Delete "$INSTDIR\JUCE7-LICENSE.md"
 Delete "$INSTDIR\VST3-LICENSE.txt"
 Delete "$INSTDIR\DejaVu-LICENSE.txt"
 Delete "$INSTDIR\Graffiti-LICENSE.txt"
 Delete "$INSTDIR\Uninstall.exe"
 RMDir "$INSTDIR"
 DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\M7GAlienTuneV1"
SectionEnd
