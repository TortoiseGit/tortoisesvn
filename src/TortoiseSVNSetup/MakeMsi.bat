@echo off
rem Check the environment
@if "%VSINSTALLDIR%"=="" call "%VS80COMNTOOLS%\vsvars32.bat"
if "%TortoiseVars%"=="" call ..\..\TortoiseVars.bat
SETLOCAL ENABLEDELAYEDEXPANSION


rem Check for the existence of dictionaries, and set env variables appropriately.
rem Note to refactorers - the ELSE must occur on the same line as the command preceding.  All Hail DOS!
ECHO MakeMsi.bat: Setting Environment Variables
if EXIST ..\..\..\Common\Spell\en_US.aff ( 
	SET DictionaryENUS=1 
) ELSE ( 
	SET DictionaryENUS=0 
)
if EXIST ..\..\..\Common\Spell\en_GB.aff ( 
	SET DictionaryENGB=1 
) ELSE ( 
	SET DictionaryENGB=0 
)
if EXIST ..\..\MYBUILD ( 
	SET IncludeCrashReportDll=1 
) ELSE ( 
	SET IncludeCrashReportDll=0
)


rem Do the SubWCRev substitution for revision numbers in the two necessary files.
ECHO MakeMsi.bat: checking version information
if NOT EXIST VersionNumberInclude.wxi (
cd ..\..\
call version.bat
cd src\TortoiseSVNSetup
)
echo checking files
if NOT EXIST VersionNumberInclude.wxi (
echo No version information!
exit /b
)
if NOT EXIST MakeMsiSub.bat (SubWCRev.exe ..\.. MakeMsiSub.in.bat MakeMsiSub.bat)
if NOT EXIST MakeMsiSub.bat (copy MakeMsiSub.in.bat MakeMsiSub.bat)


rem Build the actual setup package.
ECHO MakeMsi.bat: calling MakeMsiSub.bat
call MakeMsiSub.bat
ECHO MakeMsi.bat: Cleaning up...
del MakeMsiSub.bat
del VersionNumberInclude.wxi
ECHO MakeMsi.bat: Done.
