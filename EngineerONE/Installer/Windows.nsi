Unicode True
Target x86-unicode
!include "MUI2.nsh"
!include "x64.nsh"
!include "WinVer.nsh"
!ifndef ARTIFACT_DIR
 !error "ARTIFACT_DIR is required"
!endif
!ifndef OUTPUT_FILE
 !define OUTPUT_FILE "Makkine-Engineer-ONE-Studio-One-Edition-Windows-x64.exe"
!endif
!define PRODUCT "Makkine Engineer ONE - Studio One Edition"
Name "${PRODUCT}"
OutFile "${OUTPUT_FILE}"
InstallDir "$PROGRAMFILES64\M7G\Makkine Engineer ONE"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
BrandingText "MAKKINE ENGINEER ONE / IA AUTOMATICA / VST3 + ARA"
VIProductVersion "1.0.0.0"
VIAddVersionKey /LANG=1034 "ProductName" "${PRODUCT}"
VIAddVersionKey /LANG=1034 "FileDescription" "${PRODUCT} - Instalador VST3 Windows x64"
VIAddVersionKey /LANG=1034 "LegalCopyright" "M7G. Included JUCE 7 and font licence notices apply."
VIAddVersionKey /LANG=1034 "FileVersion" "1.0.0"
VIAddVersionKey /LANG=1034 "ProductVersion" "1.0.0"
!define MUI_ABORTWARNING
!define MUI_WELCOMEPAGE_TEXT "Ingenieria vocal automatica para Studio One.$\r$\n$\r$\nCierra Studio One antes de instalar y despues vuelve a escanear VST3.$\r$\n$\r$\nCAPTURA la pista o el evento ARA, pulsa IA, escucha y arrastra.$\r$\n$\r$\nWindows 10/11 de 64 bits. Conserva tus otros plugins y licencia global M7G."
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_TEXT "Instalado en la carpeta VST3 comun de Windows.$\r$\n$\r$\nAbre Studio One, escanea VST3 y busca Makkine Engineer ONE.$\r$\n$\r$\nLa IA afina sola. ARA aplica la correccion al evento original. WAV PCM24/48k con timestamp BWF."
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
Section "VST3 + ARA / MAKKINE ENGINEER ONE"
 SetOutPath "$COMMONFILES64\VST3\Makkine Engineer ONE - Studio One Edition.vst3"
 ClearErrors
 File /r "${ARTIFACT_DIR}\VST3\Makkine Engineer ONE - Studio One Edition.vst3\*.*"
 IfErrors 0 +3
  MessageBox MB_ICONSTOP "Cierra el DAW y vuelve a intentar la instalacion."
  Abort
 SetOutPath "$INSTDIR"
 File /oname=LEEME.md "..\README.md"
 File /oname=COPYING-GPL3.txt "..\COPYING"
 File /oname=RUBBERBAND-COPYING.txt "..\ThirdParty\rubberband\COPYING"
 File /oname=ARA-API-LICENSE.txt "..\ThirdParty\ARA_SDK\ARA_API\LICENSE.txt"
 File /oname=ARA-LIBRARY-LICENSE.txt "..\ThirdParty\ARA_SDK\ARA_Library\LICENSE.txt"
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
 WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MakkineEngineerONE" "DisplayName" "${PRODUCT}"
 WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MakkineEngineerONE" "DisplayVersion" "1.0.0"
 WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MakkineEngineerONE" "Publisher" "M7G"
 WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MakkineEngineerONE" "UninstallString" '"$INSTDIR\Uninstall.exe"'
 WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MakkineEngineerONE" "NoModify" 1
 WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MakkineEngineerONE" "NoRepair" 1
SectionEnd
Section "Uninstall"
 SetRegView 64
 RMDir /r "$COMMONFILES64\VST3\Makkine Engineer ONE - Studio One Edition.vst3"
 Delete "$INSTDIR\LEEME.md"
 Delete "$INSTDIR\COPYING-GPL3.txt"
 Delete "$INSTDIR\RUBBERBAND-COPYING.txt"
 Delete "$INSTDIR\NOTICES.md"
 Delete "$INSTDIR\ARA-API-LICENSE.txt"
 Delete "$INSTDIR\ARA-LIBRARY-LICENSE.txt"
 Delete "$INSTDIR\JUCE7-LICENSE.md"
 Delete "$INSTDIR\VST3-LICENSE.txt"
 Delete "$INSTDIR\DejaVu-LICENSE.txt"
 Delete "$INSTDIR\Graffiti-LICENSE.txt"
 Delete "$INSTDIR\Uninstall.exe"
 RMDir "$INSTDIR"
 DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MakkineEngineerONE"
SectionEnd
