@echo off
rem Script to build the language dlls

SETLOCAL ENABLEDELAYEDEXPANSION
if "%TortoiseVars%"=="" call ..\TortoiseVars.bat
set LogFile=statusreport.txt


rem Count all messages in PO Template file
FOR /F "usebackq" %%p IN (`Check_Attrib.bat Tortoise.pot`) DO SET total=%%p
FOR /F "usebackq" %%p IN (`svnversion .`) DO SET version=%%p

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
    SET unt=0
  )

  SET /A tra=!tra!-!fuz!

  SET /A wt=200*!tra!/!total!
  SET /A wf=200*!fuz!/!total!
  SET /A wu=200*!unt!/!total!

  SET /A tra=!wt!/2

)

goto :eof
