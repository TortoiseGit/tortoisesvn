<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:import href="../tools/xsl/htmlhelp/htmlhelp.xsl"/>
  <xsl:import href="./defaults.xsl"/>

<!--
  <xsl:param name="chunk.first.sections" select="1"/>
-->
  <xsl:param name="suppress.navigation" select="0"/>
  <xsl:param name="toc.section.depth" select="4"/>
  <xsl:param name="htmlhelp.force.map.and.alias" select="1"/>
  <xsl:param name="htmlhelp.show.menu" select="1"/>
  <xsl:param name="htmlhelp.show.advanced.search" select="1"/>
</xsl:stylesheet>
