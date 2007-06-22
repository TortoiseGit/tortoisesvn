@echo off
:: There is no easy way to avoid using an absolute path here.
:: It is used for a file:/// URL, so must have forward slashes.
:: To avoid editing a versioned file you should create a text file
:: called DocPath.txt which contains the path to the TSVN doc folder.
:: It must use forward slashes and URL escaping if necessary - no spaces!
if not exist DocPath.txt echo c:/TortoiseSVN/doc > DocPath.txt
for /f %%p in (DocPath.txt) do @set docpath=%%p

if exist temp rd /s/q temp
md temp
cd temp
md repos
md doc
md docs
svnadmin create repos --fs-type fsfs
svn co -q file:///%docpath%/test/temp/repos doc
svn co -q file:///%docpath%/test/temp/repos docs
cd doc
:: Copy some files from the docs to create content
for %%f in (add blame checkout commit export ignore relocate revert showlog) do (
	@copy ..\..\..\source\en\TortoiseSVN\tsvn_dug\dug_%%f.xml > nul
	@svn add dug_%%f.xml
	@svn ci -q -m "Document the %%f command" .
)
copy ..\..\subwcrev1.txt subwcrev.txt > nul
svn add subwcrev.txt
svn ci -q -m "Document the SubWCRev program" .
svn up -q ../docs
:: Change detection is broken when timestamps are the same.
:: Force a current timestamp by using type instead of copy.
type ..\..\subwcrev2.txt > subwcrev.txt
svn ci -q -m "Clarify the description of SubWCRev" .

cd ../docs
copy /y ..\..\subwcrev3.txt subwcrev.txt > nul
svn up -q

cd ..\..
set docpath=
