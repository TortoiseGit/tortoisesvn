@echo off
if "%TortoiseVars%"=="" call ..\..\TortoiseVars.bat
..\..\bin\release\ResText extract TortoiseProcLang.dll TortoiseMergeLang.dll Tmp.pot
msgremove Tmp.pot -i ignore.po -o Tortoise.pot
del Tmp.pot