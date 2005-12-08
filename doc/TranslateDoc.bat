@echo off

if [%1]==[--help] (
  echo.
  echo Usage: %0 [-s | --spellcheck] [Language]
  echo.
  echo Translates docs into the given Language where Language is the ISO-639-1 language code.
  echo If no language is given, all .po files in the po directory are used for translation.
  echo.
  echo   --spellcheck       Uses aspell to spellcheck the translation.
  exit /B
)


SETLOCAL ENABLEDELAYEDEXPANSION

rem BEWARE, this may cause a "line too long" error if called repeatedly from the same dos box
rem Try to check, whether these vars are already set
if "%VSINSTALLDIR%"=="" call "%VS80COMNTOOLS%\vsvars32.bat"
if "%TortoiseVars%"=="" call ..\TortoiseVars.bat
if "%IGNORELIST%"=="" call IgnoreList.bat

set SPELL=0
if [%1]==[-s] (
  set SPELL=1
  shift
)
if [%1]==[--spellcheck] (
  set SPELL=1
  shift
)

if [%1]==[] (
  FOR %%L IN (po\*.po) DO (
    CALL :doit %%~nL
  )
) else (
  CALL :doit %1
)

Goto :EOF
rem | End of Batch execution -> Exit script
rem +----------------------------------------------------------------------


rem +----------------------------------------------------------------------
rem | Main subroutine

:doit

echo.
echo Translating  : %1
if %SPELL% EQU 1 (
  echo Spellchecking: activated
) else (
  echo Spellchecking: off
)
echo Ignoring     : %IGNORELIST%
echo.

SET POFILE=po\%1.po
SET SRCDIR=source\en
SET TARGDIR=%~dp0source\%1
SET LOGFILE=Translate_%1.log

copy %LOGFILE% %LOGFILE%.bak >NUL
echo.>%LOGFILE%

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

msgfmt %POFILE% -o %1.mo

FOR %%F in (!CHAPTERS!) DO (
  echo %%F
  echo.>>%LOGFILE%
  echo ---------------------------------------------------------------------->>%LOGFILE%
  echo %%F >>%LOGFILE%
  echo ---------------------------------------------------------------------->>%LOGFILE%

  rem Translate file
  xml2po.py -t %1.mo %SRCDIR%\%%F > %TARGDIR%\%%F

  if %SPELL% EQU 1 (
    rem Run spellchecker on file. Uncomment if you don't want it.
    aspell --mode=sgml --encoding=utf-8 -l -d %1 < %TARGDIR%\%%F >>%LOGFILE%
  )
)

del %1.mo

echo.
echo Copying files which should not be translated (%IGNORELIST%) from english source

FOR %%F in (%IGNORELIST%) DO copy %SRCDIR%\%%F %TARGDIR%

Goto :EOF
rem | End of Subroutine -> return to caller
rem +----------------------------------------------------------------------
