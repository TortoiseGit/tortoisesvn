@echo off

echo Script to build the language dlls
echo.
echo usages %0 [lang]
echo.
echo You can either build all language dlls by specifying no parameter
echo or one language pack by specifying the language to build
echo.

SETLOCAL ENABLEDELAYEDEXPANSION

if "%TortoiseVars%"=="" call ..\TortoiseVars.bat
set OFile=product.nsh

if "%1"=="" (
  del ..\bin\Tortoise*.dll
  del ..\bin\LanguagePack*.exe
)

rem !!! ATTENTION 
rem !!! There is a real TAB key inside "delims=	;"
rem !!! Please leave it there

FOR /F "eol=# tokens=1,2,3,4,5,6 delims=	;" %%i IN (Languages.txt) DO (

  set BUILD=1
  if NOT "%1"=="" (
    if NOT "%1"=="%%i" set BUILD=0
  )

  if !BUILD! EQU 1 (
    echo.
    echo Building %%m dlls and installer
    ..\bin\release\bin\restext apply TortoiseProcLang.dll ..\bin\TortoiseProc%%j.dll Tortoise_%%i.po 
    ..\bin\release\bin\restext apply TortoiseMergeLang.dll ..\bin\TortoiseMerge%%j.dll Tortoise_%%i.po 

    echo ^^!define PRODUCT_NAME "TortoiseSVN %%n" > %OFile%
    echo ^^!define CountryCode "%%i" >> %OFile%
    echo ^^!define CountryID "%%j" >> %OFile%
    echo ^^!define InstLang "%%l" >> %OFile%
    if %%k EQU 1 echo ^^!define LangHelp >> %OFile%

    if EXIST ..\..\Subversion\Subversion\po\%%i.po (
      msgfmt ..\..\Subversion\Subversion\po\%%i.po -o subversion.mo -f
      if EXIST subversion.mo echo ^^!define MoFile "subversion.mo" >> %OFile%
    )

	if NOT EXIST LanguagePack.nsi (copy LanguagePack.in LanguagePack.nsi)
    MakeNSIS /V1 LanguagePack.nsi
    del ..\bin\TortoiseProc%%j.dll
    del ..\bin\TortoiseMerge%%j.dll
    if EXIST subversion.mo del subversion.mo
  )

)

ENDLOCAL
