!ifndef FindTSVN_INCLUDED

!define FindTSVN_INCLUDED

Var TSVNPATH

!macro FindTSVNPath

  ReadRegStr $0 HKLM "software\TortoiseSVN" "Directory"
  StrCmp $0 "" NotInstalled Installed
Installed:
  Goto Proceed
NotInstalled:
  MessageBox MB_OK "TortoiseSVN not installed!"
  Abort
Proceed:
  StrCpy $TSVNPATH $0"

!macroend

!endif
