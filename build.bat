@echo off
SETLOCAL ENABLEDELAYEDEXPANSION
set starttime=%time%
rem
rem build script for TortoiseSVN
rem
@if "%VSINSTALLDIR%"=="" call "%VS71COMNTOOLS%\vsvars32.bat"

rem OpenSSL
echo ================================================================================
echo building OpenSSL
cd ..\common\openssl
perl Configure VC-WIN32 > NUL
call ms\do_masm
call nmake -f ms\ntdll.mak
@echo off

rem ZLIB
echo ================================================================================
echo building ZLib
cd ..\zlib
copy win32\Makefile.msc Makefile.msc > NUL
call nmake -f Makefile.msc
copy zlib.lib zlibstat.lib > NUL
@echo off

rem Subversion
echo ================================================================================
echo building Subversion
cd ..\..\Subversion
copy build\generator\vcnet_sln.ezt build\generator\vcnet_sln7.ezt
copy ..\TortoiseSVN\vcnet_sln.ezt build\generator\vcnet_sln.ezt
call python gen-make.py -t vcproj --with-openssl=..\Common\openssl --with-zlib=..\Common\zlib
copy build\generator\vcnet_sln7.ezt build\generator\vcnet_sln.ezt /Y
del build\generator\vcnet_sln7.ezt
devenv subversion_vcnet.sln /rebuild debug /project "__ALL__"
devenv subversion_vcnet.sln /rebuild release /project "__ALL__"
@echo off

rem TortoiseSVN
echo ================================================================================
echo building TortoiseSVN
cd ..\TortoiseSVN\src
devenv TortoiseSVN.sln /rebuild release /project SubWCRev
..\bin\release\SubWCRev.exe .. version.in version.h
devenv TortoiseSVN.sln /rebuild release
devenv TortoiseSVN.sln /rebuild release_mbcs
devenv TortoiseSVN.sln /rebuild debug
cd Languages
FOR %%V In (*.pot) Do ..\..\bin\release\ResText.exe extract TortoiseMergeLang.dll TortoiseProcLang.dll %%V -quiet
cd ..\..
if EXIST bin\debug\iconv rmdir /S /Q bin\debug\iconv > NUL
if EXIST bin\release\iconv rmdir /S /Q bin\release\iconv > NUL
if EXIST bin\release_mbcs\iconv rmdir /S /Q bin\release_mbcs\iconv > NUL
mkdir bin\debug\iconv > NUL
mkdir bin\release\iconv > NUL
mkdir bin\release_mbcs\iconv > NUL
copy ..\Subversion\apr-iconv\LibD\iconv\*.so bin\debug\iconv > NUL
copy ..\Subversion\apr-iconv\LibR\iconv\*.so bin\release\iconv > NUL
copy ..\Subversion\apr-iconv\LibR\iconv\*.so bin\release_mbcs\iconv > NUL
copy ..\Common\openssl\out32dll\*.dll bin\debug /Y > NUL
copy ..\Common\openssl\out32dll\*.dll bin\release /Y > NUL
copy ..\Common\openssl\out32dll\*.dll bin\release_mbcs /Y > NUL
copy ..\Subversion\db4-win32\bin\libdb42d.dll bin\debug /Y > NUL
copy ..\Subversion\db4-win32\bin\libdb42.dll bin\release /Y > NUL
copy ..\Subversion\db4-win32\bin\libdb42.dll bin\release_mbcs /Y > NUL
@echo off

echo ================================================================================
echo generating docs
cd doc
call GenDoc.bat > NUL
@echo off

echo ================================================================================
echo building installers
cd ..\src\TortoiseSVNSetup
@echo off

call MakeMsi.bat

cd ..\..
echo started at: %starttime%
echo now its   : %time%