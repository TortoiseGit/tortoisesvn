@echo off
rem Script to build the language dlls

SETLOCAL ENABLEDELAYEDEXPANSION
set PATH=c:\programme\dxgettext;c:\tools
set OFile=..\..\www\translations.html

type trans_head.html > %OFile%

FOR /F " usebackq skip=1 " %%p IN (`Check_Attrib.bat Tortoise.pot`) DO SET total=%%p
FOR /F "tokens=1,2*" %%i in (Languages.txt) do call :doit %%i %%j "%%k"

:end
type trans_foot.html >> %OFile%
ENDLOCAL
goto :eof

:doit
echo Checking %3 Translation...

if exist Tortoise_%1%.po (
  set errors=0
  set tra=0
  set unt=0
  set fuz=0
  set obs=0

  FOR /F " usebackq skip=1 " %%p IN (`Check_Accel.bat Tortoise_%1.po`) DO SET errors=%%p
  FOR /F " usebackq skip=1 " %%p IN (`Check_Attrib.bat Tortoise_%1.po translated`) DO SET tra=%%p
  FOR /F " usebackq skip=1 " %%p IN (`Check_Attrib.bat Tortoise_%1.po only-fuzzy`) DO SET fuz=%%p
  FOR /F " usebackq skip=1 " %%p IN (`Check_Attrib.bat Tortoise_%1.po untranslated`) DO SET unt=%%p
rem   FOR /F " usebackq skip=1 " %%p IN (`Check_Attrib.bat Tortoise_%1.po only-obsolete`) DO SET obs=%%p

  if !tra! EQU !total! (
    echo ^<tr class="complete"^> >> %OFile%
  ) else (
    echo ^<tr class="incomplete"^> >> %OFile%
  )

  if !tra! GTR 1 SET /A tra -= 1 
  if !fuz! GTR 1 SET /A fuz -= 1 
  if !unt! GTR 1 SET /A unt -= 1

  SET /A tra = !tra! - !fuz!

  SET /A wt=200*!tra!/!total!
  SET /A wf=200*!fuz!/!total!
  SET /A wu=200*!unt!/!total!

  echo ^<td class="lang"^>%~3 ^(%1^)^</td^>>> %OFile%
  echo ^<td class="untrans"^>!errors!^</td^>>> %OFile%
  echo ^<td class="trans"^>!tra!^</td^>>> %OFile%
  echo ^<td class="fuzzy"^>!fuz!^</td^>>> %OFile%
  echo ^<td class="untrans"^>!unt!^</td^>>> %OFile%
  echo ^<td class="graph"^>>> %OFile%
	echo ^<img src="images/translated.png" width="!wt!" height="16"/^>^<img src="images/fuzzy.png" width="!wf!" height="16"/^>^<img src="images/untranslated.png" width="!wu!" height="16"/^>>> %OFile%
  echo ^</td^>>> %OFile%
 
) else (
  echo ^<tr class="incomplete"^> >> %OFile%
  echo ^<td class="lang"^>%~3 ^(%1^)^</td^> >> %OFile%
  echo ^<td class="untrans"^>Missing^</td^> >> %OFile%
  echo ^<td class="trans"^>^&nbsp;^</td^> >> %OFile%
  echo ^<td class="fuzzy"^>^&nbsp;^</td^> >> %OFile%
  echo ^<td class="untrans"^>^&nbsp;^</td^> >> %OFile%
  echo ^<td class="graph"^>^<img src="images/untranslated.png" width="200" height="16"/^>^</td^> >> %OFile%
)
echo ^<td class="download"^>^<a href="http://svn.collab.net/repos/tortoisesvn/trunk/src/Languages/Tortoise_%1.po"^>Tortoise_%1.po^</a^>^</td^> >> %OFile%
echo ^</tr^> >> %OFile%

goto :eof

:listone
echo %1 %Total%
goto :eof
