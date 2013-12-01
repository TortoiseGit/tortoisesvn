@echo off
setlocal

pushd %~dp0

rem you can set the COVDIR variable to your coverity path
if not defined COVDIR set "COVDIR=C:\cov-analysis-win32-6.6.1"
if defined COVDIR if not exist "%COVDIR%" (
  echo.
  echo ERROR: Coverity not found in "%COVDIR%"
  goto End
)

rem add the tools paths in PATH
set "NANT_PATH=C:\nant\bin"
set "PERL_PATH=C:\Perl\perl\bin"
set "PYTHON_PATH=C:\Python27"
set "PATH=%NANT_PATH%;%PERL_PATH%;%PYTHON_PATH%;%PATH%"

call "%VS110COMNTOOLS%..\..\VC\vcvarsall.bat" x86

rem cleanup the cov-dir if it's present
if exist "cov-int" rd /q /s "cov-int"

rem we need to build the libraries before our files
title nant clean ipv6 xp Subversion
nant clean ipv6 xp Subversion

rem the actual coverity command
title "%COVDIR%\bin\cov-build.exe" --dir "cov-int" nant clean ipv6 xp TortoiseSVN
"%COVDIR%\bin\cov-build.exe" --dir "cov-int" nant clean ipv6 xp TortoiseSVN
"%COVDIR%\bin\cov-build.exe" --dir "cov-int" nant clean xp Overlays

if exist "TortoiseSVN.tar" del "TortoiseSVN.tar"
if exist "TortoiseSVN.tgz" del "TortoiseSVN.tgz"


:tar
rem try the tar tool in case it's in PATH
tar --version 1>&2 2>nul || (echo. & echo ERROR: tar not found & goto SevenZip)
title Creating "TortoiseSVN.tgz"
tar czvf "TortoiseSVN.tgz" "cov-int"
goto End


:SevenZip
rem try 7-Zip if it's the default installation path
if not exist "%PROGRAMFILES%\7-zip\7z.exe" (
  echo.
  echo ERROR: "%PROGRAMFILES%\7-zip\7z.exe" not found
  goto End
)

title Creating "TortoiseSVN.tar"...
"%PROGRAMFILES%\7-zip\7z.exe" a -ttar "TortoiseSVN.tar" "cov-int"
title Creating "TortoiseSVN.tgz"...
"%PROGRAMFILES%\7-zip\7z.exe" a -tgzip "TortoiseSVN.tgz" "TortoiseSVN.tar"
if exist "TortoiseSVN.tar" del "TortoiseSVN.tar"


:End
popd
echo. & echo Press any key to close this window...
pause >nul
endlocal
exit /b
