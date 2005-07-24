@echo off
@if "%VSINSTALLDIR%"=="" call "%VS71COMNTOOLS%\vsvars32.bat"
if "%TortoiseVars%"=="" call ..\..\TortoiseVars.bat
SETLOCAL ENABLEDELAYEDEXPANSION
..\..\bin\release\bin\SubWCRev.exe ..\.. Setup.wxs Setup_good.wxs
if NOT EXIST Setup_good.wxs (copy Setup.wxs Setup_good.wxs)
..\..\bin\release\bin\SubWCRev.exe ..\.. makemsisub.in makemsisub.bat
if NOT EXIST makemsisub.bat (copy makemsisub.in makemsisub.bat)
rem include the spell checker and thesaurus if they're available
set ENSPELL=0
set ENGBSPELL=0
if EXIST ..\..\..\Common\Spell\en_US.aff (set ENSPELL=1)
if EXIST ..\..\..\Common\Spell\en_GB.aff (set ENGBSPELL=1)
rem don't include the crashreport dll if the build is not done by myself.
rem Since I don't have the debug symbols of those builds, crashreports sent
rem to me are unusable.
set MYBUILD=0
if EXIST ..\..\MYBUILD (set MYBUILD=1)
call makemsisub.bat
del makemsisub.bat