@echo off
rem Script to build the language dlls

SETLOCAL
set PATH=c:\programme\nsis;c:\tools
set OFile=product.nsh

del bin\Tortoise*.dll
del bin\LanguagePack*.exe
FOR /F "eol=# delims=; tokens=1,2,3,4,5,6" %%i in (Languages.txt) do call :doit %%i %%j %%k %%l %%m %n

:end
endlocal
goto :eof

:doit
echo.
echo Building %5 dlls and installer
restext apply TortoiseProcLang.dll bin\TortoiseProc%2.dll Tortoise_%1.po %2
restext apply TortoiseMergeLang.dll bin\TortoiseMerge%2.dll Tortoise_%1.po %2

echo !define PRODUCT_NAME "TortoiseSVN %6" > %OFile%
echo !define CountryCode "%1" >> %OFile%
echo !define CountryID "%2" >> %OFile%
echo !define InstLang "%4" >> %OFile%
if %3 EQU 1 echo !define LangHelp >> %OFile%
MakeNSIS LanguagePack.nsi
goto :eof
