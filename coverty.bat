"D:\Development\cov-analysis-win64-6.0.5\bin\cov-build.exe" --dir cov-int TSVN-covbuild.bat
"%PROGRAMFILES%\7-zip\7z.exe" a -ttar TortoiseSVN.tar cov-int
"%PROGRAMFILES%\7-zip\7z.exe" a -tgzip TortoiseSVN.tgz TortoiseSVN.tar
del TortoiseSVN.tar
