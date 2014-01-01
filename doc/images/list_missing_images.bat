@echo off
setlocal

if %1x==x goto usage
set lang=%1
for %%i in (en\*.png) do call :check %lang% %%~nxi
goto :eof

:usage
echo Usage: %~nx0 language
echo This script lists all english images that are absent for 'language'
echo Example: %~nx0 fi
goto :eof

:check
if not exist %1\%2 echo %2
goto :eof
