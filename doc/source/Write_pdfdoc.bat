@echo off
echo ^<xsl:stylesheet > pdfdoc.xsl
echo  xmlns:xsl="http://www.w3.org/1999/XSL/Transform" >> pdfdoc.xsl
echo  xmlns:fo="http://www.w3.org/1999/XSL/Format" >> pdfdoc.xsl
echo  version="1.0"^> >> pdfdoc.xsl
echo. >> pdfdoc.xsl
echo  ^<xsl:import href="%XSLROOT%/fo/docbook.xsl"/^> >> pdfdoc.xsl
echo  ^<xsl:import href="./defaults.xsl"/^> >> pdfdoc.xsl
echo  ^<xsl:param name="paper.type" select="'A4'"^>^</xsl:param^> >> pdfdoc.xsl
echo  ^<xsl:param name="double.sided" select="1"^>^</xsl:param^> >> pdfdoc.xsl
echo  ^<xsl:param name="variablelist.as.blocks" select="1"^>^</xsl:param^> >> pdfdoc.xsl
echo  ^<xsl:param name="symbol.font.family" select="'Symbol,ZapfDingbats'"^>^</xsl:param^> >> pdfdoc.xsl
echo  ^<xsl:param name="draft.mode" select="no"/^> >> pdfdoc.xsl
echo. >> pdfdoc.xsl
echo  ^<xsl:param name="formal.title.placement"^> >> pdfdoc.xsl
echo	figure after >> pdfdoc.xsl
echo	example after >> pdfdoc.xsl
echo	equation after >> pdfdoc.xsl
echo	table after >> pdfdoc.xsl
echo	procedure after >> pdfdoc.xsl
echo  ^</xsl:param^> >> pdfdoc.xsl
echo. >> pdfdoc.xsl
echo ^<xsl:template match="menuchoice"^> >> pdfdoc.xsl
echo  ^<fo:inline font-family="Helvetica"^> >> pdfdoc.xsl
echo    ^<xsl:call-template name="process.menuchoice"/^> >> pdfdoc.xsl
echo  ^</fo:inline^> >> pdfdoc.xsl
echo ^</xsl:template^> >> pdfdoc.xsl
echo. >> pdfdoc.xsl
echo ^<xsl:template match="guilabel"^> >> pdfdoc.xsl
echo  ^<fo:inline font-family="Helvetica"^> >> pdfdoc.xsl
echo    ^<xsl:call-template name="inline.charseq"/^> >> pdfdoc.xsl
echo  ^</fo:inline^> >> pdfdoc.xsl
echo ^</xsl:template^> >> pdfdoc.xsl
echo. >> pdfdoc.xsl
echo ^<xsl:template match="guibutton"^> >> pdfdoc.xsl
echo  ^<fo:inline font-family="Helvetica"^> >> pdfdoc.xsl
echo    ^<xsl:call-template name="inline.charseq"/^> >> pdfdoc.xsl
echo  ^</fo:inline^> >> pdfdoc.xsl
echo ^</xsl:template^> >> pdfdoc.xsl
echo. >> pdfdoc.xsl
echo ^<xsl:template match="keysym"^> >> pdfdoc.xsl
echo  ^<fo:inline font-family="Symbol"^> >> pdfdoc.xsl
echo    ^<xsl:call-template name="inline.charseq"/^> >> pdfdoc.xsl
echo  ^</fo:inline^> >> pdfdoc.xsl
echo ^</xsl:template^> >> pdfdoc.xsl
echo. >> pdfdoc.xsl
echo ^</xsl:stylesheet^> >> pdfdoc.xsl