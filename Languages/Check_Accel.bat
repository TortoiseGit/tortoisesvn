@echo off
SETLOCAL
if "%TortoiseVars%"=="" call ..\TortoiseVars.bat
msgfmt --check-accelerators %1 2>&1 | grep -c "Tortoise"
ENDLOCAL