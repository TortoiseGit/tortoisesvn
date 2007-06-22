@echo off
:: There is no easy way to avoid using an absolute path here.
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
md repos2
md ext
svnadmin create repos --fs-type fsfs
svnadmin create repos2 --fs-type fsfs
svn co -q file:///%docpath%/test/temp/repos doc
if errorlevel 1 goto checkout_fail
svn co -q file:///%docpath%/test/temp/repos docs
svn co -q file:///%docpath%/test/temp/repos2 ext

:: This is used to add content to the 'external' repository.
cd ext
:: Copy some files from the docs to create content
copy ..\..\..\source\en\TortoiseSVN\tsvn_server\server*.xml > nul
@svn add server*.xml
@svn ci -q -m "Document the server" .
cd ..
rd /s/q ext

cd doc
:: Copy some files from the docs to create content
for %%f in (add blame checkout commit export ignore relocate revert showlog) do (
	@copy ..\..\..\source\en\TortoiseSVN\tsvn_dug\dug_%%f.xml > nul
	@svn add dug_%%f.xml
	@svn ci -q -m "Document the %%f command" .
)

:: Add a reference to the external repository
svn ps -q svn:externals "ext file:///%docpath%/test/temp/repos2/" .
svn ci -q -m "create externals reference" .
svn up -q

copy ..\..\subwcrev1.txt subwcrev.txt > nul
svn add subwcrev.txt
svn ci -q -m "Document the SubWCRev program" .
svn up -q ../docs
:: Change detection is broken when timestamps are the same.
:: Force a current timestamp by using type instead of copy.
type ..\..\subwcrev2.txt > subwcrev.txt
svn ci -q -m "Clarify the description of SubWCRev" .
:: Add an unversioned file
echo Read Me > readme.txt

cd ..\docs
copy /y ..\..\subwcrev3.txt subwcrev.txt > nul
svn up -q

cd ..
goto end
:checkout_fail
echo Perhaps you have not set the correct unix-style path in DocPath.txt
:end
cd ..
set docpath=
