:: show properties for b1, adjust external to HEAD should return r2
:: show properties for b2, adjust external to HEAD should return r5
svnadmin create repo
svn checkout "file:///%CD%/repo" wc
pushd wc

mkdir branches
mkdir trunk
svn add trunk
svn add branches
svn ci . -m ""

echo "File 1" > trunk\file1.txt
svn add trunk\file1.txt
svn ci . -m ""

svn cp trunk branches/b1
svn ci . -m ""
svn up

svn propset svn:externals "../../trunk/file1.txt extfile.txt"  branches/b1
svn ci . -m ""
svn up

svn cp trunk branches/b2
svn ci . -m ""
svn up

svn propset svn:externals "../b2/file1.txt extfile.txt"  branches/b2
svn ci . -m ""
svn up

svn log -v branches/b1/extfile.txt
svn log -v branches/b2/extfile.txt
popd
