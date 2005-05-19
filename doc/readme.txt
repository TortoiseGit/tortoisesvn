HowTo build the docs
====================

Since you are already reading this, I assume that you have succeeded in checking 
out the TortoiseSVN or just the doc sources.

The docs are self contained which means that they contain (almost) everything you need 
(source, dtd, formatting processors, scripts) to build them from scratch.
Building the context sensitive chm help requires some tools from VS.Net to grab the
Help IDs from the source. These tools are not included. You can still build the help 
file to see what it looks like, but pressing F1 in TSVN will not "help" you :-)

Structure:
==========
The most important directories for you are:
source\en - contains the english XML text source.
images\en - contains the base (english) images for the docs. If you don't localize 
	    your screenshots, these will be used instead.
images\*  - contain the localized screenshots for other languages.
po\*      - contain the translations for each language. Best edited with poEdit.
tools     - contains the tools and the dtd to validate and build the docs.
	    You might want to place your tools directory somewhere else on your 
            harddisk, if you want to use it to build other docs too. This will 
            however require tweaking the build scripts.
            I'd recommend to leave tools\dtd in place, so the source stays 
            compatible between TSVN doc developers.

The scripts:
============
Three scripts are provided to build the documentation:

TranslateDoc.bat:
  will translate into the given language if the po file exists.
  If no parameter is given, all .po files found in doc\po will be
  used to create the corresponding target languages.
  This script uses xml2po.py and requires a Python runtime
  environment to be installed.
  This script makes use of gnu "aspell". Windows binaries and 
  dictionaries are found at ftp://ftp.gnu.org/gnu/aspell/w32/
  A log is written to "translatelog.txt"

Example: "TranslateDoc de" will use de.po and the English xml files to
  build the German document stucture in source\de

GenDoc.bat:
  will loop over all known applications and build the docs in
  English plus all .po files it finds inside "doc\po" 
  (so don't place any nonsense or backup .po files there)
  This script uses xml2po.py and requires a Python runtime
  environment to be installed.
  The script retranslates all source files while building, so there's
  no need to run "TranslateDoc" before "GenDoc". "GenDoc" doesn't
  run the spellchecker on the source.

source\MakeDoc.bat:
  is called from GenDoc.bat and will generate html, pdf and chm 
  docs for the given application and language.
  makedoc.bat takes three or more parameters where
  %1 = Target application to build docs for
  %2 = Target language (as in source/en)
  %3..%5 = any of [pdf chm html]

  "makedoc tortoisesvn de pdf"      will build the pdf docs in directory source/de  
                                    (German) for tortoisesvn. Simple.
  "makedoc tortoisesvn en chm html" will build the chm and html docs in directory 
                                    source/en (English)

Translate:
==========
If you want to translate the docs into a new language (assume french), just go ahead
and copy po\doc.pot to po\fr.po. Start to translate phrases and test your translation
with "TranslateDoc fr" and "makedoc tortoisesvn fr".

Place localized screenshots in images\fr. We document which images need updating in
ImageStatus.txt. Just add another column for french.

Now go ahead and translate the rest.

Cheers
-Lübbe