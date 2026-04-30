; ===========================================================================
; CAD-ENGINE — NSIS Installer Script
; ===========================================================================
; Creates a proper Windows installer with:
;   - Install/uninstall
;   - Start menu shortcuts
;   - Desktop shortcut (optional)
;   - Add/Remove Programs entry
;
; Build: makensis /DVERSION=0.8.0 /DPKG_DIR=path\to\pkg /DOUTPUT_DIR=path installer.nsi
; ===========================================================================

!ifndef VERSION
    !define VERSION "0.8.0"
!endif

!ifndef PKG_DIR
    !define PKG_DIR "..\..\build\package\CAD-ENGINE-v${VERSION}"
!endif

!ifndef OUTPUT_DIR
    !define OUTPUT_DIR "..\.."
!endif

; ---- Configuration ----
Name "CAD-ENGINE v${VERSION}"
OutFile "${OUTPUT_DIR}\CAD-ENGINE-v${VERSION}-setup.exe"
InstallDir "$PROGRAMFILES64\CAD-ENGINE"
InstallDirRegKey HKLM "Software\CAD-ENGINE" "InstallDir"
RequestExecutionLevel admin
SetCompressor /SOLID lzma

; ---- Interface Modern UI ----
!include "MUI2.nsh"
!include "FileFunc.nsh"

!define MUI_ABORTWARNING
!define MUI_ICON "${PKG_DIR}\..\..\packaging\linux\cad-engine.ico"
!define MUI_UNICON "${PKG_DIR}\..\..\packaging\linux\cad-engine.ico"

; ---- Pages ----
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${PKG_DIR}\..\..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

; Finish page with launch option
!define MUI_FINISHPAGE_RUN "$INSTDIR\cad-engine.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Lancer CAD-ENGINE"
!insertmacro MUI_PAGE_FINISH

; Uninstall pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; ---- Language ----
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "English"

; ===========================================================================
; Install Section
; ===========================================================================
Section "CAD-ENGINE (requis)" SecMain
    SectionIn RO
    SetOutPath "$INSTDIR"
    
    ; Copy all files from package directory
    File /r "${PKG_DIR}\*.*"
    
    ; Write install path to registry
    WriteRegStr HKLM "Software\CAD-ENGINE" "InstallDir" "$INSTDIR"
    WriteRegStr HKLM "Software\CAD-ENGINE" "Version" "${VERSION}"
    
    ; Uninstaller
    WriteUninstaller "$INSTDIR\uninstall.exe"
    
    ; Add/Remove Programs entry
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CAD-ENGINE" \
        "DisplayName" "CAD-ENGINE v${VERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CAD-ENGINE" \
        "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CAD-ENGINE" \
        "DisplayIcon" "$INSTDIR\cad-engine.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CAD-ENGINE" \
        "Publisher" "CAD-ENGINE Project"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CAD-ENGINE" \
        "DisplayVersion" "${VERSION}"
    
    ; Calculate install size
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CAD-ENGINE" \
        "EstimatedSize" "$0"
    
    ; Start Menu shortcuts
    CreateDirectory "$SMPROGRAMS\CAD-ENGINE"
    CreateShortCut "$SMPROGRAMS\CAD-ENGINE\CAD-ENGINE.lnk" "$INSTDIR\cad-engine.exe"
    CreateShortCut "$SMPROGRAMS\CAD-ENGINE\Désinstaller.lnk" "$INSTDIR\uninstall.exe"
    
    ; Desktop shortcut
    CreateShortCut "$DESKTOP\CAD-ENGINE.lnk" "$INSTDIR\cad-engine.exe"
SectionEnd

; ===========================================================================
; Uninstall Section
; ===========================================================================
Section "Uninstall"
    ; Remove files
    RMDir /r "$INSTDIR"
    
    ; Remove shortcuts
    Delete "$DESKTOP\CAD-ENGINE.lnk"
    RMDir /r "$SMPROGRAMS\CAD-ENGINE"
    
    ; Remove registry
    DeleteRegKey HKLM "Software\CAD-ENGINE"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CAD-ENGINE"
SectionEnd
