@echo off
SETLOCAL ENABLEDELAYEDEXPANSION

rem BEWARE, this may cause a "line too long" error if called repeatedly from the same dos box
rem Try to check, whether these vars are already set
@if "%VSINSTALLDIR%"=="" call "%VS71COMNTOOLS%\vsvars32.bat"

set APPS=TortoiseSVN TortoiseMerge
set TARGETS=pdf chm html

rmdir /s /q output > NUL
mkdir output > NUL

rem *** Determine available languages for documentation
cd source
set LANG=
FOR /D %%V In (*.*) Do set LANG=!LANG! %%V

echo.
echo Just Do It !!!
echo ----------------------------------------------------------------------
echo.
echo Generating Documentation for: %APPS%
echo Generating Languages: %LANG%
echo Generating Targets: %TARGETS%

rem *** loop over every available app and language
FOR %%A in (%APPS%) DO FOR %%L In (%LANG%) DO call MakeDoc %%A %%L %TARGETS%

cd ..

rem Put the stuff that copies the finished docs into the release and debug folders here