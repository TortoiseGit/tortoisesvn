@echo off
SETLOCAL
if "%TortoiseVars%"=="" call ..\..\TortoiseVars.bat
msgattrib --%2 %1 2>nul | grep -c msgid 
ENDLOCAL