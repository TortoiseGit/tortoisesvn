@echo off
:: There is no easy way to avoid using an absolute path here.
:: It is used for a file:/// URL, so must have forward slashes.
:: Sorry, but you will have to edit it.
set doctest=c:/TortoiseSVN/doc/test

if exist repos rd /s/q repos
if exist doc rd /s/q doc
if exist docs rd /s/q docs
md repos
md doc
md docs
svnadmin create repos --fs-type fsfs
svn co -q file:///%doctest%/repos doc
svn co -q file:///%doctest%/repos docs
cd doc
:: Copy some files from the docs to create content
for %%f in (add blame checkout commit export ignore relocate revert showlog) do (
	@copy ..\..\source\en\TortoiseSVN\tsvn_dug\dug_%%f.xml > nul
	@svn add dug_%%f.xml
	@svn ci -q -m "Document the %%f command" .
)
copy ..\subwcrev1.txt subwcrev.txt > nul
svn add subwcrev.txt
svn ci -q -m "Document the SubWCRev program" .
svn up -q ../docs
:: Change detection is broken when timestamps are the same.
:: Force a current timestamp by using type instead of copy.
type ..\subwcrev2.txt > subwcrev.txt
svn ci -q -m "Clarify the description of SubWCRev" .

cd ../docs
copy /y ..\subwcrev3.txt subwcrev.txt > nul
svn up -q

cd ..
set doctest=
