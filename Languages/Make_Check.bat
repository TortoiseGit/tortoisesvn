@echo off
rem Script to build the language dlls

SETLOCAL ENABLEDELAYEDEXPANSION
if "%TortoiseVars%"=="" call ..\TortoiseVars.bat
set OFile=..\www\translations.html
set LogFile=statusreport.txt

..\bin\release\bin\SubWCRev.exe . trans_head.html %OFile%

rem Count all messages in PO Template file
FOR /F "usebackq" %%p IN (`Check_Attrib.bat Tortoise.pot`) DO SET total=%%p
FOR /F "usebackq" %%p IN (`svnversion .`) DO SET version=%%p

echo ^<tr class="complete"^> >> %OFile%
echo ^<td class="lang"^>Empty Catalog^</td^> >> %OFile%
echo ^<td class="lang"^>^&nbsp;^</td^> >> %OFile%
echo ^<td class="trans"^>^&nbsp;^</td^> >> %OFile%
echo ^<td class="fuzzy"^>^&nbsp;^</td^> >> %OFile%
echo ^<td class="untrans"^>!total!^</td^> >> %OFile%
echo ^<td class="untrans"^>^&nbsp;^</td^> >> %OFile%
echo ^<td class="graph"^>^&nbsp;^</td^> >> %OFile%
echo ^<td class="download"^>^<a href="http://svn.collab.net/repos/tortoisesvn/trunk/Languages/Tortoise.pot"^>Tortoise.pot^</a^>^</td^> >> %OFile%
echo ^</tr^> >> %OFile%

copy Tortoise_*.po _Tortois_*.po /Y
FOR %%i in (_Tortois_*.po) do msgmerge --no-wrap --quiet --no-fuzzy-matching -s %%i Tortoise.pot -o %%i

echo. > %LogFile%
echo Translation Status report for r!version:~0,4! >> %LogFile%
echo ----------------------------------- >> %LogFile%
echo Total=!total! >> %LogFile%
echo. >> %LogFile%

rem fourth parameter is needed with quotes!
FOR /F "eol=# delims=; tokens=1,2,3,4,5" %%i in (Languages.txt) do call :doit %%i %%j %%m %%m

del _Tortois_*.po /Q
del *.mo

:end
type trans_foot.html >> %OFile%
ENDLOCAL
goto :eof

:doit
echo %4 (%1)
echo %4 (%1) >> %LogFile%

if exist _Tortois_%1%.po (
  set errors=0
  set tra=0
  set unt=0
  set fuz=0
  set obs=0

  FOR /F "usebackq skip=1 " %%p IN (`Check_Accel.bat _Tortois_%1.po`) DO SET errors=%%p
rem   FOR /F "usebackq" %%p IN (`Check_Attrib.bat _Tortois_%1.po --only-obsolete`) DO SET obs=%%p
  FOR /F "usebackq" %%p IN (`Check_Attrib.bat --translated --no-obsolete _Tortois_%1.po`) DO SET tra=%%p
  FOR /F "usebackq" %%p IN (`Check_Attrib.bat --only-fuzzy --no-obsolete _Tortois_%1.po`) DO SET fuz=%%p
  FOR /F "usebackq" %%p IN (`Check_Attrib.bat --untranslated --no-obsolete _Tortois_%1.po`) DO SET unt=%%p

  echo translated  :    !tra! >> %LogFile%
  echo fuzzy       :    !fuz! >> %LogFile%
  echo untranslated:    !unt! >> %LogFile%
  echo missing hotkeys: !errors! >> %LogFile%
  echo. >> %LogFile%

  if !tra! EQU !total! (
    echo ^<tr class="complete"^> >> %OFile%
    SET unt=0
rem    SET fuz=0
  ) else (
    echo ^<tr class="incomplete"^> >> %OFile%
  )

  SET /A tra=!tra!-!fuz!

  SET /A wt=200*!tra!/!total!
  SET /A wf=200*!fuz!/!total!
  SET /A wu=200*!unt!/!total!

  SET /A tra=!wt!/2

  echo ^<td class="lang"^>%~3^</td^>>> %OFile%
  echo ^<td class="lang"^>%1^</td^>>> %OFile%
  echo ^<td class="trans"^>!tra! ^%%^</td^>>> %OFile%
  echo ^<td class="fuzzy"^>!fuz!^</td^>>> %OFile%
  echo ^<td class="untrans"^>!unt!^</td^>>> %OFile%
  echo ^<td class="untrans"^>!errors!^</td^>>> %OFile%
  echo ^<td class="graph"^>>> %OFile%
  echo ^<img src="images/translated.png" width="!wt!" height="16"/^>^<img src="images/fuzzy.png" width="!wf!" height="16"/^>^<img src="images/untranslated.png" width="!wu!" height="16"/^>>> %OFile%
  echo ^</td^>>> %OFile%
 
) else (
  echo ^<tr class="incomplete"^> >> %OFile%
  echo ^<td class="lang"^>%~4^</td^>>> %OFile%
  echo ^<td class="lang"^>%1^</td^>>> %OFile%
  echo ^<td class="trans"^>^&nbsp;^</td^> >> %OFile%
  echo ^<td class="fuzzy"^>^&nbsp;^</td^> >> %OFile%
  echo ^<td class="untrans"^>^&nbsp;^</td^> >> %OFile%
  echo ^<td class="untrans"^>Missing^</td^> >> %OFile%
  echo ^<td class="graph"^>^<img src="images/untranslated.png" width="200" height="16"/^>^</td^> >> %OFile%
)
echo ^<td class="download"^>^<a href="http://svn.collab.net/repos/tortoisesvn/trunk/Languages/Tortoise_%1.po"^>Tortoise_%1.po^</a^>^</td^> >> %OFile%
echo ^</tr^> >> %OFile%

goto :eof
