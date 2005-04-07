@echo off
SETLOCAL ENABLEDELAYEDEXPANSION
..\..\bin\release\bin\SubWCRev.exe ..\.. Setup.wxs Setup_good.wxs
if NOT EXIST Setup_good.wxs (copy Setup.wxs Setup_good.wxs)
..\..\bin\release\bin\SubWCRev.exe ..\.. makemsisub.in makemsisub.bat
if NOT EXIST makemsisub.bat (copy makemsisub.in makemsisub.bat)
call makemsisub.bat
del makemsisub.bat