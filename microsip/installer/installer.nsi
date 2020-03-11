;NSIS Modern User Interface
;Welcome/Finish Page Script
;Written by Joost Verburg

;--------------------------------
;Include Modern UI

  !include "MUI2.nsh"
  !include "Library.nsh"
  !include "FileFunc.nsh"  
  !include "LogicLib.nsh"

;--------------------------------
;General

!define APPVERSON "3.19.11"
!define APPNAME "SmartPABX Client"
!define APPCOMPANY "Broadband Solutions"
!define CUSTOM
!define FAST
!define AUTORUN
 
!ifndef APPALLUSERS
  !define HKKEY HKCU
!else
  !define HKKEY HKLM
!endif

!ifndef APPLINKNAME
  !define APPLINKNAME "${APPNAME}"
!endif

!ifndef APPICONINDEX
  !define APPICONINDEX "0"
!endif

!ifndef APPLICENSE
  !define APPLICENSE "License.txt"
!endif

!ifdef CUSTOM
  !define PREFIX "langpack\"
!else
  !define PREFIX ""
!endif

!ifdef THEME_ORANGE
!ifndef APPICON
  !insertmacro MUI_DEFAULT MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\orange-install.ico"
!else
  !insertmacro MUI_DEFAULT MUI_ICON "${APPICON}"
!endif
  !insertmacro MUI_DEFAULT MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\orange-uninstall.ico"
  !define MUI_HEADERIMAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Header\orange.bmp"
  !define MUI_HEADERIMAGE_UNBITMAP  "${NSISDIR}\Contrib\Graphics\Header\orange-uninstall.bmp"
  !insertmacro MUI_DEFAULT MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\orange.bmp"
  !insertmacro MUI_DEFAULT MUI_UNWELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\orange.bmp"
!endif

Unicode true

  ;Default installation folder
!ifdef APPFOLDER
  InstallDir "$LOCALAPPDATA\${APPFOLDER}"
!else

!ifndef APPALLUSERS
  InstallDir "$LOCALAPPDATA\${APPNAME}"
!else
  InstallDir "$PROGRAMFILES\${APPNAME}"
!endif

!endif

  ;Get installation folder from registry if available
  InstallDirRegKey ${HKKEY} "Software\${APPNAME}" ""

  ;Request application privileges for Windows Vista

!ifndef APPALLUSERS
  RequestExecutionLevel user
!else
  RequestExecutionLevel admin
!endif

  
  !define ARP "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"

;--------------------------------
;Variables

  Var STARTMENU_FOLDER
  Var MUI_TEMP

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING
  !define MUI_LANGDLL_ALLLANGUAGES  
  !define MUI_HEADERIMAGE
!ifndef MUI_HEADERIMAGE_BITMAP
  !define MUI_HEADERIMAGE_BITMAP "header.bmp"
  !define MUI_HEADERIMAGE_UNBITMAP  "header-uninstall.bmp"
  !insertmacro MUI_DEFAULT MUI_WELCOMEFINISHPAGE_BITMAP "wizard.bmp"
  !insertmacro MUI_DEFAULT MUI_UNWELCOMEFINISHPAGE_BITMAP "wizard.bmp"
!ifndef APPICON
  !insertmacro MUI_DEFAULT MUI_ICON "install.ico"
!else
  !insertmacro MUI_DEFAULT MUI_ICON "${APPICON}"
!endif
  !insertmacro MUI_DEFAULT MUI_UNICON "uninstall.ico"
!endif

;--------------------------------
;Language Selection Dialog Settings

  ;The language stored in the registry will be selected by default.
  !define MUI_LANGDLL_ALWAYSSHOW

  ;Remember the installer language
  !define MUI_LANGDLL_REGISTRY_ROOT "${HKKEY}" 
  !define MUI_LANGDLL_REGISTRY_KEY "Software\${APPNAME}" 
  !define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

;--------------------------------
;Pages

  ;Start Menu Folder Page Configuration
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "${HKKEY}" 
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\${APPNAME}"
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
!ifdef APPFOLDER
  !define MUI_STARTMENUPAGE_DEFAULTFOLDER "${APPFOLDER}"
!endif

  !insertmacro MUI_PAGE_WELCOME
!ifndef FAST
  !insertmacro MUI_PAGE_LICENSE "common\${APPLICENSE}"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
!endif
  !insertmacro MUI_PAGE_STARTMENU Application $STARTMENU_FOLDER

!ifdef INSTANCE
var CustomDialog
var CustomDialogIcon
var CustomDialogImage
var CustomInstance
var CustomInstanceValue
var CustomUsername
var CustomUsernameValue
var CustomPassword
var CustomPasswordValue
Page custom nsDialogsPage nsDialogsPageLeave
Function nsDialogsPage
        !insertmacro MUI_HEADER_TEXT "Set up Account" "Enter your Instance, Extension and Password."
	nsDialogs::Create 1018
	Pop $CustomDialog

	${If} $CustomDialog == error
		Abort
	${EndIf}

	${NSD_CreateLabel} 22u 2u 70u 10u "Instance"
	${NSD_CreateText} 94u 0u 150u 14u ""
	Pop $CustomInstance
	${NSD_CreateLabel} 22u 20u 70u 10u "Extension"
	${NSD_CreateText} 94u 18u 150u 14u ""
	Pop $CustomUsername
	${NSD_CreateLabel} 22u 38u 70u 10u "Password"
	${NSD_CreateText} 94u 36u 150u 14u ""
	Pop $CustomPassword

        ${NSD_CreateIcon} 70u 58u 100% 100% ""
	Pop $CustomDialogIcon
	File /oname=$PLUGINSDIRMy_Icon.ico "..\custom\${INSTANCE}-logo.ico"
	StrCpy $0 $PLUGINSDIRMy_Icon.ico
	System::Call 'user32::LoadImage(i 0, t r0, i ${IMAGE_ICON}, i 0, i 0, i ${LR_LOADFROMFILE}) i.s'  
	Pop $CustomDialogImage
	SendMessage $CustomDialogIcon ${STM_SETIMAGE} ${IMAGE_ICON} $CustomDialogImage

	${NSD_SetFocus} $CustomInstance
	nsDialogs::Show
FunctionEnd

Function nsDialogsPageLeave
	${NSD_GetText} $CustomInstance $CustomInstanceValue
	${NSD_GetText} $CustomUsername $CustomUsernameValue
	${NSD_GetText} $CustomPassword $CustomPasswordValue
 ${If} $CustomInstanceValue == ""
 ${OrIf} $CustomUsernameValue == ""
 ${OrIf} $CustomPasswordValue == ""
        MessageBox MB_OK "Please fill in all fields"
        Abort
 ${EndIf}
    System::Call 'gdi32:DeleteObject(i $CustomDialogImage)'
    ${NSD_FreeIcon} $CustomDialogIcon
FunctionEnd
!endif

  !insertmacro MUI_PAGE_INSTFILES
!ifndef CUSTOM
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE FinishedInstall
Function FinishedInstall
ExecShell "open" "http://www.microsip.org/donate"
FunctionEnd 
!endif

  !define MUI_FINISHPAGE_RUN
  !define MUI_FINISHPAGE_RUN_FUNCTION "LaunchApp"
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_WELCOME
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_COMPONENTS
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH


;--------------------------------
;Languages

;--------------------------------
;Languages

!ifndef APPLANG

!ifdef APPLANGDEFAULT
!insertmacro MUI_LANGUAGE "${APPLANGDEFAULT}"
!endif

!insertmacro MUI_LANGUAGE "English" ;first language is the default language

!insertmacro MUI_LANGUAGE "Afrikaans"
!insertmacro MUI_LANGUAGE "Albanian"
!insertmacro MUI_LANGUAGE "Arabic"
!insertmacro MUI_LANGUAGE "Armenian"
!insertmacro MUI_LANGUAGE "Basque"
!insertmacro MUI_LANGUAGE "Belarusian"
!insertmacro MUI_LANGUAGE "Bosnian"
!insertmacro MUI_LANGUAGE "Breton"
!insertmacro MUI_LANGUAGE "Bulgarian"
!insertmacro MUI_LANGUAGE "Catalan"
!insertmacro MUI_LANGUAGE "Croatian"
!insertmacro MUI_LANGUAGE "Czech"
!insertmacro MUI_LANGUAGE "Danish"
!insertmacro MUI_LANGUAGE "Dutch"
!insertmacro MUI_LANGUAGE "Esperanto"
!insertmacro MUI_LANGUAGE "Estonian"
!insertmacro MUI_LANGUAGE "Farsi"
!insertmacro MUI_LANGUAGE "Finnish"
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "Galician"
!insertmacro MUI_LANGUAGE "Georgian"
!insertmacro MUI_LANGUAGE "German"
;!insertmacro MUI_LANGUAGE "GermanSwitzerland"
!insertmacro MUI_LANGUAGE "Greek"
!insertmacro MUI_LANGUAGE "Hebrew"
!insertmacro MUI_LANGUAGE "Hungarian"
!insertmacro MUI_LANGUAGE "Icelandic"
!insertmacro MUI_LANGUAGE "Indonesian"
!insertmacro MUI_LANGUAGE "Irish"
!insertmacro MUI_LANGUAGE "Italian"
!insertmacro MUI_LANGUAGE "Japanese"
!insertmacro MUI_LANGUAGE "Korean"
!insertmacro MUI_LANGUAGE "Kurdish"
!insertmacro MUI_LANGUAGE "Latvian"
!insertmacro MUI_LANGUAGE "Lithuanian"
!insertmacro MUI_LANGUAGE "Luxembourgish"
!insertmacro MUI_LANGUAGE "Macedonian"
!insertmacro MUI_LANGUAGE "Malay"
!insertmacro MUI_LANGUAGE "Mongolian"
!insertmacro MUI_LANGUAGE "Norwegian"
!insertmacro MUI_LANGUAGE "NorwegianNynorsk"
!insertmacro MUI_LANGUAGE "Polish"
!insertmacro MUI_LANGUAGE "Portuguese"
!insertmacro MUI_LANGUAGE "PortugueseBR"
!insertmacro MUI_LANGUAGE "Romanian"
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "Serbian"
!insertmacro MUI_LANGUAGE "SerbianLatin"
!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "Slovak"
!insertmacro MUI_LANGUAGE "Slovenian"
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_LANGUAGE "SpanishInternational"
!insertmacro MUI_LANGUAGE "Swedish"
!insertmacro MUI_LANGUAGE "Thai"
!insertmacro MUI_LANGUAGE "TradChinese"
!insertmacro MUI_LANGUAGE "Turkish"
!insertmacro MUI_LANGUAGE "Ukrainian"
!insertmacro MUI_LANGUAGE "Uzbek"
!insertmacro MUI_LANGUAGE "Vietnamese"
!insertmacro MUI_LANGUAGE "Welsh"

!else

!insertmacro MUI_LANGUAGE "${APPLANG}"

!ifdef APPLANG2
!insertmacro MUI_LANGUAGE "${APPLANG2}"
!endif
!ifdef APPLANG3
!insertmacro MUI_LANGUAGE "${APPLANG3}"
!endif
!ifdef APPLANG4
!insertmacro MUI_LANGUAGE "${APPLANG4}"
!endif
!ifdef APPLANG5
!insertmacro MUI_LANGUAGE "${APPLANG5}"
!endif


!endif

;--------------------------------
;VersionInfo

!ifdef APPVERSONFULL
  VIProductVersion "${APPVERSON}"
!else
  VIProductVersion "${APPVERSON}.0"
!endif
  VIAddVersionKey "ProductName" "${APPNAME}"
  VIAddVersionKey "ProductDescription" "${APPNAME} Setup"
  VIAddVersionKey "ProductVersion" "${APPVERSON}"
  VIAddVersionKey "CompanyName" "${APPCOMPANY}"
  VIAddVersionKey "LegalCopyright" "${APPCOMPANY}"
  VIAddVersionKey "FileDescription" "${APPNAME} Setup"
  VIAddVersionKey "FileVersion" "${APPVERSON}"

;--------------------------------
; Init

!ifdef APPLITE
	!ifdef CUSTOM
		!define EXEFILE "lite\custom\${APPNAME}.exe"
	!else
		!define EXEFILE "lite\${APPNAME}.exe"	
	!endif
!else
	!ifdef CUSTOM
		!define EXEFILE "full\custom\${APPNAME}.exe"
	!else
		!define EXEFILE "full\${APPNAME}.exe"	
	!endif
!endif

!define FileCopy `!insertmacro FileCopy`
!macro FileCopy FilePath TargetDir
  CreateDirectory `${TargetDir}`
  CopyFiles `${FilePath}` `${TargetDir}`
!macroend


!macro RemoveLinksAssociation

; for WinXP

  ReadRegStr $0 ${HKKEY} "SOFTWARE\Classes\sip" "Owner Name"
  StrCmp $0 "${APPNAME}" 0 +2
  DeleteRegKey ${HKKEY} "SOFTWARE\Classes\sip"

  ReadRegStr $0 ${HKKEY} "SOFTWARE\Classes\tel" "Owner Name"
  StrCmp $0 "${APPNAME}" 0 +2
  DeleteRegKey ${HKKEY} "SOFTWARE\Classes\tel"

  ReadRegStr $0 ${HKKEY} "SOFTWARE\Classes\callto" "Owner Name"
  StrCmp $0 "${APPNAME}" 0 +2
  DeleteRegKey ${HKKEY} "SOFTWARE\Classes\callto"


; for Vista

  ReadRegStr $0 ${HKKEY} "SOFTWARE\Microsoft\Windows\Shell\Associations\UrlAssociations\sip\UserChoice" "Progid"
  StrCmp $0 "${APPNAME}" 0 +2
  DeleteRegKey ${HKKEY} "SOFTWARE\Microsoft\Windows\Shell\Associations\UrlAssociations\sip"

  ReadRegStr $0 ${HKKEY} "SOFTWARE\Microsoft\Windows\Shell\Associations\UrlAssociations\tel\UserChoice" "Progid"
  StrCmp $0 "${APPNAME}" 0 +2
  DeleteRegKey ${HKKEY} "SOFTWARE\Microsoft\Windows\Shell\Associations\UrlAssociations\tel"

  ReadRegStr $0 ${HKKEY} "SOFTWARE\Microsoft\Windows\Shell\Associations\UrlAssociations\callto\UserChoice" "Progid"
  StrCmp $0 "${APPNAME}" 0 +2
  DeleteRegKey ${HKKEY} "SOFTWARE\Microsoft\Windows\Shell\Associations\UrlAssociations\callto"

!macroend


;--------------------------------
;Reserve Files
  
  ;If you are using solid compression, files that are required before
  ;the actual installation should be stored first in the data block,
  ;because this will make your installer start faster.
  
  !insertmacro MUI_RESERVEFILE_LANGDLL

;--------------------------------
!ifdef INNER
  !echo "Inner invocation"                  ; just to see what's going on
  OutFile "$%TEMP%\tempinstaller.exe"       ; not really important where this is
  SetCompress off                           ; for speed
!else

;Name and file
Name "${APPNAME}"
!ifdef APPLITE
  !define OUTFILE "out\${APPNAME}-Lite-${APPVERSON}.exe"
!else
  !define OUTFILE "out\${APPNAME}-${APPVERSON}.exe"
!endif
OutFile "${OUTFILE}"

!ifdef APPSING
  !echo "Outer invocation"

  ; Call makensis again, defining INNER.  This writes an installer for us which, when
  ; it is invoked, will just write the uninstaller to some location, and then exit.
  ; Be sure to substitute the name of this script here.
 
  !system "$\"${NSISDIR}\makensis$\" /DINNER installer.nsi" = 0
 
  ; So now run that installer we just created as %TEMP%\tempinstaller.exe.  Since it
  ; calls quit the return value isn't zero.
 
  !system "$%TEMP%\tempinstaller.exe" = 2
 
  ; That will have written an uninstaller binary for us.  Now we sign it with your
  ; favourite code signing tool.

  !system 'sign.bat "certs/${APPNAME}.pfx" "${APPSING}" "$%TEMP%\Uninstall.exe"' = 0
  !system 'sign.bat "certs/${APPNAME}.pfx" "${APPSING}" "${EXEFILE}"' = 0
  !finalize 'sign.bat "certs/${APPNAME}.pfx" "${APPSING}" "${OUTFILE}"' = 0

!endif
 
!endif

;--------------------------------
;Installer Sections

Section "${APPNAME} (required)" SecInstall

  SectionIn RO

  Push "${APPNAME}"
  Call .closeProgram

  SetOutPath "$INSTDIR"

!ifdef CUSTOM
  CreateDirectory "$APPDATA\${APPNAME}"
  File /nonfatal "/oname=$APPDATA\${APPNAME}\${APPNAME}.ini" "custom\${APPNAME}.ini"
!endif

!ifdef WRITEINIFILE

StrCpy $2 ""

${GetParameters} $1
${GetOptions} $1 "/profile" $1
IfErrors +2 0
StrCpy $2 "$2profile=$1$\r$\n"

${GetParameters} $1
${GetOptions} $1 "/username" $1
IfErrors +2 0
StrCpy $2 "$2username=$1$\r$\n"

${GetParameters} $1
${GetOptions} $1 "/password" $1
IfErrors +2 0
StrCpy $2 "$2password=$1$\r$\n"

${GetParameters} $1
${GetOptions} $1 "/domain" $1
IfErrors +2 0
StrCpy $2 "$2domain=$1$\r$\n"

${GetParameters} $1
${GetOptions} $1 "/port" $1
IfErrors +2 0
StrCpy $2 "$2port=$1$\r$\n"

${GetParameters} $1
${GetOptions} $1 "/transport" $1
IfErrors +2 0
StrCpy $2 "$2transport=$1$\r$\n"

StrCmp $2 "" SkipIniWrite
CreateDirectory "$APPDATA\${APPNAME}"
FileOpen $3 "$APPDATA\${APPNAME}\${APPNAME}.ini" w
FileWrite $3 "[Settings]$\r$\naccountId=1$\r$\n[Account1]$\r$\n$2"
FileClose $3
SkipIniWrite:

!endif

!ifdef INSTANCE
IfFileExists "$APPDATA\${APPNAME}\${APPNAME}.ini" SkipIniWrite2
CreateDirectory "$APPDATA\${APPNAME}"
FileOpen $3 "$APPDATA\${APPNAME}\${APPNAME}.ini" w
FileWrite $3 "[Settings]$\r$\naccountId=1$\r$\n[Account1]$\r$\nserver=$CustomInstanceValue$\r$\nusername=$CustomUsernameValue$\r$\npassword=$CustomPasswordValue$\r$\n"
FileClose $3
SkipIniWrite2:
!endif

  !insertmacro RemoveLinksAssociation

!ifdef APPALLUSERS
  SetShellVarContext all
!endif

  ;Delete desktop link
  Delete "$DESKTOP\${APPLINKNAME}.lnk"

  ;Delete startup link
  Delete "$SMSTARTUP\${APPLINKNAME}.lnk"

  SetShellVarContext current

; Delete files from old location
  Delete /REBOOTOK "$APPDATA\${APPNAME}\${APPNAME}.exe"
  Delete /REBOOTOK "$APPDATA\${APPNAME}\*.dll"
  Delete /REBOOTOK "$APPDATA\${APPNAME}\*.sample"
  Delete /REBOOTOK "$APPDATA\${APPNAME}\*.txt"
  Delete /REBOOTOK "$APPDATA\${APPNAME}\*.url"
  Delete /REBOOTOK "$APPDATA\${APPNAME}\*.wav"

  File "${EXEFILE}"

!ifdef APPLITE
  Delete /REBOOTOK "$INSTDIR\*.dll"	
!else
  File "full\*.dll"
!endif
!ifdef CUSTOM_DLL
  File "dlls\${CUSTOM_DLL}"
!endif
!ifdef CUSTOM_FILE1
  File "${CUSTOM_FILE1}"
!endif
!ifdef CUSTOM_FILE2
  File "${CUSTOM_FILE2}"
!endif
!ifdef CUSTOM_FILE3
  File "${CUSTOM_FILE3}"
!endif
!ifndef NORING
!ifdef RINGIN
  File "${RINGIN}"
!else
  File common\ringin.wav
!endif
  File common\ringin2.wav
  File common\ringout.wav
  File common\hangup.wav
!endif
  File common\messageout.wav
  File common\messagein.wav
  File /oname=License.txt "common\${APPLICENSE}"
!ifndef CUSTOM
  File "common\${APPNAME} Website.url"
!endif

!ifdef APPCOPYFILE
  File "${APPCOPYFILE}"
!endif

Delete "$INSTDIR\langpack_*"

!ifndef CUSTOM
File "${PREFIX}langpack_english.txt.sample"
!endif

!ifndef APPLANG_EMBED
  
IntCmpU $LANGUAGE 1078 0 +2 +2
File "${PREFIX}langpack_afrikaans.txt"
IntCmpU $LANGUAGE 1052 0 +2 +2
File "${PREFIX}langpack_albanian.txt"
IntCmpU $LANGUAGE 1025 0 +2 +2
File "${PREFIX}langpack_arabic.txt"
IntCmpU $LANGUAGE 1067 0 +2 +2
File "${PREFIX}langpack_armenian.txt"
IntCmpU $LANGUAGE 1069 0 +2 +2
File "${PREFIX}langpack_basque.txt"
IntCmpU $LANGUAGE 1059 0 +2 +2
File "${PREFIX}langpack_belarusian.txt"
IntCmpU $LANGUAGE 5146 0 +2 +2
File "${PREFIX}langpack_bosnian.txt"
IntCmpU $LANGUAGE 1150 0 +2 +2
File "${PREFIX}langpack_breton.txt"
IntCmpU $LANGUAGE 1026 0 +2 +2
File "${PREFIX}langpack_bulgarian.txt"
IntCmpU $LANGUAGE 1027 0 +2 +2
File "${PREFIX}langpack_catalan.txt"
IntCmpU $LANGUAGE 1050 0 +2 +2
File "${PREFIX}langpack_croatian.txt"
IntCmpU $LANGUAGE 1029 0 +2 +2
File "${PREFIX}langpack_czech.txt"
IntCmpU $LANGUAGE 1030 0 +2 +2
File "${PREFIX}langpack_danish.txt"
IntCmpU $LANGUAGE 1043 0 +2 +2
File "${PREFIX}langpack_dutchnetherlands.txt"
IntCmpU $LANGUAGE 9998 0 +2 +2
File "${PREFIX}langpack_esperanto.txt"
IntCmpU $LANGUAGE 1061 0 +2 +2
File "${PREFIX}langpack_estonian.txt"
IntCmpU $LANGUAGE 1065 0 +2 +2
File "${PREFIX}langpack_farsi.txt"
IntCmpU $LANGUAGE 1035 0 +2 +2
File "${PREFIX}langpack_finnish.txt"
IntCmpU $LANGUAGE 1036 0 +2 +2
File "${PREFIX}langpack_french.txt"
IntCmpU $LANGUAGE 1110 0 +2 +2
File "${PREFIX}langpack_galician.txt"
IntCmpU $LANGUAGE 1079 0 +2 +2
File "${PREFIX}langpack_georgian.txt"
IntCmpU $LANGUAGE 1031 0 +2 +2
File "${PREFIX}langpack_german.txt"
IntCmpU $LANGUAGE 2055 0 +2 +2
File "${PREFIX}langpack_german.txt"
IntCmpU $LANGUAGE 1032 0 +2 +2
File "${PREFIX}langpack_greek.txt"
IntCmpU $LANGUAGE 1037 0 +2 +2
File "${PREFIX}langpack_hebrew.txt"
IntCmpU $LANGUAGE 1038 0 +2 +2
File "${PREFIX}langpack_hungarian.txt"
IntCmpU $LANGUAGE 15 0 +2 +2
File "${PREFIX}langpack_icelandic.txt"
IntCmpU $LANGUAGE 1057 0 +2 +2
File "${PREFIX}langpack_indonesian.txt"
IntCmpU $LANGUAGE 2108 0 +2 +2
File "${PREFIX}langpack_irish.txt"
IntCmpU $LANGUAGE 1040 0 +2 +2
File "${PREFIX}langpack_italian.txt"
IntCmpU $LANGUAGE 1041 0 +2 +2
File "${PREFIX}langpack_japanese.txt"
IntCmpU $LANGUAGE 1042 0 +2 +2
File "${PREFIX}langpack_korean.txt"
IntCmpU $LANGUAGE 9999 0 +2 +2
File "${PREFIX}langpack_kurdish.txt"
IntCmpU $LANGUAGE 1062 0 +2 +2
File "${PREFIX}langpack_latvian.txt"
IntCmpU $LANGUAGE 1063 0 +2 +2
File "${PREFIX}langpack_lithuanian.txt"
IntCmpU $LANGUAGE 4103 0 +2 +2
File "${PREFIX}langpack_luxembourgish.txt"
IntCmpU $LANGUAGE 1071 0 +2 +2
File "${PREFIX}langpack_macedonian.txt"
IntCmpU $LANGUAGE 1086 0 +2 +2
File "${PREFIX}langpack_malay.txt"
IntCmpU $LANGUAGE 1104 0 +2 +2
File "${PREFIX}langpack_mongolian.txt"
IntCmpU $LANGUAGE 1044 0 +2 +2
File "${PREFIX}langpack_norwegian.txt"
IntCmpU $LANGUAGE 2068 0 +2 +2
File "${PREFIX}langpack_norwegiannynorsk.txt"
IntCmpU $LANGUAGE 1045 0 +2 +2
File "${PREFIX}langpack_polish.txt"
IntCmpU $LANGUAGE 2070 0 +2 +2
File "${PREFIX}langpack_portuguese.txt"
IntCmpU $LANGUAGE 1046 0 +2 +2
File "${PREFIX}langpack_portuguesebr.txt"
IntCmpU $LANGUAGE 1048 0 +2 +2
File "${PREFIX}langpack_romanian.txt"
IntCmpU $LANGUAGE 1049 0 +2 +2
File "${PREFIX}langpack_russian.txt"
IntCmpU $LANGUAGE 3098 0 +2 +2
File "${PREFIX}langpack_serbian.txt"
IntCmpU $LANGUAGE 2074 0 +2 +2
File "${PREFIX}langpack_serbianlatin.txt"
IntCmpU $LANGUAGE 2052 0 +2 +2
File "${PREFIX}langpack_simpchinese.txt"
IntCmpU $LANGUAGE 1051 0 +2 +2
File "${PREFIX}langpack_slovak.txt"
IntCmpU $LANGUAGE 1060 0 +2 +2
File "${PREFIX}langpack_slovenian.txt"
IntCmpU $LANGUAGE 1034 0 +2 +2
File "${PREFIX}langpack_spanish.txt"
IntCmpU $LANGUAGE 3082 0 +2 +2
File "${PREFIX}langpack_spanishinternational.txt"
IntCmpU $LANGUAGE 1053 0 +2 +2
File "${PREFIX}langpack_swedish.txt"
IntCmpU $LANGUAGE 1054 0 +2 +2
File "${PREFIX}langpack_thai.txt"
IntCmpU $LANGUAGE 1028 0 +2 +2
File "${PREFIX}langpack_tradchinese.txt"
IntCmpU $LANGUAGE 1055 0 +2 +2
File "${PREFIX}langpack_turkish.txt"
IntCmpU $LANGUAGE 1058 0 +2 +2
File "${PREFIX}langpack_ukrainian.txt"
IntCmpU $LANGUAGE 1091 0 +2 +2
File "${PREFIX}langpack_uzbek.txt"
IntCmpU $LANGUAGE 1066 0 +2 +2
File "${PREFIX}langpack_vietnamese.txt"
IntCmpU $LANGUAGE 1160 0 +2 +2
File "${PREFIX}langpack_welsh.txt"

!endif
  
  ;Store installation folder
WriteRegStr ${HKKEY} "Software\${APPNAME}" "" $INSTDIR

  ;Register in default applications
; for Vista and later
WriteRegStr ${HKKEY} "SOFTWARE\${APPNAME}\Capabilities" "ApplicationDescription" "Softphone"
WriteRegStr ${HKKEY} "SOFTWARE\${APPNAME}\Capabilities" "ApplicationName" "${APPLINKNAME}"
WriteRegStr ${HKKEY} "SOFTWARE\${APPNAME}\Capabilities\UrlAssociations" "tel" "${APPNAME}"
WriteRegStr ${HKKEY} "SOFTWARE\${APPNAME}\Capabilities\UrlAssociations" "callto" "${APPNAME}"
WriteRegStr ${HKKEY} "SOFTWARE\${APPNAME}\Capabilities\UrlAssociations" "sip" "${APPNAME}"

WriteRegStr ${HKKEY} "SOFTWARE\Classes\${APPNAME}" "" "Internet Call Protocol"
WriteRegStr ${HKKEY} "SOFTWARE\Classes\${APPNAME}\DefaultIcon" "" "$INSTDIR\${APPNAME}.exe,${APPICONINDEX}"
WriteRegStr ${HKKEY} "SOFTWARE\Classes\${APPNAME}\shell\open\command" "" "$\"$INSTDIR\${APPNAME}.exe$\" $\"%1$\""

WriteRegStr ${HKKEY} "SOFTWARE\RegisteredApplications" "${APPNAME}" "SOFTWARE\${APPNAME}\Capabilities"

; reset flags
  WriteRegDWORD ${HKKEY} "Software\${APPNAME}" "DesktopShortcut" "0"
  WriteRegDWORD ${HKKEY} "Software\${APPNAME}" "RunAtSystemStartup" "0"
  WriteRegDWORD ${HKKEY} "Software\${APPNAME}" "LinksAssociation" "0"


  ;Create uninstaller
!ifndef APPSING
  WriteUninstaller "$INSTDIR\Uninstall.exe"
!else
!ifndef INNER
  SetOutPath $INSTDIR
  ; this packages the signed uninstaller
  File $%TEMP%\Uninstall.exe
!endif
!endif

  ; write uninstall strings
  WriteRegStr ${HKKEY} "${ARP}" "DisplayName" "${APPNAME}"
  WriteRegStr ${HKKEY} "${ARP}" "DisplayIcon" "$INSTDIR\${APPNAME}.exe,${APPICONINDEX}"
  WriteRegStr ${HKKEY} "${ARP}" "Publisher" "${APPCOMPANY}"
  WriteRegStr ${HKKEY} "${ARP}" "DisplayVersion" "${APPVERSON}"
  WriteRegDWORD ${HKKEY} "${ARP}" "NoModify" "1"
  WriteRegDWORD ${HKKEY} "${ARP}" "NoRepair" "1"
  WriteRegStr ${HKKEY} "${ARP}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  
 ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
 IntFmt $0 "0x%08X" $0
 WriteRegDWORD ${HKKEY} "${ARP}" "EstimatedSize" "$0"
 
 
 !ifdef DSCP
  WriteRegStr ${HKKEY} "Software\Policies\Microsoft\Windows\QoS" "Application DSCP Marking Request" "Allowed"

  WriteRegStr ${HKKEY} "Software\Policies\Microsoft\Windows\QoS\VoIP" "Version" "1.0"
  WriteRegStr ${HKKEY} "Software\Policies\Microsoft\Windows\QoS\VoIP" "Application Name" "${APPNAME}.exe"
  WriteRegStr ${HKKEY} "Software\Policies\Microsoft\Windows\QoS\VoIP" "Protocol" "*"
  WriteRegStr ${HKKEY} "Software\Policies\Microsoft\Windows\QoS\VoIP" "Local Port" "*"
  WriteRegStr ${HKKEY} "Software\Policies\Microsoft\Windows\QoS\VoIP" "Local IP" "*"
  WriteRegStr ${HKKEY} "Software\Policies\Microsoft\Windows\QoS\VoIP" "Local IP Prefix Length" "*"
  WriteRegStr ${HKKEY} "Software\Policies\Microsoft\Windows\QoS\VoIP" "Remote Port" "*"
  WriteRegStr ${HKKEY} "Software\Policies\Microsoft\Windows\QoS\VoIP" "Remote IP" "*"
  WriteRegStr ${HKKEY} "Software\Policies\Microsoft\Windows\QoS\VoIP" "Remote IP Prefix Length" "*"
  WriteRegStr ${HKKEY} "Software\Policies\Microsoft\Windows\QoS\VoIP" "DSCP Value" "51"
  WriteRegStr ${HKKEY} "Software\Policies\Microsoft\Windows\QoS\VoIP" "Throttle Rate" "-1"
  
  WriteRegStr ${HKKEY} "Software\Policies\Microsoft\Windows\QoS\WebPhone" "Version" "1.0"
  WriteRegStr ${HKKEY} "Software\Policies\Microsoft\Windows\QoS\WebPhone" "Application Name" "*"
  WriteRegStr ${HKKEY} "Software\Policies\Microsoft\Windows\QoS\WebPhone" "Protocol" "*"
  WriteRegStr ${HKKEY} "Software\Policies\Microsoft\Windows\QoS\WebPhone" "Local Port" "*"
  WriteRegStr ${HKKEY} "Software\Policies\Microsoft\Windows\QoS\WebPhone" "Local IP" "*"
  WriteRegStr ${HKKEY} "Software\Policies\Microsoft\Windows\QoS\WebPhone" "Local IP Prefix Length" "*"
  WriteRegStr ${HKKEY} "Software\Policies\Microsoft\Windows\QoS\WebPhone" "Remote Port" "*"
  WriteRegStr ${HKKEY} "Software\Policies\Microsoft\Windows\QoS\WebPhone" "Remote IP" "85.119.188.0"
  WriteRegStr ${HKKEY} "Software\Policies\Microsoft\Windows\QoS\WebPhone" "Remote IP Prefix Length" "24"
  WriteRegStr ${HKKEY} "Software\Policies\Microsoft\Windows\QoS\WebPhone" "DSCP Value" "51"
  WriteRegStr ${HKKEY} "Software\Policies\Microsoft\Windows\QoS\WebPhone" "Throttle Rate" "-1"

  !endif

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
!ifdef APPALLUSERS
  SetShellVarContext all
!endif
    ;Create shortcuts
    CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
!ifdef APPMINIMIZED
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\${APPLINKNAME}.lnk" "$INSTDIR\${APPNAME}.exe" /minimized
!else
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\${APPLINKNAME}.lnk" "$INSTDIR\${APPNAME}.exe"
!endif    
!ifndef APPMENULITE
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\License.lnk" "$INSTDIR\License.txt"
!endif    
!ifndef CUSTOM    
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\${APPNAME} Website.lnk" "$INSTDIR\${APPNAME} Website.url"
!endif    
!ifndef APPMENULITE
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
!endif    
  SetShellVarContext current
  !insertmacro MUI_STARTMENU_WRITE_END
  
SectionEnd

!ifdef NODESKTOP
Section /o "Desktop Shortcut" SecDesktopShortCut
!else
Section "Desktop Shortcut" SecDesktopShortCut
!endif
  WriteRegDWORD ${HKKEY} "Software\${APPNAME}" "DesktopShortcut" "1"

!ifdef APPALLUSERS
  SetShellVarContext all
!endif
!ifdef APPMINIMIZED
    CreateShortCut "$DESKTOP\${APPLINKNAME}.lnk" "$INSTDIR\${APPNAME}.exe" /minimized
!else
    CreateShortCut "$DESKTOP\${APPLINKNAME}.lnk" "$INSTDIR\${APPNAME}.exe"
!endif   
  SetShellVarContext current

SectionEnd

!ifndef AUTORUN
!ifndef CUSTOM
!define AUTORUN
!endif
!endif  

!ifdef AUTORUN
Section "Run at System Startup" SecStartupShortCut
!else
Section /o "Run at System Startup" SecStartupShortCut
!endif  

  WriteRegDWORD ${HKKEY} "Software\${APPNAME}" "RunAtSystemStartup" "1"

!ifdef APPALLUSERS
  SetShellVarContext all
!endif
  CreateShortCut "$SMSTARTUP\${APPLINKNAME}.lnk" "$INSTDIR\${APPNAME}.exe" /minimized
  SetShellVarContext current

SectionEnd

!ifdef NOASSOC
Section /o "Links association" SecAssociate
!else
Section "Links association" SecAssociate
!endif  

WriteRegDWORD ${HKKEY} "Software\${APPNAME}" "LinksAssociation" "1"

; for WinXP and Chrome?
DeleteRegKey ${HKKEY} "SOFTWARE\Classes\sip"
DeleteRegKey ${HKKEY} "SOFTWARE\Classes\tel"
DeleteRegKey ${HKKEY} "SOFTWARE\Classes\callto"

WriteRegStr ${HKKEY} "SOFTWARE\Classes\tel" "" "Internet Call Protocol"
WriteRegStr ${HKKEY} "SOFTWARE\Classes\tel" "URL Protocol" ""
WriteRegStr ${HKKEY} "SOFTWARE\Classes\tel" "Owner Name" "${APPNAME}"
WriteRegStr ${HKKEY} "SOFTWARE\Classes\tel\DefaultIcon" "" "$INSTDIR\${APPNAME}.exe,${APPICONINDEX}"
WriteRegStr ${HKKEY} "SOFTWARE\Classes\tel\shell\open\command" "" "$\"$INSTDIR\${APPNAME}.exe$\" $\"%1$\""

WriteRegStr ${HKKEY} "SOFTWARE\Classes\callto" "" "Internet Call Protocol"
WriteRegStr ${HKKEY} "SOFTWARE\Classes\callto" "URL Protocol" ""
WriteRegStr ${HKKEY} "SOFTWARE\Classes\callto" "Owner Name" "${APPNAME}"
WriteRegStr ${HKKEY} "SOFTWARE\Classes\callto\DefaultIcon" "" "$INSTDIR\${APPNAME}.exe,${APPICONINDEX}"
WriteRegStr ${HKKEY} "SOFTWARE\Classes\callto\shell\open\command" "" "$\"$INSTDIR\${APPNAME}.exe$\" $\"%1$\""

WriteRegStr ${HKKEY} "SOFTWARE\Classes\sip" "" "Internet Call Protocol"
WriteRegStr ${HKKEY} "SOFTWARE\Classes\sip" "URL Protocol" ""
WriteRegStr ${HKKEY} "SOFTWARE\Classes\sip" "Owner Name" "${APPNAME}"
WriteRegStr ${HKKEY} "SOFTWARE\Classes\sip\DefaultIcon" "" "$INSTDIR\${APPNAME}.exe,${APPICONINDEX}"
WriteRegStr ${HKKEY} "SOFTWARE\Classes\sip\shell\open\command" "" "$\"$INSTDIR\${APPNAME}.exe$\" $\"%1$\""

; for Vista and later
WriteRegStr ${HKKEY} "SOFTWARE\Microsoft\Windows\Shell\Associations\UrlAssociations\tel\UserChoice" "Progid" "${APPNAME}"
WriteRegStr ${HKKEY} "SOFTWARE\Microsoft\Windows\Shell\Associations\UrlAssociations\callto\UserChoice" "Progid" "${APPNAME}"
WriteRegStr ${HKKEY} "SOFTWARE\Microsoft\Windows\Shell\Associations\UrlAssociations\sip\UserChoice" "Progid" "${APPNAME}"

SectionEnd

;--------------------------------
;Installer Functions

Function .onInit

!ifdef INNER
  ; If INNER is defined, then we aren't supposed to do anything except write out
  ; the installer.  This is better than processing a command line option as it means
  ; this entire code path is not present in the final (real) installer.
 
  WriteUninstaller "$%TEMP%\Uninstall.exe"
  Quit  ; just bail out quickly when running the "inner" installer
!endif

  !insertmacro MUI_LANGDLL_DISPLAY

  ReadRegDWORD $0 ${HKKEY} "Software\${APPNAME}" "DesktopShortcut"
  StrCmp $0 "1" 0 +2
  SectionSetFlags ${SecDesktopShortCut} ${SF_SELECTED}
  StrCmp $0 "0" 0 +2
  SectionSetFlags ${SecDesktopShortCut} 0

  ReadRegDWORD $0 ${HKKEY} "Software\${APPNAME}" "RunAtSystemStartup"
  StrCmp $0 "1" 0 +2
  SectionSetFlags ${SecStartupShortCut} ${SF_SELECTED}
  StrCmp $0 "0" 0 +2
  SectionSetFlags ${SecStartupShortCut} 0

  ReadRegDWORD $0 ${HKKEY} "Software\${APPNAME}" "LinksAssociation"
  StrCmp $0 "1" 0 +2
  SectionSetFlags ${SecAssociate} ${SF_SELECTED}
  StrCmp $0 "0" 0 +2
  SectionSetFlags ${SecAssociate} 0

FunctionEnd

Function .onInstSuccess
${If} ${Silent}
  Call LaunchApp
${EndIf}
FunctionEnd

Function .closeProgram
  Exch $1
  Push $0
  loop:
    FindWindow $0 $1
    IntCmp $0 0 done
      #SendMessage $0 ${WM_DESTROY} 0 0
      SendMessage $0 ${WM_CLOSE} 0 0
    Sleep 100 
    Goto loop 
  done: 
  Pop $0 
  Pop $1
FunctionEnd

Function LaunchApp
; ExecShell "" "$INSTDIR\${APPNAME}.exe"
; Exec '"$WINDIR\explorer.exe" "$INSTDIR\${APPNAME}.exe"'
;  Exec '"$WINDIR\explorer.exe" "$SMPROGRAMS\$STARTMENU_FOLDER\${APPLINKNAME}.lnk"'
 Exec "$INSTDIR\${APPNAME}.exe"
FunctionEnd

;--------------------------------
;Uninstaller Section

Section "un.${APPNAME}" UninstallSec

  Push "${APPNAME}"
  Call un.closeProgram

  DeleteRegKey ${HKKEY} "${ARP}"

  Delete /REBOOTOK "$INSTDIR\ringin.wav"
  Delete /REBOOTOK "$INSTDIR\ringin2.wav"
  Delete /REBOOTOK "$INSTDIR\ringout.wav"
  Delete /REBOOTOK "$INSTDIR\hangup.wav"
  Delete /REBOOTOK "$INSTDIR\messageout.wav"
  Delete /REBOOTOK "$INSTDIR\messagein.wav"
  Delete /REBOOTOK "$INSTDIR\License.txt"
  Delete /REBOOTOK "$INSTDIR\${APPNAME}.exe"
  Delete /REBOOTOK "$INSTDIR\*.dll"
  Delete /REBOOTOK "$INSTDIR\${APPNAME}_log.txt"
  Delete /REBOOTOK "$INSTDIR\${APPNAME} Website.url"
  Delete /REBOOTOK "$INSTDIR\langpack_*"
  Delete /REBOOTOK "$INSTDIR\Uninstall.exe"

!ifdef APPALLUSERS
  SetShellVarContext all
!endif

  Delete "$DESKTOP\${APPLINKNAME}.lnk"
  Delete "$SMSTARTUP\${APPLINKNAME}.lnk"

  !insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP
  RMDir /r /REBOOTOK "$SMPROGRAMS\$MUI_TEMP"

  SetShellVarContext current

  RMDir /r /REBOOTOK "$LOCALAPPDATA\${APPNAME}"

  DeleteRegValue ${HKKEY} "SOFTWARE\RegisteredApplications" "${APPNAME}"
  DeleteRegKey ${HKKEY} "SOFTWARE\Classes\${APPNAME}"
  DeleteRegKey ${HKKEY} "SOFTWARE\${APPNAME}"

  !insertmacro RemoveLinksAssociation

SectionEnd

Section /o "un.Configuration" UnSecConfig

  RMDir /r /REBOOTOK "$APPDATA\${APPNAME}"
  RMDir /r /REBOOTOK "$INSTDIR"
   
SectionEnd

;--------------------------------
;Uninstaller Functions

Function un.onInit

  !insertmacro MUI_UNGETLANGUAGE
  
FunctionEnd

Function un.closeProgram
  Exch $1
  Push $0
  loop:
    FindWindow $0 $1
    IntCmp $0 0 done
      #SendMessage $0 ${WM_DESTROY} 0 0
      SendMessage $0 ${WM_CLOSE} 0 0
    Sleep 100 
    Goto loop 
  done: 
  Pop $0 
  Pop $1
FunctionEnd

;--------------------------------
;Descriptions

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecInstall} "${APPNAME}"
    !insertmacro MUI_DESCRIPTION_TEXT ${SecDesktopShortCut} "Create Desktop Shortcut"
    !insertmacro MUI_DESCRIPTION_TEXT ${SecStartupShortCut} "Create Startup Shortcut"
    !insertmacro MUI_DESCRIPTION_TEXT ${SecAssociate} "Associate with the links tel: callto: and sip:"
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
