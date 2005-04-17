@echo off
SETLOCAL ENABLEDELAYEDEXPANSION

@if "%VSINSTALLDIR%"=="" call "%VS71COMNTOOLS%\vsvars32.bat"

if "%TortoiseVars%"=="" call ..\..\TortoiseVars.bat

cd ..\po

FOR %%V In (*.po) Do Call ..\source\genlang.bat %%V

cd ..
del .xml2po.mo
