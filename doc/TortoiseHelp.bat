@echo off
cd english
mkdir ..\output
mkdir ..\output\TortoiseSVN
..\tools\xsltproc.exe  --stringparam html.stylesheet styles.css --output ..\output\TortoiseSVN\help.html ..\tools\xsl\html\docbook.xsl book.xml
mkdir ..\output\TortoiseSVN\html-chunk
..\tools\xsltproc.exe  --stringparam html.stylesheet styles.css --output ..\output\TortoiseSVN\html-chunk\ ..\tools\xsl\html\chunk.xsl book.xml
copy styles.css ..\output\TortoiseSVN\html-chunk\
copy ..\images\*.png ..\output\TortoiseSVN\html-chunk\

mkdir ..\output\TortoiseSVN\html-help
..\tools\xsltproc.exe  --stringparam html.stylesheet styles.css --output ..\output\TortoiseSVN\html-help\ ..\tools\xsl\htmlhelp\htmlhelp.xsl book.xml
copy styles.css ..\output\TortoiseSVN\html-help\
copy ..\images\*.png ..\output\TortoiseSVN\html-help\
..\tools\hhc.exe ..\output\TortoiseSVN\html-help\htmlhelp.hhp
del ..\output\TortoiseSVN\html-help\TortoiseSVNHelp.chm
ren ..\output\TortoiseSVN\html-help\htmlhelp.chm TortoiseSVNHelp.chm
cd ..

