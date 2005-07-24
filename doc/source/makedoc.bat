@echo off

rem +----------------------------------------------------------------------
rem | extracting the parameters for the generation process
SET _APP=%1
shift
SET _LANG=%1
shift

if "%TortoiseVars%"=="" call ..\..\TortoiseVars.bat

call Write_htmlhelp.bat
call Write_pdfdoc.bat

SET _PDF=OFF
SET _CHM=OFF
SET _HTML=OFF

:getparam
if "%1"=="pdf" SET _PDF=ON
if "%1"=="PDF" SET _PDF=ON
if "%1"=="chm" SET _CHM=ON
if "%1"=="CHM" SET _CHM=ON
if "%1"=="single" SET _HTML=SINGLE
if "%1"=="SINGLE" SET _HTML=SINGLE
if "%1"=="html" SET _HTML=ON
if "%1"=="HTML" SET _HTML=ON
shift
if NOT "%1"=="" goto :getparam


rem +----------------------------------------------------------------------
rem | for VS.Net Projects only
rem | BEWARE, this may cause a "line too long" error if called repeatedly from the same dos box
rem | Try to check, whether these vars are already set
rem | Skip batch file if it doesn't exist (on systems without VS.Net)
if NOT "%VSINSTALLDIR%"=="" goto :vsvarsdone
if EXIST "%VS71COMNTOOLS%\vsvars32.bat" call "%VS71COMNTOOLS%\vsvars32.bat"
:vsvarsdone

rem +----------------------------------------------------------------------
rem | Set Paths to helper apps
rem | %~dp0 is the expanded pathname of the current script under NT
set _DOC_SRC=
if "%OS%"=="Windows_NT" set _DOC_SRC=%~dp0

set _DOC_HOME=%_DOC_SRC%..\

SET _XSLTPROC=xsltproc.exe
SET _HHCPROC=hhc.exe
SET _FOPPROC=fop.bat

set _OUTPUT=%_DOC_HOME%output

rem +----------------------------------------------------------------------
rem | Book xsltproc options for HTML and PDF output
SET _DOC_PDF_XSLTPROC_OPTS= --nonet --xinclude
SET _DOC_HTML_XSLTPROC_OPTS= --nonet --xinclude --stringparam html.stylesheet styles_html.css
SET _DOC_HELP_XSLTPROC_OPTS= --nonet --xinclude --stringparam html.stylesheet styles_chm.css

rem +----------------------------------------------------------------------
rem | Set Paths to source, stylesheets amd targets

set _DOC_XML_SRC=%_APP%.xml

rem Settings for PDF docs
set _DOC_XSL_FO=%_DOC_SRC%\pdfdoc.xsl

rem Settings for HTML Help docs
set _DOC_CSS_HELP=%_DOC_SRC%\styles_chm.css
set _DOC_XSL_HELP=%_DOC_SRC%\htmlhelp.xsl

rem Settings for plain HTML docs
set _DOC_CSS_HTML=%_DOC_SRC%\styles_*.css
set _DOC_XSL_HTMLSINGLE=%XSLROOT%\html\docbook.xsl
set _DOC_XSL_HTMLCHUNK=%XSLROOT%\html\chunk.xsl

set _HELP_RESOURCE=..\..\..\src\%_APP%\resource.h

rem Exception for TortoiseProc, Grmpf,
rem is there a simple way to convert the content of a variable to lowercase?
if %_APP%==TortoiseSVN  set _HELP_RESOURCE=..\..\..\src\TortoiseProc\resource.h
if %_APP%==tortoisesvn  set _HELP_RESOURCE=..\..\..\src\TortoiseProc\resource.h

set _HTML_TARGET=%_OUTPUT%\%_APP%_%_LANG%\
set _FO_TARGET=%_HTML_TARGET%\%_APP%.fo
set _PDF_TARGET=%_OUTPUT%\%_APP%_%_LANG%.pdf
set _HELP_TARGET=%_OUTPUT%\%_APP%_%_LANG%.chm

rem Get revision info only for english docs
if %_LANG%==en (
   if exist ..\..\bin\Release\bin\SubWCRev.exe (
      ..\..\bin\Release\bin\SubWCRev.exe %_DOC_SRC%%_LANG% %_DOC_SRC%Pubdate.tmpl %_DOC_SRC%%_LANG%\Pubdate.xml
   ) else (
      copy %_DOC_SRC%Pubdate.none %_DOC_SRC%%_LANG%\Pubdate.xml /Y
   )
) else (
   copy %_DOC_SRC%Pubdate.none %_DOC_SRC%%_LANG%\Pubdate.xml /Y
)

rem exit if no source exists
if not exist %_DOC_SRC%\%_LANG%\%_DOC_XML_SRC% goto :EOF

echo.
echo ----------------------------------------------------------------------
echo Generating: %_DOC_SRC% %_LANG% %_APP%
echo PDF=%_PDF% CHM=%_CHM% HTML=%_HTML%
echo.
cd %_DOC_SRC%%_LANG%

rem avoid error message when directory doesn't exist
if not exist %_HTML_TARGET% goto :no_remove_htmltarget
rmdir /s /q %_HTML_TARGET% > NUL
:no_remove_htmltarget
mkdir %_HTML_TARGET% > NUL

rem First copy the default images and afterwards copy the localized images
xcopy /y %_DOC_HOME%\images\en %_HTML_TARGET%images\ > NUL
rem Skip the localized images if there are none
if not exist %_DOC_HOME%\images\%_LANG%\*.* goto :imagecopydone
xcopy /y %_DOC_HOME%\images\%_LANG% %_HTML_TARGET%images\ > NUL
:imagecopydone

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
  copy %_DOC_CSS_HELP% %_HTML_TARGET% > NUL
  %_XSLTPROC% %_DOC_HELP_XSLTPROC_OPTS% --output %_HTML_TARGET% %_DOC_XSL_HELP% %_DOC_XML_SRC%
  call :MakeHelpContext
  %_HHCPROC% %_HTML_TARGET%\htmlhelp.hhp
  copy %_HTML_TARGET%\htmlhelp.chm %_HELP_TARGET%
)

if not %_HTML%==OFF (
  echo ----------------------------------------------------------------------
  echo Generating Help as single HTML page
  del /q %_HTML_TARGET%
  copy %_DOC_CSS_HTML% %_HTML_TARGET% > NUL
  %_XSLTPROC% %_DOC_HTML_XSLTPROC_OPTS% --output %_HTML_TARGET%\help-onepage.html %_DOC_XSL_HTMLSINGLE% %_DOC_XML_SRC%
  if %_HTML%==ON (
    echo ----------------------------------------------------------------------
    echo Generating Help as multiple HTML pages
    %_XSLTPROC% %_DOC_HTML_XSLTPROC_OPTS% --output %_HTML_TARGET% %_DOC_XSL_HTMLCHUNK% %_DOC_XML_SRC%
  )
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
