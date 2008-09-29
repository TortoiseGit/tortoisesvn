rem Copyright (C) 2004-2008 the TortoiseSVN team
rem This file is distributed under the same license as TortoiseSVN

rem Last commit by:
rem $Author$
rem $Date$
rem $Rev$

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
