@echo off
cd tools
type ..\API\Doxyfile > Doxyfile
echo HHC_LOCATION=%CD%\hhc.exe >> Doxyfile
doxygen.exe Doxyfile
del Doxyfile
del ..\output\API\TortoiseAPI.chm
copy ..\output\API\html\TortoiseAPI.chm ..\output\API\TortoiseAPI.chm
cd ..
