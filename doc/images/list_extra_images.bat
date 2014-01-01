@echo off
setlocal

if %1x==x goto usage
set lang=%1
for %%i in (%lang%\*.png) do call :check en %%~nxi
goto :eof

:usage
echo Usage: %~nx0 language
echo This script lists all images that are present for 'language' and not in English
echo Example: %~nx0 fi
goto :eof

:check
if not exist %1\%2 echo %2
goto :eof
