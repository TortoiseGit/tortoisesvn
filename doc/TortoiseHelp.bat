@echo off
call "%VS71COMNTOOLS%\vsvars32.bat"
cd english
mkdir ..\output

rmdir /s /q ..\output\TortoiseSVN\
mkdir ..\output\TortoiseSVN
..\tools\xsltproc.exe  --stringparam html.stylesheet styles.css --output ..\output\TortoiseSVN\help.html ..\tools\xsl\html\docbook.xsl book.xml

mkdir ..\output\TortoiseSVN\html-chunk
..\tools\xsltproc.exe  --stringparam html.stylesheet styles.css --output ..\output\TortoiseSVN\html-chunk\ ..\tools\xsl\html\chunk.xsl book.xml
copy styles.css ..\output\TortoiseSVN\html-chunk\
copy ..\images\*.png ..\output\TortoiseSVN\html-chunk\

mkdir ..\output\TortoiseSVN\html-help
..\tools\xsltproc.exe  --stringparam html.stylesheet styles.css --output ..\output\TortoiseSVN\html-help\ htmlhelp.xsl book.xml

rem now start the dev command to overwrite the context.h file
echo // Generated Help Map file. > "..\output\TortoiseSVN\html-help\context.h"
echo. >> "..\output\TortoiseSVN\html-help\context.h"
echo // Commands (ID_* and IDM_*) >> "..\output\TortoiseSVN\html-help\context.h"
makehm /h ID_,HID_,0x10000 IDM_,HIDM_,0x10000 "..\..\src\TortoiseProc\resource.h" >> "..\output\TortoiseSVN\html-help\context.h"
echo. >> "..\output\TortoiseSVN\html-help\context.h"
echo // Prompts (IDP_*) >> "..\output\TortoiseSVN\html-help\context.h"
makehm /h IDP_,HIDP_,0x30000 "..\..\src\TortoiseProc\resource.h" >> "..\output\TortoiseSVN\html-help\context.h"
echo. >> "..\output\TortoiseSVN\html-help\context.h"
echo // Resources (IDR_*) >> "..\output\TortoiseSVN\html-help\context.h"
makehm /h IDR_,HIDR_,0x20000 "..\..\src\TortoiseProc\resource.h" >> "..\output\TortoiseSVN\html-help\context.h"
echo. >> "..\output\TortoiseSVN\html-help\context.h"
echo // Dialogs (IDD_*) >> "..\output\TortoiseSVN\html-help\context.h"
makehm /h IDD_,HIDD_,0x20000 "..\..\src\TortoiseProc\resource.h" >> "..\output\TortoiseSVN\html-help\context.h"
echo. >> "..\output\TortoiseSVN\html-help\context.h"
echo // Frame Controls (IDW_*) >> "..\output\TortoiseSVN\html-help\context.h"
makehm /h /a afxhh.h IDW_,HIDW_,0x50000 "..\..\src\TortoiseProc\resource.h" >> "..\output\TortoiseSVN\html-help\context.h"


copy styles.css ..\output\TortoiseSVN\html-help\
copy ..\images\*.png ..\output\TortoiseSVN\html-help\
..\tools\hhc.exe ..\output\TortoiseSVN\html-help\htmlhelp.hhp
del ..\output\TortoiseSVN\html-help\TortoiseProc.chm
ren ..\output\TortoiseSVN\html-help\htmlhelp.chm TortoiseProc.chm
del ..\..\bin\Debug\TortoiseProc.chm
del ..\..\bin\Release\TortoiseProc.chm
copy ..\output\TortoiseSVN\html-help\TortoiseProc.chm ..\..\bin\Debug
copy ..\output\TortoiseSVN\html-help\TortoiseProc.chm ..\..\bin\Release
cd ..

