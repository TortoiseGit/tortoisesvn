::@echo off
set ROOT=D:\Development\SVN\SVNTests
set REPOROOT=D:/Development/SVN/SVNTests
set REPONAME=tcLocalDeleteIncomingEditRepo
set WCNAME=tcLocalDeleteIncomingEditWC
set REPO=svn://localhost:40000/
set WC=%ROOT%\%WCNAME%

set SVNCLI=D:\Development\SVN\TortoiseSVN\bin\debug\bin\svn.exe
set SVNADM=D:\Development\SVN\TortoiseSVN\bin\debug\bin\svnadmin.exe
::set SVNCLI=svn.exe
::set SVNADM=svnadmin.exe

cd %ROOT%
if exist %REPOROOT%/%REPONAME% rd /s /q %REPOROOT%/%REPONAME%
if exist %WCNAME% rd /s /q %WCNAME%

mkdir %REPOROOT%\%REPONAME%
svnadmin create %REPOROOT%\%REPONAME%

echo [general]> %REPOROOT%\%REPONAME%\conf\svnserve.conf
echo anon-access = write>> %REPOROOT%\%REPONAME%\conf\svnserve.conf
rem Launch svnserve for current directory at port 40000
start svnserve.exe --daemon --foreground --root %REPOROOT%\%REPONAME% --listen-port 40000 --listen-host localhost


%SVNCLI% co %REPO% %WC%1

echo testline1 > %WC%1\foo.c
%SVNCLI% add %WC%1\foo.c
%SVNCLI% ci %WC%1 -m ""


%SVNCLI% co %REPO% %WC%2

:: user 1 edits file
echo testlinemodified > %WC%1\foo.c
%SVNCLI% ci %WC%1 -m ""

:: user 2 moves the same file
%SVNCLI% mv %WC%2\foo.c %WC%2\bar.c

:: tree conflict on user 2 update
%SVNCLI% up %WC%2
%SVNCLI% st %WC%2

