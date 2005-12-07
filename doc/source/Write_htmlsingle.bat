@echo off
echo ^<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"^> > ..\xsl\db_htmlsingle.xsl
echo ^<xsl:import href="%XSLROOT%/html/docbook.xsl"/^> >> ..\xsl\db_htmlsingle.xsl
echo ^</xsl:stylesheet^> >> ..\xsl\db_htmlsingle.xsl
