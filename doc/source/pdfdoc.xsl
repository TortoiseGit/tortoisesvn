<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:fo="http://www.w3.org/1999/XSL/Format"
  version="1.0">

  <xsl:import href="../tools/xsl/fo/docbook.xsl"/>
  <xsl:import href="./defaults.xsl"/>
  <xsl:param name="paper.type" select="'A4'"></xsl:param>
  <xsl:param name="double.sided" select="1"></xsl:param>
  <xsl:param name="variablelist.as.blocks" select="1"></xsl:param>
  <xsl:param name="symbol.font.family" select="'Symbol,ZapfDingbats'"></xsl:param>

  <xsl:param name="formal.title.placement">
	figure after
	example after
	equation after
	table after
	procedure after
  </xsl:param>

<xsl:template match="menuchoice">
  <fo:inline font-family="Helvetica">
    <xsl:call-template name="process.menuchoice"/>
  </fo:inline>
</xsl:template>

<xsl:template match="guilabel">
  <fo:inline font-family="Helvetica">
    <xsl:call-template name="inline.charseq"/>
  </fo:inline>
</xsl:template>

<xsl:template match="guibutton">
  <fo:inline font-family="Helvetica">
    <xsl:call-template name="inline.charseq"/>
  </fo:inline>
</xsl:template>

<xsl:template match="keysym">
  <fo:inline font-family="Symbol">
    <xsl:call-template name="inline.charseq"/>
  </fo:inline>
</xsl:template>

</xsl:stylesheet>
