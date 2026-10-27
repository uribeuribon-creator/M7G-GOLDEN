Unicode True
Target amd64-unicode
!include "MUI2.nsh"
!include "x64.nsh"
!include "WinVer.nsh"
!ifndef ARTIFACT_DIR
  !error "ARTIFACT_DIR must point to the Release artefacts directory"
!endif
!ifndef OUTPUT_FILE
  !define OUTPUT_FILE "MAKKINE-VOICE-7-Setup-Windows-x64.exe"
!endif
!define PRODUCT "MAKKINE VOICE 7"
Name "${PRODUCT}"
OutFile "${OUTPUT_FILE}"
InstallDir "$PROGRAMFILES64\M7G\Golden Voice AI"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
BrandingText "MAKKINE VOICE 7"
VIProductVersion "0.2.1.0"
VIAddVersionKey /LANG=1034 "LegalCopyright" "M7G. See included third-party licenses."
VIAddVersionKey /LANG=1034 "ProductName" "${PRODUCT}"
VIAddVersionKey /LANG=1034 "FileDescription" "${PRODUCT} - Instalador VST3 Windows x64"
VIAddVersionKey /LANG=1034 "FileVersion" "0.2.1"
VIAddVersionKey /LANG=1034 "ProductVersion" "0.2.1"
!define MUI_ABORTWARNING
!define MUI_WELCOMEPAGE_TEXT "Este instalador coloca MAKKINE VOICE 7 en la carpeta VST3 de Windows.\r\n\r\nCierra tu DAW antes de instalar. Al terminar, abre el DAW y vuelve a escanear los plugins.\r\n\r\nRequiere Windows 10/11 de 64 bits."
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_TEXT "Instalacion completada. Abre tu DAW y escanea los plugins VST3.\r\n\r\nEl plugin se encuentra en:\r\n$COMMONFILES64\VST3\MAKKINE VOICE 7.vst3"
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "Spanish"
Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_ICONSTOP "Este plugin requiere Windows de 64 bits."
    Abort
  ${EndIf}
  ${IfNot} ${AtLeastWin10}
    MessageBox MB_ICONSTOP "Este plugin requiere Windows 10 o posterior."
    Abort
  ${EndIf}
  SetRegView 64
FunctionEnd
Section "Plugin VST3"
  SetOutPath "$COMMONFILES64\VST3\MAKKINE VOICE 7.vst3"
  ClearErrors
  File /r "${ARTIFACT_DIR}\VST3\MAKKINE VOICE 7.vst3\*.*"
  IfErrors 0 +3
    MessageBox MB_ICONSTOP "No se pudo instalar el plugin. Cierra el DAW y vuelve a intentarlo."
    Abort
  ; Replace the previous bundle only after the new files were copied successfully.
  RMDir /r "$COMMONFILES64\VST3\M7G Golden Voice AI.vst3"
  SetOutPath "$INSTDIR"
  File /oname=LEEME.md "..\README.md"
  File /oname=RNNoise-LICENSE.txt "..\ThirdParty\rnnoise\COPYING"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\M7GGoldenVoiceAI" "DisplayName" "${PRODUCT}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\M7GGoldenVoiceAI" "DisplayVersion" "0.2.1"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\M7GGoldenVoiceAI" "Publisher" "M7G"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\M7GGoldenVoiceAI" "UninstallString" '$\"$INSTDIR\Uninstall.exe$\"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\M7GGoldenVoiceAI" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\M7GGoldenVoiceAI" "NoRepair" 1
SectionEnd
Section "Uninstall"
  SetRegView 64
  RMDir /r "$COMMONFILES64\VST3\MAKKINE VOICE 7.vst3"
  Delete "$INSTDIR\LEEME.md"
  Delete "$INSTDIR\RNNoise-LICENSE.txt"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\M7GGoldenVoiceAI"
SectionEnd
