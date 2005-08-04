echo WiX setup building - candle
candle -nologo -out ..\..\bin\ TortoiseSVN.wxs FeaturesFragment.wxs StructureFragment.wxs UIFragment.wxs

echo WiX setup building - light
light -out ..\..\bin\TortoiseSVN-1.2.1.$WCREV$-dev-svn-1.2.1.msi ..\..\bin\TortoiseSVN.wixobj ..\..\bin\FeaturesFragment.wixobj ..\..\bin\StructureFragment.wixobj ..\..\bin\UIFragment.wixobj

cd ..\..\bin
del *.wixobj
md5 -omd5.txt TortoiseSVN-1.2.1.$WCREV$-dev-svn-1.2.1.msi
cd ..\src\TortoiseSVNSetup