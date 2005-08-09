echo MakeMsiSub.bat: candle (WiX compilation)
candle -nologo -out ..\..\bin\ TortoiseSVN.wxs FeaturesFragment.wxs StructureFragment.wxs UIFragment.wxs

echo MakeMsiSub.bat: light (WiX linking)
light -out ..\..\bin\TortoiseSVN-1.2.1.$WCREV$-dev-svn-1.2.1.msi ..\..\bin\TortoiseSVN.wixobj ..\..\bin\FeaturesFragment.wixobj ..\..\bin\StructureFragment.wixobj ..\..\bin\UIFragment.wixobj

echo MakeMsiSub.bat: Cleaning up
cd ..\..\bin
del *.wixobj

echo MakeMsiSub.bat: md5
md5 -omd5.txt TortoiseSVN-1.2.1.$WCREV$-dev-svn-1.2.1.msi
cd ..\src\TortoiseSVNSetup

echo MakeMsiSub.bat: Done