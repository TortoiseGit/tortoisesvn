@echo off
SETLOCAL ENABLEDELAYEDEXPANSION
set HELPOUTDIR=..\output\TortoiseSVN
set TOOLDIR=..\Tools
set RESOURCEFILE="..\..\src\TortoiseProc\resource.h"
set PROGRAMNAME=TortoiseSVN



call "%VS71COMNTOOLS%\vsvars32.bat" > NUL
cd Help
set LIST=

set LIST =
FOR %%V In (tortoisesvn_*.xml) Do set LIST=!LIST! %%V
set LIST=%LIST:tortoisesvn_=%
set LIST=%LIST:.xml=%

rmdir /s /q %HELPOUTDIR% > NUL
mkdir %HELPOUTDIR% > NUL

mkdir %HELPOUTDIR%\html > NUL
mkdir %HELPOUTDIR%\html-help > NUL
mkdir %HELPOUTDIR%\pdf > NUL
FOR %%V In (%LIST%) Do mkdir %HELPOUTDIR%\html\%%V > NUL
FOR %%V In (%LIST%) Do mkdir %HELPOUTDIR%\html-help\%%V > NUL
echo generating single html page(s)
FOR %%V In (%LIST%) Do %TOOLDIR%\xsltproc.exe  --stringparam html.stylesheet styles.css --output %HELPOUTDIR%\html\%%V\help-onepage.html %TOOLDIR%\xsl\html\docbook.xsl tortoisesvn_%%V.xml
echo generating multiple html page(s)
FOR %%V In (%LIST%) Do %TOOLDIR%\xsltproc.exe  --stringparam html.stylesheet styles.css --output %HELPOUTDIR%\html\%%V\ %TOOLDIR%\xsl\html\chunk.xsl tortoisesvn_%%V.xml
echo generating data for htmlhelp compiler
FOR %%V In (%LIST%) Do %TOOLDIR%\xsltproc.exe  --stringparam html.stylesheet styles.css --stringparam htmlhelp.chm %PROGRAMNAME%_%%V.chm --output %HELPOUTDIR%\html-help\%%V\ htmlhelp.xsl tortoisesvn_%%V.xml
echo generating data for pdf processing
FOR %%V In (%LIST%) Do %TOOLDIR%\xsltproc.exe  --output %HELPOUTDIR%\pdf\tortoisesvn_%%V.fo fo-stylesheet.xsl tortoisesvn_%%V.xml
FOR %%V In (%LIST%) Do copy styles.css %HELPOUTDIR%\html\%%V > NUL
FOR %%V In (%LIST%) Do copy styles.css %HELPOUTDIR%\html-help\%%V > NUL
FOR %%V In (%LIST%) Do copy ..\images\*.png %HELPOUTDIR%\html\%%V > NUL
FOR %%V In (%LIST%) Do copy ..\images\*.png %HELPOUTDIR%\html-help\%%V > NUL
copy ..\images\*.png %HELPOUTDIR%\pdf > NUL
echo processing pdf(s)
FOR %%V In (%LIST%) Do call %TOOLDIR%\fop\fop -fo %HELPOUTDIR%\pdf\tortoisesvn_%%V.fo -pdf %HELPOUTDIR%\tortoisesvn_%%V.pdf -q
echo finished processing pdf(s)
rmdir /s /q %HELPOUTDIR%\pdf

echo generate header file from resources
rem now start the dev command to overwrite the context.h file
echo // Generated Help Map file. > "%HELPOUTDIR%\html-help\context.h"
echo. >> "..\..\output\Docs\html-help\context.h"
echo // Commands (ID_* and IDM_*) >> "%HELPOUTDIR%\html-help\context.h"
makehm /h ID_,HID_,0x10000 IDM_,HIDM_,0x10000 "%RESOURCEFILE%" >> "%HELPOUTDIR%\html-help\context.h"
echo. >> "%HELPOUTDIR%\html-help\context.h"
echo // Prompts (IDP_*) >> "%HELPOUTDIR%\html-help\context.h"
makehm /h IDP_,HIDP_,0x30000 "%RESOURCEFILE%" >> "%HELPOUTDIR%\html-help\context.h"
echo. >> "%HELPOUTDIR%\html-help\context.h"
echo // Resources (IDR_*) >> "%HELPOUTDIR%\html-help\context.h"
makehm /h IDR_,HIDR_,0x20000 "%RESOURCEFILE%" >> "%HELPOUTDIR%\html-help\context.h"
echo. >> "%HELPOUTDIR%\html-help\context.h"
echo // Dialogs (IDD_*) >> "%HELPOUTDIR%\html-help\context.h"
makehm /h IDD_,HIDD_,0x20000 "%RESOURCEFILE%" >> "%HELPOUTDIR%\html-help\context.h"
echo. >> "%HELPOUTDIR%\html-help\context.h"
echo // Frame Controls (IDW_*) >> "%HELPOUTDIR%\html-help\context.h"
makehm /h /a afxhh.h IDW_,HIDW_,0x50000 "%RESOURCEFILE%" >> "%HELPOUTDIR%\html-help\context.h"

FOR %%V In (%LIST%) Do copy %HELPOUTDIR%\html-help\context.h %HELPOUTDIR%\html-help\%%V > NUL
echo build htmlhelp file(s) (*.chm)
FOR %%V In (%LIST%) Do %TOOLDIR%\hhc.exe %HELPOUTDIR%\html-help\%%V\htmlhelp.hhp > NUL

FOR %%V In (%LIST%) Do del ..\..\bin\Debug\%PROGRAMNAME%_%%V.chm > NUL
FOR %%V In (%LIST%) Do del ..\..\bin\Release\%PROGRAMNAME%_%%V.chm > NUL
FOR %%V In (%LIST%) Do copy %HELPOUTDIR%\html-help\%%V\%PROGRAMNAME%_%%V.chm %HELPOUTDIR% > NUL
FOR %%V In (%LIST%) Do copy %HELPOUTDIR%\html-help\%%V\%PROGRAMNAME%_%%V.chm ..\..\bin\Debug > NUL
FOR %%V In (%LIST%) Do copy %HELPOUTDIR%\html-help\%%V\%PROGRAMNAME%_%%V.chm ..\..\bin\Release > NUL

rmdir /s /q %HELPOUTDIR%\html-help

rem ****************************************************************************

set HELPOUTDIR=..\output\TortoiseMerge
set RESOURCEFILE="..\..\src\TortoiseMerge\resource.h"
set PROGRAMNAME=TortoiseMerge

set LIST=

set LIST =
FOR %%V In (tortoisemerge_*.xml) Do set LIST=!LIST! %%V
set LIST=%LIST:tortoisemerge_=%
set LIST=%LIST:.xml=%

rmdir /s /q %HELPOUTDIR% > NUL
mkdir %HELPOUTDIR% > NUL

mkdir %HELPOUTDIR%\html > NUL
mkdir %HELPOUTDIR%\html-help > NUL
mkdir %HELPOUTDIR%\pdf > NUL
FOR %%V In (%LIST%) Do mkdir %HELPOUTDIR%\html\%%V > NUL
FOR %%V In (%LIST%) Do mkdir %HELPOUTDIR%\html-help\%%V > NUL
echo generating single html page(s)
FOR %%V In (%LIST%) Do %TOOLDIR%\xsltproc.exe  --stringparam html.stylesheet styles.css --output %HELPOUTDIR%\html\%%V\help-onepage.html %TOOLDIR%\xsl\html\docbook.xsl tortoisemerge_%%V.xml
echo generating multiple html page(s)
FOR %%V In (%LIST%) Do %TOOLDIR%\xsltproc.exe  --stringparam html.stylesheet styles.css --output %HELPOUTDIR%\html\%%V\ %TOOLDIR%\xsl\html\chunk.xsl tortoisemerge_%%V.xml
echo generating data for htmlhelp compiler
FOR %%V In (%LIST%) Do %TOOLDIR%\xsltproc.exe  --stringparam html.stylesheet styles.css --stringparam htmlhelp.chm %PROGRAMNAME%_%%V.chm --output %HELPOUTDIR%\html-help\%%V\ htmlhelp.xsl tortoisemerge_%%V.xml
echo generating data for pdf processing
FOR %%V In (%LIST%) Do %TOOLDIR%\xsltproc.exe  --output %HELPOUTDIR%\pdf\tortoisemerge_%%V.fo fo-stylesheet.xsl tortoisemerge_%%V.xml
FOR %%V In (%LIST%) Do copy styles.css %HELPOUTDIR%\html\%%V > NUL
FOR %%V In (%LIST%) Do copy styles.css %HELPOUTDIR%\html-help\%%V > NUL
FOR %%V In (%LIST%) Do copy ..\images\*.png %HELPOUTDIR%\html\%%V > NUL
FOR %%V In (%LIST%) Do copy ..\images\*.png %HELPOUTDIR%\html-help\%%V > NUL
copy ..\images\*.png %HELPOUTDIR%\pdf > NUL
echo processing pdf(s)
FOR %%V In (%LIST%) Do call %TOOLDIR%\fop\fop -fo %HELPOUTDIR%\pdf\tortoisemerge_%%V.fo -pdf %HELPOUTDIR%\tortoisemerge_%%V.pdf -q
echo finished processing pdf(s)
rmdir /s /q %HELPOUTDIR%\pdf

echo generate header file from resources
rem now start the dev command to overwrite the context.h file
echo // Generated Help Map file. > "%HELPOUTDIR%\html-help\context.h"
echo. >> "..\..\output\Docs\html-help\context.h"
echo // Commands (ID_* and IDM_*) >> "%HELPOUTDIR%\html-help\context.h"
makehm /h ID_,HID_,0x10000 IDM_,HIDM_,0x10000 "%RESOURCEFILE%" >> "%HELPOUTDIR%\html-help\context.h"
echo. >> "%HELPOUTDIR%\html-help\context.h"
echo // Prompts (IDP_*) >> "%HELPOUTDIR%\html-help\context.h"
makehm /h IDP_,HIDP_,0x30000 "%RESOURCEFILE%" >> "%HELPOUTDIR%\html-help\context.h"
echo. >> "%HELPOUTDIR%\html-help\context.h"
echo // Resources (IDR_*) >> "%HELPOUTDIR%\html-help\context.h"
makehm /h IDR_,HIDR_,0x20000 "%RESOURCEFILE%" >> "%HELPOUTDIR%\html-help\context.h"
echo. >> "%HELPOUTDIR%\html-help\context.h"
echo // Dialogs (IDD_*) >> "%HELPOUTDIR%\html-help\context.h"
makehm /h IDD_,HIDD_,0x20000 "%RESOURCEFILE%" >> "%HELPOUTDIR%\html-help\context.h"
echo. >> "%HELPOUTDIR%\html-help\context.h"
echo // Frame Controls (IDW_*) >> "%HELPOUTDIR%\html-help\context.h"
makehm /h /a afxhh.h IDW_,HIDW_,0x50000 "%RESOURCEFILE%" >> "%HELPOUTDIR%\html-help\context.h"

FOR %%V In (%LIST%) Do copy %HELPOUTDIR%\html-help\context.h %HELPOUTDIR%\html-help\%%V > NUL
echo build htmlhelp file(s) (*.chm)
FOR %%V In (%LIST%) Do %TOOLDIR%\hhc.exe %HELPOUTDIR%\html-help\%%V\htmlhelp.hhp > NUL

FOR %%V In (%LIST%) Do del ..\..\bin\Debug\%PROGRAMNAME%_%%V.chm > NUL
FOR %%V In (%LIST%) Do del ..\..\bin\Release\%PROGRAMNAME%_%%V.chm > NUL
FOR %%V In (%LIST%) Do copy %HELPOUTDIR%\html-help\%%V\%PROGRAMNAME%_%%V.chm %HELPOUTDIR% > NUL
FOR %%V In (%LIST%) Do copy %HELPOUTDIR%\html-help\%%V\%PROGRAMNAME%_%%V.chm ..\..\bin\Debug > NUL
FOR %%V In (%LIST%) Do copy %HELPOUTDIR%\html-help\%%V\%PROGRAMNAME%_%%V.chm ..\..\bin\Release > NUL

rmdir /s /q %HELPOUTDIR%\html-help


cd ..
echo finished!

