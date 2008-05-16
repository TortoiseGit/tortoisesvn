version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

lib_LTLIBRARIES = libhunspell-1.1.la
libhunspell_1_1_includedir = $(includedir)/hunspell
libhunspell_1_1_la_SOURCES=affentry.cxx affixmgr.cxx csutil.cxx \
		     dictmgr.cxx hashmgr.cxx hunspell.cxx utf_info.cxx \
	             suggestmgr.cxx license.myspell license.hunspell

libhunspell_1_1_include_HEADERS=affentry.hxx htypes.hxx affixmgr.hxx \
	        csutil.hxx hunspell.hxx atypes.hxx dictmgr.hxx hunspell.h \
		suggestmgr.hxx baseaffix.hxx hashmgr.hxx langnum.hxx

EXTRA_DIST=hunspell.dsp makefile.mk README
