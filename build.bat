@echo off
SETLOCAL ENABLEDELAYEDEXPANSION
set starttime=%time%
set startdir=%cd%
rem
rem build script for TortoiseSVN
rem
@if "%VSINSTALLDIR%"=="" call "%VS71COMNTOOLS%\vsvars32.bat"
if "%TortoiseVars%"=="" call TortoiseVars.bat

set INCLUDE=%~dp0ext\svn-win32-libintl\inc;%INCLUDE%
set LIB=%~dp0ext\svn-win32-libintl\lib;%LIB%

if "%1"=="" (
  SET _RELEASE=ON
  SET _DEBUG=ON
  SET _DOCS=ON
  SET _SETUP=ON
)

:getparam
if "%1"=="release" SET _RELEASE=ON
if "%1"=="RELEASE" SET _RELEASE=ON
if "%1"=="debug" SET _DEBUG=ON
if "%1"=="DEBUG" SET _DEBUG=ON
if "%1"=="doc" SET _DOCS=ON
if "%1"=="DOC" SET _DOCS=ON
if "%1"=="setup" SET _SETUP=ON
if "%1"=="SETUP" SET _SETUP=ON
shift
if NOT "%1"=="" goto :getparam

rem patch apr-iconv
copy ext\apr-iconv_patch\lib\iconv_module.c ..\Subversion\apr-iconv\lib /Y

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
copy /Y win32\Makefile.msc Makefile.msc > NUL
call nmake -f Makefile.msc
copy zlib.lib zlibstat.lib > NUL
@echo off

rem Subversion
echo ================================================================================
echo building Subversion
cd ..\..\Subversion
rem perl apr-util\build\w32locatedb.pl dll .\db4-win32\include .\db4-win32\lib
copy build\generator\vcnet_sln.ezt build\generator\vcnet_sln7.ezt
copy %startdir%\vcnet_sln.ezt build\generator\vcnet_sln.ezt
xcopy /Q /Y /I /E %startdir%\ext\berkeley-db\db4.3-win32 db4-win32
rmdir /s /q build\win32\vcnet-vcproj
call python gen-make.py -t vcproj --with-openssl=..\Common\openssl --with-zlib=..\Common\zlib --with-apr=apr --with-apr-util=apr-util --with-apr-iconv=apr-iconv --enable-nls --enable-bdb-in-apr-util
copy /Y %startdir%\ext\libaprutil.vcproj apr-util\libaprutil.vcproj

copy build\generator\vcnet_sln7.ezt build\generator\vcnet_sln.ezt /Y
del neon\config.h
del neon\config.hw
copy %startdir%\neonconfig.hw neon\config.hw
del build\generator\vcnet_sln7.ezt
if DEFINED _DEBUG (
  rem first, compile without any network/repository support
  ren subversion\svn_private_config.h  svn_private_config_copy.h
  ren subversion\svn_private_config.hw  svn_private_config_copy.hw
  copy %startdir%\svn_private_config.h subversion\svn_private_config.h
  copy %startdir%\svn_private_config.h subversion\svn_private_config.hw
  rmdir /s /q Debug > NUL
  rmdir /s /q apr-util\Debug > NUL
  devenv subversion_vcnet.sln /useenv /rebuild debug /project "__ALL__"
  ren Debug\subversion subversion_netless
  del subversion\svn_private_config.h
  del subversion\svn_private_config.hw
  ren subversion\svn_private_config_copy.h svn_private_config.h
  ren subversion\svn_private_config_copy.hw svn_private_config.hw
  devenv subversion_vcnet.sln /useenv /rebuild debug /project "__ALL__"
)
if DEFINED _RELEASE (
  rem first, compile without any network/repository support
  ren subversion\svn_private_config.h  svn_private_config_copy.h
  ren subversion\svn_private_config.hw  svn_private_config_copy.hw
  copy %startdir%\svn_private_config.h subversion\svn_private_config.h
  copy %startdir%\svn_private_config.h subversion\svn_private_config.hw
  rmdir /s /q apr-util\Release > NUL
  rmdir /s /q Release > NUL
  devenv subversion_vcnet.sln /useenv /rebuild release /project "__ALL__"
  ren Release\subversion subversion_netless
  del subversion\svn_private_config.h
  del subversion\svn_private_config.hw
  ren subversion\svn_private_config_copy.h svn_private_config.h
  ren subversion\svn_private_config_copy.hw svn_private_config.hw
  devenv subversion_vcnet.sln /useenv /rebuild release /project "__ALL__"
)
@echo off
rem TortoiseSVN
echo ================================================================================
echo copying files
cd %startdir%
if DEFINED _DEBUG (
  if EXIST bin\debug\iconv rmdir /S /Q bin\debug\iconv > NUL
  mkdir bin\debug\iconv > NUL
  copy ..\Subversion\apr-iconv\Debug\iconv\*.so bin\debug\iconv > NUL
  if EXIST bin\debug\bin rmdir /S /Q bin\debug\bin > NUL
  mkdir bin\debug\bin > NUL
  copy ..\Common\openssl\out32dll\*.dll bin\debug\bin /Y > NUL
  copy .\ext\svn-win32-libintl\bin\intl3_svn.dll bin\debug\bin /Y > NUL
  copy ..\Subversion\db4-win32\bin\libdb43d.dll bin\debug\bin /Y > NUL
  copy ..\Subversion\apr\Debug\libapr.dll bin\Debug\bin /Y > NUL 
  copy ..\Subversion\apr-util\Debug\libaprutil.dll bin\Debug\bin /Y > NUL 
  copy ..\Subversion\apr-iconv\Debug\libapriconv.dll bin\Debug\bin /Y > NUL 
)
if DEFINED _RELEASE (
  if EXIST bin\release\iconv rmdir /S /Q bin\release\iconv > NUL
  mkdir bin\release\iconv > NUL
  copy ..\Subversion\apr-iconv\Release\iconv\*.so bin\release\iconv > NUL
  if EXIST bin\release\bin rmdir /S /Q bin\release\bin > NUL
  mkdir bin\release\bin > NUL
  copy ..\Common\openssl\out32dll\*.dll bin\release\bin /Y > NUL
  copy .\ext\svn-win32-libintl\bin\intl3_svn.dll bin\release\bin /Y > NUL
  copy ..\Subversion\db4-win32\bin\libdb43.dll bin\release\bin /Y > NUL
  copy ..\Subversion\apr\Release\libapr.dll bin\Release\bin /Y > NUL 
  copy ..\Subversion\apr-util\Release\libaprutil.dll bin\Release\bin /Y > NUL 
  copy ..\Subversion\apr-iconv\Release\libapriconv.dll bin\Release\bin /Y > NUL 
)
echo ================================================================================
echo building TortoiseSVN
cd src
rem Build SubWCRev twice to include its own version info
copy /y version.none version.h
if NOT EXIST ..\bin\release\bin\SubWCRev.exe devenv TortoiseSVN.sln /rebuild release /project SubWCRev
..\bin\release\bin\SubWCRev.exe .. version.in version.h
devenv TortoiseSVN.sln /rebuild release /project SubWCRev
if DEFINED _RELEASE (
  devenv TortoiseSVN.sln /rebuild release
  copy TortoiseSVNSetup\autolist.txt ..\bin\release\bin
)
if DEFINED _DEBUG (
  devenv TortoiseSVN.sln /rebuild debug
  copy TortoiseSVNSetup\autolist.txt ..\bin\debug\bin
)

echo ================================================================================
echo building Scintilla
cd Utils\scintilla\win32
nmake -f scintilla.mak
copy ..\bin\SciLexer.dll ..\..\..\..\bin\debug\bin /Y > NUL
copy ..\bin\SciLexer.dll ..\..\..\..\bin\release\bin /Y > NUL
del ..\bin\*.dll > NUL
del ..\bin\*.exp > NUL
del ..\bin\*.lib > NUL
del ..\bin\*.pdb > NUL
del *.obj > NUL
del *.res > NUL
del *.pdb > NUL

cd ..\..\..\..\Languages
call Make_Pot.bat
cd ..
@echo off

echo ================================================================================
cd doc
if DEFINED _DOCS (
  echo generating docs
  call GenDoc.bat > NUL
)
@echo off

echo ================================================================================
cd ..\src\TortoiseSVNSetup
@echo off
if DEFINED _SETUP (
  echo building installers

  call MakeMsi.bat
  cd ..\..\Languages
  call Make_Installer.bat
  cd ..\src\TortoiseSVNSetup
)

:end
cd %startdir%
echo started at: %starttime%
echo now it is : %time%