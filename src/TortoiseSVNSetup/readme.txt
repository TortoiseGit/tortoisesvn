To create an installer for TortoiseSVN you need to have the WiX package
in your PATH variable. You can get WiX from http://wix.sourceforge.net

copy ..\..\..\subversion\apr-iconv\LibR\iconv\*.so ..\..\bin\%TSVNBUILD%\iconv

Build documentation by following the instructions SVN\tsvn\doc\readme.txt
and SVN\tsvn\doc\build.txt.

Then simply run MakeMsi.bat.
