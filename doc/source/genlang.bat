@echo off
SETLOCAL ENABLEDELAYEDEXPANSION

@if "%VSINSTALLDIR%"=="" call "%VS71COMNTOOLS%\vsvars32.bat"

if "%TortoiseVars%"=="" call ..\TortoiseVars.bat

echo generating translated docs for "%1"

cd ..
rmdir /s /q source\"%1"
mkdir source\"%1"
mkdir source\"%1"\tsvn_dug
mkdir source\"%1"\tsvn_repository
mkdir source\"%1"\tsvn_server

cd source\en
FOR %%U In (*.xml) Do Set XMLFILES=!XMLFILES! %%U
cd ..\..
FOR %%V In (%XMLFILES%) Do xml2po.py -p po\\"%1" --output=source\\"%1"\%%V source\en\%%V 

cd source\en\tsvn_dug
Set XMLFILES=""
FOR %%U In (*.xml) Do Set XMLFILES=!XMLFILES! %%U
cd ..\..\..
FOR %%V In (%XMLFILES%) Do xml2po.py -p po\\"%1" --output=source\\"%1"\tsvn_dug\%%V source\en\tsvn_dug\%%V 

cd source\en\tsvn_repository
Set XMLFILES=""
FOR %%U In (*.xml) Do Set XMLFILES=!XMLFILES! %%U
cd ..\..\..
FOR %%V In (%XMLFILES%) Do xml2po.py -p po\\"%1" --output=source\\"%1"\tsvn_repository\%%V source\en\tsvn_repository\%%V 

cd source\en\tsvn_server
Set XMLFILES=""
FOR %%U In (*.xml) Do Set XMLFILES=!XMLFILES! %%U
cd ..\..\..
FOR %%V In (%XMLFILES%) Do xml2po.py -p po\\"%1" --output=source\\"%1"\tsvn_server\%%V source\en\tsvn_server\%%V 

cd po