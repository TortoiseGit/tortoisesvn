HowTo build the docs
====================

Since you are already reading this, I assume that you have succeeded in checking 
out the TortoiseSVN or just the doc sources.

Tools needed:
=============

There are some tools for processing the XML input that you need to build the docs.
Scripts and dtd are included, but the executables (formatting processor, microsoft
help compiler, translation tools) have to be installed separately.

tools\fop\		- the fop processor
tools\xsl\		- the docbook xsl files from sourceforge
tools\			- xsl processor, hhc.exe, ...

you can download all the required tools as a zip package from our website:
http://tortoisesvn.tigris.org/servlets/ProjectDocumentList?folderID=616

Currently you can either build the docs using batch files (which we'll remove in the near future)
and NAnt.

Please note that having spaces in your directory path will (for the time being)
cause the documentation build process to fail.

To build only the english docs, that's all you need. If you want to build translated
docs as well, you need:
- A Python runtime environment
  (http://www.python.org)
- The Python libxml2 bindings
  (http://users.skynet.be/sbi/libxml-python/)
- Aspell (optional) for spell checking translations
  (ftp://ftp.gnu.org/gnu/aspell/w32/)


Structure:
==========
The most important directories for you are:
source\en - contains the english XML text source.
images\en - contains the base (english) images for the docs. If you don't localize 
	    your screenshots, these will be used instead.
images\*  - contains the localized screenshots for other languages.
po\*      - contains the translations for each language. Best edited with poEdit.
xsl\	  - contains the stylesheets for the doc creation
dtd\      - contains the tools and the dtd to validate and build the docs.
	    You might want to place your tools directory somewhere else on your 
            harddisk, if you want to use it to build other docs too. This will 
            however require tweaking the build scripts.
            I'd recommend to leave dtd in place, so the source stays 
            compatible between TSVN doc developers.

Building the docs:
==================

NAnt Build:
-----------
A NAnt build script has been provided to build the docs. When doc.build is run for
the first time, the template doc.build.include.template is copied to doc.build.include.
Adjust the settings for your build environment in doc.build.include, not in the template.
If you want to build the Japanese docs, you have to copy xsl\ja\userconfig.template.xml
to xsl\ja\userconfig.xml and adjust the path settings to the japanese fonts.

Currently nant supports three targets "all", "clean" and "potfile".
- "all" builds all the docs that are out of date. Runs the target "potfile" too.
- "clean" cleans only the output directory
- "potfile" updates the po template from the source document.

All other parameters are defined in doc.build.include.

If the NAnt build process has been tested enough, the batch scripts will be removed

Batch scripts:
--------------
Make a copy of the file TortoiseVars.tmpl in the TSVN root folder and
rename that copy to TortoiseVars.bat. Then simply adjust the paths as mentioned
in that file.

Three batch scripts are provided to build the documentation:

TranslateDoc.bat:
  will translate into the given language if the po file exists.
  If no parameter is given, all .po files found in doc\po will be
  used to create the corresponding target languages.
  This script requires python, libxml2 and aspell.
  A log is written to "translatelog.txt"

Example: "TranslateDoc de" will use de.po and the English xml files to
  build the German document stucture in source\de

GenDoc.bat:
  will loop over all known applications and build the docs in
  English plus all .po files it finds inside "doc\po" 
  (so don't place any nonsense or backup .po files there)
  This script requires python and libxml2
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