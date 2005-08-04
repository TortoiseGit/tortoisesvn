@echo off
rem Check the environment
@if "%VSINSTALLDIR%"=="" call "%VS71COMNTOOLS%\vsvars32.bat"
if "%TortoiseVars%"=="" call ..\..\TortoiseVars.bat
SETLOCAL ENABLEDELAYEDEXPANSION

rem Do the SubWCRev substitution for revision numbers in the two necessary files.
..\..\bin\release\bin\SubWCRev.exe ..\.. VersionNumberInclude.in.wxi VersionNumberInclude.wxi
if NOT EXIST VersionNumberInclude.wxi (copy VersionNumberInclude.in.wxi VersionNumberInclude.wxi)
..\..\bin\release\bin\SubWCRev.exe ..\.. MakeMsiSub.in.bat MakeMsiSub.bat
if NOT EXIST MakeMsiSub.bat (copy MakeMsiSub.in.bat MakeMsiSub.bat)

rem Build the actual setup package.
call MakeMsiSub.bat
del MakeMsiSub.bat
del VersionNumberInclude.wxi