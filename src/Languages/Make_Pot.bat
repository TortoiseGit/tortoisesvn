@echo off
SETLOCAL ENABLEDELAYEDEXPANSION

if "%TortoiseVars%"=="" call ..\..\TortoiseVars.bat
..\..\bin\release\ResText extract TortoiseProcLang.dll TortoiseMergeLang.dll Tmp.pot
msgremove Tmp.pot -i ignore.po -o Tortoise.pot
del Tmp.pot

rem leave this line commented. I fear it destroys the asian .po files
rem FOR %%i in (*.po) do msgmerge --no-wrap -s %%i Tortoise.pot -o %%i

ENDLOCAL
goto :eof
