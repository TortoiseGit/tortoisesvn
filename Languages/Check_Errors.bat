@echo off
SETLOCAL
FOR /F "usebackq skip=1" %%c IN (`msgfmt %* 2^>^&1 ^| grep -c msgstr`) DO SET /A count=%%c

if Defined count (
  rem
) else (
  SET count=0
)
echo %count%
ENDLOCAL
