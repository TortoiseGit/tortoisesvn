set TSVNBUILD=release
candle -nologo -out Setup.wixobj Setup.wxs
light -nologo -out ..\..\bin\TortoiseSVN-1.1.x-UNICODE_svn-1.1.x.msi Setup.wixobj
set TSVNBUILD=release_mbcs
candle -nologo -out Setup.wixobj Setup.wxs
light -nologo -out ..\..\bin\TortoiseSVN-1.1.x-MBCS_svn-1.1.x.msi Setup.wixobj
del Setup.wixobj