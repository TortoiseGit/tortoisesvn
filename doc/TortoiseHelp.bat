@echo off
cd english
..\tools\xsltproc.exe  --stringparam html.stylesheet styles.css --output help.html ..\tools\xsl\html\docbook.xsl book.xml
mkdir html-chunk
..\tools\xsltproc.exe  --stringparam html.stylesheet styles.css --output html-chunk\ ..\tools\xsl\html\chunk.xsl book.xml
copy styles.css html-chunk\
copy ..\images\*.png html-chunk\

mkdir html-help
..\tools\xsltproc.exe  --stringparam html.stylesheet styles.css --output html-help\ ..\tools\xsl\htmlhelp\htmlhelp.xsl book.xml
copy styles.css html-help\
copy ..\images\*.png html-help\
..\tools\hhc.exe html-help\htmlhelp.hhp
del html-help\TortoiseSVNHelp.chm
ren html-help\htmlhelp.chm TortoiseSVNHelp.chm
cd ..

