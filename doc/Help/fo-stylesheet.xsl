<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version='1.0'>

  <xsl:import href="../tools/xsl/fo/docbook.xsl"/>

  <xsl:param name="fop.extensions" select="0" />
  <xsl:param name="paper.type" select="'A4'"></xsl:param>
  <xsl:attribute-set name="admonition.properties">
    <xsl:attribute name="border-top">5px solid</xsl:attribute>
    <xsl:attribute name="border-left">1px solid</xsl:attribute>
    <xsl:attribute name="background-repeat">no-repeat</xsl:attribute>
    <xsl:attribute name="background-position">5px 1.33em</xsl:attribute>
    <xsl:attribute name="background-image">
      <xsl:param name="node" select="."/>
      <xsl:choose>
        <xsl:when test="name($node)='note'">url(icon_success_lrg.png)</xsl:when>
        <xsl:when test="name($node)='warning'">url(icon_warning_lrg.png)</xsl:when>
        <xsl:when test="name($node)='caution'">url(icon_warning_lrg.png)</xsl:when>
        <xsl:when test="name($node)='tip'">url(icon_info_lrg.png)</xsl:when>
        <xsl:when test="name($node)='important'">url(icon_error_lrg.png)</xsl:when>
        <xsl:otherwise>url(icon_success_lrg.png)</xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>
    <xsl:attribute name="border-color">
      <xsl:param name="node" select="."/>
      <xsl:choose>
        <xsl:when test="name($node)='note'">#090</xsl:when>
        <xsl:when test="name($node)='warning'">#c60</xsl:when>
        <xsl:when test="name($node)='caution'">#c60</xsl:when>
        <xsl:when test="name($node)='tip'">#069</xsl:when>
        <xsl:when test="name($node)='important'">#900</xsl:when>
        <xsl:otherwise>#090</xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>

    <xsl:attribute name="margin">.67em 0 0 1em</xsl:attribute>
    <xsl:attribute name="padding">.33em 0 .67em 42px</xsl:attribute>
    <xsl:attribute name="min-height">52px</xsl:attribute>
    <xsl:attribute name="width">80%</xsl:attribute>
    <xsl:attribute name="clear">both</xsl:attribute>
  </xsl:attribute-set>
</xsl:stylesheet>
