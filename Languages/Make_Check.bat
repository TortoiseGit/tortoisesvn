@echo off
rem Script to build the language dlls

SETLOCAL ENABLEDELAYEDEXPANSION
if "%TortoiseVars%"=="" call ..\TortoiseVars.bat
set LogFile=statusreport.txt
set LotsOfBlanks="                              "

rem Count all messages in PO Template file
FOR /F "usebackq" %%p IN (`Check_Attrib.bat Tortoise.pot`) DO SET total=%%p
FOR /F "usebackq" %%p IN (`svnversion .`) DO SET version=%%p

copy Tortoise_*.po _Tortois_*.po /Y 2>NUL
echo.
echo Merging translations with latest .pot file
echo ------------------------------------------
FOR %%i in (_Tortois_*.po) DO (
  echo %%i
  msgmerge --no-wrap --quiet --no-fuzzy-matching -s %%i Tortoise.pot -o %%i 2> NUL
)

echo. > %LogFile%
echo Translation Status report for r!version:~0,4! >> %LogFile%
echo ----------------------------------- >> %LogFile%
echo Total=!total! >> %LogFile%
echo. >> %LogFile%
echo Language : translated/fuzzy/untranslated/missing accelerator keys >> %LogFile%
echo. >> %LogFile%

echo.
echo Translation Status report for r!version:~0,4!
echo ------------------------------------------
echo Total=!total!
echo.
echo Language : translated/fuzzy/untranslated/missing accelerator keys
echo.

rem !!! ATTENTION 
rem !!! There is a real TAB key inside "delims=	;"
rem !!! Please leave it there

FOR /F "eol=# delims=	; tokens=1,5" %%i IN (Languages.txt) DO (

  SET POFILE=_Tortois_%%i.po
  SET LANGNAME=%%j ^(%%i^)%LotsOfBlanks:~1,30%
  SET LANGNAME=!LANGNAME:~0,30! :

  if exist !POFILE! (
    set errors=0
    set tra=0
    set unt=0
    set fuz=0
    set obs=0

    FOR /F "usebackq skip=1 " %%p IN (`Check_Accel.bat !POFILE!`) DO SET errors=%%p
    FOR /F "usebackq" %%p IN (`Check_Attrib.bat --translated --no-obsolete !POFILE!`) DO SET tra=%%p
    FOR /F "usebackq" %%p IN (`Check_Attrib.bat --only-fuzzy --no-obsolete !POFILE!`) DO SET fuz=%%p
    FOR /F "usebackq" %%p IN (`Check_Attrib.bat --untranslated --no-obsolete !POFILE!`) DO SET unt=%%p

    SET /A errsum=!fuz!+!unt!+!errors!

    if !errsum! EQU 0 (
      echo !LANGNAME! !total! - OK
      echo !LANGNAME! !total! - OK >> %LogFile% 
    ) else (
      echo !LANGNAME! !tra!/!fuz!/!unt!/!errors!
      echo !LANGNAME! !tra!/!fuz!/!unt!/!errors! >> %LogFile% 
    )
  ) ELSE (
    echo !LANGNAME! !POFILE! not found
  )

)

del _Tortois_*.po /Q
del *.mo

:end
ENDLOCAL
goto :eof
