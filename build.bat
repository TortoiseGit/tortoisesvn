@echo off
SETLOCAL ENABLEDELAYEDEXPANSION
set starttime=%time%
rem
rem build script for TortoiseSVN
rem
@if "%VSINSTALLDIR%"=="" call "%VS71COMNTOOLS%\vsvars32.bat"
if "%TortoiseVars%"=="" call TortoiseVars.bat
echo %~dp0

if "%1"=="" (
  SET _RELEASE=ON
  SET _DEBUG=ON
  SET _RELEASE_MBCS=ON
  SET _DOCS=ON
  SET _SETUP=ON
)

:getparam
if "%1"=="release" SET _RELEASE=ON
if "%1"=="RELEASE" SET _RELEASE=ON
if "%1"=="debug" SET _DEBUG=ON
if "%1"=="DEBUG" SET _DEBUG=ON
if "%1"=="release_mbcs" SET _RELEASE_MBCS=ON
if "%1"=="RELEASE_MBCS" SET _RELEASE_MBCS=ON
if "%1"=="doc" SET _DOCS=ON
if "%1"=="DOC" SET _DOCS=ON
if "%1"=="setup" SET _SETUP=ON
if "%1"=="SETUP" SET _SETUP=ON
shift
if NOT "%1"=="" goto :getparam


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
rem perl apr-util\build\w32locatedb.pl dll .\db4-win32\include .\db4-win32\lib
copy build\generator\vcnet_sln.ezt build\generator\vcnet_sln7.ezt
copy ..\TortoiseSVN\vcnet_sln.ezt build\generator\vcnet_sln.ezt
call python gen-make.py -t vcproj --with-openssl=..\Common\openssl --with-zlib=..\Common\zlib --with-apr=apr --with-apr-util=apr-util --with-apr-iconv=apr-iconv --enable-nls
copy build\generator\vcnet_sln7.ezt build\generator\vcnet_sln.ezt /Y
del build\generator\vcnet_sln7.ezt
if %_DEBUG%==ON (
  devenv subversion_vcnet.sln /useenv /rebuild debug /project "__ALL__")
if %_RELEASE%==ON (
  devenv subversion_vcnet.sln /useenv /rebuild release /project "__ALL__"
) else if %_RELEASE_MBCS%==ON (
  devenv subversion_vcnet.sln /useenv /rebuild release /project "__ALL__")

@echo off
exit /B

rem TortoiseSVN
echo ================================================================================
echo building TortoiseSVN
if %_DEBUG%==ON (
  copy apr\Debug\libapr.dll ..\TortoiseSVN\bin\Debug /Y > NUL 
  copy apr-util\Debug\libaprutil.dll ..\TortoiseSVN\bin\Debug /Y > NUL 
  copy apr-iconv\Debug\libapriconv.dll ..\TortoiseSVN\bin\Debug /Y > NUL 
)
if %_RELEASE%==ON (
  copy apr\Release\libapr.dll ..\TortoiseSVN\bin\Release /Y > NUL 
  copy apr-util\Release\libaprutil.dll ..\TortoiseSVN\bin\Release /Y > NUL 
  copy apr-iconv\Release\libapriconv.dll ..\TortoiseSVN\bin\Release /Y > NUL 
)
if %_RELEASE_MBCS%==ON (
  copy apr\Release\libapr.dll ..\TortoiseSVN\bin\Release_MBCS /Y > NUL 
  copy apr-util\Release\libaprutil.dll ..\TortoiseSVN\bin\Release_MBCS /Y > NUL 
  copy apr-iconv\Release\libapriconv.dll ..\TortoiseSVN\bin\Release_MBCS /Y > NUL 
)

cd ..\TortoiseSVN\src
devenv TortoiseSVN.sln /rebuild release /project SubWCRev
..\bin\release\SubWCRev.exe .. version.in version.h
if %_RELEASE%==ON (
  devenv TortoiseSVN.sln /rebuild release )
if %_RELEASE_MBCS%==ON (
  devenv TortoiseSVN.sln /rebuild release_mbcs)
if %_DEBUG%==ON (
  devenv TortoiseSVN.sln /rebuild debug)
cd Languages
call Make_Pot.bat
cd ..\..
if %_DEBUG%==ON (
  if EXIST bin\debug\iconv rmdir /S /Q bin\debug\iconv > NUL
  mkdir bin\debug\iconv > NUL
  copy ..\Subversion\httpd\apr-iconv\Debug\iconv\*.so bin\debug\iconv > NUL
  copy ..\Common\openssl\out32dll\*.dll bin\debug /Y > NUL
  copy ..\Subversion\db4-win32\bin\libdb42d.dll bin\debug /Y > NUL
)
if %_RELEASE%==ON (
  if EXIST bin\release\iconv rmdir /S /Q bin\release\iconv > NUL
  mkdir bin\release\iconv > NUL
  copy ..\Subversion\httpd\apr-iconv\Release\iconv\*.so bin\release\iconv > NUL
  copy ..\Common\openssl\out32dll\*.dll bin\release /Y > NUL
  copy ..\Subversion\db4-win32\bin\libdb42.dll bin\release /Y > NUL
)
if %_RELEASE_MBCS%==ON (
  if EXIST bin\release_mbcs\iconv rmdir /S /Q bin\release_mbcs\iconv > NUL
  mkdir bin\release_mbcs\iconv > NUL
  copy ..\Subversion\httpd\apr-iconv\Release\iconv\*.so bin\release_mbcs\iconv > NUL
  copy ..\Common\openssl\out32dll\*.dll bin\release_mbcs /Y > NUL
  copy ..\Subversion\db4-win32\bin\libdb42.dll bin\release_mbcs /Y > NUL
)
@echo off

echo ================================================================================
cd doc
if %_DOCS%==ON (
  echo generating docs
  call GenDoc.bat > NUL
)
@echo off

echo ================================================================================
cd ..\src\TortoiseSVNSetup
@echo off
if %_SETUP%==ON (
  echo building installers

  call MakeMsi.bat
  cd ..\Languages
  call Make_Installer.bat
)

cd ..\..
echo started at: %starttime%
echo now its   : %time%