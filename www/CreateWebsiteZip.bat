@echo off
setlocal

rem run this batch file to create the website
rem you have to set the ZIP variable to the path of a zip tool, e.g.:
set ZIP="%PROGRAMFILES%\7-zip\7z.exe"

if not exist "node_modules" cmd /c npm install
cmd /c grunt build

if exist website.zip del website.zip

pushd dist
%ZIP% a -r -tzip ..\website.zip * > NUL
popd

endlocal
exit /b
