@echo off
cls
rem Merge translations from trunk and branch into the branch translation
rem Needs the following tools installed in your search path:
rem - Gnu gettext to combine and merge the .po files
rem - dos2unix to make sure that the line endings are consistent

setlocal EnableDelayedExpansion
set TRUNK=..\..\..\trunk\Languages

for /D %%L in (*.) DO (
  echo Merging %%L
  if exist %%L\TortoiseUI.po (
    echo - GUI Translation
    call :mergeit %%L\TortoiseUI.po TortoiseUI.pot
  )
  if exist %%L\TortoiseDoc.po (
    echo - Doc Translation
    call :mergeit %%L\TortoiseDoc.po TortoiseDoc.pot
  )
  echo.
)
goto ende

:mergeit
    rem "msgcat" deliberately uses trunk first to overwrite old translations 
    rem with newer ones from trunk!
    rem If we used branch first, we'd get only missing translations from trunk.
    msgcat --use-first %TRUNK%\%1 %1 -o temp.po 2>nul
    msgmerge temp.po %2 2>nul| msgattrib --no-obsolete -o %1 2>nul 
    dos2unix %1 2>nul
    rem Count the changes. If there's only one changed line, it's the "PO-Revision-Date" line
    rem which means that there is no real change at all.
    rem In this case revert the file.
    for /F %%I in ('svn diff "%1" ^| find /c "@@"') do set chg=%%I
    if %chg% EQU 1 (
        rem "svn revert" fails for paths which contain an "@" sign, like sr@latin.
        rem So we just append an "@" to the end of the path, because svn only 
        rem cares about the last "@".
        svn revert "%1@"
    )
    exit /b

:ende
del temp.po 2>nul
del messages.mo 2>nul
endlocal
