@echo OFF

SET ROOTDIR=%cd%
SET ROOTDIR=%ROOTDIR:\=/%
SET REPOS=propconflictupdate
SET REPOS_PATH=file:///%ROOTDIR%/%REPOS%
SET WC=%ROOTDIR%/%REPOS%-wc
set SVN=D:\Development\SVN\TortoiseSVN\bin\debug64\bin\svn.exe
set SVNADMIN=D:\Development\SVN\TortoiseSVN\bin\debug64\bin\svnadmin.exe

IF EXIST "%REPOS%" rmdir /s /q "%REPOS%"
IF EXIST "%WC%" rmdir /s /q "%WC%"

"%SVNADMIN%" create "%REPOS%"
"%SVN%" mkdir -m "" "%REPOS_PATH%/trunk" 
"%SVN%" mkdir -m "" "%REPOS_PATH%/branches" 


"%SVN%" co "%REPOS_PATH%/trunk" "%WC%1"

echo file0 > "%WC%1"\File1.txt
"%SVN%" add "%WC%1"\File1.txt
"%SVN%" ci -m "" "%WC%1" 

"%SVN%" copy -m "" "%REPOS_PATH%/trunk" "%REPOS_PATH%/branches/b1" 

"%SVN%" propset s:name val1 "%WC%1/File1.txt"
echo file1 > "%WC%1"\File1.txt
"%SVN%" ci -m "" "%WC%1"

"%SVN%" co "%REPOS_PATH%/branches/b1" "%WC%2"

"%SVN%" propset s:name val2 "%WC%1/File1.txt"
echo file2 > "%WC%1"\File1.txt
"%SVN%" ci -m "" "%WC%1"

"%SVN%" propset s:name val3 "%WC%2/File1.txt"
echo file3 > "%WC%2"\File1.txt
"%SVN%" ci -m "" "%WC%2"

"%SVN%" propset s:name val4 "%WC%2/File1.txt"
echo file4 > "%WC%2"\File1.txt

"%SVN%" up "%WC%2"

pause

"%SVN%" merge "%REPOS_PATH%/trunk"  "%WC%2"
