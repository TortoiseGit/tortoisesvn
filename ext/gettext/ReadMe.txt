This directory contains the pre-compiled GNU gettext libintl package for
windows. You can get it from here:
http://prdownloads.sourceforge.net/gnuwin32/gettext-0.14.1-src.zip

We patched it in one line in intl-w32\iconvshim.c, line 36:
- static char * f_defaults[] = { "iconv.dll", "libiconv.dll" };
+ static char * f_defaults[] = { "libapriconv.dll", "iconv.dll", "libiconv.dll" };

so it uses the apr-iconv library instead of the GNU iconv library. We did this
because we already need apr-iconv for Subversion, and there's no reason to
have a separate library to ship which does exactly the same as apr-iconv.

Also, since Windows doesn't know the locale LC_MESSAGES a patch was needed
to get rid of the "LC_XXX" part of the gettext search path. The "LC_XXX" was
replaced with an empty string ("").