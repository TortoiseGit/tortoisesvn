@echo off
echo ^<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"^> > ..\xsl\db_htmlchunk.xsl
echo ^<xsl:import href="%XSLROOT%/html/chunk.xsl"/^> >> ..\xsl\db_htmlchunk.xsl
echo ^</xsl:stylesheet^> >> ..\xsl\db_htmlchunk.xsl
