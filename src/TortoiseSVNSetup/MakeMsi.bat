@echo off
SETLOCAL ENABLEDELAYEDEXPANSION
..\..\bin\release\bin\SubWCRev.exe ..\.. Setup.wxs Setup_good.wxs
if NOT EXIST Setup_good.wxs (copy Setup.wxs Setup_good.wxs)
set TSVNBUILD=release
candle -nologo -out Setup.wixobj Setup_good.wxs
light -nologo -out ..\..\bin\TortoiseSVN-1.1.x-UNICODE_svn-1.1.x.msi Setup.wixobj
set TSVNBUILD=release_mbcs
candle -nologo -out Setup.wixobj Setup_good.wxs
light -nologo -out ..\..\bin\TortoiseSVN-1.1.x-MBCS_svn-1.1.x.msi Setup.wixobj
del Setup.wixobj
del Setup_good.wxs
