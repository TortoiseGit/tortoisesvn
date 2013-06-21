@echo off
rem run this batch file to create the website
rem you have to set the ZIP variable to the path of a zip tool, e.g.:
set ZIP="%PROGRAMFILES%\7-zip\7z.exe"

if exist ..\website rd /s /q ..\website
pushd scripts
python generatesite.py
popd

pushd ..\website
if exist website.zip del website.zip

%ZIP% a -r -tzip website.zip * > NUL
move website.zip ..
popd

rd /s /q ..\website
