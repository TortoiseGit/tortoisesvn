@echo off
SETLOCAL ENABLEDELAYEDEXPANSION

rem BEWARE, this may cause a "line too long" error if called repeatedly from the same dos box
rem Try to check, whether these vars are already set
if "%VSINSTALLDIR%"=="" call "%VS71COMNTOOLS%\vsvars32.bat"
if "%TortoiseVars%"=="" call ..\TortoiseVars.bat

if "%IGNORELIST%"=="" call IgnoreList.bat

SET LOGFILE=translatelog.txt

copy %LOGFILE% %LOGFILE%.bak
echo.>%LOGFILE%

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
echo Ignoring: %IGNORELIST%
echo.
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
  
  SET IGNORED=0
  FOR %%I IN (%IGNORELIST%) do (
    IF %%I == %%F SET IGNORED=1
  )
  IF !IGNORED! NEQ 1 SET CHAPTERS=!CHAPTERS! %%F
)

FOR /D %%D IN (*) DO (
  mkdir %TARGDIR%\%%D

  FOR %%F IN (%%D\*.xml) DO (

    SET IGNORED=0
    FOR %%I IN (%IGNORELIST%) do (
      IF %%I == %%F SET IGNORED=1
    )
    IF !IGNORED! NEQ 1 SET CHAPTERS=!CHAPTERS! %%F
  )
)

cd %~dp0

rem po File has to be copied to the same dir as xml2po.py
rem otherwise the path to the po file will be in the translated docs.

copy %POFILE% . > NUL

FOR %%F in (!CHAPTERS!) DO (
  echo %%F
  echo.>>%LOGFILE%
  echo ---------------------------------------------------------------------->>%LOGFILE%
  echo %%F >>%LOGFILE%
  echo ---------------------------------------------------------------------->>%LOGFILE%

  rem Translate file
  xml2po.py -p %1.po %SRCDIR%\%%F > %TARGDIR%\%%F

  rem Run spellchecker on file. Uncomment if you don't want it.
  aspell --mode=sgml --encoding=utf-8 -l -d %1 < %TARGDIR%\%%F >>%LOGFILE%
)

del %1.po

echo.
echo Copying files which should not be translated (%IGNORELIST%) from english source

FOR %%F in (%IGNORELIST%) DO copy %SRCDIR%\%%F %TARGDIR%

Goto :EOF
rem | End of Subroutine -> return to caller
rem +----------------------------------------------------------------------
