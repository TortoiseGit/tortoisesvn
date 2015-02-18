@echo off
setlocal

pushd %~dp0

rem you can set the COVDIR variable to your coverity path
if not defined COVDIR set "COVDIR=C:\cov-analysis"
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


:cleanup
if exist "cov-int" rd /q /s "cov-int"
if exist "TortoiseSVN.lzma" del "TortoiseSVN.lzma"
if exist "TortoiseSVN.tar"  del "TortoiseSVN.tar"
if exist "TortoiseSVN.tgz"  del "TortoiseSVN.tgz"


:main
rem Win32
call "%VS120COMNTOOLS%..\..\VC\vcvarsall.bat" x86
if %ERRORLEVEL% neq 0 (
  echo vcvarsall.bat call failed.
  goto End
)

rem we need to build the libraries before our files
title nant -buildfile:../default.build clean ipv6 Subversion
nant -buildfile:../default.build clean ipv6 Subversion

rem the actual coverity command
title "%COVDIR%\bin\cov-build.exe" --no-parallel-translate --dir "cov-int" nant -buildfile:../default.build ipv6 TortoiseSVN
"%COVDIR%\bin\cov-build.exe" --no-parallel-translate --dir "cov-int" nant -buildfile:../default.build ipv6 TortoiseSVN
"%COVDIR%\bin\cov-build.exe" --dir "cov-int" nant -buildfile:../default.build Overlays


rem x64
call "%VS120COMNTOOLS%..\..\VC\vcvarsall.bat" x86_amd64
if %ERRORLEVEL% neq 0 (
  echo vcvarsall.bat call failed.
  goto End
)
title nant -buildfile:../default.build clean x64 ipv6 cross Subversion
nant -buildfile:../default.build clean x64 ipv6 cross Subversion

title "%COVDIR%\bin\cov-build.exe" --dir "cov-int" nant -buildfile:../default.build x64 ipv6 cross TortoiseSVN
"%COVDIR%\bin\cov-build.exe" --dir "cov-int" nant -buildfile:../default.build x64 ipv6 cross TortoiseSVN


:tar
rem try the tar tool in case it's in PATH
set PATH=C:\MSYS\bin;%PATH%
tar --version 1>&2 2>nul || (echo. & echo ERROR: tar not found & goto SevenZip)
title Creating "TortoiseSVN.lzma"...
tar caf "TortoiseSVN.lzma" "cov-int"
goto End


:SevenZip
call :SubDetectSevenzipPath

rem Coverity is totally bogus with lzma...
rem And since I cannot replicate the arguments with 7-Zip, just use tar/gzip.
if exist "%SEVENZIP%" (
  title Creating "TortoiseSVN.tar"...
  "%SEVENZIP%" a -ttar "TortoiseSVN.tar" "cov-int"
  "%SEVENZIP%" a -tgzip "TortoiseSVN.tgz" "TortoiseSVN.tar"
  if exist "TortoiseSVN.tar" del "TortoiseSVN.tar"
  goto End
)


:SubDetectSevenzipPath
for %%g in (7z.exe) do (set "SEVENZIP_PATH=%%~$path:g")
if exist "%SEVENZIP_PATH%" (set "SEVENZIP=%SEVENZIP_PATH%" & exit /b)

for %%g in (7za.exe) do (set "SEVENZIP_PATH=%%~$path:g")
if exist "%SEVENZIP_PATH%" (set "SEVENZIP=%SEVENZIP_PATH%" & exit /b)

for /f "tokens=2*" %%a in (
  'reg query "HKLM\SOFTWARE\7-Zip" /v "path" 2^>nul ^| find "REG_SZ" ^|^|
   reg query "HKLM\SOFTWARE\Wow6432Node\7-Zip" /v "path" 2^>nul ^| find "REG_SZ"') do set "SEVENZIP=%%b\7z.exe"
exit /b


:End
popd
echo. & echo Press any key to close this window...
pause >nul
endlocal
exit /b
