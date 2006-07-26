<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"> 
 
<xsl:import href="../pdfdoc.xsl"/> 

<xsl:param name="body.font.family" select="'Times New Roman'"/> 
<xsl:param name="title.font.family" select="'Arial'"/> 
<xsl:param name="sans.font.family" select="'Arial'"/> 
<xsl:param name="monospace.font.family" select="'Courier New'"/> 

<!-- produce correct back-of-the-book index for non-English-alphabet languages, 
     don't work with xsltproc (tested up to v.2.06.19)
<xsl:include href="file:///C:/works/TSVN/Tools/xsl/fo/autoidx-ng.xsl"/> 
-->

<!-- to use with xml catalog 
<xsl:include href="http://docbook.sourceforge.net/release/xsl/current/fo/autoidx-ng.xsl"/> 
-->

<!-- XEP-specific parameters
<xsl:param name="xep.extensions" select="1"/> 
<xsl:param name="fop.extensions" select="0"/> 
-->

</xsl:stylesheet> 
