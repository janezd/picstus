Name "Picstus"
;Icon OrangeInstall.ico
; UninstallIcon OrangeInstall.ico

OutFile "PicstusInstallation.exe"
licensedata license.txt
licensetext "License Agreement"

AutoCloseWindow true
ShowInstDetails nevershow
SilentUninstall silent

!include "LogicLib.nsh"

Var SicstusDir
Var PythonDir

Page license
Page instfiles

Section ""
  HideWindow
  ReadRegStr $SicstusDir HKLM "SOFTWARE\SICS\sicstus3.12_win32" "SP_PATH"
  ReadRegStr $PythonDir HKLM "SOFTWARE\Python\PythonCore\2.3\InstallPath" ""

	${If} $SicstusDir == ""
    ${If} $PythonDir == ""
      MessageBox MB_OK "Please install Sicstus 3.12 and Python 2.3 first and run the installation again."
      goto abort
    ${Else}
      MessageBox MB_OK "Please install Sicstus 3.12 first and run the installation again."
      goto abort
    ${EndIf}
  ${EndIf}

	${If} $PythonDir == ""
    MessageBox MB_OK "Please install Python 2.3 first and run the installation again."
    goto abort
  ${EndIf}
  

	SetOutPath "$SicstusDir\library"
  File python.dll
  File python.pl

  SetOutPath "$PythonDir\lib\site-packages"
  File picstus.py

  CreateDirectory "$SicstusDir\doc\html\picstus"
  SetOutPath "$SicstusDir\doc\html\picstus"
  File picstus.html

  CreateDirectory "$SMPROGRAMS\Picstus\"
	CreateShortCut "$SMPROGRAMS\Picstus\Picstus Manual.lnk" "$SicstusDir\doc\html\picstus\picstus.html"

  WriteRegStr HKLM "SOFTWARE\Python\PythonCore\2.3\PythonPath\Picstus" "" "$SicstusDir\library"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Picstus" "DisplayName" "Picstus (remove only)"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Picstus" "UninstallString" '"$SicstusDir\bin\UninstallPicstus.exe"'

  WriteUninstaller "$SicstusDir\bin\UninstallPicstus.exe"

  MessageBox MB_OK "Picstus has been successfully installed."
abort:
SectionEnd


Section Uninstall
  HideWindow
	MessageBox MB_YESNO "Are you sure you want to remove Picstus?" IDNO abort
  
  DeleteRegKey HKLM "SOFTWARE\Python\PythonCore\2.3\PythonPath\Picstus"
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Picstus"

	RmDir /R "$SMPROGRAMS\Picstus"

  Delete "$INSTDIR\..\library\python.pl"
  Delete "$INSTDIR\..\library\python.dll"
  RmDir /R "$INSTDIR\..\doc\html\picstus"
  Delete "UninstallPicstus.exe"

  ReadRegStr $PythonDir HKLM "SOFTWARE\Python\PythonCore\2.3\InstallPath" ""
  Delete "$PythonDir\lib\site-packages\library\picstus.py"

	MessageBox MB_OK "Picstus has been succesfully removed from your system."

abort:
SectionEnd
