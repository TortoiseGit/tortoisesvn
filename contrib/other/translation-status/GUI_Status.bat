@echo off
rem Script to build the language dlls

SETLOCAL ENABLEDELAYEDEXPANSION
set WorkDir=..\..\..\Languages
set ScriptPath=%~dp0
set LanguageList=Languages.txt
set LogFile=%ScriptPath%\gui_translation.txt
set TmpFileDone=%ScriptPath%\gui_translation.tm1
set TmpFileReview=%ScriptPath%\gui_translation.tm2
set LotsOfBlanks="                              "
set SomeBlanks="     "

pushd %WorkDir%

FOR /F "usebackq" %%p IN (`svnversion`) DO SET version=%%p

rem CountSVN all messages in PO Template file
FOR /F "usebackq" %%p IN (`%ScriptPath%\Check_Attrib.bat Tortoise.pot`) DO SET totSVN=%%p

copy Tortoise_*.po _Tortois_*.po /Y 2>NUL
echo.
echo Merging translations with latest .pot file
echo ------------------------------------------
FOR %%i in (_Tortois_*.po) DO (
  msgmerge --no-wrap --quiet --no-fuzzy-matching -s %%i Tortoise.pot -o %%i 2> NUL
)

echo. > %LogFile%
echo TortoiseSVN GUI translation status for trunk/branch^(r!version:~0,4!^) >> %LogFile%
echo ===================================================== >> %LogFile%
echo Total=!totSVN! strings >> %LogFile%
echo. >> %LogFile%
echo Status: fu=fuzzy - un=untranslated - ma=missing accelerator keys >> %LogFile%
echo. >> %LogFile%
echo Language                       : Status  (fu/un/ma) >> %Logfile%
echo --------------------------------------------------- >> %Logfile%

echo.
echo TortoiseSVN GUI translation status for trunk/branch^(r!version:~0,4!^)
echo =====================================================
echo Total=!totSVN! strings
echo.
echo Language                       : Status  (fu/ut/ma)
echo ---------------------------------------------------

rem !!! ATTENTION 
rem !!! There is a real TAB key inside "delims=	;"
rem !!! Please leave it there

FOR /F "eol=# delims=	; tokens=1,5" %%i IN (%LanguageList%) DO (

  SET POSVN=_Tortois_%%i.po
  SET LANGNAME=%%j ^(%%i^)%LotsOfBlanks:~1,30%
  SET LANGNAME=!LANGNAME:~0,30! :

  if exist !POSVN! (
    set errSVN=0
    set accSVN=0
    set traSVN=0
    set untSVN=0
    set fuzSVN=0

    FOR /F "usebackq" %%p IN (`%ScriptPath%\Check_Errors.bat --check !POSVN!`) DO SET errSVN=%%p
    FOR /F "usebackq" %%p IN (`%ScriptPath%\Check_Errors.bat --check-accelerators !POSVN!`) DO SET accSVN=%%p
    FOR /F "usebackq" %%p IN (`%ScriptPath%\Check_Attrib.bat --translated --no-obsolete !POSVN!`) DO SET traSVN=%%p
    FOR /F "usebackq" %%p IN (`%ScriptPath%\Check_Attrib.bat --only-fuzzy --no-obsolete !POSVN!`) DO SET fuzSVN=%%p
    FOR /F "usebackq" %%p IN (`%ScriptPath%\Check_Attrib.bat --untranslated --no-obsolete !POSVN!`) DO SET untSVN=%%p

    SET /A errsumSVN=!fuzSVN!+!untSVN!+!errSVN!+!accSVN!

    if !errSVN! NEQ 0 (
      set outSVN=BROKEN
      set outStat=
    ) else if !errsumSVN! EQU 0 (
      set outSVN=OK
      set outStat=
    ) else (
      if !totSVN! EQU !traSVN! (
        set outSVN=99%%
        set outStat=- ^(!fuzSVN!/!untSVN!/!accSVN!^)
      ) else (
        set /a outTMP=100*!traSVN!/totSVN
        set outSVN=!outTMP!%%
        set outStat=- ^(!fuzSVN!/!untSVN!/!accSVN!^)
      )
    )

  ) else (
    set outSVN=NONE
  )


  rem Format output (left adjust only, right adjust is complicated)
  set outSVN=!outSVN!%SomeBlanks:~1,6%
  set outSVN=!outSVN:~0,5!

  if not "!outSVN!"=="NONE" (
    echo !LANGNAME! !outSVN! !outStat!
    echo !LANGNAME! !outSVN! !outStat! >> %Logfile% 
  )

)

del _Tortois_*.po /Q
del *.mo

:end
popd
ENDLOCAL
goto :eof
