@echo off

if "%TortoiseVars%"=="" call ..\TortoiseVars.bat

type API\DoxyfileSVN > Doxyfile
echo HHC_LOCATION=%HHCLOC% >> Doxyfile
doxygen.exe Doxyfile
del Doxyfile
del output\doxygen\SubversionAPI.chm
copy output\doxygen\html\SubversionAPI.chm output\doxygen\SubversionAPI.chm
pause
rmdir /s /q output\doxygen\html

type API\Doxyfile > Doxyfile
echo HHC_LOCATION=%HHCLOC% >> Doxyfile
doxygen.exe Doxyfile
del Doxyfile
del output\doxygen\TortoiseAPI.chm
copy output\doxygen\html\TortoiseAPI.chm output\doxygen\TortoiseAPI.chm
rmdir /s /q output\doxygen\html
