@echo off
rem Script to build the language dlls

SETLOCAL ENABLEDELAYEDEXPANSION
set WorkDir=..\..\..\doc\po
set ScriptPath=%~dp0
set LanguageList=..\..\Languages\Languages.txt
set LogFile=%ScriptPath%\doc_translation.txt
set LotsOfBlanks="                              "
set SomeBlanks="      "

pushd %WorkDir%

FOR /F "usebackq" %%p IN (`svnversion`) DO SET version=%%p

rem Count all messages in PO Template file
FOR /F "usebackq" %%p IN (`%ScriptPath%\Check_Attrib.bat TortoiseSVN.pot`) DO SET totSVN=%%p
FOR /F "usebackq" %%p IN (`%ScriptPath%\Check_Attrib.bat TortoiseMerge.pot`) DO SET totMrg=%%p

copy TortoiseSVN_*.po _TortoiseSV_*.po /Y 2>NUL
copy TortoiseMerge_*.po _TortoiseMerg_*.po /Y 2>NUL
echo.
echo Merging translations with latest .pot file
echo ------------------------------------------
FOR %%i in (_TortoiseSV_*.po) DO (
  msgmerge --no-wrap --quiet --no-fuzzy-matching -s %%i TortoiseSVN.pot -o %%i 2> NUL
)
FOR %%i in (_TortoiseMerg_*.po) DO (
  msgmerge --no-wrap --quiet --no-fuzzy-matching -s %%i TortoiseMerge.pot -o %%i 2> NUL
)

echo. > %LogFile%
echo TortoiseSVN doc translation status for trunk/branch^(r!version:~0,4!^) >> %LogFile%
echo ===================================================== >> %LogFile%
echo TortoiseSVN   = !totSVN! strings >> %LogFile%
echo TortoiseMerge = !totMrg! strings >> %LogFile%
echo. >> %LogFile%
echo Language                       : SVN   - Merge >> %Logfile%
echo ---------------------------------------------- >> %Logfile%

echo.
echo TortoiseSVN doc translation status for trunk/branch^(r!version:~0,4!^)
echo =====================================================
echo TortoiseSVN   = !totSVN! strings
echo TortoiseMerge = !totMrg! strings
echo.
echo Language                       : SVN   - Merge
echo ----------------------------------------------

rem !!! ATTENTION 
rem !!! There is a real TAB key inside "delims=	;"
rem !!! Please leave it there

FOR /F "eol=# delims=	; tokens=1,5" %%i IN (%LanguageList%) DO (

  SET POSVN=_TortoiseSV_%%i.po
  SET POMRG=_TortoiseMerg_%%i.po
  SET LANGNAME=%%j ^(%%i^)%LotsOfBlanks:~1,30%
  SET LANGNAME=!LANGNAME:~0,30! :

  if exist !POSVN! (
    set errSVN=0
    set traSVN=0
    set untSVN=0
    set fuzSVN=0

    FOR /F "usebackq" %%p IN (`%ScriptPath%\Check_Errors.bat --check !POSVN!`) DO SET errSVN=%%p
    FOR /F "usebackq" %%p IN (`%ScriptPath%\Check_Attrib.bat --translated --no-obsolete !POSVN!`) DO SET traSVN=%%p
    FOR /F "usebackq" %%p IN (`%ScriptPath%\Check_Attrib.bat --only-fuzzy --no-obsolete !POSVN!`) DO SET fuzSVN=%%p
    FOR /F "usebackq" %%p IN (`%ScriptPath%\Check_Attrib.bat --untranslated --no-obsolete !POSVN!`) DO SET untSVN=%%p

    SET /A errsumSVN=!fuzSVN!+!untSVN!+!errSVN!

    if !errSVN! NEQ 0 (
      set outSVN=BROKEN
    ) else if !errsumSVN! EQU 0 (
      set outSVN=OK
    ) else (
      if !totSVN! EQU !traSVN! (
        set outSVN=99%%
      ) else (
        set /a outTMP=100*!traSVN!/totSVN
        set outSVN=!outTMP!%%
      )
    )

  ) else (
    set outSVN=NONE
  )
  
  if exist !POMRG! (
    set errMRG=0
    set traMRG=0
    set untMRG=0
    set fuzMRG=0

    FOR /F "usebackq" %%p IN (`%ScriptPath%\Check_Errors.bat --check !POMRG!`) DO SET errMRG=%%p
    FOR /F "usebackq" %%p IN (`%ScriptPath%\Check_Attrib.bat --translated --no-obsolete !POMRG!`) DO SET traMRG=%%p
    FOR /F "usebackq" %%p IN (`%ScriptPath%\Check_Attrib.bat --only-fuzzy --no-obsolete !POMRG!`) DO SET fuzMRG=%%p
    FOR /F "usebackq" %%p IN (`%ScriptPath%\Check_Attrib.bat --untranslated --no-obsolete !POMRG!`) DO SET untMRG=%%p

    SET /A errsumMRG=!fuzMRG!+!untMRG!+!errMRG!

    if !errMRG! NEQ 0 (
      set outMRG=BROKEN 
    ) else if !errsumMRG! EQU 0 (
      set outMRG=OK
    ) else (
      if !totMRG! EQU !traMRG! (
        set outMRG=99%%
      ) else (
        set /a outTMP=100*!traMRG!/totMRG
        set outMRG=!outTMP!%%
      )
    )
  ) else (
    set outMRG=NONE
  )


  set outBoth=!outSVN!!outMRG!

  rem Format output (left adjust only, right adjust is complicated)
  set outSVN=!outSVN!%SomeBlanks:~1,6%
  set outSVN=!outSVN:~0,5!
  set outMRG=!outMRG!%SomeBlanks:~1,6%
  set outMRG=!outMRG:~0,5!

  if not "!outBoth!"=="NONENONE" (
    echo !LANGNAME! !outSVN! - !outMRG!
    echo !LANGNAME! !outSVN! - !outMRG! >> %Logfile% 
  )

)

del _TortoiseSV_*.po /Q
del _TortoiseMerg_*.po /Q
del *.mo

:end
popd
ENDLOCAL
goto :eof
