@echo off

echo Script to build the language dlls
echo.
echo usages Make_Installer [lang]
echo.
echo You can either build all language dlls by specifying no parameter
echo or one language pack by specifying the language to build
echo.

SETLOCAL ENABLEDELAYEDEXPANSION

if "%TortoiseVars%"=="" call ..\TortoiseVars.bat
set OFile=product.nsh

del ..\bin\Tortoise*.dll
del ..\bin\LanguagePack*.exe

FOR /F "eol=# tokens=1,2,3,4,5,6 delims=;" %%i in (Languages.txt) do call :doit %%i %%j %%k %%l "%%m" "%%n" %1

:end
endlocal
goto :eof

:doit
if NOT "%7"=="" (
  if NOT "%7"=="%1" goto :eof
)

echo.
echo Building %5 dlls and installer
..\bin\release\bin\restext apply TortoiseProcLang.dll ..\bin\TortoiseProc%2.dll Tortoise_%1.po 
..\bin\release\bin\restext apply TortoiseMergeLang.dll ..\bin\TortoiseMerge%2.dll Tortoise_%1.po 

echo ^^!define PRODUCT_NAME "TortoiseSVN %~6" > %OFile%
echo ^^!define CountryCode "%1" >> %OFile%
echo ^^!define CountryID "%2" >> %OFile%
echo ^^!define InstLang "%4" >> %OFile%
if %3 EQU 1 echo ^^!define LangHelp >> %OFile%

if EXIST ..\..\Subversion\Subversion\po\%1.po (
msgfmt ..\..\Subversion\Subversion\po\%1.po -o subversion.mo -f
if EXIST subversion.mo echo ^^!define MoFile "subversion.mo" >> %OFile%
)

MakeNSIS /V1 LanguagePack.nsi
del ..\bin\TortoiseProc%2.dll
del ..\bin\TortoiseMerge%2.dll
if EXIST subversion.mo del subversion.mo
goto :eof