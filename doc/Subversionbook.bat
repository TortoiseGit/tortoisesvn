@echo off
echo ^<!ENTITY svn.version "Draft"^> > ..\..\Subversion\doc\book\book\version.xml
cd ..\..\Subversion\doc\book\book
..\..\..\..\tortoisesvn\doc\tools\xsltproc.exe --stringparam html.stylesheet styles.css --output book.html ..\..\..\..\tortoisesvn\doc\tools\xsl\html\docbook.xsl book.xml
 
mkdir html-chunk
..\..\..\..\tortoisesvn\doc\tools\xsltproc.exe --stringparam html.stylesheet styles.css --output html-chunk\ ..\..\..\..\tortoisesvn\doc\tools\xsl\html\chunk.xsl book.xml
copy styles.css html-chunk\
copy images\*.png html-chunk\

mkdir html-help
..\..\..\..\tortoisesvn\doc\tools\xsltproc.exe --stringparam html.stylesheet styles.css --output html-help\ ..\..\..\..\tortoisesvn\doc\tools\xsl\htmlhelp\htmlhelp.xsl book.xml
copy styles.css html-help\
copy images\*.png html-help\
..\..\..\..\tortoisesvn\doc\tools\hhc.exe html-help\htmlhelp.hhp
del html-help\subversion-book.chm
ren html-help\htmlhelp.chm subversion-book.chm
cd ..\..\..\..\tortoisesvn\doc

