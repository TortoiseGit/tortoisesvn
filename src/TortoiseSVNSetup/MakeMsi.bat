set TSVNBUILD=release
candle -out Setup.wixobj Setup.wxs
light -out ..\..\bin\TortoiseSVN-1.1.x-UNICODE_svn-1.1.x.msi Setup.wixobj
set TSVNBUILD=release_mbcs
candle -out Setup.wixobj Setup.wxs
light -out ..\..\bin\TortoiseSVN-1.1.x-MBCS_svn-1.1.x.msi Setup.wixobj
del Setup.wixobj