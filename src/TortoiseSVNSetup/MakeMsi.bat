@echo off
rem Check the environment
@if "%VSINSTALLDIR%"=="" call "%VS71COMNTOOLS%\vsvars32.bat"
if "%TortoiseVars%"=="" call ..\..\TortoiseVars.bat
SETLOCAL ENABLEDELAYEDEXPANSION

rem Generate the RTF version of subversion license.txt for the setup package to use.
ECHO MakeMsi.bat: Generating RTF License file from subversion license at ..\..\ext\Subversion\subversion\LICENSE
Text2RTF.exe -tO "..\..\ext\Subversion\subversion\LICENSE" > "include\subversion license.rtf"


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
if EXIST ..\..\..\MYBUILD ( 
	SET IncludeCrashReportDll=1 
) ELSE ( 
	SET IncludeCrashReportDll=0
)


rem Do the SubWCRev substitution for revision numbers in the two necessary files.
ECHO MakeMsi.bat: Calling SubWCRev
..\..\bin\release\bin\SubWCRev.exe ..\.. VersionNumberInclude.in.wxi VersionNumberInclude.wxi
if NOT EXIST VersionNumberInclude.wxi (copy VersionNumberInclude.in.wxi VersionNumberInclude.wxi)
..\..\bin\release\bin\SubWCRev.exe ..\.. MakeMsiSub.in.bat MakeMsiSub.bat
if NOT EXIST MakeMsiSub.bat (copy MakeMsiSub.in.bat MakeMsiSub.bat)


rem Build the actual setup package.
ECHO MakeMsi.bat: calling MakeMsiSub.bat
call MakeMsiSub.bat
ECHO MakeMsi.bat: Cleaning up...
del MakeMsiSub.bat
del VersionNumberInclude.wxi
del "include\subversion license.rtf"
ECHO MakeMsi.bat: Done.