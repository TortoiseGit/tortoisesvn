@echo off
echo Updating doc.pot
SETLOCAL ENABLEDELAYEDEXPANSION

rem BEWARE, this may cause a "line too long" error if called repeatedly from the same dos box
rem Try to check, whether these vars are already set
@if "%VSINSTALLDIR%"=="" call "%VS80COMNTOOLS%\vsvars32.bat"

if "%TortoiseVars%"=="" call ..\TortoiseVars.bat

if "%IGNORELIST%"=="" call IgnoreList.bat

rem --------------------
rem Collect files
rem No real recursion, only one level deep

cd source\en
if exist Pubdate.xml del Pubdate.xml

FOR /F "usebackq" %%F IN (`dir /b /on *.xml`) DO (
  SET IGNORED=0
  FOR %%I IN (%IGNORELIST%) do (
    IF %%I == %%F SET IGNORED=1
  )
  IF !IGNORED! NEQ 1 SET CHAPTERS=!CHAPTERS! source/en/%%F
)

FOR /F "usebackq" %%D IN (`dir /B /ON *.`) DO (
  FOR /F "usebackq" %%F IN (`dir /b /on %%D\*.xml`) DO (

    SET IGNORED=0
    FOR %%I IN (%IGNORELIST%) do (
      IF %%I == %%F SET IGNORED=1
    )
    IF !IGNORED! NEQ 1 SET CHAPTERS=!CHAPTERS! source/en/%%D/%%F
  )
)

cd ..\..

xml2po.py -o po/doc.pot !CHAPTERS!
