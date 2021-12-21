<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:import href="./db_htmlchunk.xsl"/>
<xsl:param name="keep.relative.image.uris" select="0"/>
<xsl:param name="chunker.output.encoding" select="'UTF-8'"/>

<xsl:template name="user.header.navigation">
 <center><p><a href="https://tortoisesvn.net/support.html">Manuals</a></p></center>
</xsl:template>

<xsl:template name="user.footer.navigation">
 <center><p><a href="https://tortoisesvn.net">TortoiseSVN homepage</a></p></center>
</xsl:template>


</xsl:stylesheet>
