noinst_LIBRARIES = liblogcacheaccess.a
liblogcacheaccess_a_SOURCES = \
	CopyFollowingLogIterator.cpp\
	CSVWriter.cpp\
	LogIteratorBase.cpp\
	StrictLogIterator.cpp\
	XMLLogReader.cpp\
	XMLLogWriter.cpp

AM_CPPFLAGS = \
	-I$(srcdir)/../../Utils\
	-I$(srcdir)/../../UtilsProject\
	-I$(srcdir)/../../CompatibilityHeaders

# set the include path found by configure
INCLUDES= $(all_includes)
