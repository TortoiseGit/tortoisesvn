@echo off

rem +----------------------------------------------------------------------
rem | extracting the parameters for the generation process
SET _APP=%1
shift
SET _LANG=%1
shift

SET _PDF=OFF
SET _CHM=OFF
SET _HTML=OFF

:getparam
if "%1"=="pdf" SET _PDF=ON
if "%1"=="PDF" SET _PDF=ON
if "%1"=="chm" SET _CHM=ON
if "%1"=="CHM" SET _CHM=ON
if "%1"=="html" SET _HTML=ON
if "%1"=="HTML" SET _HTML=ON
shift
if NOT "%1"=="" goto :getparam


rem +----------------------------------------------------------------------
rem | for VS.Net Projects only
rem | BEWARE, this may cause a "line too long" error if called repeatedly from the same dos box
rem | Try to check, whether these vars are already set
@if "%VSINSTALLDIR%"=="" call "%VS71COMNTOOLS%\vsvars32.bat"

rem +----------------------------------------------------------------------
rem | Set Paths to helper apps
rem | %~dp0 is the expanded pathname of the current script under NT
set _DOC_SRC=
if "%OS%"=="Windows_NT" set _DOC_SRC=%~dp0

set _DOC_HOME=%_DOC_SRC%..\
SET _DOCTOOLS=%_DOC_HOME%tools\
SET _XSLTPROC=%_DOCTOOLS%xsltproc.exe
SET _HHCPROC=%_DOCTOOLS%hhc.exe
SET _FOPPROC=%_DOCTOOLS%fop\fop.bat 

set _OUTPUT=%_DOC_HOME%output

rem +----------------------------------------------------------------------
rem | Book xsltproc options for HTML and PDF output
rem set _DOC_PDF_XSLTPROC_OPTS=--stringparam paper.type A4
SET _DOC_HTML_XSLTPROC_OPTS=--stringparam html.stylesheet styles_html.css
SET _DOC_HELP_XSLTPROC_OPTS=--stringparam html.stylesheet styles_chm.css

rem +----------------------------------------------------------------------
rem | Set Paths to source, stylesheets amd targets

set _DOC_XML_SRC=%_APP%.xml
set _DOC_XSL_FO=%_DOC_SRC%\pdfdoc.xsl
set _DOC_XSL_HELP=%_DOC_SRC%\htmlhelp.xsl
set _DOC_XSL_HTMLSINGLE=%_DOCTOOLS%\xsl\html\docbook.xsl
set _DOC_XSL_HTMLCHUNK=%_DOCTOOLS%\xsl\html\chunk.xsl

set _HELP_RESOURCE=..\..\..\src\%_APP%\resource.h

rem Exception for TortoiseProc, Grmpf, 
rem is there a simple way to convert the content of a variable to lowercase?
if %_APP%==TortoiseSVN  set _HELP_RESOURCE=..\..\..\src\TortoiseProc\resource.h
if %_APP%==tortoisesvn  set _HELP_RESOURCE=..\..\..\src\TortoiseProc\resource.h

set _HTML_TARGET=%_OUTPUT%\%_APP%_%_LANG%\
set _FO_TARGET=%_HTML_TARGET%\%_APP%.fo
set _PDF_TARGET=%_OUTPUT%\%_APP%_%_LANG%.pdf
set _HELP_TARGET=%_OUTPUT%\%_APP%_%_LANG%.chm

rem exit if no source exists
if not exist %_DOC_SRC%\%_LANG%\%_DOC_XML_SRC% goto :EOF

echo.
echo ----------------------------------------------------------------------
echo Generating: %_DOC_SRC% %_LANG% %_APP%
echo PDF=%_PDF% CHM=%_CHM% HTML=%_HTML%
echo.
cd %_DOC_SRC%%_LANG%

rmdir /s /q %_HTML_TARGET% > NUL
mkdir %_HTML_TARGET% > NUL

rem First copy the default images and afterwards copy the localized images
xcopy %_DOC_HOME%\images %_HTML_TARGET%images\ > NUL
xcopy %_DOC_SRC%\%_LANG%\images %_HTML_TARGET%images\ > NUL

if %_PDF%==ON (
  echo ----------------------------------------------------------------------
  echo Generating Help as PDF File
  %_XSLTPROC% %_DOC_PDF_XSLTPROC_OPTS% --output %_FO_TARGET% %_DOC_XSL_FO% %_DOC_XML_SRC%
  call %_FOPPROC% -fo %_FO_TARGET% -pdf %_PDF_TARGET%
)

if %_CHM%==ON (
  echo ----------------------------------------------------------------------
  echo Generating Help as CHM File
  del /q %_HTML_TARGET%
  copy %_DOC_SRC%\styles_chm.css %_HTML_TARGET% > NUL
  %_XSLTPROC% %_DOC_HELP_XSLTPROC_OPTS% --output %_HTML_TARGET% %_DOC_XSL_HELP% %_DOC_XML_SRC%
  call :MakeHelpContext
  %_HHCPROC% %_HTML_TARGET%\htmlhelp.hhp
  copy %_HTML_TARGET%\htmlhelp.chm %_HELP_TARGET%
)

if %_HTML%==ON (
  echo ----------------------------------------------------------------------
  echo Generating Help as single HTML page
  del /q %_HTML_TARGET%
  copy %_DOC_SRC%\styles_html.css %_HTML_TARGET% > NUL
  %_XSLTPROC% %_DOC_HTML_XSLTPROC_OPTS% --output %_HTML_TARGET%\help-onepage.html %_DOC_XSL_HTMLSINGLE% %_DOC_XML_SRC%
  echo ----------------------------------------------------------------------
  echo Generating Help as multiple HTML pages
  %_XSLTPROC% %_DOC_HTML_XSLTPROC_OPTS% --output %_HTML_TARGET% %_DOC_XSL_HTMLCHUNK% %_DOC_XML_SRC%
)


echo ----------------------------------------------------------------------
echo Phew, done 
cd %_DOC_SRC%
Goto :EOF
rem | End of Batch execution -> Exit script
rem +----------------------------------------------------------------------

rem for VS.Net Projects only
rem subroutine to generate the help context file
echo ----------------------------------------------------------------------
echo Generating the help context file
:MakeHelpContext

echo // Generated Help Map file. > "%_HTML_TARGET%\context.h"
echo. >> "%_HTML_TARGET%\context.h"
echo // Commands (ID_* and IDM_*) >> "%_HTML_TARGET%\context.h"
makehm /h ID_,HID_,0x10000 IDM_,HIDM_,0x10000 "%_HELP_RESOURCE%" >> "%_HTML_TARGET%\context.h"
echo. >> "%_HTML_TARGET%\context.h"
echo // Prompts (IDP_*) >> "%_HTML_TARGET%\context.h"
makehm /h IDP_,HIDP_,0x30000 "%_HELP_RESOURCE%" >> "%_HTML_TARGET%\context.h"
echo. >> "%_HTML_TARGET%\context.h"
echo // Resources (IDR_*) >> "%_HTML_TARGET%\context.h"
makehm /h IDR_,HIDR_,0x20000 "%_HELP_RESOURCE%" >> "%_HTML_TARGET%\context.h"
echo. >> "%_HTML_TARGET%\context.h"
echo // Dialogs (IDD_*) >> "%_HTML_TARGET%\context.h"
makehm /h IDD_,HIDD_,0x20000 "%_HELP_RESOURCE%" >> "%_HTML_TARGET%\context.h"
echo. >> "%_HTML_TARGET%\context.h"
echo // Frame Controls (IDW_*) >> "%_HTML_TARGET%\context.h"
makehm /h /a afxhh.h IDW_,HIDW_,0x50000 "%_HELP_RESOURCE%" >> "%_HTML_TARGET%\context.h"

Goto :EOF
rem | End of Subroutine -> return to caller
rem +----------------------------------------------------------------------
