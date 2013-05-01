::@echo off
set ROOT=D:\Development\SVN\SVNTests
set REPOROOT=D:/Development/SVN/SVNTests
set REPONAME=localDeleteIncomingEditUponUpdateRepo
set WCNAME=localDeleteIncomingEditUponUpdateWC
set REPO=svn://localhost:40000/
set WC=%ROOT%\%WCNAME%

set SVNCLI=D:\Development\SVN\TortoiseSVN\bin\debug\bin\svn.exe
set SVNADM=D:\Development\SVN\TortoiseSVN\bin\debug\bin\svnadmin.exe
::set SVNCLI=svn.exe
::set SVNADM=svnadmin.exe

cd %ROOT%
if exist %REPONAME% rd /s /q %REPONAME%
if exist %WCNAME%1  rd /s /q %WCNAME%1
if exist %WCNAME%2  rd /s /q %WCNAME%2

mkdir %ROOT%\%REPONAME%
svnadmin create %REPOROOT%\%REPONAME%

echo [general]> %REPOROOT%\%REPONAME%\conf\svnserve.conf
echo anon-access = write>> %REPOROOT%\%REPONAME%\conf\svnserve.conf
rem Launch svnserve for current directory at port 40000
start svnserve.exe --daemon --foreground --root %REPOROOT%\%REPONAME% --listen-port 40000 --listen-host localhost


%SVNCLI% co %REPO% %WC%1

mkdir %WC%1\foofolder
%SVNCLI% add %WC%1\foofolder
echo testline1 > %WC%1\foo.c
echo testline1 > %WC%1\foo2.c
%SVNCLI% add %WC%1\foo.c
%SVNCLI% add %WC%1\foo2.c
%SVNCLI% ci %WC%1 -m ""


%SVNCLI% co %REPO% %WC%2

:: user 1 edits file
echo testlinemodified > %WC%1\foo.c
echo testlinemodified > %WC%1\foo2.c
:: user 1 edits folder
%SVNCLI% propset testprop testvalue %WC%1\foofolder
%SVNCLI% ci %WC%1 -m ""

:: user 2 moves the same file
%SVNCLI% mv %WC%2\foo.c %WC%2\bar.c
%SVNCLI% rm %WC%2\foo2.c
%SVNCLI% mv %WC%2\foofolder %WC%2\barfolder

:: tree conflict on user 2 update
%SVNCLI% up %WC%2
%SVNCLI% st %WC%2
