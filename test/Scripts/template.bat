@echo off

set ROOTDIR=%cd%
set ROOTDIR=%ROOTDIR:\=/%
set REPOS=sparserepo
set REPOS_PATH=file:///%ROOTDIR%/%REPOS%
set WC=%ROOTDIR%/%REPOS%-wc
set SVN=D:\Development\SVN\TortoiseSVN\bin\debug64\bin\svn.exe
set SVNADMIN=D:\Development\SVN\TortoiseSVN\bin\debug64\bin\svnadmin.exe

if exist "%REPOS%" rmdir /s /q "%REPOS%"
if exist "%WC%"    rmdir /s /q "%WC%"

"%SVNADMIN%" create "%REPOS%"

"%SVN%" co "%REPOS_PATH%" "%WC%"
echo file1 > "%WC%"/File1.txt
mkdir "%WC%"/folder1
mkdir "%WC%"/folder2
"%SVN%" add "%WC%"/File1.txt
"%SVN%" add "%WC%"/folder1
"%SVN%" add "%WC%"/folder2
"%SVN%" ci -m "" "%WC%"

"%SVN%" up --set-depth exclude "%WC%"/folder1
"%SVN%" up --set-depth exclude "%WC%"/File1.txt

"%SVN%" st -v "%WC%"

echo file1 > "%WC%"/File1.txt
mkdir "%WC%"/folder1

"%SVN%" st -v "%WC%"

pause
