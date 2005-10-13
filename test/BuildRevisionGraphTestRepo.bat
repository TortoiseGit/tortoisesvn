rem @echo off
svnadmin create repo --fs-type fsfs
svn co file:///d:/test/repo wc1
cd wc1
svn mkdir trunk
svn mkdir branches
svn mkdir tags
svn ci -m "create repo layout"
cd trunk
echo testline1 > file1
svn add file1
echo testline1 > file2
svn add file2
echo testline1 > file3
svn add file3
echo testline1 > file4
svn add file4
svn mkdir folder
echo testline1 > folder\file1
svn add folder\file1
echo testline1 > folder\file2
svn add folder\file2
echo testline1 > folder\file3
svn add folder\file3
echo testline1 > folder\file4
svn add folder\file4
svn ci -m "add some files and a folder with files"

svn cp file:///d:/test/repo/trunk file:///d:/test/repo/branches/TestBranch1 -m "create test branch"
svn up

echo testline > file1
svn ci -m "file modified"

svn ren folder Ordner
svn ci -m "rename a folder"

svn cp file:///d:/test/repo/trunk file:///d:/test/repo/tags/V1 -m "Version 1"
svn cp file:///d:/test/repo/tags/V1 file:///d:/test/repo/branches/RelV1 -m "Version 1 Fix branch"

svn cp file:///d:/test/repo/trunk -r2 file:///d:/test/repo/tags/V0 -m "Version 0"

svn up

echo testline2 > file1
svn ci -m ""
echo testline3 > file1
svn ci -m ""
echo testline4 > file1
svn ci -m ""
echo testline5 > file1
svn ci -m ""
echo testline6 > file1
svn ci -m ""
echo testline7 > file1
svn ci -m ""
echo testline8 > file1
svn ci -m ""
echo testline9 > file1
svn ci -m ""
rem revision 16
svn up
svn cp file:///d:/test/repo/trunk -r10 file:///d:/test/repo/tags/V2 -m "Version 2"
svn cp file:///d:/test/repo/trunk -r9 file:///d:/test/repo/tags/V2 -m "Version 1.5"
svn rm file:///d:/test/repo/tags/V2/trunk -m ""
svn cp file:///d:/test/repo/trunk -r9 file:///d:/test/repo/tags/V1.5 -m "Version 1.5"

cd ..
cd ..
