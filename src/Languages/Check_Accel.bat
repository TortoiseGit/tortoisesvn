@echo off
SETLOCAL
set PATH=c:\programme\dxgettext;c:\tools
msgfmt --check-accelerators %1 2>&1 | grep -c "Tortoise"
ENDLOCAL
