HowTo build the docs
====================

Since you are already reading this, I assume that you have succeeded in checking
out the TortoiseSVN or just the doc sources.

Tools needed:
=============

There are some tools for processing the XML input that you need to build the docs.
Scripts and dtd are included, but the executables (formatting processor, Microsoft
help compiler, translation tools) have to be installed separately.
You will also need to have a Java Runtime Environment version 1.3.x or above.

tools\fop\      - the fop processor
tools\xsl\      - the docbook xsl files from sourceforge
tools\          - xsl processor, hhc.exe...

You can download all the required tools as a 7zip package from our website:
https://sourceforge.net/projects/tortoisesvn/files/build%20tools/

The file is Tools-1.x.7z

Currently you can build the docs using NAnt. Download a current release from:
http://nant.sourceforge.net
Note: NAnt requires the full .NET framework installed, not just the client profile.

Please note that having spaces in your directory path will (for the time being)
cause the documentation build process to fail.

To build only the English docs, that's all you need. If you want to build translated
docs as well, you need:
- A Python runtime environment
  (http://www.python.org)
- The Python libxml2 bindings
  (http://users.skynet.be/sbi/libxml-python/)
- Aspell (optional) for spell checking translations
  (ftp://ftp.gnu.org/gnu/aspell/w32/)

For Chm docs you need:
- Microsoft's makehm.exe, part of visual studio, sources available on MSDN
- Microsoft's html workshop, binaries available on MSDN


Structure:
==========
The most important directories for you are:
source\en - contains the English XML text source.
images\en - contains the base (English) images for the docs. If you don't localize
            your screenshots, these will be used instead.
images\*  - contains the localized screenshots for other languages.
po\*      - contains the translations for each language. Best edited with poEdit.
xsl\      - contains the stylesheets for the doc creation
dtd\      - contains the tools and the dtd to validate and build the docs.
            You might want to place your tools directory somewhere else on your
            harddisk, if you want to use it to build other docs too. This will
            however require tweaking the build scripts.
            I'd recommend to leave dtd in place, so the source stays
            compatible between TortoiseSVN doc developers.

Building the docs:
==================

NAnt Build:
-----------
A NAnt build script has been provided to build the docs. When doc.build is run for
the first time, the template doc.build.include.template is copied to doc.build.include.

For local customizations, copy the doc.build.user.tmpl file to doc.build.user and
modify that copy to point to the location of the tools on your system.

If you want to build the Japanese docs, you have to copy xsl\ja\userconfig.template.xml
to xsl\ja\userconfig.xml and adjust the path settings to the Japanese fonts.

Currently nant supports three targets "all", "clean" and "potfile".
- "all" builds all the docs that are out of date. Runs the target "potfile" too.
- "clean" cleans only the output directory
- "potfile" updates the po template from the source document.

All other parameters are defined in doc.build.include. You can override all settings
in doc.build.user or via the NAnt command line.

Translate:
==========
If you want to translate the docs into a new language (assume French), just go ahead
and copy po\doc.pot to po\fr.po. Start to translate phrases and test your translation
with "TranslateDoc fr" and "makedoc tortoisesvn fr".

Place localized screenshots in images\fr. The file "/doc/notes/screenshots.txt"
contains some instructions on how/where the images have been captured.

Now go ahead and translate the rest.

Cheers
-Lübbe
