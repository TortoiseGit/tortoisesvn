@echo off
rem Script to calculate the GUI translation status report for TortoiseSVN

SETLOCAL ENABLEDELAYEDEXPANSION

rem Trunk and branch location. 
rem Without slash, because they're not only used for directories
set Trunk=trunk
set Brnch=branches\1.5.x

rem Paths & working directories
set ScriptPath=%~dp0
set RootDir=..\..\..\..\
set WDirTrunk=%RootDir%%Trunk%\Languages
set WDirBrnch=%RootDir%%Brnch%\Languages

rem Input and output files
set LanguageList=%WDirTrunk%\Languages.txt
set LogFile=%ScriptPath%\gui_translation.txt

rem Some blanks for formatting
set Blanks30="                              "

rem Get current revision of working copy
for /F "usebackq" %%p in (`svnversion`) do set WCRev=%%p

call :Prepare %WDirTrunk%
set TotalTrunk=%Errorlevel%
call :Prepare %WDirBrnch%
set TotalBrnch=%Errorlevel%

rem Write log file header 
echo.> %LogFile%
echo TortoiseSVN GUI translation status for revision !WCRev:~0,5!^ >> %LogFile%
echo =========================================================================== >> %LogFile%
echo                                : Developer Version   : Current Release >> %LogFile%
echo                  Location      : %Trunk%               : %Brnch% >> %LogFile%
echo                  Total strings : %TotalTrunk%                : %TotalBrnch% >> %LogFile%
echo Language                       : Status (fu/un/ma)   : Status (fu/un/ma) >> %LogFile% 
echo =========================================================================== >> %LogFile%

rem Let's loop through all trunk translations.
rem Don't care if there's a language more on the release branch (dead language anyway)  

rem !!! ATTENTION 
rem !!! There is a real TAB key inside "delims=	;"
rem !!! Please leave it there

for /F "eol=# delims=	; tokens=1,5" %%i in (%LanguageList%) do (
  set PoFile=_Tortois_%%i.po
  set LangName=%%j ^(%%i^)%Blanks30:~1,30%
  set LangName=!LangName:~0,30!

  echo Computing Status for !LANGNAME!
  for /F "usebackq delims=#" %%p in (`Check_Status.bat !WDirTrunk! !PoFile! !TotalTrunk!`) do set StatusTrunk=%%p
  for /F "usebackq delims=#" %%p in (`Check_Status.bat !WDirBrnch! !PoFile! !TotalBrnch!`) do set StatusBrnch=%%p

  echo !LANGNAME! : !StatusTrunk! : !StatusBrnch! >> %Logfile%
)

call :Cleanup %WDirTrunk%
call :Cleanup %WDirBrnch%

rem Write log file footer 
echo =========================================================================== >> %LogFile%
echo Status: fu=fuzzy - un=untranslated - ma=missing accelerator keys >> %LogFile%
echo =========================================================================== >> %LogFile%

goto :End

rem ======================================================================
rem Subroutine to prepare the working directory for the check 
rem ======================================================================
:Prepare
echo.
echo Preparing working directory %1
echo ----------------------------------------------------------------------
pushd %1
rem CountSVN all messages in PO Template file
FOR /F "usebackq" %%p IN (`%ScriptPath%\Check_Attrib.bat Tortoise.pot`) DO SET StringsTotal=%%p

copy Tortoise_*.po _Tortois_*.po /Y 1>NUL
FOR %%i in (_Tortois_*.po) DO (
  echo %%i
  msgmerge --no-wrap --quiet --no-fuzzy-matching -s %%i Tortoise.pot -o %%i 2> NUL
)
popd
rem Return number of strings in errorlevel
exit /b %StringsTotal%

rem ======================================================================
rem Subroutine to prepare the working directory for the check 
rem ======================================================================
:Cleanup
echo Cleaning up working directory %1
pushd %1
del _Tortois_*.po /Q
del *.mo
popd
exit /b 0

:End
ENDLOCAL
Exit /b 0
