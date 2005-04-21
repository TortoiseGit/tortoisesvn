@echo off
SETLOCAL ENABLEDELAYEDEXPANSION

rem BEWARE, this may cause a "line too long" error if called repeatedly from the same dos box
@echo off

if "%1"=="" (
  echo Usage:   %~n0 [two letter language code]
  echo Example: %~n0 de
  goto :EOF
)

rem Try to check, whether these vars are already set
if "%VSINSTALLDIR%"=="" call "%VS71COMNTOOLS%\vsvars32.bat"
if "%TortoiseVars%"=="" call ..\TortoiseVars.bat

SET POFILE=po\%1.po
SET SRCDIR=source\en
SET TARGDIR=%~dp0source\%1

rmdir /s /q %TARGDIR% > NUL
mkdir %TARGDIR% > NUL

rem --------------------
rem Collect files
rem No real recursion, only one level deep

cd %SRCDIR%

FOR %%F IN (*.xml) DO (
  SET CHAPTERS=!CHAPTERS! %%F
)

FOR /D %%D IN (*) DO (
  mkdir %TARGDIR%\%%D

  FOR %%F IN (%%D\*.xml) DO (
    SET CHAPTERS=!CHAPTERS! %%F
  )
)

cd %~dp0

rem po File has to be copied to the same dir as xml2po.py
rem otherwise the path to the po file will be in the translated docs.

copy %POFILE% .

FOR %%F in (!CHAPTERS!) DO (
  echo %%F
  xml2po.py -p %1.po %SRCDIR%\%%F > %TARGDIR%\%%F
)

del %1.po
