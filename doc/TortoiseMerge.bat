@echo off
call "%VS71COMNTOOLS%\vsvars32.bat"
cd english
mkdir ..\output
mkdir ..\output\TortoiseMerge
..\tools\xsltproc.exe  --stringparam html.stylesheet styles.css --output ..\output\TortoiseMerge\help.html ..\tools\xsl\html\docbook.xsl merge.xml
mkdir ..\output\TortoiseMerge\html-chunk
..\tools\xsltproc.exe  --stringparam html.stylesheet styles.css --output ..\output\TortoiseMerge\html-chunk\ ..\tools\xsl\html\chunk.xsl merge.xml
copy styles.css ..\output\TortoiseMerge\html-chunk\
copy ..\images\*.png ..\output\TortoiseMerge\html-chunk\

mkdir ..\output\TortoiseMerge\html-help
..\tools\xsltproc.exe  --stringparam html.stylesheet styles.css --output ..\output\TortoiseMerge\html-help\ htmlhelp.xsl merge.xml

rem now start the dev command to overwrite the context.h file
echo // Generated Help Map file. > "..\output\TortoiseMerge\html-help\context.h"
echo. >> "..\output\TortoiseMerge\html-help\context.h"
echo // Commands (ID_* and IDM_*) >> "..\output\TortoiseMerge\html-help\context.h"
makehm /h ID_,HID_,0x10000 IDM_,HIDM_,0x10000 "..\..\src\TortoiseMerge\resource.h" >> "..\output\TortoiseMerge\html-help\context.h"
echo. >> "..\output\TortoiseMerge\html-help\context.h"
echo // Prompts (IDP_*) >> "..\output\TortoiseMerge\html-help\context.h"
makehm /h IDP_,HIDP_,0x30000 "..\..\src\TortoiseMerge\resource.h" >> "..\output\TortoiseMerge\html-help\context.h"
echo. >> "..\output\TortoiseMerge\html-help\context.h"
echo // Resources (IDR_*) >> "..\output\TortoiseMerge\html-help\context.h"
makehm /h IDR_,HIDR_,0x20000 "..\..\src\TortoiseMerge\resource.h" >> "..\output\TortoiseMerge\html-help\context.h"
echo. >> "..\output\TortoiseMerge\html-help\context.h"
echo // Dialogs (IDD_*) >> "..\output\TortoiseMerge\html-help\context.h"
makehm /h IDD_,HIDD_,0x20000 "..\..\src\TortoiseMerge\resource.h" >> "..\output\TortoiseMerge\html-help\context.h"
echo. >> "..\output\TortoiseMerge\html-help\context.h"
echo // Frame Controls (IDW_*) >> "..\output\TortoiseMerge\html-help\context.h"
makehm /h /a afxhh.h IDW_,HIDW_,0x50000 "..\..\src\TortoiseMerge\resource.h" >> "..\output\TortoiseMerge\html-help\context.h"


copy styles.css ..\output\TortoiseMerge\html-help\
copy ..\images\*.png ..\output\TortoiseMerge\html-help\
..\tools\hhc.exe ..\output\TortoiseMerge\html-help\htmlhelp.hhp
del ..\output\TortoiseMerge\html-help\TortoiseMerge.chm
ren ..\output\TortoiseMerge\html-help\htmlhelp.chm TortoiseMerge.chm
del ..\..\bin\Debug\TortoiseMerge.chm
del ..\..\bin\Release\TortoiseMerge.chm
copy ..\output\TortoiseMerge\html-help\TortoiseMerge.chm ..\..\bin\Debug
copy ..\output\TortoiseMerge\html-help\TortoiseMerge.chm ..\..\bin\Release
cd ..

