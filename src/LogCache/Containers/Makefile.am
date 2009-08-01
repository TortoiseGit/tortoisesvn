noinst_LIBRARIES = liblogcachecontainers.a
liblogcachecontainers_a_SOURCES = \
	BlobDictionary.cpp\
	DictionaryBasedTempPath.cpp\
	IndexPairDictionary.cpp\
	PathDictionary.cpp\
	StringDictonary.cpp\
	TokenizedStringContainer.cpp\
	ContainerException.cpp \
	RevisionIndex.cpp\
	RevisionInfoContainer.cpp\
	SkipRevisionInfo.cpp\
	CachedLogInfo.cpp

AM_CPPFLAGS = \
	-I$(srcdir)/../../Utils\
	-I$(srcdir)/../../CompatibilityHeaders

# set the include path found by configure
INCLUDES= $(all_includes)
