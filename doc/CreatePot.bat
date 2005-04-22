@echo off
SETLOCAL ENABLEDELAYEDEXPANSION

rem BEWARE, this may cause a "line too long" error if called repeatedly from the same dos box
rem Try to check, whether these vars are already set
@if "%VSINSTALLDIR%"=="" call "%VS71COMNTOOLS%\vsvars32.bat"

if "%TortoiseVars%"=="" call ..\TortoiseVars.bat

cd source\en
del Pubdate.xml
FOR /F "usebackq" %%V IN (`dir /b /on *.xml`) DO Set CHAPTERS=!CHAPTERS! source/en/%%V
cd tsvn_dug
FOR /F "usebackq" %%V IN (`dir /b /on *.xml`) Do Set CHAPTERS=!CHAPTERS! source/en/tsvn_dug/%%V
cd ..\tsvn_repository
FOR /F "usebackq" %%V IN (`dir /b /on *.xml`) Do Set CHAPTERS=!CHAPTERS! source/en/tsvn_repository/%%V
cd ..\..\..

xml2po.py -o po/doc.pot !CHAPTERS!
