@echo off
setlocal

pushd %~dp0

if not exist ..\bin\release\bin\ResText.exe (
  echo "..\bin\release\bin\ResText.exe" isn't present. Did you forget to build it?
  goto end
)

..\bin\release\bin\ResText.exe extract ..\bin\release\bin\TortoiseProcLang.dll^
 ..\bin\release\bin\TortoiseMergeLang.dll ..\bin\release\bin\TortoiseIDiffLang.dll^
 ..\bin\release\bin\TortoiseUDiffLang.dll ..\bin\release\bin\TortoiseBlameLang.dll -useheaderfile TortoiseUIPotHeader.txt TortoiseUI.pot

rem leave the next two lines commented. The msgremove tool changes the
rem sequence "\r\n" to "r\n" - removing the backslash before the r!!!
rem msgremove Tmp.pot -i ignore.po -o Tortoise.pot
rem del Tmp.pot

rem leave this line commented. I fear it destroys the asian .po files
rem FOR %%i in (*.po) do msgmerge --no-wrap -s %%i Tortoise.pot -o %%i

:end
popd
endlocal
exit /b
