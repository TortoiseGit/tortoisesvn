@echo off
echo ^<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"^> > db_pdfdoc.xsl
echo ^<xsl:import href="%XSLROOT%/fo/docbook.xsl"/^> >> db_pdfdoc.xsl
echo ^</xsl:stylesheet^> >> db_pdfdoc.xsl