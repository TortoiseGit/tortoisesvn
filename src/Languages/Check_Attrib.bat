@echo off
SETLOCAL
set PATH=c:\programme\dxgettext;c:\tools
msgattrib --%2 %1 2>nul | grep -c msgid 
ENDLOCAL
