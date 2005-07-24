@echo off
echo ^<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"^> > db_htmlhelp.xsl
echo ^<xsl:import href="%XSLROOT%/htmlhelp/htmlhelp.xsl"/^> >> db_htmlhelp.xsl
echo ^</xsl:stylesheet^> >> db_htmlhelp.xsl
