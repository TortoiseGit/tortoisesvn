@echo off
SETLOCAL ENABLEDELAYEDEXPANSION

rem BEWARE, this may cause a "line too long" error if called repeatedly from the same dos box
rem Try to check, whether these vars are already set
if "%VSINSTALLDIR%"=="" call "%VS71COMNTOOLS%\vsvars32.bat"
if "%TortoiseVars%"=="" call ..\TortoiseVars.bat

SET LOGFILE=translatelog.txt

echo. >%LOGFILE%

if "%1"=="" (
  FOR %%L IN (po\*.po) DO (
    CALL :doit %%~nL
  )
) else (
  CALL :doit %1
)

Goto :EOF
rem | End of Batch execution -> Exit script
rem +----------------------------------------------------------------------

:doit
echo.
echo Translating: %1
SET POFILE=po\%1.po
SET SRCDIR=source\en
SET TARGDIR=%~dp0source\%1

rmdir /s /q %TARGDIR% > NUL
mkdir %TARGDIR% > NUL

rem --------------------
rem Collect files
rem No real recursion, only one level deep

cd %SRCDIR%

SET CHAPTERS=

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

copy %POFILE% . > NUL

FOR %%F in (!CHAPTERS!) DO (
  echo %%F
  echo ---------------------------------------------------------------------->>%LOGFILE%
  echo %%F >>%LOGFILE%

  rem Translate file
  xml2po.py -p %1.po %SRCDIR%\%%F > %TARGDIR%\%%F

  rem Run spellchecker on file. Uncomment if you don't want it.
  aspell --mode=sgml --encoding=utf-8 -l -d %1 < %TARGDIR%\%%F >>%LOGFILE%
)

del %1.po

echo.
echo Temporary solution
echo Ugly hack to prevent the automation appendix from being translated.
echo Copying tsvn_app_automation.xml from english source
copy %SRCDIR%\tsvn_app_automation.xml %TARGDIR%

Goto :EOF
rem | End of Subroutine -> return to caller
rem +----------------------------------------------------------------------
