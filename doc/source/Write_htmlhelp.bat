@echo off
echo ^<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"^> > htmlhelp.xsl
echo. >> htmlhelp.xsl
echo ^<xsl:import href="%XSLROOT%/htmlhelp/htmlhelp.xsl"/^> >> htmlhelp.xsl
echo  ^<xsl:import href="./defaults.xsl"/^> >> htmlhelp.xsl
echo. >> htmlhelp.xsl
echo  ^<xsl:param name="suppress.navigation" select="0"/^> >> htmlhelp.xsl
echo  ^<xsl:param name="toc.section.depth" select="4"/^> >> htmlhelp.xsl
echo  ^<xsl:param name="htmlhelp.force.map.and.alias" select="1"/^> >> htmlhelp.xsl
echo  ^<xsl:param name="htmlhelp.show.menu" select="1"/^> >> htmlhelp.xsl
echo  ^<xsl:param name="htmlhelp.show.advanced.search" select="1"/^> >> htmlhelp.xsl
echo ^</xsl:stylesheet^> >> htmlhelp.xsl
