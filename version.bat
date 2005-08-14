@echo off
SETLOCAL ENABLEDELAYEDEXPANSION

::
:: sets the version numbers in all files using a version number
::

@if "%VSINSTALLDIR%"=="" call "%VS71COMNTOOLS%\vsvars32.bat"
if "%TortoiseVars%"=="" call TortoiseVars.bat

bin\Release\bin\SubWCRev . version.in temp.bat || goto :error

call temp.bat
del temp.bat

call :replaceVersionStrings src\TortoiseSVNSetup\VersionNumberInclude.in.wxi src\TortoiseSVNSetup\VersionNumberInclude.wxi 
call :replaceVersionStrings src\TortoiseSVNSetup\MakeMsiSub.in.bat src\TortoiseSVNSetup\MakeMsiSub.bat 
call :replaceVersionStrings src\version.in src\version.h
call :replaceVersionStrings Languages\LanguagePack.in Languages\LanguagePack.nsi
call :replaceVersionStrings doc\source\en\Version.in doc\source\en\Version.xml

goto :eof

:: replace version values in a file, using sed
:replaceVersionStrings
setlocal
 set "file=%~1"
 set "outfile=%~2"
 if not exist "%file%" goto :error

 copy "%file%" "%file%.orig" || goto :error
 sed "s/\$MajorVersion\$/%MajorVersion%/g" "%file%.orig" > "%file%.orig2" || goto :error
 sed "s/\$MinorVersion\$/%MinorVersion%/g" "%file%.orig2" > "%file%.orig" || goto :error
 sed "s/\$MicroVersion\$/%MicroVersion%/g" "%file%.orig" > "%file%.orig2" || goto :error
 sed "s/\$WCREV\$/%WCREV%/g" "%file%.orig2" > "%outfile%" || goto :error

 del "%file%.orig"
 del "%file%.orig2"

endlocal
goto :eof
:error
 echo Error in :replaceVersionStrings "%file%"!
endlocal
goto :eof