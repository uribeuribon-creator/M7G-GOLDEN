Unicode True
Target x86-unicode
!include "MUI2.nsh"
!include "x64.nsh"
!include "WinVer.nsh"
!ifndef ARTIFACT_DIR
 !error "ARTIFACT_DIR is required"
!endif
!ifndef OUTPUT_FILE
 !define OUTPUT_FILE "M7G-Fresh-Vocal-Pro-Beast-Windows-x64.exe"
!endif
!define PRODUCT "M7G Fresh Vocal Pro Beast"
Name "${PRODUCT}"
OutFile "${OUTPUT_FILE}"
InstallDir "$PROGRAMFILES64\M7G\M7G Fresh Vocal Pro Beast"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
BrandingText "M7G FRESH VOCAL PRO BEAST / VOCAL MIX / VST3 + ARA"
VIProductVersion "1.0.0.0"
VIAddVersionKey /LANG=1034 "ProductName" "${PRODUCT}"
VIAddVersionKey /LANG=1034 "FileDescription" "${PRODUCT} - Instalador VST3 Windows x64"
VIAddVersionKey /LANG=1034 "LegalCopyright" "M7G. Included JUCE 7 and font licence notices apply."
VIAddVersionKey /LANG=1034 "FileVersion" "1.0.0"
VIAddVersionKey /LANG=1034 "ProductVersion" "1.0.0"
!define MUI_ICON "..\Assets\Icon.ico"
!define MUI_UNICON "..\Assets\Icon.ico"
!define MUI_ABORTWARNING
!define MUI_WELCOMEPAGE_TEXT "Dinamica, EQ, saturacion y espacio vocal para Studio One.$\r$\n$\r$\nCierra Studio One antes de instalar y despues vuelve a escanear VST3.$\r$\n$\r$\nElige un preset, mezcla con los controles y compara A/B.$\r$\n$\r$\nWindows 10/11 de 64 bits. Conserva tus otros plugins y licencia global M7G."
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_TEXT "Instalado en la carpeta VST3 comun de Windows.$\r$\n$\r$\nAbre Studio One, escanea VST3 y busca M7G Fresh Vocal Pro Beast.$\r$\n$\r$\n35 presets, compresion multibanda, de-esser, EQ, saturacion, reverb y delay con ducking."
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
Section "VST3 + ARA / FRESH VOCAL PRO BEAST"
 SetOutPath "$COMMONFILES64\VST3\M7G Fresh Vocal Pro Beast.vst3"
 ClearErrors
 File /r "${ARTIFACT_DIR}\VST3\M7G Fresh Vocal Pro Beast.vst3\*.*"
 IfErrors 0 +3
  MessageBox MB_ICONSTOP "Cierra el DAW y vuelve a intentar la instalacion."
  Abort
 SetOutPath "$INSTDIR"
 File /oname=LEEME.md "..\README.md"
 File /oname=COPYING-GPL3.txt "..\COPYING"
 File /oname=ARA-API-LICENSE.txt "..\ThirdParty\ARA_SDK\ARA_API\LICENSE.txt"
 File /oname=ARA-LIBRARY-LICENSE.txt "..\ThirdParty\ARA_SDK\ARA_Library\LICENSE.txt"
 File /oname=INTER-OFL.txt "..\Assets\Inter-OFL.txt"
 File /oname=NOTICES.md "..\NOTICES.md"
 File /oname=JUCE7-LICENSE.md "..\..\JUCE7\LICENSE.md"
 File /oname=VST3-LICENSE.txt "..\..\JUCE7\modules\juce_audio_processors\format_types\VST3_SDK\LICENSE.txt"
 SetShellVarContext all
 IfFileExists "$APPDATA\M7G\M7G.license" license_exists
 SetOutPath "$APPDATA\M7G"
 File "..\..\Licensing\M7G.license"
 license_exists:
 SetOutPath "$INSTDIR"
 WriteUninstaller "$INSTDIR\Uninstall.exe"
 WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\M7GFreshVocalProBeast" "DisplayName" "${PRODUCT}"
 WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\M7GFreshVocalProBeast" "DisplayVersion" "1.0.0"
 WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\M7GFreshVocalProBeast" "Publisher" "M7G"
 WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\M7GFreshVocalProBeast" "UninstallString" '"$INSTDIR\Uninstall.exe"'
 WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\M7GFreshVocalProBeast" "NoModify" 1
 WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\M7GFreshVocalProBeast" "NoRepair" 1
SectionEnd
Section "Uninstall"
 SetRegView 64
 RMDir /r "$COMMONFILES64\VST3\M7G Fresh Vocal Pro Beast.vst3"
 Delete "$INSTDIR\LEEME.md"
 Delete "$INSTDIR\COPYING-GPL3.txt"
 Delete "$INSTDIR\NOTICES.md"
 Delete "$INSTDIR\INTER-OFL.txt"
 Delete "$INSTDIR\ARA-API-LICENSE.txt"
 Delete "$INSTDIR\ARA-LIBRARY-LICENSE.txt"
 Delete "$INSTDIR\JUCE7-LICENSE.md"
 Delete "$INSTDIR\VST3-LICENSE.txt"
 Delete "$INSTDIR\Uninstall.exe"
 RMDir "$INSTDIR"
 DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\M7GFreshVocalProBeast"
SectionEnd
