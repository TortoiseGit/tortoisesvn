@echo off
rem Count messages in given PO File that match the given attributes 
SETLOCAL
FOR /F "usebackq skip=1" %%c IN (`msgattrib %* 2^>nul ^| grep -c msgid`) DO SET /A count=%%c
rem If count is set, then its always one too high
if Defined count (
  SET /A count -= 1
) else (
  SET count=0
)
echo %count%
ENDLOCAL