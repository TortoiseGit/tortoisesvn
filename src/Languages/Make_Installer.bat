rem Script to build the language dlls

@echo off
SETLOCAL
set PATH=c:\programme\nsis;c:\tools
FOR /F "tokens=1,2*" %%i in (Languages.txt) do call :doit %%i %%j %%k

:end
endlocal
goto :eof

:doit
echo Building %3 dlls and installer
restext apply bin\TortoiseProcLang.dll bin\TortoiseProc%2.dll Tortoise_%1.po
restext apply bin\TortoiseMergeLang.dll bin\TortoiseMerge%2.dll Tortoise_%1.po
rem MakeNSIS LanguagePack_%1.nsi
goto :eof
