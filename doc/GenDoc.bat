@echo off
SETLOCAL ENABLEDELAYEDEXPANSION

rem BEWARE, this may cause a "line too long" error if called repeatedly from the same dos box
rem Try to check, whether these vars are already set
if "%VSINSTALLDIR%"=="" call "%VS80COMNTOOLS%\vsvars32.bat"
if "%TortoiseVars%"=="" call ..\TortoiseVars.bat

set APPS=TortoiseSVN TortoiseMerge
set TARGETS=pdf chm html

if "%IGNORELIST%"=="" call IgnoreList.bat

rmdir /s /q output > NUL
mkdir output > NUL

rem check if the version.bat script has been run (i.e. if the version.xml file
rem is present). If not, just copy version.in to version.xml
if NOT EXIST source\en\Version.xml (copy source\en\Version.in source\en\Version.xml)

rem Determine available languages for documentation by looking for the .po files

if "%1"=="" (
  FOR %%L IN (po\*.po) DO (
    CALL :doit %%~nL %~dp0
  )
  CALL :doit en %~dp0
) else (
  CALL :doit %1 %~dp0
)

Goto :EOF
rem | End of Batch execution -> Exit script
rem +----------------------------------------------------------------------

:doit

echo.
echo Generating Documentation for: %APPS%
echo Language: %1 %2
echo Targets: %TARGETS%
echo ----------------------------------------------------------------------
echo.

rem do *NOT* create english translation from english :-)

if NOT %1 EQU en (
  call TranslateDoc.bat %1
)

echo ----------------------------------------------------------------------
echo.
echo Generating: %1 %APPS%
cd source
FOR %%A in (%APPS%) DO call MakeDoc %%A %1 %TARGETS%
cd ..

Goto :EOF
rem | End of Subroutine -> return to caller
rem +----------------------------------------------------------------------
