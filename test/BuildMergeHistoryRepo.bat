@echo off

set REPOROOT=file:///d:/_test_repo
set REPOPATH=d:\_test_repo
set WC=d:\_test_wc

:: create empty repos & working copy

rd /s /q %REPOPATH%
svnadmin create %REPOPATH%

rd /s /q %WC%
svn co %REPOROOT% %WC%

:: r1: initial content

mkdir %WC%\trunk
mkdir %WC%\branches
mkdir %WC%\tags

echo "foo" > %WC%\trunk\foo.txt
echo "bar" > %WC%\trunk\bar.txt
echo "baz" > %WC%\trunk\baz.txt

svn add %WC%\trunk
svn add %WC%\branches
svn add %WC%\tags

svn ci %WC% -m "initial content"

:: r2: create branch

svn cp %WC%\trunk %WC%\branches\b
svn ci %WC%\branches -m "open branch"

:: r3: modify all files on trunk

echo "new foo" > %WC%\trunk\foo.txt
echo "new bar" > %WC%\trunk\bar.txt
echo "new baz" > %WC%\trunk\baz.txt

svn ci %WC% -m "modify all files on trunk"

:: r4: undo some files modifications on trunk

echo "foo" > %WC%\trunk\foo.txt
echo "baz" > %WC%\trunk\baz.txt

svn ci %WC% -m "undo modifications on trunk"

:: r5: cherry-picking merge

svn merge -g -r2:3 %WC%\trunk\foo.txt %WC%\branches\b\foo.txt
svn merge -g -r2:3 %WC%\trunk\bar.txt %WC%\branches\b\bar.txt

svn ci %WC% -m "cherry pick merge"

:: r6: tagging

svn cp %WC%\branches\b %WC%\tags\t
svn ci %WC%\tags -m "tagging current b content"

:: r7: merge r4 and remainder of r3

svn up %WC%
svn merge -g -r2:4 %REPOROOT%/trunk %WC%\branches\b

svn ci %WC% -m "merge r4 and remainder of r3"

:: r8: unmerge r4

svn up %WC%
svn merge -g -r4:3 %REPOROOT%/trunk %WC%\branches\b

svn ci %WC% -m "unmerged r4"


