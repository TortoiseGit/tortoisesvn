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
source	- contains the XML text source and localized images for each language.
images	- contains the base (english) images for the docs. If you don't localize 
	  your screenshots, these will be used instead.
tools	- contains the tools and the dtd to validate and build the docs.
	  You might want to place your tools directory somewhere else on your 
	  harddisk, if you want to use it to build other docs too. This will 
	  however require tweaking the build scripts.
	  I'd recommend to leave tools\dtd in place, so the source stays 
	  compatible between TSVN doc developers.

source/en - contains the english xml sources.
source/de - contains the german xml sources. 
source/de/images - would contain localized screenshots

The scripts:
============
Two scripts are provided to build the documentation
gendoc.bat 	   - will loop over all known applications and every directory 
		     it finds inside source (so don't place any nonsense there).
source\makedoc.bat - is called from gendoc.bat and will generate html, pdf and chm 
		     docs for the given application and language.
		     makedoc.bat takes three or more parameters where
		     %1 = Target application to build docs for
		     %2 = Target language (as in source/en)
		     %3..%5 = any of [pdf chm html]

"makedoc tortoisesvn de pdf"      will build the pdf docs in directory source/de  
                                  (german) for tortoisesvn. Simple.
"makedoc tortoisesvn en chm html" will build the chm and html docs in directory 
                                  source/en (english)

Translate:
==========
If you want to translate the docs into a new language (assume french), just go ahead and copy the 
entire directory e.g. source/en to source/fr. Now open tortoisesvn.xml and change the line
<book id="tsvn" lang="en"> to <book id="tsvn" lang="fr"> and you're almost done :-)

now you can call "makedoc tortoisesvn fr" and you will see, that the navigation links are
already translated. This is done automatically by the xsl transformation.

Now go ahead and translate the rest.

Cheers
-Lübbe