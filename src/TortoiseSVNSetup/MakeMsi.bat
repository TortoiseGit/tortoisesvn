set TSVNBUILD=release
candle -out Setup.wixobj Setup.wxs
light -out ..\..\bin\TortoiseSVN-1.0.x-UNICODE_svn-1.x.x.msi Setup.wixobj
set TSVNBUILD=release_mbcs
candle -out Setup.wixobj Setup.wxs
light -out ..\..\bin\TortoiseSVN-1.0.x-MBCS_svn-1.x.x.msi Setup.wixobj
del Setup.wixobj